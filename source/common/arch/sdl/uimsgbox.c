/*
 * uimsgbox.c - Common SDL message box functions.
 *
 * Written by
 *  Marco van den Heuvel <blackystardust68@yahoo.com>
 *  Greg King <greg.king5@verizon.net>
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

#include "vice_sdl.h"
#include "types.h"

#define assert(x)
#include <string.h>

#include "lib.h"
#include "resources.h"
#include "sound.h"
#include "ui.h"
#include "uibottom.h"
#include "uifonts.h"
#include "uimenu.h"
#include "uimsgbox.h"
#include "videoarch.h"
#include "vsync.h"

#define MAX_MSGBOX_LEN 28

/** \brief  Table of number of buttons for each `message mode` enum value
 */
static int msg_mode_buttons[] = {
    1,  /* MESSAGE_OK */
    2,  /* MESSAGE_YESNO */
    2,  /* MESSAGE_CPUJAM */
    5,  /* MESSAGE_UNIT_SELECT */
};

static int buttons_OK[] = {17,4};
static int buttons_YESNO[] = {23,5,11,4};
static int buttons_UNIT_SELECT[] = {27,3,23,3,19,4,14,4,9,6};
static int buttons_CPUJAM[] = {24,7,16,10};

static menu_draw_t *menu_draw;

/** \brief  Show a message box with some buttons
 *
 * \param[in]   title           Message box title
 * \param[in]   message         Message box message/prompt
 * \param[in]   message_mode    Type of message box to display (defined in .h)
 *
 * \return  index of selected button, or -1 on cancel (Esc)
 */
static int handle_message_box(const char *title, const char *message, int message_mode)
{
    char *text, *pos;
    unsigned int msglen, len;
    int before;
    int x;
    char template[] = UIFONT_VERTICAL_STRING "                            " UIFONT_VERTICAL_STRING;
    unsigned int j = 5;
    int active = 1;
    int cur_pos = 0;
    int button_count;
	int *buttonx=NULL;
	int action, xx, yy, i;

    /* determine button count for requested `message mode` */
    if (message_mode < 0 || message_mode >= (int)(sizeof msg_mode_buttons / sizeof msg_mode_buttons[0])) {
#ifdef __func__
        fprintf(stderr, "%s:%d:%s: illegal `message_mode` value %d\n",
               __FILE__, __LINE__, __func__, message_mode);
#endif
        return -1;
    }
    button_count = msg_mode_buttons[message_mode];

    /* print the top edge of the dialog. */
    sdl_ui_clear();
    sdl_ui_print_center(UIFONT_TOPLEFT_STRING UIFONT_HORIZONTAL_STRING UIFONT_HORIZONTAL_STRING UIFONT_HORIZONTAL_STRING UIFONT_HORIZONTAL_STRING 
                        UIFONT_HORIZONTAL_STRING UIFONT_HORIZONTAL_STRING UIFONT_HORIZONTAL_STRING UIFONT_HORIZONTAL_STRING UIFONT_HORIZONTAL_STRING 
                        UIFONT_HORIZONTAL_STRING UIFONT_HORIZONTAL_STRING UIFONT_HORIZONTAL_STRING UIFONT_HORIZONTAL_STRING UIFONT_HORIZONTAL_STRING 
                        UIFONT_HORIZONTAL_STRING UIFONT_HORIZONTAL_STRING UIFONT_HORIZONTAL_STRING UIFONT_HORIZONTAL_STRING UIFONT_HORIZONTAL_STRING 
                        UIFONT_HORIZONTAL_STRING UIFONT_HORIZONTAL_STRING UIFONT_HORIZONTAL_STRING UIFONT_HORIZONTAL_STRING UIFONT_HORIZONTAL_STRING 
                        UIFONT_HORIZONTAL_STRING UIFONT_HORIZONTAL_STRING UIFONT_HORIZONTAL_STRING UIFONT_HORIZONTAL_STRING UIFONT_TOPRIGHT_STRING, 2);

    /* make sure that the title length is not more than 28 chars. */
    len = strlen(title);
    assert(len <= MAX_MSGBOX_LEN);

    /* calculate the position in the template to copy the title to. */
    before = (MAX_MSGBOX_LEN - len) / 2;

    /* copy the title into the template. */
    memcpy(template + 1 + before, title, len);

    /* print the title part of the dialog. */
    sdl_ui_print_center(template, 3);

    /* print the title/text separator part of the dialog. */
    sdl_ui_print_center(UIFONT_RIGHTTEE_STRING UIFONT_HORIZONTAL_STRING UIFONT_HORIZONTAL_STRING UIFONT_HORIZONTAL_STRING UIFONT_HORIZONTAL_STRING 
                        UIFONT_HORIZONTAL_STRING UIFONT_HORIZONTAL_STRING UIFONT_HORIZONTAL_STRING UIFONT_HORIZONTAL_STRING UIFONT_HORIZONTAL_STRING 
                        UIFONT_HORIZONTAL_STRING UIFONT_HORIZONTAL_STRING UIFONT_HORIZONTAL_STRING UIFONT_HORIZONTAL_STRING UIFONT_HORIZONTAL_STRING 
                        UIFONT_HORIZONTAL_STRING UIFONT_HORIZONTAL_STRING UIFONT_HORIZONTAL_STRING UIFONT_HORIZONTAL_STRING UIFONT_HORIZONTAL_STRING 
                        UIFONT_HORIZONTAL_STRING UIFONT_HORIZONTAL_STRING UIFONT_HORIZONTAL_STRING UIFONT_HORIZONTAL_STRING UIFONT_HORIZONTAL_STRING 
                        UIFONT_HORIZONTAL_STRING UIFONT_HORIZONTAL_STRING UIFONT_HORIZONTAL_STRING UIFONT_HORIZONTAL_STRING UIFONT_LEFTTEE_STRING, 4);

    text = lib_stralloc(message);
    msglen = (unsigned int)strlen(text);

    /* substitute forward slashes and spaces for backslashes and newline
     * characters.
     */
    pos = text + msglen;
    do {
        switch (*--pos) {
            case '\\':
                *pos = '/';
                break;
            case '\n':
                *pos = ' ';
            default:
                break;
        }
    } while (pos != text);

    while (msglen != 0) {
        len = msglen;
        if (len > MAX_MSGBOX_LEN) {
            /* fold lines at the space that is closest to the right edge. */
            len = MAX_MSGBOX_LEN + 1;
            while (pos[--len] != ' ') {
                /* if a word is too long, fold it anyway! */
                if (len == 0) {
                    len = MAX_MSGBOX_LEN;
                    break;
                }
            }
        }

        /* erase the old line. */
        memset(template + 1, ' ', MAX_MSGBOX_LEN);

        /* calculate the position in the template to copy the message line to. */
        before = (MAX_MSGBOX_LEN - len) / 2;

        /* copy the message line into the template. */
        memcpy(template + 1 + before, pos, len);

        /* print the message line. */
        sdl_ui_print_center(template, j);

        /* advance to the next message line. */
        j++;
        msglen -= len;
        pos += len;

        /* if the text was folded at a space, then move beyond that space. */
        if (*pos == ' ') {
            msglen--;
            pos++;
        }
    }
    lib_free(text);

    /* print any needed buttons. */
    sdl_ui_print_center(UIFONT_VERTICAL_STRING "                            " UIFONT_VERTICAL_STRING, j);
    switch (message_mode) {
        case MESSAGE_OK:
            sdl_ui_print_center(UIFONT_VERTICAL_STRING "            " UIFONT_TOPLEFT_STRING UIFONT_HORIZONTAL_STRING UIFONT_HORIZONTAL_STRING UIFONT_TOPRIGHT_STRING "            " UIFONT_VERTICAL_STRING, j + 1);
            x = sdl_ui_print_center(UIFONT_VERTICAL_STRING "            " UIFONT_VERTICAL_STRING "OK" UIFONT_VERTICAL_STRING "            " UIFONT_VERTICAL_STRING, j + 2);
            sdl_ui_print_center(UIFONT_VERTICAL_STRING "            " UIFONT_BOTTOMLEFT_STRING UIFONT_HORIZONTAL_STRING UIFONT_HORIZONTAL_STRING UIFONT_BOTTOMRIGHT_STRING "            " UIFONT_VERTICAL_STRING, j + 3);
			buttonx = buttons_OK;
            break;
        case MESSAGE_YESNO:
            sdl_ui_print_center(UIFONT_VERTICAL_STRING "      " UIFONT_TOPLEFT_STRING UIFONT_HORIZONTAL_STRING UIFONT_HORIZONTAL_STRING UIFONT_HORIZONTAL_STRING UIFONT_TOPRIGHT_STRING "       " UIFONT_TOPLEFT_STRING UIFONT_HORIZONTAL_STRING UIFONT_HORIZONTAL_STRING UIFONT_TOPRIGHT_STRING "      " UIFONT_VERTICAL_STRING, j + 1);
            x = sdl_ui_print_center(UIFONT_VERTICAL_STRING "      " UIFONT_VERTICAL_STRING "YES" UIFONT_VERTICAL_STRING "       " UIFONT_VERTICAL_STRING "NO" UIFONT_VERTICAL_STRING "      " UIFONT_VERTICAL_STRING, j + 2);
            sdl_ui_print_center(UIFONT_VERTICAL_STRING "      " UIFONT_BOTTOMLEFT_STRING UIFONT_HORIZONTAL_STRING UIFONT_HORIZONTAL_STRING UIFONT_HORIZONTAL_STRING UIFONT_BOTTOMRIGHT_STRING "       " UIFONT_BOTTOMLEFT_STRING UIFONT_HORIZONTAL_STRING UIFONT_HORIZONTAL_STRING UIFONT_BOTTOMRIGHT_STRING "      " UIFONT_VERTICAL_STRING, j + 3);
			buttonx = buttons_YESNO;
            break;
        case MESSAGE_UNIT_SELECT:
            /* Present a selection of '8', '9', '10', '11' or 'SKIP' */
            sdl_ui_print_center(UIFONT_VERTICAL_STRING "  " UIFONT_TOPLEFT_STRING UIFONT_HORIZONTAL_STRING UIFONT_TOPRIGHT_STRING " " UIFONT_TOPLEFT_STRING UIFONT_HORIZONTAL_STRING UIFONT_TOPRIGHT_STRING " " UIFONT_TOPLEFT_STRING UIFONT_HORIZONTAL_STRING UIFONT_HORIZONTAL_STRING UIFONT_TOPRIGHT_STRING " " UIFONT_TOPLEFT_STRING UIFONT_HORIZONTAL_STRING UIFONT_HORIZONTAL_STRING UIFONT_TOPRIGHT_STRING " " UIFONT_TOPLEFT_STRING UIFONT_HORIZONTAL_STRING UIFONT_HORIZONTAL_STRING UIFONT_HORIZONTAL_STRING UIFONT_HORIZONTAL_STRING UIFONT_TOPRIGHT_STRING "  " UIFONT_VERTICAL_STRING, j + 1);
            x = sdl_ui_print_center(UIFONT_VERTICAL_STRING "  " UIFONT_VERTICAL_STRING "8" UIFONT_VERTICAL_STRING " " UIFONT_VERTICAL_STRING "9" UIFONT_VERTICAL_STRING " " UIFONT_VERTICAL_STRING "10" UIFONT_VERTICAL_STRING " " UIFONT_VERTICAL_STRING "11" UIFONT_VERTICAL_STRING " " UIFONT_VERTICAL_STRING "SKIP" UIFONT_VERTICAL_STRING "  " UIFONT_VERTICAL_STRING, j + 2);
            sdl_ui_print_center(UIFONT_VERTICAL_STRING "  " UIFONT_BOTTOMLEFT_STRING UIFONT_HORIZONTAL_STRING UIFONT_BOTTOMRIGHT_STRING " " UIFONT_BOTTOMLEFT_STRING UIFONT_HORIZONTAL_STRING UIFONT_BOTTOMRIGHT_STRING " " UIFONT_BOTTOMLEFT_STRING UIFONT_HORIZONTAL_STRING UIFONT_HORIZONTAL_STRING UIFONT_BOTTOMRIGHT_STRING " " UIFONT_BOTTOMLEFT_STRING UIFONT_HORIZONTAL_STRING UIFONT_HORIZONTAL_STRING UIFONT_BOTTOMRIGHT_STRING " " UIFONT_BOTTOMLEFT_STRING UIFONT_HORIZONTAL_STRING UIFONT_HORIZONTAL_STRING UIFONT_HORIZONTAL_STRING UIFONT_HORIZONTAL_STRING UIFONT_BOTTOMRIGHT_STRING "  " UIFONT_VERTICAL_STRING, j + 3);
			buttonx = buttons_UNIT_SELECT;
           break;
        case MESSAGE_CPUJAM:
        default:
            sdl_ui_print_center(UIFONT_VERTICAL_STRING "     " UIFONT_TOPLEFT_STRING UIFONT_HORIZONTAL_STRING UIFONT_HORIZONTAL_STRING UIFONT_HORIZONTAL_STRING UIFONT_HORIZONTAL_STRING UIFONT_HORIZONTAL_STRING UIFONT_TOPRIGHT_STRING " " UIFONT_TOPLEFT_STRING UIFONT_HORIZONTAL_STRING UIFONT_HORIZONTAL_STRING UIFONT_HORIZONTAL_STRING UIFONT_HORIZONTAL_STRING UIFONT_HORIZONTAL_STRING UIFONT_HORIZONTAL_STRING UIFONT_HORIZONTAL_STRING UIFONT_HORIZONTAL_STRING UIFONT_TOPRIGHT_STRING "     " UIFONT_VERTICAL_STRING, j + 1);
            x = sdl_ui_print_center(UIFONT_VERTICAL_STRING "     " UIFONT_VERTICAL_STRING "RESET" UIFONT_VERTICAL_STRING " " UIFONT_VERTICAL_STRING "CONTINUE" UIFONT_VERTICAL_STRING "     " UIFONT_VERTICAL_STRING, j + 2);
            sdl_ui_print_center(UIFONT_VERTICAL_STRING "     " UIFONT_BOTTOMLEFT_STRING UIFONT_HORIZONTAL_STRING UIFONT_HORIZONTAL_STRING UIFONT_HORIZONTAL_STRING UIFONT_HORIZONTAL_STRING UIFONT_HORIZONTAL_STRING UIFONT_BOTTOMRIGHT_STRING " " UIFONT_BOTTOMLEFT_STRING UIFONT_HORIZONTAL_STRING UIFONT_HORIZONTAL_STRING UIFONT_HORIZONTAL_STRING UIFONT_HORIZONTAL_STRING UIFONT_HORIZONTAL_STRING UIFONT_HORIZONTAL_STRING UIFONT_HORIZONTAL_STRING UIFONT_HORIZONTAL_STRING UIFONT_BOTTOMRIGHT_STRING "     " UIFONT_VERTICAL_STRING, j + 3);
			buttonx = buttons_CPUJAM;
            break;
    }

    /* print the bottom part of the dialog. */
    sdl_ui_print_center(UIFONT_BOTTOMLEFT_STRING UIFONT_HORIZONTAL_STRING UIFONT_HORIZONTAL_STRING UIFONT_HORIZONTAL_STRING UIFONT_HORIZONTAL_STRING 
                        UIFONT_HORIZONTAL_STRING UIFONT_HORIZONTAL_STRING UIFONT_HORIZONTAL_STRING UIFONT_HORIZONTAL_STRING UIFONT_HORIZONTAL_STRING 
                        UIFONT_HORIZONTAL_STRING UIFONT_HORIZONTAL_STRING UIFONT_HORIZONTAL_STRING UIFONT_HORIZONTAL_STRING UIFONT_HORIZONTAL_STRING 
                        UIFONT_HORIZONTAL_STRING UIFONT_HORIZONTAL_STRING UIFONT_HORIZONTAL_STRING UIFONT_HORIZONTAL_STRING UIFONT_HORIZONTAL_STRING 
                        UIFONT_HORIZONTAL_STRING UIFONT_HORIZONTAL_STRING UIFONT_HORIZONTAL_STRING UIFONT_HORIZONTAL_STRING UIFONT_HORIZONTAL_STRING 
                        UIFONT_HORIZONTAL_STRING UIFONT_HORIZONTAL_STRING UIFONT_HORIZONTAL_STRING UIFONT_HORIZONTAL_STRING UIFONT_BOTTOMRIGHT_STRING, j + 4);

    x += (menu_draw->max_text_x - 30) / 2;
    while (active) {
        switch (message_mode) {
            case MESSAGE_OK:
                sdl_ui_reverse_colors();
                sdl_ui_print_center("OK", j + 2);
                sdl_ui_reverse_colors();
                break;
            case MESSAGE_YESNO:
                if (cur_pos == 0) {
                    sdl_ui_reverse_colors();
                }
                sdl_ui_print("YES", x - 22, j + 2);
                sdl_ui_reverse_colors();
                sdl_ui_print("NO", x - 10, j + 2);
                if (cur_pos == 1) {
                    sdl_ui_reverse_colors();
                }
                break;

            case MESSAGE_UNIT_SELECT:
                sdl_ui_print_center(UIFONT_VERTICAL_STRING "  " UIFONT_VERTICAL_STRING "8" UIFONT_VERTICAL_STRING " " UIFONT_VERTICAL_STRING "9" UIFONT_VERTICAL_STRING " " UIFONT_VERTICAL_STRING "10" UIFONT_VERTICAL_STRING " " UIFONT_VERTICAL_STRING "11" UIFONT_VERTICAL_STRING " " UIFONT_VERTICAL_STRING "SKIP" UIFONT_VERTICAL_STRING "  " UIFONT_VERTICAL_STRING, j + 2);
 
                sdl_ui_reverse_colors();
                switch (cur_pos) {
                    case 0:
                        sdl_ui_print("8", x - 26, j + 2);
                        break;
                    case 1:
                        sdl_ui_print("9", x - 22, j + 2);
                        break;
                    case 2:
                        sdl_ui_print("10", x - 18, j + 2);
                        break;
                    case 3:
                        sdl_ui_print("11", x - 13, j + 2);
                        break;
                    case 4:
                        sdl_ui_print("SKIP", x - 8, j + 2);
                        break;
                    default:
                        break;
                }
                sdl_ui_reverse_colors();
                break;
            case MESSAGE_CPUJAM:
            default:
                if (cur_pos == 0) {
                    sdl_ui_reverse_colors();
                }
                sdl_ui_print("RESET", x - 23, j + 2);
                sdl_ui_reverse_colors();
                sdl_ui_print("CONTINUE", x - 15, j + 2);
                if (cur_pos == 1) {
                    sdl_ui_reverse_colors();
                }
                break;

        }

        sdl_ui_refresh();

 		action=sdl_ui_menu_poll_input();
		while (action != MENU_ACTION_NONE) {
			i=action; action = MENU_ACTION_NONE;
			switch (i) {
				case MENU_ACTION_CANCEL:
				case MENU_ACTION_EXIT:
					cur_pos = -1;
					active = 0;
					break;
				case MENU_ACTION_SELECT:
					active = 0;
					break;
				case MENU_ACTION_LEFT:
				case MENU_ACTION_UP:
					if (message_mode != MESSAGE_OK) {
						cur_pos--;
						if (cur_pos < 0) {
							cur_pos = button_count - 1;
						}
					}
					break;
				case MENU_ACTION_RIGHT:
				case MENU_ACTION_DOWN:
					if (message_mode != MESSAGE_OK) {
						cur_pos++;
						if (cur_pos >= button_count) {
							cur_pos = 0;
						}
					}
					break;
				case MENU_ACTION_MOUSE:
					yy = lastevent.button.y*240 / sdl_active_canvas->screen->h / activefont.h;
					xx = lastevent.button.x*320 / sdl_active_canvas->screen->w / activefont.w;
					if (lastevent.type != SDL_MOUSEBUTTONUP) {
						if (yy > j && yy < j+4) {
							for (i=0; i<button_count; i++) {
								if (xx >= x-buttonx[i*2] && xx < x-buttonx[i*2]+buttonx[i*2+1]) {
									cur_pos=i;
									break;
								}
							}
						}
					} else {
						if (yy > j && yy < j+4 && xx >= x-buttonx[cur_pos*2] && xx < x-buttonx[cur_pos*2]+buttonx[cur_pos*2+1])
							action=MENU_ACTION_SELECT;
					}
					break;
				default:
					SDL_Delay(20);
					break;
			}
        }
    }
    return cur_pos;
}

int message_box(const char *title, char *message, int message_mode)
{
	int retval;

	sdl_ui_init_draw_params();
    menu_draw = sdl_ui_get_menu_param();

    // when called from the main thread, we will pause emulation,
	// backup the menu buffer, show the messagebox and restore the
	// backup buffer afterwards
	if (SDL_ThreadID() == -1) {
		int w=320,h=240,warp_state;

		uint8_t *old_menu_draw_buffer = menu_draw_buffer;
		vsync_suspend_speed_eval();
		sound_suspend();
		menu_draw_buffer = calloc(w * h,1);
		sdl_menu_state |= MSGBOX_ACTIVE;
		int kbdhidden=is_keyboard_hidden();
		if (!kbdhidden) toggle_keyboard();
		retval = handle_message_box(title, message, message_mode);
		if (kbdhidden!=is_keyboard_hidden()) toggle_keyboard();
	
		free(menu_draw_buffer);
		menu_draw_buffer=old_menu_draw_buffer;
		/* Do not resume sound if in warp mode */
		resources_get_int("WarpMode", &warp_state);
		if (warp_state == 0) {
			sound_resume();
		}
		sdl_menu_state &= ~MSGBOX_ACTIVE;
		sdl_ui_refresh();
		return retval;
    }
	// just a in-menu message box
	return handle_message_box(title, (const char *)message, message_mode);
}
