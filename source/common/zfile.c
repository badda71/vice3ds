/*
 * zfile.c - Transparent handling of compressed files.
 *
 * Written by
 *  Ettore Perazzoli <ettore@comm2000.it>
 *  Andreas Boose <viceteam@t-online.de>
 *
 * ARCHIVE, ZIPCODE and LYNX supports added by
 *  Teemu Rantanen <tvr@cs.hut.fi>
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

/* This code might be improved a lot...  */

#include "vice.h"

#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef HAVE_ERRNO_H
#include <errno.h>
#endif
#ifdef HAVE_ZLIB
#include <zlib.h>
#endif

#ifdef HAVE_STRINGS_H
#include <strings.h>
#endif

#include <zip.h>

#include "archdep.h"
#include "archdep_cp.h"
#include "archdep_xdg.h"
#include "ioutil.h"
#include "lib.h"
#include "log.h"
#include "util.h"
#include "zfile.h"


/* ------------------------------------------------------------------------- */

//#define DEBUG_ZFILE

#ifdef DEBUG_ZFILE
#define ZDEBUG(a)  log_debug a
#else
#define ZDEBUG(a)
#endif

/* We could add more here...  */
enum compression_type {
    COMPR_NONE,
    COMPR_GZIP,
    COMPR_ZIP
};

/* This defines a linked list of all the compressed files that have been
   opened.  */
struct zfile_s {
    char *tmp_name;              /* Name of the temporary file.  */
    char *orig_name;             /* Name of the original file.  */
    int write_mode;              /* Non-zero if the file is open for writing.*/
    FILE *stream;                /* Associated stdio-style stream.  */
    FILE *fd;                    /* Associated file descriptor.  */
    enum compression_type type;  /* Compression algorithm.  */
    struct zfile_s *prev, *next; /* Link to the previous and next nodes.  */
    zfile_action_t action;       /* action on close */
    char *request_string;        /* ui string for action=ZFILE_REQUEST */
};
typedef struct zfile_s zfile_t;

static zfile_t *zfile_list = NULL;

static log_t zlog = LOG_ERR;

/* ------------------------------------------------------------------------- */

static int zinit_done = 0;


/** \@brief 'Check' is file \a name is a gzip or compress file
 *
 * \param[in]   name    filename or path
 *
 * \return  bool
 *
 * \fixme   this is a silly function and should be reimplemented using the
 *          2-byte header of the file
 */
static int file_is_gzip(const char *name)
{
    size_t l = strlen(name);

    if ((l < 4 || strcasecmp(name + l - 3, ".gz"))
        && (l < 3 || strcasecmp(name + l - 2, ".z"))
        && (l < 4 || toupper(name[l - 1]) != 'Z' || name[l - 4] != '.')) {
          return 0;
    }
    return 1;
}


static void zfile_list_destroy(void)
{
    zfile_t *p;

    for (p = zfile_list; p != NULL; ) {
        zfile_t *next;

        lib_free(p->orig_name);
        lib_free(p->tmp_name);
        next = p->next;
        lib_free(p);
        p = next;
    }

    zfile_list = NULL;
}

static int zinit(void)
{
    zlog = log_open("ZFile");

    /* Free the `zfile_list' if not empty.  */
    zfile_list_destroy();

    zinit_done = 1;

    return 0;
}

/* Add one zfile to the list.  `orig_name' is automatically expanded to the
   complete path.  */
static void zfile_list_add(const char *tmp_name,
                           const char *orig_name,
                           enum compression_type type,
                           int write_mode,
                           FILE *stream, FILE *fd)
{
    zfile_t *new_zfile = lib_malloc(sizeof(zfile_t));

    /* Make sure we have the complete path of the file.  */
    archdep_expand_path(&new_zfile->orig_name, orig_name);

    /* The new zfile becomes first on the list.  */
    new_zfile->tmp_name = tmp_name ? lib_stralloc(tmp_name) : NULL;
    new_zfile->write_mode = write_mode;
    new_zfile->stream = stream;
    new_zfile->fd = fd;
    new_zfile->type = type;
    new_zfile->action = ZFILE_KEEP;
    new_zfile->request_string = NULL;
    new_zfile->next = zfile_list;
    new_zfile->prev = NULL;
    if (zfile_list != NULL) {
        zfile_list->prev = new_zfile;
    }
    zfile_list = new_zfile;
}

void zfile_shutdown(void)
{
    zfile_list_destroy();
}

/* ------------------------------------------------------------------------ */

/* Uncompression.  */

/* If `name' has a gzip-like extension, try to uncompress it into a temporary
   file using gzip or zlib if available.  If this succeeds, return the name
   of the temporary file; return NULL otherwise.  */
static char *try_uncompress_with_gzip(const char *name)
{
    FILE *fddest;
    gzFile fdsrc;
    char *tmp_name = NULL;
    int len;

    if (!file_is_gzip(name)) {
        return NULL;
    }

    fddest = archdep_mkstemp_fd(&tmp_name, MODE_WRITE);

    if (fddest == NULL) {
        return NULL;
    }

    fdsrc = gzopen(name, MODE_READ);
    if (fdsrc == NULL) {
        fclose(fddest);
        ioutil_remove(tmp_name);
        lib_free(tmp_name);
        return NULL;
    }

    do {
        char buf[256];

        len = gzread(fdsrc, (void *)buf, 256);
        if (len > 0) {
            if (fwrite((void *)buf, 1, (size_t)len, fddest) < len) {
                gzclose(fdsrc);
                fclose(fddest);
                ioutil_remove(tmp_name);
                lib_free(tmp_name);
                return NULL;
            }
        }
    } while (len > 0);

    gzclose(fdsrc);
    fclose(fddest);

    return tmp_name;
}

/* Extensions we know about */
static const char *extensions[] = {
    FSDEV_EXT_SEP_STR "d64",
    FSDEV_EXT_SEP_STR "d67",
    FSDEV_EXT_SEP_STR "d71",
    FSDEV_EXT_SEP_STR "d80",
    FSDEV_EXT_SEP_STR "d81",
    FSDEV_EXT_SEP_STR "d82",
    FSDEV_EXT_SEP_STR "d1m",
    FSDEV_EXT_SEP_STR "d2m",
    FSDEV_EXT_SEP_STR "d4m",
    FSDEV_EXT_SEP_STR "g64",
    FSDEV_EXT_SEP_STR "p64",
    FSDEV_EXT_SEP_STR "g41",
    FSDEV_EXT_SEP_STR "x64",
    FSDEV_EXT_SEP_STR "dsk",
    FSDEV_EXT_SEP_STR "t64",
    FSDEV_EXT_SEP_STR "p00",
    FSDEV_EXT_SEP_STR "prg",
    FSDEV_EXT_SEP_STR "lnx",
    FSDEV_EXT_SEP_STR "tap",
    NULL
};

static int is_valid_extension(char *str)
{
	int i;
    size_t len;
	size_t slen = strlen(str);

    for (i = 0; extensions[i]; i++) {
        len = strlen(extensions[i]);
        if (len <= slen && !strcasecmp(extensions[i], str + slen - len)) {
            return 1;
        }
    }
    return 0;
}

/* define SIZE_MAX if it does not exist (only in C99) */
#ifndef SIZE_MAX
#define SIZE_MAX ((size_t)-1)
#endif

/* If `name' has a correct extension, try to list its contents and search for
   the first file with a proper extension; if found, extract it.  If this
   succeeds, return the name of the temporary file; if the archive file is
   valid but `write_mode' is non-zero, return a zero-length string; in all
   the other cases, return NULL.  */
static char *try_uncompress_with_zip(const char *name)
{
	char *extension=".zip";
	char *tmp_name = NULL;
    size_t l = strlen(name), len;
    int found = 0;
    int exit_status;
    FILE *fd;
    char tmp[1024];
	zip_int64_t num_entries;
	zip_uint64_t i;
	const char *fname;
	struct zip_file *zf;

    /* Do we have correct extension?  */
    len = strlen(extension);
    if (l <= len || strcasecmp(name + l - len, extension) != 0) {
        return NULL;
    }

    // open the archive
	struct zip *za;
    if ((za = zip_open(name, 0, &exit_status)) == NULL)
		return NULL;
	
	/* First run listing and search for first recognizeable extension.  */
	num_entries = zip_get_num_entries(za, 0);

	for (i = 0; i < (zip_uint64_t)num_entries; i++) {
		fname = zip_get_name(za, i, 0);
		if (is_valid_extension((char *)fname)) {
			ZDEBUG(("%s: found '%s'.", __func__, fname));
			found = 1;
			break;
		}
	}

	if (!found) {
		zip_close(za);
        ZDEBUG(("%s: no valid file found.", __func__));
        return NULL;
    }

	tmp_name=archdep_join_paths(archdep_xdg_data_home(), "data", fname, NULL);

	mkpath(tmp_name,0);
	fd = fopen(tmp_name, "w");
	if (fd==NULL) {
		ZDEBUG(("%s: could not open output file %s", __func__, tmp_name));
		lib_free(tmp_name);
		zip_close(za);
		return NULL;
	}
	
	zf = zip_fopen_index(za, i, 0);
	while ((len = zip_fread(zf, tmp, 1024))>0)
		fwrite(tmp, sizeof(char), len, fd);
	zip_fclose(zf);
	fclose(fd);

	zip_close(za);

    ZDEBUG(("%s: extract '%s' successful.", __func__, tmp_name));
    return tmp_name;
}

/* Try to uncompress file `name' using the algorithms we know of.  If this is
   not possible, return `COMPR_NONE'.  Otherwise, uncompress the file into a
   temporary file, return the type of algorithm used and the name of the
   temporary file in `tmp_name'.  If `write_mode' is non-zero and the
   returned `tmp_name' has zero length, then the file cannot be accessed in
   write mode.  */
static enum compression_type try_uncompress(const char *name,
                                            char **tmp_name,
                                            int write_mode)
{
	if ((*tmp_name = try_uncompress_with_zip(name)) != NULL) {
        return COMPR_ZIP;
    }

    if ((*tmp_name = try_uncompress_with_gzip(name)) != NULL) {
        return COMPR_GZIP;
    }

    return COMPR_NONE;
}

/* ------------------------------------------------------------------------- */

/* Compression.  */

/* Compress `src' into `dest' using gzip.  */
static int compress_with_gzip(const char *src, const char *dest)
{
    FILE *fdsrc;
    gzFile fddest;
    size_t len;

    fdsrc = fopen(dest, MODE_READ);
    if (fdsrc == NULL) {
        return -1;
    }

    fddest = gzopen(src, MODE_WRITE "9");
    if (fddest == NULL) {
        fclose(fdsrc);
        return -1;
    }

    do {
        char buf[256];
        len = fread((void *)buf, 256, 1, fdsrc);
        if (len > 0) {
            gzwrite(fddest, (void *)buf, (unsigned int)len);
        }
    } while (len > 0);

    gzclose(fddest);
    fclose(fdsrc);

    ZDEBUG(("compress with zlib: OK."));

    return 0;
}

/* Compress `src' into `dest' using algorithm `type'.  */
static int zfile_compress(const char *src, const char *dest,
                          enum compression_type type)
{
    char *dest_backup_name;
    int retval;

    /* This shouldn't happen */
    if (type == COMPR_ZIP) {
        log_error(zlog, "compress: trying to compress archive-file.");
        return -1;
    }

    /* Check whether `compression_type' is a known one.  */
    if (type != COMPR_GZIP) {
        log_error(zlog, "compress: unknown compression type");
        return -1;
    }

    /* If we have no write permissions for `dest', give up.  */
    if (ioutil_access(dest, IOUTIL_ACCESS_W_OK) < 0) {
        ZDEBUG(("compress: no write permissions for `%s'",
                dest));
        return -1;
    }

    if (ioutil_access(dest, IOUTIL_ACCESS_R_OK) < 0) {
        ZDEBUG(("compress: no read permissions for `%s'", dest));
        dest_backup_name = NULL;
    } else {
        /* If `dest' exists, make a backup first.  */
        dest_backup_name = archdep_make_backup_filename(dest);
        if (dest_backup_name != NULL) {
            ZDEBUG(("compress: making backup %s... ", dest_backup_name));
        }
        if (dest_backup_name != NULL
            && ioutil_rename(dest, dest_backup_name) < 0) {
            ZDEBUG(("failed."));
            log_error(zlog, "Could not make pre-compression backup.");
            return -1;
        } else {
            ZDEBUG(("OK."));
        }
    }

    switch (type) {
        case COMPR_GZIP:
            retval = compress_with_gzip(src, dest);
            break;
        default:
            retval = -1;
    }

    if (retval == -1) {
        /* Compression failed: restore original file.  */
        if (dest_backup_name != NULL
            && ioutil_rename(dest_backup_name, dest) < 0) {
            log_error(zlog,
                      "Could not restore backup file after failed compression.");
        }
    } else {
        /* Compression succeeded: remove backup file.  */
        if (dest_backup_name != NULL
            && ioutil_remove(dest_backup_name) < 0) {
            log_error(zlog, "Warning: could not remove backup file.");
            /* Do not return an error anyway (no data is lost).  */
        }
    }

    if (dest_backup_name) {
        lib_free(dest_backup_name);
    }
    return retval;
}

/* ------------------------------------------------------------------------ */

/* Here we have the actual fopen and fclose wrappers.

   These functions work exactly like the standard library versions, but
   handle compression and decompression automatically.  When a file is
   opened, we check whether it looks like a compressed file of some kind.
   If so, we uncompress it and then actually open the uncompressed version.
   When a file that was opened for writing is closed, we re-compress the
   uncompressed version and update the original file.  */

/* `fopen()' wrapper.  */
FILE *zfile_fopen(const char *name, const char *mode)
{
    char *tmp_name;
    FILE *stream;
    enum compression_type type;
    int write_mode = 0;

    if (!zinit_done) {
        zinit();
    }

    if (name == NULL || name[0] == 0) {
        return NULL;
    }

    /* Do we want to write to this file?  */
    if ((strchr(mode, 'w') != NULL) || (strchr(mode, '+') != NULL)) {
        write_mode = 1;
    }

    /* Check for write permissions.  */
    if (write_mode && ioutil_access(name, IOUTIL_ACCESS_W_OK) < 0) {
        return NULL;
    }

    type = try_uncompress(name, &tmp_name, write_mode);
    if (type == COMPR_NONE) {
        stream = fopen(name, mode);
        if (stream == NULL) {
            return NULL;
        }
        zfile_list_add(NULL, name, type, write_mode, stream, NULL);
        return stream;
    } else if (*tmp_name == '\0') {
        errno = EACCES;
        return NULL;
    }

    /* Open the uncompressed version of the file.  */
    stream = fopen(tmp_name, mode);
    if (stream == NULL) {
        return NULL;
    }

    zfile_list_add(tmp_name, name, type, write_mode, stream, NULL);

    /* now we don't need the archdep_tmpnam allocation any more */
    lib_free(tmp_name);

    return stream;
}

/* Handle close-action of a file.  `ptr' points to the zfile to close.  */
static int handle_close_action(zfile_t *ptr)
{
    if (ptr == NULL || ptr->orig_name == NULL) {
        return -1;
    }

    switch (ptr->action) {
        case ZFILE_KEEP:
            break;
        case ZFILE_REQUEST:
        /*
          ui_zfile_close_request(ptr->orig_name, ptr->request_string);
          break;
        */
        case ZFILE_DEL:
            if (ioutil_remove(ptr->orig_name) < 0) {
                log_error(zlog, "Cannot unlink `%s': %s",
                          ptr->orig_name, strerror(errno));
            }
            break;
    }
    return 0;
}

/* Handle close of a (compressed file). `ptr' points to the zfile to close.  */
static int handle_close(zfile_t *ptr)
{
    ZDEBUG(("handle_close: closing `%s' (`%s'), write_mode = %d",
            ptr->tmp_name ? ptr->tmp_name : "(null)",
            ptr->orig_name, ptr->write_mode));

    if (ptr->tmp_name) {
        /* Recompress into the original file.  */
        if (ptr->orig_name
            && ptr->write_mode
            && zfile_compress(ptr->tmp_name, ptr->orig_name, ptr->type)) {
            return -1;
        }

        /* Remove temporary file.  */
        if (ioutil_remove(ptr->tmp_name) < 0) {
            log_error(zlog, "Cannot unlink `%s': %s", ptr->tmp_name, strerror(errno));
        }
    }

    handle_close_action(ptr);

    /* Remove item from list.  */
    if (ptr->prev != NULL) {
        ptr->prev->next = ptr->next;
    } else {
        zfile_list = ptr->next;
    }

    if (ptr->next != NULL) {
        ptr->next->prev = ptr->prev;
    }

    if (ptr->orig_name) {
        lib_free(ptr->orig_name);
    }
    if (ptr->tmp_name) {
        lib_free(ptr->tmp_name);
    }
    if (ptr->request_string) {
        lib_free(ptr->request_string);
    }

    lib_free(ptr);

    return 0;
}

/* `fclose()' wrapper.  */
int zfile_fclose(FILE *stream)
{
    zfile_t *ptr;

    if (!zinit_done) {
        errno = EBADF;
        return -1;
    }

    /* Search for the matching file in the list.  */
    for (ptr = zfile_list; ptr != NULL; ptr = ptr->next) {
        if (ptr->stream == stream) {
            /* Close temporary file.  */
            if (fclose(stream) == -1) {
                return -1;
            }
            if (handle_close(ptr) < 0) {
                errno = EBADF;
                return -1;
            }

            return 0;
        }
    }

    return fclose(stream);
}

int zfile_close_action(const char *filename, zfile_action_t action,
                       const char *request_str)
{
    char *fullname = NULL;
    zfile_t *p = zfile_list;

    archdep_expand_path(&fullname, filename);

    while (p != NULL) {
        if (p->orig_name && !strcmp(p->orig_name, fullname)) {
            p->action = action;
            p->request_string = request_str ? lib_stralloc(request_str) : NULL;
            lib_free(fullname);
            return 0;
        }
        p = p->next;
    }

    lib_free(fullname);
    return -1;
}
