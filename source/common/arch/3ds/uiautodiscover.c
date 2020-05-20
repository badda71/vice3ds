/*
 * autodiscover.c - Functions specific to autodiscovery of netplay server
 *
 * Written by
 *  Sebastian Weber <me@badda.de>
 *
 * This file is part of vice3DS
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

#include <SDL/SDL.h>
#include <errno.h>

#include "menu_common.h"
#include "network.h"
#include "resources.h"
#include "ui.h"
#include "uiautodiscover.h"
#include "uibottom.h"
#include "uimenu.h"
#include "vice3ds.h"
#include "videoarch.h"

#define AUTODISCOVER_FIRST_Y 4
#define AUTODISCOVER_MAXSERVERS 20
#define AUTODISCOVER_POLL_INTERVAL 1000 // in ms

static menu_draw_t *menu_draw;
static volatile int trap_result;

static void autodiscover_redraw(disc_server s[], char *title, int cur, int total) {
	int i, j, oldbg;
	char buf[64];

	sdl_ui_clear();
    sdl_ui_display_title(title);
	
	snprintf(buf, sizeof(buf), "Finding servers ...          %2d Found", total);
	sdl_ui_print(buf, MENU_FIRST_X, 2);
    for (i = 0; i < total; ++i) {

		snprintf(buf, sizeof(buf), "%s - %s:%d", s[i].name, s[i].addr, s[i].port);
		if (i == cur) oldbg = sdl_ui_set_cursor_colors();
		j = sdl_ui_print(buf, MENU_FIRST_X, i + AUTODISCOVER_FIRST_Y);
		if (i == cur) sdl_ui_print_eol(j, i + AUTODISCOVER_FIRST_Y);
		if (i == cur) sdl_ui_reset_cursor_colors(oldbg);
	}
}

static int autodiscover_start(disc_server *s)
{
	menu_draw = sdl_ui_get_menu_param();
	int servers_found = 0;
	disc_server servers[AUTODISCOVER_MAXSERVERS];
	int active = 1, redraw = 1, cur=-1, action, menu_retval = -1, loopc = 0, i, y;
	char *title="Netplay Autodiscovery";

	// main autodiscovery interface loop
	while (active) {
		if (redraw) {
			autodiscover_redraw(servers, title, cur, servers_found);
			redraw = 0;
	        sdl_ui_refresh();
		}
		// poll the server
		if (servers_found < AUTODISCOVER_MAXSERVERS) {
			int ret = disc_run_client(loopc < 20 ? 1 : 0, &(servers[servers_found]));
			if (ret == 0) {
				// check if we already found this server
				for (i=0; i<servers_found; i++) {
					if (strcmp(servers[i].addr, servers[servers_found].addr) == 0) break;
				}
				if (i == servers_found) {
					if (cur == -1) cur=0;
					servers_found++;
					redraw = 1;
					continue;
				}
			} else if (ret == -2) {
				ui_error(strerror(errno));
				active=0;
			}
		}
		
		// check input
		while ((action = ui_dispatch_events()) != MENU_ACTION_NONE) {
			switch (action) {
			case MENU_ACTION_EXIT:
				menu_retval = -2;
			/* fall through */
			case MENU_ACTION_LEFT:
			case MENU_ACTION_CANCEL:
				active = 0;
				break;
			case MENU_ACTION_UP:
				if (servers_found) {
					cur = (cur - 1 + servers_found) % servers_found;
				}
				redraw = 1;
				break;
			case MENU_ACTION_DOWN:
				if (servers_found) {
					cur = (cur + 1) % servers_found;
				}
				redraw = 1;
				break;
			case MENU_ACTION_SELECT:
				if (servers_found) {
					active = 0;
					menu_retval = cur;
				}
				break;
			case MENU_ACTION_MOUSE:
				y = ((lastevent.button.y*240) / sdl_active_canvas->screen->h) / activefont.h;
				if (y >= AUTODISCOVER_FIRST_Y &&
					y < AUTODISCOVER_FIRST_Y + servers_found) {
					cur = y-AUTODISCOVER_FIRST_Y;
					redraw = 1;
					if (lastevent.type == SDL_MOUSEBUTTONUP) {
						active = 0;
						menu_retval = cur;
					}
				}
				break;
			}
		}
		SDL_Delay(20);
		loopc = (loopc + 20) % AUTODISCOVER_POLL_INTERVAL;
	}
	disc_stop_client();
	if (menu_retval > -1 && menu_retval<servers_found) {
		*s = servers[menu_retval];
	}
	return menu_retval;
}

UI_MENU_CALLBACK(autodiscover_callback)
{
	if (activated) {
		if (!checkWifi()) {
			ui_error("WiFi not enabled");
			return NULL;
		}
		disc_server s={0};
		int ret = autodiscover_start(&s);
		if (ret == 0) {
			resources_set_string("NetworkServerName", s.addr);
			resources_set_int("NetworkServerPort", s.port);
		}
	}
	return NULL;
}
