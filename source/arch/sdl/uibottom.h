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

// exposed definitions
typedef struct {
	int x,y,w,h,key,row,col,sticky,flags;
	char *name;
} uikbd_key;

enum bottom_action {
	UIB_NO = 0,
	UIB_KEYPRESS_SBUTTONS = 1,
	UIB_KEYPRESS_KEYBOARD = 2,
	UIB_KEYPRESS = 3,
	UIB_SBUTTONS = 4,
	UIB_KEYBOARD = 8,
	UIB_ALL = 15,
};

// exposed variables
extern uikbd_key uikbd_keypos[];
extern int uibottom_kbdactive;
extern enum bottom_action uibottom_must_redraw;
