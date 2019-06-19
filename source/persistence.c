/*
 * persistence.c - functions for gettig/setting persitent data
 *
 * Written by
 *  Sebastian weber <me@badda.de>
 *
 * This file is part of VICE3DS
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "log.h"
#include "persistence.h"
#include "archdep_user_config_path.h"
#include "archdep_join_paths.h"

static char **data=NULL;
static int numdata=0;
static char *filename=NULL;
static int persistence_isinit=0;

ssize_t
getdelim(char **buf, size_t *bufsiz, int delimiter, FILE *fp)
{
	char *ptr, *eptr;


	if (*buf == NULL || *bufsiz == 0) {
		*bufsiz = BUFSIZ;
		if ((*buf = malloc(*bufsiz)) == NULL)
			return -1;
	}

	for (ptr = *buf, eptr = *buf + *bufsiz;;) {
		int c = fgetc(fp);
		if (c == -1) {
			if (feof(fp))
				return ptr == *buf ? -1 : ptr - *buf;
			else
				return -1;
		}
		*ptr++ = c;
		if (c == delimiter) {
			*ptr = '\0';
			return ptr - *buf;
		}
		if (ptr + 2 >= eptr) {
			char *nbuf;
			size_t nbufsiz = *bufsiz * 2;
			ssize_t d = ptr - *buf;
			if ((nbuf = realloc(*buf, nbufsiz)) == NULL)
				return -1;
			*buf = nbuf;
			*bufsiz = nbufsiz;
			eptr = nbuf + nbufsiz;
			ptr = nbuf + d;
		}
	}
}

ssize_t
getline(char **buf, size_t *bufsiz, FILE *fp)
{
	return getdelim(buf, bufsiz, '\n', fp);
}

static void persistence_fini() {
	for (int i=0;i<numdata;i++) {
		free(data[i*2]);
		free(data[i*2+1]);
	}
	if (data) free(data);
}

static void persistence_init() {
	// what is my filename?
	filename = archdep_join_paths(archdep_user_config_path(), "persistence", NULL);
	persistence_isinit=1;

	// read in persistence data
	unsigned int len;
	char *line=NULL;
	char *p,*v;

	FILE* f = fopen(filename, "r");
	if (f == NULL) return;
	while (1) {
		if (line) {
			free(line);
			line=NULL;
		}
		len=0;
		if (getline(&line,&len,f)==-1)
			break;
		if ((v=strchr(line,'='))==NULL)
			continue;
		*v++=0;
		if ((p=strchr(v,'\n'))!=NULL) // strip \n
			*p=0;
		data=realloc(data, (numdata*2+2)*sizeof(*data));
		data[numdata*2]=malloc(strlen(line)+1);
		memcpy(data[numdata*2],line,strlen(line)+1);
		data[numdata*2+1]=malloc(strlen(v)+1);
		memcpy(data[numdata*2+1],v,strlen(v)+1);
		++numdata;
	}
	if (line)
		free(line);
	fclose(f);
	atexit(persistence_fini);
}

static int findkey(char *key) {
	if (!persistence_isinit) persistence_init();
	for (int i=0; i < numdata; i++) {
		if (strcmp(key,data[i*2])==0) return i;
	}
	return -1;
}

char *persistence_get(char *key, char *deflt) {
	int i;
	if ((i=findkey(key))!=-1)
		return data[i*2+1];
	return deflt;
}

int persistence_getInt(char *key, int deflt) {
	char *d;
	if ((d=persistence_get(key,NULL))!=NULL)
		return atoi(d);
	return deflt;
}

int persistence_put(char *key, char *value) {
	int i;
	if ((i=findkey(key))!=-1) {
		if (value!=NULL) {
			// insert data at existing pos
			free(data[i*2]);
			free(data[i*2+1]);
			data[i*2]=malloc(strlen(key)+1);
		    memcpy(data[i*2],key,strlen(key)+1);
			data[i*2+1]=malloc(strlen(value)+1);
		    memcpy(data[i*2+1],value,strlen(value)+1);
		} else {
			// delete data at existing pos
			free(data[i*2]);
			free(data[i*2+1]);
			for (int i2=i+1; i2<numdata; ++i2) {
				data[i2*2-2]=data[i2*2];
				data[i2*2-1]=data[i2*2+1];
			}
			--numdata;
		}
	} else if (value!=NULL) {
		// append new data
		data=realloc(data, (numdata*2+2)*sizeof(*data));
		data[numdata*2]=malloc(strlen(key)+1);
		memcpy(data[numdata*2],key,strlen(key)+1);
		data[numdata*2+1]=malloc(strlen(value)+1);
		memcpy(data[numdata*2+1],value,strlen(value)+1);
		++numdata;
	} else return -1;

	// save persistence
	FILE* f = fopen(filename, "w");
	for (i=0;i<numdata;i++) {
		fprintf(f,"%s=%s\n",data[i*2],data[i*2+1]);
	}
	fclose(f);
	return 0;
}

int persistence_putInt(char *key, int value) {
	char c[15];
	snprintf(c,sizeof(c),"%d",value);
	return persistence_put(key,c);
}

int persistence_remove(char *key) {
	return persistence_put(key,NULL);
}
