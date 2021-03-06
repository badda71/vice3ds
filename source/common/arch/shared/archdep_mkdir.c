/** \file   archdep_mkdir.c
 * \brief   Create a directory
 * \author  Bas Wassink <b.wassink@ziggo.nl>
 */

/*
 * This file is part of VICE, the Versatile Commodore Emulator.
 * See README for copyright notice.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
 *  02111-1307  USA.
 *
 */

#include "vice.h"
#include "archdep_defs.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#ifdef AMIGA_SUPPORT
/* includes? */
#endif

#if defined(BEOS_COMPILE) || defined(UNIX_COMPILE)
# include <unistd.h>
# include <sys/stat.h>
# include <sys/types.h>
#endif

#ifdef WIN32_COMPILE
# include <direct.h>
#endif

#include "archdep_atexit.h"
#include "log.h"
#include "lib.h"

#include "archdep_mkdir.h"


/** \brief  Recursively create a directory \a pathname with \a mode
 *
 * \param[in]   pathname    directory to create
 * \param[in]   mode        access mode of directory (ignored on some systems)
 *
 * \return  0 on success, -1 on error
 */
int archdep_mkdir(const char *pathname, int mode)
{
    char *c=lib_stralloc(pathname);
	for (char* p = strchr(c + 1, '/'); p; p = strchr(p + 1, '/')) {
        *p = '\0';
        if (mkdir(c, mode) == -1) {
            if (errno != EEXIST) {
                *p = '/';
                lib_free(c);
				return -1;
            }
        }
        *p = '/';
    }
	lib_free(c);
	return mkdir(pathname, mode);
}
