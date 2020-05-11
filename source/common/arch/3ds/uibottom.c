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

#include "archdep_join_paths.h"
#include "archdep_user_config_path.h"
#include "archdep_xdg.h"
#include "joy.h"
#include "joystick.h"
#include "joyport.h"
#include "kbd.h"
#include "keyboard.h"
#include "lib.h"
#include "lib.h"
#include "log.h"
#include "machine.h"
#include "menu_common.h"
#include "menu_misc.h"
#include "mouse.h"
#include "mousedrv.h"
#include "palette.h"
#include "persistence.h"
#include "resources.h"
#include "sysfile.h"
#include "ui.h"
#include "uibottom.h"
#include "uifonts.h"
#include "uihotkey.h"
#include "uigb64.h"
#include "util.h"
#include "vice3ds.h"
#include "vice_sdl.h"
#include "videoarch.h"
#include "vsync.h"
#include <3ds.h>
#include <citro3d.h>
#include <png.h>

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
int help_on=0;
int grab_sbuttons=0;
SDL_Event lastevent;
int menu_invisible=0;

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
static DS3_Image black_spr;
static DS3_Image menubut_spr;

// dynamic sprites
static DS3_Image message_spr;
static DS3_Image touchpad_spr;
static DS3_Image menu_spr;
static DS3_Image help_top_spr;
static DS3_Image help_bot_spr;
static DS3_Image help_spr;
static DS3_Image sbutton_spr[20][4]={0};
static u32 sbutton_name_hash[20]={0};
static int sbutton_nr[20];
static int sbutton_ang[20];
static int sbutton_dang[20];
static DS3_Image colkey_spr[8];
static int colkey_nr[8];
static DS3_Image note_spr;
static DS3_Image gb64_spr;

// SDL Surfaces
static SDL_Surface *sb_img=NULL;
static SDL_Surface *chars=NULL;
SDL_Surface *mediumchars=NULL;
static SDL_Surface *smallchars=NULL;
static SDL_Surface *symchars=NULL;
static SDL_Surface *keyimg=NULL;
static SDL_Surface *touchpad_img=NULL;
static SDL_Surface *help_top_img=NULL;
static SDL_Surface *help_bottom_img=NULL;
static SDL_Surface *message_img=NULL;

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
static unsigned char keysHot[256];
static int editmode_on=0;
static int help_button_on;
static int menu_button_on;
static Handle repaintRequired;
static Handle menu_updated_event;
static volatile int help_anim;
static volatile int menu_anim=0;
static u8 *gpusrc=NULL;
static u8 *menusrc=NULL;
static u32 messagetime=0;

#define HASHSIZE 256
static tsh_object iconHash;

// sprite handling funtions
// ========================
extern C3D_Mtx projection2;
extern C3D_RenderTarget* VideoSurface2;
extern int uLoc_projection;
extern Handle privateSem1;

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
	x=(int)(((float)x*400.0f)/320.0f+0.5f);
	width=(int)(((float)width*400.0f)/320.0f+0.5f);

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

static void makeTexture(C3D_Tex *tex, u8 *mygpusrc, unsigned hw, unsigned hh) {
	svcWaitSynchronization(privateSem1, U64_MAX);
	// init texture
	C3D_TexDelete(tex);
	C3D_TexInit(tex, hw, hh, GPU_RGBA8);
	C3D_TexSetFilter(tex, GPU_NEAREST, GPU_NEAREST);

	// Convert image to 3DS tiled texture format
	GSPGPU_FlushDataCache(mygpusrc, hw*hh*4);
	C3D_SyncDisplayTransfer ((u32*)mygpusrc, GX_BUFFER_DIM(hw,hh), (u32*)(tex->data), GX_BUFFER_DIM(hw,hh), TEXTURE_TRANSFER_FLAGS);
	GSPGPU_FlushDataCache(tex->data, hw*hh*4);
	svcReleaseMutex(privateSem1);
}

// this function is NOT thread safe - it uses static buffer gpusrc!!
static void makeImage(DS3_Image *img, u8 *pixels, unsigned w, unsigned h, int noconv) {

	img->w=w;
	img->h=h;
	unsigned hw=mynext_pow2(w);
	img->fw=(float)(w)/hw;
	unsigned hh=mynext_pow2(h);
	img->fh=(float)(h)/hh;

	if (noconv) { // pixels are already in a transferable format (buffer in linear RAM, ABGR pixel format, pow2-dimensions)
		makeTexture(&(img->tex), pixels, hw, hh);
	} else {
		// GX_DisplayTransfer needs input buffer in linear RAM
		// memset(gpusrc,0,hw*hh*4);

		// copy to linear buffer, convert from RGBA to ABGR
		u8* src=pixels; u8 *dst;
		for(int y = 0; y < h; y++) {
			dst=gpusrc+y*hw*4;
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
		makeTexture(&(img->tex), gpusrc, hw, hh);
	}

	return;
}

static int loadImage(DS3_Image *img, char *fname)
{
	png_image image = {0};
	image.version = PNG_IMAGE_VERSION;
	if (png_image_begin_read_from_file(&image, fname) &&
		(image.format = PNG_FORMAT_ABGR) == PNG_FORMAT_ABGR &&
		png_image_finish_read(&image, NULL, gpusrc, mynext_pow2(image.width)*4, NULL))
	{
		makeImage(img, gpusrc, image.width, image.height, 1);
		return 0;
	}
	return -1;
}

static SDL_Surface *myIMG_Load(char *fname, int paletted)
{
	SDL_Surface *s;
	png_image image = {0};
	image.version = PNG_IMAGE_VERSION;
	if (!png_image_begin_read_from_file(&image, fname))
		return NULL;
	image.format = paletted ? PNG_FORMAT_RGBA_COLORMAP : PNG_FORMAT_RGBA;
	if (paletted)
		s = SDL_CreateRGBSurface(SDL_SWSURFACE,image.width, image.height, 8, 0, 0, 0, 0xff000000);
	else
		s = SDL_CreateRGBSurface(SDL_SWSURFACE,image.width, image.height, 32, 0x000000ff, 0x0000ff00, 0x00ff0000, 0xff000000);
	if (!s) {
		png_image_free(&image);
		return NULL;
	}
	if (!png_image_finish_read(&image, NULL, s->pixels, 0, paletted ? s->format->palette->colors: NULL)) {
		SDL_FreeSurface(s);
		return NULL;
	}
	return s;
}

static SDL_Surface *myIMG_Load_RW(FILE *fp, int paletted)
{
	SDL_Surface *s;
	png_image image = {0};
	image.version = PNG_IMAGE_VERSION;
	if (!png_image_begin_read_from_stdio(&image, fp))
		return NULL;
	image.format = paletted ? PNG_FORMAT_RGBA_COLORMAP : PNG_FORMAT_RGBA;
	if (paletted)
		s = SDL_CreateRGBSurface(SDL_SWSURFACE,image.width, image.height, 8, 0, 0, 0, 0xff000000);
	else
		s = SDL_CreateRGBSurface(SDL_SWSURFACE,image.width, image.height, 32, 0x000000ff, 0x0000ff00, 0x00ff0000, 0xff000000);
	if (!s) {
		png_image_free(&image);
		return NULL;
	}
	if (!png_image_finish_read(&image, NULL, s->pixels, 0, paletted ? s->format->palette->colors: NULL)) {
		SDL_FreeSurface(s);
		return NULL;
	}
	return s;
}

// bottom handling functions
// =========================
static inline void requestRepaint() {
	svcSignalEvent(repaintRequired);
}

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
	requestRepaint();
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

struct SDL_PrivateVideoData {
//GPU drawing specific
	int x1,y1,w1,h1; // drawing window for top screen          width and height of top screen part of the video buffer
	int x2,y2,w2,h2; //drawing window for bottom screen  and height of bottom part of the video buffer
	float l1,r1,t1,b1; // GPU source window for the top part of the video buffer
	float l2,r2,t2,b2; // GPU source window for the bottom part of the video buffer
	float scalex,scaley; // scaling factors
	float scalex2,scaley2; // scaling factors

// framebuffer data
    int w, h; // width and height of the video buffer
    void *buffer;
	Uint8 *palettedbuffer;
	GSPGPU_FramebufferFormats mode;
	unsigned int flags; // backup of create device flags
	unsigned int screens; // SDL_TOPSCR, SDL_BOTTOMSCR, SDL_DUALSCR
	unsigned int console; // SDL_CONSOLETOP, SDL_CONSOLEBOTTOM
	unsigned int fitscreen; // SDL_TRIMBOTTOMSCR, SDL_FITWIDTH, SDL_FITHEIGHT (SDL_FULLSCREEN sets both SDL_FITWIDTH and SDL_FITHEIGHT)
	int byteperpixel;
	int bpp;
// video surface
	SDL_Surface* currentVideoSurface;
// Video process flags
	bool blockVideo;  // block video output and events handlings on SDL_QUIT
};

extern void drawMainSpritesheetAt(int x, int y, int w, int h);
extern struct SDL_PrivateVideoData *vdev_hidden;

static void uibottom_repaint(void *param, int topupdated) {
	int i,update=1,j;
	uikbd_key *k;
	int drag_i=-1;
	int last_i=-1;

	if (svcWaitSynchronization(repaintRequired, 0)) update=0;

	// help on top screen
	if (help_on && (topupdated || update)) {
		int x,y,w,h;
		drawImage(&black_spr, 0,0,320,240);

		// midpoint: 200, 55, 144 -> 44
		x=200-((help_anim+44)*(200-(400-vdev_hidden->w1*vdev_hidden->scalex)/2))/144;
		y=55-((help_anim+44)*(55-(240-vdev_hidden->h1*vdev_hidden->scaley)/2))/144;
		w=((help_anim+44)*vdev_hidden->w1*vdev_hidden->scalex)/144;
		h=((help_anim+44)*vdev_hidden->h1*vdev_hidden->scaley)/144;
		drawMainSpritesheetAt(x,y,w,h);

		// xy: -363, -123 -> 0,0
		// wh: 1048, 787 -> 400, 240
		x = (help_anim*-363)/100;
		y = (help_anim*-123)/100;
		w = 320+(help_anim*728)/100;
		h = 240+(help_anim*547)/100;
		drawImage(&help_top_spr, x, y, w, h);
	}

	// paint gb64 topscreen
	if (sdl_menu_state && uigb64_top != NULL) {
//		drawImage(&black_spr, 0,0,320,240);
		drawImage(&gb64_spr, 0,0, 320, 240);
	}
	
	// paint message
	if (messagetime) {
		if (SDL_GetTicks() < messagetime)
			drawImage(&message_spr,0,0,320,12);
		else
			messagetime = 0;
	}

	if (!update) return;

	svcClearEvent(repaintRequired);
	svcWaitSynchronization(privateSem1, U64_MAX);

	if (set_kb_y_pos != -10000) {
		kb_y_pos=set_kb_y_pos;
		set_kb_y_pos=-10000;
	}

	// Render the scene
	C3D_RenderTargetClear(VideoSurface2, C3D_CLEAR_ALL, CLEAR_COLOR, 0);
	C3D_FrameDrawOn(VideoSurface2);

	if (_mouse_enabled) {
		// touchpad
		drawImage(&touchpad_spr, 0, 0, 0, 0);
	} else {
		// backound
		drawImage(&background_spr, 0, 0, 0, 0);
		// all sbuttons w/ selection mask if applicable
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
			j=keysHot[sbutton_nr[i]];
			if (j && sbutton_spr[i][j].w != 0) {
				drawImage(&(sbutton_spr[i][j]),k->x+dx,k->y-dy,0,0);
			} else {
				drawImage(&(sbutton_spr[i][0]),k->x+dx,k->y-dy,0,0);
			}
			if (keysPressed[sbutton_nr[i]] || (j && sbutton_spr[i][j].w == 0)) {
				drawImage(&(sbmask_spr),k->x+dx,k->y-dy,0,0);
			}
			if (dragging && sbutton_nr[i] != kb_activekey && sbutton_nr[i] == drag_over)
				drawImage(&(sbdrag_spr),k->x+dx,k->y-dy,0,0);
		}
		if (last_i!=-1) {
			k=&(uikbd_keypos[sbutton_nr[last_i]]);
			j=keysHot[sbutton_nr[last_i]];
			if (j && sbutton_spr[last_i][j].w != 0) {
				drawImage(&(sbutton_spr[last_i][j]),k->x,k->y,0,0);
			} else {
				drawImage(&(sbutton_spr[last_i][0]),k->x,k->y,0,0);
			}
			if (keysPressed[sbutton_nr[last_i]] || (j && sbutton_spr[last_i][j].w == 0)) {
				drawImage(&(sbmask_spr),k->x,k->y,0,0);
			}
			if (dragging && sbutton_nr[last_i] != kb_activekey && sbutton_nr[last_i] == drag_over)
				drawImage(&(sbdrag_spr),k->x,k->y,0,0);
		}
	}
	// menu
	if (sdl_menu_state && !menu_invisible) {
		// is menu updated?
		if (!svcWaitSynchronization(menu_updated_event, 0)) {
			svcClearEvent(menu_updated_event);
			makeTexture(&(menu_spr.tex), menusrc, 512, 256);
		}
		drawImage(&menu_spr, (menu_anim*-305)/100, (menu_anim*-225)/100, 0, 0);
		drawImage(&menubut_spr, ((menu_anim*-305)/100)+305,((menu_anim*-225)/100)+225,0,0);
	} else {
		if (menu_button_on) drawImage(&menubut_spr, 0,0,0,0);
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
	// keyboard twisty
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
		j=keysHot[sbutton_nr[drag_i]];
		if (j && sbutton_spr[drag_i][j].w != 0) {
			drawImage(&(sbutton_spr[drag_i][j]),x,y,w,h);
		} else {
			drawImage(&(sbutton_spr[drag_i][0]),x,y,w,h);
		}
		if (keysPressed[sbutton_nr[drag_i]] || (j && sbutton_spr[drag_i][j].w == 0)) {
			drawImage(&(sbmask_spr),x,y,w,h);
		}
	}
	// editmode notifiction
	if (editmode_on) {
		drawImage(&note_spr, 150,230,0,0);
	}
	// help screen (bottom part & button)
	if (help_on && !sdl_menu_state && !_mouse_enabled) {
		int x=(help_anim*305)/100;
		int y=(help_anim*-225)/100;
		drawImage(&help_bot_spr, x, y, 0, 0);
	} else {
		if (help_button_on && !(sdl_menu_state & ~MENU_ACTIVE) && uigb64_top == NULL) drawImage(&help_spr,305,0,0,0);
	}

	svcReleaseMutex(privateSem1);
}

static void keypress_recalc() {
	const char *s;
	ui_menu_entry_t *item;
	int key;
	unsigned char state;
//	memset(keysPressed,0,sizeof(keysPressed));
//	memset(keysHot,0,sizeof(keysHot));

	for (key = 0; uikbd_keypos[key].key!=0; ++key) {
		if (key<0 || uikbd_keypos[key].key==0) return;

		state=0;
		if (key == kb_activekey) state=1;
		else if (uikbd_keypos[key].sticky & sticky) state=1;
		keysPressed[key]=state;

		state=0;
		if (uikbd_keypos[key].flags==1 &&
			(item=sdlkbd_ui_hotkeys[uikbd_keypos[key].key]) != NULL &&
			(item->type == MENU_ENTRY_RESOURCE_TOGGLE ||
			 item->type == MENU_ENTRY_RESOURCE_RADIO ||
			 item->type == MENU_ENTRY_OTHER_TOGGLE ||
			 item->type == MENU_ENTRY_OTHER) &&
			(s=item->callback(0, item->data)) != NULL)
		{
			if (item->type == MENU_ENTRY_OTHER && (s[0] & 0xf0) == 0xf0)
				state = s[0] & 0x0f;
			else if (s[0]==UIFONT_CHECKMARK_CHECKED_CHAR)
				state=1;
		}
		keysHot[key]=state;
	}
}

// set up the colkey sprites and the positions relative to keyboard origin
static void keyboard_recalc() {
	static int oldkb = -1;
	static void *pal=NULL;
	u8 r=0,g=0,b=0;

	int kb = (sticky&7) == 2 ? 8: 0;

	if (kb!=oldkb || pal == NULL || pal != sdl_active_canvas->palette->entries) {
		oldkb=kb;
		if (sdl_active_canvas->palette) pal=sdl_active_canvas->palette->entries;

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

static SDL_Surface *loadIcon(char *name) {
	char s[100];
	int i;
	strcpy(s,"ICON_");
	for(i=0; i<90 && name[i]!=0; ++i) {
		s[i+5]=isalnum((int)(name[i]))?name[i]:'_';
	}
	strcpy(s+i+5,".png");

	SDL_Surface *r=NULL;
	FILE *fp=sysfile_open(s,NULL,"r");
	if (fp) {
		r=myIMG_Load_RW(fp, 0);
		fclose(fp);
	}
	return r;
}

typedef struct {
	SDL_Surface *img;
	int w;
	int h;
} font_info;


void get_font_info(font_info *f, enum font_size size) {
	switch (size) {
		case FONT_SMALL:
			f->img = smallchars;
			f->w=4; f->h=6;
			break;
		case FONT_MEDIUM:
			f->img = mediumchars;
			f->w=5; f->h=8;
			break;
		case FONT_SYM:
			f->img = symchars;
			f->w = f->h = 8;
			break;
		default:
			f->img = chars;
			f->w = f->h = 8;
	}
}

void uib_printtext(SDL_Surface *s, const char *str, int xo, int yo, int w, int h, enum font_size size, SDL_Color col) {
	font_info f;
	if (str==NULL) return;

	get_font_info(&f, size);
	int maxw=w/f.w;
	int maxh=h/f.h;
	char *buf=malloc(maxw*maxh);
	memset(buf,255,maxw*maxh);
	int x=0,y=0,cx=0,cy=0;
	char *t,c;
	for (int i=0; str[i] != 0 && y < maxh; ++i) {
		c=(str[i] & 0x7f)-32;
		if (c<0) c=0;
		if (c==0 && x==0) continue;
		if ((c==0 || c==92)  && (((t=strpbrk(str+i+1," |,"))==NULL && y < maxh) || t-str-i > maxw-x)) { // wrap on new word before end of line
			if (c==92) buf[x+y*maxw]=c & 0x7F;
			x=0;++y; continue;}
		buf[x+y*maxw]=c & 0x7F;
		if (cx<x) cx=x;
		if (cy<y) cy=y;
		++x;
		if (x>=maxw) { //end of line, next line
			x=0;++y;
			if ((t=strchr(str+i+1,' '))!=NULL && t-str-i-2<(maxw/2)) i=t-str; // wrap or cut?
		}
	}
	int xof = xo+(w-(cx+1)*f.w)/2;
	int yof = yo+(h-(cy+1)*f.h)/2;
	SDL_SetPalette(f.img, SDL_LOGPAL, &col, 1, 1);
	for (int i=0; i<maxw*maxh; i++) {
		c=buf[i];
		if (c==255) continue;
		SDL_BlitSurface(
			f.img,
			&(SDL_Rect){.x=(c&0x0f)*f.w, .y=(c>>4)*f.h, .w=f.w, .h=f.h},
			s,
			&(SDL_Rect){.x=xof+(i % maxw)*f.w, .y=yof+(i/maxw)*f.h});
	}
	SDL_SetPalette(f.img, SDL_LOGPAL, &(SDL_Color){0xff,0xff,0xff,0}, 1, 1);
	free(buf);
}

void uib_printstring(SDL_Surface *s, const char *str, int x, int y, int maxchars, enum str_alignment align, enum font_size size, SDL_Color col) {
	int xof, w;
	if (str==NULL || str[0]==0) return;
	font_info f;
	w=strlen(str);
	if (maxchars > 0 && w > maxchars) w=maxchars;
	get_font_info(&f,size);

	switch (align) {
		case ALIGN_CENTER:
			xof=x-(w*f.w/2);
			break;
		case ALIGN_RIGHT:
			xof=x-w*f.w;
			break;
		default:
			xof=x;
	}

	int c;
	SDL_SetPalette(f.img, SDL_LOGPAL, &col, 1, 1);
	for (int i=0; i < w; i++) {
		c=(str[i] & 0x7f)-32;
		if (c<0) c=0;
		SDL_BlitSurface(
			f.img,
			&(SDL_Rect){.x=(c&0x0f)*f.w, .y=(c>>4)*f.h, .w=f.w, .h=f.h},
			s,
			&(SDL_Rect){.x = xof+i*f.w, .y = y});
	}
	SDL_SetPalette(f.img, SDL_LOGPAL, &(SDL_Color){0xff,0xff,0xff,0}, 1, 1);
}

void uib_show_message(u32 ms_time, char *format, ... ) {
	char buffer[80];
	SDL_FillRect(message_img, NULL, SDL_MapRGBA(message_img->format,0,0,0,128));
	va_list args;
	va_start (args, format);
	vsnprintf (buffer,80,format, args);
	va_end (args);
	uib_printstring(message_img, buffer, 2, 2, 0, ALIGN_LEFT, FONT_MEDIUM, (SDL_Color){0xff,0xff,0xff,0});
//log_citra("%s: %s",__func__, buffer);
	makeImage(&message_spr, message_img->pixels, message_img->w, message_img->h, 0);
	messagetime = SDL_GetTicks() + ms_time;
}

static void blitKey(SDL_Surface *s, char *name, int y, enum str_alignment align) {
	int w;
	char *p=NULL;
	if (strlen(name)<=2) w=strlen(name)*8+7;
	else if ((p=strchr(name,'/'))!=NULL) w=MAX(p-name,strlen(p+1))*4+1;
	else w=strlen(name)*4+1;
	if (w<15) w=15;
	if (w>34) w=34;
	int xoff=
		align == ALIGN_CENTER? (34-w)/2:
		align == ALIGN_LEFT? 0:
		34-w;

	// blit the empty key width the right size
	SDL_BlitSurface(keyimg, &(SDL_Rect){.x=0, .y=0, .w=4, .h=21},
		s, &(SDL_Rect){.x=xoff, .y=y});
	SDL_BlitSurface(keyimg, &(SDL_Rect){.x=38-w, .y=0, .w=w+2, .h=21},
		s, &(SDL_Rect){.x=xoff + 4, .y=y });
	// blit the characters
	if (strlen(name)<=2) {
		uib_printstring(s, name, xoff+(w+6)/2, y+6, 0, ALIGN_CENTER, FONT_BIG, (SDL_Color){0xff,0xff,0xff,0});
	} else {
		// small characters, one or two lines
		uib_printstring(s, name, xoff+(w+8)/2, p?y+4:y+7, p?MIN(p-name,w/4):w/4, ALIGN_CENTER, FONT_SMALL, (SDL_Color){0xff,0xff,0xff,0});
		if (p)
			uib_printstring(s, p+1,xoff+(w+8)/2, y+10, w/4, ALIGN_CENTER, FONT_SMALL, (SDL_Color){0xff,0xff,0xff,0});
	}
}

static char *strtokstr_r(char *s, char *delim, char **save_ptr)
{
	char *end;
	if (s == NULL)
		s = *save_ptr;

	if (s == NULL || *s == '\0')
	{
		*save_ptr = s;
		return NULL;
	}

	// Skip leading delimiters.
	while (strstr(s,delim)==s) s+=strlen(delim);
	if (*s == '\0')
	{
		*save_ptr = s;
		return NULL;
	}

	// Find the end of the token.
	end = strstr (s, delim);
	if (end == NULL)
	{
		*save_ptr = s + strlen(s);
		return s;
	}

	// Terminate the token and make *SAVE_PTR point past it.
	memset(end, 0, strlen(delim));
	*save_ptr = end + strlen(delim);
	return s;
}

static SDL_Surface *createIcon(char *name) {
//log_citra("enter %s: %s",__func__,name);
	int i,c,xof,yof;

	SDL_Surface *icon=SDL_CreateRGBSurface(SDL_SWSURFACE,ICON_W,ICON_H,32,0x000000ff,0x0000ff00,0x00ff0000,0xff000000);
//	SDL_FillRect(icon, NULL, SDL_MapRGBA(icon->format, 0, 0, 0, 0));

	if (strncmp("Key ",name,4)==0) {
		char *saveptr1, *p, *s;
		// make a (multi) key icon
		// how many keys do we have?
		i = 0;
		s = p = lib_stralloc(name+4);
		while((p = strstr(p, " + "))!=NULL)
		{
		   i++;
		   p+=3;
		}
		// set up my key positions depending on the number of keys
		int pos[4][2]={
			{19, ALIGN_LEFT},
			{0, ALIGN_RIGHT},
			{0, ALIGN_LEFT},
			{19, ALIGN_RIGHT}};

		if(i == 0) {
			pos[0][0]=9; pos[0][1]=ALIGN_CENTER;
		}

		// paint the keys to the icon
		i = 0;
		p = strtokstr_r(s, " + ", &saveptr1);
		while(p != NULL && i < 4) {
			blitKey(icon, p, pos[i][0], pos[i][1]);
			p = strtokstr_r(NULL, " + ", &saveptr1);
			++i;
		}
		free(s);
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

static SDL_Surface *sbuttons_getIcon(char *name, int hot) {
//log_citra("enter %s: %s",__func__,name);
	SDL_Surface *img=NULL;
	if (name==NULL) return NULL;
	if (hot) {
		char ext[3]={'_', hot+48, 0};
		char *name1 = util_concat(name, ext, NULL);
		if ((img=tsh_get(&iconHash, name1))==NULL)
			img=loadIcon(name1);
		if (img) tsh_put(&iconHash, name1, img);
		lib_free(name1);
	} else {
		if ((img=tsh_get(&iconHash, name))==NULL &&
			((img=loadIcon(name))!=NULL ||
			(img=createIcon(name))!=NULL))
			tsh_put(&iconHash, name, img);
	}
	return img;
}

static char* replace_char(char* str, char find, char replace){
    char *current_pos = strchr(str,find);
    while (current_pos){
        *current_pos = replace;
        current_pos = strchr(current_pos,find);
    }
    return str;
}

static char buf[100];

char *get_key_help(int key, int inmenu, int trylen) {
	static int resolvemapping=1;
	int dev1,dev2,i1;
	ui_menu_entry_t *item;
	char *r=NULL,*p;

	if (!inmenu) {
		if (custom_help_text[key]) return custom_help_text[key];

		// check key mapping
		if (resolvemapping && (i1=keymap3ds[key])!=0) {
			if (i1 & 0x01000000) { // keymap
				resolvemapping=0;
				r=get_key_help((i1>>16) & 0xFF, 0, trylen);
				resolvemapping=1;
				if (r) return r;
			}
			r = get_3ds_mapping_name(key);
			if (r) return r;
		}

		// menu key
		if (key == sdl_ui_menukeys[0])
			return "Open Vice Menu";

		// check hotkey
		if ((item=sdlkbd_ui_hotkeys[key]) != NULL) {

			// type command hotkey -> return command
			if (item->callback == type_command_callback) {
				resources_get_string(item->data, (const char**)&p);
				snprintf(buf,100,"%s",p);
				for (p = buf; (p = strchr(p, '\\')) != NULL; *p = ' ');
				return buf;
			}

			if (!trylen) return item->string;
			p = r = sdl_ui_hotkey_path(item);
			while( strlen(p) > trylen && strchr(p,'&')) p=strchr(p,'&')+1;
			strcpy(buf,p);
			lib_free(r);
			replace_char(buf, '&', '|');
			return buf;
		}

		resources_get_int( "JoyPort1Device", &dev1);
		resources_get_int( "JoyPort2Device", &dev2);

		char *ports= dev1 == JOYPORT_ID_JOYSTICK ?
				(dev2 == JOYPORT_ID_JOYSTICK ? "1+2" : "1") :
				(dev2 == JOYPORT_ID_JOYSTICK ? "2" : NULL);

		if (ports) {
			resources_get_int("JoyDevice1", &dev1);
			resources_get_int( dev1==JOYDEV_KEYSET1?"KeySet1Fire":"KeySet2Fire", &i1);
			if (key == i1) {sprintf(buf,"Joy%s Fire", ports); return buf;}
			resources_get_int( dev1==JOYDEV_KEYSET1?"KeySet1AFire":"KeySet2AFire", &i1);
			if (key == i1) {sprintf(buf,"Joy%s AutoF", ports); return buf;}
			resources_get_int( dev1==JOYDEV_KEYSET1?"KeySet1North":"KeySet2North", &i1);
			if (key == i1) {sprintf(buf,"Joy%s Up", ports); return buf;}
			resources_get_int( dev1==JOYDEV_KEYSET1?"KeySet1South":"KeySet2South", &i1);
			if (key == i1) {sprintf(buf,"Joy%s Down", ports); return buf;}
			resources_get_int( dev1==JOYDEV_KEYSET1?"KeySet1West":"KeySet2West", &i1);
			if (key == i1) {sprintf(buf,"Joy%s Left", ports); return buf;}
			resources_get_int( dev1==JOYDEV_KEYSET1?"KeySet1East":"KeySet2East", &i1);
			if (key == i1) {sprintf(buf,"Joy%s Right", ports); return buf;}
		}
	} else {
		resources_get_int( "MenuKeyMap", &i1); if (key == i1) return "Map Hotkey";
		resources_get_int( "MenuKeyUp", &i1); if (key == i1) return "Up";
		resources_get_int( "MenuKeyDown", &i1); if (key == i1) return "Down";
		resources_get_int( "MenuKeyLeft", &i1); if (key == i1) return "Left";
		resources_get_int( "MenuKeyRight", &i1); if (key == i1) return "Right";
		resources_get_int( "MenuKeyPageUp", &i1); if (key == i1) return "Page Up";
		resources_get_int( "MenuKeyPageDown", &i1); if (key == i1) return "Page Down";
		resources_get_int( "MenuKeyHome", &i1); if (key == i1) return "Home";
		resources_get_int( "MenuKeyEnd", &i1); if (key == i1) return "End";
		resources_get_int( "MenuKeySelect", &i1); if (key == i1) return "Select";
		resources_get_int( "MenuKeyCancel", &i1); if (key == i1) return "Cancel";
		resources_get_int( "MenuKeyExit", &i1); if (key == i1) return "Exit";
		resources_get_int( "MenuKeyEmu", &i1); if (key == i1) return "Use Emulation";
	}
	return r;
}

#define LMASK 0x02010000
#define RMASK 0x02030000

static void sbuttons_recalc() {
//log_citra("enter %s",__func__);
	int i,x,y,j;
	char *name;
	static int lbut=0,rbut=0;
	SDL_Surface *img;
	u32 h;

	// create sprites for all sbuttons that are not yet created or are outdated
	for (i = 0; uikbd_keypos[i].key != 0 ; ++i) {
		if (uikbd_keypos[i].flags != 1) continue;	// not a soft button
		name=get_key_help(uikbd_keypos[i].key,0,0);
		x=uikbd_keypos[i].key-231;
		if (!name) name="";
		if ((h=hashKey((u8*)name))==sbutton_name_hash[x] && sbutton_spr[x][0].w != 0) continue; // already up to date
		SDL_Surface *s = SDL_ConvertSurface(sb_img, sb_img->format, SDL_SWSURFACE);
		SDL_SetAlpha(s, 0, 255);
		if (name && (img = sbuttons_getIcon(name, 0)) != NULL) {
			SDL_BlitSurface(img, NULL, s, &(SDL_Rect){
				.x = (s->w - img->w) / 2,
				.y = (s->h - img->h) /2});
		}
		makeImage(&(sbutton_spr[x][0]), s->pixels, s->w, s->h, 0);
		SDL_FreeSurface(s);

		for (j=1;j<4;++j) {
			if (name && (img = sbuttons_getIcon(name, j)) != NULL) {
				SDL_Surface *s = SDL_ConvertSurface(sb_img, sb_img->format, SDL_SWSURFACE);
				SDL_SetAlpha(s, 0, 255);
				SDL_BlitSurface(img, NULL, s, &(SDL_Rect){
					.x = (s->w - img->w) / 2,
					.y = (s->h - img->h) /2});
				makeImage(&(sbutton_spr[x][j]), s->pixels, s->w, s->h, 0);
				SDL_FreeSurface(s);
			} else break;
		}
		for (;j<4;++j) sbutton_spr[x][j].w=0;
		sbutton_name_hash[x]=h;
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
		if (!touchpad_img) {
			touchpad_img=myIMG_Load("romfs:/touchpad.png", 0);
			SDL_SetAlpha(touchpad_img, 0, 255);
		}
		x=154,y=148;
		i=strlen(lbuf);
		SDL_FillRect(touchpad_img, &(SDL_Rect){
			.x = 0,	.y = y, .w = x+1, .h = 9},
			SDL_MapRGB(touchpad_img->format, 0, 0, 0));
		uib_printstring(touchpad_img, lbuf, x, y, 0, ALIGN_RIGHT, FONT_BIG, (SDL_Color){0x5d,0x5d,0x5d,0});

		x=167,y=148;
		i=strlen(rbuf);
		SDL_FillRect(touchpad_img, &(SDL_Rect) {
			.x = x-1,.y = y, .w = 321-x, .h = 9},
			SDL_MapRGB(touchpad_img->format, 0, 0, 0));
		uib_printstring(touchpad_img, rbuf, x, y, 0, ALIGN_LEFT, FONT_BIG, (SDL_Color){0x5d,0x5d,0x5d,0});
		// create the sprite
		makeImage(&(touchpad_spr), touchpad_img->pixels, touchpad_img->w, touchpad_img->h, 0);
	}

}

// init bottom
static int uibottom_isinit=0;
static void uibottom_init() {
	uibottom_isinit=1;

	// pre-load images
	sb_img=myIMG_Load("romfs:/sb.png", 0);
	SDL_SetAlpha(sb_img, 0, 255);
	chars=myIMG_Load("romfs:/chars.png", 1);
	SDL_SetColorKey(chars, SDL_SRCCOLORKEY, 0x00000000);
	smallchars=myIMG_Load("romfs:/smallchars.png", 1);
	SDL_SetColorKey(smallchars, SDL_SRCCOLORKEY, 0x00000000);
	mediumchars=myIMG_Load("romfs:/mediumchars.png", 1);
	SDL_SetColorKey(mediumchars, SDL_SRCCOLORKEY, 0x00000000);
	symchars=myIMG_Load("romfs:/symchars.png", 1);
	SDL_SetColorKey(symchars, SDL_SRCCOLORKEY, 0x00000000);
	keyimg=myIMG_Load("romfs:/keyimg.png", 0);
	SDL_SetAlpha(keyimg, 0, 255);

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
	loadImage(&help_spr, "romfs:/help.png");
	loadImage(&menubut_spr, "romfs:/menu.png");
	makeImage(&keymask_spr, (u8[]){0x00, 0x00, 0x00, 0x80},1,1,0);
	makeImage(&black_spr, (u8[]){0, 0, 0, 0xFF},1,1,0);

	// create sprites
	SDL_Surface *s=SDL_CreateRGBSurface(SDL_SWSURFACE,256,12,32,0x000000ff,0x0000ff00,0x00ff0000,0xff000000);
	SDL_FillRect(s, NULL, SDL_MapRGBA(s->format,0,0,0,128));
	uib_printstring(s, "Press START to finish", 2, 2, 0, ALIGN_LEFT, FONT_BIG, (SDL_Color){0xff,0xff,0xff,0});
	makeImage(&note_spr, s->pixels, s->w, s->h, 0);
	SDL_FreeSurface(s);

	message_img=SDL_CreateRGBSurface(SDL_SWSURFACE,400,12,32,0x000000ff,0x0000ff00,0x00ff0000,0xff000000);
	SDL_FillRect(message_img, NULL, SDL_MapRGBA(message_img->format,0,0,0,128));
	makeImage(&message_spr, message_img->pixels, message_img->w, message_img->h, 0);

	svcCreateEvent(&repaintRequired, RESET_ONESHOT);
	svcCreateEvent(&menu_updated_event, RESET_ONESHOT);
	uibottom_must_redraw |= UIB_ALL;
	sdl_uibottom_draw();
	SDL_RequestCall(uibottom_repaint, NULL);
}

int menu_alpha_value=210;
int menu_alpha_max_y=240;

static palette_entry_t defentries[16] = {
	{"",0x00, 0x00, 0x00, 0},
	{"",0xff, 0xff, 0xff, 0},
	{"",0x88, 0x00, 0x00, 0},
	{"",0xaa, 0xff, 0xee, 0},
	{"",0xcc, 0x44, 0xcc, 0},
	{"",0x00, 0xcc, 0x55, 0},
	{"",0x00, 0x00, 0xaa, 0},
	{"",0x00, 0xee, 0x77, 0},
	{"",0xdd, 0x88, 0x55, 0},
	{"",0x66, 0x44, 0x00, 0},
	{"",0xff, 0x77, 0x77, 0},
	{"",0x33, 0x33, 0x33, 0},
	{"",0x77, 0x77, 0x77, 0},
	{"",0xaa, 0xff, 0x66, 0},
	{"",0x00, 0x88, 0xff, 0},
	{"",0xbb, 0xbb, 0xbb, 0}};

void menu_recalc() {
	menu_spr.w=320;
	menu_spr.h=240;
	unsigned hw=512;
	unsigned hh=256;
	menu_spr.fw=(float)(menu_spr.w)/hw;
	menu_spr.fh=(float)(menu_spr.h)/hh;
	int y,x;

	// convert the paletted menu draw buffer to ABGR
	u8* src=menu_draw_buffer;
	u8 *dst;
	palette_entry_t *pal;
	if (sdl_active_canvas->palette)
		pal= sdl_active_canvas->palette->entries;
	else
		pal=defentries;
	u8 alpha=menu_alpha_value;
	u8 alpha2=255;
	if (events_to_emu) alpha=alpha2=128;

	for(y = 0; y < 239; ++y) { // one pixel less for the white border line
		dst=menusrc+y*hw*4;
		if (y==menu_alpha_max_y) alpha=0;
		for (x=0; x<319; ++x) { // one pixel less for the white border line
			if (*src==0) *dst++ = alpha;// alpha
			else *dst++ = alpha2;
			*dst++ = pal[*src].blue;	// blue
			*dst++ = pal[*src].green;	// green
			*dst++ = pal[*src].red;		// red
			++src;
		}
		++src; // +border pixel
	}

	// draw white lines
	for (y=0;y<230;++y) {
		dst=menusrc+y*hw*4+4*319;
		*((int*)dst)=0xFFFFFFFF;
	}
	memset(menusrc+239*hw*4,255,310*4);

	// signal that menu texture needs to be updated
	svcSignalEvent(menu_updated_event);
}

void gb64_recalc() {
	if (uigb64_top == NULL) makeImage(&gb64_spr, (u8[]){0, 0, 0, 0},1,1,0);
	else makeImage(&gb64_spr, uigb64_top->pixels, uigb64_top->w, uigb64_top->h, 0);
}

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

		// recalc sbuttons if required
		if (uibottom_must_redraw_local & UIB_GET_RECALC_SBUTTONS)
			sbuttons_recalc();

		// recalc keyboard if required
		if (uibottom_must_redraw_local & UIB_GET_RECALC_KEYBOARD) {
			keyboard_recalc();
		}

		if (uibottom_must_redraw_local & UIB_GET_RECALC_KEYPRESS) {
			keypress_recalc();
		}
		if (sdl_menu_state && (uibottom_must_redraw_local & UIB_RECALC_MENU)) {
			menu_recalc();
		}

		if (sdl_menu_state && (uibottom_must_redraw_local & UIB_RECALC_GB64)) {
			gb64_recalc();
		}

		requestRepaint();
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
	int steps = a->steps != 0 ? a->steps : 15 ;
	int delay = a->delay != 0 ? a->delay : 16 ; // 1/60 sec, one 3ds frame
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
	requestRepaint();
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

int help_pos[][4] = {
	//	key, x, top-y, justification (0=left, 1=right)
	{209, 297, 224, ALIGN_LEFT},	// select
	{208, 297, 208, ALIGN_LEFT},	// start
	{201, 297, 192, ALIGN_LEFT},	// b
	{203, 297, 176, ALIGN_LEFT},	// y
	{200, 297, 160, ALIGN_LEFT},	// a
	{202, 297, 144, ALIGN_LEFT},	// x
	{214, 297, 104, ALIGN_LEFT},	// cstk up
	{215, 297, 112, ALIGN_LEFT},	// cstk dn
	{216, 297, 120, ALIGN_LEFT},	// cstk lt
	{217, 297, 128, ALIGN_LEFT},	// cstk rt
	{205, 297,  88, ALIGN_LEFT},	// r
	{207, 297,  56, ALIGN_LEFT},	// zr
	{210, 101, 192, ALIGN_RIGHT},	// dpad up
	{211, 101, 200, ALIGN_RIGHT},	// dpad dn
	{212, 101, 208, ALIGN_RIGHT},	// dpad lt
	{213, 101, 216, ALIGN_RIGHT},	// dpad rt
	{218, 101, 152, ALIGN_RIGHT},	// cpad up
	{219, 101, 160, ALIGN_RIGHT},	// cpad dn
	{220, 101, 168, ALIGN_RIGHT},	// cpad lt
	{221, 101, 176, ALIGN_RIGHT},	// cpad rt
	{206, 101, 136, ALIGN_RIGHT},	// zl
	{204, 101, 120, ALIGN_RIGHT},	// l
	{0,0,0,0}
};

void anim_callback3(void *param) {
	help_on=0;
	requestRepaint();
}

void anim_callback4(void *param) {
	sdl_menu_state &= ~MENU_ACTIVE;
	menu_anim=0;
	SDL_EnableKeyRepeat(0, 0);
	requestRepaint();
}

void toggle_menu(int active, ui_menu_entry_t *item)
{
	static int movekb=0, movemenu=0, old_kb_y_pos;

	if (active) {
		SDL_EnableKeyRepeat(SDL_DEFAULT_REPEAT_DELAY, SDL_DEFAULT_REPEAT_INTERVAL);
		menu_anim=movekb=movemenu=0;
		if (item == NULL ||
			item->type == MENU_ENTRY_DIALOG ||
			item->type == MENU_ENTRY_SUBMENU ||
			item->type == MENU_ENTRY_DYNAMIC_SUBMENU)
			movekb=1;
		if (movekb ||
			item->type == MENU_ENTRY_RESOURCE_INT ||
			item->type == MENU_ENTRY_RESOURCE_STRING)
			movemenu=1;
		old_kb_y_pos = kb_y_pos;
		if (movemenu) {
			start_worker(animate, alloc_copy(&(int[]){
				0, 0, 2, // steps, delay, nr
				(int)anim_callback, 0, // callback, callback_data
				0,0, // callback2, callback2_data
				(int)(&menu_anim), 100, 0,
				(int)(&set_kb_y_pos), kb_y_pos, movekb?240:kb_y_pos
			}, 13*sizeof(int)));
		}
		sdl_menu_state |= MENU_ACTIVE;
	} else {
		if (movemenu) {
			start_worker(animate, alloc_copy(&(int[]){
				0, 0, 2, // steps, delay, nr
				(int)anim_callback, 0, // callback, callback_data
				(int)anim_callback4, 0, // callback2, callback2_data
				(int)(&menu_anim), 0, 100,
				(int)(&set_kb_y_pos), kb_y_pos, old_kb_y_pos
			}, 13*sizeof(int)));
		} else {
			anim_callback4(NULL);
		}
	}
}

void toggle_help(int inmenu)
{
	SDL_Surface *help_img;
	static int old_kb_y_pos;

	if (!help_on) {
		char buf[40];

		// **** paint the top screen help image ********************
		if (!help_top_img) {
			help_top_img=myIMG_Load("romfs:/helpimg.png", 0);
			SDL_SetAlpha(help_top_img, 0, 255);
		}
		help_img = SDL_ConvertSurface(help_top_img, help_top_img->format, SDL_SWSURFACE);
		char *p;
		SDL_Color w=(SDL_Color){255,255,255,0};

		uib_printstring(help_img,
			inmenu?"Vice3DS Menu Help":"Vice3DS Emulator Help",
			8, 1, 0, ALIGN_LEFT, FONT_BIG, w);
		uib_printstring(help_img,
			inmenu?"=================":"=====================",
			8, 9, 0, ALIGN_LEFT, FONT_BIG, w);

		// print button functions
		for (int i=0; help_pos[i][0]!=0; i++) {
			p=get_key_help(help_pos[i][0],inmenu,0);
			snprintf(buf, 40, "%s", p?p:"-");
			uib_printstring(help_img, buf, help_pos[i][1], help_pos[i][2], 0, help_pos[i][3], FONT_MEDIUM, w);
		}
		// print 3ds slider
		switch (slider3d_func) {
			case 0:	// off
				p="-";
				break;
			case 1: // slowdown
				p="Slow down Emulation";
				break;
			default: //speedup
				p="Speed up Emulation";
		}
		uib_printstring(help_img, p, 297, 72, 0, ALIGN_LEFT, FONT_MEDIUM, w);

		// volume
		uib_printstring(help_img, "Volume", 101, 88, 0, ALIGN_RIGHT, FONT_MEDIUM, w);

		// create the sprite
		makeImage(&(help_top_spr), help_img->pixels, help_img->w, help_img->h, 0);
		SDL_FreeSurface(help_img);

		// **** paint the bottom screen help image ********************
		// 11 x 6 at 5,6
		SDL_Delay(10); // wait until previous makeImage has finished
		if (!help_bottom_img) {
			help_bottom_img=myIMG_Load("romfs:/helpoverlay.png", 0);
			SDL_SetAlpha(help_bottom_img, 0, 255);
		}
		
		help_img = SDL_ConvertSurface(help_bottom_img, help_bottom_img->format, SDL_SWSURFACE);
		if (!inmenu) {
			// write the help texts into the overlay
			for (int i = 0; uikbd_keypos[i].key != 0 ; ++i) {
				if (uikbd_keypos[i].flags != 1) continue;	// not a soft button
				char *name=get_key_help(uikbd_keypos[i].key,0,20);
				uib_printtext(help_img, name, uikbd_keypos[i].x+5, uikbd_keypos[i].y+6, 55, 48, FONT_MEDIUM, w);
			}
		}
		// create the sprite
		makeImage(&(help_bot_spr), help_img->pixels, help_img->w, help_img->h, 0);
		SDL_FreeSurface(help_img);

		old_kb_y_pos=kb_y_pos;

		help_anim=100;
		help_on=1;
		start_worker(animate, alloc_copy(&(int[]){
			0, 0, 2, // steps, delay, nr
			(int)anim_callback, 0, // callback, callback_data
			0,0, // callback2, callback2_data
			(int)(&help_anim), 100, 0,
			(int)(&set_kb_y_pos), kb_y_pos, _mouse_enabled?kb_y_pos:240
		}, 13*sizeof(int)));
	} else {
		start_worker(animate, alloc_copy(&(int[]){
			0, 0, 2, // steps, delay, nr
			(int)anim_callback, 0, // callback, callback_data
			(int)anim_callback3, 0, // callback2, callback2_data
			(int)(&help_anim), 0, 100,
			(int)(&set_kb_y_pos), _mouse_enabled?old_kb_y_pos:240, old_kb_y_pos
		}, 13*sizeof(int)));
	}
	requestRepaint();
}

static SDL_Event sdl_e;
SDL_Event lastevent;

ui_menu_action_t sdl_uibottom_mouseevent(SDL_Event *e) {

	int x = e->button.x*320/sdl_active_canvas->screen->w;
	int y = e->button.y*240/sdl_active_canvas->screen->h;
	uikbd_key *k;
	int i;
	static int menu_mousebuttonstate=0;

	// mouse press from mouse 1: comes from mapped mouse buttons -> forward to emulation
	if (e->button.which == 1) {
		if (!_mouse_enabled) return 0;
		mouse_button((int)(e->button.button), (e->button.state == SDL_PRESSED));
		return 0;
	}

	if (sdl_menu_state) lastevent = *e;

	// mousmotion
	if (e->type==SDL_MOUSEMOTION) {
		if (e->motion.state!=1) return menu_mousebuttonstate ? MENU_ACTION_MOUSE : 0;
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
			}
			requestRepaint();
		// forward to emulation
		} else if (!sdl_menu_state)
			mouse_move((int)(e->motion.xrel), (int)(e->motion.yrel));

		return menu_mousebuttonstate ? MENU_ACTION_MOUSE : 0;
	}

	// buttonup
	if (e->button.type == SDL_MOUSEBUTTONUP) {

		if (sdl_menu_state && menu_mousebuttonstate) {
			menu_mousebuttonstate = 0;
			return MENU_ACTION_MOUSE;
		}

		if (sdl_menu_state) menu_mousebuttonstate = 0;
		if (kb_activekey==-1) return 0; // did not get the button down, so ignore button up
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
			requestRepaint();
			return 0;
		}
	// buttondown
	} else {
		if (!bottom_lcd_on) {
			setBottomBacklight(1);
			kb_activekey=-1;
			uibottom_must_redraw |= UIB_RECALC_KEYPRESS;
			return 0; // do not further process the event
		}
		// help button
		if (help_button_on && !(sdl_menu_state & ~MENU_ACTIVE) && !help_on && x>=305 && y<=15) {
			toggle_help(sdl_menu_state);
			return 0;
		}
		// menu button
		if (menu_button_on && !(sdl_menu_state) && x<=15 && y<=15) {
			sdl_ui_activate();
			return 0;
		}
		for (i = 0; uikbd_keypos[i].key != 0 ; ++i) {
			if (uikbd_keypos[i].flags) {
				// soft button
				if (y >= kb_y_pos ||
					_mouse_enabled ||
					(sdl_menu_state && !grab_sbuttons))
					continue; // ignore soft buttons below keyboard or if mousepad is enabled or if in menu
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
		if (uikbd_keypos[i].key == 0) {
			if (sdl_menu_state && y < kb_y_pos) {
				if (y/8==0 && x/8<2) return MENU_ACTION_CANCEL;
				if (y>=215 && x>=305) return MENU_ACTION_EXIT;
				menu_mousebuttonstate = 1;
				return MENU_ACTION_MOUSE;
			}
			return 0;
		}
		if (i==kb_activekey) return 0; // ignore button down on an already pressed key
		kb_activekey=i;

		// start sbutton drag
		if (editmode_on && uikbd_keypos[i].flags) {
			dragging=1;
			drag_over=i;
			dragx = x; dragy = y;
			requestRepaint();
			return 0;
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
	return 0;
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

int getBottomBacklight() {
	return bottom_lcd_on;
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

static int set_help_button_on(int val, void *param)
{
	help_button_on = val ? 1 : 0;
	return 0;
}

static int set_menu_button_on(int val, void *param)
{
	menu_button_on = val ? 1 : 0;
	return 0;
}

static resource_string_t resources_string[] = {
	{ "SoftButtonPositions",
		" 0 0 64 0 128 0 192 0 256 0 0 60 64 60 128 60 192 60 256 60 0 120 64 120 128 120 192 120 256 120 0 180 64 180 128 180 192 180 256 180",
		RES_EVENT_NO, NULL,
		&soft_button_positions,
		soft_button_positions_set, NULL },
	RESOURCE_STRING_LIST_END
};

static const resource_int_t resources_int[] = {
    { "HelpButtonOn", 1, RES_EVENT_NO, (resource_value_t)1,
      &help_button_on, set_help_button_on, NULL },
    { "MenuButtonOn", 1, RES_EVENT_NO, (resource_value_t)1,
      &menu_button_on, set_menu_button_on, NULL },
    RESOURCE_INT_LIST_END
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

	// init gpusrc
	gpusrc = linearAlloc(512*256*4);
	menusrc = linearAlloc(512*256*4);

	if (resources_register_string(resources_string) < 0) {
		return -1;
	}
	if (resources_register_int(resources_int) < 0) {
        return -1;
    }

	// init icon hash
	tsh_init(&iconHash, HASHSIZE, (void (*)(void *))SDL_FreeSurface);

	return 0;
}

void uibottom_resources_shutdown() {
	lib_free(soft_button_positions);
	soft_button_positions=NULL;

	linearFree(gpusrc);
	gpusrc=NULL;
	linearFree(menusrc);
	menusrc=NULL;

	tsh_free(&iconHash);
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

void uibottom_unload_stuff() {
	// free and init the icon hash
	tsh_free(&iconHash);
	tsh_init(&iconHash, HASHSIZE, (void (*)(void *))SDL_FreeSurface);

	// images that are not always needed
	if (touchpad_img) SDL_FreeSurface(touchpad_img);
	touchpad_img=NULL;
	if (help_top_img) SDL_FreeSurface(help_top_img);
	help_top_img=NULL;
	if (help_bottom_img) SDL_FreeSurface(help_bottom_img);
	help_bottom_img=NULL;
}
