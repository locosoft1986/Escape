/**
 * $Id$
 * Copyright (C) 2008 - 2014 Nils Asmussen
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

#include <sys/common.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

static size_t envcount(void) {
	for(size_t i = 0; ; ++i) {
		if(environ[i] == NULL)
			return i;
	}
	A_UNREACHED;
}

ssize_t getenvito(char *name,size_t nameSize,size_t index) {
	if(index >= envcount())
		return -EINVAL;

	char *val = strchr(environ[index],'=');
	if(!val)
		return -EFAULT;
	*val = '\0';
	size_t len = strnzcpy(name,environ[index],nameSize);
	*val = '=';
	return len;
}
