/*
 * async_http.c - Functions specific to async file retrieval
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

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <3ds.h>
#include "async_http.h"
#include "http.h"
#include "lib.h"
#include "log.h"
#include "util.h"
#include "vice3ds.h"

#define MAX_WORKERS 10
#define STACKSIZE (4 * 1024)
#define QUEUESIZE 10

typedef struct {
	char *url;
	char *fname;
	void (*callback)(char *url, char *fname, int result, void* param);
	void *param;
	int result;
} async_http_request;

// static stuff
static volatile int isinit=0;
static int num_workers=0;
static Thread workerHandle[MAX_WORKERS] = {NULL};
static Handle workerRequest;
static tsq_object async_queue;
static tsh_object async_hash;

// async http functions
static Result async_check_cancel()
{
	return isinit?0:-1;
}

static Result async_progress(u64 total, u64 curr)
{
	return 0;
}

static void async_http_worker(void *arg) {
	async_http_request *req;
	int r=0;
	char *partname;
	while(isinit) {
		svcWaitSynchronization(workerRequest, U64_MAX);
		if (!isinit) break;
		if ((req=tsq_get(&async_queue))!=NULL) {
			// got a request, work on it
			partname = util_concat(req->fname, ".part", NULL);
			if ((r = http_download_file(req->url, partname, async_check_cancel, async_progress)) == 0 &&
				access(partname,W_OK) == 0)
			{
				rename(partname,req->fname);
			} else {
				unlink(partname);
			}
			free(partname);
			req->callback(req->url, req->fname, r, req->param);
			// remove file from get-queue
			tsh_put(&async_hash, req->url, NULL);
			// free the request
			free(req->url);
			free(req->fname);
			free(req);
		}
	}
}

void async_http_shutdown()
{
	s32 x;
	if (!isinit) return;
	isinit=0;
	svcReleaseSemaphore(&x, workerRequest, num_workers);
	for (int i=0; i<num_workers; i++) {
		threadJoin(workerHandle[i], U64_MAX);
		workerHandle[i]=NULL;
	}
	num_workers=0;
	svcCloseHandle(workerRequest);
	tsq_free(&async_queue);
	tsh_free(&async_hash);
}

void async_http_init(int numw)
{
	if (isinit) async_http_shutdown();
	num_workers = numw < MAX_WORKERS ? numw : MAX_WORKERS;
	isinit=1;

	// init and spawn my worker threads
	svcCreateSemaphore(&workerRequest, 0, QUEUESIZE-1);
	for (int i=0; i<num_workers; i++)
		workerHandle[i] = threadCreate(async_http_worker, 0, STACKSIZE, 0x2F, -1, true);
	atexit(async_http_shutdown);

	tsq_init(&async_queue, QUEUESIZE);
	tsh_init(&async_hash, (MAX_WORKERS+QUEUESIZE)*2, NULL);
}

int async_http_get(char *url, char *fname, void (*callback)(char *url, char *fname, int result,  void *param), void *param)
{
//log_citra("%s: enter - %s",__func__,url);
	s32 x;
	if (!isinit) return -1;

	// check if we are already downloading this file
	if (tsh_get(&async_hash, url)) return 0;
	if (tsh_put(&async_hash, url, (void*)1) == -1) return -1;

	async_http_request *gf = malloc(sizeof(async_http_request));
	gf->url=lib_stralloc(url);
	gf->fname=lib_stralloc(fname);
	gf->callback=callback;
	gf->param=param;
	gf=tsq_put(&async_queue, gf);
	if (gf) { // we overwrote an existing queue entry, so clean up the old one
		tsh_put(&async_hash, gf->url, NULL);
		free(gf->fname);
		free(gf->url);
		free(gf);
	}
	svcReleaseSemaphore(&x, workerRequest, 1);
	return 0;
}
