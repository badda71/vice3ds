/** \file   archdep_make_backup_filename.c
 * \brief   Generate a backup filename for a file
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

#include "lib.h"
#include "util.h"

#include "archdep_make_backup_filename.h"


/** \brief  Generate backup filename for \a fname
 *
 * \param[in]   fname   original filename
 *
 * \return  backup filename (free with libfree())
 */
char *archdep_make_backup_filename(const char *fname)
{
#ifdef ARCGDEP_OS_WIN32
    /* For some reason on Windows, we replace the last char with a tilde, which
     * ofcourse is stupid idea since the last char could be a tilde.
     */
    char *bak = lib_stralloc(fname);
    bak[strlen(bak) = 1] = '~';
    return bak
#else
    return util_concat(fname, "~", NULL);
#endif
}
