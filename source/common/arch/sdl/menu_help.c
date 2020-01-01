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
#include "uipoll.h"
#include "util.h"
#include "version.h"
#include "vicefeatures.h"
#include "uibottom.h"
#include "kbd.h"
#include "vice3ds.h"

const char info_about_text[] =
        "                Vice3DS\n"
		"             v" VERSION3DS " (" GITHASH ")\n"
		"\n"
		"           based on VICE v" VERSION "\n"
		"      Versatile Commodore Emulator\n"
        "\n"
		"    ported by badda71 <me@badda.de>\n"
		"   https://github.com/badda71/vice3ds\n";

UI_MENU_DEFINE_TOGGLE(HelpButtonOn)

static UI_MENU_CALLBACK(about_callback)
{
    if (activated) {
		ui_show_text("About", info_about_text);
    }
    return NULL;
}

static UI_MENU_CALLBACK(license_callback)
{
    menu_draw_t *menu_draw;

    if (activated) {
        menu_draw = sdl_ui_get_menu_param();
        if (menu_draw->max_text_x > 60) {
            ui_show_text("License",info_license_text);
        } else {
            ui_show_text("License",info_license_text40);
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
            ui_show_text("Warranty",info_warranty_text);
        } else {
            ui_show_text("Warranty",info_warranty_text40);
        }
    }
    return NULL;
}

static UI_MENU_CALLBACK(helpscreen_callback)
{
    if (activated) {
		toggle_help(0);
        return sdl_menu_text_exit_ui;
    }
    return NULL;
}

static UI_MENU_CALLBACK(help_text_add_callback)
{
	SDL_Event e;
	char buf[40];
    char *value = NULL;

	if (activated) {
		SDL_Event s = sdl_ui_poll_event("Key", "Custom Help Text", SDL_POLL_KEYBOARD, 5);
		if (s.type == SDL_KEYDOWN) {
			while (SDL_PollEvent(&e)); // clear event queue
			int key=s.key.keysym.sym;
			if (key>0 && key<HELPTEXT_MAX) {
				snprintf(buf, 40, "Custom Help Text for: %s", get_3ds_keyname(key));
				value = sdl_ui_text_input_dialog(buf, custom_help_text[key]?custom_help_text[key]:"");
				if (value) {
					if (custom_help_text[key]) free(custom_help_text[key]);
					if (value[0]==0) {
						custom_help_text[key]=NULL;
						free(value);
						uibottom_must_redraw |= UIB_RECALC_SBUTTONS;
						ui_message("Deleted help text for key %s", get_3ds_keyname(key));
					} else {
						custom_help_text[key]=value;
						uibottom_must_redraw |= UIB_RECALC_SBUTTONS;
						ui_message("Set help text for key %s to \"%s\"", get_3ds_keyname(key),value);
					}
					save_help_texts_to_resource();
					// recalc sbuttons in case the help text is stated there
				}
			}
		}
	}
	return NULL;
}

static UI_MENU_CALLBACK(help_text_list_callback)
{
	int i,count=0;
	char *buf;
	if (activated) {
		for (i=1;i<HELPTEXT_MAX;i++)
			if (custom_help_text[i]!=0) count++;

		buf=malloc(40*(count+3));
		buf[0]=0;

		if (count == 0) {
			sprintf(buf+strlen(buf),"-- NONE --");
		} else {
			for (int i=1;i<HELPTEXT_MAX;i++) {
				if (custom_help_text[i] == NULL) continue;
				snprintf(buf+strlen(buf),41,"%-10s: %s",get_3ds_keyname(i), custom_help_text[i]);
				sprintf(buf+strlen(buf),"\n");
			}
		}
		ui_show_text("Custom Help Texts",buf);
		free(buf);
	}
	return NULL;
}

const ui_menu_entry_t help_menu[] = {
    { "About",
      MENU_ENTRY_DIALOG,
      about_callback,
      NULL },
	{ "License",
      MENU_ENTRY_DIALOG,
      license_callback,
      NULL },
    { "Warranty",
      MENU_ENTRY_DIALOG,
      warranty_callback,
      NULL },
    { "Help screen",
      MENU_ENTRY_DIALOG,
      helpscreen_callback,
      NULL },
    { "Show help button on bottom screen",
      MENU_ENTRY_RESOURCE_TOGGLE,
      toggle_HelpButtonOn_callback,
      NULL },
	SDL_MENU_ITEM_SEPARATOR,
    SDL_MENU_ITEM_TITLE("Custom help texts"),
	{ "Add/delete custom help text",
      MENU_ENTRY_DIALOG,
      help_text_add_callback,
      NULL },
	{ "List custom help texts",
      MENU_ENTRY_DIALOG,
      help_text_list_callback,
      NULL },
	SDL_MENU_LIST_END
};
