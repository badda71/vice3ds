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

extern int LED3DS_On(unsigned char r, unsigned char g, unsigned char b);
extern int LED3DS_Off(void);

extern int start_worker(int (*fn)(void *), void *data);

extern int keymap3ds[256];
extern char *keymap3ds_resource;
extern int do_3ds_mapping(SDL_Event *e);
extern void set_3ds_mapping(int sym, SDL_Event *e, int overwrite);
extern char *get_3ds_mapping_name(int i);
extern int keymap3ds_resource_set(const char *val, void *param);

extern void vice3ds_shutdown(void);