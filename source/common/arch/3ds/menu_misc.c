/*
 * menu_misc.h - Implementation of the misc stuff menu for the SDL UI.
 *
 * Written by
 *  Sebastian Weber <me@badda.de>
 *
 * This file is part of VICE3DS
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

#include "charset.h"
#include "fullscreenarch.h"
#include "kbd.h"
#include "kbdbuf.h"
#include "lib.h"
#include "log.h"
#include "machine.h"
#include "menu_common.h"
#include "resources.h"
#include "ui.h"
#include "uibottom.h"
#include "uihotkey.h"
#include "uipoll.h"
#include "vice.h"
#include "vice3ds.h"
#include "videoarch.h"
#include <3ds.h>

static void kb_feed(const char *text) {
	char *text_in_petscii = lib_stralloc(text);
	charset_petconvstring((unsigned char*)text_in_petscii, 0);
	kbdbuf_feed(text_in_petscii);
	lib_free(text_in_petscii);
}

static UI_MENU_CALLBACK(toggle_bottomoff_callback)
{
	int r=getBottomBacklight();
	if (activated) {
		setBottomBacklight(r=!r);
    }
	return !r ? sdl_menu_text_tick : NULL;
}

static UI_MENU_CALLBACK(add_keymapping_callback)
{
	SDL_Event e;
	int m[3]={0,0,0};
	char buf[40];

	if (activated) {
		SDL_Event s = sdl_ui_poll_event("source key", 
			(int)param & SDL_POLL_JOYSTICK ? "Key->Joystick mapping" : "Key->Key mapping",
			SDL_POLL_KEYBOARD, 5);
        if (s.type == SDL_KEYDOWN) {
			for (int x=0; x<3 ;++x) {
				while (SDL_PollEvent(&e)); // clear event queue
				snprintf(buf, 40, "map target %d of max 3", x+1);
				SDL_Event t = sdl_ui_poll_event(buf, get_3ds_keyname(s.key.keysym.sym),
					(int)param, 5);
				if (t.type != SDL_KEYDOWN) break;
				m[x]=t.key.keysym.sym;
			}
			if (m[0]!=0) {
				set_3ds_mapping(s.key.keysym.sym, 1, m[0], m[1], m[2]); // set mapping
				// recalc the soft buttons just in case the mapping was done there
				uibottom_must_redraw |= UIB_RECALC_SBUTTONS;
				ui_message("Set key mapping for %s", get_3ds_keyname(s.key.keysym.sym));
			}
		}
	}
	return NULL;
}

static UI_MENU_CALLBACK(del_keymapping_callback)
{
	if (activated) {
		SDL_Event s = sdl_ui_poll_event("source key", "Key mapping",
			SDL_POLL_KEYBOARD, 5);
        if (s.type == SDL_KEYDOWN) {
			set_3ds_mapping(s.key.keysym.sym, 0, 0, 0, 0); // clear mapping
			// recalc the soft buttons just in case the mapping was done there
			uibottom_must_redraw |= UIB_RECALC_SBUTTONS;
			ui_message("Removed mapping for %s", get_3ds_keyname(s.key.keysym.sym));
		}
	}
	return NULL;
}

static UI_MENU_CALLBACK(add_keymousemapping_callback)
{
	if (activated) {
		SDL_Event s = sdl_ui_poll_event("source key", 
			(int)param == SDL_BUTTON_LEFT ? "Key->Mousebutton LEFT mapping" : "Key->Mousebutton RIGHT mapping",
			SDL_POLL_KEYBOARD, 5);
		if (s.type == SDL_KEYDOWN) {
			set_3ds_mapping(s.key.keysym.sym, 2, (int)param, 0, 0);
			// recalc the soft buttons just in case the mapping was done there
			uibottom_must_redraw |= UIB_RECALC_SBUTTONS;
			ui_message("Set mouse button mapping for %s", get_3ds_keyname(s.key.keysym.sym));
		}
	}
	return NULL;
}

static UI_MENU_CALLBACK(list_keymappings_callback)
{
	int i,count=0;
	char *buf;
	if (activated) {
		for (i=1;i<255;i++)
			if (keymap3ds[i]!=0) count++;

		buf=malloc(40*(count+3));
		buf[0]=0;

		if (count == 0) {
			sprintf(buf+strlen(buf),"-- NONE --");
		} else {
			for (int i=1;i<254;i++) {
				if (keymap3ds[i] == 0) continue;
				snprintf(buf+strlen(buf),41,"%-10s: %s",get_3ds_keyname(i), get_3ds_mapping_name(i));
				sprintf(buf+strlen(buf),"\n");
			}
		}
		ui_show_text("Key Mappings",buf);
		free(buf);
	}
	return NULL;
}

static UI_MENU_CALLBACK(toggle_hidekeyboard_callback)
{
	int r=is_keyboard_hidden();
	if (activated) {
		toggle_keyboard();
		r=!r;
	}
	return r ? sdl_menu_text_tick : NULL;
}

static UI_MENU_CALLBACK(toggle_editmode_callback)
{
	int r=uibottom_editmode_is_on();
	if (activated) {
		uibottom_toggle_editmode();
		r=!r;
		return sdl_menu_text_exit_ui;
	}
	return r ? sdl_menu_text_tick : NULL;
}

static UI_MENU_CALLBACK(list_hotkeys_callback)
{
	int i,count=0,k;
	char *hotkey_path,*p;

	char *buf;
	if (activated) {
		for (i = 0; i < SDLKBD_UI_HOTKEYS_MAX; ++i) {
			if (sdlkbd_ui_hotkeys[i]) count++;
		}

		buf=malloc(40*(count+3));
		buf[0]=0;

		if (count == 0) {
			sprintf(buf+strlen(buf),"-- NONE --");
		} else {
			for (i = 0; i < SDLKBD_UI_HOTKEYS_MAX; ++i) {
				if (sdlkbd_ui_hotkeys[i]) {
					p = hotkey_path = sdl_ui_hotkey_path(sdlkbd_ui_hotkeys[i]);
					k=strlen(get_3ds_keyname(i));
					if(k<9) k=9;
					while( strlen(p) > 38-k && strchr(p,'&')) p=strchr(p,'&')+1;
					snprintf(buf+strlen(buf),41,"%-9s: %s",get_3ds_keyname(i), p);
					sprintf(buf+strlen(buf),"\n");
					lib_free(hotkey_path);
				}
			}
		}
		ui_show_text("Hotkeys",buf);
		free(buf);
	}
	return NULL;
}

static UI_MENU_CALLBACK(unmap_hotkey_callback)
{
	if (activated) sdl_ui_hotkey_map((void*)1, 1);
	return NULL;
}

UI_MENU_CALLBACK(type_command_callback)
{
	const char *ostr;
	resources_get_string((char *)param, &ostr);

	if (activated) {
		char *str=lib_stralloc(ostr);
		for (char* p = str; (p = strchr(p, '\\')) != NULL; *p = '\r');
		kb_feed(str);
		lib_free(str);
	}
	return ostr;
}

UI_MENU_DEFINE_STRING(Command01)
UI_MENU_DEFINE_STRING(Command02)
UI_MENU_DEFINE_STRING(Command03)
UI_MENU_DEFINE_STRING(Command04)
UI_MENU_DEFINE_STRING(Command05)
UI_MENU_DEFINE_STRING(Command06)
UI_MENU_DEFINE_STRING(Command07)
UI_MENU_DEFINE_STRING(Command08)
UI_MENU_DEFINE_STRING(Command09)
UI_MENU_DEFINE_STRING(Command10)
UI_MENU_DEFINE_STRING(Command11)
UI_MENU_DEFINE_STRING(Command12)
UI_MENU_DEFINE_STRING(Command13)
UI_MENU_DEFINE_STRING(Command14)
UI_MENU_DEFINE_STRING(Command15)
UI_MENU_DEFINE_STRING(Command16)
UI_MENU_DEFINE_STRING(Command17)
UI_MENU_DEFINE_STRING(Command18)
UI_MENU_DEFINE_STRING(Command19)
UI_MENU_DEFINE_STRING(Command20)

const ui_menu_entry_t edit_commands_menu[] = {
    { "Edit 01", MENU_ENTRY_RESOURCE_STRING, string_Command01_callback, (ui_callback_data_t)"Command 01 (\\=RETURN)" },
    { "Edit 02", MENU_ENTRY_RESOURCE_STRING, string_Command02_callback, (ui_callback_data_t)"Command 02 (\\=RETURN)" },
    { "Edit 03", MENU_ENTRY_RESOURCE_STRING, string_Command03_callback, (ui_callback_data_t)"Command 03 (\\=RETURN)" },
    { "Edit 04", MENU_ENTRY_RESOURCE_STRING, string_Command04_callback, (ui_callback_data_t)"Command 04 (\\=RETURN)" },
    { "Edit 05", MENU_ENTRY_RESOURCE_STRING, string_Command05_callback, (ui_callback_data_t)"Command 05 (\\=RETURN)" },
    { "Edit 06", MENU_ENTRY_RESOURCE_STRING, string_Command06_callback, (ui_callback_data_t)"Command 06 (\\=RETURN)" },
    { "Edit 07", MENU_ENTRY_RESOURCE_STRING, string_Command07_callback, (ui_callback_data_t)"Command 07 (\\=RETURN)" },
    { "Edit 08", MENU_ENTRY_RESOURCE_STRING, string_Command08_callback, (ui_callback_data_t)"Command 08 (\\=RETURN)" },
    { "Edit 09", MENU_ENTRY_RESOURCE_STRING, string_Command09_callback, (ui_callback_data_t)"Command 09 (\\=RETURN)" },
    { "Edit 10", MENU_ENTRY_RESOURCE_STRING, string_Command10_callback, (ui_callback_data_t)"Command 10 (\\=RETURN)" },
    { "Edit 11", MENU_ENTRY_RESOURCE_STRING, string_Command11_callback, (ui_callback_data_t)"Command 11 (\\=RETURN)" },
    { "Edit 12", MENU_ENTRY_RESOURCE_STRING, string_Command12_callback, (ui_callback_data_t)"Command 12 (\\=RETURN)" },
    { "Edit 13", MENU_ENTRY_RESOURCE_STRING, string_Command13_callback, (ui_callback_data_t)"Command 13 (\\=RETURN)" },
    { "Edit 14", MENU_ENTRY_RESOURCE_STRING, string_Command14_callback, (ui_callback_data_t)"Command 14 (\\=RETURN)" },
    { "Edit 15", MENU_ENTRY_RESOURCE_STRING, string_Command15_callback, (ui_callback_data_t)"Command 15 (\\=RETURN)" },
    { "Edit 16", MENU_ENTRY_RESOURCE_STRING, string_Command16_callback, (ui_callback_data_t)"Command 16 (\\=RETURN)" },
    { "Edit 17", MENU_ENTRY_RESOURCE_STRING, string_Command17_callback, (ui_callback_data_t)"Command 17 (\\=RETURN)" },
    { "Edit 18", MENU_ENTRY_RESOURCE_STRING, string_Command18_callback, (ui_callback_data_t)"Command 18 (\\=RETURN)" },
    { "Edit 19", MENU_ENTRY_RESOURCE_STRING, string_Command19_callback, (ui_callback_data_t)"Command 19 (\\=RETURN)" },
    { "Edit 20", MENU_ENTRY_RESOURCE_STRING, string_Command20_callback, (ui_callback_data_t)"Command 20 (\\=RETURN)" },
    SDL_MENU_LIST_END
};	

const ui_menu_entry_t type_commands_menu[] = {
    { "Type 01", MENU_ENTRY_OTHER, type_command_callback, (ui_callback_data_t)"Command01" },
    { "Type 02", MENU_ENTRY_OTHER, type_command_callback, (ui_callback_data_t)"Command02" },
    { "Type 03", MENU_ENTRY_OTHER, type_command_callback, (ui_callback_data_t)"Command03" },
    { "Type 04", MENU_ENTRY_OTHER, type_command_callback, (ui_callback_data_t)"Command04" },
    { "Type 05", MENU_ENTRY_OTHER, type_command_callback, (ui_callback_data_t)"Command05" },
    { "Type 06", MENU_ENTRY_OTHER, type_command_callback, (ui_callback_data_t)"Command06" },
    { "Type 07", MENU_ENTRY_OTHER, type_command_callback, (ui_callback_data_t)"Command07" },
    { "Type 08", MENU_ENTRY_OTHER, type_command_callback, (ui_callback_data_t)"Command08" },
    { "Type 09", MENU_ENTRY_OTHER, type_command_callback, (ui_callback_data_t)"Command09" },
    { "Type 10", MENU_ENTRY_OTHER, type_command_callback, (ui_callback_data_t)"Command10" },
    { "Type 11", MENU_ENTRY_OTHER, type_command_callback, (ui_callback_data_t)"Command11" },
    { "Type 12", MENU_ENTRY_OTHER, type_command_callback, (ui_callback_data_t)"Command12" },
    { "Type 13", MENU_ENTRY_OTHER, type_command_callback, (ui_callback_data_t)"Command13" },
    { "Type 14", MENU_ENTRY_OTHER, type_command_callback, (ui_callback_data_t)"Command14" },
    { "Type 15", MENU_ENTRY_OTHER, type_command_callback, (ui_callback_data_t)"Command15" },
    { "Type 16", MENU_ENTRY_OTHER, type_command_callback, (ui_callback_data_t)"Command16" },
    { "Type 17", MENU_ENTRY_OTHER, type_command_callback, (ui_callback_data_t)"Command17" },
    { "Type 18", MENU_ENTRY_OTHER, type_command_callback, (ui_callback_data_t)"Command18" },
    { "Type 19", MENU_ENTRY_OTHER, type_command_callback, (ui_callback_data_t)"Command19" },
    { "Type 20", MENU_ENTRY_OTHER, type_command_callback, (ui_callback_data_t)"Command20" },
    SDL_MENU_LIST_END
};	

UI_MENU_DEFINE_TOGGLE(MenuButtonOn)

UI_MENU_DEFINE_RADIO(JAMAction)

const ui_menu_entry_t jam_menu[] = {
    { "Ask", MENU_ENTRY_RESOURCE_RADIO, radio_JAMAction_callback, (ui_callback_data_t)MACHINE_JAM_ACTION_DIALOG },
    { "Continue", MENU_ENTRY_RESOURCE_RADIO, radio_JAMAction_callback, (ui_callback_data_t)MACHINE_JAM_ACTION_CONTINUE },
//    { "Start monitor", MENU_ENTRY_RESOURCE_RADIO, radio_JAMAction_callback, (ui_callback_data_t)MACHINE_JAM_ACTION_MONITOR },
    { "Reset", MENU_ENTRY_RESOURCE_RADIO, radio_JAMAction_callback, (ui_callback_data_t)MACHINE_JAM_ACTION_RESET },
    { "Hard reset", MENU_ENTRY_RESOURCE_RADIO, radio_JAMAction_callback, (ui_callback_data_t)MACHINE_JAM_ACTION_HARD_RESET },
    { "Quit emulator", MENU_ENTRY_RESOURCE_RADIO, radio_JAMAction_callback, (ui_callback_data_t)MACHINE_JAM_ACTION_QUIT },
    SDL_MENU_LIST_END
};

const ui_menu_entry_t keys_commands_menu[] = {
    SDL_MENU_ITEM_TITLE("Key mappings"),
    { "Add key mapping (key -> key)",
		MENU_ENTRY_OTHER,
		add_keymapping_callback,
		(ui_callback_data_t)(SDL_POLL_KEYBOARD | SDL_POLL_MODIFIER) },
    { "Add key mapping (key -> MouseB LEFT)",
		MENU_ENTRY_OTHER,
		add_keymousemapping_callback,
		(ui_callback_data_t)SDL_BUTTON_LEFT },
    { "Add key mapping (key -> MouseB RIGHT)",
		MENU_ENTRY_OTHER,
		add_keymousemapping_callback,
		(ui_callback_data_t)SDL_BUTTON_RIGHT },
    { "Delete key mapping",
		MENU_ENTRY_OTHER,
		del_keymapping_callback,
		NULL},
    { "List key mappings",
		MENU_ENTRY_OTHER,
		list_keymappings_callback,
		NULL },
	SDL_MENU_ITEM_SEPARATOR,
    SDL_MENU_ITEM_TITLE("Hotkeys"),
    { "Unmap hotkey",
		MENU_ENTRY_OTHER,
		unmap_hotkey_callback,
		NULL },
    { "List hotkeys",
		MENU_ENTRY_OTHER,
		list_hotkeys_callback,
		NULL },
	SDL_MENU_ITEM_SEPARATOR,
    SDL_MENU_ITEM_TITLE("Commands"),
    { "Edit Commands",
      MENU_ENTRY_SUBMENU,
      submenu_callback,
      (ui_callback_data_t)edit_commands_menu },
    { "Type Commands",
      MENU_ENTRY_SUBMENU,
      submenu_callback,
      (ui_callback_data_t)type_commands_menu },
	SDL_MENU_LIST_END
};

const ui_menu_entry_t misc_menu[] = {
    { "Autostart file in image",
      MENU_ENTRY_DIALOG,
      autostart_file_callback,
      NULL },
	SDL_MENU_ITEM_SEPARATOR,
    { "Action on CPU JAM",
      MENU_ENTRY_SUBMENU,
      submenu_callback,
      (ui_callback_data_t)jam_menu },
	SDL_MENU_ITEM_SEPARATOR,
    SDL_MENU_ITEM_TITLE("3DS specific"),
    { "Power off bottom screen backlight",
		MENU_ENTRY_OTHER_TOGGLE,
		toggle_bottomoff_callback,
		NULL },
	{ "Hide keyboard",
		MENU_ENTRY_OTHER_TOGGLE,
		toggle_hidekeyboard_callback,
		NULL},
	{ "Edit bottom screen button positions",
		MENU_ENTRY_OTHER_TOGGLE,
		toggle_editmode_callback,
		NULL},
    { "Show menu button on bottom screen",
      MENU_ENTRY_RESOURCE_TOGGLE,
      toggle_MenuButtonOn_callback,
      NULL },
	SDL_MENU_LIST_END
};
 