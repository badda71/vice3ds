/*
 * menu_misc.h - Implementation of the misc stuff menu for the SDL UI.
 *
 * Written by
 *  Sebastian Weber <me@badda.de>
 *
 * This file is part of VICE3DS
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
#include "menu_common.h"
#include "log.h"

static int ui_key_press_sequence (void *param) {
	int (*keyseq)[3]=param;
	SDL_Event sdl_e;
	for (int i=0;keyseq[i][2]!=0;++i) {
		SDL_Delay(keyseq[i][0]);
		sdl_e.type = keyseq[i][1];
		sdl_e.key.keysym.unicode = sdl_e.key.keysym.sym = keyseq[i][2];
		SDL_PushEvent(&sdl_e);
	}
	return 0;
}

int keyseq_rsrestore[][3]={
	{0,   SDL_KEYDOWN,  3}, //R/S
	{20,  SDL_KEYDOWN, 25}, //Restore
	{100, SDL_KEYUP,   25},
	{0,   SDL_KEYUP,    3},
	{0,   0,            0}
};

int keyseq_load[][3]={
	{  0, SDL_KEYDOWN, 108}, //L
	{ 60, SDL_KEYUP  , 108},
	{ 10, SDL_KEYDOWN, 111}, //O
	{ 60, SDL_KEYUP  , 111},
	{ 10, SDL_KEYDOWN,  97}, //A
	{ 60, SDL_KEYUP  ,  97},
	{ 10, SDL_KEYDOWN, 100}, //D
	{ 60, SDL_KEYUP  , 100},
	{ 10, SDL_KEYDOWN,  21}, //SHIFT
	{ 10, SDL_KEYDOWN,  50}, //2
	{ 60, SDL_KEYUP  ,  50},
	{ 10, SDL_KEYUP  ,  21},
	{ 10, SDL_KEYDOWN,  42}, //*
	{ 60, SDL_KEYUP  ,  42},
	{ 10, SDL_KEYDOWN,  21}, //SHIFT
	{ 10, SDL_KEYDOWN,  50}, //2
	{ 60, SDL_KEYUP  ,  50},
	{ 10, SDL_KEYUP  ,  21},
	{ 10, SDL_KEYDOWN,  44}, //,
	{ 60, SDL_KEYUP  ,  44},
	{ 10, SDL_KEYDOWN,  56}, //8
	{ 60, SDL_KEYUP  ,  56},
	{ 10, SDL_KEYDOWN,  44}, //,
	{ 60, SDL_KEYUP  ,  44},
	{ 10, SDL_KEYDOWN,  49}, //1
	{ 60, SDL_KEYUP  ,  49},
	{ 10, SDL_KEYDOWN,  13}, //CR
	{ 60, SDL_KEYUP  ,  13},
	{100, SDL_KEYDOWN, 114}, //R
	{ 60, SDL_KEYUP  , 114},
	{ 10, SDL_KEYDOWN, 117}, //U
	{ 60, SDL_KEYUP  , 117},
	{ 10, SDL_KEYDOWN, 110}, //N
	{ 60, SDL_KEYUP  , 110},
	{ 10, SDL_KEYDOWN,  13}, //CR
	{ 60, SDL_KEYUP  ,  13},
	{  0,           0,   0}
};

static UI_MENU_CALLBACK(press_runstop_restore_callback)
{
    if (activated) {
		SDL_CreateThread(ui_key_press_sequence, keyseq_rsrestore);
        return sdl_menu_text_exit_ui;
    }
    return NULL;
}

static UI_MENU_CALLBACK(type_load_callback)
{
    if (activated) {
		SDL_CreateThread(ui_key_press_sequence, keyseq_load);
        return sdl_menu_text_exit_ui;
    }
    return NULL;
}


const ui_menu_entry_t misc_menu[] = {
    { "RUN/STOP + RESTORE",
      MENU_ENTRY_OTHER,
      press_runstop_restore_callback,
      NULL },
    { "LOAD\"*\",8,1 RUN",
      MENU_ENTRY_OTHER,
      type_load_callback,
      NULL },
    SDL_MENU_LIST_END
};
 