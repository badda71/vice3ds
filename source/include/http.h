/*
 * http.h - HTTP retrieval functions
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
 
#include <curl/curl.h>

#define SOC_ALIGN       0x1000
#define SOC_BUFFERSIZE  0x100000
#define HTTP_ERRBUFSIZE 256

typedef enum {
	MODE_FILE,
	MODE_MEMORY,
	MODE_CALLBACK,
	MODE_HEAD
} http_dlmode;

typedef struct {
	size_t (*write_callback)(char *ptr, size_t, size_t, void *userdata);
	void *userdata;
} downloadFile_callback;

typedef struct {
	long mtime;
} http_info;

extern int downloadFile(char *url,
	void *arg,
	int (*progress_callback)(void*, curl_off_t, curl_off_t, curl_off_t, curl_off_t),
	http_dlmode mode);
extern int downloadProgress(void *clientp,   curl_off_t dltotal,   curl_off_t dlnow,   curl_off_t ultotal,   curl_off_t ulnow);

extern http_info http_last_req_info;
extern char http_errbuf[HTTP_ERRBUFSIZE];
