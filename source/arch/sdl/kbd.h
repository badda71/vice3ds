/*
 * kbd.h - SDL specfic keyboard driver.
 *
 * Written by
 *  Hannu Nuotio <hannu.nuotio@tut.fi>
 *  Marco van den Heuvel <blackystardust68@yahoo.com>
 *
 * This file is part of VICE, the Versatile Commodore Emulator.
 * See README file for copyright notice.
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

#ifndef VICE_KBD_H
#define VICE_KBD_H

#include "vice_sdl.h"
#include "uimenu.h"

extern char *hotkey_file;
extern void kbd_arch_init(void);
extern int kbd_arch_get_host_mapping(void);

extern signed long kbd_arch_keyname_to_keynum(char *keyname);
extern const char *kbd_arch_keynum_to_keyname(signed long keynum);
extern void kbd_initialize_numpad_joykeys(int *joykeys);

#define KBD_PORT_PREFIX "sdl"

#define SDL_NUM_SCANCODES   512 /* this must be the same value as in SDL2 headers */

// for 3ds - see 3ds button mapping in archdep.c
#define VICE_SDLK_RIGHT     213 //d-pad right
#define VICE_SDLK_LEFT      212 //d-pad left
#define VICE_SDLK_HOME      210 //d-pad up
#define VICE_SDLK_END       211 //d-pad down
#define VICE_SDLK_F10       291

#define VICE_SDLK_BACKSPACE   20 //OSK delete
#define VICE_SDLK_ESCAPE      201 //b-button
#define VICE_SDLK_RETURN      13 //OSK return

extern SDLKey SDL2x_to_SDL1x_Keys(SDLKey key);
extern SDLKey SDL1x_to_SDL2x_Keys(SDLKey key);

extern ui_menu_action_t sdlkbd_press(SDLKey key, SDLMod mod);
extern ui_menu_action_t sdlkbd_release(SDLKey key, SDLMod mod);

extern void sdlkbd_set_hotkey(SDLKey key, SDLMod mod, ui_menu_entry_t *value);

extern int sdlkbd_hotkeys_load(char *file);
extern int sdlkbd_hotkeys_dump(FILE *fp);

extern int sdlkbd_init_resources(void);
extern void sdlkbd_resources_shutdown(void);

extern int sdlkbd_init_cmdline(void);

extern void kbd_enter_leave(void);
extern void kbd_focus_change(void);

extern int sdl_ui_menukeys[];

extern const char *kbd_get_menu_keyname(void);
extern const char *get_3ds_keyname(int);

extern ui_menu_entry_t *sdlkbd_ui_hotkeys[];

#endif
