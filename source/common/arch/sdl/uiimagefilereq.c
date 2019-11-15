/*
 * uiimagefilereq.c - Functions to select a file inside a D64/T64 Image
 *
 * Written by
 *  groepaz <groepaz@gmx.net>
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
#include "types.h"

#include "vice_sdl.h"
#include <string.h>

#include "archdep.h"
#include "diskcontents.h"
#include "tapecontents.h"
#include "imagecontents.h"
#include "ioutil.h"
#include "lib.h"
#include "ui.h"
#include "uibottom.h"
#include "uimenu.h"
#include "uifilereq.h"
#include "util.h"
#include "videoarch.h"
#include "menu_common.h"

#define SDL_FILEREQ_META_NUM 0

#define IMAGE_FIRST_Y 1

static menu_draw_t *menu_draw;
static int havescrollbar=0;

static void sdl_ui_image_file_selector_redraw(image_contents_t *contents, const char *title, int offset, int num_items, int more, ui_menu_filereq_mode_t mode, int cur_offset, int total)
{
    int i, j;
    char* title_string;
    char* name;
    uint8_t oldbg;
    image_contents_file_list_t *entry;

    title_string = image_contents_to_string(contents, 0);
    
    sdl_ui_clear();
    sdl_ui_display_title(title_string);
    lib_free(title_string);

    entry = contents->file_list;
    for (i = 0; i < offset; ++i) {
        entry = entry->next;
    }
    for (i = 0; i < num_items; ++i) {
        if (i == cur_offset) {
            oldbg = sdl_ui_set_cursor_colors();
        }

        j = MENU_FIRST_X;
        name = image_contents_file_to_string(entry, IMAGE_CONTENTS_STRING_PETSCII);
        j += sdl_ui_print(name, j, i + IMAGE_FIRST_Y + SDL_FILEREQ_META_NUM);

        if (i == cur_offset) {
            sdl_ui_print_eol(j, i + IMAGE_FIRST_Y + SDL_FILEREQ_META_NUM);
            sdl_ui_reset_cursor_colors(oldbg);
        }
        entry = entry->next;
    }
    name = lib_msprintf("%d BLOCKS FREE.", contents->blocks_free);
    sdl_ui_print(name, MENU_FIRST_X, i + IMAGE_FIRST_Y + SDL_FILEREQ_META_NUM);
    lib_free(name);

	// scrollbar
	if (more || offset) {
		havescrollbar=1;
		int menu_max = menu_draw->max_text_y - (IMAGE_FIRST_Y + SDL_FILEREQ_META_NUM)-2;
		int beg = (float)(offset * menu_max) / (float)total + 0.5f;
		int end = (float)((offset + num_items) * menu_max) / (float)total + 0.5f;
		for (i=0; i<=menu_max; ++i) {
			if (i==beg) sdl_ui_reverse_colors();
			sdl_ui_putchar(' ', menu_draw->max_text_x-1, i + IMAGE_FIRST_Y + SDL_FILEREQ_META_NUM);
			if (i==end) sdl_ui_reverse_colors();
		}
	} else {
		havescrollbar=0;
	}
}

static void sdl_ui_image_file_selector_redraw_cursor(image_contents_t *contents, int offset, int num_items, ui_menu_filereq_mode_t mode, int cur_offset, int old_offset)
{
    int i, j;
    char* name;
    uint8_t oldbg = 0;
    image_contents_file_list_t *entry;

    entry = contents->file_list;
    for (i = 0; i < offset; ++i) {
        entry = entry->next;
    }
	if (havescrollbar) --menu_draw->max_text_x;
	for (i = 0; i < num_items; ++i) {
        if ((i == cur_offset) || (i == old_offset)){
            if (i == cur_offset) {
                oldbg = sdl_ui_set_cursor_colors();
            }
            j = MENU_FIRST_X;
            name = image_contents_file_to_string(entry, IMAGE_CONTENTS_STRING_PETSCII);
            j += sdl_ui_print(name, j, i + IMAGE_FIRST_Y + SDL_FILEREQ_META_NUM);

            sdl_ui_print_eol(j, i + IMAGE_FIRST_Y + SDL_FILEREQ_META_NUM);
            if (i == cur_offset) {
                sdl_ui_reset_cursor_colors(oldbg);
            }
        }
        entry = entry->next;
    }
	if (havescrollbar) ++menu_draw->max_text_x;
}

/* ------------------------------------------------------------------ */
/* External UI interface */

int sdl_ui_image_file_selection_dialog(const char* filename, ui_menu_filereq_mode_t mode)
{
    int total, dirs = 0, files, menu_max;
    int active = 1;
    int offset = 0;
    int redraw = 1;
    int cur = 0, cur_old = -1;
  	int i, y, x, action, old_y=-1, dragging=0;

	image_contents_t *contents = NULL;
    image_contents_file_list_t *entry;
    int retval = -1;

    menu_draw = sdl_ui_get_menu_param();

    /* FIXME: it might be a good idea to wrap this into a common imagecontents_read */
    contents = tapecontents_read(filename);
    if (contents == NULL) {
        contents = diskcontents_read(filename, 0);
        if (contents == NULL) {
            return 0;
        }
    }

    /* count files in the list */
    files = 0;
    for (entry = contents->file_list; entry != NULL; entry = entry->next) {
        files++;
    }

    total = dirs + files + SDL_FILEREQ_META_NUM;
    menu_max = menu_draw->max_text_y - (IMAGE_FIRST_Y + SDL_FILEREQ_META_NUM);
    if (menu_max > 0) { menu_max--; } /* make room for BLOCKS FREE */

    while (active) {

        sdl_ui_set_active_font(MENU_FONT_IMAGES);

        if (redraw) {
            sdl_ui_image_file_selector_redraw(contents, filename, offset, 
                (total - offset > menu_max) ? menu_max : total - offset, 
                (total - offset > menu_max) ? 1 : 0, mode, cur, total);
            redraw = 0;
        } else {
            sdl_ui_image_file_selector_redraw_cursor(contents, offset, 
                    (total - offset > menu_max) ? menu_max : total - offset, 
                    mode, cur, cur_old);
        }

        sdl_ui_set_active_font(MENU_FONT_ASCII);

        sdl_ui_refresh();

 		action=sdl_ui_menu_poll_input();       
		while (action != MENU_ACTION_NONE) {
			i=action; action = MENU_ACTION_NONE;
			switch (i) {
				case MENU_ACTION_HOME:
					cur_old = cur;
					cur = -offset;
					redraw |= sdl_ui_adjust_offset(&offset, &cur,menu_max,total);
					break;
				case MENU_ACTION_END:
					cur_old = cur;
					cur = total-offset-1;
					redraw |= sdl_ui_adjust_offset(&offset, &cur,menu_max,total);
					break;
				case MENU_ACTION_UP:
					cur_old=cur;
					--cur;
					redraw |= sdl_ui_adjust_offset(&offset, &cur,menu_max,total);
					break;
				case MENU_ACTION_PAGEUP:
				case MENU_ACTION_LEFT:
					cur_old = cur;
					cur -= menu_max;
					redraw |= sdl_ui_adjust_offset(&offset, &cur,menu_max,total);
					break;
				case MENU_ACTION_DOWN:
					cur_old=cur;
					++cur;
					redraw |= sdl_ui_adjust_offset(&offset, &cur,menu_max,total);
					break;
				case MENU_ACTION_PAGEDOWN:
				case MENU_ACTION_RIGHT:
					cur_old = cur;
					cur += menu_max;
					redraw |= sdl_ui_adjust_offset(&offset, &cur,menu_max,total);
					break;
				case MENU_ACTION_SELECT:
					active = 0;
					retval = offset + cur - dirs - SDL_FILEREQ_META_NUM + 1;
					break;
				case MENU_ACTION_CANCEL:
				case MENU_ACTION_EXIT:
					active = 0;
					break;
				case MENU_ACTION_MOUSE:
					y = ((lastevent.button.y*240) / sdl_active_canvas->screen->h) / activefont.h;
					x = ((lastevent.button.x*320) / sdl_active_canvas->screen->w) / activefont.w;
					if (lastevent.type != SDL_MOUSEBUTTONUP) {
						i=offset;
						if (old_y==-1) {
							if (y >= IMAGE_FIRST_Y + SDL_FILEREQ_META_NUM && 
								y < menu_max + IMAGE_FIRST_Y + SDL_FILEREQ_META_NUM &&
								y <= MENU_FIRST_Y + total - offset + 1) {
								if (havescrollbar && (dragging || x==menu_draw->max_text_x-1)) {
									offset = ((y - IMAGE_FIRST_Y + SDL_FILEREQ_META_NUM) * total ) / menu_max - menu_max/2;
								   dragging=1;
								} else {
									if (cur != y - IMAGE_FIRST_Y + SDL_FILEREQ_META_NUM) {
										cur_old=cur;
										cur = y - IMAGE_FIRST_Y + SDL_FILEREQ_META_NUM;
									}
									old_y=y;
								}
							}
						} else {
							if (y!=old_y) {
								dragging=1;
								offset-=y-old_y;
								old_y=y;
							}
						}
						if (i != offset) {
							if (offset > total - menu_max)
								offset=total - menu_max;
							if (offset<0) offset=0;
							if (i!=offset) {
								cur-=offset-i;
								if (cur<0) cur=0;
								if (cur>=menu_max) cur=menu_max-1;
								cur_old=-1;
								redraw=1;
							}
						}
					} else {
						if (!dragging && cur == y - IMAGE_FIRST_Y - SDL_FILEREQ_META_NUM) action=MENU_ACTION_SELECT;
						dragging=0;
						old_y=-1;
					}
					break;
				default:
					SDL_Delay(10);
					break;
			}
        }
    }

    return retval;
}
