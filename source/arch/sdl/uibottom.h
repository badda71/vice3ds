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

// exposed function
extern void sdl_uibottom_draw(void);
extern int sdl_uibottom_mouseevent(SDL_Event *);
extern void toggle_keyboard(void);

// exposed definitions
typedef struct {
	int x,y,w,h,key,row,col,sticky,flags;
	char *name;
} uikbd_key;

enum bottom_action {
	UIB_NO = 0,
	UIB_KEYPRESS_SBUTTONS =		0x01,	// 0x01
	UIB_KEYPRESS_KEYBOARD =		0x02,	// 0x02
	UIB_KEYPRESS_ALL =			0x03,	// 
	UIB_REPAINT_KEYBOARD =		0x06,	// 0x04
	UIB_RECALC_KEYBOARD =		0x0C,	// 0x08
	UIB_REPAINT_SBUTTONS =		0x17,	// 0x10
	UIB_RECALC_SBUTTONS =		0x37,	// 0x20
	UIB_REPAINT_ALL =			UIB_REPAINT_KEYBOARD | UIB_REPAINT_SBUTTONS,
	UIB_ALL =					0x3f,

	UIB_GET_KEYPRESS_SBUTTONS =		0x01,
	UIB_GET_KEYPRESS_KEYBOARD =		0x02,
	UIB_GET_REPAINT_KEYBOARD =		0x04,
	UIB_GET_RECALC_KEYBOARD =		0x08,
	UIB_GET_REPAINT_SBUTTONS =		0x10,
	UIB_GET_RECALC_SBUTTONS =		0x20,
};

// exposed variables
extern uikbd_key uikbd_keypos[];
extern int uibottom_kbdactive;
extern enum bottom_action uibottom_must_redraw;
