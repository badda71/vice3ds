/*
 * autofire.c - Autofire support
 *
 * Written by
 *  Sebastian Weber <me@badda.de>
 *
 * Based on code by
 *  Bernhard Kuhn <kuhn@eikon.e-technik.tu-muenchen.de>
 *  Ulmer Lionel <ulmer@poly.polytechnique.fr>
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

#include <3ds.h>
#include "vice.h"
#include "vice_sdl.h"
#include "autofire.h"
#include "log.h"
#include "joy.h"
#include "resources.h"
#include "joystick.h"
#include "ui.h"

//The thread that will be used
static SDL_Thread *af_thread = NULL;
static Handle af_start = 0;
static volatile int af_active = 0;
static int af_init = 0;

static int autofire_thread( void *data ) {
	SDL_Event e1[3];
	SDL_Event e2[3];
	int k;

	//While the program is not over
	while( 1 ) {
		svcWaitSynchronization(af_start, U64_MAX);
		svcClearEvent(af_start);
		int s=500/joy_auto_speed;
		// setup my events
		e1[1].type = e1[2].type = SDL_KEYDOWN;
		e2[1].type = e2[2].type = SDL_KEYUP;
		resources_get_int( "KeySet1Fire", &k );
		e1[1].key.keysym.unicode = e1[1].key.keysym.sym =
		e2[1].key.keysym.unicode = e2[1].key.keysym.sym = k & 0xff;
		resources_get_int( "KeySet2Fire", &k );
		e1[2].key.keysym.unicode = e1[2].key.keysym.sym =
		e2[2].key.keysym.unicode = e2[2].key.keysym.sym = k & 0xff;
		
		// post fire events
		while (af_active) {
			k=af_active;
			for (int i=1; i<3; i++)
				if (k & i) 
					SDL_PushEvent(&e1[i]);
			SDL_Delay(s);
			for (int i=1; i<3; i++)
				if (k & i) 
					SDL_PushEvent(&e2[i]);
			SDL_Delay(s);
		}
		svcClearEvent(af_start);
	}
	return 0;
}

static int autofire_init() {
	// start autofire threads
	svcCreateEvent(&af_start,0);
	if ((af_thread = SDL_CreateThread(autofire_thread, NULL)) == NULL ) return -1;
	af_init=1;
	return 0;
}

void autofire_shutdown() {
	if (af_init) {
		if (af_thread) SDL_KillThread (af_thread);
		if(af_start) svcCloseHandle(af_start);
	}
	af_init=0;
}

int start_autofire(int keyset) {
	if (keyset<1 || keyset>2) return -1;
	// init if not alread done
	if (!af_init && autofire_init()!=0) return -1;

	// clear stop semaphore
	af_active |= keyset;
	svcSignalEvent(af_start);
	return 0;
}

void stop_autofire(int keyset) {
	// clear start semaphore
	af_active &= ~keyset;
}
