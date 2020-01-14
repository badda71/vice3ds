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

//#define EMULATION
#define UPDATE_INFO_URL "https://api.github.com/repos/badda71/vice3ds/releases/latest"
//#define UPDATE_INFO_URL "http://badda.de/vice3ds/latest"

#include <3ds.h>
#include <curl/curl.h>
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
#include "archdep_atexit.h"
#include "update.h"

#define SOC_ALIGN       0x1000
#define SOC_BUFFERSIZE  0x100000
#define ERRBUFSIZE 256

static u32 *SOC_buffer = NULL;
static char errbuf[ERRBUFSIZE];
static menu_draw_t *menu_draw;

struct MemoryStruct {
  char *memory;
  size_t size;
};

// *******************************************************
// static functions
// *******************************************************

static int sdl_ui_update_progress_bar(int total, int now)
{
//log_citra("enter %s",__func__);
	static int oldtotal=-1, oldnow=-1;

	// process events - check for menu close or cancel
	int action;
	while ((action=ui_dispatch_events()) != MENU_ACTION_NONE) {
		switch (action) {
			case MENU_ACTION_CANCEL:
			case MENU_ACTION_EXIT:
				return -1;
			default:
				break;
		}
	}

	if (oldtotal == total && oldnow == now) return 0;

	// update progress bar
	char buf[100];
	int xmax = menu_draw->max_text_x;
	int cutoff = total == 0 ? 0 : (now * xmax) / total;
	int percent = total == 0 ? 0 : (now * 100) / total;
	int y=2;

	oldtotal = total;
	oldnow = now;
	snprintf(buf, 100, "Progress: %d / %d (%d%%)", now, total, percent);
	sdl_ui_print(buf, 0, y++);
	++y;

	for (int loop = 0; loop < xmax; loop++) {
		sdl_ui_putchar (
			loop < cutoff ? UIFONT_SLIDERACTIVE_CHAR : UIFONT_SLIDERINACTIVE_CHAR,
			loop,
			y);
	}
	sdl_ui_refresh();
	return 0;
}

static void sdl_ui_init_progress_bar(char *title)
{
//log_citra("enter %s",__func__);

	int i=0;

	sdl_ui_init_draw_params();
    menu_draw = sdl_ui_get_menu_param();
    sdl_ui_clear();

    sdl_ui_reverse_colors();
	i += sdl_ui_print(title, i, 0);
    i += sdl_ui_print_eol(i, 0);
    sdl_ui_reverse_colors();

	sdl_ui_update_progress_bar(0,0);
}

static size_t WriteMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
	size_t realsize = size * nmemb;
	struct MemoryStruct *mem = (struct MemoryStruct *)userp;

	char *ptr = realloc(mem->memory, mem->size + realsize + 1);
	if(ptr == NULL) {
		/* out of memory! */ 
		snprintf(errbuf,ERRBUFSIZE,"not enough memory (realloc returned NULL)");
		return 0;
	}

	mem->memory = ptr;
	memcpy(&(mem->memory[mem->size]), contents, realsize);
	mem->size += realsize;
	mem->memory[mem->size] = 0;

	return realsize;
}

static void socShutdown() {
	socExit();
	if (SOC_buffer) {
		free(SOC_buffer);
		SOC_buffer=NULL;
	}
}

typedef enum {
	MODE_FILE,
	MODE_MEMORY,
	MODE_CALLBACK } dlmode;

static int downloadFile(char *url,
	void *arg,
	int (*progress_callback)(void*, curl_off_t, curl_off_t, curl_off_t, curl_off_t),
	dlmode mode)
{
	static int isInit=0;
	*errbuf=0;
	if (!isInit) {
		int ret;
		SOC_buffer = (u32*)memalign(SOC_ALIGN, SOC_BUFFERSIZE);
		if(SOC_buffer == NULL) {
			snprintf(errbuf,ERRBUFSIZE,"memalign failed to allocate");
			return -1;
		}
		if ((ret = socInit(SOC_buffer, SOC_BUFFERSIZE)) != 0) {
			free(SOC_buffer);
			SOC_buffer=NULL;
			snprintf(errbuf,ERRBUFSIZE,"socInit failed 0x%08X", (unsigned int)ret);
			return -1;
		}
		atexit(socShutdown);
		isInit=1;
	}

	// get my file with curl
	CURL *curl;
	CURLcode res;
	struct MemoryStruct chunk = {0};
	FILE *f;
	char **mem=NULL;
	char *path=NULL;
	downloadFile_callback *cb;

	curl = curl_easy_init();
	if(!curl) {
		snprintf(errbuf,ERRBUFSIZE,"curl_easy_init failed");
		return -1;
	}

	switch (mode) {
		case MODE_FILE:
			path = (char*)arg;
			if (!(f = fopen(path,"w"))) {
				curl_easy_cleanup(curl);
				snprintf(errbuf,ERRBUFSIZE,"Could not open %s for writing", path);	
				return -1;
			};
			curl_easy_setopt(curl, CURLOPT_WRITEDATA, f);
			break;
		case MODE_MEMORY:
			mem = (char **)arg;
			chunk.memory = malloc(1);  /* will be grown as needed by the realloc above */ 
			curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
			curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);
			break;
		case MODE_CALLBACK:
			cb=arg;
			curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, cb->write_callback);
			curl_easy_setopt(curl, CURLOPT_WRITEDATA, cb->userdata);
			break;
	}

	curl_easy_setopt(curl, CURLOPT_BUFFERSIZE, 128 * 1024);
	curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 5L);
	curl_easy_setopt(curl, CURLOPT_URL, url);
	curl_easy_setopt(curl, CURLOPT_ACCEPT_ENCODING, "");
	curl_easy_setopt(curl, CURLOPT_USERAGENT, "libcurl-agent/1.0");
	curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
	curl_easy_setopt(curl, CURLOPT_MAXREDIRS, 50L);
	curl_easy_setopt(curl, CURLOPT_HTTP_VERSION, (long)CURL_HTTP_VERSION_2TLS);
	curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
	curl_easy_setopt(curl, CURLOPT_VERBOSE, 0L);
	curl_easy_setopt(curl, CURLOPT_STDERR, stdout);
	curl_easy_setopt(curl, CURLOPT_FAILONERROR, 1L);

	curl_easy_setopt(curl, CURLOPT_NOPROGRESS,
		progress_callback != NULL ? 0L : 1L );

//log_citra("setting proxy");curl_easy_setopt(curl, CURLOPT_PROXY, "http://127.0.0.1:3128");
	curl_easy_setopt(curl, CURLOPT_PROXY, "");

	if (progress_callback != NULL)
		curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, progress_callback);
	
	/* Perform the request, res will get the return code */ 

	res = curl_easy_perform(curl);
	/* always cleanup */ 
	curl_off_t dlp;
	switch (mode) {
		case MODE_MEMORY:
			*mem=chunk.memory;
			break;
		case MODE_FILE:
			fclose(f);
			// fallthrough
		case MODE_CALLBACK:
			curl_easy_getinfo(curl, CURLINFO_SIZE_DOWNLOAD_T, &dlp);
			chunk.size=dlp;
			break;
	}
	curl_easy_cleanup(curl);

	if(res != CURLE_OK) {
		if (res == CURLE_ABORTED_BY_CALLBACK)
			snprintf(errbuf,ERRBUFSIZE,"Aborted by user");
		else
			snprintf(errbuf,ERRBUFSIZE,"curl_easy_perform failed: %s", curl_easy_strerror(res));
		if (chunk.memory) free(chunk.memory);
		return -1;
	}
	return chunk.size;
}

static int downloadProgress(void *clientp,   curl_off_t dltotal,   curl_off_t dlnow,   curl_off_t ultotal,   curl_off_t ulnow)
{
//log_citra("enter %s",__func__);
	return sdl_ui_update_progress_bar(dltotal, dlnow);
}

// *******************************************************
// exposed functions
// *******************************************************

UI_MENU_CALLBACK(update_callback)
{
    if (!activated) return NULL;
	bool ishomebrew=0;
	u32 wifi_status;
	acInit();
	ACU_GetWifiStatus(&wifi_status);
	acExit();

#ifndef EMULATION
	if (wifi_status == 0) {
		ui_error("WiFi not enabled");
		return NULL;
	}
#endif
	// check if we need an update
	char *update_info, *p;
	sdl_ui_init_progress_bar("Getting Update Information");
	if (downloadFile(UPDATE_INFO_URL, &update_info, downloadProgress, MODE_MEMORY)<1) {
		ui_error("Could not get update info: %s",errbuf);
		return NULL;
	}

	p = strstr(update_info, "tag_name");
	if (!p) {
		ui_error("Could not get latest version info from github api");
		free(update_info);
		return NULL;
	}
	p+=12;
	char *q=strchr(p,'"'); *q=0;
	if (strcasecmp(p, VERSION3DS)==0) {
		ui_message("No update available (latest version = current version = %s)", VERSION3DS);
		free(update_info);
		return NULL;
	}
	
	// ask user if we should update
	snprintf(errbuf,ERRBUFSIZE,
		"Installed version: %-9s"
		"Latest version:    %-9s"
		"                            "
		">> Install update? <<",VERSION3DS,p);
	if (message_box("Update available", errbuf, MESSAGE_YESNO) != 0 ) {
		free(update_info);
		return NULL;
	}
	
	*q='"';
	// get update URL from github
	ishomebrew = envIsHomebrew();
//	ishomebrew = false;
	
	char *ext = ishomebrew? "3dsx" : "cia";
	char *update_url=update_info;
	while ((update_url=strstr(update_url, "browser_download_url"))!=NULL) {
		update_url+=24;
		char *q=strchr(update_url,'"'); *q=0;
		if (strcasecmp(q-strlen(ext),ext) == 0) break;
		*q='"';
	}
	if (!update_url) {
		ui_error("Could not get download url from github api");
		free(update_info);
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
	if (downloadFile(update_url, update_fname, downloadProgress, MODE_FILE) < 1) {
		ui_error("Could not download update: %s",errbuf);
		free(update_info);
		free(update_fname);
		return NULL;
	}
	free(update_info);
//log_citra("downloaded update to %s",update_fname);

	// install update if necessary
	if (ishomebrew) {
		ui_message("Update downloaded, shutting down Vice3DS.");
		SDL_PushEvent( &(SDL_Event){ .type = SDL_QUIT });
	} else {
		sdl_ui_init_progress_bar("Installing Update");
		CIA_SetErrorBuffer(errbuf);
		if (CIA_InstallTitle(update_fname, sdl_ui_update_progress_bar) != 0) {
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