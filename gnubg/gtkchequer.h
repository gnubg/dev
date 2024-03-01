/*
 * Copyright (C) 2002-2003 Joern Thyssen <jthyssen@dk.ibm.com>
 * Copyright (C) 2004-2021 the AUTHORS
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
 * $Id: gtkchequer.h,v 1.24 2022/01/19 22:45:05 plm Exp $
 */

#ifndef GTKCHEQUER_H
#define GTKCHEQUER_H

#include "backgammon.h"

typedef struct {
    GtkWidget *pwMoves;         /* the movelist */
    GtkWidget *pwRollout, *pwRolloutSettings, *pwAutoRollout;   /* rollout buttons */
    GtkWidget *pwEval, *pwEvalSettings; /* evaluation buttons */
    GtkWidget *pwMove;          /* move button */
    GtkWidget *pwCopy;          /* copy button */
    GtkWidget *pwEvalPly;       /* predefined eval buttons */
    GtkWidget *pwRolloutPresets;        /* predefined Rollout buttons */
    GtkWidget *pwShow;          /* button for showing moves */
    GtkWidget *pwMWC;           /* note: pwDetails is extern variable, see below*/           
    GtkWidget *pwTempMap;       /* button for showing temperature map */
    GtkWidget *pwCmark;         /* button for marking */
    GtkWidget *pwScoreMap;      /* button for showing move score map */
    GtkWidget *pwMoneyEval;      /* button for evaluating move independently of score */
    moverecord *pmr;
    movelist *pml;
    int fButtonsValid;
    int fDestroyOnMove;
    unsigned int *piHighlight;
    int fDetails;
    int hist;
    int evalAtMoney;        /* whether evalAtMoney is toggled */
} hintdata;

extern GtkWidget *CreateMoveList(moverecord * pmr,
                                 const int fButtonsValid, const int fDestroyOnMove, const int fDetails, int hist);

extern int
 CheckHintButtons(hintdata * phd);

extern void MoveListRefreshSize(void);
extern GtkWidget *pwDetails;
extern int showMoveListDetail;

#endif
