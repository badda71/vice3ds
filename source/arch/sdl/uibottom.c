/*
 * 3dskbd.c - 3DS bottom screen handling incl. virtual keyboard
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
	{0,0,320,152},		// normal keys
	{0,152,320,152},	// shifted keys
	{0,304,320,152},	// cbm keys
	{0,456,320,152}		// ctrl keys
};

uikbd_key uikbd_keypos[] = {
	//  x,  y,   w,   h, key, row, col, sticky, name
	// F-Keys
	{ 103, 28,  29,  20, 133,   0,   4,   0, "F1"}, // F1 / F2
	{ 132, 28,  28,  20, 134,   0,   5,   0, "F3"}, // F3 / F4
	{ 160, 28,  28,  20, 135,   0,   6,   0, "F5"}, // F5 / F6
	{ 188, 28,  28,  20, 136,   0,   3,   0, "F7"}, // F7 / F8
	// top row	
	{   7, 49,  20,  20,  95,   7,   1,   0, "ArrowLeft"}, // <-
	{  27, 49,  19,  20,  49,   7,   0,   0, "1"}, // 1 / !
	{  46, 49,  19,  20,  50,   7,   3,   0, "2"}, // 2 / "
	{  65, 49,  19,  20,  51,   1,   0,   0, "3"}, // 3 / #
	{  84, 49,  19,  20,  52,   1,   3,   0, "4"}, // 4 / $
	{ 103, 49,  19,  20,  53,   2,   0,   0, "5"}, // 5 / %
	{ 122, 49,  19,  20,  54,   2,   3,   0, "6"}, // 6 / &
	{ 141, 49,  19,  20,  55,   3,   0,   0, "7"}, // 7 / '
	{ 160, 49,  19,  20,  56,   3,   3,   0, "8"}, // 8 / (
	{ 179, 49,  19,  20,  57,   4,   0,   0, "9"}, // 9 / )
	{ 198, 49,  19,  20,  48,   4,   3,   0, "0"}, // 0 / 0
	{ 217, 49,  19,  20,  43,   5,   0,   0, "+"}, // + / +
	{ 236, 49,  19,  20,  45,   5,   3,   0, "-"}, // - / |
	{ 255, 49,  19,  20,  92,   6,   0,   0, "Pound"}, // Pound / ..
	{ 274, 49,  19,  20,  19,   6,   3,   0, "CLR"}, // CLR/HOME
	{ 293, 49,  19,  20,  20,   0,   0,   0, "INST"}, // INST/DEL
	// 2nd row	
	{   7, 69,  30,  19,  24,   7,   2,   4, "CTRL"}, // CTRL - sticky ctrl
	{  37, 69,  19,  19, 113,   7,   6,   0, "Q"}, // Q
	{  56, 69,  19,  19, 119,   1,   1,   0, "W"}, // W
	{  75, 69,  19,  19, 101,   1,   6,   0, "E"}, // E
	{  94, 69,  19,  19, 114,   2,   1,   0, "R"}, // R
	{ 113, 69,  19,  19, 116,   2,   6,   0, "T"}, // T
	{ 132, 69,  19,  19, 121,   3,   1,   0, "Y"}, // Y
	{ 151, 69,  19,  19, 117,   3,   6,   0, "U"}, // U
	{ 170, 69,  19,  19, 105,   4,   1,   0, "I"}, // I
	{ 189, 69,  19,  19, 111,   4,   6,   0, "O"}, // O
	{ 208, 69,  19,  19, 112,   5,   1,   0, "P"}, // P
	{ 227, 69,  19,  19,  64,   5,   6,   0, "@"}, // @
	{ 246, 69,  19,  19,  42,   6,   1,   0, "*"}, // *
	{ 265, 69,  19,  19,  94,   6,   6,   0, "ArrowUp"}, // ^| / π
	{ 284, 69,  28,  19,  25,  -3,   0,   0, "RESTORE"}, // RESTORE
	// 3rd row	
	{   7, 88,  20,  19,   3,   7,   7,   0, "R/S"}, // RUN/STOP
	{  27, 88,  19,  19,  21,   1,   7,   1, "S/L"}, // SHIFT LOCK - sticky shift
	{  46, 88,  19,  19,  97,   1,   2,   0, "A"}, // A
	{  65, 88,  19,  19, 115,   1,   5,   0, "S"}, // S
	{  84, 88,  19,  19, 100,   2,   2,   0, "D"}, // D
	{ 103, 88,  19,  19, 102,   2,   5,   0, "F"}, // F
	{ 122, 88,  19,  19, 103,   3,   2,   0, "G"}, // G
	{ 141, 88,  19,  19, 104,   3,   5,   0, "H"}, // H
	{ 160, 88,  19,  19, 106,   4,   2,   0, "J"}, // J
	{ 179, 88,  19,  19, 107,   4,   5,   0, "K"}, // K
	{ 198, 88,  19,  19, 108,   5,   2,   0, "L"}, // L
	{ 217, 88,  19,  19,  58,   5,   5,   0, ":"}, // : / [
	{ 236, 88,  19,  19,  59,   6,   2,   0, ";"}, // ; / ]
	{ 255, 88,  19,  19,  61,   6,   5,   0, "="}, // =
	{ 274, 88,  38,  19,  13,   0,   1,   0, "CR"}, // RETURN
	// 4th row	
	{   7,107,  20,  19,  23,   7,   5,   2, "C="}, // cbm - sticky cbm
	{  27,107,  29,  19,  21,   1,   7,   1, "LSHIFT"}, // LSHIFT - sticky shift
	{  56,107,  19,  19, 122,   1,   4,   0, "Z"}, // Z
	{  75,107,  19,  19, 120,   2,   7,   0, "X"}, // X
	{  94,107,  19,  19,  99,   2,   4,   0, "C"}, // C
	{ 113,107,  19,  19, 118,   3,   7,   0, "V"}, // V
	{ 132,107,  19,  19,  98,   3,   4,   0, "B"}, // B
	{ 151,107,  19,  19, 110,   4,   7,   0, "N"}, // N
	{ 170,107,  19,  19, 109,   4,   4,   0, "M"}, // M
	{ 189,107,  19,  19,  44,   5,   7,   0, ","}, // ,
	{ 208,107,  19,  19,  46,   5,   4,   0, "."}, // .
	{ 227,107,  19,  19,  47,   6,   7,   0, "/"}, // /
	{ 246,107,  28,  19,  21,   6,   4,   1, "RSHIFT"}, // RSHIFT - sticky shift
	{ 274,107,  19,  19,  17,   0,   7,   0, "C_DOWN"}, // UP / DOWN
	{ 293,107,  19,  19,  29,   0,   2,   0, "C_RIGHT"}, // LEFT / RIGHT
	// SPACE	
	{  65,126, 191,  19,  32,   7,   4,   0, "SPACE"},  // SPACE
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
					sdl_e.type = sticky & uikbd_keypos[i].flg ? SDL_KEYDOWN : SDL_KEYUP,
					sdl_e.key.keysym.unicode = sdl_e.key.keysym.sym = uikbd_keypos[i].key;
					SDL_PushEvent(&sdl_e);
					updateKeyboard(1);
				}
			} else {
				sdl_e.type = e->button.type == SDL_MOUSEBUTTONDOWN ? SDL_KEYDOWN : SDL_KEYUP,
				sdl_e.key.keysym.unicode = sdl_e.key.keysym.sym = uikbd_keypos[i].key;
				SDL_PushEvent(&sdl_e);
				negativeRect(sdl_active_canvas->screen, uikbd_keypos[i].x+kb_x_pos, uikbd_keypos[i].y+kb_y_pos, uikbd_keypos[i].w, uikbd_keypos[i].h);
			}
		}
	}
	return 0;
}
