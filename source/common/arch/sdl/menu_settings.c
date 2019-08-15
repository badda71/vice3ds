/*
 * menu_settings.c - Common SDL settings related functions.
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
#include "types.h"

#include "vice_sdl.h"
#include <stdlib.h>
#include "archdep.h"

#include "joy.h"
#include "kbd.h"
#include "keyboard.h"
#include "lib.h"
#include "machine.h"
#include "menu_common.h"
#include "menu_settings.h"
#include "resources.h"
#include "ui.h"
#include "uifilereq.h"
#include "uimenu.h"
#include "uipoll.h"
#include "archdep_user_config_path.h"
#include "archdep_join_paths.h"
#include "uimsgbox.h"
#include "archdep_cp.h"

static UI_MENU_CALLBACK(save_settings_callback)
{
    if (activated) {
        if (resources_save(NULL) < 0) {
            ui_error("Cannot save current settings.");
        } else {
            ui_message("Settings saved.");
        }

        /* TODO:
           to be uncommented after the fliplist menus have been made
        uifliplist_save_settings(); */
    }
    return NULL;
}

static UI_MENU_CALLBACK(save_settings_to_callback)
{
    if (activated) {
        char *name = NULL;

        name = sdl_ui_file_selection_dialog("Choose settings file", FILEREQ_MODE_SAVE_FILE);

        if (name != NULL) {
            if (resources_save(name) < 0) {
                ui_error("Cannot save current settings.");
            } else {
                ui_message("Settings saved.");
            }
            lib_free(name);
        }
    }
    return NULL;
}

static UI_MENU_CALLBACK(load_settings_callback)
{
    if (activated) {
        if (resources_load(NULL) < 0) {
            ui_error("Cannot load settings.");
        } else {
            ui_message("Settings loaded.");
        }
    }
    return NULL;
}

static UI_MENU_CALLBACK(load_settings_from_callback)
{
    if (activated) {
        char *name = NULL;

        name = sdl_ui_file_selection_dialog("Choose settings file", FILEREQ_MODE_CHOOSE_FILE);

        if (name != NULL) {
            if (resources_load(name) < 0) {
                ui_error("Cannot load settings.");
            } else {
                ui_message("Settings loaded.");
            }
            lib_free(name);
        }
    }
    return NULL;
}

static UI_MENU_CALLBACK(default_settings_callback)
{
    if (activated) {
		// copy the default config file to the config directory and restart
		if (message_box("VICE QUESTION", "Do you really want to restore the default settings? This will overwrite your vicerc-file.", MESSAGE_YESNO) != 0) {
            return NULL;
        }

	    char *cfg = archdep_user_config_path();
	    char *f = archdep_join_paths(cfg, "sdl-vicerc", NULL);

		if (xcopy("romfs:/config/sdl-vicerc", f, 1, NULL)==0) {
			ui_message("Default settings restored - please restart VICE");
			archdep_vice_exit(0);
		} else {
			ui_message("Default settings could not be restored");
		}
		lib_free(f);
    }
    return NULL;
}

static UI_MENU_CALLBACK(all_default_settings_callback)
{
	if (activated) {
		// copy the complete config directory to SD-card and restart
		if (message_box("VICE QUESTION", "Do you really want to restore all the default settings?\nThis will overwrite all your config files including keymaps, joymaps and vicerc.", MESSAGE_YESNO) != 0) {
            return NULL;
        }

		if (xcopy("romfs:/config", archdep_user_config_path(), 1, NULL)==0) {
			ui_message("All default settings restored - please restart VICE");
			archdep_vice_exit(0);
		} else {
			ui_message("Default settings could not be restored");
		}
    }
    return NULL;
}
/* 3DS
static UI_MENU_CALLBACK(save_keymap_callback)
{
    if (activated) {
        char *name = NULL;

        name = sdl_ui_file_selection_dialog("Choose file for keymap", FILEREQ_MODE_SAVE_FILE);

        if (name != NULL) {
            if (keyboard_keymap_dump(name) < 0) {
                ui_error("Cannot save keymap.");
            }
            lib_free(name);
        }
    }
    return NULL;
}

UI_MENU_DEFINE_RADIO(KeymapIndex)
static const ui_menu_entry_t keymap_index_submenu[] = {
    { "Symbolic",
      MENU_ENTRY_RESOURCE_RADIO,
      radio_KeymapIndex_callback,
      (ui_callback_data_t)KBD_INDEX_SYM },
    { "Positional",
      MENU_ENTRY_RESOURCE_RADIO,
      radio_KeymapIndex_callback,
      (ui_callback_data_t)KBD_INDEX_POS },
    { "Symbolic (user)",
      MENU_ENTRY_RESOURCE_RADIO,
      radio_KeymapIndex_callback,
      (ui_callback_data_t)KBD_INDEX_USERSYM },
    { "Positional (user)",
      MENU_ENTRY_RESOURCE_RADIO,
      radio_KeymapIndex_callback,
      (ui_callback_data_t)KBD_INDEX_USERPOS },
    SDL_MENU_LIST_END
};
*/
UI_MENU_DEFINE_RADIO(KeyboardMapping)

static ui_menu_entry_t *keyboard_mapping_submenu;

ui_menu_entry_t ui_keyboard_mapping_entry = {
    NULL, MENU_ENTRY_RESOURCE_RADIO, (ui_callback_t)radio_KeyboardMapping_callback,
    (ui_callback_data_t)0
};

void uikeyboard_menu_create(void)
{
    int num;
    mapping_info_t *kbdlist = keyboard_get_info_list();
    ui_menu_entry_t *entry;

    num = keyboard_get_num_mappings();
    entry = keyboard_mapping_submenu = lib_malloc(sizeof(ui_menu_entry_t) * (num + 1));
    while(num) {
        ui_keyboard_mapping_entry.string = kbdlist->name;
        ui_keyboard_mapping_entry.data = (ui_callback_data_t)(int_to_void_ptr(kbdlist->mapping));
        memcpy(entry, &ui_keyboard_mapping_entry, sizeof(ui_menu_entry_t));
        entry++;
        kbdlist++;
        num--;
    }
    memset(entry, 0, sizeof(ui_menu_entry_t));
    settings_manager_menu[11].data = keyboard_mapping_submenu;
}


void uikeyboard_menu_shutdown(void)
{
    lib_free(settings_manager_menu[11].data);
}


/*
static UI_MENU_CALLBACK(load_sym_keymap_callback)
{
    // temporary fix until I find out what 'keymap' is supposed to be
#ifdef SDL_DEBUG
    int keymap = -1;
#endif
    if (activated) {
        char *name = NULL;

#ifdef SDL_DEBUG
        fprintf(stderr, "%s: map %i, \"%s\"\n", __func__, keymap, "KeymapUserSymFile");
#endif

        name = sdl_ui_file_selection_dialog("Choose keymap file", FILEREQ_MODE_CHOOSE_FILE);

        if (name != NULL) {
            if (resources_set_string("KeymapUserSymFile", name)) {
                ui_error("Cannot load keymap.");
            }
            lib_free(name);
        }
    }
    return NULL;
}

static UI_MENU_CALLBACK(load_pos_keymap_callback)
{
    // temporary fix until I find out what 'keymap' is supposed to be
#ifdef SDL_DEBUG
    int keymap = -1;
#endif

    if (activated) {
        char *name = NULL;

#ifdef SDL_DEBUG
        fprintf(stderr, "%s: map %i, \"%s\"\n", __func__, keymap, "KeymapUserPosFile");
#endif

        name = sdl_ui_file_selection_dialog("Choose keymap file", FILEREQ_MODE_CHOOSE_FILE);

        if (name != NULL) {
            if (resources_set_string("KeymapUserPosFile", name)) {
                ui_error("Cannot load keymap.");
            }
            lib_free(name);
        }
    }
    return NULL;
}

static UI_MENU_CALLBACK(save_hotkeys_callback)
{
    const char *file = NULL;
    if (activated) {
        if (resources_get_string("HotkeyFile", &file)) {
            ui_error("Cannot find resource.");
            return NULL;
        }
	    char *cfg = archdep_user_config_path();
	    char *f = archdep_join_paths(cfg, file, NULL);

        if (sdlkbd_hotkeys_dump(f)) {
            ui_error("Cannot save hotkeys.");
        } else {
            ui_message("Hotkeys saved.");
        }
		lib_free(f);
    }
    return NULL;
}

static UI_MENU_CALLBACK(save_hotkeys_to_callback)
{
    if (activated) {
        char *name = NULL;

        name = sdl_ui_file_selection_dialog("Choose hotkey file", FILEREQ_MODE_SAVE_FILE);

        if (name != NULL) {
            if (sdlkbd_hotkeys_dump(name) < 0) {
                ui_error("Cannot save current hotkeys.");
            } else {
                ui_message("Hotkeys saved.");
            }
            lib_free(name);
        }
    }
    return NULL;
}

static UI_MENU_CALLBACK(load_hotkeys_callback)
{
    const char *file = NULL;
    if (activated) {
        if (resources_get_string("HotkeyFile", &file)) {
            ui_error("Cannot find resource.");
            return NULL;
        }

        if (sdlkbd_hotkeys_load(file)) {
            ui_error("Cannot load hotkeys.");
        } else {
            ui_message("Hotkeys loaded.");
        }
    }
    return NULL;
}

static UI_MENU_CALLBACK(load_hotkeys_from_callback)
{
    if (activated) {
        char *name = NULL;

        name = sdl_ui_file_selection_dialog("Choose hotkeys file", FILEREQ_MODE_CHOOSE_FILE);

        if (name != NULL) {
            if (resources_set_string("HotkeyFile", name) < 0) {
                ui_error("Cannot load hotkeys.");
            } else {
                ui_message("Hotkeys loaded.");
            }
            lib_free(name);
        }
    }
    return NULL;
}

#ifdef HAVE_SDL_NUMJOYSTICKS
static UI_MENU_CALLBACK(save_joymap_callback)
{
    const char *file = NULL;
    if (activated) {
        if (resources_get_string("JoyMapFile", &file)) {
            ui_error("Cannot find resource.");
            return NULL;
        }
	    char *cfg = archdep_user_config_path();
	    char *f = archdep_join_paths(cfg, file, NULL);

        if (joy_arch_mapping_dump(f)) {
            ui_error("Cannot save joymap.");
        } else {
            ui_message("Joymap saved.");
        }
		lib_free(f);
    }
    return NULL;
}

static UI_MENU_CALLBACK(save_joymap_to_callback)
{
    if (activated) {
        char *name = NULL;

        name = sdl_ui_file_selection_dialog("Choose joystick map file", FILEREQ_MODE_SAVE_FILE);

        if (name != NULL) {
            if (joy_arch_mapping_dump(name) < 0) {
                ui_error("Cannot save joymap.");
            } else {
                ui_message("Joymap saved.");
            }
            lib_free(name);
        }
    }
    return NULL;
}

static UI_MENU_CALLBACK(load_joymap_callback)
{
    const char *file = NULL;
    if (activated) {
        if (resources_get_string("JoyMapFile", &file)) {
            ui_error("Cannot find resource.");
            return NULL;
        }

        if (joy_arch_mapping_load(file)) {
            ui_error("Cannot load joymap.");
        } else {
            ui_message("Joymap loaded.");
        }
    }
    return NULL;
}

static UI_MENU_CALLBACK(load_joymap_from_callback)
{
    if (activated) {
        char *name = NULL;

        name = sdl_ui_file_selection_dialog("Choose joystick map file", FILEREQ_MODE_CHOOSE_FILE);

        if (name != NULL) {
            if (resources_set_string("JoyMapFile", name) < 0) {
                ui_error("Cannot load joymap.");
            } else {
                ui_message("Joymap loaded.");
            }
            lib_free(name);
        }
    }
    return NULL;
}
#endif
*/

UI_MENU_DEFINE_TOGGLE(SaveResourcesOnExit)
UI_MENU_DEFINE_TOGGLE(ConfirmOnExit)

/*
static UI_MENU_CALLBACK(custom_ui_keyset_callback)
{
    SDL_Event e;
    int previous;

    if (resources_get_int((const char *)param, &previous)) {
        return sdl_menu_text_unknown;
    }

    if (activated) {
        e = sdl_ui_poll_event("key", (const char *)param, SDL_POLL_KEYBOARD | SDL_POLL_MODIFIER, 5);

        if (e.type == SDL_KEYDOWN) {
            resources_set_int((const char *)param, (int)SDL2x_to_SDL1x_Keys(e.key.keysym.sym));
        }
    } else {
        return get_3ds_keyname(previous);
    }
    return NULL;
}

static const ui_menu_entry_t define_ui_keyset_menu[] = {
    { "Activate menu",
      MENU_ENTRY_DIALOG,
      custom_ui_keyset_callback,
      (ui_callback_data_t)"MenuKey" },
    { "Menu up",
      MENU_ENTRY_DIALOG,
      custom_ui_keyset_callback,
      (ui_callback_data_t)"MenuKeyUp" },
    { "Menu down",
      MENU_ENTRY_DIALOG,
      custom_ui_keyset_callback,
      (ui_callback_data_t)"MenuKeyDown" },
    { "Menu left",
      MENU_ENTRY_DIALOG,
      custom_ui_keyset_callback,
      (ui_callback_data_t)"MenuKeyLeft" },
    { "Menu right",
      MENU_ENTRY_DIALOG,
      custom_ui_keyset_callback,
      (ui_callback_data_t)"MenuKeyRight" },
    { "Menu page up",
      MENU_ENTRY_DIALOG,
      custom_ui_keyset_callback,
      (ui_callback_data_t)"MenuKeyPageUp" },
    { "Menu page down",
      MENU_ENTRY_DIALOG,
      custom_ui_keyset_callback,
      (ui_callback_data_t)"MenuKeyPageDown" },
    { "Menu home",
      MENU_ENTRY_DIALOG,
      custom_ui_keyset_callback,
      (ui_callback_data_t)"MenuKeyHome" },
    { "Menu end",
      MENU_ENTRY_DIALOG,
      custom_ui_keyset_callback,
      (ui_callback_data_t)"MenuKeyEnd" },
    { "Menu select",
      MENU_ENTRY_DIALOG,
      custom_ui_keyset_callback,
      (ui_callback_data_t)"MenuKeySelect" },
    { "Menu cancel",
      MENU_ENTRY_DIALOG,
      custom_ui_keyset_callback,
      (ui_callback_data_t)"MenuKeyCancel" },
    { "Menu exit",
      MENU_ENTRY_DIALOG,
      custom_ui_keyset_callback,
      (ui_callback_data_t)"MenuKeyExit" },
    { "Menu map",
      MENU_ENTRY_DIALOG,
      custom_ui_keyset_callback,
      (ui_callback_data_t)"MenuKeyMap" },
    SDL_MENU_LIST_END
};*/

UI_MENU_DEFINE_TOGGLE(SDLKbdStatusbar)
UI_MENU_DEFINE_TOGGLE(SDLStatusbar)

ui_menu_entry_t settings_manager_menu[] = {
     SDL_MENU_ITEM_TITLE("VICE Settings"),
   { "Save current settings",
      MENU_ENTRY_OTHER,
      save_settings_callback,
      NULL },
    { "Load settings",
      MENU_ENTRY_OTHER,
      load_settings_callback,
      NULL },
    { "Save current settings to",
      MENU_ENTRY_OTHER,
      save_settings_to_callback,
      NULL },
    { "Load settings from",
      MENU_ENTRY_OTHER,
      load_settings_from_callback,
      NULL },
    { "Restore default settings",
      MENU_ENTRY_OTHER,
      default_settings_callback,
      NULL },
    { "Restore ALL default settings",
      MENU_ENTRY_OTHER,
      all_default_settings_callback,
      NULL },
    SDL_MENU_ITEM_SEPARATOR,
    SDL_MENU_ITEM_TITLE("Actions on exit"),
    { "Save settings on exit",
      MENU_ENTRY_RESOURCE_TOGGLE,
      toggle_SaveResourcesOnExit_callback,
      NULL },
    { "Confirm on exit",
      MENU_ENTRY_RESOURCE_TOGGLE,
      toggle_ConfirmOnExit_callback,
      NULL },
/* 3DS
	{ "Active keymap",
      MENU_ENTRY_SUBMENU,
      submenu_radio_callback,
      (ui_callback_data_t)keymap_index_submenu },
    // CAUTION: the position of this item is hardcoded above
    { "Keyboard mapping",
      MENU_ENTRY_SUBMENU,
      submenu_radio_callback,
      (ui_callback_data_t)NULL },
    { "Load symbolic user keymap",
      MENU_ENTRY_OTHER,
      load_sym_keymap_callback,
      NULL },
    { "Load positional user keymap",
      MENU_ENTRY_OTHER,
      load_pos_keymap_callback,
      NULL },
    { "Save current keymap to",
      MENU_ENTRY_OTHER,
      save_keymap_callback,
      NULL },
*/
    SDL_MENU_ITEM_SEPARATOR,
    SDL_MENU_ITEM_TITLE("Statusbar"),
    { "Show statusbar",
      MENU_ENTRY_RESOURCE_TOGGLE,
      toggle_SDLStatusbar_callback,
      NULL },
	{ "Show keyboard status in statusbar",
      MENU_ENTRY_RESOURCE_TOGGLE,
      toggle_SDLKbdStatusbar_callback,
      NULL },
 /* 3DS
	SDL_MENU_ITEM_SEPARATOR,
    { "Save hotkeys",
      MENU_ENTRY_OTHER,
      save_hotkeys_callback,
      NULL },
    { "Save hotkeys to",
      MENU_ENTRY_OTHER,
      save_hotkeys_to_callback,
      NULL },
    { "Load hotkeys from",
      MENU_ENTRY_OTHER,
      load_hotkeys_from_callback,
      NULL },
    { "Load hotkeys",
      MENU_ENTRY_OTHER,
      load_hotkeys_callback,
      NULL },
#ifdef HAVE_SDL_NUMJOYSTICKS
    SDL_MENU_ITEM_SEPARATOR,
    { "Save joystick map",
      MENU_ENTRY_OTHER,
      save_joymap_callback,
      NULL },
    { "Save joystick map to",
      MENU_ENTRY_OTHER,
      save_joymap_to_callback,
      NULL },
    { "Load joystick map from",
      MENU_ENTRY_OTHER,
      load_joymap_from_callback,
      NULL },
    { "Load joystick map",
      MENU_ENTRY_OTHER,
      load_joymap_callback,
      NULL },
#endif
    SDL_MENU_ITEM_SEPARATOR,
    { "Define UI keys",
      MENU_ENTRY_SUBMENU,
      submenu_callback,
      (ui_callback_data_t)define_ui_keyset_menu },*/
    SDL_MENU_LIST_END
};


/* vsid setting menu */
ui_menu_entry_t settings_manager_menu_vsid[] = {
    SDL_MENU_ITEM_TITLE("VICE Settings"),
    { "Save current settings",
      MENU_ENTRY_OTHER,
      save_settings_callback,
      NULL },
    { "Load settings",
      MENU_ENTRY_OTHER,
      load_settings_callback,
      NULL },
    { "Save current settings to",
      MENU_ENTRY_OTHER,
      save_settings_to_callback,
      NULL },
    { "Load settings from",
      MENU_ENTRY_OTHER,
      load_settings_from_callback,
      NULL },
    { "Restore default settings",
      MENU_ENTRY_OTHER,
      default_settings_callback,
      NULL },
    { "Restore ALL default settings",
      MENU_ENTRY_OTHER,
      all_default_settings_callback,
      NULL },
    SDL_MENU_ITEM_SEPARATOR,
    SDL_MENU_ITEM_TITLE("Actions on exit"),
    { "Save settings on exit",
      MENU_ENTRY_RESOURCE_TOGGLE,
      toggle_SaveResourcesOnExit_callback,
      NULL },
    { "Confirm on exit",
      MENU_ENTRY_RESOURCE_TOGGLE,
      toggle_ConfirmOnExit_callback,
      NULL },
/*
	SDL_MENU_ITEM_SEPARATOR,
    { "Save hotkeys",
      MENU_ENTRY_OTHER,
      save_hotkeys_callback,
      NULL },
    { "Save hotkeys to",
      MENU_ENTRY_OTHER,
      save_hotkeys_to_callback,
      NULL },
    { "Load hotkeys from",
      MENU_ENTRY_OTHER,
      load_hotkeys_from_callback,
      NULL },
    { "Load hotkeys",
      MENU_ENTRY_OTHER,
      load_hotkeys_callback,
      NULL },
      SDL_MENU_ITEM_SEPARATOR,
    { "Define UI keys",
      MENU_ENTRY_SUBMENU,
      submenu_callback,
      (ui_callback_data_t)define_ui_keyset_menu },*/
    SDL_MENU_LIST_END
};
