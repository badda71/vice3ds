/*
 * menu_help.c - SDL help menu functions.
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

#include <stdlib.h>
#include "vice_sdl.h"

#include "cmdline.h"
#include "info.h"
#include "lib.h"
#include "menu_common.h"
#include "menu_help.h"
#include "ui.h"
#include "uimenu.h"
#include "util.h"
#include "version.h"
#include "vicefeatures.h"

/* gets an offset into the text and an amount of lines to scroll
   up, returns a new offset */
static unsigned int scroll_up(const char *text, int offset, int amount)
{
    while ((amount--) && (offset >= 2)) {
        int i = offset - 2;

        while ((i >= 0) && (text[i] != '\n')) {
            --i;
        }

        offset = i + 1;
    }
    if (offset < 0) {
        offset = 0;
    }
    return offset;
}

#define CHARCODE_UMLAUT_A_LOWER         ((char)0xe4)
#define CHARCODE_UMLAUT_A_UPPER         ((char)0xc4)

#define CHARCODE_UMLAUT_O_LOWER         ((char)0xf6)
#define CHARCODE_UMLAUT_O_UPPER         ((char)0xd6)

#define CHARCODE_UMLAUT_U_LOWER         ((char)0xfc)
#define CHARCODE_UMLAUT_U_UPPER         ((char)0xdc)

#define CHARCODE_GRAVE_E_LOWER          ((char)0xe8)
#define CHARCODE_AIGU_E_LOWER           ((char)0xe9)

#define CHARCODE_KROUZEK_A_LOWER        ((char)0xe5)

static void show_text(const char *texti)
{
    int first_line = 0;
    int last_line = 0;
    int next_line = 0;
    int next_page = 0;
    unsigned int current_line = 0;
    unsigned int this_line = 0;
    unsigned int len;
    int x, y, z;
    int active = 1;
    int active_keys;
    char *string;
    menu_draw_t *menu_draw;

    menu_draw = sdl_ui_get_menu_param();

	// preprocess text and include newlines if necessary
	char *text=lib_stralloc(texti);
	int i=0,c=0;
	while(text[i] != '\0'){
		if(text[i] == '\n') c=-1;
		else if(text[i] == ' ') {
			int t=1;
			while(text[i+t]!=' ' && text[i+t]!=0 && text[i+t]!='\n') ++t;
			if(c + t - 1 >= 40) {
				text[i] = '\n';
				c = -1;
			}
		}
		c++; i++;
	}
	
    string = lib_malloc(0x400);
    len = strlen(text);

    /* find out how many lines */
    for (x = 0, z = 0; text[x] != 0; x++) {
        if (text[x] == '\n') {
            last_line++;
        }
    }

    last_line -= menu_draw->max_text_y;
    for (x = 0, z = 0; text[x] != 0; x++) {
        if (last_line == 0) {
            break;
        }
        if (text[x] == '\n') {
            last_line--;
        }
    }
    last_line = x; /* save the offset */

    while (active) {
        sdl_ui_clear();
        first_line = current_line;
        this_line = current_line;
        for (y = 0; (y < menu_draw->max_text_y) && (this_line < len); y++) {
            z = 0;
            for (x = 0; (text[this_line + x] != '\n') &&
                        (text[this_line + x] != 0); x++) {
                switch (text[this_line + x]) {
                    case '`':
                        string[x + z] = '\'';
                        break;
                    /* FIXME: we should actually be able to handle some of these */
                    case CHARCODE_UMLAUT_A_LOWER:
                    case CHARCODE_KROUZEK_A_LOWER:
                        string[x + z] = 'a';
                        break;
                    case CHARCODE_UMLAUT_A_UPPER:
                        string[x + z] = 'A';
                        break;
                    case '~':
                        string[x + z] = '-';
                        break;
                    case CHARCODE_GRAVE_E_LOWER:
                    case CHARCODE_AIGU_E_LOWER:
                        string[x + z] = 'e';
                        break;
                    case CHARCODE_UMLAUT_O_UPPER:
                        string[x + z] = 'O';
                        break;
                    case CHARCODE_UMLAUT_O_LOWER:
                        string[x + z] = 'o';
                        break;
                    case CHARCODE_UMLAUT_U_UPPER:
                        string[x + z] = 'U';
                        break;
                    case CHARCODE_UMLAUT_U_LOWER:
                        string[x + z] = 'u';
                        break;
                    case '\t':
                        string[x + z] = ' ';
                        string[x + z + 1] = ' ';
                        string[x + z + 2] = ' ';
                        string[x + z + 3] = ' ';
                        z += 3;
                        break;
                    default:
                        string[x + z] = text[this_line + x];
                        break;
                }
            }
            if (x != 0) {
                string[x + z] = 0;
                sdl_ui_print(string, 0, y);
            }
            if (y == 0) {
                next_line = this_line + x + 1;
            }
            this_line += x + 1;
        }
        next_page = this_line;
        active_keys = 1;
        sdl_ui_refresh();

        while (active_keys) {
            switch (sdl_ui_menu_poll_input()) {
                case MENU_ACTION_CANCEL:
                case MENU_ACTION_EXIT:
                case MENU_ACTION_SELECT:
                    active_keys = 0;
                    active = 0;
                    break;
                case MENU_ACTION_RIGHT:
                case MENU_ACTION_PAGEDOWN:
                    active_keys = 0;
                    if (current_line < last_line) {
                        current_line = next_page;
                        if (current_line > last_line) {
                            current_line = last_line;
                        }
                    }
                    break;
                case MENU_ACTION_DOWN:
                    active_keys = 0;
                    if (current_line < last_line) {
                        current_line = next_line;
                        if (current_line > last_line) {
                            current_line = last_line;
                        }
                    }
                    break;
                case MENU_ACTION_LEFT:
                case MENU_ACTION_PAGEUP:
                    active_keys = 0;
                    current_line = scroll_up(text, first_line, menu_draw->max_text_y);
                    break;
                case MENU_ACTION_UP:
                    active_keys = 0;
                    current_line = scroll_up(text, first_line, 1);
                    break;
                case MENU_ACTION_HOME:
                    active_keys = 0;
                    current_line = 0;
                    break;
                case MENU_ACTION_END:
                    active_keys = 0;
                    if (current_line < last_line) {
                        current_line = last_line;
                    }
                    break;
                default:
                    SDL_Delay(10);
                    break;
            }
        }
    }
    lib_free(string);
	lib_free(text);
}

const char info_about_text[] =
        "              Vice3DS v" VERSION3DS "\n"
		"\n"
		"            based on VICE v" VERSION "\n"
		"       Versatile Commodore Emulator\n"
        "\n"
		"      ported by badda71 <me@badda.de>";

static UI_MENU_CALLBACK(about_callback)
{
    if (activated) {
		show_text(info_about_text);
    }
    return NULL;
}

const char info_usage_text[] =
    "USAGE\n"
    "~~~~~\n"
    "\n"
	"Default 3DS button functions:\n"
	"- Select: open Vice menu\n"
	"- Start: quit emulator\n"
	"- Circle Pad / A-Button: Joystick Port 1\n"
	"- Directional Pad / X-Button: Joystick\n"
	"  Port 2 (Keyset 1)\n"
	"- C-Stick: cursor buttons\n"
	"  Can be mapped to any joystick in Vice\n"
	"  menu using Keyset 2\n"
	"- R/ZR-buttons: autofire at joyport 1/2\n"
	"- 3D-slider: Emulation speed\n"
	"\n"
	"Default soft buttons functions (ltr,ttb)\n"
	"- Autostart image\n"
	"- Press RUN/STOP + RESTORE\n"
	"- Toggle Swap joystick ports\n"
	"- Toggle Warp mode\n"
	"- Hard reset\n"
	"- Quickload snapshot\n"
	"- Quicksave snapshot\n"
	"- Toggle True Drive Emulation\n"
	"- Power off bottom screen backlight\n"
	"- Pause emulation\n"
	"\n"
	"This (and a lot of other things) can be changed in Vice menu. To change the mapping of a soft button (or actually any other button incl. all the 3DS- and soft keyboard buttons) to a menu item in Vice menu, touch 'M' when the menu item is selected, then touch/press the button. This button will now be mapped to the selected menu entry.\n";

static UI_MENU_CALLBACK(usage_callback)
{
    if (activated) {
		show_text(info_usage_text);
    }
    return NULL;
}

static UI_MENU_CALLBACK(license_callback)
{
    menu_draw_t *menu_draw;

    if (activated) {
        menu_draw = sdl_ui_get_menu_param();
        if (menu_draw->max_text_x > 60) {
            show_text(info_license_text);
        } else {
            show_text(info_license_text40);
        }
    }
    return NULL;
}

static UI_MENU_CALLBACK(warranty_callback)
{
    menu_draw_t *menu_draw;

    if (activated) {
        menu_draw = sdl_ui_get_menu_param();
        if (menu_draw->max_text_x > 60) {
            show_text(info_warranty_text);
        } else {
            show_text(info_warranty_text40);
        }
    }
    return NULL;
}

const ui_menu_entry_t help_menu[] = {
    { "About",
      MENU_ENTRY_DIALOG,
      about_callback,
      NULL },
    { "Usage",
      MENU_ENTRY_DIALOG,
      usage_callback,
      NULL },
    { "License",
      MENU_ENTRY_DIALOG,
      license_callback,
      NULL },
    { "Warranty",
      MENU_ENTRY_DIALOG,
      warranty_callback,
      NULL },
    SDL_MENU_LIST_END
};
