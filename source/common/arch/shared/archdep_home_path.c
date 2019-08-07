/** \file   archdep_home_path.c
 * \brief   Retrieve home directory of current user
 * \author  Bas Wassink <b.wassink@ziggo.nl>
 *
 * Retrieve the home directory of the current user on systems that have a
 * concept of a home directory. On systems that don't have a home dir '.' is
 * returned (PROGDIR: in the case of AmigaOS).
 * Of course on systems that don't have a home dir, this function simply
 * shouldn't be used, archdep_boot_path() might be a better function.
 *
 * OS support:
 *  - Linux
 *  - Windows
 *  - MacOS
 *  - BeOS/Haiku (always returns '/boot/home')
 *  - AmigaOS (untested, always returns 'PROGDIR:')
 *  - OS/2 (untested, always returns '.')
 *  - MS-DOS (untested, always returns '.')
 *
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

#include "lib.h"

#include "archdep_defs.h"

#include "archdep_home_path.h"


/** \brief  home directory reference
 *
 * Allocated once in the first call to archdep_home_path(), should be freed
 * on emulator exit with archdep_home_path_free()
 */
static char *home_dir = NULL;


/** \brief  Get user's home directory
 *
 * Free memory used on emulator exit with archdep_home_path_free()
 *
 * \return  user's home directory
 */
const char *archdep_home_path(void)
{
    if (home_dir == NULL)
	    home_dir = lib_stralloc("/");

	return home_dir;
}


/** \brief  Free memory used by the home path
 */
void archdep_home_path_free(void)
{
    if (home_dir != NULL) {
        lib_free(home_dir);
        home_dir = NULL;
    }
}

