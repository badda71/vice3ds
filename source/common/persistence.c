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
#include "archdep_xdg.h"
#include "archdep_join_paths.h"
#include "lib.h"

static char **data=NULL;
static int numdata=0;
static char *filename=NULL;
static int persistence_isinit=0;

void persistence_save() {
	// save persistence
	FILE* f = fopen(filename, "w");
	if (!f) return;
	for (int i=0;i<numdata;i++) {
		fprintf(f,"%s=%s\n",data[i*2],data[i*2+1]);
	}
	fclose(f);
}

static void persistence_fini() {
	persistence_save();
	for (int i=0;i<numdata;i++) {
		free(data[i*2]);
		free(data[i*2+1]);
	}
	if (data) free(data);
	free(filename);
}

static void persistence_init() {
	// what is my filename?
	filename = archdep_join_paths(archdep_xdg_data_home(), "persistence", NULL);
	numdata=0;
	persistence_isinit=1;

	atexit(persistence_fini);

	// read in persistence data
	FILE *f;
	if ((f = fopen(filename, "rb")) == NULL) return;
	fseek(f, 0, SEEK_END);
	long fsize = ftell(f);
	fseek(f, 0, SEEK_SET);  /* same as rewind(f); */
	char *string = malloc(fsize + 2);
	if (string==NULL) {
		fclose(f);
		return;
	}
	fread(string+1, 1, fsize, f);
	fclose(f);
	
	// check the entries
	for (int i=0; i<fsize; i++) if (string[i]=='\n') ++numdata;
	data=malloc(numdata*2*sizeof(*data));
	if (data==NULL) {
		free(string);numdata=0;
		return;
	}
	int count=0;
    char* token;
    char* rest = string;
	while (count<numdata*2 && (token = strtok_r(rest, "=", &rest))) {
		data[count++]=lib_stralloc(token);
		token = strtok_r(rest, "\n", &rest);
		data[count++]=lib_stralloc(token);
	}
	free(string);
	fclose(f);
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
			data[i*2]=lib_stralloc(key);
			data[i*2+1]=lib_stralloc(value);
		} else {
			// delete data at existing pos
			free(data[i*2]);
			free(data[i*2+1]);
			for (int i2=i+1; i2<numdata; ++i2) {
				data[i2*2-2]=data[i2*2];
				data[i2*2-1]=data[i2*2+1];
			}
			data=realloc(data, (numdata*2-2)*sizeof(*data));
			--numdata;
		}
	} else if (value!=NULL) {
		// append new data
		data=realloc(data, (numdata*2+2)*sizeof(*data));
		data[numdata*2]=lib_stralloc(key);
		data[numdata*2+1]=lib_stralloc(value);
		++numdata;
	} else return -1;
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
