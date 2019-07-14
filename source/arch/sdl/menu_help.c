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
		ui_show_text(info_about_text);
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
	"- Attach disk image to drive 8\n"
	"- Toggle Sprite-Sprite collisions\n"
	"- Toggle Sprite-Background collisions\n"
	"- Type command: LOAD\"*\",8,1 RUN\n"
	"- Type command: LOAD\"$\",8 LIST\n"
	"- Hit RUN/STOP\n"
	"- Hit SPACE\n"
	"- Hit RETURN\n"
	"- Enable Mousepad\n"
	"- Toggle No borders / Fullscreen\n"
	"\n"
	"This (and a lot of other things) can be changed in Vice menu. To change the mapping of a soft button (or actually any other button incl. all the 3DS- and soft keyboard buttons) to a menu item in Vice menu, press the L-button when the menu item is selected, then touch/press the button. This button will now be mapped to the selected menu entry.\n";

static UI_MENU_CALLBACK(usage_callback)
{
    if (activated) {
		ui_show_text(info_usage_text);
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
