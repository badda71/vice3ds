/*
 * fliplist.c
 *
 * Written by
 *  pottendo <pottendo@gmx.net>
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

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>

#include "archdep.h"
#include "attach.h"
//#include "cmdline.h"
#include "diskimage.h"
#include "drive.h"
#include "fliplist.h"
#include "fsimage.h"
#include "ioutil.h"
#include "lib.h"
#include "log.h"
#include "persistence.h"
#include "rawimage.h"
#include "resources.h"
#include "util.h"
#include "uibottom.h"

#define NUM_DRIVES 4

struct fliplist_s {
    fliplist_t next, prev;
    char *image;
    unsigned int unit;
};

static fliplist_t fliplist[NUM_DRIVES] = {
    (fliplist_t)NULL,
    (fliplist_t)NULL
};

static char *current_image = (char *)NULL;
static unsigned int current_drive;
static fliplist_t iterator;

static const char flip_file_header[] = "# Vice fliplist file";

#define buffer_size 1024

static void show_fliplist(unsigned int unit);

static char *fliplist_file_name = NULL;


static int set_fliplist_file_name(const char *val, void *param)
{
    if (util_string_set(&fliplist_file_name, val)) {
        return 0;
    }

    fliplist_load_list(FLIPLIST_ALL_UNITS, fliplist_file_name, 0);

    return 0;
}

static resource_string_t resources_string[] = {
    { "FliplistName", NULL, RES_EVENT_NO, NULL,
      &fliplist_file_name, set_fliplist_file_name, NULL },
    RESOURCE_STRING_LIST_END
};

int fliplist_resources_init(void)
{
    resources_string[0].factory_value = archdep_default_fliplist_file_name();

    if (resources_register_string(resources_string) < 0) {
        return -1;
    }

    return 0;
}

void fliplist_resources_shutdown(void)
{
    int i;

    for (i = 0; i < NUM_DRIVES; i++) {
        fliplist_clear_list(8 + i);
    }

    lib_free(fliplist_file_name);
    lib_free((char *)(resources_string[0].factory_value));
}
/*
static const cmdline_option_t cmdline_options[] =
{
    { "-flipname", SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
      NULL, NULL, "FliplistName", NULL,
      "<Name>", "Specify name of the flip list file image" },
    CMDLINE_LIST_END
};

int fliplist_cmdline_options_init(void)
{
    return cmdline_register_options(cmdline_options);
}
*/
/* ------------------------------------------------------------------------- */
/* interface functions */

void fliplist_shutdown(void)
{
    lib_free(current_image);
    current_image = NULL;
}

void fliplist_set_current(unsigned int unit, const char *filename)
{
    lib_free(current_image);
    current_image = lib_stralloc(filename);
    current_drive = unit;
}

#if 0
char *fliplist_get_head(unsigned int unit)
{
    if (fliplist[unit - 8]) {
        return fliplist[unit - 8]->image;
    }
    return (char *) NULL;
}
#endif

const char *fliplist_get_next(unsigned int unit)
{
    if (fliplist[unit - 8]) {
        return fliplist[unit - 8]->next->image;
    }
    return (const char *) NULL;
}

const char *fliplist_get_prev(unsigned int unit)
{
    if (fliplist[unit - 8]) {
        return fliplist[unit - 8]->prev->image;
    }
    return (const char *) NULL;
}

const char *fliplist_get_image(fliplist_t fl)
{
    return fl->image;
}

unsigned int fliplist_get_unit(fliplist_t fl)
{
    return fl->unit;
}

void fliplist_add_image(unsigned int unit)
{
    fliplist_t n;

    if (current_image == NULL) {
        return;
    }
    if (strcmp(current_image, "") == 0) {
        return;
    }

    n = lib_malloc(sizeof(struct fliplist_s));
    n->image = lib_stralloc(current_image);
    unit = n->unit = current_drive;

    log_message(LOG_DEFAULT, "Adding `%s' to fliplist[%d]", n->image, unit);
    if (fliplist[unit - 8]) {
        n->next = fliplist[unit - 8];
        n->prev = fliplist[unit - 8]->prev;
        n->next->prev = n;
        n->prev->next = n;
        fliplist[unit - 8] = n;
    } else {
        fliplist[unit - 8] = n;
        n->next = n;
        n->prev = n;
    }
    show_fliplist(unit);
}

void fliplist_remove(unsigned int unit, const char *image)
{
    fliplist_t tmp;

    if (fliplist[unit - 8] == NULL) {
        return;
    }
    if (image == (char *) NULL) {
        /* no image given, so remove the head */
        if ((fliplist[unit - 8] == fliplist[unit - 8]->next) &&
            (fliplist[unit - 8] == fliplist[unit - 8]->prev)) {
            /* this is the last entry */
            tmp = fliplist[unit - 8];
            fliplist[unit - 8] = (fliplist_t) NULL;
        } else {
            fliplist[unit - 8]->next->prev = fliplist[unit - 8]->prev;
            fliplist[unit - 8]->prev->next = fliplist[unit - 8]->next;
            tmp = fliplist[unit - 8];
            fliplist[unit - 8] = fliplist[unit - 8]->next;
        }
        log_message(LOG_DEFAULT, "Removing `%s' from fliplist[%d]",
                    tmp->image, unit);
        lib_free(tmp->image);
        lib_free(tmp);
        show_fliplist(unit);
        return;
    } else {
        /* do a lookup and remove it */
        fliplist_t it = fliplist[unit - 8];

        if (strcmp(it->image, image) == 0) {
            /* it's the head */
            fliplist_remove(unit, NULL);
            return;
        }
        it = it->next;
        while ((strcmp(it->image, image) != 0) &&
               (it != fliplist[unit - 8])) {
            it = it->next;
        }

        if (it == fliplist[unit - 8]) {
            log_message(LOG_DEFAULT,
                        "Cannot remove `%s'; not found in fliplist[%d]",
                        it->image, unit);
            return;
        }

        it->next->prev = it->prev;
        it->prev->next = it->next;
        lib_free(it->image);
        lib_free(it);
        show_fliplist(unit);
    }
}

static char *fliplist_get_filenameext_for_drivetype(int unit) {
	int type = drive_context[unit-8]->drive->type;
	char *ret="\0\0";
	switch (type) {
		case DRIVE_TYPE_1540:
		case DRIVE_TYPE_1541:
		case DRIVE_TYPE_1541II:
		case DRIVE_TYPE_1551:
		case DRIVE_TYPE_1570:
		case DRIVE_TYPE_2031:
		case DRIVE_TYPE_2040:
		case DRIVE_TYPE_3040:
		case DRIVE_TYPE_4040:
			ret="D64\0G64\0P64\0X64\0D67\0\0";
			break;
		case DRIVE_TYPE_1571:
		case DRIVE_TYPE_1571CR:
			ret="D64\0G64\0P64\0X64\0D67\0G71\0D71\0\0";
			break;
		case DRIVE_TYPE_1581:
			ret="D81\0\0";
			break;
		case DRIVE_TYPE_2000:
		case DRIVE_TYPE_4000:
			ret="D81\0D1M\0D2M\0D4M\0\0";
			break;
		case DRIVE_TYPE_1001:
		case DRIVE_TYPE_8050:
		case DRIVE_TYPE_8250:
			ret="D80\0D82\0\0";
			break;
	}
	return ret;
}

static int mycmp(const void *a, const void *b) {
	return strcasecmp(*((char**)a),*((char**)b));
}

void fliplist_attach_head (unsigned int unit, int direction)
{
	int i, numfiles=0, nameidx=-1, newnameidx=-1,x;
	char **fnames=NULL;
	DIR *d=NULL;
	struct dirent *entry;
	char *ex, *ext, *dir=NULL,*name=NULL, *newname=NULL;
	
	if (fliplist[unit - 8] == NULL) {
//log_citra("%s, no fliplist attached",__func__);
		// no fliplist attached, use directory of image or current directory as fliplist
		ext=fliplist_get_filenameext_for_drivetype(unit);
		if (*ext==0) {
			uib_show_message(3000, "Drive %d is not active", unit);
			goto thisexit;
		}
		// get directory of currently attached image or current directory
		if (drive_context[unit-8]->drive->image != NULL) {
			name = drive_context[unit-8]->drive->image->device == DISK_IMAGE_DEVICE_FS ?
				drive_context[unit-8]->drive->image->media.fsimage->name :
				drive_context[unit-8]->drive->image->media.rawimage->name;
			dir = lib_stralloc(name);
			if ((name=strrchr(dir,'/'))!=NULL)
				*name++ = 0;
		} else {
			dir = lib_stralloc(persistence_get("cwd","/"));
			if (*dir && dir[strlen(dir)-1]=='/') dir[strlen(dir)-1]=0;
		}
		// get all files with within this directory with the given extension
//log_citra("scanning dir %s",dir);
		if ((d = opendir(dir)) == NULL) {
			uib_show_message(3000, "Could not open directory %s", dir);
			goto thisexit;
		}
		while ((entry = readdir(d)) != NULL) {
			if (entry->d_type == DT_DIR || (ex=strrchr(entry->d_name,'.'))==NULL) continue;
			for (i=0; ext[i] != 0; i+=4) {
				if (!strcasecmp(ext+i,ex+1)) break;
			}
			if (ext[i]==0) continue;
			if ((numfiles & 0xff) == 0)
				fnames = realloc(fnames, (numfiles + 256) * sizeof(char*));
			fnames[numfiles++] = lib_stralloc(entry->d_name);
		}
		if (numfiles<1) {
			uib_show_message(3000, "No valid images in %s", dir);
			goto thisexit;
		}
		qsort(fnames, numfiles, sizeof(char*), mycmp);
		// find the image to attach
		if (!name) newnameidx=0;
		else {
			// find currently attached image
			for (i=0; i<numfiles; i++) {
//log_citra("cmp: %s %s", name, fnames[i]);
				if (!strcmp(name, fnames[i])) {
//log_citra("match!");
					nameidx=i;
					break;
				}
			}
			if (nameidx==-1) newnameidx=0;
			else if (direction)
				newnameidx = (nameidx+1) % numfiles;
			else
				newnameidx = (nameidx-1+numfiles)%numfiles;
		}

		newname = util_concat(dir, "/", fnames[newnameidx], NULL);
		if (newnameidx == nameidx) {
			uib_show_message(3000, "Drive %d: %s", unit, newname);
			goto thisexit;
		}

		i=newnameidx;
		while ((x=file_system_attach_disk(unit, newname))) {
			i = (i+(direction?1:-1)+numfiles) % numfiles;
			if (i == newnameidx) break;
			free(newname);
			newname = util_concat(dir, "/", fnames[i], NULL);
		}
		if (x) {
			uib_show_message(3000, "Could not attach %s", newname);
		} else {
			uib_show_message(3000, "Drive %d: %s", unit, newname);
		}
    } else {
		if (direction) {
			fliplist[unit - 8] = fliplist[unit - 8]->next;
		} else {
			fliplist[unit - 8] = fliplist[unit - 8]->prev;
		}
		if (file_system_attach_disk(fliplist[unit - 8]->unit, fliplist[unit - 8]->image) < 0) {
			uib_show_message(3000, "Could not attach %s", fliplist[unit - 8]->image);
		} else {
			uib_show_message(3000, "Drive %d: %s", fliplist[unit - 8]->unit, fliplist[unit - 8]->image);
		}

	}

thisexit:
		if (newname) lib_free(newname);
		if (fnames) {
			for (i=0; i<numfiles; i++)
				lib_free(fnames[i]);
			free(fnames);
		}
		if (d) closedir(d);
		if (dir) lib_free(dir);
		return;
}

fliplist_t fliplist_init_iterate(unsigned int unit)
{
    fliplist_t ret = NULL;

    iterator = fliplist[unit - 8];
    if (iterator) {
        ret = iterator;
        iterator = iterator->next;
    }
    return ret;
}

fliplist_t fliplist_next_iterate(unsigned int unit)
{
    fliplist_t ret = NULL;

    if (iterator) {
        if (iterator != fliplist[unit - 8]) {
            ret = iterator;
            iterator = iterator->next;
        }
    }
    return ret;
}

void fliplist_clear_list(unsigned int unit)
{
    fliplist_t flip = fliplist[unit - 8];

    if (flip != NULL) {
        do {
            fliplist_t tmp = flip->next;

            lib_free(flip->image);
            lib_free(flip);
            flip = tmp;
        }
        while (flip != fliplist[unit - 8]);

        fliplist[unit - 8] = NULL;
    }
}

int fliplist_save_list(unsigned int unit, const char *filename)
{
    int all_units = 0;
    fliplist_t flip;
    FILE *fp = NULL;
    char *savedir;

    /* create the directory where the fliplist should be written first */
    util_fname_split(filename, &savedir, NULL);
    ioutil_mkdir(savedir, IOUTIL_MKDIR_RWXU);
    lib_free(savedir);

    if (unit == FLIPLIST_ALL_UNITS) {
        all_units = 1;
        unit = 8;
    }

    do {
        flip = fliplist[unit - 8];

        if (flip != NULL) {
            if (!fp) {
                if ((fp = fopen(filename, MODE_WRITE)) == NULL) {
                    return -1;
                }
                fprintf(fp, "%s\n", flip_file_header);
            }

            fprintf(fp, "\nUNIT %d", unit);
            do {
                fprintf(fp, "\n%s", flip->image);
                flip = flip->next;
            }
            while (flip != fliplist[unit - 8]);
        }
        unit++;
    } while (all_units && ((unit - 8) < NUM_DRIVES));

    if (fp) {
        fclose(fp);
    }
    return 0;
}

int fliplist_load_list(unsigned int unit, const char *filename, int autoattach)
{
    FILE *fp;
    char buffer[buffer_size];
    int all_units = 0, i;
    int listok = 0;

    if (filename == NULL || *filename == 0 || (fp = fopen(filename, MODE_READ)) == NULL) {
        return -1;
    }

    buffer[0] = '\0';
    if (fgets(buffer, buffer_size, fp) == NULL) {
        fclose(fp);
        return -1;
    }

    if (strncmp(buffer, flip_file_header, strlen(flip_file_header)) != 0) {
        log_message(LOG_DEFAULT, "File %s is not a fliplist file", filename);
        fclose(fp);
        return -1;
    }
    if (unit == FLIPLIST_ALL_UNITS) {
        all_units = 1;
        for (i = 0; i < NUM_DRIVES; i++) {
            fliplist_clear_list(i + 8);
        }
    } else {
        fliplist_clear_list(unit);
    }

    while (!feof(fp)) {
        char *b;

        buffer[0] = '\0';
        if (fgets(buffer, buffer_size, fp) == NULL) {
            break;
        }

        if (strncmp("UNIT ", buffer, 5) == 0) {
            if (all_units != 0) {
                long unit_long;

                util_string_to_long(buffer + 5, NULL, 10, &unit_long);

                if (unit_long < 8 || unit_long > 11) {
                    log_message(LOG_DEFAULT,
                            "Invalid unit number %ld for fliplist\n", unit_long);
                    /* perhaps VICE should properly error out, ie quit? */
                    return -1;
                }

                unit = (unsigned int)unit_long;
            }
            continue;
        }

        /* remove trailing whitespace (linefeeds etc) */
        b = buffer + strlen(buffer);
        while ((b > buffer) && (isspace((unsigned int)(b[-1])))) {
            b--;
        }

        if (b > buffer) {
            fliplist_t tmp;

            *b = '\0';

            if (unit == FLIPLIST_ALL_UNITS) {
                log_message(LOG_DEFAULT, "Fliplist has inconsistent view for unit, assuming 8.\n");
                unit = 8;
            }

            tmp = lib_malloc(sizeof(struct fliplist_s));
            tmp->image = lib_stralloc(buffer);
            tmp->unit = unit;

            if (fliplist[unit - 8] == NULL) {
                fliplist[unit - 8] = tmp;
                tmp->prev = tmp;
                tmp->next = tmp;
            } else {
                tmp->next = fliplist[unit - 8];
                tmp->prev = fliplist[unit - 8]->prev;
                tmp->next->prev = tmp;
                tmp->prev->next = tmp;
                fliplist[unit - 8] = tmp;
            }
            listok = 1;
        }
    }

    fclose(fp);

    if (listok) {
        current_drive = unit;

        if (all_units) {
            for (i = 0; i < NUM_DRIVES; i++) {
                show_fliplist(i + 8);
            }
        } else {
            show_fliplist(unit);
        }

        if (autoattach) {
            fliplist_attach_head(unit, 1);
        }

        return 0;
    }

    return -1;
}

/* ------------------------------------------------------------------------- */

static void show_fliplist(unsigned int unit)
{
    fliplist_t it = fliplist[unit - 8];

    log_message(LOG_DEFAULT, "Fliplist[%d] contains:", unit);

    if (it) {
        do {
            log_message(LOG_DEFAULT,
                        "\tUnit %d %s (n: %s, p:%s)", it->unit, it->image,
                        it->next->image, it->prev->image);
            it = it->next;
        } while (it != fliplist[unit - 8]);
    } else {
        log_message(LOG_DEFAULT, "\tnothing");
    }
}
