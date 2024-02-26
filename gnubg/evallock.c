/*
 * Copyright (C) 2009 Michael Petch <mpetch@gnubg.org>
 * Copyright (C) 2009 Christian Anthon <anthon@kiku.dk>
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
 * $Id: evallock.c,v 1.4 2020/01/01 14:08:24 plm Exp $
 */

#include "config.h"
#if USE_MULTITHREAD
#define LOCKING_VERSION 1

#include "eval.c"
#include "rollout.c"
#endif
