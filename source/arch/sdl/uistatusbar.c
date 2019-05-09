/*
 * uistatusbar.c - SDL statusbar.
 *
 * Written by
 *  Hannu Nuotio <hannu.nuotio@tut.fi>
 *
 * Based on code by
 *  Andreas Boose <viceteam@t-online.de>
 *
 * This file is part of VICE, the Versatile Commodore Emulator.
 * See README for copyright notice.
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

#include "vice.h"

#include <stdio.h>

#include "kbd.h"
#include "resources.h"
#include "types.h"
#include "ui.h"
#include "uiapi.h"
#include "uimenu.h"
#include "uistatusbar.h"
#include "videoarch.h"
#include "log.h"
#include "3ds_led.h"

/* ----------------------------------------------------------------- */
/* static functions/variables */

#ifndef MIN
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#endif

#define MAX_STATUSBAR_LEN           128
#define STATUSBAR_SPEED_POS         0
#define STATUSBAR_PAUSE_POS         4
#define STATUSBAR_DRIVE_POS         12
#define STATUSBAR_DRIVE8_TRACK_POS  14
#define STATUSBAR_DRIVE9_TRACK_POS  19
#define STATUSBAR_DRIVE10_TRACK_POS 25
#define STATUSBAR_DRIVE11_TRACK_POS 31
#define STATUSBAR_TAPE_POS          37

static char statusbar_text[MAX_STATUSBAR_LEN] = "                                       ";
static char kbdstatusbar_text[MAX_STATUSBAR_LEN] = "                                       ";

static menufont_t *menufont = NULL;

static int tape_counter = 0;
static int tape_enabled = 0;
static int tape_motor = 0;
static int tape_control = 0;

static void display_tape(void)
{
    int len;

    if (tape_enabled) {
        len = sprintf(&(statusbar_text[STATUSBAR_TAPE_POS]), "%c%03d%c", (tape_motor) ? '*' : ' ', tape_counter, " >f<R"[tape_control]);
    } else {
        len = sprintf(&(statusbar_text[STATUSBAR_TAPE_POS]), "     ");
    }
    statusbar_text[STATUSBAR_TAPE_POS + len] = ' ';

    if (sdl_statusbar) {
        uistatusbar_must_redraw=1;
    }
}

static int per = 0;
static int fps = 0;
static int warp = 0;
static int paused = 0;

static void display_speed(void)
{
    int len;
    unsigned char sep = paused ? ('P' | 0x80) : warp ? ('W' | 0x80) : '/';

    len = sprintf(&(statusbar_text[STATUSBAR_SPEED_POS]), "%3d%%%c%2dfps", per, sep, fps);
    statusbar_text[STATUSBAR_SPEED_POS + len] = ' ';

    if (sdl_statusbar) {
        uistatusbar_must_redraw=1;
    }
}

/* ----------------------------------------------------------------- */
/* ui.h */

void ui_display_speed(float percent, float framerate, int warp_flag)
{
    per = (int)(percent + .5);
    if (per > 999) {
        per = 999;
    }

    fps = (int)(framerate + .5);
    if (fps > 99) {
        fps = 99;
    }

    warp = warp_flag;

    display_speed();
}

void ui_display_paused(int flag)
{
    paused = flag;

    display_speed();
}

/* ----------------------------------------------------------------- */
/* uiapi.h */

/* Display a mesage without interrupting emulation */
void ui_display_statustext(const char *text, int fade_out)
{
#ifdef SDL_DEBUG
    fprintf(stderr, "%s: \"%s\", %i\n", __func__, text, fade_out);
#endif
}

/* Drive related UI.  */
void ui_enable_drive_status(ui_drive_enable_t state, int *drive_led_color)
{
    int drive_number;
    int drive_state = (int)state;
    size_t offset;  /* offset in status text */
    size_t size;    /* number of bytes to bleep out */

    for (drive_number = 0; drive_number < 4; ++drive_number) {
        if (drive_state & 1) {
            ui_display_drive_led(drive_number, 0, 0);
        } else {
            switch (drive_number) {
                case 0:
                    offset = STATUSBAR_DRIVE8_TRACK_POS - 2;
                    size = 4;
                    break;
                case 1:
                    offset = STATUSBAR_DRIVE9_TRACK_POS - 2;
                    size = 4;
                    break;
                case 2:
                    offset = STATUSBAR_DRIVE10_TRACK_POS - 3;
                    size = 5;
                    break;
                case 3:
                    offset = STATUSBAR_DRIVE11_TRACK_POS - 3;
                    size = 5;
                    break;
                default:
                    /* should never get here */
                    offset = 0;
                    size = 40;
                    break;
            }
            memset(statusbar_text + offset, 0x20, size);    /* space out man */
        }
        drive_state >>= 1;
    }

    if (sdl_statusbar) {
        uistatusbar_must_redraw=1;
    }
}

void ui_display_drive_track(unsigned int drive_number, unsigned int drive_base, unsigned int half_track_number)
{
    unsigned int track_number = half_track_number / 2;

#ifdef SDL_DEBUG
    fprintf(stderr, "%s\n", __func__);
#endif

    switch (drive_number) {
        case 1:
            statusbar_text[STATUSBAR_DRIVE9_TRACK_POS] = (track_number / 10) + '0';
            statusbar_text[STATUSBAR_DRIVE9_TRACK_POS + 1] = (track_number % 10) + '0';
            break;
        case 2:
            statusbar_text[STATUSBAR_DRIVE10_TRACK_POS] = (track_number / 10) + '0';
            statusbar_text[STATUSBAR_DRIVE10_TRACK_POS + 1] = (track_number % 10) + '0';
            break;
        case 3:
            statusbar_text[STATUSBAR_DRIVE11_TRACK_POS] = (track_number / 10) + '0';
            statusbar_text[STATUSBAR_DRIVE11_TRACK_POS + 1] = (track_number % 10) + '0';
            break;
        default:
        case 0:
            statusbar_text[STATUSBAR_DRIVE8_TRACK_POS] = (track_number / 10) + '0';
            statusbar_text[STATUSBAR_DRIVE8_TRACK_POS + 1] = (track_number % 10) + '0';
            break;
    }

    if (sdl_statusbar) {
        uistatusbar_must_redraw=1;
    }
}

static int drive_led_status=0;

void ui_update_drive_led(void) {
	if (drive_led_status && drive_led) LED3DS_On(drive_led_brightness,0,0);
	else LED3DS_Off();
}

/* The pwm value will vary between 0 and 1000.  */
void ui_display_drive_led(int drive_number, unsigned int pwm1, unsigned int led_pwm2)
{
    int high;
    int low;
    int offset;

#ifdef SDL_DEBUG
    fprintf(stderr, "%s: drive %i, pwm1 = %i, led_pwm2 = %u\n", __func__, drive_number, pwm1, led_pwm2);
#endif

	if ((pwm1 > 500) != drive_led_status) {
		drive_led_status=(pwm1 > 500);
		ui_update_drive_led();
	}

    low = "8901"[drive_number] | ((pwm1 > 500) ? 0x80 : 0);
    high = '1' | ((pwm1 > 500) ? 0x80: 0);
    switch (drive_number) {
        case 0:
            offset = STATUSBAR_DRIVE8_TRACK_POS - 2;
            break;
        case 1:
            offset = STATUSBAR_DRIVE9_TRACK_POS - 2;
            break;
        case 2:
            offset = STATUSBAR_DRIVE10_TRACK_POS - 3;
            break;
        case 3:
            offset = STATUSBAR_DRIVE11_TRACK_POS - 3;
            break;
        default:
            offset = 0;
    }

    if (drive_number < 2) {
        statusbar_text[offset] = low;
        statusbar_text[offset + 1] = 'T';
    } else {

        statusbar_text[offset] = high;
        statusbar_text[offset + 1] = low;
        statusbar_text[offset + 2] = 'T';
    }


    if (sdl_statusbar) {
        uistatusbar_must_redraw=1;
    }
}

void ui_display_drive_current_image(unsigned int drive_number, const char *image)
{
#ifdef SDL_DEBUG
    fprintf(stderr, "%s\n", __func__);
#endif
}

/* Tape related UI */

void ui_set_tape_status(int tape_status)
{
    tape_enabled = tape_status;

    display_tape();
}

void ui_display_tape_motor_status(int motor)
{
    tape_motor = motor;

    display_tape();
}

void ui_display_tape_control_status(int control)
{
    tape_control = control;

    display_tape();
}

void ui_display_tape_counter(int counter)
{
    if (tape_counter != counter) {
        display_tape();
    }

    tape_counter = counter;
}

void ui_display_tape_current_image(const char *image)
{
#ifdef SDL_DEBUG
    fprintf(stderr, "%s: %s\n", __func__, image);
#endif
}

/* Recording UI */
void ui_display_playback(int playback_status, char *version)
{
#ifdef SDL_DEBUG
    fprintf(stderr, "%s: %i, \"%s\"\n", __func__, playback_status, version);
#endif
}

void ui_display_recording(int recording_status)
{
#ifdef SDL_DEBUG
    fprintf(stderr, "%s: %i\n", __func__, recording_status);
#endif
}

void ui_display_event_time(unsigned int current, unsigned int total)
{
#ifdef SDL_DEBUG
    fprintf(stderr, "%s: %i, %i\n", __func__, current, total);
#endif
}

/* Joystick UI */
void ui_display_joyport(uint8_t *joyport)
{
#ifdef SDL_DEBUG
    fprintf(stderr, "%s: %02x %02x %02x %02x %02x\n", __func__, joyport[0], joyport[1], joyport[2], joyport[3], joyport[4]);
#endif
}

/* Volume UI */
void ui_display_volume(int vol)
{
#ifdef SDL_DEBUG
    fprintf(stderr, "%s: %i\n", __func__, vol);
#endif
}


/* ----------------------------------------------------------------- */
/* uistatusbar.h */

int uistatusbar_must_redraw = 0;

#define KBDSTATUSENTRYLEN   15

// ypos of statusbar top
#define SB_Y	240

static inline void uistatusbar_putchar(uint8_t c, int pos_x, int pos_y, int color_f, int color_b)
{
    int x, y;
//    uint8_t fontchar;
    uint8_t *font_pos;
    uint8_t *offset;

	
	SDL_Surface *s = sdl_active_canvas->screen;
	int bpp = s->format->BytesPerPixel;
	
	offset = s->pixels + bpp * ((SB_Y + pos_y * menufont->h) * s->w + (s->w - 320)/2 + pos_x * menufont->w);
	font_pos = &(menufont->font[menufont->translate[(int)c]]);

	int col;
	for (y = 0; y < menufont->h; ++y) {
        for (x = 0; x < menufont->w; ++x) {
			col=(font_pos[y] & (0x80 >> x)) ? color_f : color_b;
			unsigned char *pixel = offset + bpp * (y * s->w + x);
			pixel[1]=(col & 0xff0000)>>16;
			pixel[2]=(col & 0xff00)>>8;
			pixel[3]=col & 0xff;
		}
	}
}

static void uistatusbar_updateRect()
{
	SDL_Surface *s = sdl_active_canvas->screen;
	SDL_UpdateRect(s, 0, SB_Y, s->w, menufont->h*(sdl_kbd_statusbar?2:1));
}


void uistatusbar_draw(void)
{
    int i, color_f, color_b;
    uint8_t c;
    menufont = sdl_ui_get_menu_font();

	if (sdl_statusbar && uistatusbar_must_redraw) {

		color_f = 0xffffff;
		color_b = 0x000000;

		for (i = 0; i < MAX_STATUSBAR_LEN; ++i) {
			c = statusbar_text[i];

			if (c == 0) {
				break;
			}

			if (c & 0x80) {
				uistatusbar_putchar((uint8_t)(c & 0x7f), i, 0, color_b, color_f);
			} else {
				uistatusbar_putchar(c, i, 0, color_f, color_b);
			}
		}

		if (sdl_kbd_statusbar) {
			for (i = 0; i < MAX_STATUSBAR_LEN; ++i) {
				c = kbdstatusbar_text[i];

				if (c == 0) {
					break;
				}

				if (((i / KBDSTATUSENTRYLEN) & 1) == 1) {
					uistatusbar_putchar(c, i, 1, color_b, color_f);
				} else {
					uistatusbar_putchar(c, i, 1, color_f, color_b);
				}
			}
		}

		uistatusbar_updateRect();
		uistatusbar_must_redraw = 0;
	}
}

void ui_display_kbd_status(SDL_Event *e)
{
    char *p = &kbdstatusbar_text[KBDSTATUSENTRYLEN * 2];

    if (sdl_kbd_statusbar) {
        memmove(kbdstatusbar_text, &kbdstatusbar_text[KBDSTATUSENTRYLEN], 40);
        sprintf(p, "%c%03d>%03d %c%04x    ", 
                (e->type == SDL_KEYUP) ? 'U' : 'D',
                e->key.keysym.sym & 0xffff, 
                SDL2x_to_SDL1x_Keys(e->key.keysym.sym),
                ((e->key.keysym.sym & 0xffff0000) == 0x40000000) ? 'M' : ((e->key.keysym.sym & 0xffff0000) != 0x00000000) ? 'E' : ' ',
                e->key.keysym.mod);
		
		uistatusbar_must_redraw=1;
	}
}

