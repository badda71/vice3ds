/*
 * 3dskbd.c - 3DS virtual keyboard
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
 { 283, 32,  25,  16, 133,   0,   4,   0, "F1"}, // F1 / F2
 { 283, 48,  25,  16, 134,   0,   5,   0, "F3"}, // F3 / F4
 { 283, 64,  25,  16, 135,   0,   6,   0, "F5"}, // F5 / F6
 { 283, 80,  25,  17, 136,   0,   3,   0, "F7"}, // F7 / F8
 // top row	
 {  16, 32,  16,  16,  95,   7,   1,   0, "ArrowLeft"}, // <-
 {  32, 32,  16,  16,  49,   7,   0,   0, "1"}, // 1 / !
 {  48, 32,  16,  16,  50,   7,   3,   0, "2"}, // 2 / "
 {  64, 32,  16,  16,  51,   1,   0,   0, "3"}, // 3 / #
 {  80, 32,  16,  16,  52,   1,   3,   0, "4"}, // 4 / $
 {  96, 32,  16,  16,  53,   2,   0,   0, "5"}, // 5 / %
 { 112, 32,  16,  16,  54,   2,   3,   0, "6"}, // 6 / &
 { 128, 32,  16,  16,  55,   3,   0,   0, "7"}, // 7 / '
 { 144, 32,  16,  16,  56,   3,   3,   0, "8"}, // 8 / (
 { 160, 32,  16,  16,  57,   4,   0,   0, "9"}, // 9 / )
 { 176, 32,  16,  16,  48,   4,   3,   0, "0"}, // 0 / 0
 { 192, 32,  16,  16,  43,   5,   0,   0, "+"}, // + / +
 { 208, 32,  16,  16,  45,   5,   3,   0, "-"}, // - / |
 { 224, 32,  16,  16,  92,   6,   0,   0, "Pound"}, // Pound / ..
 { 240, 32,  16,  16,  19,   6,   3,   0, "CLR"}, // CLR/HOME
 { 256, 32,  17,  16,  20,   0,   0,   0, "INST"}, // INST/DEL
 // 2nd row	
 {  16, 48,  24,  16,  24,   7,   2,   4, "CTRL"}, // CTRL - sticky ctrl
 {  40, 48,  16,  16, 113,   7,   6,   0, "Q"}, // Q
 {  56, 48,  16,  16, 119,   1,   1,   0, "W"}, // W
 {  72, 48,  16,  16, 101,   1,   6,   0, "E"}, // E
 {  88, 48,  16,  16, 114,   2,   1,   0, "R"}, // R
 { 104, 48,  16,  16, 116,   2,   6,   0, "T"}, // T
 { 120, 48,  16,  16, 121,   3,   1,   0, "Y"}, // Y
 { 136, 48,  16,  16, 117,   3,   6,   0, "U"}, // U
 { 152, 48,  16,  16, 105,   4,   1,   0, "I"}, // I
 { 168, 48,  16,  16, 111,   4,   6,   0, "O"}, // O
 { 184, 48,  16,  16, 112,   5,   1,   0, "P"}, // P
 { 200, 48,  16,  16,  64,   5,   6,   0, "@"}, // @
 { 216, 48,  16,  16,  42,   6,   1,   0, "*"}, // *
 { 232, 48,  16,  16,  94,   6,   6,   0, "ArrowUp"}, // ^| / π
 { 248, 48,  25,  16,  25,  -3,   0,   0, "RESTORE"}, // RESTORE
 // 3rd row	
 {  16, 64,  16,  16,   3,   7,   7,   0, "R/S"}, // RUN/STOP
 {  32, 64,  16,  16,  21,   1,   7,   1, "S/L"}, // SHIFT LOCK - sticky shift
 {  48, 64,  16,  16,  97,   1,   2,   0, "A"}, // A
 {  64, 64,  16,  16, 115,   1,   5,   0, "S"}, // S
 {  80, 64,  16,  16, 100,   2,   2,   0, "D"}, // D
 {  96, 64,  16,  16, 102,   2,   5,   0, "F"}, // F
 { 112, 64,  16,  16, 103,   3,   2,   0, "G"}, // G
 { 128, 64,  16,  16, 104,   3,   5,   0, "H"}, // H
 { 144, 64,  16,  16, 106,   4,   2,   0, "J"}, // J
 { 160, 64,  16,  16, 107,   4,   5,   0, "K"}, // K
 { 176, 64,  16,  16, 108,   5,   2,   0, "L"}, // L
 { 192, 64,  16,  16,  58,   5,   5,   0, ":"}, // : / [
 { 208, 64,  16,  16,  59,   6,   2,   0, ";"}, // ; / ]
 { 224, 64,  16,  16,  61,   6,   5,   0, "="}, // =
 { 240, 64,  33,  16,  13,   0,   1,   0, "CR"}, // RETURN
 // 4th row	
 {  16, 80,  16,  17,  23,   7,   5,   2, "C="}, // cbm - sticky cbm
 {  32, 80,  24,  17,  21,   1,   7,   1, "LSHIFT"}, // LSHIFT - sticky shift
 {  56, 80,  16,  17, 122,   1,   4,   0, "Z"}, // Z
 {  72, 80,  16,  17, 120,   2,   7,   0, "X"}, // X
 {  88, 80,  16,  17,  99,   2,   4,   0, "C"}, // C
 { 104, 80,  16,  17, 118,   3,   7,   0, "V"}, // V
 { 120, 80,  16,  17,  98,   3,   4,   0, "B"}, // B
 { 136, 80,  16,  17, 110,   4,   7,   0, "N"}, // N
 { 152, 80,  16,  17, 109,   4,   4,   0, "M"}, // M
 { 168, 80,  16,  17,  44,   5,   7,   0, ","}, // ,
 { 184, 80,  16,  17,  46,   5,   4,   0, "."}, // .
 { 200, 80,  16,  17,  47,   6,   7,   0, "/"}, // /
 { 216, 80,  24,  17,  21,   6,   4,   1, "RSHIFT"}, // RSHIFT - sticky shift
 { 240, 80,  16,  17,  17,   0,   7,   0, "C_DOWN"}, // UP / DOWN
 { 256, 80,  17,  17,  29,   0,   2,   0, "C_RIGHT"}, // LEFT / RIGHT
 // SPACE	
 {  63, 97, 147,  16,  32,   7,   4,   0, "SPACE"},  // SPACE
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
	char *fn;
	SDL_RWops *rwop;

	// init the 3DS bottom screen
	// keyboard image
	SDL_Surface *s=sdl_active_canvas->screen;
	if (olds != s || uibottom_must_redraw) {
		uistatusbar_must_redraw=1;
		olds=s;
		extern void *_binary_data_vice_png_start;
		extern void *_binary_data_vice_png_end;
		extern void *_binary_data_keyboard_png_start;
		extern void *_binary_data_keyboard_png_end;
		void *img1s = &_binary_data_vice_png_start;
		void *img1e = &_binary_data_vice_png_end;
		void *img2s = &_binary_data_keyboard_png_start;
		void *img2e = &_binary_data_keyboard_png_end;

		uibottom_must_redraw=0;
		bottom_r = (SDL_Rect){ .x = 0, .y = s->h/2, .w = s->w, .h=s->h/2};
		SDL_FillRect(s, &bottom_r, SDL_MapRGB(s->format, 0x60, 0x60, 0x60));

		// display background
		if (vice_img == NULL) {
			rwop = SDL_RWFromMem(img1s, img1e - img1s);
			vice_img = IMG_LoadPNG_RW(rwop);
			lib_free(fn);
		}
		SDL_BlitSurface(vice_img, NULL, s,
			&(SDL_Rect){.x = (s->w-vice_img->w)/2, .y = 240});

		// display keyboard
		if (kbd_img == NULL) {
			rwop = SDL_RWFromMem(img2s, img2e - img2s);
			kbd_img = IMG_LoadPNG_RW(rwop);
			lib_free(fn);
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

	int f,i1;
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
			if (f=uikbd_keypos[i].flg>0) {	// sticky key!!
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
}
