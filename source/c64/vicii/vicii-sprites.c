/*
 * vicii-sprites.c - Sprites for the MOS 6569 (VIC-II) emulation.
 *
 * Written by
 *  Ettore Perazzoli <ettore@comm2000.it>
 *  Andreas Boose <viceteam@t-online.de>
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>             /* memset() */

#include "lib.h"
#include "raster-cache.h"
#include "raster-changes.h"
#include "raster-sprite-status.h"
#include "raster-sprite.h"
#include "types.h"
#include "vicii-sprites.h"
#include "viciitypes.h"
#include "viewport.h"


/* margin definitions of where sprite data fetches interfere with */
/* actual sprite data display, resulting in repeated pixels or    */
/* ineffective sprite pattern data fetches                        */

/* sprite #n repeats pixel SPRITE_REPEAT_BEGIN(n)-1 up to SPRITE_REPEAT_BEGIN(n)+6 */
/* no display from SPRITE_REPEAT_BEGIN(n)+7 to SPRITE_REPEAT_BEGIN(n)+11           */

#define X_OFFSET vicii.screen_leftborderwidth

#define SPRITE_EXPANDED_REPEAT_PIXELS_START(n) (vicii.sprite_wrap_x < 0x200 ? \
                                                (0x11a + X_OFFSET + n * 0x10) : \
                                                (0x122 + X_OFFSET + n * 0x10))

#define SPRITE_NORMAL_REPEAT_PIXELS_START(n) (vicii.sprite_wrap_x < 0x200 ? \
                                              (0x132 + X_OFFSET + n * 0x10) : \
                                              (0x13a + X_OFFSET + n * 0x10))

#define SPRITE_REPEAT_PIXELS_END(n) (vicii.sprite_wrap_x < 0x200 ? \
                                     (0x157 + X_OFFSET + n * 0x10) :  \
                                     (0x15f + X_OFFSET + n * 0x10))
/* where the sprite pixels start repeating */
#define SPRITE_REPEAT_BEGIN(n) (SPRITE_REPEAT_PIXELS_END(n) - 0xc)

/* maximum x where sprite data fetched in the previous line is displayed */
#define SPRITE_DISPLAY_PREVIOUS_PATTERN (0x14c + X_OFFSET)
/* Minimum X where sprite starts immediately with data already fetched */
#define SPRITE_DISPLAY_IMMEDIATE_DATA_FETCHED(n) (0x156 + X_OFFSET + n * 0x10)


/** @brief  Three sprite fetch table entry terminators */
#define SF_END_3 { -1, -1, 0, 0 }, { -1, -1, 0, 0 }, { -1, -1, 0, 0 }

/** @brief  Two sprite fetch table entry terminators */
#define SF_END_2 { -1, -1, 0, 0 }, { -1, -1, 0, 0 }

/** @brief  A single Schwarzenegger */
#define SF_END_1 { -1, -1, 0, 0 }


/** @brief  Sprite fetch data
 *
 * FIXME:   This can do with some proper explanation!
 */
const vicii_sprites_fetch_t vicii_sprites_fetch_table[256][4] =
{
    /* $00 */ { { -1, -1, 0, 0 }, SF_END_3 },
    /* $01 */ { {  0,  5, 0, 0 }, SF_END_3 },
    /* $02 */ { {  2,  5, 1, 1 }, SF_END_3 },
    /* $03 */ { {  0,  7, 0, 1 }, SF_END_3 },
    /* $04 */ { {  4,  5, 2, 2 }, SF_END_3 },
    /* $05 */ { {  0,  9, 0, 2 }, SF_END_3 },
    /* $06 */ { {  2,  7, 1, 2 }, SF_END_3 },
    /* $07 */ { {  0,  9, 0, 2 }, SF_END_3 },
    /* $08 */ { {  6,  5, 3, 3 }, SF_END_3 },
    /* $09 */ { {  0,  5, 0, 0 }, {  6,  5, 3, 3 }, SF_END_2 },
    /* $0A */ { {  2,  9, 1, 3 }, SF_END_3 },
    /* $0B */ { {  0, 11, 0, 3 }, SF_END_3 },
    /* $0C */ { {  4,  7, 2, 3 }, SF_END_3 },
    /* $0D */ { {  0, 11, 0, 3 }, SF_END_3 },
    /* $0E */ { {  2,  9, 1, 3 }, SF_END_3 },
    /* $0F */ { {  0, 11, 0, 3 }, SF_END_3 },
    /* $10 */ { {  8,  5, 4, 4 }, SF_END_3 },
    /* $11 */ { {  0,  5, 0, 0 }, {  8,  5, 4, 4 }, SF_END_2 },
    /* $12 */ { {  2,  5, 1, 1 }, {  8,  5, 4, 4 }, SF_END_2 },
    /* $13 */ { {  0,  7, 0, 1 }, {  8,  5, 4, 4 }, SF_END_2 },
    /* $14 */ { {  4,  9, 2, 4 }, SF_END_3 },
    /* $15 */ { {  0, 13, 0, 4 }, SF_END_3 },
    /* $16 */ { {  2, 11, 1, 4 }, SF_END_3 },
    /* $17 */ { {  0, 13, 0, 4 }, SF_END_3 },
    /* $18 */ { {  6,  7, 3, 4 }, SF_END_3 },
    /* $19 */ { {  0,  5, 0, 0 }, {  6,  7, 3, 4 }, SF_END_2 },
    /* $1A */ { {  2, 11, 1, 4 }, SF_END_3 },
    /* $1B */ { {  0, 13, 0, 4 }, SF_END_3 },
    /* $1C */ { {  4,  9, 2, 4 }, SF_END_3 },
    /* $1D */ { {  0, 13, 0, 4 }, SF_END_3 },
    /* $1E */ { {  2, 11, 1, 4 }, SF_END_3 },
    /* $1F */ { {  0, 13, 0, 4 }, SF_END_3 },
    /* $20 */ { { 10,  5, 5, 5 }, SF_END_3 },
    /* $21 */ { {  0,  5, 0, 0 }, { 10,  5, 5, 5 }, SF_END_2 },
    /* $22 */ { {  2,  5, 1, 1 }, { 10,  5, 5, 5 }, SF_END_2 },
    /* $23 */ { {  0,  7, 0, 1 }, { 10,  5, 5, 5 }, SF_END_2 },
    /* $24 */ { {  4,  5, 2, 2 }, { 10,  5, 5, 5 }, SF_END_2 },
    /* $25 */ { {  0,  9, 0, 2 }, { 10,  5, 5, 5 }, SF_END_2 },
    /* $26 */ { {  2,  7, 1, 2 }, { 10,  5, 5, 5 }, SF_END_2 },
    /* $27 */ { {  0,  9, 0, 2 }, { 10,  5, 5, 5 }, SF_END_2 },
    /* $28 */ { {  6,  9, 3, 5 }, SF_END_3 },
    /* $29 */ { {  0,  5, 0, 0 }, {  6,  9, 3, 5 }, SF_END_2 },
    /* $2A */ { {  2, 13, 1, 5 }, SF_END_3 },
    /* $2B */ { {  0, 15, 0, 5 }, SF_END_3 },
    /* $2C */ { {  4, 11, 2, 5 }, SF_END_3 },
    /* $2D */ { {  0, 15, 0, 5 }, SF_END_3 },
    /* $2E */ { {  2, 13, 1, 5 }, SF_END_3 },
    /* $2F */ { {  0, 15, 0, 5 }, SF_END_3 },
    /* $30 */ { {  8,  7, 4, 5 }, SF_END_3 },
    /* $31 */ { {  0,  5, 0, 0 }, {  8,  7, 4, 5 }, SF_END_2 },
    /* $32 */ { {  2,  5, 1, 1 }, {  8,  7, 4, 5 }, SF_END_2 },
    /* $33 */ { {  0,  7, 0, 1 }, {  8,  7, 4, 5 }, SF_END_2 },
    /* $34 */ { {  4, 11, 2, 5 }, SF_END_3 },
    /* $35 */ { {  0, 15, 0, 5 }, SF_END_3 },
    /* $36 */ { {  2, 13, 1, 5 }, SF_END_3 },
    /* $37 */ { {  0, 15, 0, 5 }, SF_END_3 },
    /* $38 */ { {  6,  9, 3, 5 }, SF_END_3 },
    /* $39 */ { {  0,  5, 0, 0 }, {  6,  9, 3, 5 }, SF_END_2 },
    /* $3A */ { {  2, 13, 1, 5 }, SF_END_3 },
    /* $3B */ { {  0, 15, 0, 5 }, SF_END_3 },
    /* $3C */ { {  4, 11, 2, 5 }, SF_END_3 },
    /* $3D */ { {  0, 15, 0, 5 }, SF_END_3 },
    /* $3E */ { {  2, 13, 1, 5 }, SF_END_3 },
    /* $3F */ { {  0, 15, 0, 5 }, SF_END_3 },
    /* $40 */ { { 12,  5, 6, 6 }, SF_END_3 },
    /* $41 */ { {  0,  5, 0, 0 }, { 12,  5, 6, 6 }, SF_END_2 },
    /* $42 */ { {  2,  5, 1, 1 }, { 12,  5, 6, 6 }, SF_END_2 },
    /* $43 */ { {  0,  7, 0, 1 }, { 12,  5, 6, 6 }, SF_END_2 },
    /* $44 */ { {  4,  5, 2, 2 }, { 12,  5, 6, 6 }, SF_END_2 },
    /* $45 */ { {  0,  9, 0, 2 }, { 12,  5, 6, 6 }, SF_END_2 },
    /* $46 */ { {  2,  7, 1, 2 }, { 12,  5, 6, 6 }, SF_END_2 },
    /* $47 */ { {  0,  9, 0, 2 }, { 12,  5, 6, 6 }, SF_END_2 },
    /* $48 */ { {  6,  5, 3, 3 }, { 12,  5, 6, 6 }, SF_END_2 },
    /* $49 */ { {  0,  5, 0, 0 }, {  6,  5, 3, 3 }, { 12,  5, 6, 6 }, SF_END_1 },
    /* $4A */ { {  2,  9, 1, 3 }, { 12,  5, 6, 6 }, SF_END_2 },
    /* $4B */ { {  0, 11, 0, 3 }, { 12,  5, 6, 6 }, SF_END_2 },
    /* $4C */ { {  4,  7, 2, 3 }, { 12,  5, 6, 6 }, SF_END_2 },
    /* $4D */ { {  0, 11, 0, 3 }, { 12,  5, 6, 6 }, SF_END_2 },
    /* $4E */ { {  2,  9, 1, 3 }, { 12,  5, 6, 6 }, SF_END_2 },
    /* $4F */ { {  0, 11, 0, 3 }, { 12,  5, 6, 6 }, SF_END_2 },
    /* $50 */ { {  8,  9, 4, 6 }, SF_END_3 },
    /* $51 */ { {  0,  5, 0, 0 }, {  8,  9, 4, 6 }, SF_END_2 },
    /* $52 */ { {  2,  5, 1, 1 }, {  8,  9, 4, 6 }, SF_END_2 },
    /* $53 */ { {  0,  7, 0, 1 }, {  8,  9, 4, 6 }, SF_END_2 },
    /* $54 */ { {  4, 13, 2, 6 }, SF_END_3 },
    /* $55 */ { {  0, 17, 0, 6 }, SF_END_3 },
    /* $56 */ { {  2, 15, 1, 6 }, SF_END_3 },
    /* $57 */ { {  0, 17, 0, 6 }, SF_END_3 },
    /* $58 */ { {  6, 11, 3, 6 }, SF_END_3 },
    /* $59 */ { {  0,  5, 0, 0 }, {  6, 11, 3, 6 }, SF_END_2 },
    /* $5A */ { {  2, 15, 1, 6 }, SF_END_3 },
    /* $5B */ { {  0, 17, 0, 6 }, SF_END_3 },
    /* $5C */ { {  4, 13, 2, 6 }, SF_END_3 },
    /* $5D */ { {  0, 17, 0, 6 }, SF_END_3 },
    /* $5E */ { {  2, 15, 1, 6 }, SF_END_3 },
    /* $5F */ { {  0, 17, 0, 6 }, SF_END_3 },
    /* $60 */ { { 10,  7, 5, 6 }, SF_END_3 },
    /* $61 */ { {  0,  5, 0, 0 }, { 10,  7, 5, 6 }, SF_END_2 },
    /* $62 */ { {  2,  5, 1, 1 }, { 10,  7, 5, 6 }, SF_END_2 },
    /* $63 */ { {  0,  7, 0, 1 }, { 10,  7, 5, 6 }, SF_END_2 },
    /* $64 */ { {  4,  5, 2, 2 }, { 10,  7, 5, 6 }, SF_END_2 },
    /* $65 */ { {  0,  9, 0, 2 }, { 10,  7, 5, 6 }, SF_END_2 },
    /* $66 */ { {  2,  7, 1, 2 }, { 10,  7, 5, 6 }, SF_END_2 },
    /* $67 */ { {  0,  9, 0, 2 }, { 10,  7, 5, 6 }, SF_END_2 },
    /* $68 */ { {  6, 11, 3, 6 }, SF_END_3 },
    /* $69 */ { {  0,  5, 0, 0 }, {  6, 11, 3, 6 }, SF_END_2 },
    /* $6A */ { {  2, 15, 1, 6 }, SF_END_3 },
    /* $6B */ { {  0, 17, 0, 6 }, SF_END_3 },
    /* $6C */ { {  4, 13, 2, 6 }, SF_END_3 },
    /* $6D */ { {  0, 17, 0, 6 }, SF_END_3 },
    /* $6E */ { {  2, 15, 1, 6 }, SF_END_3 },
    /* $6F */ { {  0, 17, 0, 6 }, SF_END_3 },
    /* $70 */ { {  8,  9, 4, 6 }, SF_END_3 },
    /* $71 */ { {  0,  5, 0, 0 }, {  8,  9, 4, 6 }, SF_END_2 },
    /* $72 */ { {  2,  5, 1, 1 }, {  8,  9, 4, 6 }, SF_END_2 },
    /* $73 */ { {  0,  7, 0, 1 }, {  8,  9, 4, 6 }, SF_END_2 },
    /* $74 */ { {  4, 13, 2, 6 }, SF_END_3 },
    /* $75 */ { {  0, 17, 0, 6 }, SF_END_3 },
    /* $76 */ { {  2, 15, 1, 6 }, SF_END_3 },
    /* $77 */ { {  0, 17, 0, 6 }, SF_END_3 },
    /* $78 */ { {  6, 11, 3, 6 }, SF_END_3 },
    /* $79 */ { {  0,  5, 0, 0 }, {  6, 11, 3, 6 }, SF_END_2 },
    /* $7A */ { {  2, 15, 1, 6 }, SF_END_3 },
    /* $7B */ { {  0, 17, 0, 6 }, SF_END_3 },
    /* $7C */ { {  4, 13, 2, 6 }, SF_END_3 },
    /* $7D */ { {  0, 17, 0, 6 }, SF_END_3 },
    /* $7E */ { {  2, 15, 1, 6 }, SF_END_3 },
    /* $7F */ { {  0, 17, 0, 6 }, SF_END_3 },
    /* $80 */ { { 14,  5, 7, 7 }, SF_END_3 },
    /* $81 */ { {  0,  5, 0, 0 }, { 14,  5, 7, 7 }, SF_END_2 },
    /* $82 */ { {  2,  5, 1, 1 }, { 14,  5, 7, 7 }, SF_END_2 },
    /* $83 */ { {  0,  7, 0, 1 }, { 14,  5, 7, 7 }, SF_END_2 },
    /* $84 */ { {  4,  5, 2, 2 }, { 14,  5, 7, 7 }, SF_END_2 },
    /* $85 */ { {  0,  9, 0, 2 }, { 14,  5, 7, 7 }, SF_END_2 },
    /* $86 */ { {  2,  7, 1, 2 }, { 14,  5, 7, 7 }, SF_END_2 },
    /* $87 */ { {  0,  9, 0, 2 }, { 14,  5, 7, 7 }, SF_END_2 },
    /* $88 */ { {  6,  5, 3, 3 }, { 14,  5, 7, 7 }, SF_END_2 },
    /* $89 */ { {  0,  5, 0, 0 }, {  6,  5, 3, 3 }, { 14,  5, 7, 7 }, SF_END_1 },
    /* $8A */ { {  2,  9, 1, 3 }, { 14,  5, 7, 7 }, SF_END_2 },
    /* $8B */ { {  0, 11, 0, 3 }, { 14,  5, 7, 7 }, SF_END_2 },
    /* $8C */ { {  4,  7, 2, 3 }, { 14,  5, 7, 7 }, SF_END_2 },
    /* $8D */ { {  0, 11, 0, 3 }, { 14,  5, 7, 7 }, SF_END_2 },
    /* $8E */ { {  2,  9, 1, 3 }, { 14,  5, 7, 7 }, SF_END_2 },
    /* $8F */ { {  0, 11, 0, 3 }, { 14,  5, 7, 7 }, SF_END_2 },
    /* $90 */ { {  8,  5, 4, 4 }, { 14,  5, 7, 7 }, SF_END_2 },
    /* $91 */ { {  0,  5, 0, 0 }, {  8,  5, 4, 4 }, { 14,  5, 7, 7 }, SF_END_1 },
    /* $92 */ { {  2,  5, 1, 1 }, {  8,  5, 4, 4 }, { 14,  5, 7, 7 }, SF_END_1 },
    /* $93 */ { {  0,  7, 0, 1 }, {  8,  5, 4, 4 }, { 14,  5, 7, 7 }, SF_END_1 },
    /* $94 */ { {  4,  9, 2, 4 }, { 14,  5, 7, 7 }, SF_END_2 },
    /* $95 */ { {  0, 13, 0, 4 }, { 14,  5, 7, 7 }, SF_END_2 },
    /* $96 */ { {  2, 11, 1, 4 }, { 14,  5, 7, 7 }, SF_END_2 },
    /* $97 */ { {  0, 13, 0, 4 }, { 14,  5, 7, 7 }, SF_END_2 },
    /* $98 */ { {  6,  7, 3, 4 }, { 14,  5, 7, 7 }, SF_END_2 },
    /* $99 */ { {  0,  5, 0, 0 }, {  6,  7, 3, 4 }, { 14,  5, 7, 7 }, SF_END_1 },
    /* $9A */ { {  2, 11, 1, 4 }, { 14,  5, 7, 7 }, SF_END_2 },
    /* $9B */ { {  0, 13, 0, 4 }, { 14,  5, 7, 7 }, SF_END_2 },
    /* $9C */ { {  4,  9, 2, 4 }, { 14,  5, 7, 7 }, SF_END_2 },
    /* $9D */ { {  0, 13, 0, 4 }, { 14,  5, 7, 7 }, SF_END_2 },
    /* $9E */ { {  2, 11, 1, 4 }, { 14,  5, 7, 7 }, SF_END_2 },
    /* $9F */ { {  0, 13, 0, 4 }, { 14,  5, 7, 7 }, SF_END_2 },
    /* $A0 */ { { 10,  9, 5, 7 }, SF_END_3 },
    /* $A1 */ { {  0,  5, 0, 0 }, { 10,  9, 5, 7 }, SF_END_2 },
    /* $A2 */ { {  2,  5, 1, 1 }, { 10,  9, 5, 7 }, SF_END_2 },
    /* $A3 */ { {  0,  7, 0, 1 }, { 10,  9, 5, 7 }, SF_END_2 },
    /* $A4 */ { {  4,  5, 2, 2 }, { 10,  9, 5, 7 }, SF_END_2 },
    /* $A5 */ { {  0,  9, 0, 2 }, { 10,  9, 5, 7 }, SF_END_2 },
    /* $A6 */ { {  2,  7, 1, 2 }, { 10,  9, 5, 7 }, SF_END_2 },
    /* $A7 */ { {  0,  9, 0, 2 }, { 10,  9, 5, 7 }, SF_END_2 },
    /* $A8 */ { {  6, 13, 3, 7 }, SF_END_3 },
    /* $A9 */ { {  0,  5, 0, 0 }, {  6, 13, 3, 7 }, SF_END_2 },
    /* $AA */ { {  2, 17, 1, 7 }, SF_END_3 },
    /* $AB */ { {  0, 19, 0, 7 }, SF_END_3 },
    /* $AC */ { {  4, 15, 2, 7 }, SF_END_3 },
    /* $AD */ { {  0, 19, 0, 7 }, SF_END_3 },
    /* $AE */ { {  2, 17, 1, 7 }, SF_END_3 },
    /* $AF */ { {  0, 19, 0, 7 }, SF_END_3 },
    /* $B0 */ { {  8, 11, 4, 7 }, SF_END_3 },
    /* $B1 */ { {  0,  5, 0, 0 }, {  8, 11, 4, 7 }, SF_END_2 },
    /* $B2 */ { {  2,  5, 1, 1 }, {  8, 11, 4, 7 }, SF_END_2 },
    /* $B3 */ { {  0,  7, 0, 1 }, {  8, 11, 4, 7 }, SF_END_2 },
    /* $B4 */ { {  4, 15, 2, 7 }, SF_END_3 },
    /* $B5 */ { {  0, 19, 0, 7 }, SF_END_3 },
    /* $B6 */ { {  2, 17, 1, 7 }, SF_END_3 },
    /* $B7 */ { {  0, 19, 0, 7 }, SF_END_3 },
    /* $B8 */ { {  6, 13, 3, 7 }, SF_END_3 },
    /* $B9 */ { {  0,  5, 0, 0 }, {  6, 13, 3, 7 }, SF_END_2 },
    /* $BA */ { {  2, 17, 1, 7 }, SF_END_3 },
    /* $BB */ { {  0, 19, 0, 7 }, SF_END_3 },
    /* $BC */ { {  4, 15, 2, 7 }, SF_END_3 },
    /* $BD */ { {  0, 19, 0, 7 }, SF_END_3 },
    /* $BE */ { {  2, 17, 1, 7 }, SF_END_3 },
    /* $BF */ { {  0, 19, 0, 7 }, SF_END_3 },
    /* $C0 */ { { 12,  7, 6, 7 }, SF_END_3 },
    /* $C1 */ { {  0,  5, 0, 0 }, { 12,  7, 6, 7 }, SF_END_2 },
    /* $C2 */ { {  2,  5, 1, 1 }, { 12,  7, 6, 7 }, SF_END_2 },
    /* $C3 */ { {  0,  7, 0, 1 }, { 12,  7, 6, 7 }, SF_END_2 },
    /* $C4 */ { {  4,  5, 2, 2 }, { 12,  7, 6, 7 }, SF_END_2 },
    /* $C5 */ { {  0,  9, 0, 2 }, { 12,  7, 6, 7 }, SF_END_2 },
    /* $C6 */ { {  2,  7, 1, 2 }, { 12,  7, 6, 7 }, SF_END_2 },
    /* $C7 */ { {  0,  9, 0, 2 }, { 12,  7, 6, 7 }, SF_END_2 },
    /* $C8 */ { {  6,  5, 3, 3 }, { 12,  7, 6, 7 }, SF_END_2 },
    /* $C9 */ { {  0,  5, 0, 0 }, {  6,  5, 3, 3 }, { 12,  7, 6, 7 }, SF_END_1 },
    /* $CA */ { {  2,  9, 1, 3 }, { 12,  7, 6, 7 }, SF_END_2 },
    /* $CB */ { {  0, 11, 0, 3 }, { 12,  7, 6, 7 }, SF_END_2 },
    /* $CC */ { {  4,  7, 2, 3 }, { 12,  7, 6, 7 }, SF_END_2 },
    /* $CD */ { {  0, 11, 0, 3 }, { 12,  7, 6, 7 }, SF_END_2 },
    /* $CE */ { {  2,  9, 1, 3 }, { 12,  7, 6, 7 }, SF_END_2 },
    /* $CF */ { {  0, 11, 0, 3 }, { 12,  7, 6, 7 }, SF_END_2 },
    /* $D0 */ { {  8, 11, 4, 7 }, SF_END_3 },
    /* $D1 */ { {  0,  5, 0, 0 }, {  8, 11, 4, 7 }, SF_END_2 },
    /* $D2 */ { {  2,  5, 1, 1 }, {  8, 11, 4, 7 }, SF_END_2 },
    /* $D3 */ { {  0,  7, 0, 1 }, {  8, 11, 4, 7 }, SF_END_2 },
    /* $D4 */ { {  4, 15, 2, 7 }, SF_END_3 },
    /* $D5 */ { {  0, 19, 0, 7 }, SF_END_3 },
    /* $D6 */ { {  2, 17, 1, 7 }, SF_END_3 },
    /* $D7 */ { {  0, 19, 0, 7 }, SF_END_3 },
    /* $D8 */ { {  6, 13, 3, 7 }, SF_END_3 },
    /* $D9 */ { {  0,  5, 0, 0 }, {  6, 13, 3, 7 }, SF_END_2 },
    /* $DA */ { {  2, 17, 1, 7 }, SF_END_3 },
    /* $DB */ { {  0, 19, 0, 7 }, SF_END_3 },
    /* $DC */ { {  4, 15, 2, 7 }, SF_END_3 },
    /* $DD */ { {  0, 19, 0, 7 }, SF_END_3 },
    /* $DE */ { {  2, 17, 1, 7 }, SF_END_3 },
    /* $DF */ { {  0, 19, 0, 7 }, SF_END_3 },
    /* $E0 */ { { 10,  9, 5, 7 }, SF_END_3 },
    /* $E1 */ { {  0,  5, 0, 0 }, { 10, 9, 5, 7 }, SF_END_2 },
    /* $E2 */ { {  2,  5, 1, 1 }, { 10, 9, 5, 7 }, SF_END_2 },
    /* $E3 */ { {  0,  7, 0, 1 }, { 10, 9, 5, 7 }, SF_END_2 },
    /* $E4 */ { {  4,  5, 2, 2 }, { 10, 9, 5, 7 }, SF_END_2 },
    /* $E5 */ { {  0,  9, 0, 2 }, { 10, 9, 5, 7 }, SF_END_2 },
    /* $E6 */ { {  2,  7, 1, 2 }, { 10, 9, 5, 7 }, SF_END_2 },
    /* $E7 */ { {  0,  9, 0, 2 }, { 10, 9, 5, 7 }, SF_END_2 },
    /* $E8 */ { {  6, 13, 3, 7 }, SF_END_3 },
    /* $E9 */ { {  0,  5, 0, 0 }, {  6, 13, 3, 7 }, SF_END_2 },
    /* $EA */ { {  2, 17, 1, 7 }, SF_END_3 },
    /* $EB */ { {  0, 19, 0, 7 }, SF_END_3 },
    /* $EC */ { {  4, 15, 2, 7 }, SF_END_3 },
    /* $ED */ { {  0, 19, 0, 7 }, SF_END_3 },
    /* $EE */ { {  2, 17, 1, 7 }, SF_END_3 },
    /* $EF */ { {  0, 19, 0, 7 }, SF_END_3 },
    /* $F0 */ { {  8, 11, 4, 7 }, SF_END_3 },
    /* $F1 */ { {  0,  5, 0, 0 }, {  8, 11, 4, 7 }, SF_END_2 },
    /* $F2 */ { {  2,  5, 1, 1 }, {  8, 11, 4, 7 }, SF_END_2 },
    /* $F3 */ { {  0,  7, 0, 1 }, {  8, 11, 4, 7 }, SF_END_2 },
    /* $F4 */ { {  4, 15, 2, 7 }, SF_END_3 },
    /* $F5 */ { {  0, 19, 0, 7 }, SF_END_3 },
    /* $F6 */ { {  2, 17, 1, 7 }, SF_END_3 },
    /* $F7 */ { {  0, 19, 0, 7 }, SF_END_3 },
    /* $F8 */ { {  6, 13, 3, 7 }, SF_END_3 },
    /* $F9 */ { {  0,  5, 0, 0 }, {  6, 13, 3, 7 }, SF_END_2 },
    /* $FA */ { {  2, 17, 1, 7 }, SF_END_3 },
    /* $FB */ { {  0, 19, 0, 7 }, SF_END_3 },
    /* $FC */ { {  4, 15, 2, 7 }, SF_END_3 },
    /* $FD */ { {  0, 19, 0, 7 }, SF_END_3 },
    /* $FE */ { {  2, 17, 1, 7 }, SF_END_3 },
    /* $FF */ { {  0, 19, 0, 7 }, SF_END_3 }
};


/* This table was first published in the demo Krestage by Crest.
 *
 * FIXME:   This one can also do with some explanation on what those magic
 *          numbers are.
 */
const int vicii_sprites_crunch_table[64] =
{
     1,   4,   3,   /* 0 */
     4,   1,   0,   /* 3 */
    -1,   0,   1,   /* 6 */
     4,   3,   4,   /* 9 */
     1,   8,   7,   /* 12 */
     8,   1,   4,   /* 15 */
     3,   4,   1,   /* 18 */
     0,  -1,   0,   /* 21 */
     1,   4,   3,   /* 24 */
     4,   1,  -8,   /* 27 */
    -9,  -8,   1,   /* 30 */
     4,   3,   4,   /* 33 */
     1,   0,  -1,   /* 36 */
     0,   1,   4,   /* 39 */
     3,   4,   1,   /* 42 */
     8,   7,   8,   /* 45 */
     1,   4,   3,   /* 48 */
     4,   1,   0,   /* 51 */
    -1,   0,   1,   /* 54 */
     4,   3,   4,   /* 57 */
     1, -40, -41,   /* 60 */
     0
};


/* Each byte in this array is a bit mask representing the sprites that
   have a pixel turned on in that position.  This is used for sprite-sprite
   collision checking.  */
static uint8_t *sprline = NULL;

/* Sprite tables.  */
static uint16_t sprite_doubling_table[256];
static uint8_t mcsprtable[256];

static void init_drawing_tables(void)
{
    unsigned int i;
    uint16_t w;

    for (w = i = 0; i <= 0xff; i++) {
        mcsprtable[i] = i | ((i & 0x55) << 1) | ((i & 0xaa) >> 1);
        sprite_doubling_table[i] = w;
        w++;
        w |= (w & 0x5555) << 1;
    }
}

/* Sprite drawing macros.  */
#define SPRITE_PIXEL(do_draw, sprite_bit, imgptr, collmskptr, \
                     pos, color, collmsk_return)              \
    do {                                                      \
        if ((do_draw) && (collmskptr)[(pos)] == 0) {          \
            (imgptr)[(pos)] = (uint8_t)(color); }                \
        (collmsk_return) |= (collmskptr)[(pos)];              \
        (collmskptr)[(pos)] |= (sprite_bit);                  \
    } while (0)

/* Hires sprites */
#define _SPRITE_MASK(msk, gfxmsk, size, sprite_bit, imgptr,      \
                     collmskptr, color, collmsk_return, DRAW)    \
    do {                                                         \
        uint32_t __m;                                               \
        int __p;                                                 \
                                                                 \
        for (__m = 1 << ((size) - 1), __p = 0;                   \
             __p < (size);                                       \
             __p++, __m >>= 1) {                                 \
            if ((msk) & __m) {                                   \
                if ((gfxmsk) & __m) {                            \
                    DRAW(0, sprite_bit, imgptr, collmskptr, __p, \
                         color, collmsk_return);                 \
                } else {                                         \
                    DRAW(1, sprite_bit, imgptr, collmskptr, __p, \
                         color, collmsk_return); }               \
            }                                                    \
        }                                                        \
    } while (0)

#define SPRITE_MASK(msk, gfxmsk, size, sprite_bit, imgptr,          \
                    collmskptr, color, collmsk_return)              \
    _SPRITE_MASK(msk, gfxmsk, size, sprite_bit, imgptr, collmskptr, \
                 color, collmsk_return, SPRITE_PIXEL)

/* Multicolor sprites */
#define _MCSPRITE_MASK(mcmsk, gfxmsk, trmsk, size, sprite_bit, imgptr,   \
                       collmskptr, pixel_table, collmsk_return, DRAW)    \
    do {                                                                 \
        uint32_t __m;                                                       \
        int __p;                                                         \
                                                                         \
        for (__m = 1 << ((size) - 1), __p = 0;                           \
             __p < (size);                                               \
             __p += 2, __m >>= 2, (mcmsk) <<= 2, (trmsk) <<= 2) {        \
            uint8_t __c, __t;                                               \
                                                                         \
            __c = (uint8_t)(((mcmsk) >> 22) & 0x3);                         \
            __t = (uint8_t)(((trmsk) >> 22) & 0x3);                         \
                                                                         \
            if (__c) {                                                   \
                if (__t & 2) {                                           \
                    if ((gfxmsk) & __m) {                                \
                        DRAW(0, sprite_bit, imgptr, collmskptr, __p,     \
                             pixel_table[__c], collmsk_return);          \
                    } else {                                             \
                        DRAW(1, sprite_bit, imgptr, collmskptr, __p,     \
                             pixel_table[__c], collmsk_return);          \
                    }                                                    \
                }                                                        \
                                                                         \
                if (__t & 1) {                                           \
                    if ((gfxmsk) & (__m >> 1)) {                         \
                        DRAW(0, sprite_bit, imgptr, collmskptr, __p + 1, \
                             pixel_table[__c], collmsk_return);          \
                    } else {                                             \
                        DRAW(1, sprite_bit, imgptr, collmskptr, __p + 1, \
                             pixel_table[__c], collmsk_return);          \
                    }                                                    \
                }                                                        \
            }                                                            \
        }                                                                \
    } while (0)


#define MCSPRITE_MASK(mcmsk, gfxmsk, trmsk, size, sprite_bit, imgptr, \
                      collmskptr, pixel_table, collmsk_return)        \
    _MCSPRITE_MASK(mcmsk, gfxmsk, trmsk, size, sprite_bit, imgptr,    \
                   collmskptr, pixel_table, collmsk_return,           \
                   SPRITE_PIXEL)


#define _MCSPRITE_DOUBLE_MASK(mcmsk, gfxmsk, trmsk, size, sprite_bit, \
                              imgptr, collmskptr, pixel_table,        \
                              collmsk_return, DRAW)                   \
    do {                                                              \
        uint32_t __m;                                                    \
        int __p, __i;                                                 \
                                                                      \
        for (__m = 1U << ((size) - 1), __p = 0; __p < (size);         \
             __p += 4, (mcmsk) <<= 2, (trmsk) <<= 4) {                \
            uint8_t __c;                                                 \
            uint8_t __t;                                                 \
                                                                      \
            __c = (uint8_t)(((mcmsk) >> 22) & 0x3);                      \
            __t = (uint8_t)(((trmsk) >> (size - 4)) & 0xf);              \
                                                                      \
            for (__i = 0; __i < 4; __i++, __m >>= 1, __t <<= 1) {     \
                if (__c && (__t & 0x8)) {                             \
                    if ((gfxmsk) & __m) {                             \
                        DRAW(0, sprite_bit, imgptr, collmskptr,       \
                             __p + __i, pixel_table[__c],             \
                             collmsk_return);                         \
                    } else {                                          \
                        DRAW(1, sprite_bit, imgptr, collmskptr,       \
                             __p + __i, pixel_table[__c],             \
                             collmsk_return);                         \
                    }                                                 \
                }                                                     \
            }                                                         \
        }                                                             \
    } while (0)

#define MCSPRITE_DOUBLE_MASK(mcmsk, gfxmsk, trmsk, size, sprite_bit,        \
                             imgptr, collmskptr, pixel_table,               \
                             collmsk_return)                                \
    _MCSPRITE_DOUBLE_MASK(mcmsk, gfxmsk, trmsk, size, sprite_bit, imgptr,   \
                          collmskptr, pixel_table, collmsk_return,          \
                          SPRITE_PIXEL)


#define TRIM_MSK(msk, size)                                                 \
    do {                                                                    \
        int trm_msk_i;                                                              \
        int display_width = (MIN(sprite_xe + 1, size) - MAX(0, sprite_xs)); \
        msk = 0;                                                            \
        if (display_width > 0) {                                            \
            for (trm_msk_i = 0; trm_msk_i < display_width; trm_msk_i++) {                           \
                msk = (msk << 1) | 1;                                       \
            }                                                               \
            for (trm_msk_i = 0; trm_msk_i < size - sprite_xe - 1; trm_msk_i++) {                    \
                msk <<= 1;                                                  \
            }                                                               \
        }                                                                   \
    } while (0)


inline static void draw_hires_sprite_expanded(uint8_t *data_ptr, int n,
                                              uint8_t *msk_ptr, uint8_t *ptr,
                                              int lshift, uint8_t *sptr,
                                              raster_sprite_status_t *sprite_status,
                                              int sprite_xs, int sprite_xe)
{
    uint32_t sprmsk, collmsk;
    uint32_t trimmsk;
    uint16_t sbit = 0x101 << n;
    uint16_t cmsk = 0;

    int size = 48;
    int size1 = 32;
    int rest_of_repeat = 0;
    int must_repeat_pixels = 0;
    uint32_t repeat_pixel = 0;
    int i;
    int spritex_unwrapped = (sprite_status->sprites[n].x + vicii.sprite_wrap_x)
                            % vicii.sprite_wrap_x;

    sprmsk = (sprite_doubling_table[data_ptr[0]] << 16) | sprite_doubling_table[data_ptr[1]];


    if (spritex_unwrapped > SPRITE_EXPANDED_REPEAT_PIXELS_START(n)
        && spritex_unwrapped < SPRITE_REPEAT_PIXELS_END(n)) {
        /* sprite #n repeats pixel SPRITE_REPEAT_BEGIN(n)-1 up to SPRITE_REPEAT_BEGIN(n)+6 */
        /* no display from SPRITE_REPEAT_BEGIN(n)+7 to SPRITE_REPEAT_BEGIN(n)+11           */
        size = SPRITE_REPEAT_BEGIN(n) - spritex_unwrapped;
        must_repeat_pixels = (size > 0);
        if (size1 > size) {
            size1 = size;
        }

        if (must_repeat_pixels && size < 33) {
            size1 = size;
            sprmsk = (sprmsk >> (32 - size));
            repeat_pixel = sprmsk & 1;
            for (i = 0; i < 7 && size1 < 32; size1++, i++) {
                sprmsk = (sprmsk << 1) | repeat_pixel;
            }
            rest_of_repeat = 7 - i;
        }
    }

    collmsk = ((((msk_ptr[1] << 24) | (msk_ptr[2] << 16)
                 | (msk_ptr[3] << 8) | msk_ptr[4]) << lshift)
               | (msk_ptr[5] >> (8 - lshift)));

    collmsk = (collmsk >> (32 - size1));

    /* trim the masks regarding the display interval */
    TRIM_MSK(trimmsk, size1);

    sprmsk &= trimmsk;
    collmsk &= trimmsk;

    cmsk = 0;
    if (sprmsk & collmsk) {
        sprite_status->sprite_background_collisions |= sbit;
    }
    if (sprite_status->sprites[n].in_background) {
        SPRITE_MASK(sprmsk, collmsk, size1, sbit, ptr, sptr,
                    sprite_status->sprites[n].color, cmsk);
    } else {
        SPRITE_MASK(sprmsk, 0, size1, sbit, ptr, sptr,
                    sprite_status->sprites[n].color, cmsk);
    }

    size1 = size - size1;
    sprmsk = sprite_doubling_table[data_ptr[2]];

    if (must_repeat_pixels) {
        size1 = 0;
        if (size > 32) {
            sprmsk = sprmsk >> (48 - size);
            repeat_pixel = sprmsk & 1;
            rest_of_repeat = 7;
            size1 = size - 32;
        }
        for (i = 0; i < rest_of_repeat; i++) {
            sprmsk = (sprmsk << 1) | repeat_pixel;
        }
        size1 += rest_of_repeat;
    }

    collmsk = ((((msk_ptr[5] << 16) | (msk_ptr[6] << 8)
                 | msk_ptr[7]) << lshift)
               | (msk_ptr[8] >> (8 - lshift)));

    collmsk = (collmsk >> (24 - size1));

    /* trim the masks regarding the display interval */
    sprite_xe -= 32;
    sprite_xs -= 32;

    TRIM_MSK(trimmsk, size1);

    sprmsk &= trimmsk;
    collmsk &= trimmsk;

    if (sprmsk & collmsk) {
        sprite_status->sprite_background_collisions |= sbit;
    }
    if (sprite_status->sprites[n].in_background) {
        SPRITE_MASK(sprmsk, collmsk, size1, sbit, ptr + 32, sptr + 32,
                    sprite_status->sprites[n].color, cmsk);
    } else {
        SPRITE_MASK(sprmsk, 0, size1, sbit, ptr + 32, sptr + 32,
                    sprite_status->sprites[n].color, cmsk);
    }
    if (cmsk) {
        sprite_status->sprite_sprite_collisions
            |= (cmsk >> 8) | ((cmsk | sbit) & 0xff);
    }
}

inline static void draw_hires_sprite_normal(uint8_t *data_ptr, int n,
                                            uint8_t *msk_ptr, uint8_t *ptr,
                                            int lshift, uint8_t *sptr,
                                            raster_sprite_status_t *sprite_status,
                                            int sprite_xs, int sprite_xe)
{
    uint32_t sprmsk, collmsk;
    uint32_t trimmsk;
    uint8_t sbit = 1 << n;
    uint8_t cmsk = 0;

    int size = 24;
    int must_repeat_pixels = 0;
    uint32_t repeat_pixel;
    int i;
    int spritex_unwrapped = (sprite_status->sprites[n].x + vicii.sprite_wrap_x)
                            % vicii.sprite_wrap_x;

    sprmsk = (data_ptr[0] << 16) | (data_ptr[1] << 8) | data_ptr[2];

    if (spritex_unwrapped > SPRITE_NORMAL_REPEAT_PIXELS_START(n)
        && spritex_unwrapped < SPRITE_REPEAT_PIXELS_END(n)) {
        /* sprite #n repeats pixel SPRITE_REPEAT_BEGIN(n)-1 up to SPRITE_REPEAT_BEGIN(n)+6 */
        /* no display from SPRITE_REPEAT_BEGIN(n)+7 to SPRITE_REPEAT_BEGIN(n)+11           */
        size = SPRITE_REPEAT_BEGIN(n) - spritex_unwrapped;
        must_repeat_pixels = (size > 0);

        if (must_repeat_pixels) {
            sprmsk = (sprmsk >> (24 - size));
            repeat_pixel = sprmsk & 1;
            for (i = 0; i < 7; i++) {
                sprmsk = (sprmsk << 1) | repeat_pixel;
            }
            size += 7;
        }
    }

    collmsk = ((((msk_ptr[1] << 24) | (msk_ptr[2] << 16)
                 | (msk_ptr[3] << 8) | msk_ptr[4]) << lshift)
               | (msk_ptr[5] >> (8 - lshift)));

    collmsk = (collmsk >> (32 - size));

    /* trim the masks regarding the display interval */
    TRIM_MSK(trimmsk, size);

    sprmsk &= trimmsk;
    collmsk &= trimmsk;

    if (sprmsk & collmsk) {
        sprite_status->sprite_background_collisions |= sbit;
    }
    if (sprite_status->sprites[n].in_background) {
        SPRITE_MASK(sprmsk, collmsk, size, sbit, ptr, sptr,
                    sprite_status->sprites[n].color, cmsk);
    } else {
        SPRITE_MASK(sprmsk, 0, size, sbit, ptr, sptr,
                    sprite_status->sprites[n].color, cmsk);
    }
    if (cmsk) {
        sprite_status->sprite_sprite_collisions |= cmsk | sbit;
    }
}



/* Draw one hires sprite.  */
inline static void draw_hires_sprite(uint8_t *gfx_msk_ptr, uint8_t *data_ptr, int n,
                                     uint8_t *msk_ptr, uint8_t *ptr,
                                     int lshift, uint8_t *sptr,
                                     raster_sprite_status_t *sprite_status,
                                     int sprite_xs, int sprite_xe)
{
    if (sprite_status->sprites[n].x_expanded) {
        draw_hires_sprite_expanded(data_ptr, n, msk_ptr, ptr, lshift, sptr,
                                   sprite_status, sprite_xs, sprite_xe);
    } else {
        draw_hires_sprite_normal(data_ptr, n, msk_ptr, ptr, lshift, sptr,
                                 sprite_status, sprite_xs, sprite_xe);
    }
}

inline static void draw_mc_sprite_expanded(uint8_t *data_ptr, int n, uint32_t *c,
                                           uint8_t *msk_ptr, uint8_t *ptr,
                                           int lshift, uint8_t *sptr,
                                           raster_sprite_status_t *sprite_status,
                                           int sprite_xs, int sprite_xe)
{
    uint32_t mcsprmsk, sprmsk, collmsk;
    uint32_t trimmsk;
    uint32_t repeat_pixel = 0;
    int size = 0;
    int must_repeat_pixels = 0;
    int size_is_odd = 0;    /* which means size%4==1 in this case */
    int repeat_offset = 0;
    int shift_sprmsk;
    int delayed_shift, delayed_load;
    uint8_t cmsk = 0, sbit = 1 << n;
    uint8_t data0, data1;
    int trim_size;
    int spritex_unwrapped = (sprite_status->sprites[n].x + vicii.sprite_wrap_x)
                            % vicii.sprite_wrap_x;

    mcsprmsk = (data_ptr[0] << 16) | (data_ptr[1] << 8) | data_ptr[2];
    collmsk = ((((msk_ptr[1] << 24) | (msk_ptr[2] << 16)
                 | (msk_ptr[3] << 8) | msk_ptr[4]) << lshift)
               | (msk_ptr[5] >> (8 - lshift)));

    sprmsk = (sprite_doubling_table[mcsprtable[data_ptr[0]]] << 16)
             | sprite_doubling_table[mcsprtable[data_ptr[1]]];

    trim_size = 32;

    delayed_shift = (sprite_status->sprites[n].mc_bug >> 1);
    delayed_load = (sprite_status->sprites[n].mc_bug & 1);

    /* Fixes for the MC bug */
    if (delayed_shift) {
        ptr += 2;
        sptr += 2;
        trim_size += 2;
        mcsprmsk <<= 1;
        collmsk = (collmsk << 2 ) | (((msk_ptr[5] << 8) | msk_ptr[6]) >> (14 - lshift));
        data0 = (data_ptr[0] << 1) | (data_ptr[1] >> 7);
        data1 = (data_ptr[1] << 1);
        sprmsk = (sprite_doubling_table[mcsprtable[data0]] << 16) | sprite_doubling_table[mcsprtable[data1]];
    }

    if (delayed_load) {
        mcsprmsk &= ~(1 << (22 - (sprite_xs >> 1) + delayed_shift));
    }

    /* Fixes for the "Repeated pixels" bug */
    if (spritex_unwrapped > SPRITE_EXPANDED_REPEAT_PIXELS_START(n)
        && spritex_unwrapped < SPRITE_REPEAT_PIXELS_END(n)) {
        /* sprite #n repeats pixel SPRITE_REPEAT_BEGIN(n)-1 up to SPRITE_REPEAT_BEGIN(n)+6 */
        /* no display from SPRITE_REPEAT_BEGIN(n)+7 to SPRITE_REPEAT_BEGIN(n)+11           */
        size = SPRITE_REPEAT_BEGIN(n) - spritex_unwrapped;
        if (size < 0) {
            size = 0;
        }
        must_repeat_pixels = (size > 0);
        size_is_odd = ((size & 3) == 1) ? 1 : 0;
        shift_sprmsk = 24 + size_is_odd - ((size + 3) / 4) * 2;
        repeat_offset = (size & 1) + (((size & 3) == 2) ? 2 : 0);

        mcsprmsk = (mcsprmsk >> shift_sprmsk);
        repeat_pixel = mcsprmsk & 3;
        mcsprmsk = (mcsprmsk << shift_sprmsk);
    }

    /* trim the masks regarding the display interval */
    TRIM_MSK(trimmsk, trim_size);

    collmsk &= trimmsk;

    if (sprmsk & collmsk) {
        sprite_status->sprite_background_collisions |= sbit;
    }

    if (sprite_status->sprites[n].in_background) {
        MCSPRITE_DOUBLE_MASK(mcsprmsk, collmsk, trimmsk, 32, sbit, ptr, sptr, c, cmsk);
    } else {
        MCSPRITE_DOUBLE_MASK(mcsprmsk, 0, trimmsk, 32,
                             sbit, ptr, sptr, c, cmsk);
    }

    sprmsk = sprite_doubling_table[mcsprtable[data_ptr[2]]];
    collmsk = ((((msk_ptr[5] << 8) | msk_ptr[6]) << lshift) | (msk_ptr[7] >> (8 - lshift)));

    trim_size = 16;

    if (delayed_shift) {
        trim_size += 2;
        sprmsk = sprite_doubling_table[mcsprtable[(data_ptr[2] << delayed_shift) & 0xff]];
        collmsk = (collmsk << 2 ) | (((msk_ptr[7] << 8) | msk_ptr[8]) >> (14 - lshift));
    }

    /* trim the masks regarding the display interval */
    sprite_xe -= 32;
    sprite_xs -= 32;

    TRIM_MSK(trimmsk, trim_size);

    collmsk &= trimmsk;

    if (sprmsk & collmsk) {
        sprite_status->sprite_background_collisions |= sbit;
    }

    if (sprite_status->sprites[n].in_background) {
        MCSPRITE_DOUBLE_MASK(mcsprmsk, collmsk, trimmsk, 16,
                             sbit, ptr + 32, sptr + 32, c, cmsk);
    } else {
        MCSPRITE_DOUBLE_MASK(mcsprmsk, 0, trimmsk, 16, sbit,
                             ptr + 32, sptr + 32, c, cmsk);
    }

    if (must_repeat_pixels) {
        /* display the repeated pixel via HIRES sprite functions */
        uint32_t repeat_color;
        uint32_t special_sprmsk = 0;
        int i;

        if (size_is_odd) {
            repeat_pixel = (repeat_pixel & 1) << 1;
        } else {
            repeat_pixel = repeat_pixel & 3;
        }

        for (i = 0; i < 7; i++) {
            special_sprmsk = (special_sprmsk << 1) | (repeat_pixel ? 1 : 0);
        }

        repeat_color = c[repeat_pixel];

        /* trim the masks regarding the display interval */
        sprite_xe -= (size + repeat_offset - 32);
        sprite_xs -= (size + repeat_offset - 32);

        TRIM_MSK(trimmsk, 7 - repeat_offset);

        special_sprmsk &= trimmsk;

        SPRITE_MASK(special_sprmsk, 0,
                    7 - repeat_offset, sbit,
                    ptr + size + repeat_offset,
                    sptr + size + repeat_offset,
                    repeat_color, cmsk);

        /* this may cause a 'self-collision'; delete it */
        if (cmsk == sbit) {
            cmsk = 0;
        }
    }

    if (cmsk) {
        sprite_status->sprite_sprite_collisions |= cmsk | (sbit);
    }
}

inline static void draw_mc_sprite_normal(uint8_t *data_ptr, int n, uint32_t *c,
                                         uint8_t *msk_ptr, uint8_t *ptr,
                                         int lshift, uint8_t *sptr,
                                         raster_sprite_status_t *sprite_status,
                                         int sprite_xs, int sprite_xe)
{
    uint32_t mcsprmsk, sprmsk, collmsk;
    uint32_t trimmsk;
    uint32_t repeat_pixel = 0;
    int size = 0;
    int size_is_odd = 0;
    int must_repeat_pixels = 0;
    uint8_t cmsk = 0, sbit = 1 << n;
    int i;
    int delayed_shift;
    uint8_t data0, data1, data2;
    int trim_size;
    int spritex_unwrapped = (sprite_status->sprites[n].x + vicii.sprite_wrap_x)
                            % vicii.sprite_wrap_x;

    mcsprmsk = (data_ptr[0] << 16) | (data_ptr[1] << 8) | data_ptr[2];
    collmsk = ((((msk_ptr[0] << 24) | (msk_ptr[1] << 16)
                 | (msk_ptr[2] << 8) | msk_ptr[3]) << lshift)
               | (msk_ptr[4] >> (8 - lshift)));
    sprmsk = ((mcsprtable[data_ptr[0]] << 16)
              | (mcsprtable[data_ptr[1]] << 8)
              | mcsprtable[data_ptr[2]]);

    trim_size = 24;

    delayed_shift = (sprite_status->sprites[n].mc_bug >> 1);

    /* Fixes for the MC bug */
    if (delayed_shift) {
        ptr++;
        sptr++;
        trim_size++;
        mcsprmsk <<= 1;
        collmsk = (collmsk << 1 ) | (((msk_ptr[4] << 8) | msk_ptr[6]) >> (15 - lshift));
        data0 = (data_ptr[0] << 1) | (data_ptr[1] >> 7);
        data1 = (data_ptr[1] << 1) | (data_ptr[2] >> 7);
        data2 = (data_ptr[2] << 1);
        sprmsk = ((mcsprtable[data0] << 16) | (mcsprtable[data1] << 8) | mcsprtable[data2]);
    }

    if (spritex_unwrapped > SPRITE_NORMAL_REPEAT_PIXELS_START(n)
        && spritex_unwrapped < SPRITE_REPEAT_PIXELS_END(n)) {
        /* sprite #n repeats pixel SPRITE_REPEAT_BEGIN(n)-1 up to SPRITE_REPEAT_BEGIN(n)+6 */
        /* no display from SPRITE_REPEAT_BEGIN(n)+7 to SPRITE_REPEAT_BEGIN(n)+11           */
        size = SPRITE_REPEAT_BEGIN(n) - spritex_unwrapped;
        if (size < 0) {
            size = 0;
        }
        must_repeat_pixels = (size > 0);
        size_is_odd = size & 1;
        mcsprmsk = (mcsprmsk >> (24 - size));
        repeat_pixel = mcsprmsk & 3;
        mcsprmsk = (mcsprmsk << (24 - size));
    }

    /* trim the masks regarding the display interval */
    TRIM_MSK(trimmsk, trim_size);

    collmsk &= trimmsk;

    if (sprmsk & collmsk) {
        sprite_status->sprite_background_collisions |= sbit;
    }

    if (sprite_status->sprites[n].in_background) {
        MCSPRITE_MASK(mcsprmsk, collmsk, trimmsk, 24, sbit, ptr, sptr, c, cmsk);
    } else {
        MCSPRITE_MASK(mcsprmsk, 0, trimmsk, 24, sbit, ptr, sptr, c, cmsk);
    }

    if (must_repeat_pixels) {
        /* display the repeated pixel via HIRES sprite functions */
        uint32_t repeat_color;
        uint32_t special_sprmsk = 0;

        if (size_is_odd) {
            repeat_pixel = (repeat_pixel & 1) << 1;
        } else {
            repeat_pixel = repeat_pixel & 3;
        }

        for (i = 0; i < 7; i++) {
            special_sprmsk = (special_sprmsk << 1) | (repeat_pixel ? 1 : 0);
        }

        repeat_color = c[repeat_pixel];

        /* trim the masks regarding the display interval */
        sprite_xe -= (size + size_is_odd);
        sprite_xs -= (size + size_is_odd);

        TRIM_MSK(trimmsk, 7 - size_is_odd);

        special_sprmsk &= trimmsk;

        SPRITE_MASK(special_sprmsk, 0, 7 - size_is_odd, sbit,
                    ptr + size + size_is_odd, sptr + size + size_is_odd,
                    repeat_color, cmsk);
    }

    if (cmsk) {
        sprite_status->sprite_sprite_collisions |= cmsk | (sbit);
    }
}


/* Draw one multicolor sprite.  */
inline static void draw_mc_sprite(uint8_t *gfx_msk_ptr, uint8_t *data_ptr, int n,
                                  uint8_t *msk_ptr, uint8_t *ptr, int lshift,
                                  uint8_t *sptr,
                                  raster_sprite_status_t *sprite_status,
                                  int sprite_xs, int sprite_xe)
{
    uint32_t c[4];

    c[1] = sprite_status->mc_sprite_color_1;
    c[2] = sprite_status->sprites[n].color;
    c[3] = sprite_status->mc_sprite_color_2;

    if (sprite_status->sprites[n].x_expanded) {
        draw_mc_sprite_expanded(data_ptr, n, c, msk_ptr, ptr, lshift, sptr,
                                sprite_status, sprite_xs, sprite_xe);
    } else {
        draw_mc_sprite_normal(data_ptr, n, c, msk_ptr, ptr, lshift, sptr,
                              sprite_status, sprite_xs, sprite_xe);
    }
}

static inline void calculate_idle_sprite_data(uint8_t *data, unsigned int n)
{
    unsigned int i, line, cycle, idle_cycle;

    data[0] = 0xff;
    data[2] = 0xff;

    if (vicii.num_idle_3fff > 0) {
        idle_cycle = (57 + n * 2) % vicii.cycles_per_line;
        for (i = vicii.num_idle_3fff; i != 0; i--) {
            line = VICII_RASTER_Y(vicii.idle_3fff[i - 1].cycle);
            cycle = VICII_RASTER_CYCLE(vicii.idle_3fff[i - 1].cycle);
            if (line <= vicii.raster.current_line && cycle <= idle_cycle) {
                data[1] = vicii.idle_3fff[i - 1].value;
                return;
            }
        }
    }

    if (vicii.num_idle_3fff_old > 0) {
        idle_cycle = (57 + n * 2) % vicii.cycles_per_line;
        for (i = vicii.num_idle_3fff_old; i != 0; i--) {
            line = VICII_RASTER_Y(vicii.idle_3fff_old[i - 1].cycle);
            cycle = VICII_RASTER_CYCLE(vicii.idle_3fff_old[i - 1].cycle);
            if (line <= vicii.raster.current_line && cycle <= idle_cycle) {
                data[1] = vicii.idle_3fff_old[i - 1].value;
                return;
            }
        }
    }

    data[1] = vicii.ram_base_phi2[vicii.vbank_phi2 + 0x3fff];
}


static void draw_sprite_partial(uint8_t *line_ptr, uint8_t *gfx_msk_ptr,
                                int sprite_xs, int sprite_xe,
                                raster_sprite_status_t *sprite_status,
                                int n, int sprite_offset)
{
    uint8_t *data_ptr = NULL;

    if (sprite_status->dma_msk & (1 << n)
        && sprite_offset < SPRITE_DISPLAY_PREVIOUS_PATTERN) {
        /* display sprite data fetched in the previous line */
        data_ptr = (uint8_t *)(sprite_status->sprite_data + n);
    } else {
        if (sprite_status->new_dma_msk & (1 << n)) {
            if (sprite_offset >= SPRITE_DISPLAY_PREVIOUS_PATTERN) {
                /* sprite display starts immediately
                   without sprite data fetched */
                data_ptr = (uint8_t *)(sprite_status->sprite_data + n);
                if ((sprite_status->dma_msk & (1 << n)) == 0) {
                    calculate_idle_sprite_data(data_ptr, n);
                }
            }

            if (sprite_offset > SPRITE_DISPLAY_IMMEDIATE_DATA_FETCHED(n)) {
                /* Sprite starts immediately with data already
                   fetched */
                data_ptr = (uint8_t *)(sprite_status->new_sprite_data + n);
            }
        }
    }

    if (data_ptr != NULL) {
        uint8_t *msk_ptr, *ptr, *sptr;
        int lshift;

        msk_ptr = gfx_msk_ptr
                  + (VICII_MAX_SPRITE_WIDTH + sprite_offset
                     - VICII_RASTER_X(0) - vicii.raster.sprite_xsmooth) / 8;
        ptr = line_ptr + sprite_offset;
        lshift = (sprite_offset - vicii.raster.sprite_xsmooth) & 0x7;
        sptr = sprline - VICII_RASTER_X(0) + sprite_offset;

        if (sprite_status->sprites[n].multicolor) {
            draw_mc_sprite(gfx_msk_ptr, data_ptr, n, msk_ptr, ptr,
                           lshift, sptr, sprite_status,
                           sprite_xs, sprite_xe);
        } else {
            draw_hires_sprite(gfx_msk_ptr, data_ptr, n, msk_ptr, ptr,
                              lshift, sptr, sprite_status,
                              sprite_xs, sprite_xe);
        }
    }
}

/*
    draw sprites for part of a scanline. make sure not to draw outside the
    actually visible part of the line, see note below.
*/
static void draw_all_sprites_partial(uint8_t *line_ptr, uint8_t *gfx_msk_ptr,
                                     int xs, int xe)
{
    raster_sprite_status_t *sprite_status;
    int sprite_offset;
    int sprite_xs, sprite_xe;

    sprite_status = vicii.raster.sprite_status;

    if (sprite_status->dma_msk || sprite_status->new_dma_msk) {
        int n;

        for (n = 0; n < 8; n++) {
            if (sprite_status->sprites[n].x < vicii.sprite_wrap_x) {
                sprite_offset = sprite_status->sprites[n].x + sprite_status->sprites[n].x_shift;

                sprite_xs = xs - sprite_offset;
                sprite_xe = xe - sprite_offset;

                /* Test if the sprite is inside the area.                  */
                /* Sprite can be seven pixels wider with repeating pixels. */
                if (sprite_xe >= 0
                    && sprite_xs <
                    (sprite_status->sprites[n].x_expanded ? 24 : 0) + X_OFFSET - 1) {
                    draw_sprite_partial(line_ptr, gfx_msk_ptr,
                                        sprite_xs, sprite_xe, sprite_status, n, sprite_offset);
                }

                /* Now shift the interval one screen left */
                /* to draw the wrapped part of the sprite */
                sprite_xs += vicii.sprite_wrap_x;
                sprite_xe += vicii.sprite_wrap_x;
                sprite_offset -= vicii.sprite_wrap_x;

                if (sprite_xe >= 0
                    && sprite_xs <
                    (sprite_status->sprites[n].x_expanded ? 24 : 0) + X_OFFSET - 1) {
                    draw_sprite_partial(line_ptr, gfx_msk_ptr,
                                        sprite_xs, sprite_xe, sprite_status, n, sprite_offset);
                }
            }

            sprite_status->sprites[n].mc_bug = 0;
        }

        vicii.sprite_sprite_collisions
            |= sprite_status->sprite_sprite_collisions;
        vicii.sprite_background_collisions
            |= sprite_status->sprite_background_collisions;
    }
}

/*
    draw all sprites for a complete line.

    NOTE: make sure not to draw more than the actually visible part of the line,
    because also only that part will get overdrawn by the border color and
    excessive pixels will show up as artefacts in renderers which rely on the
    offscreen area properly being updated (such as Scale2x and CRT emulation).
 */
static void draw_all_sprites(uint8_t *line_ptr, uint8_t *gfx_msk_ptr)
{
/*
    draw_all_sprites_partial(line_ptr, gfx_msk_ptr,
                    VICII_RASTER_X(0),
                    vicii.cycles_per_line * 8 + VICII_RASTER_X(0) - 1);
*/
    draw_all_sprites_partial(line_ptr, gfx_msk_ptr,
                             VICII_RASTER_X(0) + vicii.raster.geometry->extra_offscreen_border_left,
                             VICII_RASTER_X(0) + vicii.raster.geometry->extra_offscreen_border_left +
                             (vicii.raster.geometry->screen_size.width - 1));
}

static void update_cached_sprite_collisions(raster_cache_t *cache)
{
    vicii.sprite_sprite_collisions |= cache->sprite_sprite_collisions;
    vicii.sprite_background_collisions |= cache->sprite_background_collisions;
}

void vicii_sprites_init(void)
{
    init_drawing_tables();

    raster_sprite_status_set_draw_function(vicii.raster.sprite_status,
                                           draw_all_sprites);

    raster_sprite_status_set_cache_function(vicii.raster.sprite_status,
                                            update_cached_sprite_collisions);

    raster_sprite_status_set_draw_partial_function(vicii.raster.sprite_status,
                                                   draw_all_sprites_partial);
    return;
}

/* Set the X coordinate of the `num'th sprite to `new_x'; the current
   vicii.raster X position is `raster_x'.  */
void vicii_sprites_set_x_position(unsigned int num, int new_x, int raster_x)
{
    raster_sprite_t *sprite;
    int x_offset;
    int last_pos;
    int next_pos;
    int change_pos;

    sprite = vicii.raster.sprite_status->sprites + num;

    x_offset = vicii_sprite_offset();

    /* Handle spritegap in NTSC mode */
    if (vicii.sprite_wrap_x > 0x200 && (unsigned int)new_x > 0x187) {
        new_x += (vicii.sprite_wrap_x - 0x200);
    }

    new_x += x_offset;

    /* transfer coordinates to a new timeline starting with memory fetch to
       make calculation easier */
    next_pos = (new_x - SPRITE_DISPLAY_IMMEDIATE_DATA_FETCHED(num)
                + vicii.sprite_wrap_x) % vicii.sprite_wrap_x;
    last_pos = (sprite->x - SPRITE_DISPLAY_IMMEDIATE_DATA_FETCHED(num)
                + 2 * vicii.sprite_wrap_x) % vicii.sprite_wrap_x;
    change_pos = (raster_x + 8 - SPRITE_DISPLAY_IMMEDIATE_DATA_FETCHED(num)
                  + 2 * vicii.sprite_wrap_x) % vicii.sprite_wrap_x;

    /* disabled display is at the very end even on the transfered timeline */
    if (sprite->x == vicii.sprite_wrap_x) {
        last_pos = vicii.sprite_wrap_x;
    }

    if (new_x >= vicii.sprite_wrap_x + VICII_RASTER_X(0)) {
        /* Sprites in the $1F8 - $1FF range are not visible at all and never
           cause collisions.  */
        if (new_x >= vicii.sprite_wrap_x + x_offset) {
            new_x = vicii.sprite_wrap_x;
        } else {
            new_x -= vicii.sprite_wrap_x;
        }
    }

    if (next_pos < last_pos) {
        if (change_pos <= next_pos) {
            if (raster_x + 8 > new_x) {
                /* last line was already drawn */
                raster_changes_sprites_add_int(&vicii.raster,
                                               SPRITE_DISPLAY_IMMEDIATE_DATA_FETCHED(num),
                                               &sprite->x, new_x);
            } else {
                sprite->x = new_x;
            }
        } else {
            if (change_pos <= last_pos) {
                /* too early to start at last_pos and too late to start
                   at next_pos; disable display on this line */
                sprite->x = vicii.sprite_wrap_x;
            } else {
                /* display already started on last_pos, change on next fetch */
                if (raster_x + 8 < new_x && sprite->x > raster_x + 8) {
                    /* last line was already drawn */
                    sprite->x = new_x;
                } else {
                    raster_changes_sprites_add_int(&vicii.raster,
                                                   SPRITE_DISPLAY_IMMEDIATE_DATA_FETCHED(num),
                                                   &sprite->x, new_x);
                }
            }
        }
    } else {
        /* next_pos >= last_pos */
        if (change_pos <= last_pos) {
            if (raster_x + 8 > new_x) {
                /* last line was already drawn */
                raster_changes_sprites_add_int(&vicii.raster,
                                               SPRITE_DISPLAY_IMMEDIATE_DATA_FETCHED(num),
                                               &sprite->x, new_x);
            } else {
                sprite->x = new_x;
            }
        } else {
            if (change_pos >= next_pos) {
                if (raster_x + 8 < sprite->x && new_x > raster_x + 8) {
                    /* last line was already drawn */
                    sprite->x = new_x;
                } else {
                    /* display already started on last_pos, change on next fetch */
                    raster_changes_sprites_add_int(&vicii.raster, SPRITE_DISPLAY_IMMEDIATE_DATA_FETCHED(num), &sprite->x, new_x);
                }
            }
        }
    }
    raster_changes_sprites_add_int(&vicii.raster, SPRITE_DISPLAY_IMMEDIATE_DATA_FETCHED(num), &sprite->x, new_x);
}

void vicii_sprites_reset_xshift(void)
{
    unsigned int n;

    for (n = 0; n < 8; n++) {
        vicii.raster.sprite_status->sprites[n].x_shift = 0;
        vicii.raster.sprite_status->sprites[n].x_shift_sum = 0;
        vicii.raster.sprite_status->sprites[n].mc_bug = 0;
    }

    vicii.raster.sprite_status->sprite_sprite_collisions = 0;
    vicii.raster.sprite_status->sprite_background_collisions = 0;
}

void vicii_sprites_reset_sprline(void)
{
    memset(sprline, 0, vicii.sprite_wrap_x);
}

void vicii_sprites_init_sprline(void)
{
    sprline = lib_realloc(sprline, vicii.sprite_wrap_x);
}

void vicii_sprites_shutdown(void)
{
    lib_free(sprline);
}

int vicii_sprite_offset(void)
{
    return vicii.screen_leftborderwidth - 24;
}
