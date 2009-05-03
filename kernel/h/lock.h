/**
 * $Id$
 * Copyright (C) 2008 - 2009 Nils Asmussen
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#ifndef LOCK_H_
#define LOCK_H_

#include "common.h"
#include "proc.h"

/**
 * Aquires the lock with given process and ident. If it exists and is locked,
 * the process waits here until it's unlocked
 *
 * @param pid the process-id
 * @param ident to identify the lock
 * @return 0 on success
 */
s32 lock_aquire(tPid pid,u32 ident);

/**
 * Releases the lock with given ident
 *
 * @param ident to identify the lock
 * @return 0 on success
 */
s32 lock_release(u32 ident);

#endif /* LOCK_H_ */
