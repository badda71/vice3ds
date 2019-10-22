/*
 * uibottom.h - 3DS bottom screen handling incl. virtual keyboard
 *
 * Written by
 *  Sebastian Weber <me@badda.de>
 *
 * This file is part of VICE3DS
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

#ifndef VICE_UIBOTTOM_H
#define VICE_UIBOTTOM_H

#include "uimenu.h"

// exposed function
extern void sdl_uibottom_draw(void);
extern ui_menu_action_t sdl_uibottom_mouseevent(SDL_Event *);
extern void toggle_keyboard(void);
extern void setBottomBacklight (int on);
extern int is_keyboard_hidden();
extern int uibottom_resources_init();
extern void uibottom_resources_shutdown();
extern int uibottom_editmode_is_on();
extern void uibottom_toggle_editmode();
extern void toggle_help(int inmenu);
extern void toggle_menu(int active, ui_menu_entry_t *item);

// exposed definitions
typedef struct {
	int x,y,w,h,key,shift,sticky,flags;
	char *name;
} uikbd_key;

enum bottom_action {
	UIB_NO = 0,
	UIB_REPAINT			=		0x01,	// 0x01
	UIB_RECALC_KEYPRESS =		0x03,	// 0x02
	UIB_RECALC_KEYBOARD =		0x07,	// 0x02
	UIB_RECALC_SBUTTONS =		0x0b,	// 0x04
	UIB_ALL =					0x0f,
	UIB_GET_REPAINT =			0x01,
	UIB_GET_RECALC_KEYPRESS =	0x02,
	UIB_GET_RECALC_KEYBOARD =	0x04,
	UIB_GET_RECALC_SBUTTONS =	0x08
};

enum str_alignment {
	ALIGN_LEFT,
	ALIGN_RIGHT,
	ALIGN_CENTER
};

enum font_size {
	FONT_SMALL,
	FONT_MEDIUM,
	FONT_BIG
};

// exposed variables
extern uikbd_key *uikbd_keypos;
extern volatile enum bottom_action uibottom_must_redraw;
extern int help_on;
extern int grab_sbuttons;
extern SDL_Event lastevent;

#endif
