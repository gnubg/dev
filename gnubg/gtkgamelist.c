/*
 * Copyright (C) 2004-2019 Jon Kinsey <jonkinsey@gmail.com>
 * Copyright (C) 2004-2018 the AUTHORS
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 * $Id: gtkgamelist.c,v 1.53 2023/01/01 17:48:19 plm Exp $
 */

/*
02/2024: Isaac Keslassy: 
- added an option for the quiz to always ask questions at money play so the score doesn't impact the answer.
- changed the window title during the quiz

03/2023: Isaac Keslassy: new Quiz feature:
- we start it with the quiz console
- see explanations in the quiz console: 
    * we can either launch the console through the menu bar, or through a right-click 
    menu in the gamelist; 
    * it has a configurable auto-add feature;
    * and so on.
- behind the scenes, we analyze positions in three different settings for the quiz:
    1) In quiz mode, when the user answers something, we display the result in the 
    hint window, and record the result. We use different functions for different 
    types of decisions. For instance, for a double decision, we use:
    hint_double	() -> find_skills() -> double_skill() -> qUpdate(), which updates
    the user's error with this double position
    2) When analyzing a move/game/match and automatically adding positions, we use:
    AnalyzeMove() to do so
    3) When we right-click on a position and want to add it to some arbitrary position
    category, we need the error for the position. We do so in double_skill as in the 
    first case above.
Thanks to Wayne Joseph for the feedback.
*/


#include "config.h"
#include "gtklocdefs.h"

#include <gtk/gtk.h>
#include <glib.h>
#include <stdlib.h>
#include <string.h>
// #include <stdio.h> 
#include <ctype.h> /* for toupper function*/

#include "backgammon.h"
#include "gtkboard.h"
#include "drawboard.h"
#include "positionid.h"
#include "gtkgame.h"
#include "util.h"
#include "gtkfile.h" /*for the quiz, can delete if moved elsewhere*/
#include <dirent.h> /*for opendir function in quiz */

static GtkListStore *plsGameList;
static GtkWidget *pwGameList;
static GtkStyle *psGameList, *psCurrent, *psCubeErrors[3], *psChequerErrors[3], *psLucky[N_LUCKS];

/*quiz stuff*/
static void ReloadQuizConsole(void);
 /*for quiz right-click menu, to be updated in quiz mode*/
static GtkWidget *pQuizMenu;
static GtkWidget *pwQuiz = NULL;
static void BuildQuizMenu(GdkEventButton *event);

static int
gtk_compare_fonts(GtkStyle * psOne, GtkStyle * psTwo)
{
    return pango_font_description_equal(psOne->font_desc, psTwo->font_desc);
}

static void
gtk_set_font(GtkStyle * psStyle, GtkStyle * psValue)
{
    pango_font_description_free(psStyle->font_desc);
    psStyle->font_desc = pango_font_description_copy(psValue->font_desc);
}

enum ListStoreColumns {
    GL_COL_MOVE_NUMBER,
    GL_COL_MOVE_STRING_0,
    GL_COL_MOVE_STRING_1,
    GL_COL_MOVE_RECORD_0,
    GL_COL_MOVE_RECORD_1,
    GL_COL_STYLE_0,
    GL_COL_STYLE_1,
    GL_COL_COMBINED,
    GL_COL_NUM_COLUMNS
};

extern void
GTKClearMoveRecord(void)
{
    gtk_list_store_clear(plsGameList);
}

// /* Copy the GNUBG ID to q.position */
// static void qUpdatePosition(void)
// {                               
//     // char buffer[L_MATCHID + 1 + L_POSITIONID + 1];

//     /* assuming we are in quiet mode...*/
//     // if (ms.gs == GAME_NONE) {
//     //     output(_("No game in progress."));
//     //     outputx();
//     //     return;
//     // }

//     sprintf(qNow.position, "%s:%s", PositionID(msBoard()), MatchIDFromMatchState(&ms));

//     // GTKTextToClipboard(buffer);

//     // gtk_statusbar_push(GTK_STATUSBAR(pwStatus), idOutput, _("Position and Match IDs copied to the clipboard"));
// }

// void NewPosition(GtkTreeSelection *selection, gpointer UNUSED(p))
// {
//     GtkTreeIter iter;
//     GtkTreeModel *model;
//     int value;
//     char * str;
//     g_message("in NewPosition:%d rows", gtk_tree_selection_count_selected_rows(selection));
//     if (gtk_tree_selection_get_selected(GTK_TREE_SELECTION(selection), &model, &iter))
//     {
//         gtk_tree_model_get(model, &iter, GL_COL_MOVE_NUMBER, &value,  -1);
//         gtk_tree_model_get(model, &iter, GL_COL_MOVE_STRING_1, str,  -1);
        
//         g_print("%d is selected\n", value);
//         // // g_free(value);
//         // gtk_widget_set_sensitive(startButton, (currentCategoryIndex>=0 && currentCategoryIndex<numCategories));
//         // gtk_widget_set_sensitive(renameButton, (currentCategoryIndex>=0 && currentCategoryIndex<numCategories));
//         // gtk_widget_set_sensitive(delButton, (currentCategoryIndex>=0 && currentCategoryIndex<numCategories));

//     }
// }


static GdkEventButton *LastEvent;
static int globalCounter=0;

static gboolean
GameListSelectRow(GtkTreeView * tree_view, gpointer UNUSED(p))
{
#if defined(USE_BOARD3D)
    BoardData *bd = BOARD(pwBoard)->board_data;
#endif
    moverecord *apmr[2], *pmr, *pmrPrev = NULL;
    gboolean fCombined;
    GtkTreePath *path;
    GtkTreeViewColumn *column;
    GtkTreeIter iter;
    int *pPlayer;
    listOLD *pl;

    // tree_view=GTK_TREE_VIEW(pwGameList);
    //  GdkEventButton * event = LastEvent;

    if(fInQuizMode || fDelayNewMatchTillLeavingConsole){
        // fInQuizMode=FALSE;
        TurnOffQuizMode();
        UserCommand2("new match");
        return FALSE;
    }


    // g_message("---start of GameListSelectRow, dice=%d,%d---",ms.anDice[0],ms.anDice[1]);
    gtk_tree_view_get_cursor(tree_view, &path, &column);
    if (!path)
        return FALSE;

    /* This didn't seem to happen with GTK2 but has been noticed with GTK3 */
    if (!column) {
        gtk_tree_path_free(path);
        return FALSE;
    }

    pPlayer = g_object_get_data(G_OBJECT(column), "player");
    if (!pPlayer) {
        gtk_tree_path_free(path);
        return FALSE;
    }

    gtk_tree_model_get_iter(GTK_TREE_MODEL(plsGameList), &iter, path);
    gtk_tree_model_get(GTK_TREE_MODEL(plsGameList), &iter, GL_COL_MOVE_RECORD_0, &apmr[0],
                       GL_COL_MOVE_RECORD_1, &apmr[1], GL_COL_COMBINED, &fCombined, -1);
    pmr = apmr[fCombined ? 0 : *pPlayer];

    /* Get previous move record */
    if (!fCombined && *pPlayer == 1)
        pmrPrev = apmr[0];
    else {
        if (!gtk_tree_path_prev(path))
            pmrPrev = NULL;
        else {
            gtk_tree_model_get_iter(GTK_TREE_MODEL(plsGameList), &iter, path);
            gtk_tree_model_get(GTK_TREE_MODEL(plsGameList), &iter, GL_COL_MOVE_RECORD_0, &apmr[0],
                               GL_COL_MOVE_RECORD_1, &apmr[1], GL_COL_COMBINED, &fCombined, -1);
            pmrPrev = apmr[fCombined ? 0 : 1];
        }
    }

    gtk_tree_path_free(path);

    if (!pmr && !pmrPrev)
        return FALSE;

    for (pl = plGame->plPrev; pl != plGame; pl = pl->plPrev) {
        g_assert(pl->p);
        if (pl == plGame->plPrev && pl->p == pmr && pmr && pmr->mt == MOVE_SETDICE)
            break;

        if (pl->p == pmrPrev && pmr != pmrPrev) {
            /* pmr = pmrPrev; */
            break;
        } else if (pl->plNext->p == pmr) {
            /* pmr = pl->p; */
            break;
        }
    }

    if (pl == plGame)
        /* couldn't find the moverecord they selected */
        return FALSE;

    plLastMove = pl;

    CalculateBoard();

    if (pmr && (pmr->mt == MOVE_NORMAL || pmr->mt == MOVE_SETDICE)) {
        /* roll dice */
        ms.gs = GAME_PLAYING;
        ms.anDice[0] = pmr->anDice[0];
        ms.anDice[1] = pmr->anDice[1];
    }
#if defined(USE_BOARD3D)
    if (display_is_3d(bd->rd)) {        /* Make sure dice are shown (and not rolled) */
        bd->diceShown = DICE_ON_BOARD;
        bd->diceRoll[0] = !ms.anDice[0];
    }
#endif

    UpdateSetting(&ms.nCube);
    UpdateSetting(&ms.fCubeOwner);
    UpdateSetting(&ms.fTurn);
    UpdateSetting(&ms.gs);

    SetMoveRecord(pl->p);

    ShowBoard();
    // if(qNow_NDBeforeMoveError>-0.001){
    if(fUseQuiz) {
        if(globalCounter==1) {
            globalCounter=2;
            BuildQuizMenu(LastEvent);
        }
        if (pwQuiz)
            ReloadQuizConsole();
        // GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(pwGameList));
        // NewPosition(selection, NULL);
        globalCounter=0;
    }
        // gtk_widget_show_all(pQuizMenu);

    // g_message("in end of GameListSelectRow, dice=%d,%d, qNowND:%f",ms.anDice[0],ms.anDice[1],qNow_NDBeforeMoveError);
    return FALSE;
}

extern void
GL_SetNames(void)
{
    // g_message("GL_SetNames");
    gtk_tree_view_column_set_title(gtk_tree_view_get_column(GTK_TREE_VIEW(pwGameList), 1), ap[0].szName);
    gtk_tree_view_column_set_title(gtk_tree_view_get_column(GTK_TREE_VIEW(pwGameList), 2), ap[1].szName);
}

static void
SetColourIfDifferent(GdkColor cOrg[5], GdkColor cNew[5], GdkColor cDefault[5])
{
    int i;
    for (i = 0; i < 5; i++) {
        if (memcmp(&cNew[i], &cDefault[i], sizeof(GdkColor)))
            memcpy(&cOrg[i], &cNew[i], sizeof(GdkColor));
    }
}

static void
UpdateStyle(GtkStyle * psStyle, GtkStyle * psNew, GtkStyle * psDefault)
{
    /* Only set changed attributes */
    SetColourIfDifferent(psStyle->fg, psNew->fg, psDefault->fg);
    SetColourIfDifferent(psStyle->bg, psNew->bg, psDefault->bg);
    SetColourIfDifferent(psStyle->base, psNew->base, psDefault->base);

    if (!gtk_compare_fonts(psNew, psDefault))
        gtk_set_font(psStyle, psNew);
}

void
GetStyleFromRCFile(GtkStyle ** ppStyle, const char *name, GtkStyle * psBase)
{                               /* Note gtk 1.3 doesn't seem to have a nice way to do this... */
    BoardData *bd = BOARD(pwBoard)->board_data;
    GtkStyle *psDefault, *psNew;
    GtkWidget *dummy, *temp;
    char styleName[100];
    /* Get default style so only changes to this are applied */
    temp = gtk_button_new();
    gtk_widget_ensure_style(temp);
    psDefault = gtk_widget_get_style(temp);

    /* Get Style from rc file */
    strcpy(styleName, "gnubg-");
    strcat(styleName, name);
    dummy = gtk_label_new("");
    gtk_widget_ensure_style(dummy);
    gtk_widget_set_name(dummy, styleName);
    /* Pack in box to make sure style is loaded */
    gtk_box_pack_start(GTK_BOX(bd->table), dummy, FALSE, FALSE, 0);
    psNew = gtk_widget_get_style(dummy);

    /* Base new style on base style passed in */
    *ppStyle = gtk_style_copy(psBase);
    /* Make changes to fg+bg and copy to selected states */
    if (memcmp(&psNew->fg[GTK_STATE_ACTIVE], &psDefault->fg[GTK_STATE_ACTIVE], sizeof(GdkColor)))
        memcpy(&(*ppStyle)->fg[GTK_STATE_NORMAL], &psNew->fg[GTK_STATE_ACTIVE], sizeof(GdkColor));
    if (memcmp(&psNew->fg[GTK_STATE_NORMAL], &psDefault->fg[GTK_STATE_NORMAL], sizeof(GdkColor)))
        memcpy(&(*ppStyle)->fg[GTK_STATE_NORMAL], &psNew->fg[GTK_STATE_NORMAL], sizeof(GdkColor));
    memcpy(&(*ppStyle)->fg[GTK_STATE_SELECTED], &(*ppStyle)->fg[GTK_STATE_NORMAL], sizeof(GdkColor));

    if (memcmp(&psNew->base[GTK_STATE_NORMAL], &psDefault->base[GTK_STATE_NORMAL], sizeof(GdkColor)))
        memcpy(&(*ppStyle)->base[GTK_STATE_NORMAL], &psNew->base[GTK_STATE_NORMAL], sizeof(GdkColor));
    memcpy(&(*ppStyle)->bg[GTK_STATE_SELECTED], &(*ppStyle)->base[GTK_STATE_NORMAL], sizeof(GdkColor));
    /* Update the font if different */
    if (!gtk_compare_fonts(psNew, psDefault))
        gtk_set_font(*ppStyle, psNew);

    /* Remove useless widgets */
    gtk_widget_destroy(dummy);
    g_object_ref_sink(G_OBJECT(temp));
    g_object_unref(G_OBJECT(temp));
}

static void
RenderMoveString(GtkTreeViewColumn * tree_column, GtkCellRenderer * cell, GtkTreeModel * tree_model,
                 GtkTreeIter * iter, gpointer UNUSED(p))
{
    gchar *moveString;
    GtkStyle *style;
    int *pPlayer = g_object_get_data(G_OBJECT(tree_column), "player");

    gtk_tree_model_get(tree_model, iter, GL_COL_MOVE_STRING_0 + *pPlayer, &moveString,
                       GL_COL_STYLE_0 + *pPlayer, &style, -1);
    if (!style) {
        style = psGameList;
        g_object_ref(style);
    }

    g_object_set(cell, "text", moveString, "background-gdk", &style->base[GTK_STATE_NORMAL],
                 "foreground-gdk", &style->fg[GTK_STATE_NORMAL], "font-desc", style->font_desc, NULL);

    if (moveString)
        g_free(moveString);
    g_object_unref(style);
}

static void
CreateStyles(GtkWidget * UNUSED(widget), gpointer UNUSED(p))
{
    GtkStyle *ps;

    gtk_widget_ensure_style(pwGameList);
    GetStyleFromRCFile(&ps, "gnubg", gtk_widget_get_style(pwGameList));
    ps->base[GTK_STATE_SELECTED] =
        ps->base[GTK_STATE_ACTIVE] =
        ps->base[GTK_STATE_NORMAL] = gtk_widget_get_style(pwGameList)->base[GTK_STATE_NORMAL];
    ps->fg[GTK_STATE_SELECTED] =
        ps->fg[GTK_STATE_ACTIVE] = ps->fg[GTK_STATE_NORMAL] = gtk_widget_get_style(pwGameList)->fg[GTK_STATE_NORMAL];
    gtk_widget_set_style(pwGameList, ps);

    psGameList = gtk_style_copy(ps);
    psGameList->bg[GTK_STATE_SELECTED] = psGameList->bg[GTK_STATE_NORMAL] = ps->base[GTK_STATE_NORMAL];

    psCurrent = gtk_style_copy(psGameList);
    psCurrent->bg[GTK_STATE_SELECTED] = psCurrent->bg[GTK_STATE_NORMAL] =
        psCurrent->base[GTK_STATE_SELECTED] = psCurrent->base[GTK_STATE_NORMAL] = psGameList->fg[GTK_STATE_NORMAL];
    psCurrent->fg[GTK_STATE_SELECTED] = psCurrent->fg[GTK_STATE_NORMAL] = psGameList->bg[GTK_STATE_NORMAL];

    GetStyleFromRCFile(&psCubeErrors[SKILL_VERYBAD], "gamelist-cube-blunder", psGameList);
    GetStyleFromRCFile(&psCubeErrors[SKILL_BAD], "gamelist-cube-error", psGameList);
    GetStyleFromRCFile(&psCubeErrors[SKILL_DOUBTFUL], "gamelist-cube-doubtful", psGameList);

    GetStyleFromRCFile(&psChequerErrors[SKILL_VERYBAD], "gamelist-chequer-blunder", psGameList);
    GetStyleFromRCFile(&psChequerErrors[SKILL_BAD], "gamelist-chequer-error", psGameList);
    GetStyleFromRCFile(&psChequerErrors[SKILL_DOUBTFUL], "gamelist-chequer-doubtful", psGameList);

    GetStyleFromRCFile(&psLucky[LUCK_VERYBAD], "gamelist-luck-bad", psGameList);
    GetStyleFromRCFile(&psLucky[LUCK_VERYGOOD], "gamelist-luck-good", psGameList);
}


/* ************ QUIZ ************************** */
/*
The quiz mode defines a full loop, from picking the position category to play with,
to playing, to getting the hint screen, to starting again. Formally, here are the 
functions in the loop:
QuizConsole->StartQuiz->OpenQuizPositionsFile, LoadPositionAndStart
    ->CommandSetGNUbgID->(play)->MoveListUpdate->qUpdate(play error)
    ->SaveFullPositionFile [screen: could play again] -> HintOK->BackFromHint
    ->either LoadPositionAndStart or QuizConsole
*/

// static void QuizConsole(void);

#define WIDTH   640
#define HEIGHT  480
#define MAX_ROWS 3000
#define MAX_ROW_CHARS 1024
#define ERROR_DECAY 0.6
#define SMALL_ERROR 0.2
// #define BGCOLOR "#EDF5FF"

// static char positionsFolder [200];
// char * tempPath= g_build_filename(szHomeDirectory, "quiz", NULL);
// strcpy(positionsFolder,tempPath);
    // 
    // if (g_file_test(path, G_FILE_TEST_IS_REGULAR)) {
// "/home/isaac/g/new-gnubg-features/gnubg/quiz";
// static const char positionsFolder2 []="~/g/new-gnubg-features/gnubg/quiz/";

//static char positionsFileFullPath []="./quiz/positions.csv";
// static char * positionsFileFullPath;
// static char * currentCategory;
int currentCategoryIndex=-1; /*extern*/
static quiz q[MAX_ROWS]; 
static int qLength=0;
static int iOpt=-1; 
static int iOptCounter=0; 
int skipDoubleHint=0; 
// quiz qNow={"\0",0,0,0,0.0}; /*extern*/
int counterForFile=0; /*extern*/
float latestErrorInQuiz=-1.0; /*extern*/
// char * name0BeforeQuiz=NULL; //[MAX_NAME_LEN+1];
// char * name1BeforeQuiz=NULL; //[MAX_NAME_LEN+1];
char name0BeforeQuiz[MAX_NAME_LEN];
char name1BeforeQuiz[MAX_NAME_LEN];
playertype type0BeforeQuiz;
static int fTutorBeforeQuiz;

// /*initialization: maybe not needed, using GetCategory->InitCategoryArray */
// static const categorytype emptyCategory={NULL,0,NULL,NULL};
categorytype categories[MAX_POS_CATEGORIES]; //={""};
int numCategories;//=0
// int numPositionsInCategory[MAX_POS_CATEGORIES]={0}; /* array with #positions per category file*/

enum {COLUMN_INDEX, COLUMN_STRING, COLUMN_INT, N_COLUMNS};

/* the following function:
- updates positionsFileFullPath,
- opens the corrsponding file, 
- and saves the parsing results in the categories static array 
*/
static int OpenQuizPositionsFile(const int index)
{
    char row[MAX_ROW_CHARS];
    int i = -2;
    int column = 0;

    // positionsFileFullPath = g_strconcat(positionsFolder, G_DIR_SEPARATOR_S, category, ".csv", NULL);
    // g_message("fullPath=%s",positionsFileFullPath);

	FILE* fp = fopen(categories[index].path, "r");

    if(fp==NULL) {
        char buf[100];
        sprintf(buf,_("Error: Problem reading file %s"),categories[index].path);
        GTKMessage(_(buf), DT_INFO);
        return FALSE;
    }

    /* based on standard csv program from geeksforgeeks*/
    while (fgets(row, MAX_ROW_CHARS, fp)) {
        column = 0;
        i++;

        // To avoid printing of column
        // names in file can be changed
        // according to need
        if (i == -1)
            continue;

        // Splitting the data
        char* token = strtok(row, ", ");
        // g_message("i:%d, token:%s",i,token);

        while (token) {
            // // Column 1
            // if (column == 0) {
            //     printf("Position:");
            // }
            // // Column 2
            // if (column == 1) {
            //     printf("\tError:");
            // }
            // // Column 3
            // if (column == 2) {
            //     printf("\tTime:");
            // }
            // printf("%s", token);
            if (column == 0) {
                // g_message("i=%d",i);
                // q[i].position = malloc(50 * sizeof(char));
                strcpy(q[i].position,token); 
        // g_message("read new line %d: %s\n", i, q[i].position);
            } else if (column == 1) {
                q[i].set=strtol(token, NULL, 10); //atof(token);
            } else if (column == 2) {
                q[i].player=strtol(token, NULL, 10); //atof(token);
            } else if (column == 3) {
                q[i].ewmaError=atof(token);
        // g_message("read new line %d: %s, %.3f\n", i, q[i].position, q[i].ewmaError);
            } else if (column == 4) {
                q[i].lastSeen=strtol(token, NULL, 10);
                // q[i].lastSeen=strtoimax(sz, NULL, 10);
        // g_message("read new line %d: %s, %.3f, %ld\n", i, q[i].position, q[i].ewmaError, q[i].lastSeen);
            } 
            // printf("%s", token);


            // g_message("i:%d,col=%d, token:%s",i,column,token);
            token = strtok(NULL, ", ");
            column++;
        }
        // printf("\n");
        //    intmax_t seconds = strtoimax(sz, NULL, 10);
        // g_message("read new line %d: %s, %.5f, %ld\n", i, q[i].position, q[i].ewmaError, q[i].lastSeen);

    }
    qLength=i+1;
    // g_message("qLength=%d ==??? categories[index].number=%d ",qLength,categories[index].number);
    g_assert(qLength==categories[index].number);
    // Close the file
    fclose(fp);
    if(qLength<1) {
        GTKMessage(_("Error: no positions in file?"), DT_INFO);
        // return FALSE;
    } 
	return qLength;
}

/* reminder: in backgammon.h, we define: */
// typedef struct {
//     char position [100]; 
//     int player; /*player when adding position*/
//     quizset set;
//     float ewmaError; 
//     long int lastSeen; 
//     float priority;
// } quiz;


static void writeQuizHeader (FILE* fp) {
        fprintf(fp, "position, set, player, ewmaError, lastSeen\n");
}
static void writeQuizLine (quiz q, FILE* fp) {
        fprintf(fp, "%s, %d, %d, %.5f, %ld\n", q.position, q.set, q.player, q.ewmaError, q.lastSeen);
}
static int writeQuizLineFull (quiz q, char * file, int quiet) {
    /*test that file exists, else write header*/
    if(!g_file_test(file, G_FILE_TEST_EXISTS )){
        // g_message("writeQuizLineFull file doesn't exist: %s",file);
        	FILE* fp0 = fopen(file, "w");        
            if(!fp0){
                // g_message("writeQuizLineFull file had a pointer problem: fp0-> %s",file);
                if(!quiet) {
                    char buf[100];
                    sprintf(buf,_("Error: cannot write into file %s"),file);
                    GTKMessage(_(buf), DT_INFO);
                }
                return FALSE;
            }
            writeQuizHeader(fp0);
            fclose(fp0);
    }
    if(!g_file_test(file, G_FILE_TEST_IS_REGULAR)){
        // g_message("writeQuizLineFull problem with file %s",file);
        if(!quiet) {
            char buf[100];
            sprintf(buf,_("Error: problem with file %s, not a regular file?"),file);
            GTKMessage(_(buf), DT_INFO);
        }
        return FALSE;
    }
    FILE* fp = fopen(file, "r+");
    if(!fp){
        // g_message("writeQuizLineFull cannot read/write: fp-> %s",file);
        if(!quiet) {
            char buf[100];
            sprintf(buf,_("Error: cannot read/write file %s"),file);
            GTKMessage(_(buf), DT_INFO);
        }
        return FALSE;
    }
    char line[MAX_ROW_CHARS];
    int lineCounter=-2;
    while (fgets(line, sizeof(line), fp)){
        lineCounter++;
        // g_message("%s->%s?",line, q.position);
        if (strstr(line, q.position)!=NULL) {
            // g_message("writeQuizLineFull found a match for position");
            if(!quiet) {
                char buf[100];
                sprintf(buf,_("Error: the position is already in file %s"),file);
                GTKMessage(_(buf), DT_INFO);
            }
            fclose(fp);
            return FALSE;
        }
    }
    // g_message("lineCounter=%d",lineCounter);
    if(lineCounter>MAX_ROWS){
        // g_message("writeQuizLineFull full file-> %s",file);
        if(!quiet) {
            char buf[100];
            sprintf(buf,_("Error: file %s has reached the maximum allowed number %d of positions"),
                file,MAX_ROWS);
            GTKMessage(_(buf), DT_INFO);
        }
        fclose(fp);
        return FALSE;
    }    
    writeQuizLine (q, fp);
    // g_message("Added a line");
    fclose(fp);
    return TRUE;
}

// extern int AddQuizPosition(quiz qRow, categorytype * pcategory)
// {
//     writeQuizLineFull (qRow, pcategory->path, FALSE);

// 	// FILE* fp = fopen(pcategory->path, "a");

// 	// if (!fp){
//     //     GTKMessage(_("Error: problem saving quiz position, cannot open file"), DT_INFO);
// 	// 	// printf("Can't open file\n");
//     //     return FALSE;
//     // } 
//     // // Saving data in file
//     // // fprintf(fp, "%s, %d, %.5f, %ld\n", qRow.position, qRow.player, qRow.ewmaError, qRow.lastSeen);
//     // writeQuizLine (qRow, fp);
//     // g_message("Added a line");
//     // fclose(fp);
//     // return TRUE;
// }

extern int AutoAddQuizPosition(quiz q, quizdecision qdec) {

    char * file;
    // char * file = g_build_filename(szHomeDirectory, "quiz", "autoadd.csv", NULL);

    // if (!g_file_test(file, G_FILE_TEST_IS_REGULAR)) {
    //     g_message("autoadd file had a problem: %s",file);
    //     return FALSE;
    // }

    if(qdec==QUIZ_MOVE)
        file = g_build_filename(szHomeDirectory, "quiz", "MOVE-autoadded.csv", NULL);
    else if(qdec==QUIZ_DOUBLE || qdec==QUIZ_NODOUBLE)
        file = g_build_filename(szHomeDirectory, "quiz", "DOUBLE-NO_DOUBLE-autoadded.csv", NULL);
    else if(qdec==QUIZ_PASS || qdec==QUIZ_TAKE)
        file = g_build_filename(szHomeDirectory, "quiz", "PASS-TAKE-autoadded.csv", NULL);
    else
        return FALSE;

    return (writeQuizLineFull (q, file, TRUE));//TRUE for quiet

    // if(!g_file_test(file, G_FILE_TEST_EXISTS )){
    //     g_message("autoadd file doesn't exist: %s",file);
    //     	FILE* fp0 = fopen(file, "w");        
    //         if(!fp0){
    //             g_message("autoadd file had a pointer problem:fp0-> %s",file);
    //             return FALSE;
    //         }
    //         writeQuizHeader (fp0);
    //         fclose(fp0);
    // }
    // FILE* fp = fopen(file, "a");
    // if(!fp){
    //     g_message("autoadd file had a pointer problem: %s",file);
    //     return FALSE;
    // }
    // writeQuizLine (q, fp);
    // g_message("Added a line");
    // fclose(fp);
    // return TRUE;
}

// extern int AutoAddQuizPosition(-rSkill,QUIZ_NODOUBLE)(float error, quizdecision qdec) {
//     g_message("Adding position: %s to category %s, error: %f",
//         qNow.position,pcategory->name,qNow.ewmaError);
//     qNow.lastSeen=(long int) (time(NULL));
//     return (AddQuizPosition(qNow,pcategory));
// }


/* Here we save a full file with all positions. We assume that they were already checked to 
be distinct. */
static int SaveFullPositionFile(void)
{
        // szFile = g_build_filename(szHomeDirectory, "gnubgautorc", NULL);

    FILE* fp = fopen(categories[currentCategoryIndex].path, "w");

	if (!fp){
        GTKMessage(_("Error: problem saving quiz position, cannot open file"), DT_INFO);
        return FALSE;
    } 
    /*header*/
    // fprintf(fp, "position, player, ewmaError, lastSeen\n");
    writeQuizHeader (fp);
    for (int i = 0; i < qLength; ++i) {
        // Saving data in file
        // fprintf(fp, "%s, %d, %.5f, %ld\n", q[i].position, q[i].player, q[i].ewmaError, q[i].lastSeen);
        writeQuizLine (q[i], fp);
        // writeQuizLineFull (quiz q, categories[currentCategoryIndex].path, int quiet);
    }
    // g_message("Saved q");
    fclose(fp);
    return TRUE;
}

extern void DeletePosition(void) {
    if (!GTKGetInputYN(_("Are you sure you want to delete this position?")))
        return;
    q[iOpt]=q[qLength-1];
    qLength-=1;
    SaveFullPositionFile();
    GTKMessage(_("Position deleted."), DT_INFO);
    if (qLength==0) {
        StopLoopClicked(NULL,NULL);
        return;
    }
    else {
        return;
    }
}


extern void qUpdate(float error) {
    /* if someone makes a mistake, then plays again the same position after seeing the 
    answer, only the first time should count => we use iOptCounter 
    */

    if(iOptCounter==0) {
        // g_message("updating in qUpdate with error=%f",error);
        // g_message("q[iOpt].ewmaError=%f, error=%f => ?",q[iOpt].ewmaError,error);//        2/3*q[iOpt].ewmaError+1/3*error);
        q[iOpt].ewmaError=ERROR_DECAY*(q[iOpt].ewmaError)+(1-ERROR_DECAY)*error;
        // g_message("result: q[iOpt].ewmaError=%f", q[iOpt].ewmaError);  
        q[iOpt].lastSeen=(long int) (time(NULL));
        /*Here we save the whole file again. If it gets slow, alternative=keep line
        number and only update this line.*/
        SaveFullPositionFile();
        latestErrorInQuiz=error;
        iOptCounter=1;
        counterForFile++;
        // g_message("qUpdate:counterForFile=%d",counterForFile);
    } 
    // else  
    //      g_message("NOT updating in qUpdate! 2nd update or more...");

}
// #if(0) /***********/
static void DisplayCategories(void)
{
    for(int i=0;i < numCategories; i++) {
        g_message("in DisplayCategories: %d->%s,%d,%s", 
            i,categories[i].name,categories[i].number,categories[i].path);
    }
        g_message("numCategories=%d",numCategories);
}

// int NameIsCategory (const char sz[]) {
//     for(int i=0;i < numCategories; i++) {
//         if (!strcmp(sz, categories[i])) {
//             // g_message("NameIsKey: EXISTS! %s=%s at i=%d", sz,categories[i],i);
//             return 1;
//         }
//     }
//     return 0;
// }

static void InitCategoryArray(void) {
    const categorytype emptyCategory={"\0",0,"\0"};
    for(int i=0;i < MAX_POS_CATEGORIES; i++) {
        categories[i]=emptyCategory;
        // categories[i][0]='\0';
        // numCategories=0;
    }
    numCategories=0;
}


// /* from ad absurdum, stackoverflow*/
// void strip_ext(char *fname)
// {
//     char *end = fname + strlen(fname);

//     while (end > fname && *end != '.' && *end != '\\' && *end != '/') {
//         --end;
//     }
//     if ((end > fname && *end == '.') &&
//         (*(end - 1) != '\\' && *(end - 1) != '/')) {
//         *end = '\0';
//     }  
// }


static int CountPositions(categorytype category) {

    //char * fullPath = g_strconcat(positionsFolder, G_DIR_SEPARATOR_S, category, ".csv", NULL);
    // g_message("fullPath=%s",fullPath);


	FILE* fp = fopen(category.path, "r");
    if (fp == NULL) {
        // g_message("error: opening file: %s ", category.path);
        return 0; //category.number=0;
    }

    int c;    // this must be an int
    int count = -1; //don't count the title

    for (c = getc(fp); c != EOF; c = getc(fp)) {
        if (c == '\n') // Increment count if this character is newline 
            count = count + 1;
    }

    fclose(fp);
    // g_message("count=%d",count);
    return count; //category.number=count;
}

// /*case-insensitive string comparison, inspired by minilibc code from JL2210 */

// int strcasecmp(const char *restrict str1, const char *restrict str2)
// {
//     const unsigned char *s1 = (const unsigned char *)str1,
//                         *s2 = (const unsigned char *)str2;

//     if(str1 == str2)
//         return 0;

//     while( *s1 && toupper(*s1) == toupper(*s2) )
//     {
//         s1++;
//         s2++;
//     }

//     return *s1 - *s2;
// }

/*case-insensitive string comparison*/
// int string_cmp (const categorytype * p1, const categorytype * p2) {
static int string_cmp (const void * p1, const void * p2) {
    // const char * pa = (const char *) a;
    // const char * pb = (const char *) b;
    // return strcmp(pa,pb);
    // return strcmp(((const categorytype *)p1)->name, ((const categorytype *)p2)->name);
    const unsigned char *s1 = (const unsigned char *)((const categorytype *)p1)->name,
                        *s2 = (const unsigned char *)((const categorytype *)p2)->name;
    if(s1 == s2)
        return 0;
    while( *s1 && toupper(*s1) == toupper(*s2) )
    {
        s1++;
        s2++;
    }
    // g_message("string comp: %s vs %s, %c vs %c=>%d|%d",s1,s2,(toupper(*s1)),(toupper(*s2)),*s1 - *s2,toupper(*s1) - toupper(*s2));
    return toupper(*s1) - toupper(*s2);
}

static void populateCategory(const int index, const char * name, const int updateNumber) {
    /* add name*/
    strcpy(categories[index].name, name);
    /* add file path */
    // strcpy(categories[index].path,g_strconcat(positionsFolder, 
    //     G_DIR_SEPARATOR_S, categories[index].name, ".csv", NULL));
    gchar *file = g_strdup_printf("%s.csv", name);
    gchar *path = g_build_filename(szHomeDirectory, "quiz", file, NULL);
    strcpy(categories[index].path,path);

    /*compute and add number of positions*/
    if (updateNumber)
        categories[index].number=CountPositions(categories[index]);
    // g_message("at index %d with name=%s, number=%d",index,name,categories[index].number);
}

extern void GetPositionCategories(void) {
    //     char buffer[MAX_LEN];
    // FilePreviewData *fdp;
    struct dirent* entry;
    // time_t recenttime = 0;
    // struct stat statbuf;
    InitCategoryArray();

    DIR* dir  = opendir(g_build_filename(szHomeDirectory, "quiz", NULL));
    if (!dir)
        return;
    while (NULL != (entry = readdir(dir))) {
        //g_message("entry->d_name=%s",entry->d_name);
        const char *dot = strrchr(entry->d_name, '.');
        if(dot && dot != entry->d_name && (strcmp(dot+1,"csv") == 0)) {
            int offset = dot - entry->d_name;
            entry->d_name[offset] = '\0';
            populateCategory(numCategories, entry->d_name,TRUE);
            // strcpy(categories[numCategories].name, entry->d_name);
            // // g_message("stripped entry->d_name=%s=%s",entry->d_name,
            //     // categories[numCategories]);
            // /*computes number of positions*/
            // CountPositions(&(categories[numCategories]));
            // // categories[numCategories].number=CountPositions(categories[numCategories]);
            // categories[numCategories].path=g_strconcat(positionsFolder, 
            //     G_DIR_SEPARATOR_S, categories[numCategories].name, ".csv", NULL);
            
            numCategories++;
        }
    }
    /* sorting alphabetically */
    qsort(categories, numCategories, sizeof(categorytype), string_cmp );

    closedir(dir);
    if(0)
        DisplayCategories();
}

/* for reference: code for deleting several rows, if needed once (Fedor, stackoverflow):
static void
delete_selected_rows (GtkButton * activated, GtkTreeView * tree_view)
{
  GtkTreeSelection * selection = gtk_tree_view_get_selection (tree_view);
  GtkTreeModel *model;
  GList * selected_list = gtk_tree_selection_get_selected_rows (selection, &model);
  GList * cursor = g_list_last (selected_list);
  while (cursor != NULL) {
    GtkTreeIter iter;
    GtkTreePath * path = cursor->data;
    gtk_tree_model_get_iter (model, &iter, path);
    gtk_list_store_remove (GTK_LIST_STORE (model), &iter);
    cursor = cursor->prev;
  }
  g_list_free_full (selected_list, (GDestroyNotify) gtk_tree_path_free);
}
*/

/* another reference: the code below could be shorter with:
  if (gtk_tree_model_iter_nth_child(GTK_TREE_MODEL(store), &iter, NULL, n))
    {
      gtk_list_store_remove(store, &iter);
      return TRUE;
    }
*/

/*  delete a position category from the categories array */
static int DeleteCategory(const int categoryIndex) {
    // g_message("in DeleteCategory: %s, length=%zu", sz, strlen(sz));
    // g_message("before loop in DeleteCategory:numCategories=%d, look for %s ",numCategories, sz);
    if(categoryIndex<0 || categoryIndex>=numCategories) {
        GTKMessage(_("Error: you forgot to select a position category"), DT_INFO);
        return 0;
    }
    // for(int i=0;i < numCategories; i++) {
    //     g_message("loop in DeleteCategory: i=%d, numCategories=%d, %s =?= %s ",i,numCategories, sz,categories[i]);
    //     if (!strcmp(sz, categories[i].name)) {
    //         g_message("EXISTS! %s=%s, i=%d, numCategories=%d", sz,categories[i],i,numCategories);
    //         // char *fullPath = g_strconcat(positionsFolder, G_DIR_SEPARATOR_S, sz, ".csv", NULL);
    if (remove(categories[categoryIndex].path)!=0){
        GTKMessage(_("Error: File could not be removed"), DT_INFO);
        return 0;
    }
    // if (numCategories==(categoryIndex+1)) {
    //         g_message("DeleteCategory: last category");
    //         numCategories--;
    //         // UserCommand2("save settings");
    //         // return 1;
    // } else {

    for(int j=categoryIndex+1;j < numCategories; j++) {
        // g_message("loop2 in DeleteCategory: j=%d",j);
        categories[j-1]=categories[j]; 
    }
    numCategories--;
        // DisplayCategories();
        // UserCommand2("save settings");
        // return 1;
    // g_message("end of deletecategory");
    return 1;
}

static int CheckBadCharacters(const char * name) {
    // g_message("checking");
    char bad_chars[] = "!@%^*~|<>:$/?\\\"";
    int invalid_found = FALSE;
    int i;
    for (i = 0; i < (int) strlen(bad_chars); ++i) {
        if (strchr(name, bad_chars[i]) != NULL) {
            invalid_found = TRUE;
            // g_message("invalid:%c",bad_chars[i]);
            break;
        }
    }
    return invalid_found;
}

/*  add a new position category to the categories array ... 
and make a corresponding file.
Based on AddkeyName() function (and same for the similar functions above)
Return 1 if success, 0 if problem */
static int
AddCategory(const char *sz)
{
    // g_message("in AddCategory: %s, length=%zu", sz, strlen(sz));
    /* check that the name doesn't contain "\t", "\n" */
    if (strstr(sz, "\t") != NULL || strstr(sz, "\n") != NULL || CheckBadCharacters(sz)) {
        // for(unsigned int j=0;j < strlen(sz); j++) {
        //     g_message("%c", sz[j]);
        // }
            GTKMessage(_("Error: Position category name contains unallowed character"), DT_INFO);
        return 0;
    }

    /* check that the category name is not too long*/
    if(strlen(sz) > MAX_CATEGORY_NAME_LENGTH) {
            GTKMessage(_("Error: Position category name is too long"), DT_INFO);
        return 0;
    }

    /* check that the categories array is not full */
    if(numCategories >= MAX_POS_CATEGORIES) {
            GTKMessage(_("Error: We have reached the maximum allowed number of position categories"), DT_INFO);
        return 0;
    }

    /* check that the position category doesn't already exist */
    for(int i=0;i < numCategories; i++) {
        if (!strcmp(sz, categories[i].name)) {
            GTKMessage(_("Error: This category name already exists"), DT_INFO);
            return 0;
        }
    }

    /* check that file can be added*/
    gchar *file = g_strdup_printf("%s.csv", sz);
    gchar *fullPath = g_build_filename(szHomeDirectory, "quiz", file, NULL);
    // char *fullPath = g_strconcat(positionsFolder, G_DIR_SEPARATOR_S, sz, ".csv", NULL);
    // g_message("fullPath=%s",fullPath);

	FILE* fp = fopen(fullPath, "w");

    if(fp==NULL) {
        char buf[100];
        sprintf(buf,_("Error: Problem writing into file %s"),fullPath);
        GTKMessage(_(buf), DT_INFO);
        return 0;
    }
    //  perror("fopen");
    /*header*/
    writeQuizHeader (fp);
    // fprintf(fp, "position, cubedecision, ewmaError, lastSeen\n");
    fclose(fp);

	// FILE* fp2 = fopen(sz, "r");

    // if(fp2==NULL) {
    //     char buf[100];
    //     sprintf(buf,_("Error: Could not create file %s, maybe we do not have writing right?"),
    //         fullPath);
    //     GTKMessage(_(buf), DT_INFO);
    //     return 0;
    // }
    // fclose(fp2);

    populateCategory(numCategories,sz,FALSE);

    // strcpy(categories[numCategories].name,sz); 
    numCategories++;

        /* sorting alphabetically */
    qsort(categories, numCategories, sizeof(categorytype), string_cmp );
    BuildQuizMenu(NULL);

    // DisplayCategories();
    // UserCommand2("save settings");
    return 1;
}

/*  rename a position category in the array, and its corresponding file.
Return the position of the category, -1 if problem */
static int
RenameCategory(const char * szOld, const char * szNew)
{
    if (!strcmp(szOld, szNew)) {
        GTKMessage(_("Error: The old and new category names are the same"), DT_INFO);
        return -1;
    }

    /* check that the new name doesn't contain "\t", "\n" */
    if (strstr(szNew, "\t") != NULL || strstr(szNew, "\n") != NULL
         || CheckBadCharacters(szNew)) {
        GTKMessage(_("Error: Position category name contains unallowed character"), DT_INFO);
        return -1;
    }

    /* check that the new category name is not too long*/
    if(strlen(szNew) > MAX_CATEGORY_NAME_LENGTH) {
        GTKMessage(_("Error: Position category name is too long"), DT_INFO);
        return -1;
    }

    /* check that the old position category exists, and the new one does not! */
    int positionIndex=-1;
    for(int i=0;i < numCategories; i++) {
        if (!strcmp(szNew, categories[i].name)) {
            GTKMessage(_("Error: This category name already exists"), DT_INFO);
            return -1;
        }
        if (!strcmp(szOld, categories[i].name)) {
            positionIndex=i;
        }
    }
    if(positionIndex==-1) {
        GTKMessage(_("Error: Could not find the old position name"), DT_INFO);
        return -1;
    }

    /* replace file*/
    // char *fullOldPath = g_strconcat(positionsFolder, G_DIR_SEPARATOR_S, szOld, ".csv", NULL);
    // char fullOldPath [MAX_CATEGORY_PATH_LENGTH];
    // strcpy(fullOldPath, categories[i].path);
    // char *fullNewPath = g_strconcat(positionsFolder, G_DIR_SEPARATOR_S, szNew, ".csv", NULL);
    gchar *file = g_strdup_printf("%s.csv", szNew);
    gchar *fullNewPath = g_build_filename(szHomeDirectory, "quiz", file, NULL);

    int result = rename(categories[positionIndex].path, fullNewPath);
	
    if(result!=0) {
        char buf[200];
        sprintf(buf,_("Error: Problem renaming file %s as %s"),categories[positionIndex].path,fullNewPath);
        GTKMessage(_(buf), DT_INFO);
        return -1;
    }

    // strcpy(categories[positionIndex].name,szNew); 
    populateCategory(positionIndex,szNew,TRUE);

    // /* sorting alphabetically */
    // qsort(categories, numCategories, sizeof(categorytype), string_cmp );

    DisplayCategories();
    // UserCommand2("save settings");
    return positionIndex;
}

// #endif /***********/


static GtkTreeIter selected_iter;
static GtkListStore *nameStore;

static int
GetSelectedCategoryIndex(GtkTreeView * treeview)
{
    GtkTreeModel *model;
    // char *category = NULL;
    GtkTreeSelection *sel = gtk_tree_view_get_selection(treeview);
    if (gtk_tree_selection_count_selected_rows(sel) != 1) {
        // g_message("no index!");
        return (-1);
    }
    GList * selected_list = gtk_tree_selection_get_selected_rows (sel, &model);
    GList * cursor = g_list_first (selected_list);
    GtkTreePath * path = cursor->data;
    gint * a = gtk_tree_path_get_indices (path);
    // g_message("index=%d",a[0]);
    return a[0];
}

static char *
GetSelectedCategory(GtkTreeView * treeview)
{
    GtkTreeModel *model;
    char *categoryName = NULL;
    GtkTreeSelection *sel = gtk_tree_view_get_selection(treeview);
    if (gtk_tree_selection_count_selected_rows(sel) != 1)
        return NULL;
    
    /* Sets selected_iter to the currently selected node: */
    gtk_tree_selection_get_selected(sel, &model, &selected_iter);
    
    /* Gets the value of the char* cell (in column COLUMN_STRING) in the row 
        referenced by selected_iter */
    gtk_tree_model_get(model, &selected_iter, COLUMN_STRING, &categoryName, -1);
    // g_message("GetSelectedCategory gives categoryName=%s",categoryName);
    return categoryName;
}

static void
DestroyQuizDialog(gpointer UNUSED(p), GObject * UNUSED(obj))
/* Called by gtk when the quiz console window is closed.
Allows garbage collection.
*/
{
    // g_message("in destroy");
    // sprintf(name0BeforeQuiz, "%s",ap[0].szName);
    // sprintf(name1BeforeQuiz, "%s",ap[0].szName);
    
    if (pwQuiz!=NULL) { //i.e. we haven't closed it using DestroyQuizDialog()
        // g_message("1st if option: pwQuiz exists");
        gtk_widget_destroy(gtk_widget_get_toplevel(pwQuiz));
        pwQuiz = NULL;
    }
    if(fDelayNewMatchTillLeavingConsole) {
        // g_message("2nd option: fDelayNewMatchTillLeavingConsole");
        fDelayNewMatchTillLeavingConsole=0;
        fQuietNewMatch=1;
        UserCommand2("new match");
        StatusBarMessage("Out of quiz mode");
#if defined(USE_GTK)
        if (fX) {
            gtk_window_set_title(GTK_WINDOW(pwMain), "GNU Backgammon - out of quiz mode");
        }
#endif   
        fQuietNewMatch=0;
    }
        // g_message("done w/ destroy");
}

static void ReloadQuizConsole(void) {
    // GetPositionCategories();
    
    // UpdateGame(FALSE);
    // GTKClearMoveRecord();
            // CreateGameWindow();
            //CreatePanels();
    // UserCommand2("set dockpanels off"); //       DockPanels();


    /*this was needed to apply the change in the gamelist menu, but is not anymore*/
    //UserCommand2("set dockpanels on"); //       DockPanels();
    g_message("reload console");

    DestroyQuizDialog(NULL,NULL);
    QuizConsole();

}
// extern void
// CommandAddQuizPosition(char *sz)
// {

//     char *pch = NextToken(&sz);
//     int i;

//     if (!pch) {
//         outputl(_("To which quiz category do you want to add this position?"));
//         return;
//     }

//     GetPositionCategories();
//     for(int i=0;i < numCategories; i++) {
//         if (!strcmp(sz, categories[i].name)) {
//             AddPositionToFile(&(categories[i]),NULL);
//             return;
//         }
//     }
//     outputl(_("Error: This category does not exist; please create it first"), DT_INFO);
//     // GTKMessage(_("Error: This category name already exists"), DT_INFO);
// }

static int AddPositionToFile(categorytype * pcategory, GtkWidget * UNUSED(pw)) {
    // g_message("I'm in the test func: %s", pcategory->name);
    // g_message("Adding position: %s to category %s, error: %f",
    //     qNow.position,pcategory->name,qNow.ewmaError);
    qNow.lastSeen=(long int) (time(NULL));
    // return (AddQuizPosition(qNow,pcategory));
    return(writeQuizLineFull (qNow, pcategory->path, FALSE));

}

static void
AddPositionClicked(GtkButton * UNUSED(button), gpointer treeview)
{
    // GtkTreeIter iter;
    // GetSelectedCategoryIndex(GTK_TREE_VIEW(treeview));
    int categoryIndex = GetSelectedCategoryIndex(GTK_TREE_VIEW(treeview));
    if(categoryIndex<0 || categoryIndex>=numCategories) {
        GTKMessage(_("Error: you forgot to select a position category"), DT_INFO);
        return;
    }
    // g_position("got back with int=%d",categoryIndex);
    if(AddPositionToFile(&(categories[categoryIndex]),NULL)){
        GTKMessage(_("The position was added successfully."), DT_INFO);
        ReloadQuizConsole();
    }
    return;    
}

static int AddNDPositionToFile(categorytype * pcategory, GtkWidget * UNUSED(pw)) {
    // g_message("I'm in the test func: %s", pcategory->name);
    UserCommand2("previous roll");
    UserCommand2("previous roll");
    UserCommand2("next roll");
    qNow.ewmaError=qNow_NDBeforeMoveError;
    // g_message("Adding CUBE position: %s to category %s, error: %f",
    //     qNow.position,pcategory->name,qNow.ewmaError);
    qNow.lastSeen=(long int) (time(NULL));
    return(writeQuizLineFull (qNow, pcategory->path, FALSE));
    // return (AddQuizPosition(qNow,pcategory));
}

static void
AddNDPositionClicked(GtkButton * UNUSED(button), gpointer treeview)
{
    if(qNow_NDBeforeMoveError <-0.001) {
        /*should not happen, there is no ND position; maybe user clicked outside 
        the manage window*/
        GTKMessage(_("Error: the selected move seems to have changed, reloading the quiz window"), DT_INFO);
        ReloadQuizConsole();
        return;        
    }
    int categoryIndex = GetSelectedCategoryIndex(GTK_TREE_VIEW(treeview));
    if(categoryIndex<0 || categoryIndex>=numCategories) {
        GTKMessage(_("Error: you forgot to select a position category"), DT_INFO);
        return;
    }
    if(AddNDPositionToFile(&(categories[categoryIndex]),NULL)){
        GTKMessage(_("The double/no-double decision was added successfully."), DT_INFO);
        ReloadQuizConsole();
    }
    return; 
}

static void AddCategoryClicked(GtkButton * UNUSED(button), gpointer UNUSED(treeview))
{
    // GtkTreeIter iter;
    char *categoryName = GTKGetInput(_("Add position category"), 
        _("Position category:"), NULL);
    if(!categoryName) 
        return;
    // g_message("add=%s",categoryName);
    if (AddCategory(categoryName)) {
    // AddCategory(category);
        // gtk_list_store_append(GTK_LIST_STORE(nameStore), &iter);
        // gtk_list_store_set(GTK_LIST_STORE(nameStore), &iter, 0, categoryName, -1);
        // gtk_tree_selection_select_iter(gtk_tree_view_get_selection(GTK_TREE_VIEW(treeview)), &iter);
        // selected_iter=iter;
        ReloadQuizConsole();
    }  
    /* assuming AddCategory already gave an error message*/
    // else {
    //     GTKMessage(_("Error: problem adding this position category"), DT_INFO);
    // }

    /*if we don't refresh the game window, the right-click won't show the newly added
    category*/
    
    // ShowHidePanel(WINDOW_GAME);
    // ShowHidePanel(WINDOW_GAME);
    // GL_Create();

    // GTKRegenerateGames();
    // gtk_list_store_clear(plsGameList);

    // 
    g_free(categoryName);
    return;    
}

static void RenameCategoryClicked(GtkButton * UNUSED(button), gpointer treeview)
{
    GtkTreeIter iter;
    // GetSelectedCategoryIndex(GTK_TREE_VIEW(treeview));
    char * oldCategory = GetSelectedCategory(GTK_TREE_VIEW(treeview));
    if(!oldCategory){
        GTKMessage(_("Error: please select a position category to rename"), DT_INFO);
    }
    char *newCategory = GTKGetInput(_("Add position category"), 
        _("Position category:"), NULL);
    if(!newCategory) {
        g_free(oldCategory);
        return;
    }
    
    int positionIndex = RenameCategory(oldCategory, newCategory);

    gtk_list_store_remove(GTK_LIST_STORE(nameStore), &selected_iter);
    gtk_list_store_insert(GTK_LIST_STORE(nameStore), &iter, positionIndex);
    gtk_list_store_set(GTK_LIST_STORE(nameStore), &iter, 0, newCategory, -1);
    selected_iter=iter;

    ReloadQuizConsole();

    g_free(newCategory);
    g_free(oldCategory);
}

static void
DeleteCategoryClicked(GtkButton * UNUSED(button), gpointer treeview)
{

    int categoryIndex = GetSelectedCategoryIndex(GTK_TREE_VIEW(treeview));
    if(categoryIndex<0 || categoryIndex>=numCategories) {
        GTKMessage(_("Error: you forgot to select a position category"), DT_INFO);
        return;
    }
    if (!GTKGetInputYN(_("Are you sure?"))) { 
        GTKMessage(_("Error: problem deleting this position category"), DT_INFO);
        return;
    }
    /*just to update the selected_iter...*/
    GetSelectedCategory(GTK_TREE_VIEW(treeview));
    // char *categoryName =g_strdup(GetSelectedCategory(GTK_TREE_VIEW(treeview)));
    // g_message("categoryName=%s",categoryName);
    gtk_list_store_remove(GTK_LIST_STORE(nameStore), &selected_iter);

    if (!DeleteCategory(categoryIndex)) {
        return;
    }


    ReloadQuizConsole();
    // DisplayCategories();
    
        return;
}

/* secret formula with secret sauce for quiz*/
static void updatePriority(quiz * pq, long int seconds) {
    pq->priority = (pq->ewmaError + SMALL_ERROR) * (float) (seconds-pq->lastSeen);
    // pq->priority = (pq->ewmaError + SMALL_ERROR) * (pq->ewmaError + SMALL_ERROR) * (float) (seconds-pq->lastSeen);
    // return ( (q.ewmaError + SMALL_ERROR) * (q.ewmaError + SMALL_ERROR) * (float) (seconds-q.lastSeen));
}

extern void LoadPositionAndStart (void) {
    gchar *quizTitle;

    // fInQuizMode=TRUE;
    qDecision=QUIZ_UNKNOWN;
    // sprintf(name0BeforeQuiz, "%s", ap[0].szName);
    // sprintf(name1BeforeQuiz, "%s", ap[1].szName);

    /*this should not happen as it was previously checked before calling the function:*/
    if(qLength<0) {
        GTKMessage(_("Error: no positions in this category"), DT_INFO);
        QuizConsole();
        return;
    }

    long int seconds=(long int) (time(NULL));
    // float maxPriority=0;
    iOpt=0;
    updatePriority(&(q[iOpt]),seconds);
    for (int i = 1; i < qLength; ++i) {
        /* heuristic formula, the "secret sauce"...
        1) Very roughly: a 2x bigger typical error should be seen 2x more often -
        and then the error hopefully goes down.
        2) If we add a non-analyzed position, or a position with 0 error, then it will never appear in the loop 
        unless we add something to get a non-null priority 
        */
        updatePriority(&(q[i]),seconds);
        // q[i].priority=(q[i].ewmaError + SMALL_ERROR) * (q[i].ewmaError + SMALL_ERROR) * (float) (seconds-q[i].lastSeen); // /1000.0;
        // g_message("priority %.3f <-- %.3f, %.3f, %ld\n", q[i].priority, q[i].ewmaError, (float) (seconds-q[i].lastSeen)/1000.0,q[i].lastSeen);
        if(q[i].priority>q[iOpt].priority) {
            // maxPriority=q[i].priority;
            iOpt=i;
        }
    }
    iOptCounter=0;
    // g_message("iOpt=%d: priority %.3f <-- error %.3f, delta t %.3f, %ld\n", 
    //     iOpt,
    //     q[iOpt].priority, q[iOpt].ewmaError, (float) (seconds-q[iOpt].lastSeen),
    //     q[iOpt].lastSeen);

    // qNow=q[iOpt];

    // // the compiler states that the following is never false => removed
    // if(!q[iOpt].position){
    //     char buf[100];
    //     sprintf(buf, _("Error: wrong position in line %d of file?"), iOpt+1);
    //     GTKMessage(_(buf), DT_INFO);
    //     return;
    // }

    /* start quiz-mode play! */
    // fInQuizMode=TRUE;
    // g_message("names: %s %s",ap[0].szName,ap[1].szName);
  
    CommandSetGNUbgID(q[iOpt].position); 

    // g_message("fDoubled=%d, fMove=%d, fTurn=%d, recorderdplayer=%d",
    //         ms.fDoubled, ms.fMove, ms.fTurn, q[iOpt].player);
    if (q[iOpt].player==0) {
    // if (q[iOpt].player==0 && !(q[iOpt].set==QUIZ_P_T)) {
        // |        (q[iOpt].player==0 &&  (q[iOpt].set==QUIZ_P_T))) {
    // if(ms.fTurn == 0) {
        // g_message("swap!");
        // SwapSides(ms.anBoard);
        CommandSwapPlayers(NULL);   
        // sprintf(ap[1].szName, "%s", name1BeforeQuiz);
        // g_message("names: %s %s",ap[0].szName,ap[1].szName);
    }

    // char name0InQuiz[MAX_NAME_LEN]="Quiz Opponent";
    // // char name1InQuiz[MAX_NAME_LEN];
    // if(strcmp(ap[0].szName,name0InQuiz))
    //     strncpy(ap[0].szName, name0InQuiz, MAX_NAME_LEN);
    // if(strcmp(ap[1].szName,name1BeforeQuiz))
    //     strncpy(ap[1].szName, name1BeforeQuiz, MAX_NAME_LEN);
    // g_message("Starting quiz mode: ap names: (%s,%s) vs: (%s,%s)",
    //     ap[0].szName,ap[1].szName,name0BeforeQuiz,name1BeforeQuiz);        

    /* this changes the state to an unlimited (money) game if the option is checked */
    if (fQuizAtMoney)
        edit_new(0);
    UserCommand2("set player 0 human");
    UserCommand2("set player 1 human");

    // g_message("names: %s %s",ap[0].szName,ap[1].szName);

    // if (ap[ms.fTurn].pt != PLAYER_HUMAN) {
        // g_message("player=%d",q[iOpt].player);

    // g_message("Pre-double: fDoubled=%d, fMove=%d, fTurn=%d, recorderdplayer=%d",
    //     ms.fDoubled, ms.fMove, ms.fTurn, q[iOpt].player);
    // CommandPlay(""); //<-if opponent is computer; but what if its right 
                        //decision is not to double? We need a double
    /* User is player 1.
    When player 1 has a take, we need player 0 to double, else 1 cannot play. 
    In such a case:
    Before doubling: fMove=0,fTurn=0 (and fDoubled=0)
    After doubling: fMove=0,fTurn=1 (and fDoubled=0)

    Intuitive idea:
        if(ms.fTurn ==0) { <T/P decision>}
        else if(ms.anDice[0]>0) {<move decision>}
        else {<D/ND decision>}

    Problem:
        P/T decisions:  fDoubled=0, fMove=1, fTurn=1
        D/ND decisions: fDoubled=0, fMove=1, fTurn=1 
        => the same!

    Reason:  When we have a P/T decision, gnubg doesn't want to set it up, and 
    insists to go back to the double decision. 
    
    Solution: We define a quiz position "set" property to tell us what type of 
    decision it is.
    */
    // g_message("iopt ID:%d",q[iOpt].player);
    if(q[iOpt].set==QUIZ_P_T) { /* P / T decision */
        skipDoubleHint=1;
        CommandDouble(NULL);
        CommandSwapPlayers(NULL);   
        quizTitle =g_strdup(_("Quiz position: TAKE or PASS?"));
    } else if (q[iOpt].set==QUIZ_M) {  /* move decision*/
        skipDoubleHint=0;
        quizTitle =g_strdup(_("Quiz position: BEST MOVE?"));
    } else {   /* D / ND decision */
        skipDoubleHint=0;
        quizTitle=g_strdup(_("Quiz position: DOUBLE or NO-DOUBLE?"));
    }
    StatusBarMessage(quizTitle);
#if defined(USE_GTK)
    if (fX) {
        gtk_window_set_title(GTK_WINDOW(pwMain), quizTitle);
    }
#endif   
    g_free(quizTitle);
    char buf[100];
    /* Problem if we attempt to assign a used name, so picking a 
    random temp name momentarily */
    // sprintf(buf,"set player %d name temp-delete",1);
    UserCommand2("set player 1 name temp-delete");
    UserCommand2("set player 0 name QuizOpponent");
    // UserCommand2(buf);    
    // sprintf(buf,"set player %d name QuizOpponent",0);
    // UserCommand2("set player 0 name QuizOpponent");
    // UserCommand2(buf);
    sprintf(buf,"set player 1 name %s",name1BeforeQuiz);
    UserCommand2(buf);


    // g_message("double");
    //  g_message("Post: fDoubled=%d, fMove=%d, fTurn=%d, recorderdplayer=%d",
    //     ms.fDoubled, ms.fMove, ms.fTurn, q[iOpt].player);
    // gtk_statusbar_push(GTK_STATUSBAR(pwStatus), idOutput, _("Your turn to play this quiz position!"));

}


static GtkWidget * BuildCategoryList(void) {
   // GtkWidget *pwQuiz;
    GtkWidget *treeview;
    GtkCellRenderer *renderer;
    GtkTreeViewColumn *column;
    // GtkListStore *store;
    GtkTreeIter iter;
    // GtkTreeViewColumn   *col;
 
    // DisplayCategories();

    /* We always check the positions again because the user may have added positions since the
    gnubg start-up */
    // if(!categories[0].name) {
    //     g_message("The categories array was expected to be initialized at start-up...");
    GetPositionCategories();
    // }

    /* create list store */

    nameStore = gtk_list_store_new(N_COLUMNS, G_TYPE_INT, G_TYPE_STRING, G_TYPE_INT);

    for(int i=0;i < numCategories; i++) {
        gtk_list_store_append(nameStore, &iter);
        // int numberPositions=CountPositions(categories[i]);
        gtk_list_store_set(nameStore, &iter, 
                            COLUMN_INDEX,i,
                            COLUMN_STRING, categories[i].name,
                            COLUMN_INT, categories[i].number,
                            -1);
        // g_message("in DisplayCategories: %d->%s", i,categories[i]);
    }

    /* create tree view */
    treeview = gtk_tree_view_new_with_model(GTK_TREE_MODEL(nameStore));

    g_object_unref(nameStore);

    renderer = gtk_cell_renderer_text_new();
    column = gtk_tree_view_column_new_with_attributes(_("#"), 
        renderer, "text", COLUMN_INDEX, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(treeview), column);

    renderer = gtk_cell_renderer_text_new();
    column = gtk_tree_view_column_new_with_attributes(_("Category"), 
        renderer, "text", COLUMN_STRING, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(treeview), column);

    renderer = gtk_cell_renderer_text_new();
    column = gtk_tree_view_column_new_with_attributes(_("Num. positions"), 
        renderer, "text", COLUMN_INT, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(treeview), column);

    return treeview;
}
// static int eventCounter=0;

// static gboolean QuizManageReleased(GtkWidget * UNUSED(widget), GdkEventButton  * event, GtkWidget * myTreeView)
// {
//     switch (event->type) {
//     // case GDK_BUTTON_PRESS:
//     //     fScrollComplete = FALSE;
//     //     break;
//     case GDK_BUTTON_RELEASE:
//         // fScrollComplete = TRUE;
//         // g_signal_emit_by_name(G_OBJECT(prw->pScale), "value-changed", prw);
//         // g_message("released");
//         g_message("released time=%d",event->time);

        
//         currentCategoryIndex = GetSelectedCategoryIndex(GTK_TREE_VIEW(myTreeView));
//         g_message("currentCategoryIndex=%d",currentCategoryIndex);
//         // gtk_widget_destroy(gtk_widget_get_toplevel(pw));
//         if(currentCategoryIndex<0 || currentCategoryIndex>=numCategories) {
//             // GTKMessage(_("Error: you forgot to select a position category"), DT_INFO);
//             // QuizConsole();
//             return FALSE;
//         }
//         break;
//     default:
//         ;
//     }

//     return FALSE;
// }

static void TurnOnQuizMode(void){
    fInQuizMode=TRUE;
    
    if(strcmp(ap[0].szName,name0BeforeQuiz))
        // name0BeforeQuiz=g_strdup(ap[0].szName);
        strncpy(name0BeforeQuiz, ap[0].szName, MAX_NAME_LEN);
    if(strcmp(ap[1].szName,name1BeforeQuiz))
        strncpy(name1BeforeQuiz, ap[1].szName, MAX_NAME_LEN);
    // g_message("After TurnOn: ap names: (%s,%s) vs: (%s,%s)",
    //     ap[0].szName,ap[1].szName,name0BeforeQuiz,name1BeforeQuiz);
    
    type0BeforeQuiz=ap[0].pt;
    ap[0].pt = PLAYER_HUMAN;

    fTutorBeforeQuiz=fTutor;
    fTutor=FALSE;
}
extern void TurnOffQuizMode(void){
    fInQuizMode=FALSE;
    // g_message("before TurnOff: ap names: (%s,%s) vs: (%s,%s)",
        // ap[0].szName,ap[1].szName,name0BeforeQuiz,name1BeforeQuiz);
    // if(strcmp(ap[0].szName,name0BeforeQuiz))
    //     // sprintf(ap[0].szName, "%s",name0BeforeQuiz);
    //     strncpy(ap[0].szName, name0BeforeQuiz, MAX_NAME_LEN);
    // if(strcmp(ap[1].szName,name1BeforeQuiz))
    //     strncpy(ap[1].szName, name1BeforeQuiz, MAX_NAME_LEN);
    // g_message("TurnOff: ap names: (%s,%s) vs: (%s,%s)",
    //     ap[0].szName,ap[1].szName,name0BeforeQuiz,name1BeforeQuiz);
    char buf[100];
    sprintf(buf,"set player 0 name %s",name0BeforeQuiz);
    UserCommand2(buf);
    sprintf(buf,"set player 1 name %s",name1BeforeQuiz);
    UserCommand2(buf);
    // g_message("after TurnOff: ap names: (%s,%s) vs: (%s,%s)",
    // ap[0].szName,ap[1].szName,name0BeforeQuiz,name1BeforeQuiz);
    
    ap[0].pt = type0BeforeQuiz;

    fTutor=fTutorBeforeQuiz;
}

static void StartQuiz(GtkWidget * UNUSED(pw), GtkTreeView * treeview) {
    // g_message("in StartQuiz");
    // outputerrf("in StartQuiz");

    // currentCategory = GetSelectedCategory(treeview);
    // if(!currentCategory) {
    //     GTKMessage(_("Error: you forgot to select a position category"), DT_INFO);
    //     QuizConsole();
    //     return;
    // }

    currentCategoryIndex = GetSelectedCategoryIndex(GTK_TREE_VIEW(treeview));
    // g_message("currentCategoryIndex=%d",currentCategoryIndex);
    // gtk_widget_destroy(gtk_widget_get_toplevel(pw));
    if(currentCategoryIndex<0 || currentCategoryIndex>=numCategories) {
        GTKMessage(_("Error: you forgot to select a position category"), DT_INFO);
        // QuizConsole();
        return;
    }
    if(!fInQuizMode)
        TurnOnQuizMode();
        
    /* the following function:
    - updates positionsFileFullPath,
    - opens the corrsponding file, 
    - and saves the parsing results in the categories static array */
    // int result= OpenQuizPositionsFile(currentCategoryIndex);
    OpenQuizPositionsFile(currentCategoryIndex);
    // g_free(currentCategory); //when should we free a static var that is to be reused?

    /* no need for message, the OpenQuizPositionsFile already gives it*/
    // /*empty file, no positions*/
    // if(result==0) {
    //     GTKMessage(_("Error: problem with the file of this category"), DT_INFO);
    //     // QuizConsole();
    //     return;
    // }

    /*start!*/
    DestroyQuizDialog(NULL,NULL);

    // // sprintf(name0BeforeQuiz, "%s", ap[0].szName);
    // // sprintf(name1BeforeQuiz, "%s", ap[1].szName);
    // name0BeforeQuiz=g_strdup(ap[0].szName);
    // name1BeforeQuiz=g_strdup(ap[1].szName);

    counterForFile=0; /*first one for this category in this round*/
    LoadPositionAndStart();
}

static void QuizManageClicked(GtkWidget * UNUSED(widget), GdkEventButton  * event, GtkTreeView * treeview) {
    
    // g_message("before: currentCategoryIndex=%d",currentCategoryIndex);
    currentCategoryIndex = GetSelectedCategoryIndex(GTK_TREE_VIEW(treeview));
    // g_message("after: currentCategoryIndex=%d",currentCategoryIndex);

    /*double-click, e.g. w/ left or middle click, not right*/
    if (event->type == GDK_2BUTTON_PRESS  &&  event->button != 3){
        // g_message("double-click time=%d",event->time);
        // gtk_widget_destroy(gtk_widget_get_toplevel(pw));
        // if(currentCategoryIndex<0 || currentCategoryIndex>=numCategories) {
        //     // GTKMessage(_("Error: you forgot to select a position category"), DT_INFO);
        //     // QuizConsole();
        // }
        if(currentCategoryIndex>=0 && currentCategoryIndex<numCategories) {
            StartQuiz(NULL, treeview);
        }
    }
}
/* We disable / enable buttons depending on whether a category is 
selected. To do so, (1) here, we set these variables static.
Potential alternatives: (2) Refresh the quiz manage window when
a category is selected to enable the buttons, or 
(3) not disable the buttons at all. */
static GtkWidget *delButton;
static GtkWidget *renameButton;
static GtkWidget *startButton;

static void on_changed(GtkTreeSelection *selection, gpointer UNUSED(p))
{
    GtkTreeIter iter;
    GtkTreeModel *model;
    // int value;

    if (gtk_tree_selection_get_selected(GTK_TREE_SELECTION(selection), &model, &iter))
    {
        gtk_tree_model_get(model, &iter, COLUMN_INDEX, &currentCategoryIndex,  -1);
        // g_print("%d is selected\n", currentCategoryIndex);
        // g_free(value);
        gtk_widget_set_sensitive(startButton, (currentCategoryIndex>=0 && currentCategoryIndex<numCategories));
        gtk_widget_set_sensitive(renameButton, (currentCategoryIndex>=0 && currentCategoryIndex<numCategories));
        gtk_widget_set_sensitive(delButton, (currentCategoryIndex>=0 && currentCategoryIndex<numCategories));

    }
}

// display info on quiz feature
static void ExplanationsClicked(GtkWidget * UNUSED(widget), GtkWidget* pwParent) {
    
    GtkWidget* pwInfoDialog, * pwBox;
    // const char* pch;

    pwInfoDialog = GTKCreateDialog(_("Quiz Info"), DT_INFO, pwParent, DIALOG_FLAG_MODAL, NULL, NULL);
#if GTK_CHECK_VERSION(3,0,0)
    pwBox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
#else
    pwBox = gtk_vbox_new(FALSE, 0);
#endif
    gtk_container_set_border_width(GTK_CONTAINER(pwBox), 8);

    gtk_container_add(GTK_CONTAINER(DialogArea(pwInfoDialog, DA_MAIN)), pwBox);

    AddText(pwBox, _("The quiz feature enables you to collect and try again positions where you blundered. \
        \n\n- It is managed through the quiz console. To launch the quiz console, either: \
        \n\t (1) right-click and select it, or \
        \n\t (2) use the menu: \"Game > Quiz \", \
        \n\n- There are two ways to collect positions: \
        \n\t * AUTOMATIC: GNUBG can automatically add blundered positions to three pre-defined files, \
        \n\t\t depending on whether they correspond to (1) move, (2) double/no-double or \
        \n\t\t (3) pass/take decisions. \
        \n\t * MANUAL: You can create a category in the quiz console, and manually add a position \
        \n\t\t to this category: \
        \n\t\t (1) Either by right-clicking on the position in the (top-right) move-list window \
        \n\t\t (2) Or by launching the quiz console, then selecting the category, then clicking the \
        \n\t\t\t add-position button. \
        \n\t\t Note that sometimes, a position in the move-list window corresponds to two consecutive \
        \n\t\t decisions: roll the dice (ND=no-double) instead of doubling, then move. GNUBG enables you \
        \n\t\t to add both decisions to the quiz positions. \
        \n\n- To start playing the quiz with the positions of a given category, open the quiz console, \
        \n\t then either double-click on the category, or click once and select the play-quiz arrow. \
        \n\t GNUBG manages the behind-the-scenes algorithms that present you first the positions \
        \n\t that have bigger blunders and/or that are harder to you and/or that you haven't seen \
        \n\t for a while. \
        \n\n - You can change the quiz options in \"Settings > Options > Quiz\", e.g., to disable the \
        \n\t automatic collection of blundered positions, or to disable the quiz feature completely."));

    GTKRunDialog(pwInfoDialog);
}

/*"Quiz console" = central management window for the quiz feature */

extern void QuizConsole(void) {

    GtkWidget *pwScrolled;
    GtkWidget *pwMainHBox;
    GtkWidget *pwVBox;
    GtkWidget *addButton; /*and more buttons are in static outside the function*/
    GtkWidget *helpButton;
    GtkWidget *pwMainVBox;
    GtkWidget *addPos1Button;
    GtkWidget *addPos2Button;

    // g_message("start of console");

    currentCategoryIndex=-1;
    /* [not relevant anymore?
    "putting true means that we need to end it when we leave by using the close button and
    only for this screen; so we put false by default for now"] */
    if(fInQuizMode) {
        // g_message("console: in quiz mode");
        TurnOffQuizMode();
        // UserCommand2("new match");
    }

    GtkWidget *treeview = BuildCategoryList();

    pwScrolled = gtk_scrolled_window_new(NULL, NULL);

    if (pwQuiz) { //i.e. we didn't close it using DestroyQuizDialog()
        // g_message("start of console: non-zero pwQuiz");
        gtk_widget_destroy(gtk_widget_get_toplevel(pwQuiz));
        pwQuiz = NULL;
    }

#if GTK_CHECK_VERSION(3,0,0)
    pwQuiz = (GtkWindow*)gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_default_size (pwQuiz, WIDTH, HEIGHT);
    gtk_window_set_position     (pwQuiz, GTK_WIN_POS_CENTER);
    gtk_window_set_title        (pwQuiz, "Quiz console");
    g_signal_connect(pwQuiz, "destroy", G_CALLBACK(gtk_widget_destroy), NULL);
    gtk_widget_show_all ((GtkWidget*)pwQuiz);
#else
    /*  V1: non-modal, looks nicer
        V2: non-modal causes the fact that we cannot re-open the window after 
        closing it. Back to modal.
    */
    pwQuiz = GTKCreateDialog(_("Quiz console"), DT_INFO, NULL, 
        // DIALOG_FLAG_NOOK, NULL, NULL);
        // DIALOG_FLAG_NONE, NULL, NULL);
        // DIALOG_FLAG_NONE, (GCallback) StartQuiz, treeview);
        // DIALOG_FLAG_MODAL | DIALOG_FLAG_CLOSEBUTTON,(GCallback) StartQuiz, 
        //     GTK_TREE_VIEW(treeview));
        DIALOG_FLAG_MODAL | DIALOG_FLAG_CLOSEBUTTON, NULL, NULL);
        // DIALOG_FLAG_MODAL | DIALOG_FLAG_CLOSEBUTTON, NULL, treeview);
    //    DIALOG_FLAG_NONE
    gtk_window_set_default_size (GTK_WINDOW (pwQuiz), WIDTH, 350);
    GdkColor color;
    gdk_color_parse ("#EDF5FF", &color);
    gtk_widget_modify_bg(pwQuiz, GTK_STATE_NORMAL, &color);
    g_signal_connect(G_OBJECT(pwQuiz), "destroy", G_CALLBACK(DestroyQuizDialog), NULL);

    // color.red = 0xcfff;
    // color.green = 0xdfff;
    // color.blue = 0xefff;
    // gdk_color_parse ("red", &color);
    // gdk_color_parse ("black", &color);
    //  gdk_color_parse (BGCOLOR, &color);    
    // gtk_window_set_title(GTK_WINDOW(pwQuiz), "hello");
    // if (gdk_color_parse("#c0deed", &color)) {
    // } else {
    //     gtk_widget_modify_bg(pwQuiz, GTK_STATE_NORMAL, &color);
    // }
    // gtk_widget_modify_bg(pwMain, GTK_STATE_NORMAL, &color);
    // gtk_widget_show_all(pwQuiz);
#endif
#if GTK_CHECK_VERSION(3,0,0)
    pwMainVBox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 2);
#else
    pwMainVBox = gtk_vbox_new(FALSE, 2);
#endif

    gtk_container_add(GTK_CONTAINER(DialogArea(pwQuiz, DA_MAIN)), pwMainVBox);

   AddText(pwMainVBox, _("\nSelect a category to start playing"));

#if GTK_CHECK_VERSION(3,0,0)
    pwMainHBox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 2);
#else
    pwMainHBox = gtk_hbox_new(FALSE, 2);
#endif
    gtk_box_pack_start(GTK_BOX(pwMainVBox), pwMainHBox, FALSE, FALSE, 0);
    // gtk_container_add(GTK_CONTAINER(DialogArea(pwQuiz, DA_MAIN)), pwMainHBox);
    gtk_box_pack_start(GTK_BOX(pwMainHBox), pwScrolled, TRUE, TRUE, 0);
    gtk_widget_set_size_request(pwScrolled, 100, 200);//-1);
#if GTK_CHECK_VERSION(3,0,0)
    pwVBox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 2);
#else
    pwVBox = gtk_vbox_new(FALSE, 2);
#endif
    gtk_box_pack_start(GTK_BOX(pwMainHBox), pwVBox, FALSE, FALSE, 0);
    // gtk_container_set_border_width(GTK_CONTAINER(pwVBox), 8);

#if GTK_CHECK_VERSION(3, 8, 0)
    gtk_container_add(GTK_CONTAINER(pwScrolled), treeview);
#else
    gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(pwScrolled), treeview);
#endif
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(pwScrolled), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

    GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(treeview));
    g_signal_connect(selection, "changed", G_CALLBACK(on_changed), NULL);

// #if GTK_CHECK_VERSION(3,0,0)
//     hb1 = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
// #else
//     hb1 = gtk_hbox_new(FALSE, 0);
// #endif
    // gtk_box_pack_start(GTK_BOX(pwVBox), hb1, FALSE, FALSE, 0);


    if(numCategories>0) {
        /*first, if single position to add from current board*/
        if(qNow_NDBeforeMoveError <-0.001) {
            addPos1Button = gtk_button_new_with_label(_("Add position to category"));
            g_signal_connect(addPos1Button, "clicked", G_CALLBACK(AddPositionClicked), GTK_TREE_VIEW(treeview));
            gtk_box_pack_start(GTK_BOX(pwVBox), addPos1Button, FALSE, FALSE, 0);
            gtk_widget_set_sensitive(addPos1Button, (currentCategoryIndex>=0 && currentCategoryIndex<numCategories));
        } else {
            /*now it means there is a no-double event before a move event,
            so we need to allow users to add both*/
            addPos1Button = gtk_button_new_with_label(_("Add move decision to category"));
            g_signal_connect(addPos1Button, "clicked", G_CALLBACK(AddPositionClicked), GTK_TREE_VIEW(treeview));
            gtk_box_pack_start(GTK_BOX(pwVBox), addPos1Button, FALSE, FALSE, 0);
            gtk_widget_set_sensitive(addPos1Button, (currentCategoryIndex>=0 && currentCategoryIndex<numCategories));

            addPos2Button = gtk_button_new_with_label(_("Add double/no-double decision to category"));
            g_signal_connect(addPos2Button, "clicked", G_CALLBACK(AddNDPositionClicked), GTK_TREE_VIEW(treeview));
            gtk_box_pack_start(GTK_BOX(pwVBox), addPos2Button, FALSE, FALSE, 0);
            gtk_widget_set_sensitive(addPos2Button, (currentCategoryIndex>=0 && currentCategoryIndex<numCategories));
        }
    }

#if GTK_CHECK_VERSION(3,0,0)
    gtk_box_pack_start(GTK_BOX(pwVBox), gtk_separator_new(GTK_ORIENTATION_HORIZONTAL), FALSE, FALSE, 2);
#else
    gtk_box_pack_start(GTK_BOX(pwVBox), gtk_hseparator_new(), FALSE, FALSE, 2);
#endif

    addButton = gtk_button_new_with_label(_("Add category"));
    g_signal_connect(addButton, "clicked", G_CALLBACK(AddCategoryClicked), GTK_TREE_VIEW(treeview));
    gtk_box_pack_start(GTK_BOX(pwVBox), addButton, FALSE, FALSE, 0);
 
    renameButton = gtk_button_new_with_label(_("Rename category"));
    g_signal_connect(renameButton, "clicked", G_CALLBACK(RenameCategoryClicked), GTK_TREE_VIEW(treeview));
    gtk_box_pack_start(GTK_BOX(pwVBox), renameButton, FALSE, FALSE, 0);
 
    delButton = gtk_button_new_with_label(_("Delete category"));
    g_signal_connect(delButton, "clicked", G_CALLBACK(DeleteCategoryClicked), GTK_TREE_VIEW(treeview));
    gtk_box_pack_start(GTK_BOX(pwVBox), delButton, FALSE, FALSE, 4);

#if GTK_CHECK_VERSION(3,0,0)
    gtk_box_pack_start(GTK_BOX(pwVBox), gtk_separator_new(GTK_ORIENTATION_HORIZONTAL), FALSE, FALSE, 2);
#else
    gtk_box_pack_start(GTK_BOX(pwVBox), gtk_hseparator_new(), FALSE, FALSE, 2);
#endif

    helpButton = gtk_button_new_with_label(_("Explanations"));
    g_signal_connect(helpButton, "clicked", G_CALLBACK(ExplanationsClicked), pwQuiz);
    gtk_box_pack_start(GTK_BOX(pwVBox), helpButton, FALSE, FALSE, 4);
    gtk_widget_set_tooltip_text(helpButton, _("Click to obtain more explanations on the quiz mode")); 

    // startButton = gtk_button_new_from_stock(GTK_STOCK_GO_FORWARD);
    startButton = gtk_button_new(); 
    //gtk_button_new_with_label(_("Start quiz!"));
    // button = gtk_button_new();
    // GtkWidget *image = gtk_image_new_from_stock(GTK_STOCK_GO_FORWARD, GTK_ICON_SIZE_DIALOG);
    GtkWidget *image = gtk_image_new_from_stock(GTK_STOCK_GO_FORWARD, GTK_ICON_SIZE_BUTTON);
    // gtk_button_set_use_stock (GTK_BUTTON(startButton), FALSE);
    gtk_button_set_image(GTK_BUTTON(startButton), image);
    // gtk_button_set_label(GTK_BUTTON(startButton), _("Start quiz!"));
    // gtk_button_set_always_show_image (GTK_BUTTON (button), TRUE);
    // gtk_widget_set_sensitive(startButton, (&selected_iter!=NULL));

    // g_message("in window: currentCategoryIndex=%d",currentCategoryIndex);
    
    gtk_widget_set_sensitive(startButton, (currentCategoryIndex>=0 && currentCategoryIndex<numCategories));
    gtk_widget_set_sensitive(renameButton, (currentCategoryIndex>=0 && currentCategoryIndex<numCategories));
    gtk_widget_set_sensitive(delButton, (currentCategoryIndex>=0 && currentCategoryIndex<numCategories));
    // gtk_button_set_relief(GTK_BUTTON(startButton), GTK_RELIEF_NONE);

    g_signal_connect(startButton, "clicked", G_CALLBACK(StartQuiz), GTK_TREE_VIEW(treeview));
    gtk_box_pack_start(GTK_BOX(pwMainVBox), startButton, TRUE, TRUE, 10);
    // gtk_dialog_add_button(GTK_DIALOG(pwQuiz), _("Start quiz!"),
    //                           GTK_RESPONSE_YES);
    // gtk_dialog_set_default_response(GTK_DIALOG(pwQuiz), GTK_RESPONSE_YES);

    g_signal_connect(G_OBJECT(treeview), "button-press-event", G_CALLBACK(QuizManageClicked), GTK_TREE_VIEW(treeview));
    // g_signal_connect(G_OBJECT(treeview), "button-release-event", G_CALLBACK(QuizManageReleased), treeview);
    
    // g_message("end of console");
    // g_object_weak_ref(G_OBJECT(pwQuiz), DestroyQuizDialog, NULL);

    GTKRunDialog(pwQuiz);
}

// /* Now we are back from the hint function within quiz mode. 
// - The position category is well known. 
// - The values have already been updated and saved in a file 
//         (in MoveListUpdate->qUpdate->SaveFullPositionFile)
// We need to ask the player whether to play again within this category.
//     (YES) We want to pick a new position in the category and start
//     (NO) We get back to the main panel in case the user wants to 
//             change category.
// */
// extern void BackFromHint (void) {
//     char buf[200];
//     // counterForFile++;
//     sprintf(buf,_("%d positions played in category %s (which has %d positions)."
//         "\n\n Play another position?"), counterForFile,categories[currentCategoryIndex].name,
//         categories[currentCategoryIndex].number);

//     if (GTKGetInputYN(buf)) {
//         LoadPositionAndStart();
//     } else {
//         fInQuizMode=FALSE;
//         QuizConsole();
//     }
// }




/* ************ END OF QUIZ ************************** */


/* ************** right-click quiz menu **************  */

// static    GtkWidget *menu_item;



static void QuizConsoleClicked(GtkWidget * UNUSED(pw), gpointer UNUSED(userdata)) {
    QuizConsole();
}

// /* inspired by https://docs.gtk.org/gtk3/treeview-tutorial.html*/
// void
// view_popup_menu_onDoSomething (GtkWidget *menuitem,
//                                gpointer userdata)
// {
//   /* we passed the view as userdata when we connected the signal */
//   GtkTreeView *treeview = GTK_TREE_VIEW(userdata);

//   g_print ("Do something!\n");
// }


static void BuildQuizMenu(GdkEventButton *event){
    pQuizMenu = gtk_menu_new(); /*extern*/
    GtkWidget *menu_item;
    char buf[MAX_CATEGORY_NAME_LENGTH+30];



    // g_message("BuildQuizMenu: numCategories=%d,qNow_NDBeforeMoveError=%f,globalCounter=%d",
    //     numCategories,qNow_NDBeforeMoveError,globalCounter);
    // gtk_image_menu_item_set_always_show_image(menu_item,TRUE);


    /*Convoluted setting: by default right-click gets first, then left-click.
    But we want left-click to go first, select the row, then compute whether
    to display a menu with "no-double before move" or not, and only then
    display the right-click menu. 
    On the other hand, we don't want a menu with left-click only.
    So we use a globalCounter:
    right-click=>globalCounter=1
        =>go to left-click and select row=>globalCounter=2
        =>come back to right-click and display menu=>reset globalCounter=0
     */
    if(globalCounter>1){
        menu_item = gtk_menu_item_new_with_label(_("Start quiz! (quiz console)"));
        gtk_menu_shell_append(GTK_MENU_SHELL(pQuizMenu), menu_item);
        gtk_widget_show(menu_item);
        g_signal_connect(G_OBJECT(menu_item), "activate", 
            G_CALLBACK(QuizConsoleClicked), NULL); 
        if(numCategories>0) {
            /*separator*/
            menu_item = gtk_menu_item_new();
            gtk_menu_shell_append(GTK_MENU_SHELL(pQuizMenu), menu_item);
            gtk_widget_show(menu_item);
            /*first set of categories*/
            for(int i=0;i < numCategories; i++) {
                sprintf(buf,_("+ %s"),categories[i].name);
                // sprintf(buf,_("Add to: %s"),categories[i].name);
                menu_item = gtk_menu_item_new_with_label(_(buf));
                gtk_menu_shell_append(GTK_MENU_SHELL(pQuizMenu), menu_item);
                gtk_widget_show(menu_item);
                g_signal_connect_swapped(G_OBJECT(menu_item), "activate", 
                    G_CALLBACK(AddPositionToFile), &(categories[i])); 
            }    
            /* If there was a no-double=roll event just before the move, we need to 
            be able to add it as well. It's a problem because the gamelist doesn't 
            really show it. Now we offer the user the option to pick it.

            Non-implemented potential future alternatives:
            (A) Let the user right-click on the cube decision
            panel. But it's not clear that this is better, because then the user
            (1) needs to know he can do it, and (2) needs to start clicking around. 
            (B) Only present each category once, but when a user picks a category,
            a window asks whether to add the ND decision or the move decision.  */
            // g_message("Build:  qNowND:%f",qNow_NDBeforeMoveError);
            /*second set of categories for ND before move*/
            if(qNow_NDBeforeMoveError >-0.001) {
                // g_message("creating 2nd part of popup");
                /*separator*/
                menu_item = gtk_menu_item_new();
                gtk_menu_shell_append(GTK_MENU_SHELL(pQuizMenu), menu_item);
                gtk_widget_show(menu_item);
                /*add the no-double decision*/
                for(int i=0;i < numCategories; i++) {
                    // char buf[MAX_CATEGORY_NAME_LENGTH+8];
                    sprintf(buf,_("(ND) + %s"),categories[i].name);
                    menu_item = gtk_menu_item_new_with_label(_(buf));
                    gtk_menu_shell_append(GTK_MENU_SHELL(pQuizMenu), menu_item);
                    gtk_widget_show(menu_item);
                    g_signal_connect_swapped(G_OBJECT(menu_item), "activate", 
                        G_CALLBACK(AddNDPositionToFile), &(categories[i])); 
                }    
            }
        }
        gtk_widget_show_all(pQuizMenu);

        /* Note: event can be NULL here when called from view_onPopupMenu;
        * gdk_event_get_time() accepts a NULL argument
        */
        if(event != NULL){
            // g_message("popup event!");
            gtk_menu_popup(GTK_MENU(pQuizMenu), NULL, NULL, NULL, NULL,
                            (event != NULL) ? event->button : 0,
                            gdk_event_get_time((GdkEvent*)event));
        }                    
    } 
// else
//         LastEvent=event;
}

// void
// view_popup_menu (GtkWidget *treeview, GdkEventButton *event, gpointer userdata) 
// {
//     // GtkWidget *menu;
//     // GtkWidget *menu_item;
//     // menu = gtk_menu_new();
//     // int tempArray [numCategories];
//     // g_message("numCategories=%d",numCategories);
//     // // gtk_image_menu_item_set_always_show_image(menu_item,TRUE);
//     // for(int i=0;i < numCategories; i++) {
//     //     tempArray[i]=i;
//     //     char buf[MAX_CATEGORY_NAME_LENGTH+8];
//     //     sprintf(buf,_("Add to: %s"),categories[i].name);
//     //     menu_item = gtk_menu_item_new_with_label(_(buf));
//     //     gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
//     //     gtk_widget_show(menu_item);
//     //     // g_signal_connect_swapped(G_OBJECT(menu_item), "activate", G_CALLBACK(AddPositionToFile), 
//     //     //     &(categories[i])); //(tempArray[i]));
//     //     g_signal_connect(menuitem, "activate",
//     //                (GCallback) view_popup_menu_onDoSomething, treeview);

//     // }    
//     GtkWidget *menu, *menuitem;
//     menu = gtk_menu_new();
//     menuitem = gtk_menu_item_new_with_label("Do something");
//     g_signal_connect(menuitem, "activate",
//                     (GCallback) view_popup_menu_onDoSomething, treeview);
//     gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);
//     menuitem = gtk_menu_item_new_with_label("Do something2");
//     g_signal_connect(menuitem, "activate",
//                     (GCallback) view_popup_menu_onDoSomething, treeview);
//     gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);
    
//     gtk_widget_show_all(menu);
//     /* Note: event can be NULL here when called from view_onPopupMenu;
//     * gdk_event_get_time() accepts a NULL argument
//     */
//     gtk_menu_popup(GTK_MENU(menu), NULL, NULL, NULL, NULL,
//                     (event != NULL) ? event->button : 0,
//                     gdk_event_get_time((GdkEvent*)event));
// }

// // int show_popup(GtkWidget *widget, GdkEvent *event,gpointer *treeview) {
// int show_popup(GtkTreeView *widget, GdkEvent *event) {

//     const gint RIGHT_CLICK = 3;
//     // g_message("I'm in the show popup func");
//             //   GameListSelectRow(treeview, NULL);
//     if (event->type == GDK_BUTTON_PRESS) {
//         // g_message("button press");
      
//         GdkEventButton *bevent = (GdkEventButton *) event;
      
//         if (bevent->button == RIGHT_CLICK) {      
//             // g_message("right button press");          
//             gtk_menu_popup(GTK_MENU(widget), NULL, NULL, NULL, NULL,
//                     bevent->button, bevent->time);
//             //   GameListSelectRow(treeview, NULL);
//             return FALSE; //<-- to have the menu with usual clicks!!!
//         }
//       return FALSE;
//   } else 
//         return FALSE;
// }

static gboolean
view_onButtonPressed (GtkWidget *UNUSED(treeview),
                      GdkEventButton *event,
              gpointer UNUSED(userdata))
{

    // GameListSelectRow(GTK_TREE_VIEW(pwGameList), NULL);

    /* single click with the right mouse button? */
    if (event->type == GDK_BUTTON_PRESS  &&  event->button == 3){
    // if (event->type == GDK_BUTTON_PRESS) {
    //     GdkEventButton *bevent = (GdkEventButton *) event;     
    //     if (bevent->button == 3) {         
        // g_print ("Single right click on the tree view.\n");
        
        // CalculateBoard();
        // ShowBoard();
        // GameListSelectRow(GTK_TREE_VIEW(pwGameList), NULL);
        //     CalculateBoard();
        // ShowBoard();
        // UserCommand2("set dockpanels on");

        // /* optional: select row if no row is selected or only
        // *  one other row is selected (will only do something
        // *  if you set a tree selection mode as described later
        // *  in the tutorial) */
        // if (0){            
        //     GtkTreeSelection *selection;

        //     selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(pwGameList));

        //     /* Note: gtk_tree_selection_count_selected_rows() does not
        //     *   exist in gtk+-2.0, only in gtk+ >= v2.2 ! */
        //     if (gtk_tree_selection_count_selected_rows(selection)  <= 1){
        //         GtkTreePath *path;

        //         /* Get tree path for row that was clicked */
        //         if (gtk_tree_view_get_path_at_pos(GTK_TREE_VIEW(pwGameList),
        //                                         (gint) event->x, 
        //                                         (gint) event->y,
        //                                         &path, NULL, NULL, NULL)){
        //         gtk_tree_selection_unselect_all(selection);
        //         gtk_tree_selection_select_path(selection, path);
        //         gtk_tree_path_free(path);
        //         }
        //     }
        // } /* end of optional bit */
        LastEvent=event;
        globalCounter=1;
        BuildQuizMenu(event);
        // view_popup_menu(treeview, event, userdata);

        /*this enables it to apply also the left button and go there!*/
        return FALSE; //TRUE; //GDK_EVENT_STOP;
    }

    return FALSE; //GDK_EVENT_PROPAGATE;
}

// /* seems unused now */
// static gboolean view_onPopupMenu (GtkWidget * UNUSED(treeview), gpointer UNUSED(userdata)) {
// //   view_popup_menu(treeview, NULL, userdata);
// // g_message("view_onPopupMenu");
//     BuildQuizMenu(NULL);
//     return FALSE; //GDK_EVENT_STOP;
// }


extern GtkWidget *
GL_Create(void)
{
    static int player[] = { 0, 1 };
    GtkTreeViewColumn *column;
    GtkCellRenderer *renderer;
    gint nMaxWidth;
    PangoRectangle logical_rect;
    PangoLayout *layout;
    int i;

    plsGameList = gtk_list_store_new(GL_COL_NUM_COLUMNS, G_TYPE_INT, G_TYPE_STRING, G_TYPE_STRING,
                                     G_TYPE_POINTER, G_TYPE_POINTER, G_TYPE_OBJECT, G_TYPE_OBJECT, G_TYPE_BOOLEAN);

    pwGameList = gtk_tree_view_new_with_model(GTK_TREE_MODEL(plsGameList));
    gtk_tree_view_set_headers_clickable(GTK_TREE_VIEW(pwGameList), FALSE);
    gtk_tree_selection_set_mode(gtk_tree_view_get_selection(GTK_TREE_VIEW(pwGameList)), GTK_SELECTION_NONE);
    gtk_tree_view_set_fixed_height_mode(GTK_TREE_VIEW(pwGameList), TRUE);

    renderer = gtk_cell_renderer_text_new();
    g_object_set(renderer, "ypad", 0, NULL);
    gtk_cell_renderer_set_alignment(renderer, 1.0, 0.5);
    column = gtk_tree_view_column_new_with_attributes(_("#"), renderer, "text", GL_COL_MOVE_NUMBER, NULL);
    gtk_tree_view_column_set_alignment(column, 1.0);
    gtk_tree_view_column_set_sizing(column, GTK_TREE_VIEW_COLUMN_FIXED);
    gtk_tree_view_append_column(GTK_TREE_VIEW(pwGameList), column);

    for (i = 0; i < 2; i++) {
        renderer = gtk_cell_renderer_text_new();
        g_object_set(renderer, "ypad", 0, NULL);
        column = gtk_tree_view_column_new();
        g_object_set_data(G_OBJECT(column), "player", &player[i]);
        gtk_tree_view_column_pack_start(column, renderer, FALSE);
        gtk_tree_view_column_set_cell_data_func(column, renderer, RenderMoveString, NULL, NULL);
        gtk_tree_view_column_set_sizing(column, GTK_TREE_VIEW_COLUMN_FIXED);
        gtk_tree_view_column_set_expand(column, TRUE);
        gtk_tree_view_append_column(GTK_TREE_VIEW(pwGameList), column);
    }

    GL_SetNames();

    layout = gtk_widget_create_pango_layout(pwGameList, "99");
    pango_layout_get_pixel_extents(layout, NULL, &logical_rect);
    g_object_unref(layout);
    nMaxWidth = logical_rect.width;
    gtk_tree_view_column_set_fixed_width(gtk_tree_view_get_column(GTK_TREE_VIEW(pwGameList), 0), nMaxWidth + 8);

    layout = gtk_widget_create_pango_layout(pwGameList, " (set board AAAAAAAAAAAAAA)");
    pango_layout_get_pixel_extents(layout, NULL, &logical_rect);
    g_object_unref(layout);
    nMaxWidth = logical_rect.width;
    gtk_tree_view_column_set_fixed_width(gtk_tree_view_get_column(GTK_TREE_VIEW(pwGameList), 1), nMaxWidth - 22);
    gtk_tree_view_column_set_fixed_width(gtk_tree_view_get_column(GTK_TREE_VIEW(pwGameList), 2), nMaxWidth - 22);

    g_signal_connect(G_OBJECT(pwGameList), "cursor-changed", G_CALLBACK(GameListSelectRow), NULL);
    // GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(pwGameList));
    // g_signal_connect(selection, "changed", G_CALLBACK(NewPosition), NULL);
    if (fUseQuiz) 
        g_signal_connect(pwGameList, "button-press-event", G_CALLBACK(view_onButtonPressed), NULL);    

#if GTK_CHECK_VERSION(3,0,0)
    /* Set up styles after the widget's base GtkStyleContext has been created */
    g_signal_connect(G_OBJECT(pwGameList), "style-updated", G_CALLBACK(CreateStyles), NULL);
#else
    CreateStyles(pwGameList, NULL);
#endif

    // /* list view (selections) */
    // GtkWidget *copyMenu;
    // GtkWidget *menu_item;
    // copyMenu = gtk_menu_new();

    // menu_item = gtk_menu_item_new_with_label(_("Copy selection"));
    // gtk_menu_shell_append(GTK_MENU_SHELL(copyMenu), menu_item);
    // gtk_widget_show(menu_item);
    // g_signal_connect(G_OBJECT(menu_item), "activate", G_CALLBACK(show_popup), pwGameList);

    // menu_item = gtk_menu_item_new_with_label(_("Copy all"));
    // gtk_menu_shell_append(GTK_MENU_SHELL(copyMenu), menu_item);
    // gtk_widget_show(menu_item);
    // g_signal_connect(G_OBJECT(menu_item), "activate", G_CALLBACK(show_popup), pwGameList);

    // g_signal_connect(G_OBJECT(pwGameList), "button-press-event", G_CALLBACK(show_popup), copyMenu);

        // g_signal_connect(pwGameList, "popup-menu", G_CALLBACK(view_onPopupMenu), NULL); 

        // GtkWidget *pQuizMenu;

        // pQuizMenu = gtk_menu_new();
        // GtkWidget *menu_item;
        // // int tempArray [numCategories];
        // g_message("numCategories=%d",numCategories);
        // // gtk_image_menu_item_set_always_show_image(menu_item,TRUE);
        // for(int i=0;i < numCategories; i++) {
        //     // tempArray[i]=i;
        //     char buf[MAX_CATEGORY_NAME_LENGTH+8];
        //     sprintf(buf,_("Add to: %s"),categories[i].name);
        //     menu_item = gtk_menu_item_new_with_label(_(buf));
        //     gtk_menu_shell_append(GTK_MENU_SHELL(pQuizMenu), menu_item);
        //     gtk_widget_show(menu_item);
        //     g_signal_connect_swapped(G_OBJECT(menu_item), "activate", G_CALLBACK(AddPositionToFile), 
        //         &(categories[i])); //(tempArray[i]));
        // }       

        // g_signal_connect_swapped(pwGameList, "button-press-event", 
        //     G_CALLBACK(show_popup), pQuizMenu);  
        
        // g_signal_connect(pwGameList, "button-press-event", G_CALLBACK(view_onButtonPressed), 
        //     NULL);    
        // g_signal_connect(pwGameList, "popup-menu", G_CALLBACK(view_onPopupMenu), NULL); 

   
    return pwGameList;
}

        // GtkWidget *window;
        // GtkWidget *ebox;
        // GtkWidget *hideMi;
        // GtkWidget *quitMi;

        // window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
        // gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER);
        // gtk_window_set_default_size(GTK_WINDOW(window), 300, 200);
        // gtk_window_set_title(GTK_WINDOW(window), "Popup menu");

        // ebox = gtk_event_box_new();
        // gtk_container_add((column), ebox);
        // gtk_container_add(GTK_CONTAINER(pwGameList), ebox);
        // gtk_container_add(GTK_CONTAINER(woPanel[WINDOW_GAME].pwWin), ebox);
        // gtk_event_box_set_visible_window(ebox, FALSE);
        // gtk_event_box_set_above_child (ebox,TRUE);
        // gtk_widget_hide(ebox);
        
        // hideMi = gtk_menu_item_new_with_label("Minimize");
        // gtk_widget_show(hideMi);
        // gtk_menu_shell_append(GTK_MENU_SHELL(pmenu), hideMi);
        
        // quitMi = gtk_menu_item_new_with_label("Print");
        // gtk_widget_show(quitMi);
        // gtk_menu_shell_append(GTK_MENU_SHELL(pmenu), quitMi);
        
        // // g_signal_connect_swapped(G_OBJECT(hideMi), "activate", 
        // //     G_CALLBACK(gtk_window_iconify), GTK_WINDOW(woPanel[WINDOW_GAME].pwWin));    
        // g_signal_connect(G_OBJECT(quitMi), "activate", 
        //     G_CALLBACK(AddPositionToFile), NULL);  

        /*separator*/
        // menu_item = gtk_menu_item_new();
        // gtk_menu_shell_append(GTK_MENU_SHELL(pmenu), menu_item);
        // gtk_widget_show(menu_item);
        // menu_item = gtk_menu_item_new_with_label(_("Add to: Blitzing"));
        // gtk_menu_shell_append(GTK_MENU_SHELL(pmenu), menu_item);
        // gtk_widget_show(menu_item);
        // g_signal_connect(G_OBJECT(menu_item), "activate", G_CALLBACK(AddPositionToFile), NULL);
        // menu_item = gtk_menu_item_new();
        // gtk_menu_shell_append(GTK_MENU_SHELL(pmenu), menu_item);
        // gtk_widget_show(menu_item);
        // menu_item = gtk_menu_item_new_with_label(_("Add to: 5-pt holding"));
        // gtk_menu_shell_append(GTK_MENU_SHELL(pmenu), menu_item);
        // gtk_widget_show(menu_item);
        // g_signal_connect(G_OBJECT(menu_item), "activate", G_CALLBACK(AddPositionToFile), NULL);
        // menu_item = gtk_menu_item_new_with_label(_("Add to: 4-pt holding"));
        // gtk_menu_shell_append(GTK_MENU_SHELL(pmenu), menu_item);
        // gtk_widget_show(menu_item);
        // g_signal_connect(G_OBJECT(menu_item), "activate", G_CALLBACK(AddPositionToFile), NULL);
        // menu_item = gtk_menu_item_new_with_label(_("Add to: 1-pt holding"));
        // gtk_menu_shell_append(GTK_MENU_SHELL(pmenu), menu_item);
        // gtk_widget_show(menu_item);
        // g_signal_connect(G_OBJECT(menu_item), "activate", G_CALLBACK(AddPositionToFile), NULL);

    // g_signal_connect_swapped((pwGameList), "cursor-changed", 
    //     G_CALLBACK(show_popup), pmenu);  


    // g_signal_connect(G_OBJECT(woPanel[WINDOW_GAME].pwWin), "destroy",
    //     G_CALLBACK(gtk_main_quit), NULL);
            



static void
AddStyle(GtkStyle ** ppsComb, GtkStyle * psNew)
{
    if (!*ppsComb) {
        *ppsComb = psNew;
        g_object_ref(*ppsComb);
    } else {
        GtkStyle *copy = gtk_style_copy(*ppsComb);
        g_object_unref(*ppsComb);
        *ppsComb = copy;
        UpdateStyle(*ppsComb, psNew, psGameList);
    }
}

static void
SetCellStyle(GtkTreeIter * iter, int fPlayer, moverecord * pmr)
{
    GtkStyle *pStyle = NULL;

    if (fStyledGamelist) {
        if (pmr->lt == LUCK_VERYGOOD) {
            pStyle = psLucky[LUCK_VERYGOOD];
            g_object_ref(pStyle);
        } else if (pmr->lt == LUCK_VERYBAD) {
            pStyle = psLucky[LUCK_VERYBAD];
            g_object_ref(pStyle);
        }

        if (pmr->n.stMove == SKILL_DOUBTFUL)
            AddStyle(&pStyle, psChequerErrors[SKILL_DOUBTFUL]);
        if (pmr->stCube == SKILL_DOUBTFUL)
            AddStyle(&pStyle, psCubeErrors[SKILL_DOUBTFUL]);

        if (pmr->n.stMove == SKILL_BAD)
            AddStyle(&pStyle, psChequerErrors[SKILL_BAD]);
        if (pmr->stCube == SKILL_BAD)
            AddStyle(&pStyle, psCubeErrors[SKILL_BAD]);

        if (pmr->n.stMove == SKILL_VERYBAD)
            AddStyle(&pStyle, psChequerErrors[SKILL_VERYBAD]);
        if (pmr->stCube == SKILL_VERYBAD)
            AddStyle(&pStyle, psCubeErrors[SKILL_VERYBAD]);
    }

    if (!pStyle) {
        pStyle = psGameList;
        g_object_ref(pStyle);
    }

    gtk_list_store_set(plsGameList, iter, GL_COL_STYLE_0 + fPlayer, pStyle, -1);
    g_object_unref(pStyle);
}

/* Add a moverecord to the game list window.  NOTE: This function must be
 * called _before_ applying the moverecord, so it can be displayed
 * correctly. */
extern void
GTKAddMoveRecord(moverecord * pmr)
{
    moverecord *apmr[2] = { NULL, NULL };
    gboolean fCombined = TRUE;
    int fPlayer, moveNum = 0;
    GtkTreeIter iter;
    const char *pch = GetMoveString(pmr, &fPlayer, TRUE);
    if (!pch)
        return;

    if (gtk_tree_model_get_iter_first(GTK_TREE_MODEL(plsGameList), &iter)) {
        GtkTreeIter next = iter;
        while (gtk_tree_model_iter_next(GTK_TREE_MODEL(plsGameList), &next))
            iter = next;

        gtk_tree_model_get(GTK_TREE_MODEL(plsGameList), &iter, GL_COL_MOVE_NUMBER, &moveNum, GL_COL_MOVE_RECORD_0,
                           &apmr[0], GL_COL_MOVE_RECORD_1, &apmr[1], GL_COL_COMBINED, &fCombined, -1);
    }

    if (fCombined || apmr[1] || (fPlayer != 1 && apmr[0])) {
        /* Add new row */
        gtk_list_store_append(plsGameList, &iter);
        ++moveNum;
    }

    if (fPlayer == -1) {
        fCombined = TRUE;
        fPlayer = 0;
    } else
        fCombined = FALSE;

    gtk_list_store_set(plsGameList, &iter, GL_COL_MOVE_NUMBER, moveNum, GL_COL_MOVE_STRING_0 + fPlayer, pch,
                       GL_COL_MOVE_RECORD_0 + fPlayer, pmr, GL_COL_COMBINED, fCombined, -1);

    SetCellStyle(&iter, fPlayer, pmr);
}


/* Select a moverecord as the "current" one.  NOTE: This function must be
 * called _after_ applying the moverecord. */
extern void
GTKSetMoveRecord(moverecord * pmr)
{

    /* highlighted row/col in game record */
    static GtkTreePath *path = NULL;
    static int fPlayer = -1;

    GtkTreeIter iter;
    moverecord *apmr[2];
    gboolean iterValid, lastRow = FALSE;
    /* Avoid lots of screen updates */
    if (!frozen)
        SetAnnotation(pmr);

#ifdef UNDEF
    {
        GtkWidget *pwWin = GetPanelWidget(WINDOW_HINT);
        if (pwWin) {
            hintdata *phd = g_object_get_data(G_OBJECT(pwWin), "user_data");
            phd->fButtonsValid = FALSE;
            CheckHintButtons(phd);
        }
    }
#endif

    if (path && fPlayer != -1) {
        moverecord *pmrLast = NULL;
        if (gtk_tree_model_get_iter(GTK_TREE_MODEL(plsGameList), &iter, path)) {
            gtk_tree_model_get(GTK_TREE_MODEL(plsGameList), &iter, GL_COL_MOVE_RECORD_0, &apmr[0],
                               GL_COL_MOVE_RECORD_1, &apmr[1], -1);
            pmrLast = apmr[fPlayer];
            if (pmrLast)
                SetCellStyle(&iter, fPlayer, pmrLast);
            else
                gtk_list_store_set(plsGameList, &iter, GL_COL_STYLE_0 + fPlayer, psGameList, -1);
        }
    }

    if (path)
        gtk_tree_path_free(path);
    path = NULL;
    fPlayer = -1;

    if (!pmr)
        return;

    if (pmr == plGame->plNext->p) {
        g_assert(pmr->mt == MOVE_GAMEINFO);
        path = gtk_tree_path_new_first();

        if (plGame->plNext->plNext->p) {
            moverecord *pmrNext = plGame->plNext->plNext->p;

            if (pmrNext->mt == MOVE_NORMAL && pmrNext->fPlayer == 1)
                fPlayer = 1;
            else
                fPlayer = 0;
        } else
            fPlayer = 0;
    } else {
        iterValid = gtk_tree_model_get_iter_first(GTK_TREE_MODEL(plsGameList), &iter);
        while (iterValid) {
            gtk_tree_model_get(GTK_TREE_MODEL(plsGameList), &iter, GL_COL_MOVE_RECORD_0, &apmr[0],
                               GL_COL_MOVE_RECORD_1, &apmr[1], -1);
            if (apmr[1] == pmr) {
                fPlayer = 1;
                break;
            } else if (apmr[0] == pmr) {
                fPlayer = 0;
                break;
            }
            iterValid = gtk_tree_model_iter_next(GTK_TREE_MODEL(plsGameList), &iter);
        }

        if (iterValid) {
            path = gtk_tree_model_get_path(GTK_TREE_MODEL(plsGameList), &iter);
            lastRow = !gtk_tree_model_iter_next(GTK_TREE_MODEL(plsGameList), &iter);
            gtk_tree_model_get_iter(GTK_TREE_MODEL(plsGameList), &iter, path);
        }

        if (path && !(pmr->mt == MOVE_SETDICE && lastRow)) {
            do {
                if (++fPlayer > 1) {
                    fPlayer = 0;
                    gtk_tree_path_next(path);
                    iterValid = gtk_tree_model_iter_next(GTK_TREE_MODEL(plsGameList), &iter);
                }

                if (iterValid) {
                    GtkTreeIter tmpIter = iter;
                    lastRow = !gtk_tree_model_iter_next(GTK_TREE_MODEL(plsGameList), &tmpIter);
                    gtk_tree_model_get(GTK_TREE_MODEL(plsGameList), &iter, GL_COL_MOVE_RECORD_0, &apmr[0],
                                       GL_COL_MOVE_RECORD_1, &apmr[1], -1);
                }
            } while (iterValid && !lastRow && !apmr[fPlayer]);

            if (!iterValid) {
                int *moveIndex = gtk_tree_path_get_indices(path);
                gtk_list_store_append(plsGameList, &iter);
                gtk_list_store_set(plsGameList, &iter, GL_COL_MOVE_NUMBER, *moveIndex + 1, -1);
            }
        }
    }

    /* Highlight current move */
    if (path && gtk_tree_model_get_iter(GTK_TREE_MODEL(plsGameList), &iter, path)) {
        gtk_list_store_set(plsGameList, &iter, GL_COL_STYLE_0 + fPlayer, psCurrent, -1);
        gtk_tree_view_scroll_to_cell(GTK_TREE_VIEW(pwGameList), path, NULL, TRUE, 0.8f, 0.5f);
    }
}

extern void
GTKPopMoveRecord(moverecord * const pmr)
{
    GtkTreeIter iter;
    gboolean iterValid, recordFound = FALSE;
    moverecord *apmr[2];

    iterValid = gtk_tree_model_get_iter_first(GTK_TREE_MODEL(plsGameList), &iter);
    while (iterValid) {
        gtk_tree_model_get(GTK_TREE_MODEL(plsGameList), &iter, GL_COL_MOVE_RECORD_0, &apmr[0],
                           GL_COL_MOVE_RECORD_1, &apmr[1], -1);
        if (apmr[0] == pmr || recordFound) {
            /* the left column matches; delete the row */
            iterValid = gtk_list_store_remove(plsGameList, &iter);
            recordFound = TRUE;
        } else if (apmr[1] == pmr) {
            /* the right column matches; delete that column only */
            gtk_list_store_set(plsGameList, &iter, GL_COL_MOVE_STRING_1, NULL,
                               GL_COL_MOVE_RECORD_1, NULL, GL_COL_STYLE_1, NULL, -1);
            iterValid = gtk_tree_model_iter_next(GTK_TREE_MODEL(plsGameList), &iter);
            recordFound = TRUE;
        } else
            iterValid = gtk_tree_model_iter_next(GTK_TREE_MODEL(plsGameList), &iter);
    }
}
