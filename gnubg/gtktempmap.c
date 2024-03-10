/*
 * Copyright (C) 2002-2003 Joern Thyssen <jth@gnubg.org>
 * Copyright (C) 2003-2022 the AUTHORS
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
 * $Id: gtktempmap.c,v 1.66 2024/02/24 21:09:34 plm Exp $
 */

/*
 * Based on Sho Sengoku's Equity Temperature Map
 * https://bkgm.com/articles/Sengoku/TemperatureMap/index.html 
 */

/* 03/2024: Isaac Keslassy: new TempMap with relative equity between alternative decisions:
 * - the goal is to illustrate the subtle differences between close decisions
 * - new TempMap colors, gauge, titles, tooltips, button, etc.
 * - made TempMap non-modal so we can see the position as well; and with a window-maximizing button
 * - updated the description in the menu
 */



#include "config.h"

#include <gtk/gtk.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <glib.h>

#include "backgammon.h"
#include "eval.h"
#include "gtktempmap.h"
#include "gtkgame.h"
#include "drawboard.h"
#include "format.h"
#include "render.h"
#include "renderprefs.h"
#include "gtkboard.h"
#include "gtkwindows.h"
#include "gtkcube.h"

#define SIZE_QUADRANT 52


typedef struct {

    matchstate *pms;
    float aarEquity[6][6]; /* equity for each quadrant */
    float rAverage;
    float aarEquityDiff[6][6];
    float dAverage;
    int aaanMove[6][6][8]; /* best move for each quadrant */

    GtkWidget *aapwDA[6][6];
    GtkWidget *aapwe[6][6];
    GtkWidget *pwAverage;
    GtkWidget *pweAverage;
    GtkWidget *Frame; /* frame with title to be changed */

    gchar *szTitle;
    char TwoRollFirstMove[FORMATEDMOVESIZE]; /* in 2roll mode, best roll used in m=0 before getting map at m=1 */
} tempmap;


typedef struct {

    unsigned char *achDice[2];
    unsigned char *achPips[2];
    int fShowEquity;
    int fShowBestMove;
    int fShowDiff;
    int fShowTwoRolls;
    int fShowMode;
    int fInvert;
    GtkWidget * pwGauge;
    GtkWidget *aapwGaugeDA[32];
    GtkWidget *aapweGauge[32];
    // GtkWidget * pwGaugeDiff;
    GtkWidget *apwGauge[2];
    // GtkWidget *apwGaugeDiff[2];
    GtkWidget * pwContainer;

    float rMin, rMax;
    float dMin, dMax, dMaxAbs; /* min/max equity differences and max absolute value*/

    tempmap *atm;
    int n;

    int nSizeDie;

    tempmap TM1;
    tempmap oppTM[6][6]; /* when considering two consecutive rolls: getting a second-roll opponent TM for each possible 1st roll */
        
    evalcontext ec; /* includes the eval ply */
} tempmapwidget;

static char *aszTempMap[] =  {
    N_("Absolute equity"), N_("Relative equity"), N_("Two-roll equity"), N_("O'Hagan's Law")//, NULL
};
// int modesTempMap[] = { ABS, DIFF, TWO_ROLL, OHAGAN };
static char *aszTMTooltip[] =  {
    N_("In each scenario, obtain a map of the 36 equities corresponding to the 36 possible rolls."), 
    N_("The first map provides an absolute equity that serves as a basis, while the other maps provide an equity increase or decrease "
            "relative to this basis."), 
    N_("The first map shows the equity following the first roll of the observed player, and the second map shows "
            "the equity following the roll of his opponent. E.g., the 1-ply eval of a given roll in the first map "
            "should equal the corresponding 0-ply (top-left quadrant) average eval of the rolls in the second map, "
            "assuming the 0-ply eval picked the same best move in the first roll as the 1-ply eval."), 
    N_("O'Hagan's Law")
};

// static char szTwoRollSelect[] = g_strdup_printf("%s",_("Click on some square to select it as a first roll and check the opponent's second-roll responses."));
static char szTwoRollSelect[] =N_("Click on some square to select it as a first roll and check the opponent's second-roll responses.");
// static char szDICE[] = N_("<die> <die>"),

/* Retain these from one GTKShowTempMap() to the next */
static int fShowEquity = FALSE;
static int fShowBestMove = FALSE;
static int fShowDiff = FALSE;
static int fShowTwoRolls = FALSE;
static int fTwoRollsSelected = FALSE;
static int fShowMode = 0;
static int iDefault = 0;
static int jDefault = 0;

/* computes all equities and best moves for a given TempMap and stores them */
static int
TempMapEquities(evalcontext * pec, const matchstate * pms, float aarEquity[6][6], int aaanMove[6][6][8], const gchar * szTitle, const float rFac)
{

    int i, j;
    float arOutput[NUM_ROLLOUT_OUTPUTS];
    TanBoard anBoard;
    int aaan[6][6][8];
    float aar[6][6];
    cubeinfo ci;
    cubeinfo cix;


    /* calculate equities */

    GetMatchStateCubeInfo(&cix, pms);

    if (szTitle && *szTitle) {
        gchar *sz = g_strdup_printf(_("Calculating equities for %s"), szTitle);
        ProgressStartValue(sz, 21);
        g_free(sz);
    } else
        ProgressStartValue(_("Calculating equities"), 21);

    for (i = 0; i < 6; ++i)
        for (j = 0; j <= i; ++j) {

            memcpy(&ci, &cix, sizeof ci);

            /* find best move */

            memcpy(anBoard, pms->anBoard, sizeof(anBoard));

            if (FindBestMove(aaan[i][j], i + 1, j + 1, anBoard, &ci, pec, defaultFilters) < 0) {
                ProgressEnd();
                return -1;
            }
            
            // char szMove[FORMATEDMOVESIZE];
            // FormatMove(szMove, (ConstTanBoard)anBoard, aaan[i][j]);
            // g_message("best move=%s",szMove);

            /* evaluate resulting position */

            SwapSides(anBoard);
            ci.fMove = !ci.fMove;

            if (GeneralEvaluationE(arOutput, (ConstTanBoard) anBoard, &ci, pec) < 0) {
                ProgressEnd();
                return -1;
            }

            InvertEvaluationR(arOutput, &cix);

            if (!cix.nMatchTo && rFac != 1.0f)
                arOutput[OUTPUT_CUBEFUL_EQUITY] *= rFac; /* money game: multiply by 2x the value if the cube has been set */

            aar[i][j] = arOutput[OUTPUT_CUBEFUL_EQUITY];
            aar[j][i] = arOutput[OUTPUT_CUBEFUL_EQUITY];
            // g_message("i=%d, j=%d, aar[i][j]=%f",i,j,aar[i][j]);

            if (i != j)
                memcpy(aaan[j][i], aaan[i][j], sizeof aaan[0][0]);

            ProgressValueAdd(1);

        }

    ProgressEnd();

    memcpy(aarEquity, aar, sizeof aar);
    memcpy(aaanMove, aaan, sizeof aaan);

    return 0;

}

/* in two-roll mode, computes all equities for 2nd roll after 1st roll (i0,j0) */
static int
SecondRollEquities(int i0, int j0, evalcontext * pec, const matchstate * pms, float aarEquity[6][6], int aaanMove[6][6][8], 
                        char TwoRollFirstMove[FORMATEDMOVESIZE], const gchar * szTitle)
{
    int i, j;
    float arOutput[NUM_ROLLOUT_OUTPUTS];
    TanBoard anBoard,anBoard2;
    int tmp[8];
    int aaan[6][6][8];
    float aar[6][6];
    cubeinfo ci,ci2;
    cubeinfo cix;
    // char szMove[FORMATEDMOVESIZE];
                
    // evalcontext ec = {TRUE, 0, FALSE, TRUE, 0.0, FALSE};

    GetMatchStateCubeInfo(&cix, pms);
    memcpy(&ci, &cix, sizeof ci); /* copy cix->ci*/

    /* find best move */

    memcpy(anBoard, pms->anBoard, sizeof(anBoard)); /* copy {pms->anBoard} -> anBoard */

    if (FindBestMove(tmp, i0 + 1, j0 + 1, anBoard, &ci, pec, defaultFilters) < 0) {
        return -1;
    }

    // memcpy(TwoRollFirstMove, tmp, sizeof(tmp));

    FormatMove(TwoRollFirstMove, (ConstTanBoard)anBoard, tmp);
    // g_message("(i0=%d, j0=%d) -> best move=%s",i0,j0,TwoRollFirstMove);


    /* evaluate resulting position */

    SwapSides(anBoard);
    ci.fMove = !ci.fMove;

    /* calculate equities */

    // GetMatchStateCubeInfo(&cix, pms);

    if (szTitle && *szTitle) {
        gchar *sz = g_strdup_printf(_("Calculating equities for %s"), szTitle);
        ProgressStartValue(sz, 21);
        g_free(sz);
    } else
        ProgressStartValue(_("Calculating equities"), 21);

    for (i = 0; i < 6; ++i)
        for (j = 0; j <= i; ++j) {

            memcpy(&ci2, &ci, sizeof ci); /* copy ci->ci2*/

            /* find best move */

            memcpy(anBoard2, anBoard, sizeof(anBoard)); /* copy anBoard -> anBoard2 */

            if (FindBestMove(aaan[i][j], i + 1, j + 1, anBoard2, &ci2, pec, defaultFilters) < 0) {
                ProgressEnd();
                return -1;
            }

            // if (0) {
            //     FormatMove(szMove, (ConstTanBoard)anBoard2, aaan[i][j]);
            //     g_message("i=%d, j=%d, best move=%s",i,j,szMove);
            // }

            /* evaluate resulting position */

            SwapSides(anBoard2);
            ci2.fMove = !ci2.fMove;

            if (GeneralEvaluationE(arOutput, (ConstTanBoard) anBoard2, &ci2, pec) < 0) {
                ProgressEnd();
                return -1;
            }

            // InvertEvaluationR(arOutput, &ci2); /* don't invert b/c we want our equity, not our opponent's */

            aar[i][j] = arOutput[OUTPUT_CUBEFUL_EQUITY];
            aar[j][i] = arOutput[OUTPUT_CUBEFUL_EQUITY];

                // g_message("... i+1=%d, j+1=%d, aar[i][j]=%f",i+1,j+1,aar[i][j]);

            if (i != j)
                memcpy(aaan[j][i], aaan[i][j], sizeof aaan[0][0]);

            ProgressValueAdd(1);

        }

    ProgressEnd();

    memcpy(aarEquity, aar, sizeof aar);
    memcpy(aaanMove, aaan, sizeof aaan);

    return 0;

}

static int
CalcTempMapEquities(tempmapwidget * ptmw, int fRecomputeMap0)
{
    // const evalcontext *pec = { TRUE, 0, FALSE, TRUE, 0.0, FALSE };

    for (int m = 0; m < ptmw->n; ++m) {

        if (m==1 && fShowTwoRolls) { /* 2nd-roll equities for opponent roll in map m=1 following our (now-computed) 1st roll in map m=0 */
            for (int i = 0; i < 6; ++i) {
                for (int j = 0; j < 6; ++j) {
                    if (SecondRollEquities(i, j, &ptmw->ec, ptmw->atm[0].pms, ptmw->oppTM[i][j].aarEquity, ptmw->oppTM[i][j].aaanMove,
                                        ptmw->oppTM[i][j].TwoRollFirstMove, ptmw->atm[0].szTitle) < 0) {
                        g_message("problem in 2roll equity computation");
                        return -1;
                    }
                    // g_message("2roll calc: i=%d,j=%d,move=%s",i,j,ptmw->oppTM[i][j].TwoRollFirstMove);
                }
            }  
        } else {  /* compute the regular absolute equities for map m... */
            if (m>0 || (m==0 && fRecomputeMap0)) { /* ... but don't recompute map m=0 if it was already computed at this ply to avoid wasting resources */
                if (TempMapEquities(&ptmw->ec, ptmw->atm[m].pms, ptmw->atm[m].aarEquity, ptmw->atm[m].aaanMove,
                                    ptmw->atm[m].szTitle, (float) (ptmw->atm[m].pms->nCube / ptmw->atm[0].pms->nCube)) < 0)
                    return -1;
            }
        }
    }
    return 0;

}

/*********************** Done w/ computations, now dealing with the display ************************/

static void
UpdateStyle(GtkWidget * pw, const float r, const int fColor)
{

    GtkStyle *ps = gtk_style_copy(gtk_widget_get_style(pw));
    double *gbval;

    gbval = g_malloc(sizeof(*gbval));
    *gbval = 1.0 - (double)r;
    g_object_set_data_full(G_OBJECT(pw), "gbval", gbval, g_free);

    if (fColor==0) { /* ??? for absolute equity*/
        ps->bg[GTK_STATE_NORMAL].green = (guint16) ((1.0f - 0.3f * r) * 0xFFFF);
        ps->bg[GTK_STATE_NORMAL].blue = 0xFFFF;
        ps->bg[GTK_STATE_NORMAL].red = (guint16) ((1.0f - r) * 0xFFFF);
    } else if (fColor==1) { /* green for positive relative equity*/
        ps->bg[GTK_STATE_NORMAL].green= 0xFFFF;
        ps->bg[GTK_STATE_NORMAL].blue = ps->bg[GTK_STATE_NORMAL].red  = (guint16) ((1.0f - r) * 0xFFFF);
    } else { /* red for negative relative equity*/
        ps->bg[GTK_STATE_NORMAL].red = 0xFFFF;
        ps->bg[GTK_STATE_NORMAL].blue = ps->bg[GTK_STATE_NORMAL].green = (guint16) ((1.0f - r) * 0xFFFF);        
    }
    gtk_widget_set_style(pw, ps);

}

static void
SetStyleDiff(GtkWidget * pw, float dEquity, const float dMaxAbs) //, const int fInvert)
{

    // if (fInvert)
    //     dEquity = -dEquity;
    // g_message("dEquity=%+.3f,dMaxAbs=%+.3f,fInvert=%d",dEquity,dMaxAbs,fInvert);
    if (dEquity>0)
        UpdateStyle(pw, dEquity/dMaxAbs, 1);
    else
        UpdateStyle(pw, -dEquity/dMaxAbs, 2);


}

static void
SetStyle(GtkWidget * pw, const float rEquity, const float rMin, const float rMax, const int fInvert)
{

    float r = (rEquity - rMin) / (rMax - rMin);
    UpdateStyle(pw, fInvert ? (1.0f - r) : r, 0);

}


/* based on OutputMWC(), returns equity as float and not string */
static float
GetEquityAux(const float r, const cubeinfo * pci)
{
    if (!pci->nMatchTo) {
        return r;
    } else {
        if (!fOutputMWC) {
            return mwc2eq(r, pci);
        } else if (fOutputMatchPC) {
            return 100.0f * r;
        } else {
            return r;
        }
    }
}

static float
GetEquity(const float rEquity, const cubeinfo * pci, const int fInvert)
{

    float r;

    if (fInvert) {
        /* invert equity */
        if (pci->nMatchTo)
            r = 1.0f - rEquity;
        else
            r = -rEquity;
    } else
        r = rEquity;

    if (fInvert) {
        cubeinfo ci;
        memcpy(&ci, pci, sizeof ci);
        ci.fMove = !ci.fMove;

        return GetEquityAux(r, &ci);
    } else
        return GetEquityAux(r, pci);

}


static char *
GetEquityDiffStringAux(const float diff, const cubeinfo * pci)
{
    #define OUTPUT_SZ_LENGTH (5 + MAX_OUTPUT_DIGITS)
    static char sz[OUTPUT_SZ_LENGTH];

    if (!pci->nMatchTo || !fOutputMWC) {
            snprintf(sz, OUTPUT_SZ_LENGTH, "%+*.*f", fOutputDigits + 3, fOutputDigits, diff);
    } else if (fOutputMatchPC) { /* MWC shown as percentage? */
        // snprintf(sz, OUTPUT_SZ_LENGTH, "%*.*f%%", fOutputDigits + 3, fOutputDigits > 1 ? fOutputDigits - 1 : 0, diff);
        // snprintf(sz, OUTPUT_SZ_LENGTH, "%*.*f%%", fOutputDigits + 1, fOutputDigits > 1 ? fOutputDigits - 1 : 0, diff);
        snprintf(sz, OUTPUT_SZ_LENGTH, "%+.*f%%", fOutputDigits > 1 ? fOutputDigits - 1 : 0, diff);

    } else /* MWC shown as probability? */ {
        // snprintf(sz, OUTPUT_SZ_LENGTH, "%+*.*f", fOutputDigits + 3, fOutputDigits + 1, diff); 
        snprintf(sz, OUTPUT_SZ_LENGTH, "%+.*f", fOutputDigits + 1, diff); 
    }
    // g_message("MWC:%s,%d,%d,%d",sz,OUTPUT_SZ_LENGTH,fOutputDigits,fOutputMatchPC);
    return sz;
}

static char *
GetEquityDiffString(const float rEquity0, const float rEquity, const cubeinfo * pci, const int fInvert)
{
    // float r0,r;
    float diff = GetEquity(rEquity, pci, fInvert)-GetEquity(rEquity0, pci, fInvert);
    // g_message("diff=%f-%f=%f",GetEquity(rEquity, pci, fInvert),GetEquity(rEquity0, pci, fInvert),diff);

    // if (fInvert) {
    //     cubeinfo ci;
    //     memcpy(&ci, pci, sizeof ci);
    //     ci.fMove = !ci.fMove;

    //     return OutputEquityDiff(r0, r, &ci);
    // } else

    return GetEquityDiffStringAux(diff,pci);
}

static char *
GetEquityString(const float rEquity, const cubeinfo * pci, const int fInvert)
{

    float r;

    if (fInvert) {
        /* invert equity */
        if (pci->nMatchTo)
            r = 1.0f - rEquity;
        else
            r = -rEquity;
    } else
        r = rEquity;

    if (fInvert) {
        cubeinfo ci;
        memcpy(&ci, pci, sizeof ci);
        ci.fMove = !ci.fMove;

        return OutputMWC(r, &ci, TRUE);
    } else
        return OutputMWC(r, pci, TRUE);
}



/* Use equity value for each quadrant, and compute min, max and average.  
 *   The quadrant equity computations happen earlier. 
 *   Four options for what to display: 
 *   (1) Absolute equity: equity [in MWC terms?] 
 *   (2) Relative equity: equity difference with 1st map (in normalized equity) 
 *   (3) Two-roll equity: equity for 2nd roll on the right map after some given 1st roll on the left map 
 *   (4) O'Hagan: ...
 */
static void
UpdateTempMapEquities(tempmapwidget * ptmw)
{

    int i, j;
    float rMax, rMin, r;
    float dMax, dMin, d; /* equity differences */
    cubeinfo ci;
    int m;
    char szMove[FORMATEDMOVESIZE];
    int mMax;

    if (fShowTwoRolls)
        g_assert(ptmw->n == 2);

    if (fShowDiff)
        mMax = 1;
    else /* incl. two-roll equity, with n==2 */
        mMax = ptmw->n;

    rMax = -10000;
    rMin = +10000;
    for (m = 0; m < mMax; ++m) {
        ptmw->atm[m].rAverage = 0.0f;
        for (i = 0; i < 6; ++i)
            for (j = 0; j < 6; ++j) {
                if (m==1 && fShowTwoRolls)
                    r = ptmw->oppTM[iDefault][jDefault].aarEquity[i][j];
                else
                    r = ptmw->atm[m].aarEquity[i][j];
                // g_message("r=%f",r);
                ptmw->atm[m].rAverage += r;
                if (r > rMax)
                    rMax = r;
                if (r < rMin)
                    rMin = r;
            }
        ptmw->atm[m].rAverage /= 36.0f;
    }

    ptmw->rMax = rMax;
    ptmw->rMin = rMin;

    if (fShowDiff) {
        GetMatchStateCubeInfo(&ci, ptmw->atm[0].pms);
        dMax = -10000.0;
        dMin = +10000.0;
        for (m = 1; m < ptmw->n; ++m) {
            ptmw->atm[m].dAverage = 0.0f;
            for (i = 0; i < 6; ++i)
                for (j = 0; j < 6; ++j) {
                    d = ptmw->atm[m].aarEquityDiff[i][j] = GetEquity(ptmw->atm[m].aarEquity[i][j], &ci, ptmw->fInvert)
                                                        - GetEquity(ptmw->atm[0].aarEquity[i][j], &ci, ptmw->fInvert);
                    // g_message("m=%d,i=%d,j=%d -> d=%+.3f",m,i,j,d);
                    ptmw->atm[m].dAverage += d;
                    if (d > dMax)
                        dMax = d;
                    if (d < dMin)
                        dMin = d;
                }
            ptmw->atm[m].dAverage /= 36.0f;
        }

        ptmw->dMax = dMax;
        ptmw->dMin = dMin;
        ptmw->dMaxAbs = MAX(-dMin,dMax);
        // float dMAX = (dMax > (-dMin)) ? dMax : -dMin;
        // g_message("dMax=%f,dMin=%f,ptmw->dMaxAbs=%f",dMax,dMin,ptmw->dMaxAbs);
        g_assert(ptmw->n <=1 || ptmw->dMaxAbs>=0.0f);
    } 

    // if (fShowTwoRolls) {
    //     /* compute equities and best moves of all 36*36 options following the 36 possible rolls */
    //     SecondRollEquities(5,2,&ec, ptmw->atm[0].pms, ptmw->oppTM[iDefault][jDefault].aarEquity, ptmw->oppTM[iDefault][jDefault].aaanMove,
    //                     ptmw->atm[0].szTitle); 
        
    // }

    /* update styles (cell colors) & tooltips */

    /* a bit confusing: it looks like the TempMap uses the "aarEquity" r (same as MWC?) to pick the color of each cell, 
    but it then displays the equity of each cell, not its MWC */

    GetMatchStateCubeInfo(&ci, ptmw->atm[0].pms);

    /* for translations */
    char szROLL[]= "Equity following roll";
    char szAVG[]= "Average equity";
    char szREL[]= "Relative";
    char szABS[]= "Absolute";
    char szMOVE[]= "Best move";

    gchar *sz;
    for (m = 0; m < ptmw->n; ++m) {
        for (i = 0; i < 6; ++i)
            for (j = 0; j < 6; ++j) {
                // g_message("updateTMequities: before setstyle: m=%d,i=%d,j=%d",m,i,j);
                /* tooltip text & colors */
                if (m>0 && fShowDiff) { /* relative equities */
                    sz = g_strdup_printf("[%s (%d,%d)]\n\n%s: \t\t%s\n%s: \t\t%s\n%s: \t%s", _(szROLL), i+1,j+1, _(szREL),
                            GetEquityDiffString(ptmw->atm[0].aarEquity[i][j], ptmw->atm[m].aarEquity[i][j],
                                                                &ci, ptmw->fInvert), _(szABS),
                            GetEquityString(ptmw->atm[m].aarEquity[i][j], &ci, ptmw->fInvert), _(szMOVE),
                            FormatMove(szMove, (ConstTanBoard) ptmw->atm[m].pms->anBoard,
                                                        ptmw->atm[m].aaanMove[i][j]));
                    // g_message("%s",sz);
                    SetStyleDiff(ptmw->atm[m].aapwDA[i][j], ptmw->atm[m].aarEquityDiff[i][j], ptmw->dMaxAbs);
                } else if (m==1 && fShowTwoRolls) { /* 2nd-roll equities for opponent roll following our roll in m=0 */
                    /* For FormatMove: should board be updated before? Is it more intuitive to express it
                    in the opponent's view or in the player's view? (e.g. 24/18 or 1/7?) */
                    sz = g_strdup_printf("[%s (%d,%d)]\n\n%s: \t\t%s\n%s: \t%s",_(szROLL),  i+1,j+1, _(szABS),
                            GetEquityString(ptmw->oppTM[iDefault][jDefault].aarEquity[i][j], &ci, ptmw->fInvert), _(szMOVE),
                            FormatMove(szMove, (ConstTanBoard) ptmw->atm[m].pms->anBoard,
                                                        ptmw->oppTM[iDefault][jDefault].aaanMove[i][j])); 
                    SetStyle(ptmw->atm[m].aapwDA[i][j],ptmw->oppTM[iDefault][jDefault].aarEquity[i][j], rMin, rMax, ptmw->fInvert);
                } else { /* absolute equities */
                    sz = g_strdup_printf("[%s (%d,%d)]\n\n%s: \t\t%s\n%s: \t%s", _(szROLL), i+1,j+1, _(szABS),
                            GetEquityString(ptmw->atm[m].aarEquity[i][j], &ci, ptmw->fInvert),  _(szMOVE),
                            FormatMove(szMove, (ConstTanBoard) ptmw->atm[m].pms->anBoard,
                                                        ptmw->atm[m].aaanMove[i][j]));
                    // g_message("equity=%s",sz);
                    SetStyle(ptmw->atm[m].aapwDA[i][j], ptmw->atm[m].aarEquity[i][j], rMin, rMax, ptmw->fInvert);
                } 
                
                // /* colors */
                // if (!fShowDiff || m==0) /* absolute equities or 2nd-roll equities*/
                //     SetStyle(ptmw->atm[m].aapwDA[i][j], ptmw->atm[m].aarEquity[i][j], rMin, rMax, ptmw->fInvert);
                // else /* difference i.e. relative equities */
                //     SetStyleDiff(ptmw->atm[m].aapwDA[i][j], ptmw->atm[m].aarEquityDiff[i][j], ptmw->dMaxAbs);

                gtk_widget_set_tooltip_text(ptmw->atm[m].aapwe[i][j], sz);
                g_free(sz);
                gtk_widget_queue_draw(ptmw->atm[m].aapwDA[i][j]);

            }

        /* now dealing with the quadrant of the average on the top left of each map */
        if (m>0 && fShowDiff) { /* relative equities */
            sz = g_strdup_printf("[%s]\n\n%s: \t%s\n%s: \t%s", _(szAVG), _(szREL),
                            GetEquityDiffString(ptmw->atm[0].rAverage, ptmw->atm[m].rAverage,
                                                                &ci, ptmw->fInvert), _(szABS),
                            GetEquityString(ptmw->atm[m].rAverage, &ci, ptmw->fInvert));
            gtk_widget_set_tooltip_text(ptmw->atm[m].pweAverage,sz);
            SetStyleDiff(ptmw->atm[m].pwAverage, ptmw->atm[m].dAverage, ptmw->dMaxAbs);
        // } else if (m==1 && fShowTwoRolls) { /* 2nd-roll equities (i.e. comsidering opponent roll) following our roll in m=0 */
        //    gtk_widget_set_tooltip_text(ptmw->atm[m].pweAverage,
        //                             GetEquityString(ptmw->atm[m].rAverage, &ci, ptmw->fInvert));
        //     SetStyle(ptmw->atm[m].pwAverage, ptmw->atm[m].rAverage, rMin, rMax, ptmw->fInvert);
        } else { /* absolute equities OR 2nd-roll equities*/
            gtk_widget_set_tooltip_text(ptmw->atm[m].pweAverage,
                                    GetEquityString(ptmw->atm[m].rAverage, &ci, ptmw->fInvert));
            SetStyle(ptmw->atm[m].pwAverage, ptmw->atm[m].rAverage, rMin, rMax, ptmw->fInvert);
        } 
          
        gtk_widget_queue_draw(ptmw->atm[m].pwAverage);

    }

    /* update labels on gauge */
    if (fShowDiff) {
        gtk_label_set_text(GTK_LABEL(ptmw->apwGauge[0]), GetEquityDiffStringAux(-ptmw->dMaxAbs,&ci));
        gtk_label_set_text(GTK_LABEL(ptmw->apwGauge[1]), GetEquityDiffStringAux(ptmw->dMaxAbs,&ci));    
    } else {
        gtk_label_set_text(GTK_LABEL(ptmw->apwGauge[ptmw->fInvert]), GetEquityString(rMin, &ci, ptmw->fInvert));
        gtk_label_set_text(GTK_LABEL(ptmw->apwGauge[!ptmw->fInvert]), GetEquityString(rMax, &ci, ptmw->fInvert));
    } 
}

/* Function to draw quadrant and show equity and best move (in addition to the function above to show
 *  the tooltip with the equity and best move, and to choose the right color based on equity).
 *  All computations happen earlier. 
 */
static gboolean
DrawQuadrant(GtkWidget * pw, cairo_t * cr, tempmapwidget * ptmw)
{
    const int *pi = (int *) g_object_get_data(G_OBJECT(pw), "user_data");
    int i = 0;
    int j = 0;
    int m = 0;
    cubeinfo ci;
    PangoLayout *layout;
    PangoFontDescription *description;
    float y;
    GString *str;
    char *pch, *tmp, *tmp2=" ";
    GtkAllocation allocation;
    gtk_widget_get_allocation(pw, &allocation);

#if GTK_CHECK_VERSION(3,0,0)
    double *gbval = g_object_get_data(G_OBJECT(pw), "gbval");
    guint width, height;

    width = gtk_widget_get_allocated_width(pw);
    height = gtk_widget_get_allocated_height(pw);

    cairo_rectangle(cr, 0, 0, width, height);
    cairo_set_source_rgb(cr, 1.0, *gbval, *gbval);
    cairo_fill(cr);

    gtk_render_frame(gtk_widget_get_style_context(pw), cr, 0, 0, width, height);
#else
    (void) cr;                  /* silence compiler warning */
    gtk_paint_box(gtk_widget_get_style(pw), gtk_widget_get_window(pw), GTK_STATE_NORMAL,
                  GTK_SHADOW_IN, NULL, NULL, NULL, 0, 0, allocation.width, allocation.height);
#endif
    if (pi == NULL)
        return TRUE;

    if (*pi >= 0) {
        i = (*pi % 100) / 6;
        j = (*pi % 100) % 6;
        m = *pi / 100;
    } else {
        m = -(*pi + 1);
        j = -1;
    }

    str = g_string_new("");

    /* show equity */

    if (ptmw->fShowEquity) {
        GetMatchStateCubeInfo(&ci, ptmw->atm[0].pms);

        if (m==1 && fShowTwoRolls) { /* 2nd-roll equities for opponent roll following our roll in m=0 */
            if (j >= 0)
                tmp = GetEquityString(ptmw->oppTM[iDefault][jDefault].aarEquity[i][j], &ci, ptmw->fInvert);
                    // FormatMove(szMove, (ConstTanBoard) ptmw->atm[m].pms->anBoard,
                    //                             ptmw->oppTM[iDefault][jDefault].aaanMove[i][j])); /* TBD: check if board needs to be updated before */
            else if (j == -1)
                tmp = GetEquityString(ptmw->atm[m].rAverage, &ci, ptmw->fInvert);
            g_string_append(str, tmp);
        } else {
            /* absolute equities */
            float r = 0.0f;
            if (j >= 0)
                r = ptmw->atm[m].aarEquity[i][j];
            else if (j == -1)
                r = ptmw->atm[m].rAverage;
            // g_message("m=%d,i=%d,j=%d -> r=%f",m,i,j,r);
            tmp = GetEquityString(r, &ci, ptmw->fInvert);
            while (*tmp == ' ')
                tmp++;
            /* relative equities */
            if (fShowDiff && m>0) {
                // float auxEquity=GetEquity(r, &ci, ptmw->fInvert)-GetEquity(ptmw->atm[0].aarEquity[i][j], &ci, ptmw->fInvert);
                // if (i==0 && j==0)
                    // g_message("myequitydiff=%+.3f",auxEquity);
                if (j >= 0)
                    tmp2=GetEquityDiffString(ptmw->atm[0].aarEquity[i][j],r,&ci,ptmw->fInvert);
                else if (j == -1)
                    tmp2=GetEquityDiffString(ptmw->atm[0].rAverage,r,&ci,ptmw->fInvert);

                // g_message("m=%d,i=%d,j=%d -> tmp=%s",m,i,j,tmp);
                // tmp = sprintf(tmp,"%+.3f",auxEquity);

                g_string_append_printf(str, "%s (%s)", tmp2, tmp);
            } else
                g_string_append(str, tmp);
        }
    }

    /* show best move */

    if (j >= 0 && ptmw->fShowBestMove) {
        char szMove[FORMATEDMOVESIZE];
        if (m==1 && fShowTwoRolls) { /* 2nd-roll equities for opponent roll following our roll in m=0 */
            FormatMove(szMove, (ConstTanBoard) ptmw->atm[m].pms->anBoard,
                                                ptmw->oppTM[iDefault][jDefault].aaanMove[i][j]); 
        } else {
            // FormatMovePlain(szMove, (ConstTanBoard)ptmw->atm[m].pms->anBoard, ptmw->atm[m].aaanMove[i][j]);
            FormatMove(szMove, (ConstTanBoard)ptmw->atm[m].pms->anBoard, ptmw->atm[m].aaanMove[i][j]);
        }
        
        if (ptmw->fShowEquity)
            g_string_append_printf(str, " [%s]", szMove);
        else
            g_string_append(str, szMove);
    }

    if (str->len == 0) {
        g_string_free(str, TRUE);
        return TRUE;
    }

    pch = str->str;
    /* y is the starting y-axis position for the text? cf. gtk_paint_layout later */
    if (ptmw->fShowEquity && j >= 0 && ptmw->fShowBestMove)
        y = 2;
    else if (ptmw->fShowEquity)
        y = (float)(allocation.height - 4) / 2.0f;
    else
        y = 2 + (float)(allocation.height - 4) / 10.0f;
    // if (ptmw->fShowDiff)
    //     y+=1;

    description = pango_font_description_from_string("sans");
    // if (ptmw->fShowDiff)
    //     pango_font_description_set_size(description, allocation.height * PANGO_SCALE / 10);
    // else
    pango_font_description_set_size(description, allocation.height * PANGO_SCALE / 8);
    layout = gtk_widget_create_pango_layout(pw, NULL);
    pango_layout_set_font_description(layout, description);
    do {
        tmp = strchr(pch, ' ');
        if (tmp)
            *tmp = 0;
        pango_layout_set_text(layout, pch, -1);
#if GTK_CHECK_VERSION(3,0,0)
        gtk_render_layout(gtk_widget_get_style_context(pw), cr, 2.0, (double)y, layout);
#else
        gtk_paint_layout(gtk_widget_get_style(pw), gtk_widget_get_window(pw),
                                GTK_STATE_NORMAL, TRUE, NULL, pw, NULL, 2, (int) y, layout);
#endif
        if (tmp) {
            pch = tmp + 1;
            y += (float)(allocation.height - 4) / 5.0f;
        }
    } while (tmp);

    g_object_unref(layout);
    g_string_free(str, TRUE);

    return TRUE;
}

#if ! GTK_CHECK_VERSION(3,0,0)
static void
ExposeQuadrant(GtkWidget * pw, GdkEventExpose * UNUSED(pev), tempmapwidget * ptmw)
{
    cairo_t *cr = gdk_cairo_create(gtk_widget_get_window(pw));
    DrawQuadrant(pw, cr, ptmw);
    cairo_destroy(cr);
}
#endif

static void
ExposeDieArea(GtkWidget * pw, cairo_t * cr, gint area_x, gint area_y, gint area_width, gint area_height, tempmapwidget * ptmw)
{
    const int *pi = (int *) g_object_get_data(G_OBJECT(pw), "user_data");
    int x, y;
    int nSizeDie;
    GtkAllocation allocation;
    gtk_widget_get_allocation(pw, &allocation);

    nSizeDie = (allocation.width - 4) / 7;
    if (nSizeDie > ((allocation.height - 4) / 7))
        nSizeDie = (allocation.height - 4) / 7;

    if (ptmw->nSizeDie != nSizeDie) {
        int i;
        renderdata rd;

        /* render die */

        CopyAppearance(&rd);
        rd.nSize = ptmw->nSizeDie = nSizeDie;
#if defined(USE_BOARD3D)
        Copy3dDiceColour(&rd);
#endif

        for (i = 0; i < 2; ++i) {
            g_free(ptmw->achDice[i]);
            g_free(ptmw->achPips[i]);

            ptmw->achDice[i] = (unsigned char *) g_malloc(nSizeDie * nSizeDie * 7 * 7 * 4);
            ptmw->achPips[i] = (unsigned char *) g_malloc(nSizeDie * nSizeDie * 3);
        }

        RenderDice(&rd, ptmw->achDice[0], ptmw->achDice[1], nSizeDie * 7 * 4, FALSE);
        RenderPips(&rd, ptmw->achPips[0], ptmw->achPips[1], nSizeDie * 3);
    }

    x = (allocation.width - ptmw->nSizeDie * 7) / 2;
    y = (allocation.height - ptmw->nSizeDie * 7) / 2;

    cairo_save(cr);
    cairo_rectangle(cr, area_x, area_y, area_width, area_height);
    cairo_set_operator(cr, CAIRO_OPERATOR_CLEAR);
    cairo_fill(cr);
    cairo_restore(cr);
    DrawDie(cr, ptmw->achDice, ptmw->achPips, ptmw->nSizeDie, x, y, ptmw->atm[0].pms->fMove, *pi + 1, FALSE);
}

#if GTK_CHECK_VERSION(3,0,0)
static gboolean
ExposeDie(GtkWidget * pw, cairo_t * cr, tempmapwidget * ptmw)
{
    ExposeDieArea(pw, cr, 3, 3, gtk_widget_get_allocated_width(pw) - 6, gtk_widget_get_allocated_height(pw) - 6, ptmw);
    return TRUE;
}
#else
static void
ExposeDie(GtkWidget * pw, GdkEventExpose * pev, tempmapwidget * ptmw)
{
    cairo_t *cr = gdk_cairo_create(gtk_widget_get_window(pw));
    ExposeDieArea(pw, cr, pev->area.x, pev->area.y, pev->area.width, pev->area.height, ptmw);
    cairo_destroy(cr);
}
#endif


static void 
DrawGauge(tempmapwidget *ptmw)
{
    for (int i = 0; i < 32; ++i) {

//         ptmw->aapwGaugeDA[i] = gtk_drawing_area_new();
//         gtk_widget_set_size_request(ptmw->aapwGaugeDA[i], 7, 20);

// #if GTK_CHECK_VERSION(3,0,0)
//         gtk_grid_attach(GTK_GRID(ptmw->pwGauge), ptmw->aapwGaugeDA[i], i, 1, 1, 1);
//         gtk_widget_set_hexpand(ptmw->aapwGaugeDA[i], TRUE);
// #else
//         gtk_table_attach_defaults(GTK_TABLE(ptmw->pwGauge), ptmw->aapwGaugeDA[i], i, i + 1, 1, 2);
// #endif

//         g_object_set_data(G_OBJECT(ptmw->aapwGaugeDA[i]), "user_data", NULL);

// #if GTK_CHECK_VERSION(3,0,0)
//         gtk_style_context_add_class(gtk_widget_get_style_context(ptmw->aapwGaugeDA[i]), "gnubg-temp-map-quadrant");
//         g_signal_connect(G_OBJECT(ptmw->aapwGaugeDA[i]), "draw", G_CALLBACK(DrawQuadrant), NULL);
// #else
//         g_signal_connect(G_OBJECT(ptmw->aapwGaugeDA[i]), "expose_event", G_CALLBACK(ExposeQuadrant), NULL);
// #endif

        if (!fShowDiff) {
            UpdateStyle(ptmw->aapwGaugeDA[i], (float)i / 31.0f, 0);
            gtk_widget_set_tooltip_text(ptmw->aapweGauge[i], _("Gauge for the absolute equity used in the temperature maps"));        
        } else {
            if (i-15.5f>0)
                UpdateStyle(ptmw->aapwGaugeDA[i], (float)((i-15.5f) / 15.5f), 1);
            else
                UpdateStyle(ptmw->aapwGaugeDA[i], (float)(-(i-15.5f) / 15.5f), 2);
            gtk_widget_set_tooltip_text(ptmw->aapweGauge[i], _("Gauge for the relative equity used in the relative-equity temperature maps"));        
        }
        // g_message("fShowDiff=%d",fShowDiff);
        gtk_widget_queue_draw(ptmw->aapwGaugeDA[i]);
    }
}

static void
UpdateTitles(tempmapwidget * ptmw) {
    char buf[100];
    // char szMove[FORMATEDMOVESIZE];

    for (int m = 0; m < ptmw->n; ++m) {
        if (fShowMode == 0)
            gtk_frame_set_label(GTK_FRAME(ptmw->atm[m].Frame),ptmw->atm[m].szTitle);
        else {
            if (fShowDiff) {
                if (m==0)
                    sprintf(buf, _("%s: basis equity"), ptmw->atm[m].szTitle);
                else
                    sprintf(buf, _("%s: equity relative to basis"), ptmw->atm[m].szTitle);
            } else { /* 2roll mode */
                if (m==0)
                    sprintf(buf, _("%s: 1st-roll equity"), ptmw->atm[m].szTitle);
                else {
                    // g_message("i=%d,j=%d,move=%s",iDefault,jDefault,ptmw->oppTM[iDefault][jDefault].TwoRollFirstMove);
                    // FormatMove(szMove, (ConstTanBoard)anBoard, ptmw->TwoRollFirstMove);
                    // FormatMove(szMove, (ConstTanBoard)ptmw->atm[0].pms->anBoard, ptmw->TwoRollFirstMove[i][j]);
                    sprintf(buf, _("2nd-roll equity. The 1st roll was (%d,%d) with best move %s"), 
                        iDefault+1,jDefault+1,ptmw->oppTM[iDefault][jDefault].TwoRollFirstMove);
                }
            }
            gtk_frame_set_label(GTK_FRAME(ptmw->atm[m].Frame),buf);
        }
    }
}

static void
HideGhostMaps(tempmapwidget * ptmw) {

    gtk_widget_show_all(ptmw->pwContainer); /* use this if non-modal*/

    /* hiding ghost maps */
    if (ptmw->n==1 && !fShowTwoRolls)
        gtk_widget_hide(ptmw->atm[1].Frame);   
    else if (ptmw->n>2 && fShowTwoRolls) {
        for (int m = 2; m < ptmw->n; ++m) {
            gtk_widget_hide(ptmw->atm[m].Frame);   
        }
    }
}


/* here we update all TempMap elements assuming the eval ply has not changed */
static void
UpdateAll(tempmapwidget * ptmw) {
    // if (fShowTwoRolls) {
    //     // evalcontext ec = { TRUE, 0, FALSE, TRUE, 0.0, FALSE};
    //     SecondRollEquities(ptmw->atm[0].pms, ptmw->oppTM[iDefault][jDefault].aarEquity, ptmw->oppTM[iDefault][jDefault].aaanMove,
    //                                     ptmw->atm[0].szTitle);
    // } 
    HideGhostMaps(ptmw);
    UpdateTempMapEquities(ptmw);
    DrawGauge(ptmw);
    UpdateTitles(ptmw);
}

static void
TempMapPlyToggled(GtkWidget * pw, tempmapwidget * ptmw)
{
    const int *pi = (int *) g_object_get_data(G_OBJECT(pw), "user_data");

    // evalcontext ec = { TRUE, 0, FALSE, TRUE, 0.0, FALSE };

    if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(pw))) {

        /* recalculate equities */

        ptmw->ec.nPlies = *pi;

        if (CalcTempMapEquities(ptmw, TRUE))
            return;

        UpdateAll(ptmw);
        // UpdateTempMapEquities(ptmw);

    }

}


static void
ShowEquityToggled(GtkWidget * pw, tempmapwidget * ptmw)
{

    int f = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(pw));

    if (f != ptmw->fShowEquity) {
        fShowEquity = ptmw->fShowEquity = f;
        UpdateTempMapEquities(ptmw);
    }

}


static void
ShowBestMoveToggled(GtkWidget * pw, tempmapwidget * ptmw)
{

    int f = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(pw));

    if (f != ptmw->fShowBestMove) {
        fShowBestMove = ptmw->fShowBestMove = f;
        UpdateTempMapEquities(ptmw);
    }
}



// static void
// ShowDiffToggled(GtkWidget * pw, tempmapwidget * ptmw)
// {

//     int f = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(pw));

//     if (f != ptmw->fShowDiff) {
//         fShowDiff = ptmw->fShowDiff = f;
//         UpdateTempMapEquities(ptmw);
//         DrawGauge(ptmw);
//     }
//     // if (fShowDiff){
//     //     gtk_widget_hide(ptmw->pwGauge);
//     //     gtk_widget_show(ptmw->pwGaugeDiff);
//     // } else {
//     //     gtk_widget_hide(ptmw->pwGaugeDiff);
//     //     gtk_widget_show(ptmw->pwGauge);
//     // }
//     for (int i = 0; i < ptmw->n; ++i) {
//         UpdateTitles(ptmw,i);
//     }
// }

// static void
// ShowTwoRollsToggled(GtkWidget * pw, tempmapwidget * ptmw)
// {

//     int f = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(pw));

//     if (f != ptmw->fShowTwoRolls) {
//         fShowTwoRolls = ptmw->fShowTwoRolls = f;
//         UpdateTempMapEquities(ptmw);
//         DrawGauge(ptmw);
//             for (int i = 0; i < ptmw->n; ++i) {
//         UpdateTitles(ptmw,i);
//     }
//     }
// }

static void
ShowModeToggled(GtkWidget * pw, tempmapwidget * ptmw)
/* This is called by gtk when the user clicks on the "display eval" radio buttons.
*/
{
    int OldMode;
    int *pi = (int *) g_object_get_data(G_OBJECT(pw), "user_data");
    if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(pw) )) {
        if (fShowMode != (*pi)) {
            OldMode = fShowMode;
            g_message("*pi=%d",*pi);
            fShowMode = ptmw->fShowMode = (*pi);
            fShowDiff = ptmw->fShowDiff = (*pi==1);
            fShowTwoRolls = ptmw->fShowTwoRolls = (*pi==2);
            if (OldMode == 2 && fTwoRollsSelected) { /* we used the maps for 2roll mode */
                fTwoRollsSelected = FALSE;
                if (CalcTempMapEquities(ptmw, TRUE)) /* don't recompute m=0, only the other maps*/
                    return;
            } else {
                fTwoRollsSelected = FALSE;
                if (CalcTempMapEquities(ptmw, FALSE)) /* don't recompute m=0, only the other maps*/
                    return;
            }
            UpdateAll(ptmw);
            if (fShowTwoRolls) {
                outputerrf("%s",szTwoRollSelect);
            }
        }
    }
}

static void
key_press(GtkWidget * pw, GdkEvent * UNUSED(event), tempmapwidget * ptmw)
{
    int m,i,j;

    
    const int *pi = (int *) g_object_get_data(G_OBJECT(pw), "user_data");
    /*  *pi = i * 6 + j + m * 100; */
    g_assert(*pi >= 0);
    m = *pi / 100;
    i = (*pi % 100) / 6;
    j = (*pi % 100) % 6;
    if (!(m>0 && fShowTwoRolls)) { /*exclude case where we click on the 2nd-roll answer*/
        fTwoRollsSelected = TRUE;
        if (fShowTwoRolls) { /* zoom in on some square of TM m=0 */
            ptmw->TM1 = ptmw->atm[m];
            // memcpy(ptmw->TM1, ptmw->atm[m], sizeof(tempmap));
        } else {
            /* we enter a 2roll mode */
            fShowMode = ptmw->fShowMode = 2;
            fShowDiff = ptmw->fShowDiff = FALSE;
            fShowTwoRolls = ptmw->fShowTwoRolls = TRUE;
        }
        iDefault=i;
        jDefault=j;

        if (CalcTempMapEquities(ptmw, FALSE)) /* don't recompute m=0, only the other maps*/
            return;



    }

    g_assert(m==0 && 0<=iDefault && iDefault<=5 && 0<=jDefault && jDefault<=5);
    g_message("m=%d,i=%d,j=%d",m,iDefault,jDefault);

    UpdateAll(ptmw);

        // if (iDefault==5 && jDefault==5) {
        //     g_message("hide");
        //     gtk_widget_hide(ptmw->atm[1].Frame);        
        // }
    

}

static void
DestroyDialog(gpointer p, GObject * UNUSED(obj))
{
    tempmapwidget *ptmw = (tempmapwidget *) p;
    int i;
    int n2=MAX(ptmw->n,2);
    /* garbage collect */

    g_free(ptmw->achDice[0]);
    g_free(ptmw->achDice[1]);
    g_free(ptmw->achPips[0]);
    g_free(ptmw->achPips[1]);
    for (i = 0; i < n2; ++i) {
        g_free(ptmw->atm[i].pms);
        g_free(ptmw->atm[i].szTitle);
    }

    g_free(ptmw->atm);

    g_free(ptmw);

}


extern void
GTKShowTempMap(const matchstate ams[], const int n, gchar * aszTitle[], const int fInvert, const int fCube)
{

    evalcontext ec = { TRUE, 0, FALSE, TRUE, 0.0, FALSE};

    tempmapwidget *ptmw;
    int *pi;
    int i, j;

    GtkWidget *pwDialog;
    GtkWidget *pwv;
    GtkWidget *pw;
    GtkWidget *pwh;
    GtkWidget *pwh2;
    GtkWidget *pwhgrid;
    GtkWidget *pwvgrid;
    // GtkWidget *pwhbox;
    GtkWidget *pwx = NULL;
#if GTK_CHECK_VERSION(3,0,0)
    // GtkWidget *pwOuterGrid;
    GtkWidget *pwGrid = NULL;
#else
    // GtkWidget *pwOuterTable;
    GtkWidget *pwTable = NULL;
#endif
    GtkWidget * pwFrame;

    int k, l, km, lm, m;

    /* dialog */
    if (!cubeTempMapAtMoney) {
        pwDialog = GTKCreateDialog(_("Sho Sengoku Temperature Map - Distribution of Rolls"),
                               DT_INFO, NULL, DIALOG_FLAG_MINMAXBUTTONS, NULL, NULL);
                            //    DT_INFO, NULL, DIALOG_FLAG_MODAL, NULL, NULL);
    } else {
        pwDialog = GTKCreateDialog(_("Temperature Map in Hypothetical Money Play"),
                               DT_INFO, NULL, DIALOG_FLAG_MINMAXBUTTONS, NULL, NULL);
    }
                               

    ptmw = (tempmapwidget *) g_malloc(sizeof(tempmapwidget));
    ptmw->fShowBestMove = fShowBestMove;
    ptmw->fShowEquity = fShowEquity;
    ptmw->fInvert = fInvert;
    ptmw->n = n;
    // if (n<=1)
    //     fShowDiff = 0;
    ptmw->fShowDiff = fShowDiff;
    if (!fCube)
        fShowTwoRolls = 0; /* only interesting in cube decisions */ 
    ptmw->fShowTwoRolls = fShowTwoRolls;
    ptmw->fShowMode = fShowMode;
    ptmw->nSizeDie = -1;
    ptmw->achDice[0] = ptmw->achDice[1] = NULL;
    ptmw->achPips[0] = ptmw->achPips[1] = NULL;
    ptmw->dMaxAbs=1; /* only to initialize and avoid division by 0 */
    ptmw->ec=ec;

    /* if n=1, we build 2 maps for the 2roll mode, but hide the 2nd map */
    int n2=MAX(n,2);

    ptmw->atm = (tempmap *) g_malloc(n2 * sizeof(tempmap));
    for (i = 0; i < n2; ++i) {
        ptmw->atm[i].pms = (matchstate *) g_malloc(sizeof(matchstate));
        if (i<n)
            memcpy(ptmw->atm[i].pms, &ams[i], sizeof(matchstate));
    }

    /* vbox to hold tree widget and buttons */

#if GTK_CHECK_VERSION(3,0,0)
    pwv = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
#else
    pwv = gtk_vbox_new(FALSE, 6);
#endif
    gtk_container_set_border_width(GTK_CONTAINER(pwv), 6);
    gtk_container_add(GTK_CONTAINER(DialogArea(pwDialog, DA_MAIN)), pwv);

    ptmw->pwContainer = pwv;

    /* calculate number of rows and columns */

    for (lm = 1; /**/; ++lm) /* computing lm=max(l)+1*/
        if (lm * lm >= n2)
            break;

    for (km = 1; km * lm < n2; ++km); /* computing km=max(k)+1*/

    g_message("n=%d,n2=%d,km=%d,lm=%d",n,n2,km,lm);

// #if GTK_CHECK_VERSION(3,0,0)
//     pwOuterGrid = gtk_grid_new();
//     gtk_grid_set_column_homogeneous(GTK_GRID(pwOuterGrid), TRUE);
//     gtk_grid_set_row_homogeneous(GTK_GRID(pwOuterGrid), TRUE);
//     gtk_box_pack_start(GTK_BOX(pwv), pwOuterGrid, TRUE, TRUE, 0);
// #else
//     pwOuterTable = gtk_table_new(km, lm, TRUE);
//     gtk_box_pack_start(GTK_BOX(pwv), pwOuterTable, TRUE, TRUE, 0);
// #endif

    for (k = m = 0; k < km; ++k) { /* m is the global index, (k,l) the position; km,lm are k_max, l_max as computed previously, not k*m and l*m!*/
        // g_message("in k: n=%d,n2=%d,k=%d < km=%d?, m=%d",n,n2,k,km,m); /* why does this start with k=1? shouldn't it be k=0???*/

#if GTK_CHECK_VERSION(3,0,0)
        pwhgrid = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
#else
        pwhgrid = gtk_hbox_new(FALSE, 0);
#endif
        gtk_box_pack_start(GTK_BOX(pwv), pwhgrid, FALSE, FALSE, 0);

        for (l = 0; l < lm && m < n2; ++l, ++m) { 
            // g_message("in l: n=%d,n2=%d,k=%d < km=%d?, l=%d < lm=%d? ,m=%d",n,n2,k,km,l,lm,m); 

#if GTK_CHECK_VERSION(3,0,0)
            pwvgrid = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
#else
            pwvgrid = gtk_vbox_new(FALSE, 6);
#endif
            gtk_box_pack_start(GTK_BOX(pwhgrid), pwvgrid, FALSE, FALSE, 0);

            tempmap *ptm = &ptmw->atm[m];

            if (m<n)
                ptm->szTitle = (aszTitle && aszTitle[m] && *aszTitle[m]) ? g_strdup(aszTitle[m]) : NULL;
            else
                ptm->szTitle = g_strdup("dummy frame for 2roll mode"); /* this is purposeful so we can g_free it like the other strings */

            ptm->Frame = gtk_frame_new(ptm->szTitle);    
            // gtk_container_add(GTK_CONTAINER(pwhgrid), pw);
            gtk_box_pack_start(GTK_BOX(pwvgrid), ptm->Frame, FALSE, FALSE, 0);

#if GTK_CHECK_VERSION(3,0,0)
            // gtk_grid_attach(GTK_GRID(pwOuterGrid), ptm->Frame, l, k, 1, 1);

            pwGrid = gtk_grid_new();
            gtk_grid_set_column_homogeneous(GTK_GRID(pwGrid), TRUE);
            gtk_grid_set_row_homogeneous(GTK_GRID(pwGrid), TRUE);
            gtk_container_add(GTK_CONTAINER(ptm->Frame), pwGrid);
#else
            // gtk_table_attach_defaults(GTK_TABLE(pwOuterTable), ptm->Frame, l, l + 1, k, k + 1);

            pwTable = gtk_table_new(7, 7, TRUE);
            gtk_container_add(GTK_CONTAINER(ptm->Frame), pwTable);
#endif

            /* drawing areas */

            for (i = 0; i < 6; ++i) {
                for (j = 0; j < 6; ++j) {

                    ptm->aapwDA[i][j] = gtk_drawing_area_new();
                    ptm->aapwe[i][j] = gtk_event_box_new();
                    gtk_event_box_set_visible_window(GTK_EVENT_BOX(ptm->aapwe[i][j]), FALSE);

                    gtk_container_add(GTK_CONTAINER(ptm->aapwe[i][j]), ptm->aapwDA[i][j]);

                    gtk_widget_set_size_request(ptm->aapwDA[i][j], SIZE_QUADRANT, SIZE_QUADRANT);

#if GTK_CHECK_VERSION(3,0,0)
                    gtk_grid_attach(GTK_GRID(pwGrid), ptm->aapwe[i][j], i + 1, j + 1, 1, 1);
#else
                    gtk_table_attach_defaults(GTK_TABLE(pwTable), ptm->aapwe[i][j], i + 1, i + 2, j + 1, j + 2);
#endif

                    pi = (int *) g_malloc(sizeof(int));
                    *pi = i * 6 + j + m * 100;

                    g_object_set_data_full(G_OBJECT(ptm->aapwDA[i][j]), "user_data", pi, g_free); /* (1) sets association from object to pi using
                                                        key "user_data", and (2) adds notification for when the association is destroyed */
#if GTK_CHECK_VERSION(3,0,0)
                    gtk_style_context_add_class(gtk_widget_get_style_context(ptm->aapwDA[i][j]), "gnubg-temp-map-quadrant");
                    g_signal_connect(G_OBJECT(ptm->aapwDA[i][j]), "draw", G_CALLBACK(DrawQuadrant), ptmw);
#else
                    g_signal_connect(G_OBJECT(ptm->aapwDA[i][j]), "expose_event", G_CALLBACK(ExposeQuadrant), ptmw);
#endif
                    gtk_widget_add_events(ptm->aapwDA[i][j], GDK_BUTTON_PRESS_MASK);
                    g_signal_connect(G_OBJECT(ptm->aapwDA[i][j]), "button_press_event", G_CALLBACK(key_press), ptmw);
                            // sprintf(sz, _("In edit mode, click on this checker to set player %d on roll."), iPlayer);
                        // gtk_widget_set_tooltip_text(ptm->aapwDA[i][j], _("In two-roll equity mode, click on this square to map the corresponding second-roll equities."));
                }

                /* die */

                pw = gtk_drawing_area_new();
                gtk_widget_set_size_request(pw, SIZE_QUADRANT, SIZE_QUADRANT);

#if GTK_CHECK_VERSION(3,0,0)
                gtk_grid_attach(GTK_GRID(pwGrid), pw, 0, i + 1, 1, 1);
#else
                gtk_table_attach_defaults(GTK_TABLE(pwTable), pw, 0, 1, i + 1, i + 2);
#endif

                pi = (int *) g_malloc(sizeof(int));
                *pi = i;

                g_object_set_data_full(G_OBJECT(pw), "user_data", pi, g_free);

#if GTK_CHECK_VERSION(3,0,0)
                g_signal_connect(G_OBJECT(pw), "draw", G_CALLBACK(ExposeDie), ptmw);
#else
                g_signal_connect(G_OBJECT(pw), "expose_event", G_CALLBACK(ExposeDie), ptmw);
#endif
                /* die */

                pw = gtk_drawing_area_new();
                gtk_widget_set_size_request(pw, SIZE_QUADRANT, SIZE_QUADRANT);

#if GTK_CHECK_VERSION(3,0,0)
                gtk_grid_attach(GTK_GRID(pwGrid), pw, i + 1, 0, 1, 1);
#else
                gtk_table_attach_defaults(GTK_TABLE(pwTable), pw, i + 1, i + 2, 0, 1);
#endif

                pi = (int *) g_malloc(sizeof(int));
                *pi = i;

                g_object_set_data_full(G_OBJECT(pw), "user_data", pi, g_free);

#if GTK_CHECK_VERSION(3,0,0)
                g_signal_connect(G_OBJECT(pw), "draw", G_CALLBACK(ExposeDie), ptmw);
#else
                g_signal_connect(G_OBJECT(pw), "expose_event", G_CALLBACK(ExposeDie), ptmw);
#endif

            }

            /* drawing area for average */

            ptm->pwAverage = gtk_drawing_area_new();
            ptm->pweAverage = gtk_event_box_new();
            gtk_event_box_set_visible_window(GTK_EVENT_BOX(ptm->pweAverage), FALSE);

            gtk_container_add(GTK_CONTAINER(ptm->pweAverage), ptm->pwAverage);

            gtk_widget_set_size_request(ptm->pwAverage, SIZE_QUADRANT, SIZE_QUADRANT);

#if GTK_CHECK_VERSION(3,0,0)
            gtk_grid_attach(GTK_GRID(pwGrid), ptm->pweAverage, 0, 0, 1, 1);
#else
            gtk_table_attach_defaults(GTK_TABLE(pwTable), ptm->pweAverage, 0, 1, 0, 1);
#endif

            pi = (int *) g_malloc(sizeof(int));
            *pi = -m - 1;

            g_object_set_data_full(G_OBJECT(ptm->pwAverage), "user_data", pi, g_free);

#if GTK_CHECK_VERSION(3,0,0)
            gtk_style_context_add_class(gtk_widget_get_style_context(ptm->pwAverage), "gnubg-temp-map-quadrant");
            g_signal_connect(G_OBJECT(ptm->pwAverage), "draw", G_CALLBACK(DrawQuadrant), ptmw);
#else
            g_signal_connect(G_OBJECT(ptm->pwAverage), "expose_event", G_CALLBACK(ExposeQuadrant), ptmw);
#endif

        }
    }

    /* map titles */
    UpdateTitles(ptmw);

    /* separator */

#if GTK_CHECK_VERSION(3,0,0)
    gtk_box_pack_start(GTK_BOX(pwv), gtk_separator_new(GTK_ORIENTATION_HORIZONTAL), FALSE, FALSE, 0);
#else
    gtk_box_pack_start(GTK_BOX(pwv), gtk_hseparator_new(), FALSE, FALSE, 0);
#endif

    /* gauge */

#if GTK_CHECK_VERSION(3,0,0)
    ptmw->pwGauge = gtk_grid_new();
    gtk_box_pack_start(GTK_BOX(pwv), ptmw->pwGauge, FALSE, FALSE, 0);
#else
    ptmw->pwGauge =  gtk_table_new(2, 32, FALSE);
    gtk_box_pack_start(GTK_BOX(pwv), ptmw->pwGauge, FALSE, FALSE, 0);
#endif

    for (int i = 0; i < 32; ++i) {

        ptmw->aapwGaugeDA[i] = gtk_drawing_area_new();
        ptmw->aapweGauge[i] = gtk_event_box_new();
        
        gtk_event_box_set_visible_window(GTK_EVENT_BOX(ptmw->aapweGauge[i]), FALSE);
        gtk_container_add(GTK_CONTAINER(ptmw->aapweGauge[i]), ptmw->aapwGaugeDA[i]);
        
        gtk_widget_set_size_request(ptmw->aapwGaugeDA[i], 7, 20);

#if GTK_CHECK_VERSION(3,0,0)
        gtk_grid_attach(GTK_GRID(ptmw->pwGauge), ptmw->aapweGauge[i], i, 1, 1, 1);
        gtk_widget_set_hexpand(ptmw->aapweGauge[i], TRUE);
#else
        gtk_table_attach_defaults(GTK_TABLE(ptmw->pwGauge), ptmw->aapweGauge[i], i, i + 1, 1, 2);
#endif

        g_object_set_data(G_OBJECT(ptmw->aapwGaugeDA[i]), "user_data", NULL);

#if GTK_CHECK_VERSION(3,0,0)
        gtk_style_context_add_class(gtk_widget_get_style_context(ptmw->aapweGauge[i]), "gnubg-temp-map-quadrant");
        g_signal_connect(G_OBJECT(ptmw->aapweGauge[i]), "draw", G_CALLBACK(DrawQuadrant), NULL);
#else
        g_signal_connect(G_OBJECT(ptmw->aapwGaugeDA[i]), "expose_event", G_CALLBACK(ExposeQuadrant), NULL);
#endif
    }

    DrawGauge(ptmw);

    for (i = 0; i < 2; ++i) {
        ptmw->apwGauge[i] = gtk_label_new("");
#if GTK_CHECK_VERSION(3,0,0)
        gtk_grid_attach(GTK_GRID(ptmw->pwGauge), ptmw->apwGauge[i], 31 * i, 0, 1, 1);
#else
        gtk_table_attach_defaults(GTK_TABLE(ptmw->pwGauge), ptmw->apwGauge[i], 31 * i, 31 * i + 1, 0, 1);
#endif
    }

    /* separator */

#if GTK_CHECK_VERSION(3,0,0)
    gtk_box_pack_start(GTK_BOX(pwv), gtk_separator_new(GTK_ORIENTATION_HORIZONTAL), FALSE, FALSE, 0);
#else
    gtk_box_pack_start(GTK_BOX(pwv), gtk_hseparator_new(), FALSE, FALSE, 0);
#endif

    /* buttons */


    /* frame for TM modes*/
    char * szToolTip = "temp";
    // if (n >= 2) {
    pwFrame=gtk_frame_new(_("Map:")); //"In all maps except the first, display:"
    gtk_box_pack_start(GTK_BOX(pwv), pwFrame, FALSE, FALSE, 0);
    gtk_widget_set_tooltip_text(pwFrame, _(szToolTip)); 
    // gtk_widget_set_sensitive(pwFrame, TRUE);

#if GTK_CHECK_VERSION(3,0,0)
    pwh2 = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
#else
    pwh2 = gtk_hbox_new(FALSE, 8);
#endif
    gtk_container_add(GTK_CONTAINER(pwFrame), pwh2);

    for(i=0; i<3; i++) {
        if (!(i==1 && n==1)) { /* don't offer a diff button when there is a single map */
            if (i==0) {
                pw = pwx = gtk_radio_button_new_with_label(NULL, _(aszTempMap[0])); // First radio button
            } else { //if ((i==1) || ((i==2) && fCube) ){
                pw =  gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(pwx), _(aszTempMap[i])); // Associate this to the other radio buttons
            }
            gtk_box_pack_start(GTK_BOX(pwh2), pw, FALSE, FALSE, 0);
            gtk_widget_set_tooltip_text(pw, _(aszTMTooltip[i]));
            pi = (int *)g_malloc(sizeof(int));
            *pi=(int)i; // here use "=(int)labelEnum[i];" and put it in the input of the function if needed, while
                        //  defining sth like " int labelEnum[] = { NUMBERS, ENGLISH, BOTH };" before calling the function
            g_object_set_data_full(G_OBJECT(pw), "user_data", pi, g_free);
            if (fShowMode==i) // again use "if (DefaultLabel==labelEnum[i])" if needed
                gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(pw), 1); //we set this to toggle it on in case it's the default option
            g_signal_connect(G_OBJECT(pw), "toggled", G_CALLBACK(ShowModeToggled), ptmw);
        }
    }
    // }

    /* k-ply eval*/
#if GTK_CHECK_VERSION(3,0,0)
    pwh = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
#else
    pwh = gtk_hbox_new(FALSE, 4);
#endif
    gtk_box_pack_start(GTK_BOX(pwv), pwh, FALSE, FALSE, 0);

    for (i = 0; i < 4; ++i) {

        gchar *sz = g_strdup_printf(_("%d ply"), i);
        if (i == 0)
            pw = pwx = gtk_radio_button_new_with_label(NULL, sz);
        else
            pw = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(pwx), sz);
        g_free(sz);

        gtk_box_pack_start(GTK_BOX(pwh), pw, FALSE, FALSE, 0);

        pi = (int *) g_malloc(sizeof(int));
        *pi = i;

        g_object_set_data_full(G_OBJECT(pw), "user_data", pi, g_free);

        g_signal_connect(G_OBJECT(pw), "toggled", G_CALLBACK(TempMapPlyToggled), ptmw);

    }

    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(pwx), TRUE);


//     /* show-buttons */

//     if (n < 2) {
// #if GTK_CHECK_VERSION(3,0,0)
//         pwh = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
// #else
//         pwh = gtk_hbox_new(FALSE, 4);
// #endif
//         gtk_box_pack_start(GTK_BOX(pwv), pwh, FALSE, FALSE, 0);
//     }
  
    // /* radio button for O'Hagan's Rule */
    // if (n==2) {
    //     pw = gtk_check_button_new_with_label(_("Show O'Hagan"));
    //     gtk_toggle_button_set_active((GtkToggleButton *) pw, ptmw->fShowTwoRolls);
    //     gtk_box_pack_end(GTK_BOX(pwh), pw, FALSE, FALSE, 0);
    //     g_signal_connect(G_OBJECT(pw), "toggled", G_CALLBACK(ShowTwoRollsToggled), ptmw);
    // }

    // /* radio button for relative equities */
    // if (n>1) {
    //     pw = gtk_check_button_new_with_label(_("Show diff"));
    //     gtk_toggle_button_set_active((GtkToggleButton *) pw, ptmw->fShowDiff);
    //     gtk_box_pack_end(GTK_BOX(pwh), pw, FALSE, FALSE, 0);
    //     g_signal_connect(G_OBJECT(pw), "toggled", G_CALLBACK(ShowDiffToggled), ptmw);
    // }



//     pwev = gtk_event_box_new();
//     gtk_event_box_set_visible_window(GTK_EVENT_BOX(pwev), FALSE);
//     gtk_box_pack_end(GTK_BOX(pwh), pwev, FALSE, FALSE, 4);
// #if GTK_CHECK_VERSION(3,0,0)
//     pwhbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
// #else
//     pwhbox = gtk_hbox_new(FALSE, 4);
// #endif
//     gtk_container_add(GTK_CONTAINER(pwev), pwhbox);

//     pw = gtk_combo_box_text_new();

//     gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(pw), _("Absolute equity"));
//     gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(pw), _("Relative equity"));
//     gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(pw), _("O'Hagan's law"));

//     gtk_combo_box_set_active(GTK_COMBO_BOX(pow->pwShowPips), gui_show_pips);
//     gtk_box_pack_start(GTK_BOX(pwhbox), gtk_label_new(_("Show Pips")), FALSE, FALSE, 0);
//     gtk_box_pack_start(GTK_BOX(pwhbox), pow->pwShowPips, FALSE, FALSE, 0);
//     gtk_box_pack_start(GTK_BOX(pwvbox), pwhbox, FALSE, FALSE, 0);

//     gtk_box_pack_end(GTK_BOX(pwhbox), pw, FALSE, FALSE, 0);
//     for (ppch = aszTempMap; *ppch; ppch++) {
//         gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(pow->pwQuizSkill), gettext(*ppch));
//     }
//     // g_assert(nQuizSkillCurrent >= 0 && nQuizSkillCurrent <= 2);
//     pow->pwShowPips = gtk_combo_box_text_new();
        
    // gtk_widget_set_tooltip_text(pwev,
    //                             _("Specify how bad GNU Backgammon must think a "
    //                               "decision is before adding it to the quiz positions."));
    // gtk_box_pack_start(GTK_BOX(pwhbox), gtk_label_new(_("Test:")), FALSE, FALSE, 0);


    pw = gtk_check_button_new_with_label(_("Text for best move"));
    gtk_toggle_button_set_active((GtkToggleButton *) pw, ptmw->fShowBestMove);
    gtk_box_pack_end(GTK_BOX(pwh), pw, FALSE, FALSE, 0);
    g_signal_connect(G_OBJECT(pw), "toggled", G_CALLBACK(ShowBestMoveToggled), ptmw);

    pw = gtk_check_button_new_with_label(_("Text for equity"));
    gtk_toggle_button_set_active((GtkToggleButton *) pw, ptmw->fShowEquity);
    gtk_box_pack_end(GTK_BOX(pwh), pw, FALSE, FALSE, 0);
    g_signal_connect(G_OBJECT(pw), "toggled", G_CALLBACK(ShowEquityToggled), ptmw);




    /* update */

    CalcTempMapEquities(ptmw, TRUE);
    UpdateTempMapEquities(ptmw);


    /* modality */

    gtk_window_set_default_size(GTK_WINDOW(pwDialog), 400, 500);
    g_object_weak_ref(G_OBJECT(pwDialog), DestroyDialog, ptmw);

    gtk_widget_show_all (pwDialog); /* use this if non-modal*/
    // GTKRunDialog(pwDialog); /* use this if modal */

    HideGhostMaps(ptmw);

    /* in 2roll mode, choose some default TM for the 1st roll  */
    if (fShowTwoRolls) {
        fTwoRollsSelected=FALSE;
        // outputerrf(_("Click on some square to select it as a first roll and check the opponent's second-roll responses."));
        outputerrf("%s", szTwoRollSelect);
        // memcpy(ptmw->TM1, ptmw->atm[m], sizeof ptmw->atm[m]);
    }

    // /* hiding ghost maps */
    // if (n==1 && !fShowTwoRolls)
    //     gtk_widget_hide(ptmw->atm[1].Frame);   
    // else if (n>2 && fShowTwoRolls) {
    //     for (m = 2; m < n; ++m) {
    //         gtk_widget_hide(ptmw->atm[m].Frame);   
    //     }
    // }


    // return ptmw;
}

// extern void
// GTKShowTempMap(const matchstate ams[], const int n, gchar * aszTitle[], const int fInvert)
// {
//     tempmapwidget *ptmw = GTKShowTempMapAux(ams, n, aszTitle, fInvert);
//     // g_message("fShowDiff=%d",fShowDiff);
//     // if (fShowDiff){
//     //     gtk_widget_hide(ptmw->pwGauge);
//     //     gtk_widget_show(ptmw->pwGaugeDiff);
//     // } else {
//     //     gtk_widget_hide(ptmw->pwGaugeDiff);
//     //     gtk_widget_show(ptmw->pwGauge);
//     // }
// }