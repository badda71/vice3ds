/*
 * menu_joyport.c - Joyport menu for SDL UI.
 *
 * Written by
 *  Marco van den Heuvel <blackystardust68@yahoo.com>
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

#include "types.h"

#include "menu_common.h"
#include "joyport.h"
#include "uimenu.h"
#include "lib.h"
#include "joy.h"
#include "resources.h"
#include "joystick.h"
#include "uipoll.h"
#include "kbd.h"
#include "uibottom.h"

#include "menu_joyport.h"

UI_MENU_DEFINE_RADIO(JoyPort1Device)
UI_MENU_DEFINE_RADIO(JoyPort2Device)

static ui_menu_entry_t joyport1_dyn_menu[JOYPORT_MAX_DEVICES + 1];
static ui_menu_entry_t joyport2_dyn_menu[JOYPORT_MAX_DEVICES + 1];

static int joyport1_dyn_menu_init = 0;
static int joyport2_dyn_menu_init = 0;

static void sdl_menu_joyport1_free(void)
{
    int i;

    for (i = 0; joyport1_dyn_menu[i].string != NULL; i++) {
        lib_free(joyport1_dyn_menu[i].string);
    }
}

static void sdl_menu_joyport2_free(void)
{
    int i;

    for (i = 0; joyport2_dyn_menu[i].string != NULL; i++) {
        lib_free(joyport2_dyn_menu[i].string);
    }
}

static UI_MENU_CALLBACK(JoyPort1Device_dynmenu_callback)
{
    joyport_desc_t *devices = joyport_get_valid_devices(JOYPORT_1);
    int i;
    static char buf[100] = MENU_SUBMENU_STRING " ";
    char *dest = &(buf[3]);
    const char *src = NULL;

    /* rebuild menu if it already exists. */
    if (joyport1_dyn_menu_init != 0) {
        sdl_menu_joyport1_free();
    } else {
        joyport1_dyn_menu_init = 1;
    }

    for (i = 0; devices[i].name; ++i) {
        joyport1_dyn_menu[i].string = (char *)lib_stralloc(devices[i].name);
        joyport1_dyn_menu[i].type = MENU_ENTRY_RESOURCE_RADIO;
        joyport1_dyn_menu[i].callback = radio_JoyPort1Device_callback;
        joyport1_dyn_menu[i].data = (ui_callback_data_t)int_to_void_ptr(devices[i].id);
        if (radio_JoyPort1Device_callback(0, joyport1_dyn_menu[i].data) != NULL)
            src = joyport1_dyn_menu[i].string;
    }

    joyport1_dyn_menu[i].string = NULL;
    joyport1_dyn_menu[i].type = 0;
    joyport1_dyn_menu[i].callback = NULL;
    joyport1_dyn_menu[i].data = NULL;

    lib_free(devices);

    if (src == NULL) {
        return MENU_SUBMENU_STRING " ???";
    }

    while ((*dest++ = *src++)) {
    }

    return buf;
}

static UI_MENU_CALLBACK(JoyPort2Device_dynmenu_callback)
{
    joyport_desc_t *devices = joyport_get_valid_devices(JOYPORT_2);
    int i;
    static char buf[100] = MENU_SUBMENU_STRING " ";
    char *dest = &(buf[3]);
    const char *src = NULL;

    /* rebuild menu if it already exists. */
    if (joyport2_dyn_menu_init != 0) {
        sdl_menu_joyport2_free();
    } else {
        joyport2_dyn_menu_init = 1;
    }

    for (i = 0; devices[i].name; ++i) {
        joyport2_dyn_menu[i].string = (char *)lib_stralloc(devices[i].name);
        joyport2_dyn_menu[i].type = MENU_ENTRY_RESOURCE_RADIO;
        joyport2_dyn_menu[i].callback = radio_JoyPort2Device_callback;
        joyport2_dyn_menu[i].data = (ui_callback_data_t)int_to_void_ptr(devices[i].id);
        if (radio_JoyPort2Device_callback(0, joyport2_dyn_menu[i].data) != NULL)
            src = joyport2_dyn_menu[i].string;
	}

    joyport2_dyn_menu[i].string = NULL;
    joyport2_dyn_menu[i].type = 0;
    joyport2_dyn_menu[i].callback = NULL;
    joyport2_dyn_menu[i].data = NULL;

    lib_free(devices);

    if (src == NULL) {
        return MENU_SUBMENU_STRING " ???";
    }

    while ((*dest++ = *src++)) {
    }

    return buf;
}

static UI_MENU_CALLBACK(custom_swap_ports_callback)
{
    if (activated) {
        sdljoy_swap_ports();
    }
    return sdljoy_get_swap_ports() ? MENU_CHECKMARK_CHECKED_STRING : NULL;
}

static UI_MENU_CALLBACK(custom_joy_auto_speed_callback)
{
    static char buf[20];
    int previous, new_value;

    resources_get_int("JoyAutoSpeed", &previous);

    if (activated) {
        new_value = sdl_ui_slider_input_dialog("Select speed (clks/s)", previous, 2, 100);
        if (new_value != previous) {
            resources_set_int("JoyAutoSpeed", joy_auto_speed=new_value);
        }
    } else {
        sprintf(buf, "%3i", previous);
        return buf;
    }
    return NULL;
}

UI_MENU_DEFINE_TOGGLE(JoyOpposite)

static UI_MENU_CALLBACK(custom_keyset_callback)
{
    SDL_Event e;
    int previous;

    if (activated) {
        e = sdl_ui_poll_event("key", (const char *)param, SDL_POLL_KEYBOARD | SDL_POLL_MODIFIER, 5);

        if (e.type == SDL_KEYDOWN) {
            resources_set_int((const char *)param, (int)SDL2x_to_SDL1x_Keys(e.key.keysym.sym));
        }
		// recalc the soft buttons just in case the mapping was done there
		uibottom_must_redraw |= UIB_RECALC_SBUTTONS;
    } else {
		if (resources_get_int((const char *)param, &previous)) {
			return sdl_menu_text_unknown;
		}
		return get_3ds_keyname(previous);
    }
    return NULL;
}

static const ui_menu_entry_t define_keyset_menu[] = {
    { "Keyset Up",
      MENU_ENTRY_DIALOG,
      custom_keyset_callback,
      (ui_callback_data_t)"KeySet1North" },
    { "Keyset Down",
      MENU_ENTRY_DIALOG,
      custom_keyset_callback,
      (ui_callback_data_t)"KeySet1South" },
    { "Keyset Left",
      MENU_ENTRY_DIALOG,
      custom_keyset_callback,
      (ui_callback_data_t)"KeySet1West" },
    { "Keyset Right",
      MENU_ENTRY_DIALOG,
      custom_keyset_callback,
      (ui_callback_data_t)"KeySet1East" },
    { "Keyset Fire",
      MENU_ENTRY_DIALOG,
      custom_keyset_callback,
      (ui_callback_data_t)"KeySet1Fire" },
    { "Keyset Autofire",
      MENU_ENTRY_DIALOG,
      custom_keyset_callback,
      (ui_callback_data_t)"KeySet1AFire" },
    SDL_MENU_LIST_END
};

ui_menu_entry_t joyport_menu[] = {
	{ "Control port 1",
	  MENU_ENTRY_DYNAMIC_SUBMENU,
	  JoyPort1Device_dynmenu_callback,
	  (ui_callback_data_t)joyport1_dyn_menu},
	{ "Control port 2",
	  MENU_ENTRY_DYNAMIC_SUBMENU,
	  JoyPort2Device_dynmenu_callback,
	  (ui_callback_data_t)joyport2_dyn_menu},
	{ "Swap joyports",
      MENU_ENTRY_OTHER_TOGGLE,
      custom_swap_ports_callback,
      NULL },
	SDL_MENU_ITEM_SEPARATOR,
    SDL_MENU_ITEM_TITLE("Joystick settings"),
	{ "Joystick Autofire speed",
      MENU_ENTRY_DIALOG,
      custom_joy_auto_speed_callback,
      NULL },
    { "Allow opposite directions",
      MENU_ENTRY_RESOURCE_TOGGLE,
      toggle_JoyOpposite_callback,
      NULL },
    { "Define keyset",
      MENU_ENTRY_SUBMENU,
      submenu_callback,
      (ui_callback_data_t)define_keyset_menu },
	SDL_MENU_LIST_END
};

/** \brief  Clean up memory used by the dynamically created joyport menus
 */
void uijoyport_menu_shutdown(void)
{
    if (joyport1_dyn_menu_init) {
        sdl_menu_joyport1_free();
    }
    if (joyport2_dyn_menu_init) {
        sdl_menu_joyport2_free();
    }
}

