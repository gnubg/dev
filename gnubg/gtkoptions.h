/*
 * Copyright (C) 2003 Joern Thyssen <jth@gnubg.org>
 * Copyright (C) 2003-2007 the AUTHORS
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
 * $Id: gtkoptions.h,v 1.9 2023/03/19 21:04:25 plm Exp $
 */

#ifndef GTKOPTIONS_H
#define GTKOPTIONS_H

extern void GTKSetOptions(void);
extern void GTKSound(void);
extern void GTKCommandEditKeyNames(GtkWidget * UNUSED(pw), GtkWidget * pwParent);

typedef enum {
    IDLE, BELOW_NORMAL, NORMAL, ABOVE_NORMAL, HIGH, REALTIME, NUM_PRIORITY
} priority;
extern priority DefaultPriority;
extern const char* aszPriority[NUM_PRIORITY];
extern const char* aszPriorityCommands[NUM_PRIORITY];

#endif                          /* GTKOPTIONS_H */
