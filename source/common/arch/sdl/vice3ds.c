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
#include "lib.h"
#include "resources.h"

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
#ifdef HAVE_SDL_NUMJOYSTICKS
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
#endif
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

void set_3ds_mapping(int sym, SDL_Event *e, int overwrite) {
	if (!sym) return;
	int m=
		e==NULL ? 0 :
		(0x01000000 |	// active
		(e->type==SDL_JOYBUTTONDOWN ? 0x00040000 : (e->type==SDL_JOYAXISMOTION ? 0x00020000 : (e->type==SDL_KEYDOWN ? 0x00010000 : 0x00080000))) | //type
		(e->type==SDL_KEYDOWN ? ((e->key.keysym.mod & 0xFF) << 8) : (e->type==SDL_JOYAXISMOTION ? (e->jaxis.value <0 ? 0x200 : 0x100 ) : 0)) | // mod
		(e->type==SDL_JOYBUTTONDOWN ? e->jbutton.button : (e->type==SDL_JOYAXISMOTION ? e->jaxis.axis : (e->type==SDL_KEYDOWN ? e->key.keysym.sym : e->button.button)))); //type
	if (overwrite) {
		for (int i=1; i<255; i++)
			if (keymap3ds[i]==m) keymap3ds[i]=0;
	}
	keymap3ds[sym]=m;
	save_3ds_mapping();
}

static char buf[21];
static char *buttonname[SDL_BUTTON_WHEELDOWN+1]={
	"None","Left","Middle","Right","WheelUp","WheelDown"};

char *get_3ds_mapping_name(int i) {
	int k;
	
	if (!keymap3ds[i]) return NULL;
	k=keymap3ds[i];
	switch(k & 0x00FF0000) {
	case 0x00010000:	// key
		snprintf(buf,20,"Key %s",get_3ds_keyname(k & 0xFF));
		break;
#ifdef HAVE_SDL_NUMJOYSTICKS
	case 0x00020000:	// joy axis
		int v,a;
		a = k & 0xFF;
		v = k>>8 &0xFF;
		snprintf(buf,20,"Joy %s",
			a == 1 && v==2 ? "Up":(
			a == 1 && v==1 ? "Down":(
			a == 0 && v==2 ? "Left":
			"Right")));
		break;
	case 0x00040000:	// joy button
		snprintf(buf,20,"Joy Fire%d",k & 0xFF);
		break;
#endif
	case 0x00080000:	// joy button
		snprintf(buf,20,"Mousebutton %s", buttonname[(k & 0xFF)%(SDL_BUTTON_WHEELDOWN+1)]);
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

static char *command[20];

static int set_command(const char *val, void *param)
{
    int idx=(int)param;

    if (val) {
        util_string_set(&command[idx], val);
	} else {
		util_string_set(&command[idx], "");
    }
    return 0;
}

static resource_string_t resources_string[] = {
	{ "Command01", "load\"*\",8,1\\run\\", RES_EVENT_NO, NULL,
		&command[0], set_command, (void*)0},
	{ "Command02", "load\"$\",8\\list\\", RES_EVENT_NO, NULL,
		&command[1], set_command, (void*)1},
	{ "Command03", "", RES_EVENT_NO, NULL,
		&command[2], set_command, (void*)2},
	{ "Command04", "", RES_EVENT_NO, NULL,
		&command[3], set_command, (void*)3},
	{ "Command05", "", RES_EVENT_NO, NULL,
		&command[4], set_command, (void*)4},
	{ "Command06", "", RES_EVENT_NO, NULL,
		&command[5], set_command, (void*)5},
	{ "Command07", "", RES_EVENT_NO, NULL,
		&command[6], set_command, (void*)6},
	{ "Command08", "", RES_EVENT_NO, NULL,
		&command[7], set_command, (void*)7},
	{ "Command09", "", RES_EVENT_NO, NULL,
		&command[8], set_command, (void*)8},
	{ "Command10", "", RES_EVENT_NO, NULL,
		&command[9], set_command, (void*)9},
	{ "Command11", "", RES_EVENT_NO, NULL,
		&command[10], set_command, (void*)10},
	{ "Command12", "", RES_EVENT_NO, NULL,
		&command[11], set_command, (void*)11},
	{ "Command13", "", RES_EVENT_NO, NULL,
		&command[12], set_command, (void*)12},
	{ "Command14", "", RES_EVENT_NO, NULL,
		&command[13], set_command, (void*)13},
	{ "Command15", "", RES_EVENT_NO, NULL,
		&command[14], set_command, (void*)14},
	{ "Command16", "", RES_EVENT_NO, NULL,
		&command[15], set_command, (void*)15},
	{ "Command17", "", RES_EVENT_NO, NULL,
		&command[16], set_command, (void*)16},
	{ "Command18", "", RES_EVENT_NO, NULL,
		&command[17], set_command, (void*)17},
	{ "Command19", "", RES_EVENT_NO, NULL,
		&command[18], set_command, (void*)18},
	{ "Command20", "", RES_EVENT_NO, NULL,
		&command[19], set_command, (void*)19},
	RESOURCE_STRING_LIST_END
};


int vice3ds_resources_init(void)
{
	if (resources_register_string(resources_string) < 0) {
		return -1;
	}
//	if (resources_register_int(resources_int) < 0) {
//		return -1;
//	}
	return 0;
}

void vice3ds_resources_shutdown(void)
{
	LED3DS_Off();
	lib_free(keymap3ds_resource);
	keymap3ds_resource=NULL;
}

int do_common_3DS_actions(SDL_Event *e){
	// deactivate help screen if applicable
	if (help_on) {
		if (e->type == SDL_KEYDOWN || e->type==SDL_MOUSEBUTTONDOWN) {
			SDL_Event v;
			toggle_help(sdl_menu_state);
			while (SDL_PollEvent(&v)); // empty event queue
		}
		return 1;
	}
	if (e->type == SDL_KEYDOWN) {
		if (e->key.keysym.sym == 208) { // start
			if (_mouse_enabled)	{
				set_mouse_enabled(0, NULL);
				return 1;
			}
			if (uibottom_editmode_is_on()) {
				uibottom_toggle_editmode();
				return 1;
			}
		}
		if (e->key.keysym.sym == 255) {
			toggle_keyboard();
			return 1;
		}
	}
	return 0;
}