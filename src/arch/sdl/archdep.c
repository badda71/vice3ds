/*
 * archdep.c - Miscellaneous system-specific stuff.
 *
 * Written by
 *  Marco van den Heuvel <blackystardust68@yahoo.com>
 *
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

#include "vice_sdl.h"
#include <stdio.h>

/* These functions are defined in the files included below and
   used lower down. */
static int archdep_init_extra(int *argc, char **argv);
static void archdep_shutdown_extra(void);

#ifdef AMIGA_SUPPORT
#include "archdep_amiga.c"
#endif

#ifdef BEOS_COMPILE
#include "archdep_beos.c"
#endif

#ifdef __OS2__
#include "archdep_os2.c"
#endif

#ifdef UNIX_COMPILE
#include "archdep_unix-c.h"
#endif

#ifdef WIN32_COMPILE
#include "archdep_win32.c"
#endif

#include "kbd.h"

#ifndef SDL_REALINIT
#define SDL_REALINIT SDL_Init
#endif

/*
 * XXX: this will get fixed once the code in this file is moved into
 *      src/arch/shared
 */
#include "../shared/archdep_atexit.h"
#include "../shared/archdep_create_user_config_dir.h"
#include <3ds.h>


int archdep_init(int *argc, char **argv)
{
	
	Result rc = romfsInit();
	if (rc) {
		log_error(LOG_ERR, "failed to init romfs: %08lX\n", rc);
        archdep_vice_exit(1);
	}

	archdep_program_path_set_argv0(argv[0]);

    archdep_create_user_config_dir();

    if (SDL_REALINIT(SDL_INIT_VIDEO | SDL_INIT_TIMER) < 0) {
        log_debug("SDL error: %s\n", SDL_GetError());
        return 1;
    }

	// 3ds buttons must be remapped - the standard conflicts with the keyboard
	// key codes over 200 are unused
	SDL_N3DSKeyBind(KEY_A, 200);
	SDL_N3DSKeyBind(KEY_B, 201);
	SDL_N3DSKeyBind(KEY_X, 202);
	SDL_N3DSKeyBind(KEY_Y, 203);
	SDL_N3DSKeyBind(KEY_L, 204);
	SDL_N3DSKeyBind(KEY_R, 205);
	SDL_N3DSKeyBind(KEY_ZL, 206);
	SDL_N3DSKeyBind(KEY_ZR, 207);
	SDL_N3DSKeyBind(KEY_START, 208);
	SDL_N3DSKeyBind(KEY_SELECT, 209);
	SDL_N3DSKeyBind(KEY_UP, 210);
	SDL_N3DSKeyBind(KEY_DOWN, 211);
	SDL_N3DSKeyBind(KEY_LEFT, 212);
	SDL_N3DSKeyBind(KEY_RIGHT, 213);
	SDL_N3DSKeyBind(KEY_CSTICK_UP, 214);
	SDL_N3DSKeyBind(KEY_CSTICK_DOWN, 215);
	SDL_N3DSKeyBind(KEY_CSTICK_LEFT, 216);
	SDL_N3DSKeyBind(KEY_CSTICK_RIGHT, 217);
	SDL_N3DSKeyBind(KEY_CPAD_UP, 218);
	SDL_N3DSKeyBind(KEY_CPAD_DOWN, 219);
	SDL_N3DSKeyBind(KEY_CPAD_LEFT, 220);
	SDL_N3DSKeyBind(KEY_CPAD_RIGHT, 221);

    /*
     * Call SDL_Quit() via atexit() to avoid segfaults on exit.
     * See: https://wiki.libsdl.org/SDL_Quit
     * I'm not sure this actually registers SDL_Quit() as the last atexit()
     * call, but it appears to work at least (BW)
     */
    if (archdep_vice_atexit(SDL_Quit) != 0) {
        log_error(LOG_ERR,
                "failed to register SDL_Quit() with archdep_vice_atexit().");
        archdep_vice_exit(1);
    }

    if (archdep_vice_atexit((void *)romfsExit) != 0) {
        log_error(LOG_ERR,
                "failed to register romfsExit() with archdep_vice_atexit().");
        archdep_vice_exit(1);
    }

    return archdep_init_extra(argc, argv);
}


void archdep_shutdown(void)
{
    archdep_program_name_free();
    archdep_program_path_free();
    archdep_boot_path_free();
    archdep_home_path_free();
    archdep_default_sysfile_pathlist_free();

#ifdef HAVE_NETWORK
    archdep_network_shutdown();
#endif
    archdep_shutdown_extra();
    archdep_extra_title_text_free();
}
