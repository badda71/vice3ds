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
#include "archdep_xdg.h"
#include "lib.h"
#include "keyboard.h"
#include "mousedrv.h"
#include "log.h"
#include "videoarch.h"
#include "uistatusbar.h"
#include "kbd.h"
#include "uifonts.h"
#include "menu_misc.h"
#include "ui.h"
#include "palette.h"
#include "persistence.h"
#include "3ds_threadworker.h"
#include <SDL/SDL_image.h>
#include <3ds.h>

int uikbd_pos[4][4] = {
	{0,0,320,120},		// normal keys
	{0,120,320,120},	// shifted keys
	{0,240,320,120},	// cbm keys
	{0,360,320,120}		// ctrl keys
};

uikbd_key uikbd_keypos[] = {
	{   0,-10,  36,  10, 255,   0,   0,   0,  0,  "ToggleKB"},
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
	{   0,120,  64,  60, 241,   0,   0,   0,  1,  "SButton11"},
	{  64,120,  64,  60, 242,   0,   0,   0,  1,  "SButton12"},
	{ 128,120,  64,  60, 243,   0,   0,   0,  1,  "SButton13"},
	{ 192,120,  64,  60, 244,   0,   0,   0,  1,  "SButton14"},
	{ 256,120,  64,  60, 245,   0,   0,   0,  1,  "SButton15"},
	{   0,180,  64,  60, 246,   0,   0,   0,  1,  "SButton16"},
	{  64,180,  64,  60, 247,   0,   0,   0,  1,  "SButton17"},
	{ 128,180,  64,  60, 248,   0,   0,   0,  1,  "SButton18"},
	{ 192,180,  64,  60, 249,   0,   0,   0,  1,  "SButton19"},
	{ 256,180,  64,  60, 250,   0,   0,   0,  1,  "SButton20"},
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

// exposed variables
int uibottom_kbdactive = 1;
enum bottom_action uibottom_must_redraw = UIB_NO;
int uibottom_must_update_key = -1;
int bottom_lcd_off=0;

// internal variables
#define ICON_W 40
#define ICON_H 40

#define MAX(x, y) (((x) > (y)) ? (x) : (y))
#define MIN(x, y) (((x) < (y)) ? (x) : (y))

static SDL_Surface *vice_img=NULL;
static SDL_Surface *kbd_img=NULL;
static SDL_Surface *calcsb_img=NULL;
static SDL_Surface *twistyup_img=NULL;
static SDL_Surface *twistydn_img=NULL;

static int kb_x_pos = 0;
static int kb_y_pos = 0;
static int set_kb_y_pos = -10000;
static int bs_x_pos = 0;
static int bs_y_pos = 0;
static int kb_activekey;
static int sticky=0;
static int keypressed=-1;
static SDL_Surface *sbmask=NULL;
static int lock_updates=0;

static unsigned char keysPressed[256];
static void sbutton_repaint(int);

static void pressKey(int key, int press) {
	if (keysPressed[key]==press) return;
	keysPressed[key]=press;
	SDL_Surface *s=sdl_active_canvas->screen;
	int x,y,w,h;

	if (uikbd_keypos[key].flags==0) {
		// keyboard key - negative the key
		x=uikbd_keypos[key].x + kb_x_pos;
		y=uikbd_keypos[key].y + kb_y_pos;
		w=uikbd_keypos[key].w;
		h=uikbd_keypos[key].h;
		for (int yy = y; yy < MIN(y+h,s->h); yy++)
		{
			for (int xx = x; xx < x+w; xx++)
			{
				unsigned char *pixel = s->pixels + s->format->BytesPerPixel * (yy * s->w + xx);  
				pixel[1] = 255 - pixel[1];
				pixel[2] = 255 - pixel[2];
				pixel[3] = 255 - pixel[3];
			}
		}
	} else {
		// soft button - blit the mask image or background image onto the screen
		x=uikbd_keypos[key].x + bs_x_pos;
		y=uikbd_keypos[key].y + bs_y_pos;
		w=sbmask->w;
		h=sbmask->h;
		// set a clipping rectangle to exlude the keyboard
		SDL_SetClipRect(s, &(SDL_Rect){.x = bs_x_pos, .y=bs_y_pos, .w=320, .h=kb_y_pos-240});
		if (press) {
			// blit the mask image
			SDL_BlitSurface(sbmask, NULL, s, &(SDL_Rect){
				.x = bs_x_pos+uikbd_keypos[key].x,
				.y = bs_y_pos+uikbd_keypos[key].y});
		} else {
			// blit the background + icon
			sbutton_repaint(key);
		}
		SDL_SetClipRect(s, NULL);
	}
	if (!lock_updates && (sdl_menu_state || ui_emulation_is_paused())) SDL_UpdateRect(s, x,y,w,h);
}

static void updateKey(int key) {
	const char *s;
	if (key<0 || uikbd_keypos[key].key==0) return;
	int state=0;
	if (key == keypressed) state=1;
	else if (uikbd_keypos[key].sticky & sticky) state=1;
	else if (uikbd_keypos[key].flags==1) {
		ui_menu_entry_t *item;
		if ((item=sdlkbd_ui_hotkeys[uikbd_keypos[key].key]) != NULL &&
			(s=item->callback(0, item->data)) != NULL &&
			s[0]==UIFONT_CHECKMARK_CHECKED_CHAR)
			state=1;
	}
	pressKey(key,state);
}

static void updateKeys() {
	for (int i = 0; uikbd_keypos[i].key!=0; ++i) {
		updateKey(i);
	}
}

// paint the keyboard onto the background
static void keyboard_paint() {
	SDL_Rect rdest = {
		.x = kb_x_pos,
		.y = kb_y_pos};
	
	// which keyboard for whick sticky keys?
	int kb = 
		sticky == 1 ? 1:
		sticky == 2 ? 2:
		sticky >= 4 ? 3:
		0;

	// do we need to color any keys?
	if (kb>1) {
		// color keys "1"-"8" (49-56)
		for (int i = 0; uikbd_keypos[i].key != 0 ; ++i) {
			int k=uikbd_keypos[i].key-49;
			if (k<0 || k>7) continue;
			if (kb==2) k+=8;

			u8 r=sdl_active_canvas->palette->entries[k].red;
			u8 g=sdl_active_canvas->palette->entries[k].green;
			u8 b=sdl_active_canvas->palette->entries[k].blue;

			SDL_FillRect(
				sdl_active_canvas->screen,
				&(SDL_Rect){
					.x = kb_x_pos+uikbd_keypos[i].x,
					.y = kb_y_pos+uikbd_keypos[i].y,
					.w = uikbd_keypos[i].w,
					.h = uikbd_keypos[i].h},
				SDL_MapRGB(sdl_active_canvas->screen->format, r, g, b)
			);
		}
	}
	
	// blit the keyboard
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
	for(i=0; i<95 && name[i]!=0; ++i) {
		s[i]=isalnum(name[i])?name[i]:'_';
	}
	strcpy(s+i,".png");
	
	char *fn=archdep_join_paths(archdep_xdg_data_home(),"icons",s,NULL);
//log_3ds("loadIcon: %s",fn);
	SDL_Surface *r=IMG_Load(fn);
	lib_free(fn);
	return r;
}

static SDL_Surface *createIcon(char *name) {
//log_3ds("createIcon: %s",name);
	static SDL_Surface *chars=NULL;

	if (chars==NULL) {
		chars=IMG_Load("romfs:/charset.png");
		SDL_SetAlpha(chars, 0, 255);
	}
	SDL_Surface *icon=SDL_CreateRGBSurface(SDL_SWSURFACE,ICON_W,ICON_H,32,0xff000000,0x00ff0000,0x0000ff00,0x000000ff);
//	SDL_FillRect(icon, NULL, SDL_MapRGBA(icon->format, 0, 0, 0, 0));

	int maxw=ICON_W/8;
	int maxh=ICON_H/8;
	int x=0,y=0,c;
	char *t;
	for (int i=0; name[i] != 0 && y < maxh; ++i) {
		c=(name[i] & 0x7f)-32;
		if (c<0) c=0;
		if (c==0 && x==0) continue;
		if (c==0 && (((t=strchr(name+i+1,' '))==NULL && y<=maxh-1) || t-name-i > maxw-x)) {
			x=0;++y; continue;}
		SDL_BlitSurface(
			chars,
			&(SDL_Rect){.x=(c&0x0f)*8, .y=(c>>4)*8, .w=8, .h=8},
			icon,
			&(SDL_Rect){.x=x*8, .y=y*8});
		++x;
		if (x>=maxw) {
			x=0;++y;
			if ((t=strchr(name+i+1,' '))!=NULL && t-name-i-2<(maxw/2)) i=t-name;
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

// paint one soft button onto my calcsb_img
static void sbutton_update(int i) {
	ui_menu_entry_t *item;
	SDL_Surface *img;

	// blit icon
	if ((item=sdlkbd_ui_hotkeys[uikbd_keypos[i].key]) == NULL) return;	
	if ((img = sbuttons_getIcon(item->string)) == NULL) return;
	SDL_BlitSurface(img, NULL, calcsb_img, &(SDL_Rect){
		.x = uikbd_keypos[i].x + (uikbd_keypos[i].w - img->w)/2,
		.y = uikbd_keypos[i].y + (uikbd_keypos[i].h - img->h)/2});
}

// paint sbutton to screen - if i==-1 then repaint all
static void sbutton_repaint(int i) {
	// blit background image part
	SDL_BlitSurface(calcsb_img,
		i == -1 ? NULL : &(SDL_Rect){.x = uikbd_keypos[i].x, .y=uikbd_keypos[i].y, .w=uikbd_keypos[i].w, .h=uikbd_keypos[i].h},
		sdl_active_canvas->screen,
		&(SDL_Rect){
			.x = bs_x_pos + (i==-1?0:uikbd_keypos[i].x),
			.y = bs_y_pos + (i==-1?0:uikbd_keypos[i].y)});
}

// init calcsb_img and update all soft buttons 
static void sbuttons_recalc() {
	SDL_FreeSurface(calcsb_img);
	calcsb_img = SDL_ConvertSurface(vice_img, vice_img->format, SDL_SWSURFACE);

	for (int i = 0; uikbd_keypos[i].key != 0 ; ++i) {
		if (uikbd_keypos[i].flags == 1) sbutton_update(i);
	}
}

// paint the right twisty at the right place
void paint_twisty() {
	SDL_BlitSurface(
		kb_y_pos >= 480 ? twistyup_img: twistydn_img,
		NULL,
		sdl_active_canvas->screen,
		&(SDL_Rect){
			.x = kb_x_pos,
			.y = kb_y_pos - twistyup_img->h}
	);
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
	twistyup_img = IMG_Load("romfs:/kbd_twistyup.png");
	twistydn_img = IMG_Load("romfs:/kbd_twistydn.png");

	// calc global vars
	kb_x_pos = (s->w - uikbd_pos[0][2]) / 2;
	kb_y_pos = 
		persistence_getInt("kbd_hidden",0) ? s->h : s->h - uikbd_pos[0][3];
	bs_x_pos = (s->w - 320) / 2;
	bs_y_pos = s->h - 240;
	uibottom_must_redraw = UIB_ALL;
}

// update the whole bottom screen
void sdl_uibottom_draw(void)
{	
#ifndef UIBOTTOMOFF
	
	static void *olds = NULL;
	SDL_Surface *s=sdl_active_canvas->screen;
	enum bottom_action uibottom_must_redraw_local;

	// init if needed
	if (!uibottom_isinit) uibottom_init();

	if (olds != s || uibottom_must_redraw) {

		// needed for mutithreading
		uibottom_must_redraw_local = uibottom_must_redraw;
		uibottom_must_redraw = UIB_NO;
		if (set_kb_y_pos != -10000) {
			kb_y_pos=set_kb_y_pos;
			set_kb_y_pos=-10000;
		}
		
		olds=s;
		uistatusbar_must_redraw |= uibottom_must_redraw_local & UIB_GET_REPAINT_SBUTTONS;

		// recalc sbuttons if required
		if (uibottom_must_redraw_local & UIB_GET_RECALC_SBUTTONS)
			sbuttons_recalc();
		
		// repaint sbuttons if required
		if (uibottom_must_redraw_local & UIB_GET_REPAINT_SBUTTONS) {
			sbutton_repaint(-1);
		}

		// repaint keyboard if required
		if (uibottom_must_redraw_local & UIB_GET_REPAINT_KEYBOARD)
			keyboard_paint();

		// zero keypress status if we updated anything
		if (uibottom_must_redraw_local & (UIB_GET_REPAINT_KEYBOARD | UIB_GET_REPAINT_SBUTTONS)) {
			memset(keysPressed,0,sizeof(keysPressed));
		}

		// press the right keys
		lock_updates=1;
		updateKeys();
		lock_updates=0;

		// paint the twisty
		paint_twisty();

		// update screen
		if (sdl_menu_state || ui_emulation_is_paused()) SDL_UpdateRect(s, bs_x_pos, bs_y_pos, 320, 240);

		uibottom_must_update_key = -1;

	} else if (uibottom_must_update_key != -1) {
		updateKey(uibottom_must_update_key);
		if (uikbd_keypos[uibottom_must_update_key].flags == 1) {
			paint_twisty();
		}
		uibottom_must_update_key=-1;
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
		if (e->button.type != SDL_MOUSEBUTTONDOWN) {
			i=kb_activekey;
			kb_activekey=-1;
		} else {
			if (bottom_lcd_off) {
				GSPLCD_PowerOnBacklight(GSPLCD_SCREEN_BOTTOM);
				bottom_lcd_off=0;
				kb_activekey=-1;
				return 0; // do not further process the event
			}
			for (i = 0; uikbd_keypos[i].key != 0 ; ++i) {
				if (uikbd_keypos[i].flags == 1) {
					// soft button
					if (y >= kb_y_pos) continue; // ignore soft buttons below keyboard
					if (x >= uikbd_keypos[i].x + bs_x_pos &&
						x <  uikbd_keypos[i].x + uikbd_keypos[i].w + bs_x_pos &&
						y >= uikbd_keypos[i].y + bs_y_pos &&
						y <  uikbd_keypos[i].y + uikbd_keypos[i].h + bs_y_pos) break;
				} else {
					// keyboard button
					if (x >= uikbd_keypos[i].x + kb_x_pos &&
						x <  uikbd_keypos[i].x + uikbd_keypos[i].w + kb_x_pos &&
						y >= uikbd_keypos[i].y + kb_y_pos &&
						y <  uikbd_keypos[i].y + uikbd_keypos[i].h + kb_y_pos) break;
				}
			}
			kb_activekey=i;
		}
		if (i != -1 && uikbd_keypos[i].key != 0) { // got a hit
			if ((f=uikbd_keypos[i].sticky)>0) {	// ... on a sticky key
				if (e->button.type == SDL_MOUSEBUTTONDOWN) {
					sticky = sticky ^ uikbd_keypos[i].sticky;
					sdl_e.type = sticky & uikbd_keypos[i].sticky ? SDL_KEYDOWN : SDL_KEYUP;
					sdl_e.key.keysym.unicode = sdl_e.key.keysym.sym = uikbd_keypos[i].key;
					SDL_PushEvent(&sdl_e);
					uibottom_must_redraw |= UIB_RECALC_KEYBOARD;
				}
			} else { // ... on a normal key
				sdl_e.type = e->button.type == SDL_MOUSEBUTTONDOWN ? SDL_KEYDOWN : SDL_KEYUP;
				sdl_e.key.keysym.unicode = sdl_e.key.keysym.sym = uikbd_keypos[i].key;
				keypressed = (e->button.type == SDL_MOUSEBUTTONDOWN) ? i : -1;
				uibottom_must_update_key=i;
				uibottom_must_redraw |= UIB_KEYPRESS_ALL;
				SDL_PushEvent(&sdl_e);
			}
		}
	}
	return 0;
}

#define STEP 5
#define DELAY 10
int toggle_keyboard_thread(void *data) {
	if (kb_y_pos < 480) {
		// hide
		for (int i=kb_y_pos - kb_y_pos % STEP + STEP; i<=480; i+=STEP) {
			set_kb_y_pos=i;
			uibottom_must_redraw |= UIB_REPAINT_ALL;
			SDL_Delay(DELAY);
		}
	} else {
		// show
		for (int i=kb_y_pos - kb_y_pos % STEP - STEP; i>=480-uikbd_pos[0][3]; i-=STEP) {
			set_kb_y_pos=i;
			uibottom_must_redraw |= UIB_REPAINT_ALL;
			SDL_Delay(DELAY);
		}
	}
	return 0;
}

void toggle_keyboard() {
	persistence_putInt("kbd_hidden",kb_y_pos >= 480 ? 0 : 1);
	start_worker(toggle_keyboard_thread,NULL);
}