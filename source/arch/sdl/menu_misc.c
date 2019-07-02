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
#include "lib.h"
#include "charset.h"
#include "kbdbuf.h"
#include "uibottom.h"
#include "vice3ds.h"
#include "resources.h"
#include "fullscreenarch.h"
#include "videoarch.h"
#include "uipoll.h"
#include "kbd.h"
#include "ui.h"
#include <3ds.h>

static int ui_key_press_sequence (void *param) {
	int (*keyseq)[3]=param;
	SDL_Event sdl_e;
	for (int i=0;keyseq[i][2]!=0;++i) {
		SDL_Delay(keyseq[i][0]);
		sdl_e.type = keyseq[i][1];
		sdl_e.key.keysym.unicode = sdl_e.key.keysym.sym = keyseq[i][2];
		SDL_PushEvent(&sdl_e);
	}
	SDL_Delay(50);
	return 0;
}

static void kb_feed(char *text) {
	char *text_in_petscii = lib_stralloc(text);
	charset_petconvstring((unsigned char*)text_in_petscii, 0);
	kbdbuf_feed(text_in_petscii);
	lib_free(text_in_petscii);
}

static UI_MENU_CALLBACK(keystroke_callback)
{
	if (activated) {
		start_worker(ui_key_press_sequence, (int(*)[3])param);
        return sdl_menu_text_exit_ui;
    }
    return NULL;
}

static UI_MENU_CALLBACK(command_callback)
{
	if (activated) {
		kb_feed((char *)param);
		return sdl_menu_text_exit_ui;
	}
	return NULL;
}

static UI_MENU_CALLBACK(bottomoff_callback)
{
    if (activated) {
		setBottomBacklight(0);
		return sdl_menu_text_exit_ui;
    }
    return NULL;
}

static UI_MENU_CALLBACK(toggle_MaxScreen_callback)
{
	static int s_fs=-1,s_fss=-1,s_fsm=-1;
	int r=0, fs=0, fss=0, w=0, h=0, fsm=0;
	
	char c_fs[25],c_fsm[25];
	snprintf(c_fs,25,"%sFullscreen",sdl_active_canvas->videoconfig->chip_name);
	resources_get_int(c_fs, &fs); // 1?
	snprintf(c_fsm,25,"%sSDLFullscreenMode",sdl_active_canvas->videoconfig->chip_name);
	resources_get_int(c_fsm, &fsm); // FULLSCREEN_MODE_CUSTOM ?
	resources_get_int("SDLCustomWidth", &w); // 320?
	resources_get_int("SDLCustomHeight", &h); // 200?
	resources_get_int("SDLFullscreenStretch", &fss); // SDL_FULLSCREEN?

	if (fs==1 && w==320 && h==200 && fss==SDL_FULLSCREEN && fsm==FULLSCREEN_MODE_CUSTOM) r=1;

	if (activated) {
		if (r==0) {
			r=1;
			s_fs=fs; s_fss=fss; s_fsm=fsm;
			resources_set_int("SDLCustomWidth", 320);
			resources_set_int("SDLCustomHeight", 200);
			resources_set_int("SDLFullscreenStretch", SDL_FULLSCREEN);
			resources_set_int(c_fsm, FULLSCREEN_MODE_CUSTOM);
			resources_set_int(c_fs, 1);
		} else {
			r=0;
			resources_set_int(c_fs, s_fs);
			resources_set_int(c_fsm, s_fsm);
			resources_set_int("SDLFullscreenStretch", s_fss);
			resources_set_int("SDLCustomWidth", 400);
			resources_set_int("SDLCustomHeight", 240);
		}	
		// update keypresses on bottom screen in case we have anything mapped
		uibottom_must_redraw |= UIB_REPAINT_SBUTTONS;
	}
	return r ? sdl_menu_text_tick : NULL;
}

static UI_MENU_CALLBACK(add_keymapping_callback)
{
	SDL_Event e;
	if (activated) {
		SDL_Event s = sdl_ui_poll_event("key", "extra key mapping", SDL_POLL_KEYBOARD, 5);
        if (s.type == SDL_KEYDOWN) {
			while (SDL_PollEvent(&e)); // clear event queue
			SDL_Event t = sdl_ui_poll_event("mapping target", get_3ds_keyname(s.key.keysym.sym),
				(int)param, 5);
			set_3ds_mapping(s.key.keysym.sym, t.type ? &t : NULL); // set or clear mapping
		}
	}
	return NULL;
}

static UI_MENU_CALLBACK(list_keymappings_callback)
{
	int i,count=0;
	char *buf;
	if (activated) {
		for (i=1;i<255;i++)
			if (keymap3ds[i]!=0) count++;

		buf=malloc(40*(count+3));
		buf[0]=0;

		sprintf(buf+strlen(buf),
			"Extra key mappings\n"
			"~~~~~~~~~~~~~~~~~~\n"
			"\n");

		if (count == 0) {
			sprintf(buf+strlen(buf),"-- NONE --");
		} else {
			for (int i=1;i<254;i++) {
				if (keymap3ds[i] == 0) continue;
				sprintf(buf+strlen(buf),"%-12s-> %s",get_3ds_keyname(i), get_3ds_mapping_name(i));
			}
		}
		ui_show_text(buf);
		free(buf);
	}
	return NULL;
}

static UI_MENU_CALLBACK(toggle_hidekeyboard_callback)
{
	int r=is_keyboard_hidden();
	if (activated) {
		toggle_keyboard();
		r=!r;
	}
	return r ? sdl_menu_text_tick : NULL;
}

const ui_menu_entry_t misc_menu[] = {
    SDL_MENU_ITEM_TITLE("3DS specific"),
    { "Power off bottom screen backlight",
		MENU_ENTRY_OTHER,
		bottomoff_callback,
		NULL },
	{ "Hide keyboard",
		MENU_ENTRY_OTHER_TOGGLE,
		toggle_hidekeyboard_callback,
		NULL},
	{ "Hide Border / Fullscreen",
		MENU_ENTRY_OTHER_TOGGLE,
		toggle_MaxScreen_callback,
		NULL},
	SDL_MENU_ITEM_SEPARATOR,
    SDL_MENU_ITEM_TITLE("Extra key mappings"),
    { "Add extra key mapping (key -> key)",
		MENU_ENTRY_OTHER,
		add_keymapping_callback,
		(ui_callback_data_t)(SDL_POLL_KEYBOARD | SDL_POLL_MODIFIER)},
    { "Add extra key mapping (key -> joystick)",
		MENU_ENTRY_OTHER,
		add_keymapping_callback,
		(ui_callback_data_t)SDL_POLL_JOYSTICK },
    { "List extra key mappings",
		MENU_ENTRY_OTHER,
		list_keymappings_callback,
		NULL },
	SDL_MENU_ITEM_SEPARATOR,
    SDL_MENU_ITEM_TITLE("Commands"),
    { "LOAD\"*\",8,1 RUN",
		MENU_ENTRY_OTHER,
		command_callback,
		(ui_callback_data_t)"lO\"*\",8,1\rrun\r" },
    { "LOAD\"$\",8 LIST",
		MENU_ENTRY_OTHER,
		command_callback,
		(ui_callback_data_t)"lO\"$\",8\rlist\r" },
    SDL_MENU_ITEM_SEPARATOR,
    SDL_MENU_ITEM_TITLE("Keystrokes"),
	{ "RUN/STOP + RESTORE",
		MENU_ENTRY_OTHER,
		keystroke_callback,
		(ui_callback_data_t)((int[][3]){
		{0,   SDL_KEYDOWN,  3}, //R/S
		{20,  SDL_KEYDOWN, 25}, //Restore
		{100, SDL_KEYUP,   25},
		{0,   SDL_KEYUP,    3},
		{0,   0,            0}})},
	{ "RUN/STOP",
		MENU_ENTRY_OTHER,
		keystroke_callback,
		(ui_callback_data_t)((int[][3]){
		{0,   SDL_KEYDOWN,  3}, //R/S
		{50, SDL_KEYUP,    3},
		{0,   0,            0}})},
	{ "SPACE",
		MENU_ENTRY_OTHER,
		keystroke_callback,
		(ui_callback_data_t)((int[][3]){
		{0,   SDL_KEYDOWN,  32},
		{50, SDL_KEYUP,    32},
		{0,   0,            0}})},
	{ "RETURN",
		MENU_ENTRY_OTHER,
		keystroke_callback,
		(ui_callback_data_t)((int[][3]){
		{0,   SDL_KEYDOWN,  13},
		{50, SDL_KEYUP,    13},
		{0,   0,            0}})},
	{ "Y",
		MENU_ENTRY_OTHER,
		keystroke_callback,
		(ui_callback_data_t)((int[][3]){
		{0,   SDL_KEYDOWN, 121},
		{50, SDL_KEYUP,   121},
		{0,   0,            0}})},
	{ "N",
		MENU_ENTRY_OTHER,
		keystroke_callback,
		(ui_callback_data_t)((int[][3]){
		{0,   SDL_KEYDOWN, 121},
		{50, SDL_KEYUP,   121},
		{0,   0,            0}})},
	{ "0",
		MENU_ENTRY_OTHER,
		keystroke_callback,
		(ui_callback_data_t)((int[][3]){
		{0,   SDL_KEYDOWN,  48},
		{50, SDL_KEYUP,    48},
		{0,   0,            0}})},
	SDL_MENU_LIST_END
};
 