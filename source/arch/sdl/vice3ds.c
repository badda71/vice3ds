/*
 * vice3ds.c - Function specific to vice3DS port
 *
 * Written by
 *  Sebastian Weber <me@badda.de>
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
#include <string.h>
#include <SDL/SDL.h>
#include "vice3ds.h"
#include "uibottom.h"
#include "util.h"
#include "kbd.h"
#include "mousedrv.h"

// LED-related vars / functions
static Handle ptmsysmHandle = 0;
static unsigned char ledpattern[100] = {0};

static int LED3DS_Init() {
	if (ptmsysmHandle == 0) {
		srvInit();
		Result res = srvGetServiceHandle(&ptmsysmHandle, "ptm:sysm");
		srvExit();
		if (res < 0) return -1;
	}
	return 0;
}

int LED3DS_On(unsigned char r, unsigned char g, unsigned char b) {
	int i;
	for (i=0;i<4;++i) ledpattern[i]=0;
	for (i=4;i<36;++i) ledpattern[i]=r;
	for (i=36;i<68;++i) ledpattern[i]=g;
	for (i=68;i<100;++i) ledpattern[i]=b;

	if (LED3DS_Init() < 0) return -1;

	u32* ipc = getThreadCommandBuffer();
    ipc[0] = 0x8010640;
    memcpy(&ipc[1], ledpattern, 0x64);
    Result ret = svcSendSyncRequest(ptmsysmHandle);
    if(ret < 0 || ipc[1]<0) return -1;
	return 0;
}

int LED3DS_Off() {
	return LED3DS_On(0,0,0);
}

// threadworker-related vars / functions
#define MAXPENDING 10

static SDL_Thread *worker=NULL;
static SDL_sem *worker_sem=NULL;
static volatile int ((*worker_fn[MAXPENDING])(void *));
static void *worker_data[MAXPENDING];

static int worker_thread(void *data) {
	static int p2=0;
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
	static int p1=0;
	if (!worker_sem) worker_init();
	if (worker_fn[p1] != NULL) return -1;
	worker_fn[p1]=fn;
	worker_data[p1]=data;
	SDL_SemPost(worker_sem);	
	p1=(p1+1)%MAXPENDING;
	return 0;
}

// additional input mapping functions

char *keymap3ds_resource=NULL;
int keymap3ds[256] = {0}; // active(1|0) - type (1|2|4) - (mod|value|0) - (sym|axis|button)

static int load_3ds_mapping(char *s) {
	char *p;
	unsigned long int i;
	memset(keymap3ds,0,sizeof(keymap3ds));
	while((i=strtoul(s,&p,16))!=0) {
		keymap3ds[i>>24] = (i & 0xFFFFFF) | 0x01000000;
		s=p;
	}
	uibottom_must_redraw |= UIB_RECALC_SBUTTONS;

	return 0;
}

static int save_3ds_mapping() {
	int i,c=0;
	if (keymap3ds_resource != NULL) free(keymap3ds_resource);
	for (i=1;i<255;i++)
		if (keymap3ds[i]) ++c;
	keymap3ds_resource=malloc(c*9+1);
	c=0;
	for (i=1;i<255;i++)
		if (keymap3ds[i])
			c+=sprintf(keymap3ds_resource+c,"%02x%06x ",i,keymap3ds[i] & 0xffffff);
	keymap3ds_resource[c-1]=0;
	return 0;
}

int do_3ds_mapping(SDL_Event *e) {
	int i;
	if (e->type != SDL_KEYDOWN && e->type != SDL_KEYUP) return 0; // not the right event type
	if (!((i=keymap3ds[e->key.keysym.sym]) & 0x01000000)) return 0; // not mapped
	switch (i & 0x00FF0000) {
		case 0x00010000:
			e->key.keysym.unicode = e->key.keysym.sym = i & 0x000000FF;
			e->key.keysym.mod = (i & 0x0000FF00) >> 8;
			break;
		case 0x00020000:
			e->jaxis.which = 0;
			e->jaxis.axis = i & 0x000000FF;
			e->jaxis.value = e->type == SDL_KEYUP ? 0:
				((i & 0x0000FF00) == 0x100 ? 32760 : -32760);
			e->type = SDL_JOYAXISMOTION;
			break;
		case 0x00040000:
			e->jbutton.which = 0;
			e->jbutton.button = i & 0x000000FF;
			e->jbutton.state = e->type == SDL_KEYDOWN ? SDL_PRESSED : SDL_RELEASED;
			e->type = e->type == SDL_KEYDOWN ? SDL_JOYBUTTONDOWN : SDL_JOYBUTTONUP;
			break;
		case 0x00080000:
			e->button.which = 1; // mouse 0 is the touchpad, mouse 1 is the mapped mouse keys
			e->button.button = i & 0x000000FF;
			e->button.state = e->type == SDL_KEYDOWN ? SDL_PRESSED : SDL_RELEASED;
			e->type = e->type == SDL_KEYDOWN ? SDL_MOUSEBUTTONDOWN : SDL_MOUSEBUTTONUP;
			e->button.x = mousedrv_get_x();
			e->button.y = mousedrv_get_x();
			break;
	}
	return 1;
}

void set_3ds_mapping(int sym, SDL_Event *e) {
	if (!sym) return;
	keymap3ds[sym]=
		e==NULL ? 0 :
		(0x01000000 |	// active
		(e->type==SDL_JOYBUTTONDOWN ? 0x00040000 : (e->type==SDL_JOYAXISMOTION ? 0x00020000 : (e->type==SDL_KEYDOWN ? 0x00010000 : 0x00080000))) | //type
		(e->type==SDL_KEYDOWN ? ((e->key.keysym.mod & 0xFF) << 8) : (e->type==SDL_JOYAXISMOTION ? (e->jaxis.value <0 ? 0x200 : 0x100 ) : 0)) | // mod
		(e->type==SDL_JOYBUTTONDOWN ? e->jbutton.button : (e->type==SDL_JOYAXISMOTION ? e->jaxis.axis : (e->type==SDL_KEYDOWN ? e->key.keysym.sym : e->button.button)))); //type
	save_3ds_mapping();
}

char buf[20];
char *get_3ds_mapping_name(int i) {
	int a,k,v;
	
	if (!keymap3ds[i]) return NULL;
	k=keymap3ds[i];
	switch(k & 0x00FF0000) {
	case 0x00010000:	// key
		snprintf(buf,20,"Key %s",get_3ds_keyname(k & 0xFF));
		break;
	case 0x00020000:	// joy axis
		a = k & 0xFF;
		v = k>>8 &0xFF;
		snprintf(buf,20,"Joy %s",
			a == 1 && v==2 ? "UP":(
			a == 1 && v==1 ? "DOWN":(
			a == 0 && v==2 ? "LEFT":
			"RIGHT")));
		break;
	case 0x00040000:	// joy button
		snprintf(buf,20,"Joy FIRE%d",k & 0xFF);
		break;
	case 0x00080000:	// joy button
		snprintf(buf,20,"Mousebutton %s", (k & 0xFF) == SDL_BUTTON_LEFT ?"LEFT":"RIGHT");
		break;
	default:
		return NULL;
	}
	return buf;
}

int keymap3ds_resource_set(const char *val, void *param)
{
	if (util_string_set(&keymap3ds_resource, val)) {
		return 0;
	}
	return load_3ds_mapping(keymap3ds_resource);
}

void vice3ds_shutdown() {
	free(keymap3ds_resource);
}
