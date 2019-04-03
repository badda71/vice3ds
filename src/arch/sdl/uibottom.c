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
 //  x,  y,   w,   h, key, row, col, flg
 // F-Keys
 { 283, 32,  25,  16,   1,   0,   4,   0}, // F1 / F2
 { 283, 48,  25,  16,   2,   0,   5,   0}, // F3 / F4
 { 283, 64,  25,  16,   3,   0,   6,   0}, // F4 / F5
 { 283, 80,  25,  17,   4,   0,   3,   0}, // F6 / F7
 // top row	
 {  16, 32,  16,  16,   5,   7,   1,   0}, // <-
 {  32, 32,  16,  16,   6,   7,   0,   0}, // 1 / !
 {  48, 32,  16,  16,   7,   7,   3,   0}, // 2 / "
 {  64, 32,  16,  16,   8,   1,   0,   0}, // 3 / #
 {  80, 32,  16,  16,   9,   1,   3,   0}, // 4 / $
 {  96, 32,  16,  16,  10,   2,   0,   0}, // 5 / %
 { 112, 32,  16,  16,  11,   2,   3,   0}, // 6 / &
 { 128, 32,  16,  16,  12,   3,   0,   0}, // 7 / '
 { 144, 32,  16,  16,  13,   3,   3,   0}, // 8 / (
 { 160, 32,  16,  16,  14,   4,   0,   0}, // 9 / )
 { 176, 32,  16,  16,  15,   4,   3,   0}, // 0 / 0
 { 192, 32,  16,  16,  16,   5,   0,   0}, // + / +
 { 208, 32,  16,  16,  17,   5,   3,   0}, // - / |
 { 224, 32,  16,  16,  18,   6,   0,   0}, // Pound / ..
 { 240, 32,  16,  16,  19,   6,   3,   0}, // CLR/HOME
 { 256, 32,  17,  16,  20,   0,   0,   0}, // INST/DEL
 // 2nd row	
 {  16, 48,  24,  16,  21,   7,   2,   1}, // CTRL - sticky ctrl
 {  40, 48,  16,  16,  22,   7,   6,   0}, // Q
 {  56, 48,  16,  16,  23,   1,   1,   0}, // W
 {  72, 48,  16,  16,  24,   1,   6,   0}, // E
 {  88, 48,  16,  16,  25,   2,   1,   0}, // R
 { 104, 48,  16,  16,  26,   2,   6,   0}, // T
 { 120, 48,  16,  16,  27,   3,   1,   0}, // Y
 { 136, 48,  16,  16,  28,   3,   6,   0}, // U
 { 152, 48,  16,  16,  29,   4,   1,   0}, // I
 { 168, 48,  16,  16,  30,   4,   6,   0}, // O
 { 184, 48,  16,  16,  31,   5,   1,   0}, // P
 { 200, 48,  16,  16,  32,   5,   6,   0}, // @
 { 216, 48,  16,  16,  33,   6,   1,   0}, // *
 { 232, 48,  16,  16,  34,   6,   6,   0}, // ^| / π
 { 248, 48,  25,  16,  35,  -3,   0,   0}, // RESTORE
 // 3rd row	
 {  16, 64,  16,  16,  36,   7,   7,   0}, // RUN/STOP
 {  32, 64,  16,  16,  37,   1,   7,   2}, // SHIFT LOCK - sticky shift
 {  48, 64,  16,  16,  38,   1,   2,   0}, // A
 {  64, 64,  16,  16,  39,   1,   5,   0}, // S
 {  80, 64,  16,  16,  40,   2,   2,   0}, // D
 {  96, 64,  16,  16,  41,   2,   5,   0}, // F
 { 112, 64,  16,  16,  42,   3,   2,   0}, // G
 { 128, 64,  16,  16,  43,   3,   5,   0}, // H
 { 144, 64,  16,  16,  44,   4,   2,   0}, // J
 { 160, 64,  16,  16,  45,   4,   5,   0}, // K
 { 176, 64,  16,  16,  46,   5,   2,   0}, // L
 { 192, 64,  16,  16,  47,   5,   5,   0}, // : / [
 { 208, 64,  16,  16,  48,   6,   2,   0}, // ; / ]
 { 224, 64,  16,  16,  49,   6,   5,   0}, // =
 { 240, 64,  33,  16,  50,   0,   1,   0}, // RETURN
 // 4th row	
 {  16, 80,  16,  17,  51,   7,   5,   3}, // cbm - sticky cbm
 {  32, 80,  24,  17,  52,   1,   7,   2}, // SHIFT - sticky shift
 {  56, 80,  16,  17,  53,   1,   4,   0}, // Z
 {  72, 80,  16,  17,  54,   2,   7,   0}, // X
 {  88, 80,  16,  17,  55,   2,   4,   0}, // C
 { 104, 80,  16,  17,  56,   3,   7,   0}, // V
 { 120, 80,  16,  17,  57,   3,   4,   0}, // B
 { 136, 80,  16,  17,  58,   4,   7,   0}, // N
 { 152, 80,  16,  17,  59,   4,   4,   0}, // M
 { 168, 80,  16,  17,  60,   5,   7,   0}, // ,
 { 184, 80,  16,  17,  61,   5,   4,   0}, // .
 { 200, 80,  16,  17,  62,   6,   7,   0}, // /
 { 216, 80,  24,  17,  63,   6,   4,   2}, // SHIFT - sticky shift
 { 240, 80,  16,  17,  64,   0,   7,   0}, // UP / DOWN
 { 256, 80,  17,  17,  65,   0,   2,   0}, // LEFT / RIGHT
 // SPACE	
 {  63, 97, 147,  16,  66,   7,   4,   0}  // SPACE
};

static SDL_Surface *kbd_img=NULL;
int uibottom_kbdactive = 0;

static int kb_x_pos = 0;
static int kb_y_pos = 0;
static int kb_activekey;

static void negativeRect (SDL_Surface *, int, int, int, int);

void sdl_uibottom_draw(void)
{	
	// init the 3DS bottom screen
	// keyboard image
	SDL_Surface *s=sdl_active_canvas->screen;
	
	SDL_FillRect(s, &(SDL_Rect){ .x = 0, .y = 241, .w = s->w, .h=s->h-240}, SDL_MapRGB(s->format, 0x30, 0x30, 0x30));
	if (kbd_img == NULL) {
		char *fname = archdep_join_paths(archdep_user_config_path(),"ui_keyboard.bmp",NULL);
		kbd_img = SDL_LoadBMP(fname);
		lib_free(fname);
	}
	kb_x_pos = (s->w - uikbd_pos[0][2]) / 2;
	kb_y_pos = s->h - uikbd_pos[0][3];
	
	SDL_Rect rdest = {
		.x = kb_x_pos,
		.y = kb_y_pos};
	SDL_Rect rsrc = {
		.x = uikbd_pos[0][0],
		.y = uikbd_pos[0][1],
		.w = uikbd_pos[0][2],
		.h = uikbd_pos[0][3]};

	SDL_BlitSurface(kbd_img, &rsrc, s, &rdest);
	SDL_Flip(s);
}
/*
int uibottom_mouseevent(SDL_Event *e) {

	int x = (x=e->button.x * BOTTOMSCREEN_W) / uibottom_scr->w;
	int y = (y=e->button.y * BOTTOMSCREEN_H) / uibottom_scr->h;

	if (uibottom_kbdactive) {
		int i;
		// check which button was pressed
		for (i = 0; i < KBD_NUMKEYS; i++) {
			if (x >= uikbd_keypos[i].x + kb_x_pos &&
				x <  uikbd_keypos[i].x + uikbd_keypos[i].w + kb_x_pos &&
				y >= uikbd_keypos[i].y + kb_y_pos &&
				y <  uikbd_keypos[i].y + uikbd_keypos[i].h + kb_y_pos) break;
		}		
		if (i < KBD_NUMKEYS) { // got a hit
			if (e->button.type == SDL_MOUSEBUTTONDOWN) {
				keyboard_set_keyarr_any(uikbd_keypos[kb_activekey=i].row, uikbd_keypos[i].col, 1);
				negativeRect(uibottom_scr, uikbd_keypos[i].x, uikbd_keypos[i].y, uikbd_keypos[i].w, uikbd_keypos[i].h);
			} else {
				keyboard_set_keyarr_any(uikbd_keypos[kb_activekey].row, uikbd_keypos[kb_activekey].col, 0);
			}
		}
	}
	mouse_button((int)(e->button.button), (e->button.state == SDL_PRESSED));
}


static void negativeRect (SDL_Surface *s, int x, int y, int w, int h) {
	for (int yy = y; yy < y+h; yy++)
	{
		for (int xx = x; xx < x+w; xx++)
		{
			unsigned char *pixel = s->pixels + s->format->BytesPerPixel * (yy * s->w + xx);  
			pixel[0] = 255 - pixel[0];
			pixel[1] = 255 - pixel[1];
			pixel[2] = 255 - pixel[2];
		}
	}
	SDL_Flip(s);
}
*/
/*
#include "vice.h"

#include <stdio.h>

#include "joy.h"
#include "kbd.h"
#include "keyboard.h"
#include "menu_common.h"
#include "types.h"
#include "ui.h"
#include "uimenu.h"
#include "uipoll.h"
#include "videoarch.h"
#include "vkbd.h"

int sdl_vkbd_state = 0;


static int vkbd_pos_x = 0;
static int vkbd_pos_y = 0;
static int vkbd_pos_max_x = 0;
static int vkbd_pos_max_y = 0;

static vkbd_t *vkbd = NULL;
static int vkbd_w = 0;
static int vkbd_h = 0;

static int vkbd_x = 0;
static int vkbd_y = 0;

static int vkbd_move = 0;

static int vkbd_shiftflags;

static UI_MENU_CALLBACK(custom_shift_callback)
{
    int flag = (1 << (vice_ptr_to_int(param)));

    if (activated) {
        vkbd_shiftflags ^= flag;
    } else {
        if (vkbd_shiftflags & flag) {
            return sdl_menu_text_tick;
        }
    }
    return NULL;
}

static const ui_menu_entry_t define_shift_options_menu[] = {
    { "Virtual shift",
      MENU_ENTRY_OTHER,
      custom_shift_callback,
      (ui_callback_data_t)0 },
    { "Left shift",
      MENU_ENTRY_OTHER,
      custom_shift_callback,
      (ui_callback_data_t)1 },
    { "Right shift",
      MENU_ENTRY_OTHER,
      custom_shift_callback,
      (ui_callback_data_t)2 },
    { "Allow shift",
      MENU_ENTRY_OTHER,
      custom_shift_callback,
      (ui_callback_data_t)3 },
    { "Deshift shift",
      MENU_ENTRY_OTHER,
      custom_shift_callback,
      (ui_callback_data_t)4 },
    { "Allow other",
      MENU_ENTRY_OTHER,
      custom_shift_callback,
      (ui_callback_data_t)5 },
    { "Alt map",
      MENU_ENTRY_OTHER,
      custom_shift_callback,
      (ui_callback_data_t)8 },
    SDL_MENU_LIST_END
};

static const ui_menu_entry_t shift_menu[] = {
    { "Define shift options",
      MENU_ENTRY_SUBMENU,
      submenu_radio_callback,
      (ui_callback_data_t)define_shift_options_menu },
    SDL_MENU_LIST_END
};

#define VKBD_COMMAND_MOVE  0xff
#define VKBD_COMMAND_CLOSE 0xfe

static void sdl_vkbd_key_press(int value, int shift)
{
    int mr, mc, neg;
    uint8_t b, sb;

    b = vkbd->keytable[vkbd_x + vkbd_y * vkbd_w];

    if ((b == VKBD_COMMAND_CLOSE) && (value)) {
        sdl_vkbd_close();
        return;
    }

    if (b == VKBD_COMMAND_MOVE) {
        if (value && shift) {
            sdl_vkbd_close();
        } else {
            vkbd_move = value;
        }
        return;
    }

    neg = b & 0x08;

    if (shift && !neg) {
        sb = vkbd->shift;
        mc = (int)(sb & 0xf);
        mr = (int)((sb >> 4) & 0xf);
        keyboard_set_keyarr(mr, mc, value);
log_debug("keyboard_set_keyarr %d %d %d",mr,mc,value);
    }

    mc = (int)(b & 0x7);
    mr = (int)((b >> 4) & 0xf);

    if (neg) {
        mr = -mr;
    }

    keyboard_set_keyarr_any(mr, mc, value);
log_debug("keyboard_set_keyarr_any %d %d %d",mr,mc,value);
}

static void sdl_vkbd_key_map(void)
{
    int mr, mc, neg, i, j;
    uint8_t b;
    SDL_Event e;
    int unmap = 0;
    char keyname[10];

    b = vkbd->keytable[vkbd_x + vkbd_y * vkbd_w];

    if (b == VKBD_COMMAND_CLOSE) {
        return;
    }
    sdl_vkbd_state &= SDL_VKBD_REPAINT;

    sdl_ui_activate_pre_action();

    // Use blank for unmapping 
    if (b == VKBD_COMMAND_MOVE) {
        unmap = 1;
    } else {
        // get the key name for displaying 
        for (j = vkbd_x; (j > -1) && (vkbd->keytable[j + vkbd_y * vkbd_w] == b); --j) {
        }
        ++j;

        for (i = 0; ((i + j) < vkbd_w) && (vkbd->keytable[i + j + vkbd_y * vkbd_w] == b); ++i) {
            keyname[i] = vkbd->keyb[vkbd_y][i + j];
        }

        keyname[i] = 0;
    }

    neg = b & 0x08;
    mc = (int)(b & 0x7);
    mr = (int)((b >> 4) & 0xf);

    if (neg) {
        mr = -mr;
    }

    e = sdl_ui_poll_event("key or joystick event", unmap ? "(unmap)" : keyname, SDL_POLL_KEYBOARD | SDL_POLL_MODIFIER | SDL_POLL_JOYSTICK, 5);

    // TODO check if key/event is suitable 
    switch (e.type) {
        case SDL_KEYDOWN:
            if (!unmap) {
                vkbd_shiftflags = (1 << 3);
                sdl_ui_external_menu_activate((ui_menu_entry_t *)shift_menu);
                keyboard_set_map_any((signed long)SDL2x_to_SDL1x_Keys(e.key.keysym.sym), mr, mc, vkbd_shiftflags);
            } else {
                keyboard_set_unmap_any((signed long)SDL2x_to_SDL1x_Keys(e.key.keysym.sym));
            }
            break;
#ifdef HAVE_SDL_NUMJOYSTICKS
        case SDL_JOYAXISMOTION:
        case SDL_JOYBUTTONDOWN:
        case SDL_JOYHATMOTION:
            if (unmap) {
                sdljoy_unset(e);
            } else {
                sdljoy_set_keypress(e, mr, mc);
            }
            break;
#endif
        default:
            break;
    }

    sdl_ui_activate_post_action();
    sdl_vkbd_state = SDL_VKBD_ACTIVE;
}

static inline void sdl_vkbd_move(int *var, int amount, int min, int max)
{
    *var += amount;

    if (*var < min) {
        *var = max - 1;
    } else if (*var >= max) {
        *var = min;
    }

    sdl_vkbd_state |= SDL_VKBD_REPAINT;
}

// ------------------------------------------------------------------ 
// External interface 

void sdl_vkbd_set_vkbd(const vkbd_t *machine_vkbd)
{
    vkbd = (vkbd_t *)machine_vkbd;

    if (vkbd == NULL) {
        return;
    }

    for (vkbd_h = 0; vkbd->keyb[vkbd_h] != NULL; ++vkbd_h) {
    }

    if (vkbd_h > 0) {
        for (vkbd_w = 0; vkbd->keyb[0][vkbd_w] != 0; ++vkbd_w) {
        }
    }
}

void sdl_vkbd_activate(void)
{
    menu_draw_t *limits = NULL;

    if (vkbd == NULL) {
        return;
    }

    sdl_ui_init_draw_params();
    limits = sdl_ui_get_menu_param();

    vkbd_pos_max_x = limits->max_text_x - vkbd_w + 1;
    vkbd_pos_max_y = limits->max_text_y - vkbd_h + 1;

    if (vkbd_pos_x >= vkbd_pos_max_x) {
        vkbd_pos_x = vkbd_pos_max_x - 1;
    }

    if (vkbd_pos_y >= vkbd_pos_max_y) {
        vkbd_pos_y = vkbd_pos_max_y - 1;
    }

    sdl_vkbd_state = SDL_VKBD_ACTIVE | SDL_VKBD_REPAINT;
    vkbd_move = 0;
}

void sdl_vkbd_close(void)
{
    keyboard_key_clear();
    sdl_vkbd_state = SDL_VKBD_REPAINT;
}

void sdl_vkbd_draw(void)
{
    int i;
    sdl_ui_set_active_font(MENU_FONT_MONITOR);

    for (i = 0; i < vkbd_h; ++i) {
        sdl_ui_print(vkbd->keyb[i], vkbd_pos_x, vkbd_pos_y + i);
    }

    sdl_ui_invert_char(vkbd_pos_x + vkbd_x, vkbd_pos_y + vkbd_y);
    
    sdl_ui_set_active_font(MENU_FONT_ASCII);
    
}

int sdl_vkbd_process(ui_menu_action_t input)
{
    int retval = 1;

    switch (input) {
        case MENU_ACTION_UP:
            if (vkbd_move) {
                sdl_vkbd_move(&vkbd_pos_y, -1, 0, vkbd_pos_max_y);
            } else {
                sdl_vkbd_move(&vkbd_y, -1, 0, vkbd_h);
            }
            break;
        case MENU_ACTION_DOWN:
            if (vkbd_move) {
                sdl_vkbd_move(&vkbd_pos_y, 1, 0, vkbd_pos_max_y);
            } else {
                sdl_vkbd_move(&vkbd_y, 1, 0, vkbd_h);
            }
            break;
        case MENU_ACTION_LEFT:
            if (vkbd_move) {
                sdl_vkbd_move(&vkbd_pos_x, -1, 0, vkbd_pos_max_x);
            } else {
                sdl_vkbd_move(&vkbd_x, -1, 0, vkbd_w);
            }
            break;
        case MENU_ACTION_RIGHT:
            if (vkbd_move) {
                sdl_vkbd_move(&vkbd_pos_x, 1, 0, vkbd_pos_max_x);
            } else {
                sdl_vkbd_move(&vkbd_x, 1, 0, vkbd_w);
            }
            break;
        case MENU_ACTION_SELECT:
            sdl_vkbd_key_press(1, 0);
            break;
        case MENU_ACTION_SELECT_RELEASE:
            sdl_vkbd_key_press(0, 0);
            break;
        case MENU_ACTION_CANCEL:
            sdl_vkbd_key_press(1, 1);
            break;
        case MENU_ACTION_CANCEL_RELEASE:
            sdl_vkbd_key_press(0, 1);
            break;
        case MENU_ACTION_MAP:
            sdl_vkbd_key_map();
            retval = 0;
            break;
        case MENU_ACTION_EXIT:
            sdl_vkbd_close();
            retval = 0;
            break;
        default:
            retval = 0;
            break;
    }

    return retval;
}

// ------------------------------------------------------------------ 
// virtual keyboards 

static const char *keyb_c64[] = {
    "X \x5f 1234567890+-\x5ch del  F1",
    "ctrl QWERTYUIOP@*\x5e rstr F3",
    "r/s   ASDFGHJKL:;= rtrn F5",
    "c= sh  ZXCVBNM,./v> sh  F7",
    "        space             ",
    NULL
};

static const uint8_t keytable_c64[] =
    "\xfe\xff\x71\xff\x70\x73\x10\x13\x20\x23\x30\x33\x40\x43\x50\x53\x60\x63\xff\x00\x00\x00\xff\xff\x04\x04"
    "\x72\x72\x72\x72\xff\x76\x11\x16\x21\x26\x31\x36\x41\x46\x51\x56\x61\x66\xff\x38\x38\x38\x38\xff\x05\x05"
    "\x77\x77\x77\xff\xff\xff\x12\x15\x22\x25\x32\x35\x42\x45\x52\x55\x62\x65\xff\x01\x01\x01\x01\xff\x06\x06"
    "\x75\x75\xff\x17\x17\xff\xff\x14\x27\x24\x37\x34\x47\x44\x57\x54\x67\x07\x02\xff\x64\x64\xff\xff\x03\x03"
    "\xff\xff\xff\xff\xff\xff\xff\xff\x74\x74\x74\x74\x74\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff";

static const uint8_t keytable_vic20[] =
    "\xfe\xff\x01\xff\x00\x07\x10\x17\x20\x27\x30\x37\x40\x47\x50\x57\x60\x67\xff\x70\x70\x70\xff\xff\x74\x74"
    "\x02\x02\x02\x02\xff\x06\x11\x16\x21\x26\x31\x36\x41\x46\x51\x56\x61\x66\xff\x38\x38\x38\x38\xff\x75\x75"
    "\x03\x03\x03\xff\xff\xff\x12\x15\x22\x25\x32\x35\x42\x45\x52\x55\x62\x65\xff\x71\x71\x71\x71\xff\x76\x76"
    "\x05\x05\xff\x13\x13\xff\xff\x14\x23\x24\x33\x34\x43\x44\x53\x54\x63\x73\x72\xff\x64\x64\xff\xff\x77\x77"
    "\xff\xff\xff\xff\xff\xff\xff\xff\x04\x04\x04\x04\x04\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff";

vkbd_t vkbd_c64 = {
    keyb_c64,
    keytable_c64,
    '\x17'
};

vkbd_t vkbd_vic20 = {
    keyb_c64,
    keytable_vic20,
    '\x13'
};

static const char *keyb_c64dtv[] = {
    "X \x5f 1234567890+-\x5ch del  F1",
    "ctrl QWERTYUIOP@*\x5e rstr F3",
    "r/s   ASDFGHJKL:;= rtrn F5",
    "c= sh  ZXCVBNM,./v> sh  F7",
    "        space      ABCD R ",
    NULL
};

static const uint8_t keytable_c64dtv[] =
    "\xfe\xff\x71\xff\x70\x73\x10\x13\x20\x23\x30\x33\x40\x43\x50\x53\x60\x63\xff\x00\x00\x00\xff\xff\x04\x04"
    "\x72\x72\x72\x72\xff\x76\x11\x16\x21\x26\x31\x36\x41\x46\x51\x56\x61\x66\xff\x38\x38\x38\x38\xff\x05\x05"
    "\x77\x77\x77\xff\xff\xff\x12\x15\x22\x25\x32\x35\x42\x45\x52\x55\x62\x65\xff\x01\x01\x01\x01\xff\x06\x06"
    "\x75\x75\xff\x17\x17\xff\xff\x14\x27\x24\x37\x34\x47\x44\x57\x54\x67\x07\x02\xff\x64\x64\xff\xff\x03\x03"
    "\xff\xff\xff\xff\xff\xff\xff\xff\x74\x74\x74\x74\x74\xff\xff\xff\xff\xff\xff\x04\x00\x01\x02\xff\x03\xff";

vkbd_t vkbd_c64dtv = {
    keyb_c64dtv,
    keytable_c64dtv,
    '\x17'
};

static const char *keyb_c128[] = {
    "X etac hldn \x5ev<> f1 f3 f5 f7",
    "  \x5f 1234567890+-\x5ch  del 789+",
    "ctrl QWERTYUIOP@*\x5e rstr 456-",
    "r/s   ASDFGHJKL:;= rtrn 123e",
    "c= sh  ZXCVBNM,./v> sh  0 .e",
    "         space              ",
    NULL
};

static const uint8_t keytable_c128[] =
    "\xfe\xff\x90\x83\xa0\x49\xff\x80\x93\x48\xa7\xff\xa3\xa4\xa5\xa6\xff\x04\x04\xff\x05\x05\xff\x06\x06\xff\x03\x03"
    "\xff\xff\x71\xff\x70\x73\x10\x13\x20\x23\x30\x33\x40\x43\x50\x53\x60\x63\xff\xff\x00\x00\x00\xff\x86\x81\x96\x91"
    "\x72\x72\x72\x72\xff\x76\x11\x16\x21\x26\x31\x36\x41\x46\x51\x56\x61\x66\xff\x38\x38\x38\x38\xff\x85\x82\x95\x92"
    "\x77\x77\x77\xff\xff\xff\x12\x15\x22\x25\x32\x35\x42\x45\x52\x55\x62\x65\xff\x01\x01\x01\x01\xff\x87\x84\x97\x94"
    "\x75\x75\xff\x17\x17\xff\xff\x14\x27\x24\x37\x34\x47\x44\x57\x54\x67\x07\x02\xff\x64\x64\xff\xff\xa1\xff\xa2\x94"
    "\xff\xff\xff\xff\xff\xff\xff\xff\xff\x74\x74\x74\x74\x74\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff";

vkbd_t vkbd_c128 = {
    keyb_c128,
    keytable_c128,
    '\x17'
};

static const char *keyb_plus4[] = {
    "X  F1 F2 F3 Help       ",
    "esc 1234567890+-=h  del",
    "ctrl QWERTYUIOP@\x5c* ctrl",
    "rs sh ASDFGHJKL:; rtrn ",
    "c=  sh ZXCVBNM,./ sh \x5e ",
    "         space      \x5fv>",
    NULL
};

static const uint8_t keytable_plus4[] =
    "\xfe\xff\xff\x04\x04\xff\x05\x05\xff\x06\x06\xff\x03\x03\x03\x03\xff\xff\xff\xff\xff\xff\xff"
    "\x64\x64\x64\xff\x70\x73\x10\x13\x20\x23\x30\x33\x40\x43\x66\x56\x65\x71\xff\xff\x00\x00\x00"
    "\x72\x72\x72\x72\xff\x76\x11\x16\x21\x26\x31\x36\x41\x46\x51\x07\x02\x61\xff\x72\x72\x72\x72"
    "\x77\x77\xff\x17\x17\xff\x12\x15\x22\x25\x32\x35\x42\x45\x52\x55\x62\xff\x01\x01\x01\x01\xff"
    "\x75\x75\xff\xff\x17\x17\xff\x14\x27\x24\x37\x34\x47\x44\x57\x54\x67\xff\x17\x17\xff\x53\xff"
    "\xff\xff\xff\xff\xff\xff\xff\xff\xff\x74\x74\x74\x74\x74\xff\xff\xff\xff\xff\xff\x60\x50\x63";

vkbd_t vkbd_plus4 = {
    keyb_plus4,
    keytable_plus4,
    '\x17'
};

static const char *keyb_cbm2[] = {
    "X f1234567890 v\x5e<> conr/s",
    "esc 1234567890-=\x5ci/d ?c",
    "tab  QWERTYUIOP()rtn 789-",
    "shift ASDFGHJKL;'_rt 456+",
    " shift ZXCVBNM,./sc= 123e",
    "    ctrl space       0.0e",
    NULL
};

static const uint8_t keytable_cbm2[] =
    "\xfe\xff\x80\x80\x90\xa0\xb0\xc0\xd0\xe0\xf0\x00\x10\xff\x20\x30\x31\x32\xff\x40\x50\x60\x70\x70\x70"
    "\x70\x70\x70\xff\x91\xa1\xb1\xc1\xd1\xd2\xe1\xf1\x01\x11\x12\x21\x22\x33\x33\x33\xff\x41\x51\x61\x71"
    "\x82\x82\x82\xff\xff\x92\xa2\xb2\xc2\xc3\xd3\xe2\xf2\x02\x13\x14\x23\x24\x24\x24\xff\x42\x52\x62\x72"
    "\x84\x84\x84\x84\x84\xff\x93\xa3\xb3\xb4\xc4\xd4\xe3\xf3\x03\x04\x15\x25\x24\x24\xff\x43\x53\x63\x73"
    "\xff\x84\x84\x84\x84\x84\xff\x94\xa4\xa5\xb5\xc5\xd5\xe4\xf4\xf5\x05\x83\x34\x34\xff\x44\x54\x64\x74"
    "\xff\xff\xff\xff\x85\x85\x85\x85\xff\xe5\xe5\xe5\xe5\xe5\xff\xff\xff\xff\xff\xff\xff\x45\x55\x65\x74";

vkbd_t vkbd_cbm2 = {
    keyb_cbm2,
    keytable_cbm2,
    '\x84'
};

// FIXME: support all PET keyboards (see pet-resources.h) 

static const char *keyb_pet_uk[] = {
    "X \x5f 1234567890:-\x5e> r/s  789",
    "tab QWERTYUIOP\x5b\\v del   456",
    "esc ASDFGHJKL;@\x5d  rtrn  123",
    "rvs sh ZXCVBNM,./ sh rh 0 .",
    "        space              ",
    NULL
};

static const uint8_t keytable_pet_uk[] =
    "\xfe\xff\x90\xff\x10\x00\x91\x11\x01\x92\x12\x02\x93\x13\x95\x03\x15\x05\xff\x20\x20\x20\xff\xff\x14\x04\x17"
    "\x40\x40\x40\xff\x50\x41\x51\x42\x52\x43\x53\x45\x55\x46\x56\x44\x54\xff\x47\x47\x47\xff\xff\xff\x57\x27\x37"
    "\x20\x20\x20\xff\x30\x21\x31\x22\x32\x23\x33\x25\x35\x26\x36\x24\xff\xff\x34\x34\x34\x34\xff\xff\x87\x77\x67"
    "\x80\x80\x80\xff\x60\x60\xff\x70\x81\x61\x71\x62\x72\x83\x73\x63\x86\xff\x66\x66\xff\x76\x84\xff\x74\xff\x64"
    "\xff\xff\xff\xff\xff\xff\xff\xff\x74\x74\x74\x74\x74\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff";

vkbd_t vkbd_pet_uk = {
    keyb_pet_uk,
    keytable_pet_uk,
    '\x60'
};

static const char *keyb_pet_gr[] = {
    "X   @!\"#$%'&\\()\x5f[]    hv<i",
    "rvs QWERTYUIOP\x5e<>     789/",
    "    ASDFGHJKL: rs rtn 456*",
    " sh ZXCVBNM,;? sh     123+",
    "        space         0.-=",
    NULL
};

static const uint8_t keytable_pet_gr[] =
    "\xfe\xff\xff\xff\x81\x00\x10\x01\x11\x02\x12\x03\x13\x04\x14\x05\x91\x82\xff\xff\xff\xff\x06\x16\x07\x18"
    "\x90\x90\x90\xff\x20\x30\x21\x31\x22\x32\x23\x33\x24\x34\x25\x93\x84\xff\xff\xff\xff\xff\x26\x36\x27\x37"
    "\xff\xff\xff\xff\x40\x50\x41\x51\x42\x52\x43\x53\x44\x54\xff\x94\x94\xff\x65\x65\x65\xff\x46\x56\x47\xff"
    "\xff\x80\x80\xff\x60\x70\x61\x71\x62\x72\x63\x73\x64\x74\xff\x85\x85\xff\xff\xff\xff\xff\x66\x76\x67\x77"
    "\xff\xff\xff\xff\xff\xff\xff\xff\x92\x92\x92\x92\x92\xff\xff\xff\xff\xff\xff\xff\xff\xff\x86\x96\x87\x97";

vkbd_t vkbd_pet_gr = {
    keyb_pet_gr,
    keytable_pet_gr,
    '\x80'
};
*/