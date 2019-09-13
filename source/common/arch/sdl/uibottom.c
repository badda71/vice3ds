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
#include "log.h"
#include "videoarch.h"
#include "kbd.h"
#include "uifonts.h"
#include "menu_misc.h"
#include "ui.h"
#include "palette.h"
#include "persistence.h"
#include "vice3ds.h"
#include "machine.h"
#include "mouse.h"
#include "menu_common.h"
#include "mousedrv.h"
#include "util.h"
#include "sysfile.h"
#include "resources.h"
#include "lib.h"
#include <SDL/SDL_image.h>
#include <3ds.h>
#include <citro3d.h>

int uikbd_pos[4][4] = {
	{0,0,320,120},		// normal keys
	{0,120,320,120},	// shifted keys
	{0,240,320,120},	// cbm keys
	{0,360,320,120}		// ctrl keys
};

uikbd_key uikbd_keypos_C64[] = {
	//  x,  y,   w,   h, key,shft,sticky, flags, name
	// toggle kb button
	{   0,-15,  36,  15, 255,   0,   0,  0,  "ToggleKB"},
	// soft buttons
	{   0,  0,  64,  60, 231,   0,   0,  1,  "SButton1"},
	{  64,  0,  64,  60, 232,   0,   0,  1,  "SButton2"},
	{ 128,  0,  64,  60, 233,   0,   0,  1,  "SButton3"},
	{ 192,  0,  64,  60, 234,   0,   0,  1,  "SButton4"},
	{ 256,  0,  64,  60, 235,   0,   0,  1,  "SButton5"},
	{   0, 60,  64,  60, 236,   0,   0,  1,  "SButton6"},
	{  64, 60,  64,  60, 237,   0,   0,  1,  "SButton7"},
	{ 128, 60,  64,  60, 238,   0,   0,  1,  "SButton8"},
	{ 192, 60,  64,  60, 239,   0,   0,  1,  "SButton9"},
	{ 256, 60,  64,  60, 240,   0,   0,  1,  "SButton10"},
	{   0,120,  64,  60, 241,   0,   0,  1,  "SButton11"},
	{  64,120,  64,  60, 242,   0,   0,  1,  "SButton12"},
	{ 128,120,  64,  60, 243,   0,   0,  1,  "SButton13"},
	{ 192,120,  64,  60, 244,   0,   0,  1,  "SButton14"},
	{ 256,120,  64,  60, 245,   0,   0,  1,  "SButton15"},
	{   0,180,  64,  60, 246,   0,   0,  1,  "SButton16"},
	{  64,180,  64,  60, 247,   0,   0,  1,  "SButton17"},
	{ 128,180,  64,  60, 248,   0,   0,  1,  "SButton18"},
	{ 192,180,  64,  60, 249,   0,   0,  1,  "SButton19"},
	{ 256,180,  64,  60, 250,   0,   0,  1,  "SButton20"},
	// F-Keys
	{  81,  1,  30,  20, 133,   0,   0,  0,  "F1"}, // F1
	{ 111,  1,  30,  20, 137,   0,   0,  0,  "F2"}, // F2
	{ 141,  1,  30,  20, 134,   0,   0,  0,  "F3"}, // F3
	{ 171,  1,  30,  20, 138,   0,   0,  0,  "F4"}, // F4
	{ 201,  1,  30,  20, 135,   0,   0,  0,  "F5"}, // F5
	{ 231,  1,  30,  20, 139,   0,   0,  0,  "F6"}, // F6
	{ 261,  1,  30,  20, 136,   0,   0,  0,  "F7"}, // F7
	{ 291,  1,  29,  20, 140,   0,   0,  0,  "F8"}, // F8
	// top row	
	{   1, 21,  20,  20,  95,   0,   0,  0,  "ArrowLeft"}, // <-
	{  21, 21,  20,  20,  49,  33,   0,  0,  "1"}, // 1 / !
	{  41, 21,  20,  20,  50,  34,   0,  0,  "2"}, // 2 / "
	{  61, 21,  20,  20,  51,  35,   0,  0,  "3"}, // 3 / #
	{  81, 21,  20,  20,  52,  36,   0,  0,  "4"}, // 4 / $
	{ 101, 21,  20,  20,  53,  37,   0,  0,  "5"}, // 5 / %
	{ 121, 21,  20,  20,  54,  38,   0,  0,  "6"}, // 6 / &
	{ 141, 21,  20,  20,  55,  39,   0,  0,  "7"}, // 7 / '
	{ 161, 21,  20,  20,  56,  40,   0,  0,  "8"}, // 8 / (
	{ 181, 21,  20,  20,  57,  41,   0,  0,  "9"}, // 9 / )
	{ 201, 21,  20,  20,  48,   0,   0,  0,  "0"}, // 0 / 0
	{ 221, 21,  20,  20,  43,   0,   0,  0,  "+"}, // + / +
	{ 241, 21,  20,  20,  45, 124,   0,  0,  "-"}, // - / |
	{ 261, 21,  20,  20,  92,   0,   0,  0,  "Pound"}, // Pound / ..
	{ 281, 21,  20,  20,  19,   0,   0,  0,  "CLR/HOME"}, // CLR/HOME
	{ 301, 21,  19,  20,  20,   0,   0,  0,  "INST/DEL"}, // INST/DEL
	// 2nd row	
	{   1, 41,  30,  20,  24,   0,   4,  0,  "CTRL"}, // CTRL - sticky ctrl
	{  31, 41,  20,  20, 113,  81,   0,  0,  "Q"}, // Q
	{  51, 41,  20,  20, 119,  87,   0,  0,  "W"}, // W
	{  71, 41,  20,  20, 101,  69,   0,  0,  "E"}, // E
	{  91, 41,  20,  20, 114,  82,   0,  0,  "R"}, // R
	{ 111, 41,  20,  20, 116,  84,   0,  0,  "T"}, // T
	{ 131, 41,  20,  20, 121,  89,   0,  0,  "Y"}, // Y
	{ 151, 41,  20,  20, 117,  85,   0,  0,  "U"}, // U
	{ 171, 41,  20,  20, 105,  73,   0,  0,  "I"}, // I
	{ 191, 41,  20,  20, 111,  79,   0,  0,  "O"}, // O
	{ 211, 41,  20,  20, 112,  80,   0,  0,  "P"}, // P
	{ 231, 41,  20,  20,  64,   0,   0,  0,  "@"}, // @
	{ 251, 41,  20,  20,  42,   0,   0,  0,  "*"}, // *
	{ 271, 41,  20,  20,  94, 227,   0,  0,  "ArrowUp"}, // ^| / π
	{ 291, 41,  29,  20,  25,   0,   0,  0,  "RESTORE"}, // RESTORE
	// 3rd row	
	{   1, 61,  20,  20,   3,   0,   0,  0,  "RUN/STOP"}, // RUN/STOP
	{  21, 61,  20,  20,  21,   0,   1,  0,  "SHIFT"}, // SHIFT LOCK - sticky shift
	{  41, 61,  20,  20,  97,  65,   0,  0,  "A"}, // A
	{  61, 61,  20,  20, 115,  83,   0,  0,  "S"}, // S
	{  81, 61,  20,  20, 100,  68,   0,  0,  "D"}, // D
	{ 101, 61,  20,  20, 102,  70,   0,  0,  "F"}, // F
	{ 121, 61,  20,  20, 103,  71,   0,  0,  "G"}, // G
	{ 141, 61,  20,  20, 104,  72,   0,  0,  "H"}, // H
	{ 161, 61,  20,  20, 106,  74,   0,  0,  "J"}, // J
	{ 181, 61,  20,  20, 107,  75,   0,  0,  "K"}, // K
	{ 201, 61,  20,  20, 108,  76,   0,  0,  "L"}, // L
	{ 221, 61,  20,  20,  58,  91,   0,  0,  ":"}, // : / [
	{ 241, 61,  20,  20,  59,  93,   0,  0,  ";"}, // ; / ]
	{ 261, 61,  20,  20,  61,   0,   0,  0,  "="}, // =
	{ 281, 61,  39,  20,  13,   0,   0,  0,  "RETURN"}, // RETURN
	// 4th row	
	{   1, 81,  20,  20,  23,   0,   2,  0,  "C="}, // cbm - sticky cbm
	{  21, 81,  30,  20,  21,   0,   1,  0,  "SHIFT"}, // LSHIFT - sticky shift
	{  51, 81,  20,  20, 122,  90,   0,  0,  "Z"}, // Z
	{  71, 81,  20,  20, 120,  88,   0,  0,  "X"}, // X
	{  91, 81,  20,  20,  99,  67,   0,  0,  "C"}, // C
	{ 111, 81,  20,  20, 118,  86,   0,  0,  "V"}, // V
	{ 131, 81,  20,  20,  98,  66,   0,  0,  "B"}, // B
	{ 151, 81,  20,  20, 110,  78,   0,  0,  "N"}, // N
	{ 171, 81,  20,  20, 109,  77,   0,  0,  "M"}, // M
	{ 191, 81,  20,  20,  44,  60,   0,  0,  ","}, // ,
	{ 211, 81,  20,  20,  46,  62,   0,  0,  "."}, // .
	{ 231, 81,  20,  20,  47,  63,   0,  0,  "/"}, // /
	{ 251, 81,  30,  20,  21,   0,   1,  0,  "SHIFT"}, // RSHIFT - sticky shift
	// SPACE	
	{  61,101, 180,  19,  32,   0,   0,  0,  "SPACE"},  // SPACE
	// Cursor Keys
	{ 281, 81,  20,  20,  30,   0,   0,  0,  "C_UP"}, // DOWN
	{ 301,101,  19,  19,  29,   0,   0,  0,  "C_RIGHT"}, // RIGHT
	{ 281,101,  20,  19,  17,   0,   0,  0,  "C_DOWN"}, // UP
	{ 261,101,  20,  19,  31,   0,   0,  0,  "C_LEFT"}, // LEFT
	// Finish
	{   0,  0,   0,   0,   0,   0,   0,  0,  ""}
};

uikbd_key uikbd_keypos_C128[] = {
	//  x,  y,   w,   h, key,shft,sticky, flags, name
	// toggle kb button
	{   0,-15,  36,  15, 255,   0,   0,  0,  "ToggleKB"},
	// soft buttons
	{   0,  0,  64,  60, 231,   0,   0,  1,  "SButton1"},
	{  64,  0,  64,  60, 232,   0,   0,  1,  "SButton2"},
	{ 128,  0,  64,  60, 233,   0,   0,  1,  "SButton3"},
	{ 192,  0,  64,  60, 234,   0,   0,  1,  "SButton4"},
	{ 256,  0,  64,  60, 235,   0,   0,  1,  "SButton5"},
	{   0, 60,  64,  60, 236,   0,   0,  1,  "SButton6"},
	{  64, 60,  64,  60, 237,   0,   0,  1,  "SButton7"},
	{ 128, 60,  64,  60, 238,   0,   0,  1,  "SButton8"},
	{ 192, 60,  64,  60, 239,   0,   0,  1,  "SButton9"},
	{ 256, 60,  64,  60, 240,   0,   0,  1,  "SButton10"},
	{   0,120,  64,  60, 241,   0,   0,  1,  "SButton11"},
	{  64,120,  64,  60, 242,   0,   0,  1,  "SButton12"},
	{ 128,120,  64,  60, 243,   0,   0,  1,  "SButton13"},
	{ 192,120,  64,  60, 244,   0,   0,  1,  "SButton14"},
	{ 256,120,  64,  60, 245,   0,   0,  1,  "SButton15"},
	{   0,180,  64,  60, 246,   0,   0,  1,  "SButton16"},
	{  64,180,  64,  60, 247,   0,   0,  1,  "SButton17"},
	{ 128,180,  64,  60, 248,   0,   0,  1,  "SButton18"},
	{ 192,180,  64,  60, 249,   0,   0,  1,  "SButton19"},
	{ 256,180,  64,  60, 250,   0,   0,  1,  "SButton20"},
	// 128 keys
	{   1,  1,  20,  20, 150,   0,   0,  0,  "ESC"}, // ESC
	{  21,  1,  20,  20, 151,   0,   0,  0,  "TAB"}, // TAB
	{  41,  1,  20,  20, 152,   0,   0,  0,  "ALT"}, // Alt
	{  61,  1,  20,  20, 153,   0,   0,  0,  "CAPS"}, // Caps Lock
	{  81,  1,  20,  20, 154,   0,   0,  0,  "HELP"}, // Help
	{ 101,  1,  20,  20, 155,   0,   0,  0,  "LF"}, // Line feed
	{ 121,  1,  20,  20, 156,   0,   0,  0,  "40/80"}, // 40/80 Display
	{ 141,  1,  20,  20, 157,   0,   0,  0,  "NoScroll"}, // No Scroll
	{ 161,  1,  20,  20, 133,   0,   0,  0,  "F1"}, // F1
	{ 181,  1,  20,  20, 137,   0,   0,  0,  "F2"}, // F2
	{ 201,  1,  20,  20, 134,   0,   0,  0,  "F3"}, // F3
	{ 221,  1,  20,  20, 138,   0,   0,  0,  "F4"}, // F4
	{ 241,  1,  20,  20, 135,   0,   0,  0,  "F5"}, // F5
	{ 261,  1,  20,  20, 139,   0,   0,  0,  "F6"}, // F6
	{ 281,  1,  20,  20, 136,   0,   0,  0,  "F7"}, // F7
	{ 301,  1,  19,  20, 140,   0,   0,  0,  "F8"}, // F8

	// top row	
	{   1, 21,  20,  20,  95,   0,   0,  0,  "ArrowLeft"}, // <-
	{  21, 21,  20,  20,  49,  33,   0,  0,  "1"}, // 1 / !
	{  41, 21,  20,  20,  50,  34,   0,  0,  "2"}, // 2 / "
	{  61, 21,  20,  20,  51,  35,   0,  0,  "3"}, // 3 / #
	{  81, 21,  20,  20,  52,  36,   0,  0,  "4"}, // 4 / $
	{ 101, 21,  20,  20,  53,  37,   0,  0,  "5"}, // 5 / %
	{ 121, 21,  20,  20,  54,  38,   0,  0,  "6"}, // 6 / &
	{ 141, 21,  20,  20,  55,  39,   0,  0,  "7"}, // 7 / '
	{ 161, 21,  20,  20,  56,  40,   0,  0,  "8"}, // 8 / (
	{ 181, 21,  20,  20,  57,  41,   0,  0,  "9"}, // 9 / )
	{ 201, 21,  20,  20,  48,   0,   0,  0,  "0"}, // 0 / 0
	{ 221, 21,  20,  20,  43,   0,   0,  0,  "+"}, // + / +
	{ 241, 21,  20,  20,  45, 124,   0,  0,  "-"}, // - / |
	{ 261, 21,  20,  20,  92,   0,   0,  0,  "Pound"}, // Pound / ..
	{ 281, 21,  20,  20,  19,   0,   0,  0,  "CLR/HOME"}, // CLR/HOME
	{ 301, 21,  19,  20,  20,   0,   0,  0,  "INST/DEL"}, // INST/DEL
	// 2nd row	
	{   1, 41,  30,  20,  24,   0,   4,  0,  "CTRL"}, // CTRL - sticky ctrl
	{  31, 41,  20,  20, 113,  81,   0,  0,  "Q"}, // Q
	{  51, 41,  20,  20, 119,  87,   0,  0,  "W"}, // W
	{  71, 41,  20,  20, 101,  69,   0,  0,  "E"}, // E
	{  91, 41,  20,  20, 114,  82,   0,  0,  "R"}, // R
	{ 111, 41,  20,  20, 116,  84,   0,  0,  "T"}, // T
	{ 131, 41,  20,  20, 121,  89,   0,  0,  "Y"}, // Y
	{ 151, 41,  20,  20, 117,  85,   0,  0,  "U"}, // U
	{ 171, 41,  20,  20, 105,  73,   0,  0,  "I"}, // I
	{ 191, 41,  20,  20, 111,  79,   0,  0,  "O"}, // O
	{ 211, 41,  20,  20, 112,  80,   0,  0,  "P"}, // P
	{ 231, 41,  20,  20,  64,   0,   0,  0,  "@"}, // @
	{ 251, 41,  20,  20,  42,   0,   0,  0,  "*"}, // *
	{ 271, 41,  20,  20,  94, 227,   0,  0,  "ArrowUp"}, // ^| / π
	{ 291, 41,  29,  20,  25,   0,   0,  0,  "RESTORE"}, // RESTORE
	// 3rd row	
	{   1, 61,  20,  20,   3,   0,   0,  0,  "RUN/STOP"}, // RUN/STOP
	{  21, 61,  20,  20,  21,   0,   1,  0,  "SHIFT"}, // SHIFT LOCK - sticky shift
	{  41, 61,  20,  20,  97,  65,   0,  0,  "A"}, // A
	{  61, 61,  20,  20, 115,  83,   0,  0,  "S"}, // S
	{  81, 61,  20,  20, 100,  68,   0,  0,  "D"}, // D
	{ 101, 61,  20,  20, 102,  70,   0,  0,  "F"}, // F
	{ 121, 61,  20,  20, 103,  71,   0,  0,  "G"}, // G
	{ 141, 61,  20,  20, 104,  72,   0,  0,  "H"}, // H
	{ 161, 61,  20,  20, 106,  74,   0,  0,  "J"}, // J
	{ 181, 61,  20,  20, 107,  75,   0,  0,  "K"}, // K
	{ 201, 61,  20,  20, 108,  76,   0,  0,  "L"}, // L
	{ 221, 61,  20,  20,  58,  91,   0,  0,  ":"}, // : / [
	{ 241, 61,  20,  20,  59,  93,   0,  0,  ";"}, // ; / ]
	{ 261, 61,  20,  20,  61,   0,   0,  0,  "="}, // =
	{ 281, 61,  39,  20,  13,   0,   0,  0,  "RETURN"}, // RETURN
	// 4th row	
	{   1, 81,  20,  20,  23,   0,   2,  0,  "C="}, // cbm - sticky cbm
	{  21, 81,  30,  20,  21,   0,   1,  0,  "SHIFT"}, // LSHIFT - sticky shift
	{  51, 81,  20,  20, 122,  90,   0,  0,  "Z"}, // Z
	{  71, 81,  20,  20, 120,  88,   0,  0,  "X"}, // X
	{  91, 81,  20,  20,  99,  67,   0,  0,  "C"}, // C
	{ 111, 81,  20,  20, 118,  86,   0,  0,  "V"}, // V
	{ 131, 81,  20,  20,  98,  66,   0,  0,  "B"}, // B
	{ 151, 81,  20,  20, 110,  78,   0,  0,  "N"}, // N
	{ 171, 81,  20,  20, 109,  77,   0,  0,  "M"}, // M
	{ 191, 81,  20,  20,  44,  60,   0,  0,  ","}, // ,
	{ 211, 81,  20,  20,  46,  62,   0,  0,  "."}, // .
	{ 231, 81,  20,  20,  47,  63,   0,  0,  "/"}, // /
	{ 251, 81,  30,  20,  21,   0,   1,  0,  "SHIFT"}, // RSHIFT - sticky shift
	// SPACE	
	{  61,101, 180,  19,  32,   0,   0,  0,  "SPACE"},  // SPACE
	// Cursor Keys
	{ 281, 81,  20,  20,  30,   0,   0,  0,  "C_UP"}, // DOWN
	{ 301,101,  19,  19,  29,   0,   0,  0,  "C_RIGHT"}, // RIGHT
	{ 281,101,  20,  19,  17,   0,   0,  0,  "C_DOWN"}, // UP
	{ 261,101,  20,  19,  31,   0,   0,  0,  "C_LEFT"}, // LEFT
	// Finish
	{   0,  0,   0,   0,   0,   0,   0,  0,  ""}
};

typedef struct {
	unsigned w;
	unsigned h;
	float fw,fh;
	C3D_Tex tex;
} DS3_Image;

uikbd_key *uikbd_keypos=NULL;

// exposed variables
volatile enum bottom_action uibottom_must_redraw = UIB_NO;
int bottom_lcd_on=1;

// internal variables
#define ICON_W 40
#define ICON_H 40

#define MAX(x, y) (((x) > (y)) ? (x) : (y))
#define MIN(x, y) (((x) < (y)) ? (x) : (y))

// static sprites
static DS3_Image twistyup_spr;
static DS3_Image twistydn_spr;
static DS3_Image kbd1_spr;
static DS3_Image kbd2_spr;
static DS3_Image kbd3_spr;
static DS3_Image kbd4_spr;
static DS3_Image background_spr;
static DS3_Image sbmask_spr;
static DS3_Image sbdrag_spr;
static DS3_Image keymask_spr;
// dynamic sprites
static DS3_Image touchpad_spr;
static DS3_Image sbutton_spr[20]={0};
static char *sbutton_name[20]={NULL};
static int sbutton_nr[20];
static int sbutton_ang[20];
static int sbutton_dang[20];
static DS3_Image colkey_spr[8];
static int colkey_nr[8];
// SDL Surfaces
static SDL_Surface *sb_img=NULL;
static SDL_Surface *chars=NULL;
static SDL_Surface *smallchars=NULL;
static SDL_Surface *keyimg=NULL;
static SDL_Surface *joyimg=NULL;
static SDL_Surface *touchpad_img=NULL;

// variables for draging sbuttons
static int dragging;
static int drag_over;
static int dragx,dragy;
static int sb_paintlast;

// other stuff
static int kb_y_pos = 0;
static volatile int set_kb_y_pos = -10000;
static int kb_activekey=-1;
static int sticky=0;
static unsigned char keysPressed[256];
static int editmode_on=0;

// sprite handling funtions
// ========================
extern C3D_Mtx projection2;
extern C3D_RenderTarget* VideoSurface2;
extern int uLoc_projection;

int sintable[32]={0,6,12,18,23,27,30,31,32,31,30,27,23,18,12,6,0,-6,-12,-18,-23,-27,-30,-31,-32,-31,-30,-27,-23,-18,-12,-6};

#define CLEAR_COLOR 0x000000FF
// Used to convert textures to 3DS tiled format
// Note: vertical flip flag set so 0,0 is top left of texture
#define TEXTURE_TRANSFER_FLAGS \
	(GX_TRANSFER_FLIP_VERT(1) | GX_TRANSFER_OUT_TILED(1) | GX_TRANSFER_RAW_COPY(0) | \
	GX_TRANSFER_IN_FORMAT(GX_TRANSFER_FMT_RGBA8) | GX_TRANSFER_OUT_FORMAT(GX_TRANSFER_FMT_RGBA8) | \
	GX_TRANSFER_SCALING(GX_TRANSFER_SCALE_NO))

#define TEX_MIN_SIZE 64
unsigned int mynext_pow2(unsigned int v)
{
	v--;
	v |= v >> 1;
	v |= v >> 2;
	v |= v >> 4;
	v |= v >> 8;
	v |= v >> 16;
	v++;
	return v >= TEX_MIN_SIZE ? v : TEX_MIN_SIZE;
}

//---------------------------------------------------------------------------------
static inline void  drawImage( DS3_Image *img, int x, int y, int width, int height) {
//---------------------------------------------------------------------------------
	if (!img) return;
	if (!width) width=img->w;
	if (!height) height=img->h;
	x=(x*400)/320;
	width=(width*400)/320;

	C3D_TexBind(0, &(img->tex));
		
	// Draw a textured quad directly
	C3D_ImmDrawBegin(GPU_TRIANGLE_STRIP);
		C3D_ImmSendAttrib( x, y, 0.5f, 0.0f);		// v0 = position
		C3D_ImmSendAttrib( 0.0f, 0.0f, 0.0f, 0.0f);	// v1 = texcoord0

		C3D_ImmSendAttrib( x, y+height, 0.5f, 0.0f);
		C3D_ImmSendAttrib( 0.0f, img->fh, 0.0f, 0.0f);

		C3D_ImmSendAttrib( x+width, y, 0.5f, 0.0f);
		C3D_ImmSendAttrib( img->fw, 0.0f, 0.0f, 0.0f);

		C3D_ImmSendAttrib( x+width, y+height, 0.5f, 0.0f);
		C3D_ImmSendAttrib( img->fw, img->fh, 0.0f, 0.0f);
	C3D_ImmDrawEnd();
}

static void makeImage(DS3_Image *img, u8 *pixels, unsigned w, unsigned h, int noconv) {

	img->w=w;
	img->h=h;
	unsigned hw=mynext_pow2(w);
	img->fw=(float)(w)/hw;
	unsigned hh=mynext_pow2(h);
	img->fh=(float)(h)/hh;
	
	// GX_DisplayTransfer needs input buffer in linear RAM
	u8 *gpusrc = linearAlloc(hw*hh*4);
	memset(gpusrc,0,hw*hh*4);

	// copy to linear buffer, convert from RGBA to ABGR if needed
	u8* src=pixels; u8 *dst;
	unsigned pitch=w*4;
	for(int y = 0; y < h; y++) {
		dst=gpusrc+y*hw*4;
		if (noconv) {
			memcpy(dst,src,pitch);
			src+=pitch;
		} else {
			for (int x=0; x<w; x++) {
				int r = *src++;
				int g = *src++;
				int b = *src++;
				int a = *src++;

				*dst++ = a;
				*dst++ = b;
				*dst++ = g;
				*dst++ = r;
			}
		}
	}

	// init texture
	C3D_TexInit(&(img->tex), hw, hh, GPU_RGBA8);
	C3D_TexSetFilter(&(img->tex), GPU_NEAREST, GPU_NEAREST);

	// Convert image to 3DS tiled texture format
	GSPGPU_FlushDataCache(gpusrc, hw*hh*4);
	C3D_SyncDisplayTransfer ((u32*)gpusrc, GX_BUFFER_DIM(hw,hh), (u32*)(img->tex.data), GX_BUFFER_DIM(hw,hh), TEXTURE_TRANSFER_FLAGS);
	GSPGPU_FlushDataCache(img->tex.data, hw*hh*4);
	
	// cleanup
	linearFree(gpusrc);
	return;
}

static int loadImage(DS3_Image *img, char *fname) {
	SDL_Surface *s=IMG_Load(fname);
	if (!s) return -1;

	makeImage(img, s->pixels, s->w,s->h,0);
	SDL_FreeSurface(s);
	return 0;
}

// bottom handling functions
// =========================
static char *soft_button_positions=NULL;

static int soft_button_positions_load() {
	if (soft_button_positions == NULL) return 0;
	char *p=soft_button_positions;
	for (int i = 0; uikbd_keypos[i].key != 0; ++i) {
		if (uikbd_keypos[i].flags != 1) continue;	// not a soft button
		if (strlen(p)==0) break;
		uikbd_keypos[i].x=(int)strtoul(p,&p,10);
		uikbd_keypos[i].y=(int)strtoul(p,&p,10);
	}
	uibottom_must_redraw |= UIB_REPAINT;
	return 0;
}

static int soft_button_positions_save() {
	char s[256]={0};
	for (int i = 0; uikbd_keypos[i].key != 0; ++i) {
		if (uikbd_keypos[i].flags == 1) {
			sprintf(s+strlen(s)," %d %d",uikbd_keypos[i].x,uikbd_keypos[i].y);
		}
	}
	lib_free(soft_button_positions);
	soft_button_positions=lib_stralloc(s);
	return 0;
}

static int soft_button_positions_set(const char *val, void *param) {
	if (util_string_set(&soft_button_positions, val)) {
		return 0;
	}
	return soft_button_positions_load();
}

static void uibottom_repaint() {
	int i;
	uikbd_key *k;
	int drag_i=-1;
	int last_i=-1;

	// Render the scene
	C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
	C3D_RenderTargetClear(VideoSurface2, C3D_CLEAR_ALL, CLEAR_COLOR, 0);
	C3D_FrameDrawOn(VideoSurface2);
	// Update the uniforms
	C3D_FVUnifMtx4x4(GPU_VERTEX_SHADER, uLoc_projection, &projection2);

	// now, draw the sprites
	// first, the background
	if (_mouse_enabled) {
		drawImage(&touchpad_spr, 0, 0, 0, 0);
	} else {
		drawImage(&background_spr, 0, 0, 0, 0);
		// second, all sbuttons w/ selection mask if applicable
		for (i=0;i<20;i++) {
			if (sbutton_nr[i] == sb_paintlast) {
				last_i=i;
				continue;
			}
			if (dragging && sbutton_nr[i] == kb_activekey) {
				drag_i=i;
				continue;
			}

			// if editmode is on, "wiggle" the icons
			int dx=0,dy=0;
			if (editmode_on) {
				dx=sintable[sbutton_ang[i]]/17;
				dy=sintable[(sbutton_ang[i]+8) & 0x1F]/17;
				sbutton_ang[i]=(sbutton_ang[i] + sbutton_dang[i]) % 32;
			}

			k=&(uikbd_keypos[sbutton_nr[i]]);

			drawImage(&(sbutton_spr[i]),k->x+dx,k->y-dy,0,0);
			if (keysPressed[sbutton_nr[i]])
				drawImage(&(sbmask_spr),k->x+dx,k->y-dy,0,0);
			if (dragging && sbutton_nr[i] != kb_activekey && sbutton_nr[i] == drag_over)
				drawImage(&(sbdrag_spr),k->x+dx,k->y-dy,0,0);
		}
	}

	if (last_i!=-1) {
		k=&(uikbd_keypos[sbutton_nr[last_i]]);
		drawImage(&(sbutton_spr[last_i]),k->x,k->y,0,0);
		if (keysPressed[sbutton_nr[last_i]])
			drawImage(&(sbmask_spr),k->x,k->y,0,0);
		if (dragging && sbutton_nr[last_i] != kb_activekey && sbutton_nr[last_i] == drag_over)
			drawImage(&(sbdrag_spr),k->x,k->y,0,0);
	}
	// color sprites for keyboard
	for (i=0;i<8;i++) {
		k=&(uikbd_keypos[colkey_nr[i]]);
		drawImage(&(colkey_spr[i]),k->x,k->y+kb_y_pos,k->w,k->h);
	}
	// keyboard
	drawImage(
		(sticky&7) == 1 ? &(kbd2_spr):
		(sticky&7) == 2 ? &(kbd3_spr):
		(sticky&7) >= 4 ? &(kbd4_spr):
		&(kbd1_spr), 0, kb_y_pos, 0, 0);
	// twisty
	drawImage(
		kb_y_pos >= 240 ? &(twistyup_spr):&(twistydn_spr),
		0, kb_y_pos - twistyup_spr.h, 0, 0);
	// keys pressed
	for (i=0;uikbd_keypos[i].key!=0; ++i) {
		k=&(uikbd_keypos[i]);
		if (k->flags==1 || keysPressed[i]==0) continue;
		drawImage(&(keymask_spr),k->x,k->y+kb_y_pos,k->w,k->h);
	}
	// dragged icon (if applicable)
	if (drag_i!=-1) {
		int x=dragx-sb_img->w / 2;
		int y=dragy-sb_img->h / 2;
		int w=sb_img->w * 1.125;
		int h=sb_img->h * 1.125;
		drawImage(&(sbutton_spr[drag_i]),x,y,w,h);
		if (keysPressed[sbutton_nr[drag_i]])
			drawImage(&(sbmask_spr),x,y,w,h);
	}
	
	C3D_FrameEnd(0);
}

static void keypress_recalc() {
	const char *s;
	ui_menu_entry_t *item;
	int state,key;

	memset(keysPressed,0,sizeof(keysPressed));

	for (key = 0; uikbd_keypos[key].key!=0; ++key) {		
		if (key<0 || uikbd_keypos[key].key==0) return;

		state=0;
		if (key == kb_activekey) state=1;
		else if (uikbd_keypos[key].flags==1 && 
			(item=sdlkbd_ui_hotkeys[uikbd_keypos[key].key]) != NULL &&
			(item->type == MENU_ENTRY_RESOURCE_TOGGLE ||
			 item->type == MENU_ENTRY_RESOURCE_RADIO ||
			 item->type == MENU_ENTRY_OTHER_TOGGLE) &&
			(s=item->callback(0, item->data)) != NULL &&
			s[0]==UIFONT_CHECKMARK_CHECKED_CHAR)
			state=1;
		else if (uikbd_keypos[key].sticky & sticky) state=1;
		keysPressed[key]=state;
	}
}

// set up the colkey sprites and the positions relative to keyboard origin
static void keyboard_recalc() {
	static int oldkb = -1;
	static void *pal=NULL;
	u8 r=0,g=0,b=0;

	int kb = (sticky&7) == 2 ? 8: 0;
	if (kb!=oldkb || pal != sdl_active_canvas->palette->entries) {
		oldkb=kb;
		pal=sdl_active_canvas->palette->entries;

		// color keys "1"-"8" (49-56)
		for (int i = 0; uikbd_keypos[i].key != 0 ; ++i) {
			int k=uikbd_keypos[i].key-49;
			if (k<0 || k>7) continue;
			int col=k+kb;

			if (sdl_active_canvas->palette) {
				r=sdl_active_canvas->palette->entries[col].red;
				g=sdl_active_canvas->palette->entries[col].green;
				b=sdl_active_canvas->palette->entries[col].blue;
			}

			makeImage(&(colkey_spr[k]), (u8[]){r, g, b, 0xFF},1,1,0);
			colkey_nr[k]=i;
		}
	}
}

typedef struct {
	char *key;
	void *val;
} hash_item;

#define HASHSIZE 256
static hash_item iconHash[HASHSIZE]={{0,NULL}};

static unsigned char hashKey(char *key) {
	int i=0;
	while (*key!=0) i ^= *(key++);
	return i % HASHSIZE;
}

static void *hash_get(char *key) {
	if (key==NULL) return NULL;
	int i=hashKey(key);
	while (iconHash[i].key != NULL) {
		if (strcmp(key,iconHash[i].key)==0) return iconHash[i].val;
		++i; i %= HASHSIZE;
	}
	return NULL;
}

static void hash_put(char *key, void *val) {
	if (key==NULL) return;
	int i=hashKey(key);
	while (iconHash[i].key!=NULL && strcmp(iconHash[i].key,key)!=0) {
		++i; i %= HASHSIZE;
	}
	if (iconHash[i].key != NULL)
		lib_free(iconHash[i].key);
	iconHash[i].key=lib_stralloc(key);
	iconHash[i].val=val;
}

static SDL_Surface *loadIcon(char *name) {
	char s[100];
	int i;
	strcpy(s,"ICON_");
	for(i=0; i<90 && name[i]!=0; ++i) {
		s[i+5]=isalnum((int)(name[i]))?name[i]:'_';
	}
	strcpy(s+i+5,".png");
	
//log_3ds("loadIcon: %s",s);
	SDL_Surface *r=NULL;
	FILE *fp=sysfile_open(s,NULL,"r");
	if (fp) r=IMG_Load_RW(SDL_RWFromFP(fp,1),1);
	return r;
}

static SDL_Surface *createIcon(char *name) {
//log_3ds("createIcon: %s",name);
	int i,c,w,n,xof,yof;

	SDL_Surface *icon=SDL_CreateRGBSurface(SDL_SWSURFACE,ICON_W,ICON_H,32,0xff000000,0x00ff0000,0x0000ff00,0x000000ff);
//	SDL_FillRect(icon, NULL, SDL_MapRGBA(icon->format, 0, 0, 0, 0));

	if (strncmp("Joy ",name,4)==0) {
		// make a joy icon
		name+=4;
		w=strlen(name);
		if (w>5) w=5;
		xof=20-w*4;
		SDL_BlitSurface(joyimg,NULL,icon,NULL);
		for(i=0;i<w;i++) {
			c=(name[i] & 0x7f)-32;
			SDL_BlitSurface(chars,&(SDL_Rect){.x=(c&0x0f)*8, .y=(c>>4)*8, .w=8, .h=8},
				icon,&(SDL_Rect){.x=xof+i*8, .y=32});
		}	
	} else if (strncmp("Key ",name,4)==0) {
		// make a key icon
		// check width
		char *p=NULL;
		name+=4;
		if (strlen(name)<=2) w=strlen(name)*8+7;
		else if ((p=strchr(name,'/'))!=NULL) w=MAX(p-name,strlen(p+1))*4+1;
		else w=strlen(name)*4+1;
		if (w<15) w=15;
		if (w>34) w=34;

		// blit the empty key width the right size
		SDL_BlitSurface(keyimg, &(SDL_Rect){.x=0, .y=0, .w=4, .h=21},
			icon, &(SDL_Rect){.x=(34-w)/2 , .y=9 });
		SDL_BlitSurface(keyimg, &(SDL_Rect){.x=38-w, .y=0, .w=w+2, .h=21},
			icon, &(SDL_Rect){.x=(34-w)/2+4 , .y=9 });

		// blit the characters
		if (strlen(name)<=2) {
			// big characters
			xof=19-strlen(name)*4;
			yof=15;
			for (i=0;i<strlen(name);i++) {
				c=(name[i] & 0x7f)-32;
				SDL_BlitSurface(chars,&(SDL_Rect){.x=(c&0x0f)*8, .y=(c>>4)*8, .w=8, .h=8},
					icon,&(SDL_Rect){.x=xof+i*8, .y=yof});
			}
		} else {
			// small character, one or two lines
			char *p1[3] = {name, p == NULL ? NULL: p+1, NULL};
			if (p) *p=0;
			for (i=0; p1[i] != NULL; i++) {
				xof=20-MIN(w, strlen(p1[i])*4)/2;
				yof=(p?13:16)+i*6;
				for (n=0; n < strlen(p1[i]) && (n+1)*4 <= w; n++) {
					c=(p1[i][n] & 0x7f)-32;
					SDL_BlitSurface(smallchars,&(SDL_Rect){.x=(c&0x0f)*4, .y=(c>>4)*6, .w=4, .h=6},
						icon,&(SDL_Rect){.x=xof+n*4, .y=yof});
				}
			}
		}
	} else {
		// Just write the name on the surface
		int maxw=ICON_W/8;
		int maxh=ICON_H/8;
		char *buf=malloc(maxw*maxh);
		memset(buf,255,maxw*maxh);
		int x=0,y=0,cx=0,cy=0;
		char *t;
		for (i=0; name[i] != 0 && y < maxh; ++i) {
			c=(name[i] & 0x7f)-32;
			if (c<0) c=0;
			if (c==0 && x==0) continue;
			if (c==0 && (((t=strchr(name+i+1,' '))==NULL && y<=maxh-1) || t-name-i > maxw-x)) {
				x=0;++y; continue;}
			buf[x+y*maxw]=c & 0x7F;
			if (cx<x) cx=x;
			if (cy<y) cy=y;
			++x;
			if (x>=maxw) {
				x=0;++y;
				if ((t=strchr(name+i+1,' '))!=NULL && t-name-i-2<(maxw/2)) i=t-name;
			}
		}
		xof = (ICON_W-(cx+1)*8)/2;
		yof = (ICON_H-(cy+1)*8)/2;
		for (i=0; i<maxw*maxh; i++) {
			c=buf[i];
			if (c==255) continue;
			SDL_BlitSurface(
				chars,
				&(SDL_Rect){.x=(c&0x0f)*8, .y=(c>>4)*8, .w=8, .h=8},
				icon,
				&(SDL_Rect){.x=xof+(i % maxw)*8, .y=yof+(i/maxw)*8});
		}
		free(buf);
	}
	return icon;
}

static SDL_Surface *sbuttons_getIcon(char *name) {
//log_3ds("getIcon: %s",name);
	SDL_Surface *img;
	if (name==NULL) return NULL;
	if ((img=hash_get(name))!=NULL) return img;
	if ((img=loadIcon(name))==NULL &&
		(img=createIcon(name))==NULL) return NULL;
	hash_put(name, img);
	return img;
}

char *get_key_help(int key) {
	ui_menu_entry_t *item;
	char *r=NULL;
	r = get_3ds_mapping_name(key);
	if (r == NULL && (item=sdlkbd_ui_hotkeys[key]) != NULL)
		r = item->string;
	return r;
}

static void printstring(SDL_Surface *s, const char *str, int x, int y, SDL_Color col) {
	if (str==NULL) return;
	int c;
	SDL_SetPalette(chars, SDL_LOGPAL, &col, 1, 1);
	for (int i=0; str[i]!=0 ; i++) {
		c=(str[i] & 0x7f)-32;
		if (c<0) c=0;
		SDL_BlitSurface(
			chars,
			&(SDL_Rect){.x=(c&0x0f)*8, .y=(c>>4)*8, .w=8, .h=8},
			s,
			&(SDL_Rect){.x = x+i*8, .y = y});
	}
	SDL_SetPalette(chars, SDL_LOGPAL, &(SDL_Color){0xff,0xff,0xff,0}, 1, 1);
}

#define LMASK 0x01080001
#define RMASK 0x01080003

static void sbuttons_recalc() {
	int i,x,y;
	char *name;
	static int lbut=0,rbut=0;
	SDL_Surface *img;

	// create sprites for all sbuttons that are not yet created or are outdated
	for (i = 0; uikbd_keypos[i].key != 0 ; ++i) {
		if (uikbd_keypos[i].flags != 1) continue;	// not a soft button
		name=get_key_help(uikbd_keypos[i].key);
		x=uikbd_keypos[i].key-231;
		if (sbutton_spr[x].w != 0 && name == sbutton_name[x]) continue; // already up to date
		SDL_Surface *s = SDL_ConvertSurface(sb_img, sb_img->format, SDL_SWSURFACE);
		SDL_SetAlpha(s, 0, 255);
		if (name && (img = sbuttons_getIcon(name)) != NULL) {
			SDL_BlitSurface(img, NULL, s, &(SDL_Rect){
				.x = (s->w - img->w) / 2,
				.y = (s->h - img->h) /2});
		}
		makeImage(&(sbutton_spr[x]), s->pixels, s->w, s->h, 0);
		sbutton_name[x]=name;
		SDL_FreeSurface(s);
	}

	// recalc touchpad image
	const char *lbuf=NULL,*rbuf=NULL;

	// first check, if we have to update at all
	if (lbut == 0 ||
		rbut == 0 ||
		keymap3ds[lbut] != LMASK ||
		keymap3ds[rbut] != RMASK) {
	
		// check which buttons are mapped to mouse buttons
		for (i=1; i<255; i++) {
			switch (keymap3ds[i]) {
				case (LMASK):
					lbuf=get_3ds_keyname(i);
					lbut=i;
					break;
				case (RMASK):
					rbuf=get_3ds_keyname(i);
					rbut=i;
					break;
			}
		}
		if (!lbuf) rbuf="not mapped";
		if (!rbuf) lbuf="not mapped";

		// print info to touchpad image
		x=154,y=148;
		i=strlen(lbuf);
		SDL_FillRect(touchpad_img, &(SDL_Rect){
			.x = 0,	.y = y, .w = x+1, .h = 9},
			SDL_MapRGB(touchpad_img->format, 0, 0, 0));
		printstring(touchpad_img, lbuf, x-i*8,y, (SDL_Color){0x5d,0x5d,0x5d,0});

		x=167,y=148;
		i=strlen(rbuf);
		SDL_FillRect(touchpad_img, &(SDL_Rect) {
			.x = x-1,.y = y, .w = 321-x, .h = 9},
			SDL_MapRGB(touchpad_img->format, 0, 0, 0));
		printstring(touchpad_img, rbuf, x, y, (SDL_Color){0x5d,0x5d,0x5d,0});
		// create the sprite
		makeImage(&(touchpad_spr), touchpad_img->pixels, touchpad_img->w, touchpad_img->h, 0);
	}
	
}

// init bottom
static int uibottom_isinit=0;
static void uibottom_init() {
	uibottom_isinit=1;

	// pre-load images
	sb_img=IMG_Load("romfs:/sb.png");
	SDL_SetAlpha(sb_img, 0, 255);
	chars=IMG_Load("romfs:/chars.png");
	SDL_SetColorKey(chars, SDL_SRCCOLORKEY, 0x00000000);
	smallchars=IMG_Load("romfs:/smallchars.png");
	SDL_SetColorKey(smallchars, SDL_SRCCOLORKEY, 0x00000000);
	keyimg=IMG_Load("romfs:/keyimg.png");
	SDL_SetAlpha(keyimg, 0, 255);
	joyimg=IMG_Load("romfs:/joyimg.png");
	SDL_SetAlpha(joyimg, 0, 255);
	touchpad_img=IMG_Load("romfs:/touchpad.png");
	SDL_SetAlpha(touchpad_img, 0, 255);

	// pre-load sprites
	loadImage(&twistyup_spr, "romfs:/twistyup.png");
	loadImage(&twistydn_spr, "romfs:/twistydn.png");
	loadImage(&kbd1_spr, "romfs:/kbd1.png");
	loadImage(&kbd2_spr, "romfs:/kbd2.png");
	loadImage(&kbd3_spr, "romfs:/kbd3.png");
	loadImage(&kbd4_spr, "romfs:/kbd4.png");
	loadImage(&background_spr, "romfs:/background.png");
	loadImage(&sbmask_spr, "romfs:/sbmask.png");
	loadImage(&sbdrag_spr, "romfs:/sbdrag.png");
	makeImage(&keymask_spr, (u8[]){0x00, 0x00, 0x00, 0x80},1,1,0);

	// setup c3d texture environment
	// Configure depth test to overwrite pixels with the same depth (needed to draw overlapping sprites)
	C3D_DepthTest(true, GPU_GEQUAL, GPU_WRITE_ALL);

	uibottom_must_redraw |= UIB_ALL;
}

/*
void show_help()
{
	for(int i=200;i<222;i++) {
		log_citra("%s : %s", get_3ds_keyname(i), get_key_help(i));
	}
}
*/
// update the whole bottom screen
void sdl_uibottom_draw(void)
{	
	enum bottom_action uibottom_must_redraw_local;

	// init if needed
	if (!uibottom_isinit) {
		uibottom_init();
	}

	if (uibottom_must_redraw || editmode_on) {
		// needed for mutithreading
		uibottom_must_redraw_local = uibottom_must_redraw;
		uibottom_must_redraw = UIB_NO;
		if (set_kb_y_pos != -10000) {
			kb_y_pos=set_kb_y_pos;
			set_kb_y_pos=-10000;
		}

		// recalc sbuttons if required
		if (uibottom_must_redraw_local & UIB_GET_RECALC_SBUTTONS)
			sbuttons_recalc();

		// recalc keyboard if required
		if (uibottom_must_redraw_local & UIB_GET_RECALC_KEYBOARD) {
			keyboard_recalc();
		}

		// zero keypress status if we updated anything
		if (uibottom_must_redraw_local & UIB_GET_RECALC_KEYPRESS) {
			keypress_recalc();
		}

		uibottom_repaint();

		// check if our internal state matches the SDL state
		// (actually a stupid workaround for the issue that fullscreen sbutton does not unstick)
		if (!_mouse_enabled && kb_activekey != -1 && !SDL_GetMouseState(NULL, NULL))
			SDL_PushEvent( &(SDL_Event){ .type = SDL_MOUSEBUTTONUP });
	}
}

typedef struct animation {
	int *var;
	int from;
	int to;
} animation;

typedef struct animation_set {
	int steps;
	int delay;
	int nr;
	void (*callback)(void *);
	void *callback_data;
	void (*callback2)(void *);
	void *callback2_data;
	animation anim[];
} animation_set;

int animate(void *data){
	animation_set *a=data;
	int steps = a->steps != 0 ? a->steps : 24 ;
	int delay = a->delay != 0 ? a->delay : 10 ;
	for (int s=0; s <= steps; s++) {
		for (int i=0; i < a->nr; i++) {
			*(a->anim[i].var)=
				a->anim[i].from +
				(((a->anim[i].to - a->anim[i].from) * s ) / steps);
		}
		if (a->callback) (a->callback)(a->callback_data);
		if (s != steps) SDL_Delay(delay);
	}
	if (a->callback2) (a->callback2)(a->callback2_data);
	free(data);
	return 0;
}

void anim_callback(void *param) {
	uibottom_must_redraw |= UIB_REPAINT;
}

void anim_callback2(void *param) {
	soft_button_positions_save();
	sb_paintlast=-1;
}

void *alloc_copy(void *p, size_t s) {
	void *d=malloc(s);
	memcpy(d,p,s);
	return d;
}

static SDL_Event sdl_e;
void sdl_uibottom_mouseevent(SDL_Event *e) {
	int x = e->button.x*320/sdl_active_canvas->screen->w;
	int y = e->button.y*240/sdl_active_canvas->screen->h;
	uikbd_key *k;
	int i;

	// mouse press from mouse 1: comes from mapped mouse buttons -> forward to emulation
	if (e->button.which == 1) {
		if (sdl_menu_state || !_mouse_enabled) return;
		mouse_button((int)(e->button.button), (e->button.state == SDL_PRESSED));
		return;
	}

	// mousmotion
	if (e->type==SDL_MOUSEMOTION) {
		if (e->motion.state!=1) return;
		if (dragging) {
			// drag sbutton
			if (y>=kb_y_pos) y=kb_y_pos-1;
			dragx = x; dragy = y;
			for (i=0;i<20;i++) {
				k=&(uikbd_keypos[sbutton_nr[i]]);
				if (sbutton_nr[i] != drag_over &&
					x >= k->x &&
					x <  k->x + k->w  &&
					y >= k->y &&
					y <  k->y + k->h) {
					drag_over=sbutton_nr[i];
					break;
				}
				uibottom_must_redraw |= UIB_REPAINT;
			}

		// forward to emulation
		} else if (!sdl_menu_state)
			mouse_move((int)(e->motion.xrel), (int)(e->motion.yrel));

		return;
	}

	// buttonup
	if (e->button.type == SDL_MOUSEBUTTONUP) {
		if (kb_activekey==-1) return; // did not get the button down, so ignore button up
		i=kb_activekey;
		kb_activekey=-1;
		if (dragging) {
			// softbutton drop
			dragging=0;
			// swap icons: i <-> drag_over
			void *p=alloc_copy(&(int[]){
				0, 0, 2, // steps, delay, nr
				(int)anim_callback, 0, // callback, callback_data
				(int)anim_callback2, 0, // callback, callback_data
				(int)(&(uikbd_keypos[drag_over].x)),
					uikbd_keypos[drag_over].x,
					uikbd_keypos[i].x,
				(int)(&(uikbd_keypos[drag_over].y)),
					uikbd_keypos[drag_over].y,
					uikbd_keypos[i].y
			}, 13*sizeof(int));

			uikbd_keypos[i].x = uikbd_keypos[drag_over].x;
			uikbd_keypos[i].y = uikbd_keypos[drag_over].y;
			sb_paintlast=drag_over;
			start_worker(animate, p);
			uibottom_must_redraw |= UIB_REPAINT;
			return;
		}
	// buttondown
	} else {
		if (!bottom_lcd_on) {
			setBottomBacklight(1);
			kb_activekey=-1;
			return; // do not further process the event
		}
		for (i = 0; uikbd_keypos[i].key != 0 ; ++i) {
			if (uikbd_keypos[i].flags) {
				// soft button
				if (y >= kb_y_pos || _mouse_enabled) continue; // ignore soft buttons below keyboard or if mousepad is enabled
				if (x >= uikbd_keypos[i].x &&
					x <  uikbd_keypos[i].x + uikbd_keypos[i].w  &&
					y >= uikbd_keypos[i].y &&
					y <  uikbd_keypos[i].y + uikbd_keypos[i].h) break;
			} else {
				// keyboard button
				if (x >= uikbd_keypos[i].x &&
					x <  uikbd_keypos[i].x + uikbd_keypos[i].w &&
					y >= uikbd_keypos[i].y + kb_y_pos &&
					y <  uikbd_keypos[i].y + uikbd_keypos[i].h + kb_y_pos) break;
			}
		}
		if (uikbd_keypos[i].key == 0) return;
		kb_activekey=i;
		
		// start sbutton drag
		if (editmode_on && uikbd_keypos[i].flags) {
			dragging=1;
			drag_over=i;
			dragx = x; dragy = y;
			uibottom_must_redraw |= UIB_REPAINT;
			return;
		}
	}

	// sticky key press
	if (uikbd_keypos[i].sticky>0) {
		if (e->button.type == SDL_MOUSEBUTTONDOWN) {
			sticky = sticky ^ uikbd_keypos[i].sticky;
			sdl_e.type = sticky & uikbd_keypos[i].sticky ? SDL_KEYDOWN : SDL_KEYUP;
			sdl_e.key.keysym.sym = uikbd_keypos[i].key;
			sdl_e.key.keysym.unicode = 0;
			SDL_PushEvent(&sdl_e);
			uibottom_must_redraw |= UIB_RECALC_KEYBOARD;
		}
	} else {
		// normal key press
		sdl_e.type = e->button.type == SDL_MOUSEBUTTONDOWN ? SDL_KEYDOWN : SDL_KEYUP;
		sdl_e.key.keysym.sym = uikbd_keypos[i].key;
		if ((sticky & 1) && uikbd_keypos[i].shift)
			sdl_e.key.keysym.unicode = uikbd_keypos[i].shift;
		else
			sdl_e.key.keysym.unicode = sdl_e.key.keysym.sym;
		SDL_PushEvent(&sdl_e);
	}
	uibottom_must_redraw |= UIB_RECALC_KEYPRESS;
}

static int kb_hidden=-1;
int is_keyboard_hidden() {
	if (kb_hidden == -1)
		kb_hidden= kb_y_pos >= 240 ? 1:0;
	return kb_hidden;
}

void toggle_keyboard() {
	persistence_putInt("kbd_hidden",kb_hidden=(kb_y_pos >= 240 ? 0 : 1));
	start_worker(animate, alloc_copy(&(int[]){
		0, 0, 1, // steps, delay, nr
		(int)anim_callback, 0, // callback, callback_data
		0,0, // callback2, callback2_data
		(int)(&set_kb_y_pos), kb_y_pos < 240 ? 120 : 240, kb_y_pos < 240 ? 240 : 120
	}, 10*sizeof(int)));
}

void setBottomBacklight (int on) {
	if (on == bottom_lcd_on) return;
	gspLcdInit();
	bottom_lcd_on=on;
	if (on) {
		GSPLCD_PowerOnBacklight(GSPLCD_SCREEN_BOTTOM);
	} else {
		GSPLCD_PowerOffBacklight(GSPLCD_SCREEN_BOTTOM);
	}
	gspLcdExit();
}

void uibottom_shutdown() {
	// free the hash
	for (int i=0; i<HASHSIZE;++i)
		free(iconHash[i].key);
}

static resource_string_t resources_string[] = {
	{ "SoftButtonPositions",
		" 0 0 64 0 128 0 192 0 256 0 0 60 64 60 128 60 192 60 256 60 0 120 64 120 128 120 192 120 256 120 0 180 64 180 128 180 192 180 256 180",
		RES_EVENT_NO, NULL,
		&soft_button_positions,
		soft_button_positions_set, NULL },
	RESOURCE_STRING_LIST_END
};

int uibottom_resources_init() {
	if (strcmp("C64", machine_get_name())==0) {
		uikbd_keypos = uikbd_keypos_C64;
	} else {
		uikbd_keypos = uikbd_keypos_C128;
	}

	// calc global vars
	kb_y_pos = 
		persistence_getInt("kbd_hidden",0) ? 240 : 240 - uikbd_pos[0][3];

	for (int i = 0; uikbd_keypos[i].key != 0 ; ++i)
		if (uikbd_keypos[i].flags==1) sbutton_nr[uikbd_keypos[i].key-231]=i;

	srand(time(NULL));
	for (int i = 0; i<20; i++) {
		sbutton_ang[i] = rand() % 32;
		sbutton_dang[i] = rand() % 3 + 1;
		sbutton_dang[i] = (rand()%2) ? (32-sbutton_dang[i]) : sbutton_dang[i];
	}

	if (resources_register_string(resources_string) < 0) {
		return -1;
	}
	return 0;
}

void uibottom_resources_shutdown() {
	lib_free(soft_button_positions);
	soft_button_positions=NULL;
}

int uibottom_editmode_is_on() {
	return editmode_on;
}

void uibottom_toggle_editmode() {
	static int mouse;

	if (editmode_on) {
		_mouse_enabled=mouse;
		editmode_on=0;
	} else {
		mouse=_mouse_enabled;
		_mouse_enabled=0;
		editmode_on=1;
	}
	uibottom_must_redraw |= UIB_RECALC_KEYPRESS;
}