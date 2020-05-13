/*
 * vice3ds.h - Function specific to vice3DS port
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
#include <SDL/SDL.h>

#define KEYMAPPINGS_DEFAULT " c9 01d20000 cc 02010000 ce 02030000 d6 011e0000 d7 01110000 d8 011f0000 d9 011d0000 e8 01031900 f6 01030000 f7 01200000 f8 010d0000"
#define HELPTEXT_MAX 256

typedef struct {
	Handle mutex;
	void **queue;
	int size;
	int head;
	int tail;
	int locked;
} tsq_object;

typedef struct {
	char *key;
	void *val;
} tsh_item;

typedef struct {
	Handle mutex;
	tsh_item *hash;
	int size;
	int locked;
	void (*free_callback)(void *val);
} tsh_object;

extern u32 hashKey(u8 *key);
extern void tsh_init(tsh_object *o, int size, void (*free_callback)(void *val));
extern void *tsh_get(tsh_object *o, char *key);
extern int tsh_put(tsh_object *o, char *key, void *val);
extern void tsh_free(tsh_object *o);

extern void tsq_init(tsq_object *o, int size);
extern void tsq_lock(tsq_object *o, int lock);
extern void tsq_free(tsq_object *o);
extern void *tsq_get(tsq_object *o);
extern void *tsq_put(tsq_object *o, void *p);
extern void *tsq_pop(tsq_object* o);

extern char *chg_root_directory;
extern int keymap3ds[256];
extern char *keymap3ds_resource;
extern char *custom_help_text[HELPTEXT_MAX];

extern int LED3DS_On(unsigned char r, unsigned char g, unsigned char b);
extern int LED3DS_Off(void);
extern int start_worker(int (*fn)(void *), void *data);
extern int do_3ds_mapping(SDL_Event *e);
extern int save_3ds_mapping();
extern void set_3ds_mapping(int sym, int key_or_mouse, int key1, int key2, int key3);
extern char *get_3ds_mapping_name(int i);
extern int keymap3ds_resource_set(const char *val, void *param);
extern int vice3ds_resources_init(void);
extern void vice3ds_resources_shutdown(void);
extern int do_common_3DS_actions(SDL_Event *e);
extern void waitSync(int);
extern void triggerSync(int);
extern void copy_autocopy_dir();
extern int save_help_texts_to_resource();
extern int isN3DS();
extern char *result_translate(Result r);
