/*
 * archdep_cp.h - custom implementation of xcopy
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

// copies source to destination, creating destination and all directories on
// the way
// returns 0 on success, -1 on failure
// source: source file or directory
// destination: destination file or directory
// overwrite: 1 = overwrite existing files, 0 = leave existing files untouched

extern int xcopy(char *source, char *destination, int overwrite, void (*callback)());
extern int mkpath(char* file_path, int complete);
