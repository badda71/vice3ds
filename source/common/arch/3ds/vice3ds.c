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
#include <arpa/inet.h>
#include <errno.h>

#include "archdep.h"
#include "vice3ds.h"
#include "http.h"
#include "uibottom.h"
#include "util.h"
#include "kbd.h"
#include "mousedrv.h"
#include "lib.h"
#include "log.h"
#include "resources.h"
#include "archdep_xdg.h"
#include "archdep_cp.h"
#include "uigb64.h"

// thread safe hash functions

// Fowler-Noll-Vo Hash (FNV1a)
u32 hashKey(u8 *key) {
	u32 hash=0x811C9DC5;
	while (*key!=0) hash=(*(key++)^hash) * 0x01000193;
	return hash;
}

void tsh_init(tsh_object *o, int size, void (*free_callback)(void *val)) {
	svcCreateMutex(&(o->mutex), false);
	o->hash=malloc(size*sizeof(tsh_item));
	memset(o->hash, 0, size*sizeof(tsh_item));
	o->size=size;
	o->locked=0;
	o->free_callback=free_callback;
}

void tsh_free(tsh_object *o) {
	// free all emements
	int i;
	for (i=0;i<o->size;i++) {
		if (o->hash[i].key != NULL)
			free(o->hash[i].key);
		if (o->hash[i].val != NULL && o->free_callback)
			o->free_callback(o->hash[i].val);
	}
	free(o->hash);
}

void *tsh_get(tsh_object *o, char *key) {
//log_citra("enter %s: %s",__func__,key);
	if (key==NULL) return NULL;
	int i=hashKey((u8*)key) % o->size;
	int count=0;
	void *r=NULL;
	svcWaitSynchronization(o->mutex, U64_MAX);
	while (o->hash[i].key != NULL && ++count <= o->size) {
		if (strcmp(key,o->hash[i].key)==0) {
			r=o->hash[i].val;
			break;
		}
		++i; i %= o->size;
	}
	svcReleaseMutex(o->mutex);
	return r;
}

int tsh_put(tsh_object *o, char *key, void *val) {
//log_citra("enter %s: %s",__func__,key);
	if (key==NULL) return -1;
	int i=hashKey((u8*)key) % o->size;
	int count = 0;
	svcWaitSynchronization(o->mutex, U64_MAX);
	while (o->hash[i].key != NULL && strcmp(o->hash[i].key, key) != 0) {
		++i; i %= o->size;
		if (++count >= o->size) {
			svcReleaseMutex(o->mutex);
			return -1;
		}
	}
	if (o->hash[i].key != NULL) {
		lib_free(o->hash[i].key);
		if (o->free_callback) o->free_callback(o->hash[i].val);
	}
	o->hash[i].key = (val == NULL) ? NULL : lib_stralloc(key);
	o->hash[i].val = val;
	svcReleaseMutex(o->mutex);
	return 0;
}

// thread safe queue functions
void tsq_init(tsq_object *o, int size) {
	svcCreateMutex(&(o->mutex), false);
	o->queue=malloc(size*sizeof(void*));
	memset(o->queue, 0, size*sizeof(void*));
	o->size=size;
	o->head=o->tail=0;
	o->locked=0;
}

void tsq_free(tsq_object *o) {
	tsq_lock(o, 1);
	free(o->queue);
}

void tsq_lock(tsq_object *o, int lock) {
	svcWaitSynchronization(o->mutex, U64_MAX);
	o->locked=lock;
	svcReleaseMutex(o->mutex);
}

void *tsq_get(tsq_object* o) {
	void *r = NULL;
	svcWaitSynchronization(o->mutex, U64_MAX);
	if (o->tail != o->head) {
		r = o->queue[o->tail];
		o->queue[o->tail]=NULL;
		o->tail = ( o->tail + 1 ) % o->size;
	}
	svcReleaseMutex(o->mutex);
	return r;
}

void *tsq_pop(tsq_object* o) {
	void *r = NULL;
	svcWaitSynchronization(o->mutex, U64_MAX);
	if (o->tail != o->head) {
		o->head = ( o->head + o->size - 1 ) % o->size;
		r=o->queue[o->head];
		o->queue[o->head] = NULL;
	}
	svcReleaseMutex(o->mutex);
	return r;
}

void *tsq_put(tsq_object* o, void *p) {
	void *r = NULL;
	svcWaitSynchronization(o->mutex, U64_MAX);
	if (!o->locked) {
		r=o->queue[o->head];
		o->queue[o->head]=p;
		o->head = ( o->head + 1 ) % o->size;
		if (o->tail == o->head) // if overwriting an old entry, tail is moved as well
			o->tail = ( o->tail + 1 ) % o->size;
	}
	svcReleaseMutex(o->mutex);
	return r;
}

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

int save_3ds_mapping() {
	int i,c=0;
	if (keymap3ds_resource != NULL) free(keymap3ds_resource);
	for (i=1;i<255;i++)
		if (keymap3ds[i]) ++c;
	keymap3ds_resource=malloc(c*12+1);
	c=0;
	for (i=1;i<255;i++)
		if (keymap3ds[i])
			c+=sprintf(keymap3ds_resource+c," %02x %08x",i,keymap3ds[i]);
	keymap3ds_resource[c]=0;
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

	// shutdown other 3ds stuff
	gb64_shutdown();
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

static int is_n3ds=-1;

int isN3DS() {
	if (is_n3ds == -1) {
		u8 i;
		cfguInit();
		CFGU_GetSystemModel(&i);
		cfguExit();
		is_n3ds = (i==2 || i==4 || i==5) ? 1 : 0;
	}
	return is_n3ds;
}

typedef struct {
	u32 num;
	char *string;
} err_t;

static err_t err_module[]={
	{0, "Common"},
	{1, "Kernel"},
	{2, "Util"},
	{3, "File server"},
	{4, "Loader server"},
	{5, "TCB"},
	{6, "OS"},
	{7, "DBG"},
	{8, "DMNT"},
	{9, "PDN"},
	{10, "GX"},
	{11, "I2C"},
	{12, "GPIO"},
	{13, "DD"},
	{14, "CODEC"},
	{15, "SPI"},
	{16, "PXI"},
	{17, "FS"},
	{18, "DI"},
	{19, "HID"},
	{20, "CAM"},
	{21, "PI"},
	{22, "PM"},
	{23, "PM_LOW"},
	{24, "FSI"},
	{25, "SRV"},
	{26, "NDM"},
	{27, "NWM"},
	{28, "SOC"},
	{29, "LDR"},
	{30, "ACC"},
	{31, "RomFS"},
	{32, "AM"},
	{33, "HIO"},
	{34, "Updater"},
	{35, "MIC"},
	{36, "FND"},
	{37, "MP"},
	{38, "MPWL"},
	{39, "AC"},
	{40, "HTTP"},
	{41, "DSP"},
	{42, "SND"},
	{43, "DLP"},
	{44, "HIO_LOW"},
	{45, "CSND"},
	{46, "SSL"},
	{47, "AM_LOW"},
	{48, "NEX"},
	{49, "Friends"},
	{50, "RDT"},
	{51, "Applet"},
	{52, "NIM"},
	{53, "PTM"},
	{54, "MIDI"},
	{55, "MC"},
	{56, "SWC"},
	{57, "FatFS"},
	{58, "NGC"},
	{59, "CARD"},
	{60, "CARDNOR"},
	{61, "SDMC"},
	{62, "BOSS"},
	{63, "DBM"},
	{64, "Config"},
	{65, "PS"},
	{66, "CEC"},
	{67, "IR"},
	{68, "UDS"},
	{69, "PL"},
	{70, "CUP"},
	{71, "Gyroscope"},
	{72, "MCU"},
	{73, "NS"},
	{74, "News"},
	{75, "RO"},
	{76, "GD"},
	{77, "Card SPI"},
	{78, "EC"},
	{79, "RO"},
	{80, "Web Browser"},
	{81, "Test"},
	{82, "ENC"},
	{83, "PIA"},
	{92, "MVD"},
	{96, "QTM"},
	{254, "Application"},
	{255, "Invalid Result Value"},
	{0, NULL}
};

static err_t err_description[] = {
	{0, "Success"},
	{2, "Invalid memory permissions (kernel)"},
	{4, "Invalid ticket version (AM)"},
	{5, "String too big? This error is returned when service name length is greater than 8. (srv)"},
	{6, "Access denied? This error is returned when you request a service that you don't have access to. (srv)"},
	{7, "String too small? This error is returned when service name contains an unexpected null byte. (srv)"},
	{10, "Not enough memory (os)"},
	{26, "Session closed by remote (os)"},
	{30, "Port name too long (os)"},
	{37, "Invalid NCCH? (AM)"},
	{39, "Invalid title version (AM)"},
	{43, "Database doesn't exist / failed to open (AM)"},
	{44, "Trying to uninstall system-app (AM)"},
	{120, "Title/object not found? (fs)"},
	{141, "Gamecard not inserted? (fs)"},
	{230, "Invalid open-flags / permissions? (fs)"},
	{391, "NCCH hash-check failed? (fs)"},
	{392, "RSA/AES-MAC verification failed? (fs)"},
	{395, "RomFS hash-check failed? (fs)"},
	{630, "Command not allowed / missing permissions? (fs)"},
	{702, "Invalid path? (fs)"},
	{761, "Incorrect read-size for ExeFS? (fs)"},
	{1000, "Invalid section"},
	{1001, "Too large"},
	{1002, "Not authorized"},
	{1003, "Already done"},
	{1004, "Invalid size"},
	{1005, "Invalid enum value"},
	{1006, "Invalid combination"},
	{1007, "No data"},
	{1008, "Busy"},
	{1009, "Misaligned address"},
	{1010, "Misaligned size"},
	{1011, "Out of memory"},
	{1012, "Not implemented"},
	{1013, "Invalid address"},
	{1014, "Invalid pointer"},
	{1015, "Invalid handle"},
	{1016, "Not initialized"},
	{1017, "Already initialized"},
	{1018, "Not found"},
	{1019, "Cancel requested"},
	{1020, "Already exists"},
	{1021, "Out of range"},
	{1022, "Timeout"},
	{1023, "Invalid result value"},
	{0, NULL}
};

/*
static err_t err_level[] = {
	{0, "Success"},
	{1, "Info"},
	{25, "Status"},
	{26, "Temporary"},
	{27, "Permanent"},
	{28, "Usage"},
	{29, "Reinitialize"},
	{30, "Reset"},
	{31, "Fatal"},
	{0, NULL}
};

static err_t err_summary[] = {
	{0, "Success"},
	{1, "Nothing happened"},
	{2, "Would block"},
	{3, "Out of resource"},
	{4, "Not found"},
	{5, "Invalid state"},
	{6, "Not supported"},
	{7, "Invalid argument"},
	{8, "Wrong argument"},
	{9, "Canceled"},
	{10, "Status changed"},
	{11, "Internal"},
	{63, "Invalid Result Value"},
	{0, NULL}
};
*/

static char *get_entry(err_t *list, u32 key) {
	int i=0;
	while (list[i].string != NULL) {
		if (key == list[i].num) return list[i].string;
		++i;
	}
	return NULL;
}

char *result_translate(Result r) {
	static char buf[256];
	u32 desc = r & 0x3FF;
	u32 mod  = (r >> 10) & 0xFF;
//	u32 sum = (r >> 21) & 0x3F;
//	u32 lvl = (r >> 27) & 0x1F;

	char *s_desc = get_entry(err_description, desc);
	char *s_mod = get_entry(err_module, mod);
//	char *s_sum = get_entry(err_summary, sum);
//	char *s_lvl = get_entra(err_level, lvl);

	snprintf(buf, 256, "%s in %s Module", s_desc?s_desc:"Error", s_mod?s_mod:"Unknown");
	return buf;
}

// ************************************************************************
// autodiscovery server & client function
// ************************************************************************

static int disc_serv_socket=-1;
static int disc_client_socket=-1;
const static char disc_req_msg[] = "vice3DS-WHO_IS_SERVER";
const static char disc_ack_msg[] = "vice3DS-I_AM_SERVER";
const static int disc_port = 19050;
static char disc_name[40] = {0};

int disc_start_server()
{
	if (disc_serv_socket > -1) return disc_serv_socket;
	if (archdep_network_init()) return -1;

	if (*disc_name == 0) {
		frdInit();
		FRD_GetMyScreenName(disc_name, sizeof(disc_name));
		frdExit();
		if (*disc_name == 0)
			strcpy(disc_name,"-");
	}

	struct sockaddr_in server_addr = {0};

	disc_serv_socket = socket(AF_INET, SOCK_DGRAM, 0);
	if (disc_serv_socket < 0) {
		log_error(LOG_ERR, "socket(): %s",strerror(errno));
		return -1;
	}
	int broadcast=1;
	if(setsockopt(disc_serv_socket,SOL_SOCKET,0x04,&broadcast,sizeof(broadcast)) < 0) {
		log_error(LOG_ERR, "setsockopt(): %s",strerror(errno));
		disc_stop_server();
		return -1;
	}
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = INADDR_ANY;
	server_addr.sin_port = htons(disc_port);

	int ret = bind(disc_serv_socket, (struct sockaddr*)&server_addr, sizeof(server_addr));
	if (ret < 0) {
		disc_stop_server();
		return -1;
	}
//log_citra("disc server started on port %d\n",disc_port);
	return disc_serv_socket;
}

void disc_stop_server()
{
	if (disc_serv_socket>-1) {
		closesocket(disc_serv_socket);
		disc_serv_socket = -1;
//log_citra("disc server stopped\n");
	}
}
void disc_run_server()
{
	char buffer[128];
	struct sockaddr_in client_addr;
	socklen_t addr_len = sizeof(client_addr);
	recvfrom(disc_serv_socket, buffer, sizeof(buffer), 0, (struct sockaddr*)&client_addr, &addr_len);
//log_citra("disc server received '%s' from %s : %d\n",buffer, inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
	if (strstr(buffer, disc_req_msg)) {
		int p;
		resources_get_int( "NetworkServerPort", &p );
		snprintf(buffer, sizeof(buffer), "%s:%s:%d", disc_ack_msg, disc_name, p);
//log_citra("sending response: %s\n", buffer);
		sendto(disc_serv_socket, buffer, strlen(buffer), 0, (struct sockaddr*)&client_addr, addr_len);
	}
}

static unsigned long disc_get_broadcast_addr() {
	static unsigned long s_addr=0;
	if (!s_addr) {
		// check SOCU_IPInfo first;
		SOCU_IPInfo ipinfo={0};
		socklen_t ipinfo_len = sizeof(ipinfo);
		SOCU_GetNetworkOpt(SOL_CONFIG, NETOPT_IP_INFO , &ipinfo, &ipinfo_len);
		if (ipinfo.broadcast.s_addr) s_addr = ipinfo.broadcast.s_addr;
		else {
			s_addr = INADDR_BROADCAST;
		}
	}
	return s_addr;
}

int disc_run_client(int send_broadcast, disc_server *d_server)
{
	static struct sockaddr_in broadcastaddr = {0};

	if (disc_client_socket < 0) {
		if (archdep_network_init()) {
			log_error(LOG_ERR, "could not init network");
			return -2;
		}
		disc_client_socket = socket(AF_INET, SOCK_DGRAM, 0);
	    if (disc_client_socket < 0) {
			log_error(LOG_ERR, "socket(): %s",strerror(errno));
			return -2;
		}

		int broadcastEnable = 1;
		if (setsockopt(disc_client_socket, SOL_SOCKET, 0x04, &broadcastEnable, sizeof(broadcastEnable)) < 0) {
			log_error(LOG_ERR, "setsockopt(): %s",strerror(errno));
			disc_stop_client();
			return -2;
		}
		broadcastaddr.sin_family = AF_INET;
		broadcastaddr.sin_port = htons(disc_port);
		broadcastaddr.sin_addr.s_addr = disc_get_broadcast_addr();
	}
	struct sockaddr_in server_addr;
	fd_set readfd;
	char buffer[128];
	socklen_t addr_len = sizeof(server_addr);
	char *saveptr,*p;

	struct timeval t={0, 0};
	
	// send request
	if (send_broadcast) {
//log_citra("disc client broadcast to %s %d\n", inet_ntoa(broadcastaddr.sin_addr), disc_port);
		sendto(disc_client_socket, disc_req_msg, strlen(disc_req_msg), 0, (struct sockaddr *)&broadcastaddr, sizeof(broadcastaddr));
	}

	// check for ack
	FD_ZERO(&readfd);
	FD_SET(disc_client_socket, &readfd);
	int ret = select(disc_client_socket + 1, &readfd, NULL, NULL, &t);
	if (ret > 0) {
//log_citra("select returned ok\n");
		if (FD_ISSET(disc_client_socket, &readfd))
		{
			recvfrom(disc_client_socket, buffer, sizeof(buffer), 0, (struct sockaddr*)&server_addr, &addr_len);
//log_citra("disc client received '%s' from %s : %d\n",buffer, inet_ntoa(server_addr.sin_addr), ntohs(server_addr.sin_port));
			if (strstr(buffer, disc_ack_msg)) {
				if (strtok_r(buffer,":",&saveptr) && (p=strtok_r(NULL,":",&saveptr)))
					snprintf(d_server->name, sizeof(d_server->name), "%s", p);
				if ((p=strtok_r(NULL,":",&saveptr)))
					d_server->port = atoi(p);
				snprintf(d_server->addr, sizeof(d_server->addr), "%s", inet_ntoa(server_addr.sin_addr));
//log_citra("disc client finish: %s %s %d", d_server->name, d_server->addr, d_server->port);
				return 0;
			}
		}
	}
	return -1;
}

void disc_stop_client()
{
	if (disc_client_socket > -1) {
		closesocket(disc_client_socket);
		disc_client_socket = -1;
//log_citra("disc client stopped\n");
	}
}

int checkWifi() {
	static int isAcInit=0;
	u32 wifi_status;
	if (!isAcInit) {
		acInit();
		atexit(acExit);
		isAcInit=1;
	}

#ifndef CITRA
	ACU_GetWifiStatus(&wifi_status);
	if (wifi_status == 0) {
		return 0;
	}
#endif
	return 1;
}
