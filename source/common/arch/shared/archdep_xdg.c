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

static char *xdg_data_home = "/3ds/vice3ds";
static char *xdg_config_home = "/3ds/vice3ds/config";
static char *xdg_cache_home = "/3ds/vice3ds/cache";

char *archdep_xdg_data_home(void)
{
	return xdg_data_home;
}

char *archdep_xdg_config_home(void)
{
	return xdg_config_home;
}

char *archdep_xdg_cache_home(void)
{
	return xdg_cache_home;
}
