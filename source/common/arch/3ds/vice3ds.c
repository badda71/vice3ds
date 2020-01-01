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
#include <sys/stat.h>
#include "archdep.h"
#include "vice3ds.h"
#include "uibottom.h"
#include "util.h"
#include "kbd.h"
#include "mousedrv.h"
#include "lib.h"
#include "log.h"
#include "resources.h"
#include "archdep_xdg.h"
#include "archdep_cp.h"

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
int keymap3ds[256] = {0}; // type (1|2) - (key|mousebutton) - (combokey2) - (combokey3)

static int load_3ds_mapping(char *s) {
	unsigned long int i,k;
	memset(keymap3ds,0,sizeof(keymap3ds));
	while(s && (i=strtoul(s,&s,16))!=0 && i<256 && s) {
		k=strtoul(s,&s,16);
		keymap3ds[i] = k;
	}
	uibottom_must_redraw |= UIB_RECALC_SBUTTONS;

	return 0;
}

static int save_3ds_mapping() {
	int i,c=0;
	if (keymap3ds_resource != NULL) free(keymap3ds_resource);
	for (i=1;i<255;i++)
		if (keymap3ds[i]) ++c;
	keymap3ds_resource=malloc(c*12+1);
	c=0;
	for (i=1;i<255;i++)
		if (keymap3ds[i])
			c+=sprintf(keymap3ds_resource+c," %02x %08x",i,keymap3ds[i]);
	keymap3ds_resource[c-1]=0;
	return 0;
}

int do_3ds_mapping(SDL_Event *e) {
	int i,x,i1;
	if (e->type != SDL_KEYDOWN && e->type != SDL_KEYUP) return 0; // not the right event type
	if (e->key.keysym.mod & KMOD_RESERVED) return 0; // event was already mapped - this prevents loops
	if ((i=keymap3ds[e->key.keysym.sym]) == 0 ) return 0; // not mapped
	switch (i >> 24) {
		case 1:
			// modify the current event
			e->key.keysym.unicode = e->key.keysym.sym = (i >> 16) & 0xFF;
			// fire additional events if needed
			for (x=8; x >=0 ; x -= 8) {
				if ((i1 = (i >> x) & 0xFF ) == 0) break;
				SDL_Event sdl_e;
				sdl_e.type = e->type;
				sdl_e.key.keysym.unicode = sdl_e.key.keysym.sym = i1;
				sdl_e.key.keysym.mod = KMOD_RESERVED;
				SDL_PushEvent(&sdl_e);
			}
			break;
		case 2:
			e->button.which = 1; // mouse 0 is the touchpad, mouse 1 is the mapped mouse keys
			e->button.button = (i >> 16) & 0xFF;
			e->button.state = e->type == SDL_KEYDOWN ? SDL_PRESSED : SDL_RELEASED;
			e->type = e->type == SDL_KEYDOWN ? SDL_MOUSEBUTTONDOWN : SDL_MOUSEBUTTONUP;
			e->button.x = mousedrv_get_x();
			e->button.y = mousedrv_get_y();
			break;
	}
	return 1;
}

void set_3ds_mapping(int sym, int key_or_mouse, int key1, int key2, int key3) {
	if (!sym) return;
	int m = key_or_mouse == 0 ? 0 : (key_or_mouse << 24 | key1 << 16 | key2 <<8 | key3);
	keymap3ds[sym]=m;
	save_3ds_mapping();
}

static char buf[41];
static char *buttonname[SDL_BUTTON_WHEELDOWN+1]={
	"None","Left","Middle","Right","WheelUp","WheelDown"};

char *get_3ds_mapping_name(int i) {
	int k,k1;
	
	if (!keymap3ds[i]) return NULL;
	k=keymap3ds[i];
	switch(k >> 24) {
	case 1:	// key
		snprintf(buf,41,"Key %s",get_3ds_keyname((k >> 16) & 0xFF));
		for (int x=8; x>=0; x -= 8) {
			if ((k1=(k >> x) & 0xFF)==0 || strlen(buf) >= 40) break;
			snprintf(buf+strlen(buf), 41-strlen(buf), " + %s",get_3ds_keyname((k >> x) & 0xFF));
		}
		break;
	case 2:	// mouse button
		snprintf(buf,41,"Mousebutton %s", buttonname[((k >> 16) & 0xFF) % (SDL_BUTTON_WHEELDOWN+1) ]);
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

char *chg_root_directory;

static int set_chg_root_directory(const char *val, void *param)
{
    const char *s = val;

	if (s==NULL || s[0]==0) {
		if (chg_root_directory) free(chg_root_directory);
		chg_root_directory=lib_stralloc("");
		return 0;
	}

	char *p = malloc(strlen(s)+2);
	strcpy(p,s);
	// Make sure string does not end with FSDEV_DIR_SEP_STR
    while (p[0]!=0 && p[strlen(p) - 1] == FSDEV_DIR_SEP_CHR) {
		p[strlen(p) - 1]=0;
	}
	struct stat sb;
	if (stat(p, &sb) == 0 && S_ISDIR(sb.st_mode)) {
//		strcpy(p+strlen(p),FSDEV_DIR_SEP_STR); // add the trailing '/'
		if (chg_root_directory) free(chg_root_directory);
		chg_root_directory=p;
		return 0;
	}
	free(p);
	return -1;
}

char *custom_help_text[HELPTEXT_MAX]={NULL};
static char* custom_help_texts=NULL;

static int load_help_texts_from_resource() {
    char *p,*saveptr;
    int keynum;

	if (custom_help_texts == NULL) return 0;
	char *buffer=lib_stralloc(custom_help_texts);
    // clear help texts
	for (int i = 0; i < HELPTEXT_MAX; ++i) {
		if (custom_help_text[i]) free(custom_help_text[i]);
		custom_help_text[i] = NULL;
    }

	p = strtok_r(buffer, " \t:", &saveptr);
	while (p) {
		keynum = (int)strtol(p,NULL,10);

		if (keynum >= HELPTEXT_MAX || keynum<1) {
			log_error(LOG_ERR, "Help text keynum not valid: %i!", keynum);
			strtok_r(NULL, "|\r\n", &saveptr);
		} else if ((p = strtok_r(NULL, "|\r\n", &saveptr)) != NULL) {
			custom_help_text[keynum]=lib_stralloc(p);
		}
		p = strtok_r(NULL, " \t:", &saveptr);
	}
	lib_free(buffer);
	uibottom_must_redraw |= UIB_RECALC_SBUTTONS;
	return 0;
}

int save_help_texts_to_resource() {
	if (custom_help_texts) free(custom_help_texts);
	custom_help_texts=malloc(1);
	custom_help_texts[0]=0;
	int count=0;

	// write the hotkeys
	for (int i = 0; i < HELPTEXT_MAX; ++i) {
		if (custom_help_text[i]) {
			custom_help_texts=realloc(custom_help_texts, strlen(custom_help_texts) + strlen (custom_help_text[i]) + 7);
			sprintf(custom_help_texts + strlen(custom_help_texts), "%s%d %s", count++?"|":"", i, custom_help_text[i]);
		}
	}
	return 0;
}

static int set_custom_help_texts(const char *val, void *param)
{
	if (util_string_set(&custom_help_texts, val)) {
		return 0;
	}
	return load_help_texts_from_resource();
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
	
	{ "ChgRootDirectory", "", RES_EVENT_NO, NULL,
		&chg_root_directory, set_chg_root_directory, NULL},

	{ "CustomHelpTexts", "", RES_EVENT_NO, NULL,
		&custom_help_texts, set_custom_help_texts, NULL},
	RESOURCE_STRING_LIST_END
};
/*
static const resource_int_t resources_int[] = {
    { "ChgRootEnable", 0, RES_EVENT_NO, (resource_value_t)0,
      &chg_root_enable, set_chg_root_enable, NULL },
	RESOURCE_INT_LIST_END
};
*/
#define MAX_SYNC_HANDLES 1
static Handle sync_handle[MAX_SYNC_HANDLES] = {0L};

int vice3ds_resources_init(void)
{
	if (resources_register_string(resources_string) < 0) {
		return -1;
	}
/*	if (resources_register_int(resources_int) < 0) {
		return -1;
	}*/
	for (int i=0; i<MAX_SYNC_HANDLES;i++) {
		svcCreateEvent(&(sync_handle[i]),0);
	}
	return 0;
}

void vice3ds_resources_shutdown(void)
{
	LED3DS_Off();
	lib_free(keymap3ds_resource);
	keymap3ds_resource=NULL;
	for (int i=0; i<MAX_SYNC_HANDLES;i++) {
		if (sync_handle[i]) {
			svcCloseHandle(sync_handle[i]);
			sync_handle[i]=0L;
		}
	}
	// free custom help texts
	for (int i = 0; i < HELPTEXT_MAX; ++i) {
		if (custom_help_text[i]) free(custom_help_text[i]);
		custom_help_text[i] = NULL;
    }
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

void waitSync(int i)
{
	if (i>=MAX_SYNC_HANDLES) return;
	svcWaitSynchronization(sync_handle[i], U64_MAX);
	svcClearEvent(sync_handle[i]);
}

void triggerSync(int i)
{
	if (i>=MAX_SYNC_HANDLES) return;
	svcSignalEvent(sync_handle[i]);
}

void copy_autocopy_dir() {
	xcopy("romfs:/autocopy",archdep_xdg_data_home(),0,NULL);
}
