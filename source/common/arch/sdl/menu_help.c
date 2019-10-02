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
#include "uibottom.h"

const char info_about_text[] =
        "              Vice3DS v" VERSION3DS "\n"
		"\n"
		"            based on VICE v" VERSION "\n"
		"       Versatile Commodore Emulator\n"
        "\n"
		"      ported by badda71 <me@badda.de>";

UI_MENU_DEFINE_TOGGLE(HelpButtonOn)

static UI_MENU_CALLBACK(about_callback)
{
    if (activated) {
		ui_show_text(info_about_text);
    }
    return NULL;
}

static UI_MENU_CALLBACK(license_callback)
{
    menu_draw_t *menu_draw;

    if (activated) {
        menu_draw = sdl_ui_get_menu_param();
        if (menu_draw->max_text_x > 60) {
            ui_show_text(info_license_text);
        } else {
            ui_show_text(info_license_text40);
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
            ui_show_text(info_warranty_text);
        } else {
            ui_show_text(info_warranty_text40);
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
	SDL_MENU_LIST_END
};
