/*
 * ui.c - Common UI routines.
 *
 * Written by
 *  Hannu Nuotio <hannu.nuotio@tut.fi>
 *  Marco van den Heuvel <blackystardust68@yahoo.com>
 *
 * Based on code by
 *  Andreas Boose <viceteam@t-online.de>
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

/* #define SDL_DEBUG */

#include "vice.h"

#include "vice_sdl.h"
#include <stdio.h>
#include <3ds.h>

#include "autostart.h"
#include "cmdline.h"
#include "archdep.h"
#include "color.h"
#include "fullscreenarch.h"
#include "joy.h"
#include "kbd.h"
#include "lib.h"
#include "lightpen.h"
#include "log.h"
#include "machine.h"
#include "mouse.h"
#include "mousedrv.h"
#include "resources.h"
#include "types.h"
#include "ui.h"
#include "uiapi.h"
#include "uicolor.h"
#include "uifilereq.h"
#include "uimenu.h"
#include "uimsgbox.h"
#include "videoarch.h"
#include "vkbd.h"
#include "vsidui_sdl.h"
#include "vsync.h"
#include "uibottom.h"
#include "uistatusbar.h"

#ifndef SDL_DISABLE
#define SDL_DISABLE SDL_IGNORE
#endif

#ifdef SDL_DEBUG
#define DBG(x)  log_debug x
#else
#define DBG(x)
#endif

#ifdef ANDROID_COMPILE 
extern void keyboard_key_pressed(signed long key);
#endif


static int sdl_ui_ready = 0;
static int save_resources_on_exit;


static void (*psid_init_func)(void) = NULL;
static void (*psid_play_func)(int) = NULL;


/* ----------------------------------------------------------------- */
/* ui.h */

/* Misc. SDL event handling */
void ui_handle_misc_sdl_event(SDL_Event e)
{
    switch (e.type) {
        case SDL_QUIT:
            DBG(("ui_handle_misc_sdl_event: SDL_QUIT"));
			if (save_resources_on_exit) resources_save(NULL);
			archdep_vice_exit(0);
            break;
/*        case SDL_VIDEORESIZE:
            DBG(("ui_handle_misc_sdl_event: SDL_VIDEORESIZE (%d,%d)", (unsigned int)e.resize.w, (unsigned int)e.resize.h));
            sdl_video_resize_event((unsigned int)e.resize.w, (unsigned int)e.resize.h);
            video_canvas_refresh_all(sdl_active_canvas);
            break;*/
        case SDL_ACTIVEEVENT:
            DBG(("ui_handle_misc_sdl_event: SDL_ACTIVEEVENT"));
            if ((e.active.state & SDL_APPACTIVE) && e.active.gain) {
                video_canvas_refresh_all(sdl_active_canvas);
            }
            break;
        case SDL_VIDEOEXPOSE:
            DBG(("ui_handle_misc_sdl_event: SDL_VIDEOEXPOSE"));
            video_canvas_refresh_all(sdl_active_canvas);
            break;
#ifdef SDL_DEBUG
        case SDL_USEREVENT:
            DBG(("ui_handle_misc_sdl_event: SDL_USEREVENT"));
            break;
        case SDL_SYSWMEVENT:
            DBG(("ui_handle_misc_sdl_event: SDL_SYSWMEVENT"));
            break;
#endif
        default:
            DBG(("ui_handle_misc_sdl_event: unhandled"));
            break;
    }
}

/* Main event handler */
ui_menu_action_t ui_dispatch_events(void)
{
    SDL_Event e;
    ui_menu_action_t retval = MENU_ACTION_NONE;

    // check 3d slider and adjust emulation speed if necessary
	static float slider3d = -1.0;
	static int sliderf=-1;
	float f;
	if ((sliderf != slider3d_func && (f=osGet3DSliderState())!=-1) ||
		(sliderf && (f=osGet3DSliderState())!=slider3d)) {
		int w=0,s=100;
		switch (slider3d_func) {
			case 0:	// off
				w=0;
				s=100;
				break;
			case 1: // slowdown
				w=0;
				s=100-(int)(f * 100.0);
				if (f == 1.0) ui_pause_emulation(1);
				if (slider3d == 1.0) ui_pause_emulation(0);
				break;
			default: //speedup
				// 0 = 100, 0.8=400, 0.9=0, 1=warp
				if (f == 1.0) w=1;
				else {
					w=0;
					s= f >= 0.9 ? 0 : 100+(int)(f * 375.0);
				}
		}
		slider3d=f;
		sliderf=slider3d_func;
		resources_set_int("WarpMode", w);
		resources_set_int("Speed", s);
	}
	
	// check event queue
	while (SDL_PollEvent(&e)) {
        switch (e.type) {
            case SDL_KEYDOWN:
                if (e.key.keysym.sym != 0) {
					ui_display_kbd_status(&e);
	                retval = sdlkbd_press(SDL2x_to_SDL1x_Keys(e.key.keysym.sym), e.key.keysym.mod);
//log_3ds("Keydown %d",e.key.keysym.sym);
				}
                break;
            case SDL_KEYUP:
                if (e.key.keysym.sym != 0) {
					ui_display_kbd_status(&e);
					retval = sdlkbd_release(SDL2x_to_SDL1x_Keys(e.key.keysym.sym), e.key.keysym.mod);
//log_3ds("Keyup %d",e.key.keysym.sym);
				}
				break;
#ifdef HAVE_SDL_NUMJOYSTICKS
            case SDL_JOYAXISMOTION:
                retval = sdljoy_axis_event(e.jaxis.which, e.jaxis.axis, e.jaxis.value);
                break;
            case SDL_JOYBUTTONDOWN:
//log_3ds("Joybutton down %d",e.jbutton.button);
                retval = sdljoy_button_event(e.jbutton.which, e.jbutton.button, 1);
                break;
            case SDL_JOYBUTTONUP:
//log_3ds("Joybutton up %d",e.jbutton.button);
                retval = sdljoy_button_event(e.jbutton.which, e.jbutton.button, 0);
                break;
            case SDL_JOYHATMOTION:
                retval = sdljoy_hat_event(e.jhat.which, e.jhat.hat, e.jhat.value);
                break;
#endif
            /*case SDL_MOUSEMOTION:
                sdl_ui_consume_mouse_event(&e);
                mouse_move((int)(e.motion.xrel), (int)(e.motion.yrel));
                break;*/
            case SDL_MOUSEBUTTONDOWN:
            case SDL_MOUSEBUTTONUP:
				retval = sdl_uibottom_mouseevent(&e);
				//mouse_button((int)(e.button.button), (e.button.state == SDL_PRESSED));
                break;
            default:
                /* SDL_EventState(SDL_VIDEORESIZE, SDL_IGNORE); */
                ui_handle_misc_sdl_event(e);
                /* SDL_EventState(SDL_VIDEORESIZE, SDL_ENABLE); */
                break;
        }
        /* When using the menu or vkbd, pass every meaningful event to the caller */
        if (((sdl_menu_state) || (sdl_vkbd_state & SDL_VKBD_ACTIVE)) && (retval != MENU_ACTION_NONE) && (retval != MENU_ACTION_NONE_RELEASE)) {
            break;
        }
    }
    return retval;
}

/* note: we need to be a bit more "radical" about disabling the (mouse) pointer.
 * in practise, we really only need it for the lightpen emulation.
 *
 * TODO: and perhaps in windowed mode enable it when the mouse is moved.
 */

void ui_check_mouse_cursor(void)
{
    if (_mouse_enabled && !lightpen_enabled && !sdl_menu_state) {
        /* mouse grabbed, not in menu. grab input but do not show a pointer */
        SDL_ShowCursor(SDL_DISABLE);
#ifndef USE_SDLUI2
        SDL_WM_GrabInput(SDL_GRAB_ON);
#else
        set_arrow_cursor();
        SDL_SetRelativeMouseMode(SDL_TRUE);
#endif
    } else if (lightpen_enabled && !sdl_menu_state) {
        /* lightpen active, not in menu. show a pointer for the lightpen emulation */
        SDL_ShowCursor(SDL_ENABLE);
#ifndef USE_SDLUI2
        SDL_WM_GrabInput(SDL_GRAB_OFF);
#else
        set_crosshair_cursor();
        SDL_SetRelativeMouseMode(SDL_FALSE);
#endif
    } else {
        if (sdl_active_canvas->fullscreenconfig->enable) {
            /* fullscreen, never show pointer (we really never need it) */
            SDL_ShowCursor(SDL_DISABLE);
#ifndef USE_SDLUI2
            SDL_WM_GrabInput(SDL_GRAB_OFF);
#else
            set_arrow_cursor();
            SDL_SetRelativeMouseMode(SDL_FALSE);
#endif
        } else {
            /* windowed, TODO: disable pointer after time-out */
            SDL_ShowCursor(SDL_DISABLE);
#ifndef USE_SDLUI2
            SDL_WM_GrabInput(SDL_GRAB_OFF);
#else
            set_arrow_cursor();
            SDL_SetRelativeMouseMode(SDL_FALSE);
#endif
        }
    }
}

void ui_message(const char* format, ...)
{
    va_list ap;
    char *tmp;

    va_start(ap, format);
    tmp = lib_mvsprintf(format, ap);
    va_end(ap);

    if (sdl_ui_ready) {
        message_box("VICE MESSAGE", tmp, MESSAGE_OK);
    } else {
        log_debug("%s\n", tmp);
    }
    lib_free(tmp);
}

/* ----------------------------------------------------------------- */
/* uiapi.h */

static int confirm_on_exit;
int sdl_kbd_statusbar;
int sdl_statusbar;
int drive_led;
int drive_led_brightness;

static int set_ui_menukey(int val, void *param)
{
    sdl_ui_menukeys[vice_ptr_to_int(param)] = SDL2x_to_SDL1x_Keys(val);
    return 0;
}

static int set_save_resources_on_exit(int val, void *param)
{
    save_resources_on_exit = val ? 1 : 0;

    return 0;
}

static int set_confirm_on_exit(int val, void *param)
{
    confirm_on_exit = val ? 1 : 0;

    return 0;
}

static int set_sdl_kbd_statusbar(int val, void *param)
{
    sdl_kbd_statusbar = val ? 1 : 0;
	uibottom_must_redraw = 1;
    return 0;
}

static int set_sdl_statusbar(int val, void *param)
{
    sdl_statusbar = val ? 1 : 0;
	uibottom_must_redraw = 1;
    return 0;
}

static int set_drive_led(int val, void *param)
{
    drive_led = val ? 1 : 0;
	ui_update_drive_led();
	return 0;
}

static int set_drive_led_brightness(int val, void *param)
{
    if ((val < 0) || (val > 255)) {
        return -1;
    }
	drive_led_brightness = val;
	ui_update_drive_led();
	return 0;
}

#ifdef ALLOW_NATIVE_MONITOR
int native_monitor;

static int set_native_monitor(int val, void *param)
{
    native_monitor = val;
    return 0;
}
#endif

#ifdef __sortix__
#define DEFAULT_MENU_KEY SDLK_END
#endif

#ifndef DEFAULT_MENU_KEY
# ifdef MACOSX_SUPPORT
#  define DEFAULT_MENU_KEY SDLK_F10
# else
#  define DEFAULT_MENU_KEY SDLK_F12
# endif
#endif

static const resource_int_t resources_int[] = {
    { "MenuKey", DEFAULT_MENU_KEY, RES_EVENT_NO, NULL,
      &sdl_ui_menukeys[0], set_ui_menukey, (void *)MENU_ACTION_NONE },
    { "MenuKeyUp", SDLK_UP, RES_EVENT_NO, NULL,
      &sdl_ui_menukeys[1], set_ui_menukey, (void *)MENU_ACTION_UP },
    { "MenuKeyDown", SDLK_DOWN, RES_EVENT_NO, NULL,
      &sdl_ui_menukeys[2], set_ui_menukey, (void *)MENU_ACTION_DOWN },
    { "MenuKeyLeft", SDLK_LEFT, RES_EVENT_NO, NULL,
      &sdl_ui_menukeys[3], set_ui_menukey, (void *)MENU_ACTION_LEFT },
    { "MenuKeyRight", SDLK_RIGHT, RES_EVENT_NO, NULL,
      &sdl_ui_menukeys[4], set_ui_menukey, (void *)MENU_ACTION_RIGHT },
    { "MenuKeySelect", SDLK_RETURN, RES_EVENT_NO, NULL,
      &sdl_ui_menukeys[5], set_ui_menukey, (void *)MENU_ACTION_SELECT },
    { "MenuKeyCancel", SDLK_BACKSPACE, RES_EVENT_NO, NULL,
      &sdl_ui_menukeys[6], set_ui_menukey, (void *)MENU_ACTION_CANCEL },
    { "MenuKeyExit", SDLK_ESCAPE, RES_EVENT_NO, NULL,
      &sdl_ui_menukeys[7], set_ui_menukey, (void *)MENU_ACTION_EXIT },
    { "MenuKeyMap", SDLK_m, RES_EVENT_NO, NULL,
      &sdl_ui_menukeys[8], set_ui_menukey, (void *)MENU_ACTION_MAP },
    { "MenuKeyPageUp", SDLK_PAGEUP, RES_EVENT_NO, NULL,
      &sdl_ui_menukeys[9], set_ui_menukey, (void *)MENU_ACTION_PAGEUP },
    { "MenuKeyPageDown", SDLK_PAGEDOWN, RES_EVENT_NO, NULL,
      &sdl_ui_menukeys[10], set_ui_menukey, (void *)MENU_ACTION_PAGEDOWN },
    { "MenuKeyHome", SDLK_HOME, RES_EVENT_NO, NULL,
      &sdl_ui_menukeys[11], set_ui_menukey, (void *)MENU_ACTION_HOME },
    { "MenuKeyEnd", SDLK_END, RES_EVENT_NO, NULL,
      &sdl_ui_menukeys[12], set_ui_menukey, (void *)MENU_ACTION_END },
    { "SaveResourcesOnExit", 0, RES_EVENT_NO, NULL,
      &save_resources_on_exit, set_save_resources_on_exit, NULL },
    { "ConfirmOnExit", 0, RES_EVENT_NO, NULL,
      &confirm_on_exit, set_confirm_on_exit, NULL },
    { "SDLKbdStatusbar", 0, RES_EVENT_NO, NULL,
      &sdl_kbd_statusbar, set_sdl_kbd_statusbar, NULL },
    { "SDLStatusbar", 0, RES_EVENT_NO, NULL,
      &sdl_statusbar, set_sdl_statusbar, NULL },
    { "DriveLED", 0, RES_EVENT_NO, NULL,
      &drive_led, set_drive_led, NULL },
    { "DriveLEDBrightness", 40, RES_EVENT_NO, (resource_value_t)40,
      &drive_led_brightness, set_drive_led_brightness, NULL },
#ifdef ALLOW_NATIVE_MONITOR
    { "NativeMonitor", 0, RES_EVENT_NO, NULL,
      &native_monitor, set_native_monitor, NULL },
#endif
    RESOURCE_INT_LIST_END
};

void ui_sdl_quit(void)
{
    if (confirm_on_exit) {
        if (message_box("VICE QUESTION", "Do you really want to exit?", MESSAGE_YESNO) != 0) {
            return;
        }
    }

    if (save_resources_on_exit) {
        if (resources_save(NULL) < 0) {
            ui_error("Cannot save current settings.");
        }
    }
    archdep_vice_exit(0);
}

/* Initialization  */
int ui_resources_init(void)
{
    DBG(("%s", __func__));
    if (resources_register_int(resources_int) < 0) {
        return -1;
    }
    return 0;
}

void ui_resources_shutdown(void)
{
    DBG(("%s", __func__));
    joy_arch_resources_shutdown();
    sdlkbd_resources_shutdown();
}

static const cmdline_option_t cmdline_options[] =
{
    { "-menukey", SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
      NULL, NULL, "MenuKey", NULL,
      "<key>", "Keycode of the menu activate key" },
    { "-menukeyup", SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
      NULL, NULL, "MenuKeyUp", NULL,
      "<key>", "Keycode of the menu up key" },
    { "-menukeydown", SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
      NULL, NULL, "MenuKeyDown", NULL,
      "<key>", "Keycode of the menu down key" },
    { "-menukeyleft", SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
      NULL, NULL, "MenuKeyLeft", NULL,
      "<key>", "Keycode of the menu left key" },
    { "-menukeyright", SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
      NULL, NULL, "MenuKeyRight", NULL,
      "<key>", "Keycode of the menu right key" },
    { "-menukeypageup", SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
      NULL, NULL, "MenuKeyPageUp", NULL,
      "<key>", "Keycode of the menu page up key" },
    { "-menukeypagedown", SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
      NULL, NULL, "MenuKeyPageDown", NULL,
      "<key>", "Keycode of the menu page down key" },
    { "-menukeyhome", SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
      NULL, NULL, "MenuKeyHome", NULL,
      "<key>", "Keycode of the menu home key" },
    { "-menukeyend", SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
      NULL, NULL, "MenuKeyEnd", NULL,
      "<key>", "Keycode of the menu end key" },
    { "-menukeyselect", SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
      NULL, NULL, "MenuKeySelect", NULL,
      "<key>", "Keycode of the menu select key" },
    { "-menukeycancel", SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
      NULL, NULL, "MenuKeyCancel", NULL,
      "<key>", "Keycode of the menu cancel key" },
    { "-menukeyexit", SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
      NULL, NULL, "MenuKeyExit", NULL,
      "<key>", "Keycode of the menu exit key" },
    { "-menukeymap", SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
      NULL, NULL, "MenuKeyMap", NULL,
      "<key>", "Keycode of the menu map key" },
    { "-saveres", SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, "SaveResourcesOnExit", (resource_value_t)1,
      NULL, "Enable saving of the resources on exit" },
    { "+saveres", SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, "SaveResourcesOnExit", (resource_value_t)0,
      NULL, "Disable saving of the resources on exit" },
    { "-confirmonexit", SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, "ConfirmOnExit", (resource_value_t)1,
      NULL, "Enable confirm on exit" },
    { "+confirmonexit", SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, "ConfirmOnExit", (resource_value_t)0,
      NULL, "Disable confirm on exit" },
#ifdef ALLOW_NATIVE_MONITOR
    { "-nativemonitor", SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, "NativeMonitor", (resource_value_t)1,
      NULL, "Enable native monitor" },
    { "+nativemonitor", SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, "NativeMonitor", (resource_value_t)0,
      NULL, "Disable native monitor" },
#endif
    CMDLINE_LIST_END
};

int ui_cmdline_options_init(void)
{
    DBG(("%s", __func__));

    return cmdline_register_options(cmdline_options);
}

int ui_init(int *argc, char **argv)
{
    DBG(("%s", __func__));
    return 0;
}

int ui_init_finish(void)
{
    DBG(("%s", __func__));
    return 0;
}

int ui_init_finalize(void)
{
    DBG(("%s", __func__));

    if (!console_mode) {
        sdl_ui_init_finalize();
#ifndef USE_SDLUI2
        SDL_WM_SetCaption(sdl_active_canvas->viewport->title, "VICE");
#endif
        sdl_ui_ready = 1;
    }
    return 0;
}

void ui_shutdown(void)
{
    DBG(("%s", __func__));
#ifdef USE_SDLUI2
    if (arrow_cursor) {
        SDL_FreeCursor(arrow_cursor);
        arrow_cursor = NULL;
    }
    if (crosshair_cursor) {
        SDL_FreeCursor(crosshair_cursor);
        crosshair_cursor = NULL;
    }
#endif
    sdl_ui_file_selection_dialog_shutdown();
}

/* Print an error message.  */
void ui_error(const char *format, ...)
{
    va_list ap;
    char *tmp;

    va_start(ap, format);
    tmp = lib_mvsprintf(format, ap);
    va_end(ap);

    if (sdl_ui_ready) {
        message_box("VICE ERROR", tmp, MESSAGE_OK);
    } else {
        log_debug("%s\n", tmp);
    }
    lib_free(tmp);
}

/* Let the user browse for a filename; display format as a titel */
char* ui_get_file(const char *format, ...)
{
    return NULL;
}

/* Drive related UI.  */
int ui_extend_image_dialog(void)
{
    if (message_box("VICE QUESTION", "Do you want to extend the disk image?", MESSAGE_YESNO) == 0) {
        return 1;
    }
    return 0;
}

/* Show a CPU JAM dialog.  */
ui_jam_action_t ui_jam_dialog(const char *format, ...)
{
    int retval;

    retval = message_box("VICE CPU JAM", "a CPU JAM has occured, choose the action to take", MESSAGE_CPUJAM);
    if (retval == 0) {
        return UI_JAM_HARD_RESET;
    }
    if (retval == 1) {
        return UI_JAM_MONITOR;
    }
    return UI_JAM_NONE;
}

/* Update all menu entries.  */
void ui_update_menus(void)
{
}

/* ----------------------------------------------------------------- */
/* uicolor.h */

int uicolor_alloc_color(unsigned int red, unsigned int green, unsigned int blue, unsigned long *color_pixel, uint8_t *pixel_return)
{
    DBG(("%s", __func__));
    return 0;
}

void uicolor_free_color(unsigned int red, unsigned int green, unsigned int blue, unsigned long color_pixel)
{
    DBG(("%s", __func__));
}

void uicolor_convert_color_table(unsigned int colnr, uint8_t *data, long color_pixel, void *c)
{
    DBG(("%s", __func__));
}

int uicolor_set_palette(struct video_canvas_s *c, const struct palette_s *palette)
{
    DBG(("%s", __func__));
    return 0;
}

/* ---------------------------------------------------------------------*/
/* vsidui_sdl.h */

/* These items are here so they can be used in generic UI
   code without vsidui.c being linked into all emulators. */
int sdl_vsid_state = 0;

static ui_draw_func_t vsid_draw_func = NULL;

void sdl_vsid_activate(void)
{
    sdl_vsid_state = SDL_VSID_ACTIVE | SDL_VSID_REPAINT;
}

void sdl_vsid_close(void)
{
    sdl_vsid_state = 0;
}

void sdl_vsid_draw_init(ui_draw_func_t func)
{
    vsid_draw_func = func;
}

void sdl_vsid_draw(void)
{
    if (vsid_draw_func) {
        vsid_draw_func();
    }
}


/*
 * Work around linking differences between VSID and other emu's
 */

/** \brief  Set PSID init function pointer
 *
 * \param[in]   func    function pointer
 */
void sdl_vsid_set_init_func(void (*func)(void))
{
    psid_init_func = func;
}


/** \brief  Set PSID play function pointer
 *
 * \param[in]   func    function pointer
 */
void sdl_vsid_set_play_func(void (*func)(int))
{
    psid_play_func = func;
}
