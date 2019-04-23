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

#include "vice.h"
#include "vice_sdl.h"
#include "autofire.h"
#include "log.h"
#include "joy.h"
#include "resources.h"
#include "joystick.h"

//The thread that will be used
SDL_Thread *af_port1_thread = NULL;
SDL_Thread *af_port2_thread = NULL;
int af_key[2] = { 0, 0 };
SDL_sem *af_start[2];
SDL_sem *af_stop[2];
int af_active[2] = {0,0};
int af_init = 0;
int af_button_joy = 0;
int af_button_ks1 = 0;
int af_button_ks2 = 0;

static int autofire_thread( void *data ) {
	int port=*((int*)data);
	SDL_Event e1,e2;
	
	//While the program is not over
	while( 1 ) {
		SDL_SemWait(af_start[port]);
		int s=500/joy_auto_speed;
		int k=af_key[port];
		switch (k >> 8) {
			case 1:
				e1.type = SDL_JOYBUTTONDOWN;
				e2.type = SDL_JOYBUTTONUP;
				e1.jbutton.which = e2.jbutton.which = 0;
				e1.jbutton.button = e2.jbutton.button = k & 0xff;
				break;
			default:
				e1.type = SDL_KEYDOWN;
				e2.type = SDL_KEYUP;
				e1.key.keysym.unicode = e1.key.keysym.sym =
				e2.key.keysym.unicode = e2.key.keysym.sym = k & 0xff;
		}
		while(SDL_SemTryWait(af_stop[port])) {
			SDL_PushEvent(&e1);
			SDL_Delay(s);
			SDL_PushEvent(&e2);
			SDL_Delay(s);
		}
	}
	return 0;
}

static int autofire_init() {
	// check fire buttons for joystick, keyset 1&2
	af_button_joy = sdl_joy_fire;
	resources_get_int( "KeySet1Fire", &af_button_ks1 );
	resources_get_int( "KeySet2Fire", &af_button_ks2 );


	// start autofire threads
	af_start[0] = SDL_CreateSemaphore(0);
	af_start[1] = SDL_CreateSemaphore(0);	
	af_stop[0] = SDL_CreateSemaphore(0);
	af_stop[1] = SDL_CreateSemaphore(0);	
	int i1=0,i2=1;
	if ((af_port1_thread = SDL_CreateThread(autofire_thread, &i1)) == NULL ) return -1;
	if ((af_port2_thread = SDL_CreateThread(autofire_thread, &i2)) == NULL ) return -1;
	af_init=1;
	return 0;
}

void autofire_shutdown() {
	if (af_init) {
		if (af_port1_thread) SDL_KillThread (af_port1_thread);
		if (af_port2_thread) SDL_KillThread (af_port2_thread);
		if(af_start[0]) SDL_DestroySemaphore(af_start[0]);
		if(af_start[1]) SDL_DestroySemaphore(af_start[1]);
		if(af_stop[0]) SDL_DestroySemaphore(af_stop[0]);
		if(af_stop[1]) SDL_DestroySemaphore(af_stop[1]);
	}
	af_init=0;
}

static int af_get_joydev_fire(int port) {
	int dev=0;
	resources_get_int( port==0?"JoyDevice1":"JoyDevice2" ,&dev);

	switch(dev){
		case 2: // Keyset 1
			return af_button_ks1;
		case 3: // Keyset 2
			return af_button_ks2;
		case 4:	// joystick
			return (0x100 | af_button_joy);
	}
	return 0;
}

int start_autofire(int port) {
	if (port<0 || port>1) return -1;
	// init if not alread done
	if (!af_init && autofire_init()!=0) return -1;

	if ((af_key[port]=af_get_joydev_fire(port))==0) return -1;
	SDL_SemPost(af_start[port]);
	return 0;
}

void stop_autofire(int port) {
	SDL_SemPost(af_stop[port]);
}
