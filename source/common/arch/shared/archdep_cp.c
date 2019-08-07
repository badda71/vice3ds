/*
 * archdep_cp.c - custom implementation of xcopy
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
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <dirent.h>
#include <stdio.h>
#include "log.h"
#include "archdep_cp.h"

#define BUFFERSIZE 1024
#define MKDIRMOD 0644
#define PATH_SEP_CHAR '/'
#define PATH_SEP_STRING "/"

struct stat path_stat;
int overwrite=1;
static void *callback=NULL;

static int mkpath(char* file_path, int complete) {
	char* p;

	for (p=strchr(file_path+1, PATH_SEP_CHAR); p; p=strchr(p+1, PATH_SEP_CHAR)) {
		*p='\0';
		if (mkdir(file_path, MKDIRMOD)==-1) {
			if (errno!=EEXIST) { *p=PATH_SEP_CHAR; goto mkpath_err; }
		}
		*p=PATH_SEP_CHAR;
	}
	if (complete) {
		mkdir(file_path, MKDIRMOD);
	}
	return 0;
mkpath_err:
	return 1;
}

static int filecopy (char *src, char *dest)
{
	FILE *in_fd, *out_fd;
	int n_chars;
	char buf[BUFFERSIZE];

	// file exists?
	if (stat(dest, &path_stat) == 0) {
		if (overwrite==0) goto filecopy_ok;
	} else {
		mkpath(dest, 0);
	}
	if( (in_fd=fopen(src, "r")) == NULL ) goto filecopy_err;
	if( (out_fd=fopen(dest, "w")) == NULL ) goto filecopy_err;
	while( (n_chars = fread(buf, sizeof(char), sizeof(buf), in_fd)) > 0 )
	{
		if( fwrite(buf, sizeof(char), n_chars, out_fd) != n_chars ) break;
	}
	fclose(out_fd);
	fclose(in_fd);
filecopy_ok:
	return 0;
filecopy_err:
	return -1;
}

static int dircopy(char *src, char *dest)
{
	DIR *dir_ptr = NULL;
    struct dirent *direntp;
	char *newsrc,*newdest;

	mkpath(dest, 1);

	if( (dir_ptr = opendir(src)) == NULL ) goto dircopy_err;
	
	while( (direntp = readdir(dir_ptr)))
	{
		if (strcmp(direntp->d_name,".")==0 ||
			strcmp(direntp->d_name,"..")==0) continue;
		// new source
		newsrc=malloc(strlen(src)+strlen(direntp->d_name)+2);
		strcpy(newsrc, src);
		if (newsrc[strlen(newsrc)-1] != PATH_SEP_CHAR)
			strcat(newsrc, PATH_SEP_STRING);
		strcat(newsrc, direntp->d_name);
		// new destination
		newdest=malloc(strlen(dest)+strlen(direntp->d_name)+2);
		strcpy(newdest, dest);
		if (newdest[strlen(newdest)-1] != PATH_SEP_CHAR)
			strcat(newdest, PATH_SEP_STRING);
		strcat(newdest, direntp->d_name);
		if (xcopy(newsrc, newdest, overwrite, callback)!=0) {
			free(newdest);free(newsrc);
			goto dircopy_err;
		}
		free(newsrc);free(newdest);
	}
	closedir(dir_ptr);
	return 0;
dircopy_err:
	return -1;
}


// copy src to dest, returns 0 on success, -1 on error
int xcopy(char *src, char *dest, int overwrt,  void (*call)())
{
	callback=call;
	
	overwrite=overwrt;
	
	// copy dir or file
	if (stat(src, &path_stat) != 0 ) goto err_xcopy;
	if (S_ISDIR(path_stat.st_mode)) {
		if (dircopy(src,dest)!= 0) goto err_xcopy;
	} else {
		if (filecopy(src,dest)!=0) goto err_xcopy;
		if (call!=NULL) call();
	}

	return 0;

err_xcopy:
	return -1;
}
