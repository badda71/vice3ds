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
#include "3ds_threadworker.h"
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

static UI_MENU_CALLBACK(keystroke_callback_noexit)
{
	if (activated) {
		start_worker(ui_key_press_sequence, (int(*)[3])param);
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

const ui_menu_entry_t misc_menu[] = {
    SDL_MENU_ITEM_TITLE("3DS specific"),
    { "Power off bottom screen backlight",
		MENU_ENTRY_OTHER,
		bottomoff_callback,
		NULL },
	{ "Toggle hide keyboard",
		MENU_ENTRY_OTHER,
		keystroke_callback_noexit,
		(ui_callback_data_t)((int[][3]){
		{0,  SDL_KEYDOWN,  255},
		{50, SDL_KEYUP,    255},
		{0,   0,            0}})},
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
 