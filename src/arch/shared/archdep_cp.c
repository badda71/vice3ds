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
#define COPYMOD 0644
#define PATH_SEP_CHAR '/'
#define PATH_SEP_STRING "/"
//#define DEBUG 1

struct stat path_stat;
int overwrite=1;

#ifndef _3DS
static char *basename(char *path)
{
	char *bname;
	if ((bname = strrchr(path, PATH_SEP_CHAR)+1) == (void *)1)
		bname=path;
#ifdef DEBUG
	log_debug("basename %s -> %s",path,bname);
#endif
	return bname;
}
#endif
/*
int main (int argc, char **argv,1) {
	if (argc!=3) return -1;
	xcopy (argv[1],argv[2],1);
}
*/
static int mkpath(char* file_path, int complete) {
#ifdef DEBUG
	log_debug("mkpath %s %d",file_path, complete);
#endif
	char* p=file_path;
	while (p=strchr(p+1, '/')) {
		*p='\0';
		if (mkdir(file_path, COPYMOD) != 0) {
			if (errno!=EEXIST) { *p='/'; return -1; }
		}
		*p='/';
	}
	if (complete) {
		mkdir(file_path, COPYMOD);
	}

	return 0;
}

static int filecopy (char *src, char *dest)
{
#ifdef DEBUG
	log_debug("filecopy %s %s",src, dest);
#endif
	FILE *in_fd, *out_fd;
	int n_chars;
	char buf[BUFFERSIZE];

	// file exists?
	if (stat(dest, &path_stat) == 0 && overwrite==0) return 0;

	mkpath(dest, 0);
	if( (in_fd=fopen(src, "r")) == NULL ) return -1;
	if( (out_fd=fopen(dest, "w")) == NULL ) return -1;
	while( (n_chars = fread(buf, 1, BUFFERSIZE, in_fd)) > 0 )
	{
		if( fwrite(buf, 1, n_chars, out_fd) != n_chars ) break;
	}
	fclose(in_fd);
	fclose(out_fd);
	return 0;
}

static int dircopy(char *src, char *dest)
{
#ifdef DEBUG
	log_debug("dircopy %s %s",src, dest);
#endif
	DIR *dir_ptr = NULL;
    struct dirent *direntp;
	char *newsrc,*newdest;

	mkpath(dest, 1);

	if( (dir_ptr = opendir(src)) == NULL ) return -1;
	
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
		if (xcopy(newsrc, newdest, overwrite)!=0) {
			free(newdest);free(newsrc);
			return -1;
		}
		free(newsrc);free(newdest);
	}
}


// copy src to dest, returns 0 on success, -1 on error
int xcopy(char *src, char *dest, int overwrt)
{
#ifdef DEBUG
	log_debug("xcopy %s %s %d",src, dest, overwrt);
#endif
	overwrite=overwrt;

	// get my basename
	char *bname = basename(src);
	
	// copy dir or file
	if (stat(src, &path_stat) != 0 ) goto err_xcopy;
	if (S_ISDIR(path_stat.st_mode)) {
		if (dircopy(src,dest)!= 0) goto err_xcopy;
	} else {
		if (filecopy(src,dest)!=0) goto err_xcopy;
	}

	return 0;

err_xcopy:
	return -1;
}
