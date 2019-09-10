/** \file   archdep_create_user_config_dir.c
 * \brief   Create user config dir if it doesn't exist already
 *
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
#include <errno.h>
#include <string.h>

#include "lib.h"
#include "log.h"
#include "archdep_atexit.h"
#include "archdep_home_path.h"
#include "archdep_join_paths.h"
#include "archdep_mkdir.h"
#include "archdep_xdg.h"

#ifdef ARCHDEP_OS_UNIX
# include <sys/stat.h>
# include <sys/types.h>
#endif

#include "archdep_create_user_config_dir.h"

#define MKDIRMOD 0644

void archdep_create_user_config_dir(void)
{
    char *cfg = archdep_xdg_data_home();

	if (archdep_mkdir(cfg, MKDIRMOD) == 0) {
        return;     /* we created the dir */
    } else if (errno != EEXIST) {
		log_error(LOG_ERR, "failed to create user config dir '%s': %d: %s.",
                cfg, errno, strerror(errno));
        archdep_vice_exit(1);
    }
}
