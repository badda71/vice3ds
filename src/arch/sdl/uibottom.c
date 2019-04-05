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
#include "viceicon.h"

int uikbd_pos[4][4] = {
	{0,0,320,120},		// normal keys
	{0,120,320,120},	// shifted keys
	{0,240,320,120},	// cbm keys
	{0,360,320,120}		// ctrl keys
};

#define KBD_NUMKEYS 66
#define BOTTOMSCREEN_W 320
#define BOTTOMSCREEN_H 240

typedef struct {
	int x,y,w,h,key,row,col,flg;
} uikbd_key;

uikbd_key uikbd_keypos[KBD_NUMKEYS] = {
 //  x,  y,   w,   h, key, row, col, sticky
 // F-Keys
 { 283, 32,  25,  16, 133,   0,   4,   0}, // F1 / F2
 { 283, 48,  25,  16, 134,   0,   5,   0}, // F3 / F4
 { 283, 64,  25,  16, 135,   0,   6,   0}, // F5 / F6
 { 283, 80,  25,  17, 136,   0,   3,   0}, // F7 / F8
 // top row	
 {  16, 32,  16,  16,  95,   7,   1,   0}, // <-
 {  32, 32,  16,  16,  49,   7,   0,   0}, // 1 / !
 {  48, 32,  16,  16,  50,   7,   3,   0}, // 2 / "
 {  64, 32,  16,  16,  51,   1,   0,   0}, // 3 / #
 {  80, 32,  16,  16,  52,   1,   3,   0}, // 4 / $
 {  96, 32,  16,  16,  53,   2,   0,   0}, // 5 / %
 { 112, 32,  16,  16,  54,   2,   3,   0}, // 6 / &
 { 128, 32,  16,  16,  55,   3,   0,   0}, // 7 / '
 { 144, 32,  16,  16,  56,   3,   3,   0}, // 8 / (
 { 160, 32,  16,  16,  57,   4,   0,   0}, // 9 / )
 { 176, 32,  16,  16,  48,   4,   3,   0}, // 0 / 0
 { 192, 32,  16,  16,  43,   5,   0,   0}, // + / +
 { 208, 32,  16,  16,  45,   5,   3,   0}, // - / |
 { 224, 32,  16,  16,  92,   6,   0,   0}, // Pound / ..
 { 240, 32,  16,  16,  19,   6,   3,   0}, // CLR/HOME
 { 256, 32,  17,  16,  20,   0,   0,   0}, // INST/DEL
 // 2nd row	
 {  16, 48,  24,  16,  23,   7,   2,   4}, // CTRL - sticky ctrl
 {  40, 48,  16,  16, 113,   7,   6,   0}, // Q
 {  56, 48,  16,  16, 119,   1,   1,   0}, // W
 {  72, 48,  16,  16, 101,   1,   6,   0}, // E
 {  88, 48,  16,  16, 114,   2,   1,   0}, // R
 { 104, 48,  16,  16, 116,   2,   6,   0}, // T
 { 120, 48,  16,  16, 121,   3,   1,   0}, // Y
 { 136, 48,  16,  16, 117,   3,   6,   0}, // U
 { 152, 48,  16,  16, 105,   4,   1,   0}, // I
 { 168, 48,  16,  16, 111,   4,   6,   0}, // O
 { 184, 48,  16,  16, 112,   5,   1,   0}, // P
 { 200, 48,  16,  16,  64,   5,   6,   0}, // @
 { 216, 48,  16,  16,  42,   6,   1,   0}, // *
 { 232, 48,  16,  16,  94,   6,   6,   0}, // ^| / π
 { 248, 48,  25,  16,  24,  -3,   0,   0}, // RESTORE
 // 3rd row	
 {  16, 64,  16,  16,   3,   7,   7,   0}, // RUN/STOP
 {  32, 64,  16,  16,  21,   1,   7,   1}, // SHIFT LOCK - sticky shift
 {  48, 64,  16,  16,  97,   1,   2,   0}, // A
 {  64, 64,  16,  16, 115,   1,   5,   0}, // S
 {  80, 64,  16,  16, 100,   2,   2,   0}, // D
 {  96, 64,  16,  16, 102,   2,   5,   0}, // F
 { 112, 64,  16,  16, 103,   3,   2,   0}, // G
 { 128, 64,  16,  16, 104,   3,   5,   0}, // H
 { 144, 64,  16,  16, 106,   4,   2,   0}, // J
 { 160, 64,  16,  16, 107,   4,   5,   0}, // K
 { 176, 64,  16,  16, 108,   5,   2,   0}, // L
 { 192, 64,  16,  16,  58,   5,   5,   0}, // : / [
 { 208, 64,  16,  16,  59,   6,   2,   0}, // ; / ]
 { 224, 64,  16,  16,  61,   6,   5,   0}, // =
 { 240, 64,  33,  16,  13,   0,   1,   0}, // RETURN
 // 4th row	
 {  16, 80,  16,  17,  22,   7,   5,   2}, // cbm - sticky cbm
 {  32, 80,  24,  17,  21,   1,   7,   1}, // SHIFT - sticky shift
 {  56, 80,  16,  17, 122,   1,   4,   0}, // Z
 {  72, 80,  16,  17, 120,   2,   7,   0}, // X
 {  88, 80,  16,  17,  99,   2,   4,   0}, // C
 { 104, 80,  16,  17, 118,   3,   7,   0}, // V
 { 120, 80,  16,  17,  98,   3,   4,   0}, // B
 { 136, 80,  16,  17, 110,   4,   7,   0}, // N
 { 152, 80,  16,  17, 109,   4,   4,   0}, // M
 { 168, 80,  16,  17,  44,   5,   7,   0}, // ,
 { 184, 80,  16,  17,  46,   5,   4,   0}, // .
 { 200, 80,  16,  17,  47,   6,   7,   0}, // /
 { 216, 80,  24,  17,  21,   6,   4,   1}, // SHIFT - sticky shift
 { 240, 80,  16,  17,  17,   0,   7,   0}, // UP / DOWN
 { 256, 80,  17,  17,  29,   0,   2,   0}, // LEFT / RIGHT
 // SPACE	
 {  63, 97, 147,  16,  32,   7,   4,   0}  // SPACE
};

int uibottom_kbdactive = 1;

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
	for (int i = 0; i < KBD_NUMKEYS; i++) {
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
	
	// init the 3DS bottom screen
	// keyboard image
	SDL_Surface *s=sdl_active_canvas->screen;
	if (olds != s) {
		olds=s;
		bottom_r = (SDL_Rect){ .x = 0, .y = s->h/2, .w = s->w, .h=s->h/2};
		SDL_FillRect(s, &bottom_r, SDL_MapRGB(s->format, 0x30, 0x30, 0x30));

		// display vice icon
		if (vice_img == NULL) {
			vice_img = SDL_CreateRGBSurfaceFrom(
				viceicon.pixel_data,
				viceicon.width,
				viceicon.height,
				viceicon.bytes_per_pixel*8,
				viceicon.bytes_per_pixel*viceicon.width,
				0x000000ff,
				0x0000ff00,
				0x00ff0000,
				0xff000000);
		}
		SDL_BlitSurface(vice_img, NULL, s,
			&(SDL_Rect){.x = (s->w-vice_img->w)/2, .y = 248});

		
		if (kbd_img == NULL) {
			char *fname = archdep_join_paths(archdep_user_config_path(),"ui_keyboard.bmp",NULL);
			kbd_img = SDL_LoadBMP(fname);
			lib_free(fname);
		}
		kb_x_pos = (s->w - uikbd_pos[0][2]) / 2;
		kb_y_pos = s->h - uikbd_pos[0][3];
		updateKeyboard(0);
		SDL_UpdateRect(sdl_active_canvas->screen, bottom_r.x, bottom_r.y, bottom_r.w, bottom_r.h);
	}
//	uistatusbar_draw();

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
			for (i = 0; i < KBD_NUMKEYS; i++) {
				if (x >= uikbd_keypos[i].x &&
					x <  uikbd_keypos[i].x + uikbd_keypos[i].w &&
					y >= uikbd_keypos[i].y &&
					y <  uikbd_keypos[i].y + uikbd_keypos[i].h) break;
			}
			kb_activekey=i;
		}
		if (i < KBD_NUMKEYS) { // got a hit
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
