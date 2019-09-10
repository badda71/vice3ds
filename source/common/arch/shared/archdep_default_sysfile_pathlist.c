/** \file   archdep_default_sysfile_pathlist.c
 * \brief   Get a list of paths of required data files
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
#include <stddef.h>

#include "lib.h"
#include "log.h"
#include "util.h"

#include "archdep_boot_path.h"
#include "archdep_join_paths.h"
#include "archdep_user_config_path.h"


#include "archdep_default_sysfile_pathlist.h"

/** \brief  Total number of pathnames to store in the pathlist
 *
 * 16 seems to be enough, but it can always be increased to support more.
 */
#define TOTAL_PATHS 16


/** \brief  Reference to the sysfile pathlist
 *
 * This keeps a copy of the generated sysfile pathlist so we don't have to
 * generate it each time it is needed.
 */
static char *sysfile_path = NULL;

#define SYSFILE_PATH_MAXlEN 200

/** \brief  Generate a list of search paths for VICE system files
 *
 * \param[in]   emu_id  emulator ID (ie 'C64 or 'VSID')
 *
 * \return  heap-allocated string, to be freed by the caller
 */
char *archdep_default_sysfile_pathlist(const char *emu_id)
{
    
	sysfile_path=malloc(SYSFILE_PATH_MAXlEN);
	snprintf(sysfile_path,SYSFILE_PATH_MAXlEN,"/3ds/vice3ds/config/%s;/3ds/vice3ds/config;romfs:/config/%s;romfs:/config",emu_id,emu_id);

    /* sysfile.c appears to free() this (ie TODO: fix sysfile.c) */
    return lib_stralloc(sysfile_path);
}


/** \brief  Free the internal copy of the sysfile pathlist
 *
 * Call on emulator exit
 */

void archdep_default_sysfile_pathlist_free(void)
{
    if (sysfile_path != NULL) {
        lib_free(sysfile_path);
        sysfile_path = NULL;
    }
}
