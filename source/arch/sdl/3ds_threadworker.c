/*
 * 3ds_threadworker.c - 3DS small worker thread handling
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
#include <SDL/SDL.h>

#define MAXPENDING 10

static SDL_Thread *worker=NULL;
static SDL_sem *worker_sem=NULL;
static int ((*worker_fn[MAXPENDING])(void *));
static void *worker_data[MAXPENDING];
static int p1=0;
static int p2=0;

static int worker_thread(void *data) {
	while( 1 ) {
		SDL_SemWait(worker_sem);
		(worker_fn[p2])(worker_data[p2]);
		worker_fn[p2]=NULL;
		p2=(p2+1)%MAXPENDING;
	}
	return 0;
}

void worker_init() {
	worker_sem = SDL_CreateSemaphore(0);
	worker = SDL_CreateThread(worker_thread,NULL);
	memset(worker_fn, 0, sizeof(worker_fn));
	memset(worker_data, 0, sizeof(worker_data));
}

int start_worker(int (*fn)(void *), void *data) {
	if (!worker_sem) worker_init();
	if (worker_fn[p1] != NULL) return -1;
	worker_fn[p1]=fn;
	worker_data[p1]=data;
	SDL_SemPost(worker_sem);	
	p1=(p1+1)%MAXPENDING;
	return 0;
}
