/*
 * http.c - HTTP retrieval functions
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

#include <3ds.h>
#include <malloc.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include "archdep_cp.h"
#include "http.h"
#include "log.h"
#include "uimenu.h"

static u32 *SOC_buffer = NULL;
char http_errbuf[HTTP_ERRBUFSIZE];
http_info http_last_req_info = {0};

struct MemoryStruct {
  char *memory;
  size_t size;
};

int downloadProgress(void *clientp,   curl_off_t dltotal,   curl_off_t dlnow,   curl_off_t ultotal,   curl_off_t ulnow)
{
//log_citra("enter %s",__func__);
	return sdl_ui_update_progress_bar(dltotal, dlnow);
}

static size_t WriteMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
	size_t realsize = size * nmemb;
	struct MemoryStruct *mem = (struct MemoryStruct *)userp;

	char *ptr = realloc(mem->memory, mem->size + realsize + 1);
	if(ptr == NULL) {
		/* out of memory! */
		snprintf(http_errbuf,HTTP_ERRBUFSIZE,"not enough memory (realloc returned NULL)");
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

int downloadFile(char *url,
	void *arg,
	int (*progress_callback)(void*, curl_off_t, curl_off_t, curl_off_t, curl_off_t),
	http_dlmode mode)
{
	static int isInit=0;
	*http_errbuf=0;
	if (!isInit) {
		int ret;
		SOC_buffer = (u32*)memalign(SOC_ALIGN, SOC_BUFFERSIZE);
		if(SOC_buffer == NULL) {
			snprintf(http_errbuf,HTTP_ERRBUFSIZE,"memalign failed to allocate");
			return -1;
		}
		if ((ret = socInit(SOC_buffer, SOC_BUFFERSIZE)) != 0) {
			free(SOC_buffer);
			SOC_buffer=NULL;
			snprintf(http_errbuf,HTTP_ERRBUFSIZE,"socInit failed 0x%08X", (unsigned int)ret);
			return -1;
		}
		atexit(socShutdown);
		isInit=1;
	}

	// get my file with curl
	CURL *curl;
	CURLcode res;
	struct MemoryStruct chunk = {0};
	FILE *f=NULL;
	char **mem=NULL;
	char *path=NULL;
	downloadFile_callback *cb;
	struct curl_slist *list = NULL;

	curl = curl_easy_init();
	if(!curl) {
		snprintf(http_errbuf,HTTP_ERRBUFSIZE,"curl_easy_init failed");
		return -1;
	}

	log_message(LOG_DEFAULT,"downloading file %s",url);

	switch (mode) {
		case MODE_FILE:
			path = (char*)arg;
			mkpath(path, 0);
			if (!(f = fopen(path,"w"))) {
				curl_easy_cleanup(curl);
				snprintf(http_errbuf,HTTP_ERRBUFSIZE,"Could not open %s for writing", path);
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
		case MODE_HEAD:
			curl_easy_setopt(curl, CURLOPT_NOBODY, 1L);
			list = curl_slist_append(list, "Cache-Control: no-cache");
			curl_easy_setopt(curl, CURLOPT_HTTPHEADER, list);
			break;
	}

	curl_easy_setopt(curl, CURLOPT_BUFFERSIZE, 128 * 1024);
	curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 10L);
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
	curl_easy_setopt(curl, CURLOPT_FILETIME, 1L);

	curl_easy_setopt(curl, CURLOPT_NOPROGRESS,
		progress_callback != NULL ? 0L : 1L );

	curl_easy_setopt(curl, CURLOPT_PROXY, "");
//log_citra("setting proxy");curl_easy_setopt(curl, CURLOPT_PROXY, "http://127.0.0.1:3128");

	if (progress_callback != NULL)
		curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, progress_callback);

	/* Perform the request, res will get the return code */

	res = curl_easy_perform(curl);
	curl_slist_free_all(list);
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
		default:
			break;
	}

	// set info object
	curl_easy_getinfo(curl, CURLINFO_FILETIME, &(http_last_req_info.mtime));
	curl_easy_cleanup(curl);

	if(res != CURLE_OK) {
		if (res == CURLE_ABORTED_BY_CALLBACK)
			snprintf(http_errbuf,HTTP_ERRBUFSIZE,"Aborted by user");
		else
			snprintf(http_errbuf,HTTP_ERRBUFSIZE,"curl_easy_perform failed: %s", curl_easy_strerror(res));
		if (mode == MODE_MEMORY) free(chunk.memory);
		if (mode == MODE_FILE) unlink(path);
		return res;
	}
	return 0;
}