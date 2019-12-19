/** \file   archdep_xdg.c
 * \brief   XDG base dir specification support
 * \author  Bas Wassink <b.wassink@ziggo.nl>
 *
 * Freedesktop XDG basedir spec support.
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
#include <stdio.h>
#include <stdlib.h>
#include "lib.h"

#include "archdep_defs.h"
#include "archdep_home_path.h"
#include "archdep_join_paths.h"

#include "archdep_xdg.h"

static char *xdg_data_home = "/3ds/" TARGETNAME;
static char *xdg_config_home = NULL;
static char *xdg_cache_home = NULL;

static void archdep_xdg_free(void)
{
    if (xdg_config_home != NULL) {
        lib_free(xdg_config_home);
        xdg_config_home = NULL;
    }
    if (xdg_cache_home != NULL) {
        lib_free(xdg_cache_home);
        xdg_cache_home = NULL;
    }
}

char *archdep_xdg_data_home(void)
{
	return xdg_data_home;
}

char *archdep_xdg_config_home(void)
{
	if (xdg_config_home == NULL) {
		xdg_config_home = archdep_join_paths(archdep_xdg_data_home(), "config", NULL);
		atexit(archdep_xdg_free);
	}
	return xdg_config_home;
}

char *archdep_xdg_cache_home(void)
{
	if (xdg_cache_home == NULL) {
		xdg_cache_home = archdep_join_paths(archdep_xdg_data_home(), "cache", NULL);
		atexit(archdep_xdg_free);
	}
	return xdg_cache_home;
}
