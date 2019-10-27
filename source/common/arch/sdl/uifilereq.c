/*
 * uifilereq.c - Common SDL file selection dialog functions.
 *
 * Written by
 *  Marco van den Heuvel <blackystardust68@yahoo.com>
 *  Hannu Nuotio <hannu.nuotio@tut.fi>
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
#include "ioutil.h"
#include "lib.h"
#include "menu_common.h"
#include "ui.h"
#include "uibottom.h"
#include "uimenu.h"
#include "uifilereq.h"
#include "util.h"
#include "videoarch.h"
#include <unistd.h>

static menu_draw_t *menu_draw;
static char *last_selected_file = NULL;
static int havescrollbar=0;

/* static int last_selected_pos = 0; */ /* ? */
int last_selected_image_pos = 0;    /* FIXME: global variable. ugly. */

#define SDL_FILEREQ_META_FILE 0
#define SDL_FILEREQ_META_PATH 1

/* any platform that supports drive letters/names should
   define SDL_CHOOSE_DRIVES and provide the drive functions:
   char **archdep_list_drives(void);
   char *archdep_get_current_drive(void);
   void archdep_set_current_drive(const char *drive);
*/

#ifdef SDL_CHOOSE_DRIVES
#define SDL_FILEREQ_META_DRIVE 2
#define SDL_FILEREQ_META_NUM 3
#else
#define SDL_FILEREQ_META_NUM 2
#endif


/* ------------------------------------------------------------------ */
/* static functions */

static int sdl_ui_file_selector_recall_location(ioutil_dir_t *directory)
{
    unsigned int i, j, k, count;
    int direction;

    if (!last_selected_file) {
        return 0;
    }

    /* the file list is sorted by ioutil, do binary search */
    i = 0;
    k = directory->file_amount;

    /* ...but keep a failsafe in case the above assumption breaks */
    count = 50;

    while ((i < k) && (--count)) {
        j = i + ((k - i) >> 1);
        direction = strcmp(last_selected_file, directory->files[j].name);

        if (direction > 0) {
            i = j + 1;
        } else if (direction < 0) {
            k = j;
        } else {
            return j + directory->dir_amount + SDL_FILEREQ_META_NUM;
        }
    }

    return 0;
}

static char* sdl_ui_get_file_selector_entry(ioutil_dir_t *directory, int offset, int *isdir, ui_menu_filereq_mode_t mode)
{
    *isdir = 0;

    if (offset >= (directory->dir_amount + directory->file_amount + SDL_FILEREQ_META_NUM)) {
        return NULL;
    }

    if (offset == SDL_FILEREQ_META_FILE) {
        switch (mode) {
            case FILEREQ_MODE_CHOOSE_FILE:
            case FILEREQ_MODE_CHOOSE_FILE_IN_IMAGE:
            case FILEREQ_MODE_SAVE_FILE:
                return "<enter filename>";

            case FILEREQ_MODE_CHOOSE_DIR:
                return "<choose current directory>";
        }
    }

    if (offset == SDL_FILEREQ_META_PATH) {
        return "<enter path>";
    }

#ifdef SDL_FILEREQ_META_DRIVE
    if (offset == SDL_FILEREQ_META_DRIVE) {
        return "<choose drive>";
    }
#endif

    if (offset >= (directory->dir_amount + SDL_FILEREQ_META_NUM)) {
        return directory->files[offset - (directory->dir_amount + SDL_FILEREQ_META_NUM)].name;
    } else {
        *isdir = 1;
        return directory->dirs[offset - SDL_FILEREQ_META_NUM].name;
    }
}

#if (FSDEV_DIR_SEP_CHR == '\\')
static void sdl_ui_print_translate_seperator(const char *text, int x, int y)
{
    unsigned int len;
    unsigned int i;
    char *new_text = NULL;

    len = strlen(text);
    new_text = lib_stralloc(text);

    for (i = 0; i < len; i++) {
        if (new_text[i] == '\\') {
            new_text[i] = '/';
        }
    }
    sdl_ui_print(new_text, x, y);
    lib_free(new_text);
}
#else
#define sdl_ui_print_translate_seperator(t, x, y) sdl_ui_print(t, x, y)
#endif

static void sdl_ui_display_path(const char *current_dir)
{
    int len;
    char *text = NULL;
    char *temp = NULL;
    int before = 0;
    int after = 0;
    int pos = 0;
    int amount = 0;
    int i;

    len = strlen(current_dir);

    if (len > menu_draw->max_text_x) {
        text = lib_stralloc(current_dir);

        temp = strchr(current_dir + 1, FSDEV_DIR_SEP_CHR);
        before = (int)(temp - current_dir + 1);

        while (temp != NULL) {
            amount++;
            temp = strchr(temp + 1, FSDEV_DIR_SEP_CHR);
        }

        while (text[len - after] != FSDEV_DIR_SEP_CHR) {
            after++;
        }

        if (amount > 1 && (before + after + 3) < menu_draw->max_text_x) {
            temp = strchr(current_dir + 1, FSDEV_DIR_SEP_CHR);
            while (((temp - current_dir + 1) < (menu_draw->max_text_x - after - 3)) && temp != NULL) {
                before = (int)(temp - current_dir + 1);
                temp = strchr(temp + 1, FSDEV_DIR_SEP_CHR);
            }
        } else {
            before = (menu_draw->max_text_x - 3) / 2;
            after = len - (len - menu_draw->max_text_x) - before - 3;
        }
        pos = len - after;
        text[before] = '.';
        text[before + 1] = '.';
        text[before + 2] = '.';
        for (i = 0; i < after; i++) {
            text[before + 3 + i] = text[pos + i];
        }
        text[before + 3 + after] = 0;
        sdl_ui_print_translate_seperator(text, 0, 2);
    } else {
        sdl_ui_print_translate_seperator(current_dir, 0, 2);
    }

    lib_free(text);
}

static void sdl_ui_file_selector_redraw(ioutil_dir_t *directory, const char *title, const char *current_dir, int offset, int num_items, int more, ui_menu_filereq_mode_t mode, int cur_offset, int total)
{
    int i, j, isdir = 0;
    char* title_string;
    char* name;
    uint8_t oldbg;

    title_string = lib_msprintf("%-35.35s", title);

    sdl_ui_clear();
    sdl_ui_display_title(title_string);
    lib_free(title_string);
    sdl_ui_display_path(current_dir);

    for (i = 0; i < num_items; ++i) {
        
        if (i == cur_offset) {
            oldbg = sdl_ui_set_cursor_colors();
        }
        
        j = MENU_FIRST_X;
        name = sdl_ui_get_file_selector_entry(directory, offset + i, &isdir, mode);
        if (isdir) {
            j += sdl_ui_print("(D) ", MENU_FIRST_X, i + MENU_FIRST_Y + SDL_FILEREQ_META_NUM);
        }
        j += sdl_ui_print(name, j, i + MENU_FIRST_Y + SDL_FILEREQ_META_NUM);

        if (i == cur_offset) {
            sdl_ui_print_eol(j, i + MENU_FIRST_Y + SDL_FILEREQ_META_NUM);
            sdl_ui_reset_cursor_colors(oldbg);
        }
    }

	// scrollbar
	if (more || offset) {
		havescrollbar=1;
		int menu_max = menu_draw->max_text_y - (MENU_FIRST_Y + SDL_FILEREQ_META_NUM) - 1;
		int beg = (float)(offset * menu_max) / (float)total + 0.5f;
		int end = (float)((offset + num_items) * menu_max) / (float)total + 0.5f;
		for (i=0; i<=menu_max; ++i) {
			if (i==beg) sdl_ui_reverse_colors();
			sdl_ui_putchar(' ', menu_draw->max_text_x-1, i + MENU_FIRST_Y + SDL_FILEREQ_META_NUM);
			if (i==end) sdl_ui_reverse_colors();
		}
	} else {
		havescrollbar=0;
	}
}

static void sdl_ui_file_selector_redraw_cursor(ioutil_dir_t *directory, int offset, int num_items, ui_menu_filereq_mode_t mode, int cur_offset, int old_offset)
{
    int i, j, isdir = 0;
    char* name;
    uint8_t oldbg = 0;

	if (havescrollbar) --menu_draw->max_text_x;
    for (i = 0; i < num_items; ++i) {
        if ((i == cur_offset) || (i == old_offset)){
            if (i == cur_offset) {
                oldbg = sdl_ui_set_cursor_colors();
            }
            j = MENU_FIRST_X;
            name = sdl_ui_get_file_selector_entry(directory, offset + i, &isdir, mode);
            if (isdir) {
                j += sdl_ui_print("(D) ", MENU_FIRST_X, i + MENU_FIRST_Y + SDL_FILEREQ_META_NUM);
            }
            j += sdl_ui_print(name, j, i + MENU_FIRST_Y + SDL_FILEREQ_META_NUM);

            sdl_ui_print_eol(j, i + MENU_FIRST_Y + SDL_FILEREQ_META_NUM);
            if (i == cur_offset) {
                sdl_ui_reset_cursor_colors(oldbg);
            }
        }
    }
	if (havescrollbar) ++menu_draw->max_text_x;
}

#ifdef SDL_FILEREQ_META_DRIVE
static char *current_drive = NULL;
static int drive_set = 0;

static UI_MENU_CALLBACK(drive_callback)
{
    char *drive;

    drive = (char *)param;

    if (strcmp(drive, current_drive) == 0) {
        return sdl_menu_text_tick;
    } else if (activated) {
        current_drive = drive;
        drive_set = 1;
        return sdl_menu_text_tick;
    }

    return NULL;
}

static char * display_drive_menu(void)
{
    char **drives, **p;
    char *result = NULL;
    char *previous_drive;
    int drive_count = 1;
    ui_menu_entry_t *sub_menu, *pm;
    ui_menu_entry_t menu;

    drives = archdep_list_drives();

    p = drives;

    while (*p != NULL) {
        ++drive_count;
        ++p;
    }

    sub_menu = lib_malloc(sizeof(ui_menu_entry_t) * drive_count);
    pm = sub_menu;

    p = drives;

    while (*p != NULL) {
        pm->string = *p;
        pm->type = MENU_ENTRY_OTHER;
        pm->callback = drive_callback;
        pm->data = (ui_callback_data_t)*p;
        ++pm;
        ++p;
    }

    pm->string = NULL;
    pm->type = MENU_ENTRY_TEXT;
    pm->callback = NULL;
    pm->data = (ui_callback_data_t)NULL;

    menu.string = "Choose drive";
    menu.type = MENU_ENTRY_DYNAMIC_SUBMENU;
    menu.callback = NULL;
    menu.data = (ui_callback_data_t)sub_menu;

    previous_drive = archdep_get_current_drive();

    current_drive = previous_drive;
    drive_set = 0;
    sdl_ui_external_menu_activate(&menu);

    if (drive_set) {
        result = current_drive;
    }

    lib_free(previous_drive);

    lib_free(sub_menu);

    p = drives;

    while (*p != NULL) {
        if (*p != result) {
            lib_free(*p);
        }
        ++p;
    }
    lib_free(drives);

    return result;
}
#endif

/* ------------------------------------------------------------------ */
/* External UI interface */

#ifdef UNIX_COMPILE
#define SDL_FILESELECTOR_DIRMODE    IOUTIL_OPENDIR_NO_DOTFILES
#else
#define SDL_FILESELECTOR_DIRMODE    IOUTIL_OPENDIR_ALL_FILES
#endif

#include "persistence.h"

char* sdl_ui_file_selection_dialog(const char* title, ui_menu_filereq_mode_t mode)
{
    int total, dirs, files, menu_max;
    int active = 1;
    int offset = 0;
    int redraw = 1;
    char *retval = NULL;
    int cur = 0, cur_old = -1;
    ioutil_dir_t *directory;
    char *current_dir;
    char *backup_dir;
    char *inputstring;
    unsigned int maxpathlen;
    int i, y, x, action, old_y=-1, dragging=0;

    last_selected_image_pos = 0;

    menu_draw = sdl_ui_get_menu_param();
    maxpathlen = ioutil_maxpathlen();
    current_dir = lib_malloc(maxpathlen);

    strncpy(current_dir,persistence_get("cwd","/"),maxpathlen);

    ioutil_chdir(current_dir);
    backup_dir = lib_stralloc(current_dir);

    // clear screen
    sdl_ui_file_selector_redraw(NULL, title, current_dir, offset, 0, 0, mode, 0, 0);

    directory = ioutil_opendir(current_dir, SDL_FILESELECTOR_DIRMODE);
    if (directory == NULL) {
        return NULL;
    }

    dirs = directory->dir_amount;
    files = directory->file_amount;
    total = dirs + files + SDL_FILEREQ_META_NUM;
    menu_max = menu_draw->max_text_y - (MENU_FIRST_Y + SDL_FILEREQ_META_NUM);

    if ((mode == FILEREQ_MODE_CHOOSE_FILE) || (mode == FILEREQ_MODE_CHOOSE_FILE_IN_IMAGE)) {
        offset = sdl_ui_file_selector_recall_location(directory);
		if (offset > total - menu_max) {
			i=offset;
			offset = total>menu_max?total - menu_max:0;
			cur -= offset - i;
		}
    }

    while (active) {
        if (redraw) {
            sdl_ui_file_selector_redraw(directory, title, current_dir, offset, 
                (total - offset > menu_max) ? menu_max : total - offset, 
                (total - offset > menu_max) ? 1 : 0, mode, cur, total);
            redraw = 0;
        } else {
            sdl_ui_file_selector_redraw_cursor(directory, offset, 
                    (total - offset > menu_max) ? menu_max : total - offset, 
                    mode, cur, cur_old);
        }
        sdl_ui_refresh();

 		action=sdl_ui_menu_poll_input();       
		while (action != MENU_ACTION_NONE) {
			i=action; action = MENU_ACTION_NONE;
			switch (i) {
				case MENU_ACTION_HOME:
					cur_old = cur;
					cur = 0;
					offset = 0;
					redraw = 1;
					break;

				case MENU_ACTION_END:
					cur_old = cur;
					if (total < (menu_max - 1)) {
						cur = total - 1;
						offset = 0;
					} else {
						cur = menu_max - 1;
						offset = total - menu_max;
					}
					redraw = 1;
					break;

				case MENU_ACTION_UP:
					if (cur > 0) {
						cur_old = cur;
						--cur;
					} else {
						if (offset > 0) {
							offset--;
							redraw = 1;
						}
					}
					break;

				case MENU_ACTION_PAGEUP:
				case MENU_ACTION_LEFT:
					offset -= menu_max;
					if (offset < 0) {
						offset = 0;
						cur_old = -1;
						cur = 0;
					}
					redraw = 1;
					break;

				case MENU_ACTION_DOWN:
					if (cur < (menu_max - 1)) {
						if ((cur + offset) < total - 1) {
							cur_old = cur;
							++cur;
						}
					} else {
						if (offset < (total - menu_max)) {
							offset++;
							redraw = 1;
						}
					}
					break;

				case MENU_ACTION_PAGEDOWN:
				case MENU_ACTION_RIGHT:
					offset += menu_max;
					if (offset > total - menu_max) {
						cur -= total - menu_max - offset;
						offset = total - menu_max;
					}
					if ((cur + offset) >= total) {
						cur = total - offset - 1;
					}
					redraw = 1;
					break;

				case MENU_ACTION_SELECT:
					switch (offset + cur) {
						case SDL_FILEREQ_META_FILE:
							if ((mode == FILEREQ_MODE_CHOOSE_FILE) || (mode == FILEREQ_MODE_CHOOSE_FILE_IN_IMAGE) || (mode == FILEREQ_MODE_SAVE_FILE)) {
								inputstring = sdl_ui_text_input_dialog("Enter filename", NULL);
								if (inputstring == NULL || inputstring[0]==0) {
									redraw = 1;
								} else {
									if (archdep_path_is_relative(inputstring) && (strchr(inputstring, FSDEV_DIR_SEP_CHR) != NULL)) {
										retval = inputstring;
									} else {
										retval = archdep_join_paths(current_dir, inputstring, NULL);
										lib_free(inputstring);
									}
									active = 0;
								}
							} else {
								retval = lib_stralloc(current_dir);
								active = 0;
							}
							break;
						case SDL_FILEREQ_META_PATH:
							inputstring = sdl_ui_text_input_dialog("Enter path", NULL);
							if (inputstring != NULL) {
								ioutil_chdir(inputstring);
								lib_free(inputstring);
								ioutil_closedir(directory);
								ioutil_getcwd(current_dir, maxpathlen);
								directory = ioutil_opendir(current_dir, SDL_FILESELECTOR_DIRMODE);
								offset = 0;
								cur_old = -1;
								cur = 0;
								dirs = directory->dir_amount;
								files = directory->file_amount;
								total = dirs + files + SDL_FILEREQ_META_NUM;
							}
							redraw = 1;
							break;

	#ifdef SDL_FILEREQ_META_DRIVE
						case SDL_FILEREQ_META_DRIVE:
							inputstring = display_drive_menu();
							if (inputstring != NULL) {
								archdep_set_current_drive(inputstring);
								lib_free(inputstring);
								ioutil_closedir(directory);
								ioutil_getcwd(current_dir, maxpathlen);
								directory = ioutil_opendir(current_dir, SDL_FILESELECTOR_DIRMODE);
								offset = 0;
								cur_old = -1;
								cur = 0;
								dirs = directory->dir_amount;
								files = directory->file_amount;
								total = dirs + files + SDL_FILEREQ_META_NUM;
							}
							redraw = 1;
							break;
	#endif
						default:
							if (offset + cur < (dirs + SDL_FILEREQ_META_NUM)) {
								/* enter subdirectory */
								ioutil_chdir(directory->dirs[offset + cur - SDL_FILEREQ_META_NUM].name);
								ioutil_closedir(directory);
								ioutil_getcwd(current_dir, maxpathlen);
								directory = ioutil_opendir(current_dir, SDL_FILESELECTOR_DIRMODE);
								offset = 0;
								cur_old = -1;
								cur = 0;
								dirs = directory->dir_amount;
								files = directory->file_amount;
								total = dirs + files + SDL_FILEREQ_META_NUM;
								redraw = 1;
							} else {
								char *selected_file = directory->files[offset + cur - dirs - SDL_FILEREQ_META_NUM].name;
								if ((mode == FILEREQ_MODE_CHOOSE_FILE) || (mode == FILEREQ_MODE_CHOOSE_FILE_IN_IMAGE) || (mode == FILEREQ_MODE_CHOOSE_FILE_IN_IMAGE)) {
									lib_free(last_selected_file);
									last_selected_file = lib_stralloc(selected_file);
								}
								retval = archdep_join_paths(current_dir, selected_file, NULL);

								if (mode == FILEREQ_MODE_CHOOSE_FILE_IN_IMAGE) {
									/* browse image */
									int retval2;
									retval2 = sdl_ui_image_file_selection_dialog(retval, mode);
									if (retval2 != -1) {
										active = 0;
									}
									last_selected_image_pos = retval2;
									redraw = 1;
								} else {
									active = 0;
									last_selected_image_pos = 0;
								}
							}
							break;
					}
					break;

				case MENU_ACTION_CANCEL:
				case MENU_ACTION_EXIT:
					retval = NULL;
					active = 0;
					ioutil_chdir(backup_dir);
					break;

				case MENU_ACTION_MOUSE:
					y = ((lastevent.button.y*240) / sdl_active_canvas->screen->h) / activefont.h;
					x = ((lastevent.button.x*320) / sdl_active_canvas->screen->w) / activefont.w;					
					if (lastevent.type != SDL_MOUSEBUTTONUP) {
						i=offset;
						if (old_y==-1) {
							if (y >= MENU_FIRST_Y + SDL_FILEREQ_META_NUM && 
								y < menu_draw->max_text_y &&
								y <= MENU_FIRST_Y + total - offset + 1) {
								if (havescrollbar && (dragging || x==menu_draw->max_text_x-1)) {
									offset = ((y - MENU_FIRST_Y - SDL_FILEREQ_META_NUM) * total) / menu_max - menu_max/2;
									dragging=1;
								} else {
									if (cur != y - MENU_FIRST_Y - SDL_FILEREQ_META_NUM) {
										cur_old=cur;
										cur = y - MENU_FIRST_Y - SDL_FILEREQ_META_NUM;
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
						if (!dragging && cur == y - MENU_FIRST_Y - SDL_FILEREQ_META_NUM) action=MENU_ACTION_SELECT;
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
    ioutil_closedir(directory);
    persistence_put("cwd",current_dir);

    lib_free(current_dir);
    lib_free(backup_dir);

    return retval;
}

void sdl_ui_file_selection_dialog_shutdown(void)
{
    lib_free(last_selected_file);
    last_selected_file = NULL;
}
