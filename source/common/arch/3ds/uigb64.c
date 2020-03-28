/*
 * uigb64.c - Functions specific to gamebase64 interface
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
#include <dirent.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <3ds.h>
#include <curl/curl.h>
#include <SDL/SDL_image.h>
#include <zip.h>

#include "archdep_cp.h"
#include "archdep_xdg.h"
#include "async_http.h"
#include "attach.h"
#include "autostart.h"
#include "c64model.h"
#include "joyport.h"
#include "http.h"
#include "kbd.h"
#include "lib.h"
#include "mouse.h"
#include "log.h"
#include "menu_common.h"
#include "menu_joyport.h"
#include "palette.h"
#include "persistence.h"
#include "resources.h"
#include "tape.h"
#include "ui.h"
#include "uiapi.h"
#include "uibottom.h"
#include "uimenu.h"
#include "uimsgbox.h"
#include "util.h"
#include "vice3ds.h"
#include "videoarch.h"

// defines
#define GB64_FIRST_Y 3
#define GB64_SS_URL "http://www.gb64.com/Screenshots/"
#define GB64_GAME_URL "ftp://8bitfiles.net/gamebase_64/Games/"
#define GB64_DBGZ_URL "http://badda.de/vice3ds/gb64/gb64.db.gz"

typedef struct {
	char *blob;
	long blobsize;
	int num_entries;
	char **entries;
} picoDB;

// exposed variables
SDL_Surface *uigb64_top=NULL;

// static variables
static int havescrollbar=0;
static menu_draw_t *menu_draw;
static volatile int gb64_top_must_redraw=0;
static SDL_Surface *loading_img=NULL;
static u8 *downloaded;
static char *gamedir=NULL;
static int list_filter;
static picoDB *db=NULL;
static u32 wifi_status = 0;
static int *searchresult=NULL;
static int gb64_set_modes = 7;

static picoDB *pdb_initDB(char *filename) {
	// read in the file
	FILE *f;

	if ((f = fopen(filename, "rb")) == NULL) {
		log_error(LOG_DEFAULT,"Could not open %s",filename);
		return NULL;
	}

	fseek(f, 0, SEEK_END);
	long fsize = ftell(f);
	fseek(f, 0, SEEK_SET);  /* same as rewind(f); */

	char *string = malloc(fsize + 1);
	if (string==NULL) {
		fclose(f);
		log_error(LOG_DEFAULT,"Could not allocate memory for database");
		return NULL;
	}
	fread(string, 1, fsize, f);
	fclose(f);

	// check the entries
	int count=0;
	if (string[0]!='#') ++count;
	for (int i=0; i<fsize; i++) if (string[i]=='\n' && string[i+1]!='#') ++count;
	char **entries=malloc((count+1)*sizeof(char*));
	if (entries==NULL) {
		free(string);
		log_error(LOG_DEFAULT,"Could not allocate memory for entry table");
		return NULL;
	}
	count=0;
	if (string[0]!='#') entries[count++]=string;
	for (int i=0; i<fsize; i++) {
		switch (string[i]) {
			case '\n':
				if (string[i+1]!='#') entries[count++]=string+i+1; // skip comments
				// fallthrough
			case '\t':
				string[i]=0;
		}
	}
	entries[count]=string+fsize;

	// make struct
	picoDB *d = malloc(sizeof(picoDB));
	if (d==NULL) {
		free(string);
		free(entries);
		log_error(LOG_DEFAULT,"Could not allocate memory for database descriptor");
		return NULL;
	}
	d->blob=string;
	d->blobsize=fsize;
	d->num_entries=count;
	d->entries=entries;
	return d;
}

static void pdb_freeDB(picoDB *d) {
	free(d->entries);
	free(d->blob);
	free(d);
}

static char *pdb_getEntry(picoDB *d, int row, int col) {
	if (row<0 || row>=d->num_entries) return NULL;
	char *p=d->entries[row];
	char *e=d->entries[row+1];
	int count=0;
	char *i=p;
	do {
		if (count==col) return i;
		if (*i==0) ++count;
		i++;
	} while (i<e);
	return NULL;
}

static int sdl_ui_mediumprint(const char *text, int pos_x, int pos_y)
{
	const int fw = 5, fh = 8;
	int i = 0, x, y;
	u8 c, *draw_pos, *p= mediumchars->pixels;

	if (text == NULL) return 0;

	if ((pos_x >= menu_draw->max_text_x) || (pos_y >= menu_draw->max_text_y))
		return -1;

	while (((c = text[i]) != 0) && (pos_x * activefont.w + i * fw < menu_draw->max_text_x * activefont.w - fw)) {
		c=(c & 0x7f)-32;
		if (c<0) c=0;
		draw_pos =
			menu_draw_buffer +
			pos_y*activefont.h*menu_draw->pitch +
			pos_x*activefont.w +
			i*fw +
			menu_draw->offset;

		for (y = 0; y < fh; ++y) {
			int sy= (c>>4) * fh + y;
			for (x = 0; x < fw; ++x) {
				int sx= (c & 0x0F) * fw + x;
				u8 col= p[sx + sy * mediumchars->pitch] ? 12 : menu_draw->color_back;
				draw_pos[x] = col;
			}
			draw_pos += menu_draw->pitch;
		}
		++i;
	}

    return i;
}

#define NAMECOL 1
#define DOWN_COL 5

static void uigb64_redraw(int *searchresult, const char *title, char *search, int searchpos, int offset, int num_items, int more, int cur_offset, int total)
{
    int i, j, x;
    char* name;
    uint8_t oldbg;
	char buf[65];

    sdl_ui_clear();
    sdl_ui_display_title(title);
    j = MENU_FIRST_X;
	j += sdl_ui_print("Search: ", j, 1);
	j += sdl_ui_print(search,j,1);
	sdl_ui_invert_char(j-strlen(search)+searchpos, 1);

    for (i = 0; i < num_items; ++i) {

        x=searchresult[offset + i];
        j = MENU_FIRST_X;
        name = pdb_getEntry(db, x, NAMECOL);
		snprintf(buf, 65, " (%s) %s",pdb_getEntry(db, x, 2), pdb_getEntry(db, x, 10));

		if (i == cur_offset) oldbg = sdl_ui_set_cursor_colors();
		if (downloaded[x]) menu_draw->color_front = DOWN_COL;
		j += sdl_ui_print(name, j, i + GB64_FIRST_Y);
		if (i == cur_offset) sdl_ui_print_eol(j, i + GB64_FIRST_Y);
		sdl_ui_mediumprint(buf, j, i + GB64_FIRST_Y);
        if (i == cur_offset) sdl_ui_reset_cursor_colors(oldbg);
		if (downloaded[x]) menu_draw->color_front = 1;
	}

	// scrollbar
	havescrollbar = sdl_ui_draw_scrollbar(GB64_FIRST_Y, menu_draw->max_text_y - GB64_FIRST_Y, offset, total);
}

static void uigb64_redraw_cursor(int *searchresult, int offset, int num_items, int cur_offset, int old_offset)
{
    int j,x;
    char* name;
    uint8_t oldbg = 0;
	char buf[65];

	if (num_items < 1) return;
	if (havescrollbar) --menu_draw->max_text_x;
	if (old_offset>=0) {
		x=searchresult[offset + old_offset];
		name = pdb_getEntry(db, x, NAMECOL);
		snprintf(buf, 65, " (%s) %s",pdb_getEntry(db, x, 2), pdb_getEntry(db, x, 10));
        j = MENU_FIRST_X;
		if (downloaded[x]) menu_draw->color_front = DOWN_COL;
        j += sdl_ui_print(name, j, old_offset + GB64_FIRST_Y);
        sdl_ui_print_eol(j, old_offset + GB64_FIRST_Y );
		sdl_ui_mediumprint(buf, j, old_offset + GB64_FIRST_Y);
		if (downloaded[x]) menu_draw->color_front = 1;
	}
	if (cur_offset>=0) {
		x=searchresult[offset + cur_offset];
		snprintf(buf, 65, " (%s) %s",pdb_getEntry(db, x, 2), pdb_getEntry(db, x, 10));
        oldbg = sdl_ui_set_cursor_colors();
		name = pdb_getEntry(db, x, NAMECOL);
        j = MENU_FIRST_X;
		if (downloaded[x]) menu_draw->color_front = DOWN_COL;
        j += sdl_ui_print(name, j, cur_offset + GB64_FIRST_Y);
        sdl_ui_print_eol(j, cur_offset + GB64_FIRST_Y );
		sdl_ui_mediumprint(buf, j, cur_offset + GB64_FIRST_Y);
		if (downloaded[x]) menu_draw->color_front = 1;
        sdl_ui_reset_cursor_colors(oldbg);

		// update top screen
	}
	if (havescrollbar) ++menu_draw->max_text_x;
}

typedef struct {
	char *path;
	void *param;
} img_callback;

static ui_menu_action_t uigb64_poll_input(int *key, SDLMod *mod)
{
    SDL_Event e;
    ui_menu_action_t retval = MENU_ACTION_NONE;
	int gotkey=0;
	*key=*mod=0;

	// check event queue
	while (!gotkey) {
		if (gb64_top_must_redraw) return -1;
//		sdl_uibottom_draw();
		SDL_Delay(20);
		if (SDL_PollEvent(&e)) {
			switch (e.type) {
			case SDL_KEYDOWN:
				if (e.key.keysym.sym != 0) {
					if (e.key.keysym.sym == 255) {
						toggle_keyboard();
						break;
					}
					retval = sdlkbd_press(SDL2x_to_SDL1x_Keys(e.key.keysym.sym), e.key.keysym.mod, sdl_menu_state);
					*key = e.key.keysym.sym;
					*mod = e.key.keysym.mod;
					gotkey=1;
				}
				break;
			case SDL_KEYUP:
				if (e.key.keysym.sym != 0) {
					retval = sdlkbd_release(SDL2x_to_SDL1x_Keys(e.key.keysym.sym), e.key.keysym.mod, sdl_menu_state);
				}
				break;
			case SDL_MOUSEBUTTONUP:
			case SDL_MOUSEBUTTONDOWN:
			case SDL_MOUSEMOTION:
				retval = sdl_uibottom_mouseevent(&e);
			}
		}
		if (retval != MENU_ACTION_NONE && retval != MENU_ACTION_NONE_RELEASE)
			break;
	}
    return retval;
}

static void uigb64_callback_imgLoad(char *url, char *fname, int result, void* param) {
	int idx=(int)param;
	if (result == 0) {
		gb64_top_must_redraw=1;
	} else {
		if (result == CURLE_HTTP_RETURNED_ERROR && idx!=0) {
			char *xname=lib_stralloc(fname);
			char *p=strchr(xname,'.');
			sprintf(p,".x");
			fclose(fopen(xname,"w"));
			free(xname);
		}
	}
}

static void rmtree(char *path)
{
	char *full_path;
	DIR *dir;
	struct dirent *entry;

	if ((dir = opendir(path)) == NULL) return;
	while ((entry = readdir(dir)) != NULL) {
		if (!strcmp(entry->d_name, ".") || !strcmp(entry->d_name, ".."))
			continue;
		full_path = util_concat(path, "/", entry->d_name, NULL);
		if (entry->d_type == DT_DIR) {
			rmtree(full_path);
		} else {
			unlink(full_path);
		}
		free(full_path);
	}
	closedir(dir);
	rmdir(path);
}

/*
static char *gb64_gamedir_old(int idx) {
	char buf[256];
	char *f = pdb_getEntry(db, idx, 3);
	char *p = strrchr(f,'/');
	p = p?p+1:f;
	snprintf(buf,256,"%s/%d_%s",gamedir,idx,p);
	p = strrchr(buf,'.');
	if (p) *p=0;
	return (lib_stralloc(buf));
}
*/

static char *gb64_gamedir(int idx) {
	char *f = util_concat(gamedir, "/", pdb_getEntry(db, idx, 3), NULL);
	char *p = strrchr(f,'.');
	if (p) *p=0;
	return f;
}

static void gb64_delete(int idx) {
	char *p = gb64_gamedir(idx);
	const char *s;

	// detach any images in this directory
	for (int i=8; i<12; i++) {
		s=file_system_get_disk_name(i);
		if (s && strncmp(file_system_get_disk_name(i), p, strlen(p))==0)
			file_system_detach_disk(i);
	}
	s=tape_get_file_name();
	if (s && strncmp(tape_get_file_name(), p, strlen(p))==0)
			tape_image_detach(1);

	// remove directory
	rmtree(p);
	free(p);
	downloaded[idx]=0;
	ui_message("Successfully deleted \"%s\"", pdb_getEntry(db, idx, NAMECOL));
}

static void gb64_download(int idx) {
	char *buf,buf2[1024],*p;
	// download zip file
	char *title = pdb_getEntry(db, idx, NAMECOL);
	snprintf(buf2, 1024, "Downloading %s", title);
	sdl_ui_init_progress_bar(buf2);
	sdl_ui_refresh();

	char *url = util_concat(GB64_GAME_URL, pdb_getEntry(db, idx, 3), NULL);
	char *fname = util_concat(archdep_xdg_data_home(), "/tmp_game.zip",NULL);
	if (downloadFile(url, fname, downloadProgress, MODE_FILE)) {
		ui_error("Could not download game: %s",http_errbuf);
	} else {
		// unzip to games/<nr>_<zipname>
		int i,len;
		struct zip *za;
	    struct zip_file *zf;
	    struct zip_stat sb;
		FILE *fd;
		buf = gb64_gamedir(idx);
		if ((za = zip_open(fname, 0, NULL)) == NULL) {
			ui_error("Could not extract downloaded game");
		} else {
			for (i = 0; i < zip_get_num_entries(za, 0); i++) {
				if (zip_stat_index(za, i, 0, &sb) == 0) {
					p = util_concat(buf, "/", sb.name, NULL);
					len = strlen(p);
					if (p[len - 1] == '/') continue;
					zf = zip_fopen_index(za, i, 0);
					if (zf) {
						mkpath(p,0);
						fd = fopen(p, "w");
						if (fd) {
							while ((len=zip_fread(zf, buf2, 1024))>0) {
								fwrite(buf2, 1, len, fd);
							}
							fclose(fd);
						}
						zip_fclose(zf);
					}
					free(p);
				}
			}
			zip_close(za);
		}
		free(buf);
		downloaded[idx]=1;
		ui_message("Successfully downloaded \"%s\"", title);
	}
	unlink(fname);
	free(url);
	free(fname);
}

static void uigb64_update_topscreen(SDL_Surface *s, int entry, int *screenshotidx) {
	char buf[256],buf2[256],url[256];
	int j=0,numimg=0;
	char *p,*p1;
	static SDL_Color c_gray=(SDL_Color){0x70,0x70,0x70,0};
	static SDL_Color c_black=(SDL_Color){0x00,0x00,0x00,0};
	static SDL_Color c_blue=(SDL_Color){0x00,0x00,0xff,0};
	static SDL_Color c_green=(SDL_Color){0x00,0x80,0x00,0};

	// clear surface
	SDL_FillRect(s, NULL, SDL_MapRGBA(s->format,208,209,203,255));
	if (entry>=0) {
		// write info to screen
		j=snprintf(buf, 51, "%s", pdb_getEntry(db, entry, 1));
		uib_printstring(s, buf, 0, 200, 50, ALIGN_LEFT, FONT_BIG, c_black);

		j=snprintf(buf, 81, "Published: ");
		uib_printstring(s, buf, 0, 208, 80, ALIGN_LEFT, FONT_MEDIUM, c_blue);
		p=pdb_getEntry(db, entry, 10);
		snprintf(buf, 81-j, "%s, %s", pdb_getEntry(db, entry, 2), *p?p:"(unknown)");
		uib_printstring(s, buf, j*5, 208, 81-j, ALIGN_LEFT, FONT_MEDIUM, c_black);

		j=snprintf(buf, 81, "Language:  ");
		uib_printstring(s, buf, 0, 216, 80, ALIGN_LEFT, FONT_MEDIUM, c_blue);
		snprintf(buf, 81-j, "%s", pdb_getEntry(db, entry, 14));
		uib_printstring(s, buf, j*5, 216, 80, ALIGN_LEFT, FONT_MEDIUM, c_black);

		j=snprintf(buf, 42, " Genre: ");
		uib_printstring(s, buf, 195, 216, 40, ALIGN_LEFT, FONT_MEDIUM, c_blue);
		p=pdb_getEntry(db, entry, 8);
		p1=pdb_getEntry(db, entry, 9);

		snprintf(buf, 42-j,	"%s%s%s", pdb_getEntry(db, entry, 9),(*p && *p1)?" - ":"", pdb_getEntry(db, entry, 8));
		uib_printstring(s, buf, 195+j*5, 216, 40, ALIGN_LEFT, FONT_MEDIUM, c_black);

		p=pdb_getEntry(db, entry, 18);
		p1=pdb_getEntry(db, entry, 19);
		uib_printstring(s, p, 0, 224, 80, ALIGN_LEFT, FONT_MEDIUM, c_black);
		uib_printstring(s, p1, 0, *p?232:224, 80, ALIGN_LEFT, FONT_MEDIUM, c_black);

		// check if all screenshots exist, request loading if not
		p=pdb_getEntry(db, entry, 6);
		char *pimg_fpath=util_concat(archdep_xdg_cache_home(), "/", p, NULL);
		char *base_fname=lib_stralloc(p);
		*(strrchr(base_fname,'.'))=0;
		char *base_fpath=util_concat(archdep_xdg_cache_home(), "/", base_fname, NULL);

		// check primary image
		if (access(pimg_fpath,R_OK) != 0) {
			snprintf(url, 256, "%s%s", GB64_SS_URL, p);
			async_http_get(url, pimg_fpath, uigb64_callback_imgLoad, 0);
		} else numimg=1;

		// check all other images (max 9)
		for(j=1; j<10; j++) {
			// check end file
			snprintf(buf2, 256, "%s_%d.x", base_fpath, j);
			if (access(buf2,F_OK) ==0) break;

			// check img file
			snprintf(buf2, 256, "%s_%d.png", base_fpath, j);
			if (access(buf2,R_OK) != 0) {
				snprintf(url, 256, "%s%s_%d.png", GB64_SS_URL, base_fname, j);
				async_http_get(url, buf2, uigb64_callback_imgLoad, (void*)j);
				break;
			} else numimg=j+1;
		}

		// paint image
		if (*screenshotidx > numimg-1) *screenshotidx = 0;
		if (*screenshotidx < 0 && numimg > 0) *screenshotidx = numimg-1;
		if (*screenshotidx == 0)
			snprintf(buf2, 256, "%s", pimg_fpath);
		else
			snprintf(buf2, 256, "%s_%d.png", base_fpath, *screenshotidx);
		SDL_Surface *i=IMG_Load(buf2);
		if (i) {
			SDL_BlitSurface ( i, NULL, s, &(SDL_Rect){.x=0, .y=0, .w=320, .h=200});
			SDL_FreeSurface(i);
		} else {
			SDL_BlitSurface ( loading_img, NULL, s, &(SDL_Rect){.x=0, .y=0, .w=320, .h=200});
		}

		// free allocated buffers
		free(pimg_fpath);
		free(base_fname);
		free(base_fpath);

		// draw right side info
		if (numimg>1) {
			uib_printstring(s, "Screenshots", 324, 4, 0, ALIGN_LEFT, FONT_MEDIUM, c_blue);
			uib_printstring(s, " ", 324, 12, 0, ALIGN_LEFT, FONT_SYM, c_blue);
			if (snprintf(buf, 8, "%d/%d", *screenshotidx+1, numimg) < 0)
				log_error(LOG_ERR,"screenshot index out of bounds");
			uib_printstring(s, buf, 336, 12, 0, ALIGN_LEFT, FONT_MEDIUM, c_black);
			uib_printstring(s, "!", 358, 12, 0, ALIGN_LEFT, FONT_SYM, c_blue);
		}
		if (downloaded[entry]) {
			uib_printstring(s, "DOWNLOADED", 324, 28, 0, ALIGN_LEFT, FONT_MEDIUM, c_green);
		}
		uib_printstring(s, "\"", 324, 44, 0, ALIGN_LEFT, FONT_SYM, c_blue);
		uib_printstring(s, downloaded[entry]?"Start":"Download", 336, 44, 0, ALIGN_LEFT, FONT_MEDIUM, c_black);
		if (downloaded[entry]) {
			uib_printstring(s, "$", 324, 52, 0, ALIGN_LEFT, FONT_SYM, c_blue);
			uib_printstring(s, "Delete", 336, 52, 0, ALIGN_LEFT, FONT_MEDIUM, c_black);
		}

		uib_printstring(s, "*+,", 324, 92, 0, ALIGN_LEFT, FONT_SYM, c_blue);
		uib_printstring(s, "Automode", 344, 92, 0, ALIGN_LEFT, FONT_MEDIUM, c_black);
		// 0=PAL, 1=PAL+NTSC, 2=NTSC, 3=PAL(+NTSC?)
		uib_printstring(s, "Mode:", 324, 100, 0, ALIGN_LEFT, FONT_MEDIUM, (gb64_set_modes&1)?c_blue:c_gray);
		uib_printstring(s, *(pdb_getEntry(db, entry, 20))=='2'?"NTSC":"PAL", 350, 100, 0, ALIGN_LEFT, FONT_MEDIUM, (gb64_set_modes&1)?c_black:c_gray);
		// TDE
		uib_printstring(s, "TDE:", 324, 108, 0, ALIGN_LEFT, FONT_MEDIUM, (gb64_set_modes&2)?c_blue:c_gray);
		uib_printstring(s, *(pdb_getEntry(db, entry, 21))=='0'?"No":"Yes", 350, 108, 0, ALIGN_LEFT, FONT_MEDIUM, (gb64_set_modes&2)?c_black:c_gray);
		// Control
		char *con = pdb_getEntry(db, entry, 24);
		if (con != NULL && con[0]!=0) {
			uib_printstring(s, "Ctrl:", 324, 116, 0, ALIGN_LEFT, FONT_MEDIUM, (gb64_set_modes&4)?c_blue:c_gray);
			switch(con[0]) {
				case '0': con="Joy Port2";	break;
				case '1': con="Joy Port1";	break;
				case '2': con="Keyboard";	break;
				case '3': con="Paddle 2";	break;
				case '4': con="Paddle 1";	break;
				case '5': con="Mouse";		break;
				case '6': con="Light Pen";	break;
				case '7': con="Koala Pad";	break;
				case '8': con="Light Gun";	break;
			}
			uib_printstring(s, con, 350, 116, 0, ALIGN_LEFT, FONT_MEDIUM, (gb64_set_modes&4)?c_black:c_gray);
		}
	}

	uib_printstring(s, "%", 324, 60, 0, ALIGN_LEFT, FONT_SYM, c_blue);
	uib_printstring(s, "List Filter", 336, 60, 0, ALIGN_LEFT, FONT_MEDIUM, c_black);
	uib_printstring(s, "Popular", 336, 68, 0, ALIGN_LEFT, FONT_MEDIUM, c_black);
	uib_printstring(s, "Downloaded", 336, 76, 0, ALIGN_LEFT, FONT_MEDIUM, c_black);
	if (list_filter & 1)
		uib_printstring(s, ">", 332, 68, 0, ALIGN_LEFT, FONT_MEDIUM, c_blue);
	if (list_filter & 2)
		uib_printstring(s, ">", 332, 76, 0, ALIGN_LEFT, FONT_MEDIUM, c_blue);

	uibottom_must_redraw |= UIB_RECALC_GB64;
}

#define SEARCH_MAX 31
extern char *strcasestr(const char *haystack, const char *needle);

static char *uigb64_start()
{
	int menu_max;
	int active = 1;
	int redraw = 1, cur_old = -1;
	int i, y, x, action, old_y=-1, dragging=0;
	char *title="Gamebase 64";
	char search[41];
	char oldsearch[41];
    int retval = -1;
	int key;
	SDLMod mod;
	int searchpos=0,searchsize,redraw_search=0;
	int screenshotidx=0;
	char *p;
	char buf[256];

	static int top_shows = -1;
	static int offset = 0, cur = 0, search_changed=1, total=0;

	acInit();
	ACU_GetWifiStatus(&wifi_status);
	acExit();
	if (wifi_status == 0 &&
		message_box("Gamebase64", "WiFi not enabled. You will only be able to browse local or cached content.", MESSAGE_YESNO) != 0)
		return NULL;

	menu_draw = sdl_ui_get_menu_param();

	// init the database
	if (db == NULL) {
		FILE *fd;
		// check if file exists
		char *dbname=util_concat(archdep_xdg_data_home(),"/gb64.db",NULL);
		if (access(dbname, R_OK)!=0) {
			if (message_box("Gamebase64", "Gamebase64 database not found. Download now?", MESSAGE_YESNO) != 0) {
				free(dbname);
				return NULL;
			}
dldb:
			// download (will be automatically unpacked by curl)
			sdl_ui_init_progress_bar("Downloading Gamebase64 database");
			if (downloadFile(GB64_DBGZ_URL, dbname, downloadProgress, MODE_FILE)) {
				ui_error("Could not download database: %s",http_errbuf);
				free(dbname);
				return NULL;
			}
tagdb:
			// write the file mtime into the file (because 3DS SD library does not suppot setting mtimes)
			fd=fopen(dbname,"r+");
			if (fd) {
				fseek(fd, 1, SEEK_SET);
				snprintf(buf, 256, "%ld ", http_last_req_info.mtime);
				fwrite(buf, 1, strlen(buf), fd);
				fclose(fd);
			}
		} else {
			// check if the file is up to date
			if ((i=downloadFile(GB64_DBGZ_URL, NULL, NULL, MODE_HEAD)) == 0 ) {
				FILE *fd=fopen(dbname,"r");
				if (fd) {
					fread(buf, 1, 20, fd);
					fclose(fd);
					if (http_last_req_info.mtime > strtol(buf+1, NULL, 0)) {
						if (message_box("Gamebase64", "Gamebase64 database update found. Download now?", MESSAGE_YESNO) == 0) {
							unlink(dbname);
							goto dldb;
						} else {
							if (message_box("Gamebase64", "Keep prompting for updates when opening gamebase64?", MESSAGE_YESNO) != 0) {
								goto tagdb;
							}
						}
					}
				}
			}
		}
		db = pdb_initDB(dbname);
		free(dbname);
		if (db == NULL) {
			ui_error("Cannot read gamebase64 database");
			return NULL;
		}
	}

	// init search result array
	if (searchresult == NULL)
		searchresult=malloc(db->num_entries * sizeof(int));
	if (searchresult == NULL) {
		ui_error("Cannot allocate searchresult");
		return NULL;
	}

	// init my top screen
	if (uigb64_top) SDL_FreeSurface(uigb64_top);
	uigb64_top = SDL_CreateRGBSurface(SDL_SWSURFACE,400,240,32,0x000000ff,0x0000ff00,0x00ff0000,0xff000000);
	gb64_top_must_redraw=1;

	// init file getter
	async_http_init(1);

	// some other init stuff
	loading_img=IMG_Load("romfs:/loading.png");
	gamedir = util_concat(archdep_xdg_data_home(), "/", "games", NULL);
	list_filter = persistence_getInt("gb64_list_filter",1);
    menu_max = menu_draw->max_text_y - GB64_FIRST_Y;
	memset(search,0,41);

	// migrate old download directory structure to new download structure
	DIR *dir;
    struct dirent *dent;
	char **dname;

	if (persistence_getInt("gb64_migrated",0) == 0) {
		dir=opendir(gamedir);
		if (dir) {
			int migcount=0;
			dname=NULL;
			while ((dent = readdir(dir)) != NULL) {
				if (dent->d_type == DT_DIR && strchr(dent->d_name, '_') != NULL) {
					if ((migcount & 0xff) == 0)
						dname=realloc(dname, migcount+256);
					dname[migcount++]=lib_stralloc(dent->d_name);
				}
			}
			closedir(dir);
			
			for (i=0; i<migcount; i++) {
				x = atoi(dname[i]);
				char *ndir = gb64_gamedir(x);
				snprintf(buf,256,"%s/%s",gamedir,dname[i]);
				mkpath(ndir, 0);
				rename(buf, ndir);
				free(ndir);
				free(dname[i]);
			}
			if (dname) free (dname);
		}
		persistence_putInt("gb64_migrated",1);
	}

	// check what is downloaded
	DIR *dir2;
    struct dirent *dent2;
	downloaded = malloc(db->num_entries);
	memset(downloaded, 0, db->num_entries);
	dir=opendir(gamedir);
	if (dir) {
		// get a list of all game directories
		int nr_downloaded=0;
		dname=NULL;
		while((dent = readdir(dir)) != NULL) {
			if (dent->d_type == DT_DIR) {
				char *sdir_name=util_concat(gamedir,"/",dent->d_name,NULL);
				dir2=opendir(sdir_name);
				if (dir2) {
					while((dent2 = readdir(dir2)) != NULL) {
						if ((nr_downloaded & 0xff) == 0)
							dname=realloc(dname, nr_downloaded+256);
						dname[nr_downloaded++]=util_concat(dent->d_name, "/", dent2->d_name, NULL);
					}
				}
				closedir(dir2);
				free(sdir_name);
			}
		}
		closedir(dir);

		// check all games if they are already downloaded
		if (nr_downloaded) {
			for (i=0; i < db->num_entries; ++i) {
				p=pdb_getEntry(db, i, 3);
				y=strlen(p)-4;
				for (x=0; x < nr_downloaded; ++x) {
					if (dname[x] != NULL && strncmp(dname[x], p, y) == 0) {
						// check if start image exists in the directory
						snprintf(buf, 256, "%s/%s/%s", gamedir, dname[x], pdb_getEntry(db, i, 4));
						if (access(buf,R_OK)==0)
							downloaded[i]=1;
						free (dname[x]);
						dname[x]=NULL;
						break;
					}
				}
			}
			for (x=0; x < nr_downloaded; ++x)
				if (dname[x]) free(dname[x]);
			free(dname);
		}					
	}

	// main gb64 interface loop
	while (active) {
		searchsize=strlen(search);
		if (search_changed) {
			total=0;
			int curset=0;
			cur = offset = 0;
			cur_old=-1;
			for (i=0; i < db->num_entries; ++i) {
				if (!((list_filter & 1) && *(pdb_getEntry(db, i, 16)) == '0') &&
					!((list_filter & 2) && downloaded[i] != 1) &&
					(searchsize == 0 || (
					(p=pdb_getEntry(db, i, NAMECOL)) !=NULL &&
					strcasestr(p, search)))) {
					searchresult[total++]=i;
				}
				if (!curset && i >= top_shows) {
					curset=1;
					if (total) cur=total-1;
				}
			}
			sdl_ui_adjust_offset(&offset, &cur,menu_max,total);
			search_changed=0;
			redraw=1;
			strcpy(oldsearch,search);
		}

        if (redraw || redraw_search) {
            uigb64_redraw(searchresult, title, search, searchpos, offset,
                (total - offset > menu_max) ? menu_max : total - offset,
                (total - offset > menu_max) ? 1 : 0, cur, total);
            redraw = redraw_search = 0;
        } else {
            uigb64_redraw_cursor(searchresult, offset,
                    (total - offset > menu_max) ? menu_max : total - offset,
                    cur, cur_old);
        }
        sdl_ui_refresh();

		if (top_shows != searchresult[offset+cur] || gb64_top_must_redraw) {
			if (top_shows != searchresult[offset+cur]) screenshotidx=0;
			gb64_top_must_redraw=0;
			top_shows = total==0 ? -1 : searchresult[offset+cur];
			uigb64_update_topscreen(uigb64_top, top_shows, &screenshotidx);
		}

		action=uigb64_poll_input(&key, &mod);
		while (action != -1) {
			i=action; action = -1;
			switch (i) {
			case MENU_ACTION_HOME:
				cur_old = cur;
				cur = -offset;
				redraw |= sdl_ui_adjust_offset(&offset, &cur,menu_max,total);
				break;
			case MENU_ACTION_END:
				cur_old = cur;
				cur = total-offset-1;
				redraw |= sdl_ui_adjust_offset(&offset, &cur,menu_max,total);
				break;
			case MENU_ACTION_UP:
				cur_old=cur;
				--cur;
				redraw |= sdl_ui_adjust_offset(&offset, &cur,menu_max,total);
				break;
			case MENU_ACTION_PAGEUP:
			case MENU_ACTION_LEFT:
				cur_old = cur;
				cur -= menu_max;
				if (cur_old + offset != 0 && cur + offset < 0) cur=-offset;
				redraw |= sdl_ui_adjust_offset(&offset, &cur,menu_max,total);
				break;
			case MENU_ACTION_DOWN:
				cur_old=cur;
				++cur;
				redraw |= sdl_ui_adjust_offset(&offset, &cur,menu_max,total);
				break;
			case MENU_ACTION_PAGEDOWN:
			case MENU_ACTION_RIGHT:
				cur_old = cur;
				cur += menu_max;
				if (cur_old + offset != total-1 && cur + offset > total-1) cur=total-offset-1;
				redraw |= sdl_ui_adjust_offset(&offset, &cur,menu_max,total);
				break;
			case MENU_ACTION_SELECT:
				if (top_shows<0) break;
				if (!downloaded[top_shows]) {
					snprintf(buf,256,"Download \"%s\"?",pdb_getEntry(db, top_shows, NAMECOL));
					if (message_box("Download Game", buf, MESSAGE_YESNO) == 0) {
						gb64_download(top_shows);
						gb64_top_must_redraw = 1;
					}
					redraw = 1;
				} else {
					active = 0;
					retval = top_shows;
				}
				break;
			case MENU_ACTION_CANCEL:
			case MENU_ACTION_EXIT:
				active = 0;
				break;
			case MENU_ACTION_MOUSE:
				y = ((lastevent.button.y*240) / sdl_active_canvas->screen->h) / activefont.h;
				x = ((lastevent.button.x*320) / sdl_active_canvas->screen->w) / activefont.w;
				if (lastevent.type != SDL_MOUSEBUTTONUP) {
					i=offset;
					if (old_y==-1) {
						if (y >= GB64_FIRST_Y &&
							y < menu_max + GB64_FIRST_Y &&
							y <= MENU_FIRST_Y + total - offset) {
							if (havescrollbar && (dragging || x==menu_draw->max_text_x-1)) {
								offset = sdl_calc_scrollbar_pos(
									(lastevent.button.y*240) / sdl_active_canvas->screen->h,
									GB64_FIRST_Y, menu_max, total);
								dragging=1;
							} else {
								old_y=y;
							}
						}
					} else {
						if (y!=old_y) {
							dragging=1;
							offset-=y-old_y;
							old_y=y;
						}
					}
					if (i != offset) {
						if (offset > total - menu_max)
							offset=total - menu_max;
						if (offset<0) offset=0;
						if (i!=offset) {
							cur-=offset-i;
							cur_old=-1;
							redraw=1;
						}
					}
				} else {
					if (!dragging &&
						y >= GB64_FIRST_Y &&
						y < menu_max + GB64_FIRST_Y &&
						y <= MENU_FIRST_Y + total - offset) {
						cur_old=cur;
						cur = y - GB64_FIRST_Y;
					}
					dragging=0;
					old_y=-1;
				}
				break;
			default:
				if (key) {
					switch (key) {
					case 204:	// L-button
						screenshotidx--;
						gb64_top_must_redraw = 1;
						break;
					case 205:	// R-button
						screenshotidx++;
						gb64_top_must_redraw = 1;
						break;
					case 203:	// Y-button
						list_filter++;
						if (list_filter > 2) list_filter=0;
						persistence_putInt("gb64_list_filter",list_filter);
						search_changed = gb64_top_must_redraw = 1;
						break;
					case 202:	// X-button
						if (top_shows >= 0 && downloaded[top_shows]) {
							snprintf(buf,256,"Really delete game \"%s\"?",pdb_getEntry(db, top_shows, NAMECOL));
							if (message_box("Delete Game", buf, MESSAGE_YESNO) == 0) {
								gb64_delete(top_shows);
								search_changed = 1;
							}
							redraw = 1;
						}
						break;
					case 208:	// START-button
						gb64_set_modes = (gb64_set_modes + 1) % 8;
						gb64_top_must_redraw = 1;
						break;
					case VICE_SDLK_LEFT:
					case 31:
						if (searchpos > 0) {
							--searchpos;
						}
						break;
					case VICE_SDLK_RIGHT:
					case 29:
						if (searchpos < searchsize) {
							++searchpos;
						}
						break;
					case VICE_SDLK_HOME:
					case 30:	// C_UP
						searchpos = 0;
						break;
					case VICE_SDLK_END:
					case 17:	// C_DOWN
						searchpos = searchsize;
						break;
					case VICE_SDLK_BACKSPACE:
						if (searchpos > 0) {
							memmove(search + searchpos - 1, search + searchpos, searchsize - searchpos + 1);
							--searchpos;
						}
						break;
					case 19: // CLR/HOME
						memset(search,0,41);
						searchpos = 0;
						break;
					default:
						if (searchsize < SEARCH_MAX && key<128 && key>31) {
							memmove(search + searchpos + 1, search + searchpos, searchsize - searchpos);
							search[searchpos++] = (char)key;
						}
						break;
					}
					if (strcmp(oldsearch,search)) search_changed = 1;
					redraw_search=1;
				}
				break;
			}
        }
    }

	char *c=NULL;
	if (retval >= 0) {
		// get the autostart image location
		p=gb64_gamedir(top_shows);
		persistence_put("cwd",p);
		c=util_concat(p, "/", pdb_getEntry(db, top_shows, 4), NULL);
		free (p);

		// set video mode
		if (gb64_set_modes & 1)
			c64model_set(*(pdb_getEntry(db, top_shows, 20))=='2' ? C64MODEL_C64C_NTSC : C64MODEL_C64C_PAL);
		// set TDE
		if (gb64_set_modes & 2) 
			resources_set_int("DriveTrueEmulation", *(pdb_getEntry(db, top_shows, 21))=='0' ? 0 : 1);
		// set controller
		if (gb64_set_modes & 4) {
			p = pdb_getEntry(db, top_shows, 24);
			if (p != NULL && p[0]!=0) {
				switch (p[0]) {
					case '0': // Joy Port2
						custom_joyport_toggle_callback(1, (void*)2);
						break;
					case '1': // Joy Port1
						custom_joyport_toggle_callback(1, (void*)1);
						break;
					case '2': // Keyboard
						if (is_keyboard_hidden()) toggle_keyboard();
						break;
					case '3': // Paddle 2
						custom_joyport_toggle_callback(1, (void*)1);
						resources_set_int("JoyPort2Device", JOYPORT_ID_PADDLES);
						set_mouse_enabled(1,NULL);
						break;
					case '4': // Paddle 1
						custom_joyport_toggle_callback(1, (void*)2);
						resources_set_int("JoyPort1Device", JOYPORT_ID_PADDLES);
						set_mouse_enabled(1,NULL);
						break;
					case '5': // Mouse
						custom_joyport_toggle_callback(1, (void*)2);
						resources_set_int("JoyPort1Device", JOYPORT_ID_MOUSE_1351);
						set_mouse_enabled(1,NULL);
						break;
					case '6': // Light Pen
						custom_joyport_toggle_callback(1, (void*)2);
						resources_set_int("JoyPort1Device", JOYPORT_ID_LIGHTPEN_U);
						set_mouse_enabled(1,NULL);
						break;
					case '7': // Koala Pad
						custom_joyport_toggle_callback(1, (void*)2);
						resources_set_int("JoyPort1Device", JOYPORT_ID_KOALAPAD);
						set_mouse_enabled(1,NULL);
						break;
					case '8': // Light Gun
						custom_joyport_toggle_callback(1, (void*)2);
						resources_set_int("JoyPort1Device", JOYPORT_ID_LIGHTGUN_Y);
						set_mouse_enabled(1,NULL);
						break;
					default:
						break;
				}
			}
		}
	}

	// clean up
	SDL_FreeSurface(uigb64_top);
	uigb64_top=NULL;
	uibottom_must_redraw |= UIB_RECALC_GB64;
	SDL_FreeSurface(loading_img);
	async_http_shutdown();
	free (gamedir);
	free (downloaded);

	// return NULL or autostart image name
	if (retval >= 0) return c;
	return NULL;
}

UI_MENU_CALLBACK(gb64_callback)
{
    if (activated) {
		ui_pause_emulation(1);
		char *name=uigb64_start();
		ui_pause_emulation(0);
		if (name != NULL) {
			if (autostart_autodetect(name, NULL, 0, AUTOSTART_MODE_RUN) < 0) {
				ui_error("could not start auto-image");
			}
			lib_free(name);
			return sdl_menu_text_exit_ui;
		}
	}
    return NULL;
}

void gb64_shutdown() {
	if (db != NULL) pdb_freeDB(db);
	if (searchresult != NULL) free(searchresult);
	db=NULL;
}