/*
 * uibottom.c - 3DS bottom screen handling incl. virtual keyboard
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

#include "vice_sdl.h"
#include "uibottom.h"
#include "archdep_join_paths.h"
#include "archdep_user_config_path.h"
#include "lib.h"
#include "keyboard.h"
#include "mousedrv.h"
#include "log.h"
#include "videoarch.h"
#include "uistatusbar.h"
#include "SDL/SDL_image.h"

int uikbd_pos[4][4] = {
	{0,0,320,120},		// normal keys
	{0,120,320,120},	// shifted keys
	{0,240,320,120},	// cbm keys
	{0,360,320,120}		// ctrl keys
};

uikbd_key uikbd_keypos[] = {
	//  x,  y,   w,   h, key, row, col, sticky, name
	// F-Keys
	{ 201,  1,  30,  20, 133,   0,   4,   0, "F1"}, // F1 / F2
	{ 231,  1,  30,  20, 134,   0,   5,   0, "F3"}, // F3 / F4
	{ 261,  1,  30,  20, 135,   0,   6,   0, "F5"}, // F5 / F6
	{ 291,  1,  29,  20, 136,   0,   3,   0, "F7"}, // F7 / F8
	// top row	
	{   1, 21,  20,  20,  95,   7,   1,   0, "ArrowLeft"}, // <-
	{  21, 21,  20,  20,  49,   7,   0,   0, "1"}, // 1 / !
	{  41, 21,  20,  20,  50,   7,   3,   0, "2"}, // 2 / "
	{  61, 21,  20,  20,  51,   1,   0,   0, "3"}, // 3 / #
	{  81, 21,  20,  20,  52,   1,   3,   0, "4"}, // 4 / $
	{ 101, 21,  20,  20,  53,   2,   0,   0, "5"}, // 5 / %
	{ 121, 21,  20,  20,  54,   2,   3,   0, "6"}, // 6 / &
	{ 141, 21,  20,  20,  55,   3,   0,   0, "7"}, // 7 / '
	{ 161, 21,  20,  20,  56,   3,   3,   0, "8"}, // 8 / (
	{ 181, 21,  20,  20,  57,   4,   0,   0, "9"}, // 9 / )
	{ 201, 21,  20,  20,  48,   4,   3,   0, "0"}, // 0 / 0
	{ 221, 21,  20,  20,  43,   5,   0,   0, "+"}, // + / +
	{ 241, 21,  20,  20,  45,   5,   3,   0, "-"}, // - / |
	{ 261, 21,  20,  20,  92,   6,   0,   0, "Pound"}, // Pound / ..
	{ 281, 21,  20,  20,  19,   6,   3,   0, "CLR"}, // CLR/HOME
	{ 301, 21,  19,  20,  20,   0,   0,   0, "INST"}, // INST/DEL
	// 2nd row	
	{   1, 41,  30,  20,  24,   7,   2,   4, "CTRL"}, // CTRL - sticky ctrl
	{  31, 41,  20,  20, 113,   7,   6,   0, "Q"}, // Q
	{  51, 41,  20,  20, 119,   1,   1,   0, "W"}, // W
	{  71, 41,  20,  20, 101,   1,   6,   0, "E"}, // E
	{  91, 41,  20,  20, 114,   2,   1,   0, "R"}, // R
	{ 111, 41,  20,  20, 116,   2,   6,   0, "T"}, // T
	{ 131, 41,  20,  20, 121,   3,   1,   0, "Y"}, // Y
	{ 151, 41,  20,  20, 117,   3,   6,   0, "U"}, // U
	{ 171, 41,  20,  20, 105,   4,   1,   0, "I"}, // I
	{ 191, 41,  20,  20, 111,   4,   6,   0, "O"}, // O
	{ 211, 41,  20,  20, 112,   5,   1,   0, "P"}, // P
	{ 231, 41,  20,  20,  64,   5,   6,   0, "@"}, // @
	{ 251, 41,  20,  20,  42,   6,   1,   0, "*"}, // *
	{ 271, 41,  20,  20,  94,   6,   6,   0, "ArrowUp"}, // ^| / π
	{ 291, 41,  29,  20,  25,  -3,   0,   0, "RESTORE"}, // RESTORE
	// 3rd row	
	{   1, 61,  20,  20,   3,   7,   7,   0, "R/S"}, // RUN/STOP
	{  21, 61,  20,  20,  21,   1,   7,   1, "S/L"}, // SHIFT LOCK - sticky shift
	{  41, 61,  20,  20,  97,   1,   2,   0, "A"}, // A
	{  61, 61,  20,  20, 115,   1,   5,   0, "S"}, // S
	{  81, 61,  20,  20, 100,   2,   2,   0, "D"}, // D
	{ 101, 61,  20,  20, 102,   2,   5,   0, "F"}, // F
	{ 121, 61,  20,  20, 103,   3,   2,   0, "G"}, // G
	{ 141, 61,  20,  20, 104,   3,   5,   0, "H"}, // H
	{ 161, 61,  20,  20, 106,   4,   2,   0, "J"}, // J
	{ 181, 61,  20,  20, 107,   4,   5,   0, "K"}, // K
	{ 201, 61,  20,  20, 108,   5,   2,   0, "L"}, // L
	{ 221, 61,  20,  20,  58,   5,   5,   0, ":"}, // : / [
	{ 241, 61,  20,  20,  59,   6,   2,   0, ";"}, // ; / ]
	{ 261, 61,  20,  20,  61,   6,   5,   0, "="}, // =
	{ 281, 61,  39,  20,  13,   0,   1,   0, "CR"}, // RETURN
	// 4th row	
	{   1, 81,  20,  20,  23,   7,   5,   2, "C="}, // cbm - sticky cbm
	{  21, 81,  30,  20,  21,   1,   7,   1, "LSHIFT"}, // LSHIFT - sticky shift
	{  51, 81,  20,  20, 122,   1,   4,   0, "Z"}, // Z
	{  71, 81,  20,  20, 120,   2,   7,   0, "X"}, // X
	{  91, 81,  20,  20,  99,   2,   4,   0, "C"}, // C
	{ 111, 81,  20,  20, 118,   3,   7,   0, "V"}, // V
	{ 131, 81,  20,  20,  98,   3,   4,   0, "B"}, // B
	{ 151, 81,  20,  20, 110,   4,   7,   0, "N"}, // N
	{ 171, 81,  20,  20, 109,   4,   4,   0, "M"}, // M
	{ 191, 81,  20,  20,  44,   5,   7,   0, ","}, // ,
	{ 211, 81,  20,  20,  46,   5,   4,   0, "."}, // .
	{ 231, 81,  20,  20,  47,   6,   7,   0, "/"}, // /
	{ 251, 81,  30,  20,  21,   6,   4,   1, "RSHIFT"}, // RSHIFT - sticky shift
	// SPACE	
	{  61,101, 180,  19,  32,   7,   4,   0, "SPACE"},  // SPACE
	// Cursor Keys
	{ 281, 81,  20,  20,  30,   0,   7,   0, "C_UP"}, // DOWN
	{ 301,101,  19,  19,  29,   0,   2,   0, "C_RIGHT"}, // RIGHT
	{ 281,101,  20,  19,  17,   0,   2,   0, "C_DOWN"}, // UP
	{ 261,101,  20,  19,  31,   0,   2,   0, "C_LEFT"}, // LEFT
	// Finish
	{   0,  0,   0,   0,   0,   0,   0,   0, ""}
};

int uibottom_kbdactive = 1;
int uibottom_must_redraw = 0;

static SDL_Surface *vice_img=NULL;
static SDL_Surface *kbd_img=NULL;
static SDL_Rect bottom_r;
static int kb_x_pos = 0;
static int kb_y_pos = 0;
static int kb_activekey;
static int sticky=0;

static void negativeRect (SDL_Surface *s, int x, int y, int w, int h) {
	for (int yy = y; yy < y+h; yy++)
	{
		for (int xx = x; xx < x+w; xx++)
		{
			unsigned char *pixel = s->pixels + s->format->BytesPerPixel * (yy * s->w + xx);  
			pixel[1] = 255 - pixel[1];
			pixel[2] = 255 - pixel[2];
			pixel[3] = 255 - pixel[3];
		}
	}
	SDL_UpdateRect(s, x,y,w,h);
}

static void reverseKey(int i) {
	negativeRect(sdl_active_canvas->screen, uikbd_keypos[i].x+kb_x_pos, uikbd_keypos[i].y+kb_y_pos, uikbd_keypos[i].w, uikbd_keypos[i].h);
}

static void updateSticky() {
	for (int i = 0; uikbd_keypos[i].key!=0; ++i) {
		if (uikbd_keypos[i].flg & sticky)
			reverseKey(i);
	}
}

static void updateKeyboard(int rupdate) {
	SDL_Rect rdest = {
		.x = kb_x_pos,
		.y = kb_y_pos};
	// which keyboard for whick sticky keys?
	
	int kb = 
		sticky == 1 ? 1:
		sticky == 2 ? 2:
		sticky >= 4 ? 3:
		0;

	SDL_Rect rsrc = {
		.x = uikbd_pos[kb][0],
		.y = uikbd_pos[kb][1],
		.w = uikbd_pos[kb][2],
		.h = uikbd_pos[kb][3]};

	SDL_BlitSurface(kbd_img, &rsrc, sdl_active_canvas->screen, &rdest);
	updateSticky();
	if (rupdate) SDL_UpdateRect(sdl_active_canvas->screen, rdest.x, rdest.y, rsrc.w, rsrc.h);
}

void sdl_uibottom_draw(void)
{	
#ifndef UIBOTTOMOFF
	
	static void *olds = NULL;
	SDL_RWops *rwop;

	// init the 3DS bottom screen
	// keyboard image
	SDL_Surface *s=sdl_active_canvas->screen;
	if (olds != s || uibottom_must_redraw) {
		uistatusbar_must_redraw=1;
		olds=s;
		uibottom_must_redraw=0;
		bottom_r = (SDL_Rect){ .x = 0, .y = s->h/2, .w = s->w, .h=s->h/2};
		SDL_FillRect(s, &bottom_r, SDL_MapRGB(s->format, 0x60, 0x60, 0x60));

		// display background
		if (vice_img == NULL) {
			rwop=SDL_RWFromFile("romfs:/vice.png", "r");
			vice_img = IMG_LoadPNG_RW(rwop);
		}
		SDL_BlitSurface(vice_img, NULL, s,
			&(SDL_Rect){.x = (s->w-vice_img->w)/2, .y = 240});

		// display keyboard
		if (kbd_img == NULL) {
			rwop=SDL_RWFromFile("romfs:/keyboard.png", "r");
			kbd_img = IMG_LoadPNG_RW(rwop);
		}
		kb_x_pos = (s->w - uikbd_pos[0][2]) / 2;
		kb_y_pos = s->h - uikbd_pos[0][3];
		updateKeyboard(0);
		SDL_UpdateRect(s, bottom_r.x, bottom_r.y, bottom_r.w, bottom_r.h);
	}
	uistatusbar_draw();
	//SDL_Flip(s);
#endif
}

static SDL_Event sdl_e;
int sdl_uibottom_mouseevent(SDL_Event *e) {

	int f;
	int x = e->button.x - kb_x_pos;
	int y = e->button.y - kb_y_pos;

	if (uibottom_kbdactive) {
		int i;
		// check which button was pressed
		if (e->button.type != SDL_MOUSEBUTTONDOWN) i=kb_activekey;
		else {
			for (i = 0; uikbd_keypos[i].key != 0 ; ++i) {
				if (x >= uikbd_keypos[i].x &&
					x <  uikbd_keypos[i].x + uikbd_keypos[i].w &&
					y >= uikbd_keypos[i].y &&
					y <  uikbd_keypos[i].y + uikbd_keypos[i].h) break;
			}
			kb_activekey=i;
		}
		if (uikbd_keypos[i].key != 0) { // got a hit
			if ((f=uikbd_keypos[i].flg)>0) {	// sticky key!!
				if (e->button.type == SDL_MOUSEBUTTONDOWN) {
					sticky = sticky ^ uikbd_keypos[i].flg;
					sdl_e.type = sticky & uikbd_keypos[i].flg ? SDL_KEYDOWN : SDL_KEYUP;
					sdl_e.key.keysym.unicode = sdl_e.key.keysym.sym = uikbd_keypos[i].key;
					SDL_PushEvent(&sdl_e);
					updateKeyboard(1);
				}
			} else {
				sdl_e.type = e->button.type == SDL_MOUSEBUTTONDOWN ? SDL_KEYDOWN : SDL_KEYUP;
				sdl_e.key.keysym.unicode = sdl_e.key.keysym.sym = uikbd_keypos[i].key;
				SDL_PushEvent(&sdl_e);
				negativeRect(sdl_active_canvas->screen, uikbd_keypos[i].x+kb_x_pos, uikbd_keypos[i].y+kb_y_pos, uikbd_keypos[i].w, uikbd_keypos[i].h);
			}
		}
	}
	return 0;
}
