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
#include <SDL/SDL_image.h>
#include "kbd.h"

int uikbd_pos[4][4] = {
	{0,0,320,120},		// normal keys
	{0,120,320,120},	// shifted keys
	{0,240,320,120},	// cbm keys
	{0,360,320,120}		// ctrl keys
};

uikbd_key uikbd_keypos[] = {
	//  x,  y,   w,   h, key, row, col, sticky, flags, name
	// soft buttons
	{   0,  0,  64,  60, 231,   0,   0,   0,  1,  "SButton1"},
	{  64,  0,  64,  60, 232,   0,   0,   0,  1,  "SButton2"},
	{ 128,  0,  64,  60, 233,   0,   0,   0,  1,  "SButton3"},
	{ 192,  0,  64,  60, 234,   0,   0,   0,  1,  "SButton4"},
	{ 256,  0,  64,  60, 235,   0,   0,   0,  1,  "SButton5"},
	{   0, 60,  64,  60, 236,   0,   0,   0,  1,  "SButton6"},
	{  64, 60,  64,  60, 237,   0,   0,   0,  1,  "SButton7"},
	{ 128, 60,  64,  60, 238,   0,   0,   0,  1,  "SButton8"},
	{ 192, 60,  64,  60, 239,   0,   0,   0,  1,  "SButton9"},
	{ 256, 60,  64,  60, 240,   0,   0,   0,  1,  "SButton10"},
	// F-Keys
	{  81,  1,  30,  20, 133,   0,   4,   0,  0,  "F1"}, // F1
	{ 111,  1,  30,  20, 137,   0,   4,   0,  0,  "F2"}, // F2
	{ 141,  1,  30,  20, 134,   0,   5,   0,  0,  "F3"}, // F3
	{ 171,  1,  30,  20, 138,   0,   5,   0,  0,  "F4"}, // F4
	{ 201,  1,  30,  20, 135,   0,   6,   0,  0,  "F5"}, // F5
	{ 231,  1,  30,  20, 139,   0,   6,   0,  0,  "F6"}, // F6
	{ 261,  1,  30,  20, 136,   0,   3,   0,  0,  "F7"}, // F7
	{ 291,  1,  29,  20, 140,   0,   3,   0,  0,  "F8"}, // F8
	// top row	
	{   1, 21,  20,  20,  95,   7,   1,   0,  0,  "ArrowLeft"}, // <-
	{  21, 21,  20,  20,  49,   7,   0,   0,  0,  "1"}, // 1 / !
	{  41, 21,  20,  20,  50,   7,   3,   0,  0,  "2"}, // 2 / "
	{  61, 21,  20,  20,  51,   1,   0,   0,  0,  "3"}, // 3 / #
	{  81, 21,  20,  20,  52,   1,   3,   0,  0,  "4"}, // 4 / $
	{ 101, 21,  20,  20,  53,   2,   0,   0,  0,  "5"}, // 5 / %
	{ 121, 21,  20,  20,  54,   2,   3,   0,  0,  "6"}, // 6 / &
	{ 141, 21,  20,  20,  55,   3,   0,   0,  0,  "7"}, // 7 / '
	{ 161, 21,  20,  20,  56,   3,   3,   0,  0,  "8"}, // 8 / (
	{ 181, 21,  20,  20,  57,   4,   0,   0,  0,  "9"}, // 9 / )
	{ 201, 21,  20,  20,  48,   4,   3,   0,  0,  "0"}, // 0 / 0
	{ 221, 21,  20,  20,  43,   5,   0,   0,  0,  "+"}, // + / +
	{ 241, 21,  20,  20,  45,   5,   3,   0,  0,  "-"}, // - / |
	{ 261, 21,  20,  20,  92,   6,   0,   0,  0,  "Pound"}, // Pound / ..
	{ 281, 21,  20,  20,  19,   6,   3,   0,  0,  "CLR"}, // CLR/HOME
	{ 301, 21,  19,  20,  20,   0,   0,   0,  0,  "INST"}, // INST/DEL
	// 2nd row	
	{   1, 41,  30,  20,  24,   7,   2,   4,  0,  "CTRL"}, // CTRL - sticky ctrl
	{  31, 41,  20,  20, 113,   7,   6,   0,  0,  "Q"}, // Q
	{  51, 41,  20,  20, 119,   1,   1,   0,  0,  "W"}, // W
	{  71, 41,  20,  20, 101,   1,   6,   0,  0,  "E"}, // E
	{  91, 41,  20,  20, 114,   2,   1,   0,  0,  "R"}, // R
	{ 111, 41,  20,  20, 116,   2,   6,   0,  0,  "T"}, // T
	{ 131, 41,  20,  20, 121,   3,   1,   0,  0,  "Y"}, // Y
	{ 151, 41,  20,  20, 117,   3,   6,   0,  0,  "U"}, // U
	{ 171, 41,  20,  20, 105,   4,   1,   0,  0,  "I"}, // I
	{ 191, 41,  20,  20, 111,   4,   6,   0,  0,  "O"}, // O
	{ 211, 41,  20,  20, 112,   5,   1,   0,  0,  "P"}, // P
	{ 231, 41,  20,  20,  64,   5,   6,   0,  0,  "@"}, // @
	{ 251, 41,  20,  20,  42,   6,   1,   0,  0,  "*"}, // *
	{ 271, 41,  20,  20,  94,   6,   6,   0,  0,  "ArrowUp"}, // ^| / π
	{ 291, 41,  29,  20,  25,  -3,   0,   0,  0,  "RESTORE"}, // RESTORE
	// 3rd row	
	{   1, 61,  20,  20,   3,   7,   7,   0,  0,  "R/S"}, // RUN/STOP
	{  21, 61,  20,  20,  21,   1,   7,   1,  0,  "S/L"}, // SHIFT LOCK - sticky shift
	{  41, 61,  20,  20,  97,   1,   2,   0,  0,  "A"}, // A
	{  61, 61,  20,  20, 115,   1,   5,   0,  0,  "S"}, // S
	{  81, 61,  20,  20, 100,   2,   2,   0,  0,  "D"}, // D
	{ 101, 61,  20,  20, 102,   2,   5,   0,  0,  "F"}, // F
	{ 121, 61,  20,  20, 103,   3,   2,   0,  0,  "G"}, // G
	{ 141, 61,  20,  20, 104,   3,   5,   0,  0,  "H"}, // H
	{ 161, 61,  20,  20, 106,   4,   2,   0,  0,  "J"}, // J
	{ 181, 61,  20,  20, 107,   4,   5,   0,  0,  "K"}, // K
	{ 201, 61,  20,  20, 108,   5,   2,   0,  0,  "L"}, // L
	{ 221, 61,  20,  20,  58,   5,   5,   0,  0,  ":"}, // : / [
	{ 241, 61,  20,  20,  59,   6,   2,   0,  0,  ";"}, // ; / ]
	{ 261, 61,  20,  20,  61,   6,   5,   0,  0,  "="}, // =
	{ 281, 61,  39,  20,  13,   0,   1,   0,  0,  "CR"}, // RETURN
	// 4th row	
	{   1, 81,  20,  20,  23,   7,   5,   2,  0,  "C="}, // cbm - sticky cbm
	{  21, 81,  30,  20,  21,   1,   7,   1,  0,  "LSHIFT"}, // LSHIFT - sticky shift
	{  51, 81,  20,  20, 122,   1,   4,   0,  0,  "Z"}, // Z
	{  71, 81,  20,  20, 120,   2,   7,   0,  0,  "X"}, // X
	{  91, 81,  20,  20,  99,   2,   4,   0,  0,  "C"}, // C
	{ 111, 81,  20,  20, 118,   3,   7,   0,  0,  "V"}, // V
	{ 131, 81,  20,  20,  98,   3,   4,   0,  0,  "B"}, // B
	{ 151, 81,  20,  20, 110,   4,   7,   0,  0,  "N"}, // N
	{ 171, 81,  20,  20, 109,   4,   4,   0,  0,  "M"}, // M
	{ 191, 81,  20,  20,  44,   5,   7,   0,  0,  ","}, // ,
	{ 211, 81,  20,  20,  46,   5,   4,   0,  0,  "."}, // .
	{ 231, 81,  20,  20,  47,   6,   7,   0,  0,  "/"}, // /
	{ 251, 81,  30,  20,  21,   6,   4,   1,  0,  "RSHIFT"}, // RSHIFT - sticky shift
	// SPACE	
	{  61,101, 180,  19,  32,   7,   4,   0,  0,  "SPACE"},  // SPACE
	// Cursor Keys
	{ 281, 81,  20,  20,  30,   0,   7,   0,  0,  "C_UP"}, // DOWN
	{ 301,101,  19,  19,  29,   0,   2,   0,  0,  "C_RIGHT"}, // RIGHT
	{ 281,101,  20,  19,  17,   0,   7,   0,  0,  "C_DOWN"}, // UP
	{ 261,101,  20,  19,  31,   0,   2,   0,  0,  "C_LEFT"}, // LEFT
	// Finish
	{   0,  0,   0,   0,   0,   0,   0,   0,  0,  ""}
};

int uibottom_kbdactive = 1;
int uibottom_must_redraw = 0;

#define ICON_W 40
#define ICON_H 40

static SDL_Surface *vice_img=NULL;
static SDL_Surface *kbd_img=NULL;
static SDL_Rect bottom_r;
static int kb_x_pos = 0;
static int kb_y_pos = 0;
static int bs_x_pos = 0;
static int bs_y_pos = 0;
static int kb_activekey;
static int sticky=0;
static int keypressed=-1;
static SDL_Surface *sbmask=NULL;
static int lock_updates=0;

static void negativeKey (SDL_Surface *s, int x, int y, int w, int h, SDL_Surface *mask) {
	for (int yy = y; yy < y+h; yy++)
	{
		for (int xx = x; xx < x+w; xx++)
		{
			unsigned char *pixel = s->pixels + s->format->BytesPerPixel * (yy * s->w + xx);  
			if (mask == NULL ||
				((unsigned char *)mask->pixels + s->format->BytesPerPixel * ((yy - y) * mask->w + (xx - x)))[0]) {
				pixel[1] = 255 - pixel[1];
				pixel[2] = 255 - pixel[2];
				pixel[3] = 255 - pixel[3];
			} 
		}
	}
	if (!lock_updates) SDL_UpdateRect(s, x,y,w,h);
}

static unsigned char keysPressed[256];

static void pressKey(int key, int press) {
	if (keysPressed[key]==press) return;
	keysPressed[key]=press;
	if (uikbd_keypos[key].flags==0) {
		negativeKey(sdl_active_canvas->screen,
			uikbd_keypos[key].x + kb_x_pos,
			uikbd_keypos[key].y + kb_y_pos,
			uikbd_keypos[key].w,
			uikbd_keypos[key].h, NULL);
	} else {
		negativeKey(sdl_active_canvas->screen,
			uikbd_keypos[key].x + bs_x_pos + (uikbd_keypos[key].w - sbmask->w)/2,
			uikbd_keypos[key].y + bs_y_pos + (uikbd_keypos[key].h - sbmask->h)/2,
			sbmask->w,
			sbmask->h,
			sbmask);
	}
}

static void updateKey(int key) {
	if (key<0 || uikbd_keypos[key].key==0) return;
	int state=0;
	if (key == keypressed) state=1;
	else if (uikbd_keypos[key].sticky & sticky) state=1;
	else if (uikbd_keypos[key].flags==1) {
		ui_menu_entry_t *item;
		if ((item=sdlkbd_ui_hotkeys[uikbd_keypos[key].key]) != NULL &&
			item->callback(0, item->data) != NULL) state=1;
	}
	pressKey(key,state);
}

static void updateKeys() {
	for (int i = 0; uikbd_keypos[i].key!=0; ++i) {
		updateKey(i);
	}
}

static void updateKeyboard() {
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
}

typedef struct {
	char *key;
	void *val;
} hash_item;

static hash_item iconHash[256];

static unsigned char hashKey(char *key) {
	int i=0;
	while (*key!=0) i ^= *(key++);
	return i % 256;
}

static void *hash_get(char *key) {
	int i=hashKey(key);
	while (iconHash[i].key != NULL) {
		if (strcmp(key,iconHash[i].key)==0) return iconHash[i].val;
		++i; i %= 256;
	}
	return NULL;
}

static void hash_put(char *key, void *val) {
	int i=hashKey(key);
	while (iconHash[i].key!=NULL && strcmp(iconHash[i].key,key)!=0) {
		++i; i %= 256;
	}
	iconHash[i].key=key;
	iconHash[i].val=val;
}

static SDL_Surface *loadIcon(char *name) {
	char s[100];
	int i;
	strcpy(s,"romfs:/icons/");
	for(i=strlen(s); i<90 && *name!=0; ++i, ++name) {
		s[i]=isalnum(*name)?*name:'_';
	}
	strcpy(s+i,".png");
//log_3ds("loadIcon: %s",s);
	return IMG_Load(s);
}

static SDL_Surface *createIcon(char *name) {
//log_3ds("createIcon: %s",name);
	static SDL_Surface *chars=NULL;

	if (chars==NULL) {
		chars=IMG_Load("romfs:/charset.png");
		SDL_SetAlpha(chars, 0, 255);
	}
	SDL_Surface *icon=SDL_CreateRGBSurface(SDL_SWSURFACE,ICON_W,ICON_H,32,0xff000000,0x00ff0000,0x0000ff00,0x000000ff);
	SDL_FillRect(icon, NULL, SDL_MapRGBA(icon->format, 0, 0, 0, 0));

	int maxw=ICON_W/8;
	int maxh=ICON_H/8;
	int x=0,y=0,c;
	char *t;
	for (int i=0; name[i] != 0 && y < maxh; ++i) {
		c=(name[i] & 0x7f)-32;
		if (c==0 && x==0) continue;
		if (c==0) {x=0;++y; continue;}
		if (c<0) c=0;
		SDL_BlitSurface(
			chars,
			&(SDL_Rect){.x=(c&0x0f)*8, .y=(c>>4)*8, .w=8, .h=8},
			icon,
			&(SDL_Rect){.x=x*8, .y=y*8});
		++x;
		if (x>=maxw) {
			x=0;++y;
			if ((t=strchr(name+i+1,' '))!=NULL) i=t-name;
		}
	}
	return icon;
}

static SDL_Surface *sbuttons_getIcon(char *name) {
//log_3ds("getIcon: %s",name);
	SDL_Surface *img;
	if ((img=hash_get(name))!=NULL) return img;
	if ((img=loadIcon(name))==NULL &&
		(img=createIcon(name))==NULL) return NULL;
	hash_put(name, img);
	return img;
}

// update the icons on my soft buttons
static void sbuttons_update() {
	int i;
	ui_menu_entry_t *item;
	SDL_Surface *img;
	for (i = 0; uikbd_keypos[i].key != 0 ; ++i) {
		if (uikbd_keypos[i].flags != 1) continue;
		if ((item=sdlkbd_ui_hotkeys[uikbd_keypos[i].key]) == NULL) continue;
		
		if ((img = sbuttons_getIcon(item->string)) == NULL) continue;
		SDL_BlitSurface(img, NULL, sdl_active_canvas->screen, &(SDL_Rect){
				.x = bs_x_pos+uikbd_keypos[i].x+(uikbd_keypos[i].w - img->w)/2,
				.y = bs_y_pos+uikbd_keypos[i].y+(uikbd_keypos[i].h - img->h)/2});
	}
}

// init bottom
static int uibottom_isinit=0;
static void uibottom_init() {
	uibottom_isinit=1;
	SDL_Surface *s=sdl_active_canvas->screen;

	// pre-load some images
	sbmask=IMG_Load("romfs:/sbmask.png");
	vice_img = IMG_Load("romfs:/vice.png");
	kbd_img = IMG_Load("romfs:/keyboard.png");

	// calc global vars
	bottom_r = (SDL_Rect){ .x = 0, .y = s->h/2, .w = s->w, .h=s->h/2};
	kb_x_pos = (s->w - uikbd_pos[0][2]) / 2;
	kb_y_pos = s->h - uikbd_pos[0][3];
	bs_x_pos = (s->w - 320) / 2;
	bs_y_pos = s->h - 240;

}

// update the whole bottom screen
void sdl_uibottom_draw(void)
{	
#ifndef UIBOTTOMOFF
	
	static void *olds = NULL;
	SDL_Surface *s=sdl_active_canvas->screen;

	// init if needed
	if (!uibottom_isinit) uibottom_init();

	if (olds != s || uibottom_must_redraw) {
//log_3ds("Updating bottom screen");
		uistatusbar_must_redraw=1;
		olds=s;
		uibottom_must_redraw=0;
				
		// display the background image
		SDL_BlitSurface(vice_img, NULL, s,
			&(SDL_Rect){.x = (s->w-vice_img->w)/2, .y = 240});

		// display keyboard
		updateKeyboard();
		
		// update icons on sbuttons
		sbuttons_update();

		// press the right keys
		lock_updates=1;
		memset(keysPressed,0,sizeof(keysPressed));
		updateKeys();
		lock_updates=0;

		// update screen
		SDL_UpdateRect(s, bottom_r.x, bottom_r.y, bottom_r.w, bottom_r.h);
	}
	uistatusbar_draw();	
	//SDL_Flip(s);
#endif
}

static SDL_Event sdl_e;
int sdl_uibottom_mouseevent(SDL_Event *e) {

	int f;
	int x = e->button.x;
	int y = e->button.y;

	if (uibottom_kbdactive) {
		int i;
		// check which button was pressed
		if (e->button.type != SDL_MOUSEBUTTONDOWN) i=kb_activekey;
		else {
			for (i = 0; uikbd_keypos[i].key != 0 ; ++i) {
				if (x >= uikbd_keypos[i].x + kb_x_pos &&
					x <  uikbd_keypos[i].x + uikbd_keypos[i].w + kb_x_pos &&
					y >= uikbd_keypos[i].y + (uikbd_keypos[i].flags==0?kb_y_pos:bs_y_pos) &&
					y <  uikbd_keypos[i].y + uikbd_keypos[i].h + (uikbd_keypos[i].flags==0?kb_y_pos:bs_y_pos)) break;
			}
			kb_activekey=i;
		}
		if (uikbd_keypos[i].key != 0) { // got a hit
			if ((f=uikbd_keypos[i].sticky)>0) {	// sticky key!!
				if (e->button.type == SDL_MOUSEBUTTONDOWN) {
					sticky = sticky ^ uikbd_keypos[i].sticky;
					sdl_e.type = sticky & uikbd_keypos[i].sticky ? SDL_KEYDOWN : SDL_KEYUP;
					sdl_e.key.keysym.unicode = sdl_e.key.keysym.sym = uikbd_keypos[i].key;
					SDL_PushEvent(&sdl_e);
					uibottom_must_redraw=1;
				}
			} else {
				sdl_e.type = e->button.type == SDL_MOUSEBUTTONDOWN ? SDL_KEYDOWN : SDL_KEYUP;
				sdl_e.key.keysym.unicode = sdl_e.key.keysym.sym = uikbd_keypos[i].key;
				SDL_PushEvent(&sdl_e);
				keypressed = (e->button.type == SDL_MOUSEBUTTONDOWN) ? i : -1;
				updateKey(i);
			}
		}
	}
	return 0;
}
