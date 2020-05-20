/*
 * update.c - Update support
 *
 * Written by
 *  Sebastian Weber <me@badda.de>
 *
 * This file is part of VICE3DS
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

#define UPDATE_INFO_URL "https://api.github.com/repos/badda71/vice3ds/releases/latest"
//#define UPDATE_INFO_URL "http://badda.de/vice3ds/latest"

#include <3ds.h>
#include <malloc.h>
#include <unistd.h>
#include "archdep_join_paths.h"
#include "archdep_xdg.h"
#include "archdep_cp.h"
#include "lib.h"
#include "archdep_program_path.h"
#include "ui.h"
#include "uimenu.h"
#include "menu_common.h"
#include "uimsgbox.h"
#include "3ds_cia.h"
#include "update.h"
#include "http.h"

#define ERRBUFSIZE 256

static int update_progress_bar(int total, int cur) {
	sdl_ui_update_progress_bar((u64)total, (u64)cur);
	return ((int)sdl_ui_check_cancel());
}

static char errbuf[ERRBUFSIZE];

// *******************************************************
// exposed functions
// *******************************************************

UI_MENU_CALLBACK(update_callback)
{
    if (!activated) return NULL;
	bool ishomebrew=0;
	if (!checkWifi()) {
		ui_error("WiFi not enabled");
		return NULL;
	}
	
	// check if we need an update
	char update_info[16 * 1024] = {0};
	u32 dSize = sizeof(update_info);
	char *p;
	sdl_ui_init_progress_bar("Getting Update Information");
	if (http_download_buffer(UPDATE_INFO_URL, &dSize, update_info, sizeof(update_info))) {
		ui_error("Could not get update info: %s",http_errbuf);
		return NULL;
	}

	p = strstr(update_info, "tag_name");
	if (!p) {
		ui_error("Could not get latest version info from github api");
		return NULL;
	}
	p+=11;
	char *q=strchr(p,'"'); *q=0;
	if (strcasecmp(p, VERSION3DS)==0) {
		ui_message("No update available (latest version = current version = %s)", VERSION3DS);
		return NULL;
	}
	
	// ask user if we should update
	snprintf(errbuf,ERRBUFSIZE,
		"Installed version: %-9s"
		"Latest version:    %-9s"
		"                            "
		">> Install update? <<",VERSION3DS,p);
	if (message_box("Update available", errbuf, MESSAGE_YESNO) != 0 ) {
		return NULL;
	}
	
	*q='"';
	// get update URL from github
	ishomebrew = envIsHomebrew();
//	ishomebrew = false;
	
	char *ext = ishomebrew? "3dsx" : "cia";
	char *update_url=update_info;
	while ((update_url=strstr(update_url, "browser_download_url"))!=NULL) {
		update_url+=23;
		char *q=strchr(update_url,'"'); *q=0;
		if (strcasecmp(q-strlen(ext),ext) == 0) break;
		*q='"';
	}
	if (!update_url) {
		ui_error("Could not get download url from github api");
		return NULL;
	}

	// download update
	char *update_fname;
	
	if (ishomebrew) {
		update_fname = lib_stralloc(archdep_program_path());
	} else {
		update_fname = archdep_join_paths(archdep_xdg_data_home(),strrchr(update_url,'/'),NULL);
	}
	mkpath(update_fname, 0);

	sdl_ui_init_progress_bar("Downloading Update");
	if (http_download_file(update_url, update_fname, sdl_ui_check_cancel, sdl_ui_update_progress_bar)) {
		ui_error("Could not download update: %s",http_errbuf);
		free(update_fname);
		return NULL;
	}
//log_citra("downloaded update to %s",update_fname);

	// install update if necessary
	if (ishomebrew) {
		ui_message("Update downloaded, shutting down Vice3DS.");
		SDL_PushEvent( &(SDL_Event){ .type = SDL_QUIT });
	} else {
		sdl_ui_init_progress_bar("Installing Update");
		CIA_SetErrorBuffer(errbuf);
		if (CIA_InstallTitle(update_fname, update_progress_bar) != 0) {
			unlink(update_fname);
			free(update_fname);
			ui_error("Could not install update: %s",errbuf);
			return NULL;
		} else {
			unlink(update_fname);
			free(update_fname);
			ui_message("Update downloaded and installed, restarting Vice3DS.");
			if (CIA_LaunchLastTitle() != 0) {
				ui_error("Could not launch updated title, please start manually: %s",errbuf);
				SDL_PushEvent( &(SDL_Event){ .type = SDL_QUIT });
			}
		}
	}
	return sdl_menu_text_exit_ui;
}