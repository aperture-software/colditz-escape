/*
 *  Colditz Escape! - Rewritten Engine for "Escape From Colditz"
 *  copyright (C) 2008-2009 Aperture Software 
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 *  ---------------------------------------------------------------------------
 *  anti-tampering.h: Optional anti tampering
 *  ---------------------------------------------------------------------------
 */

#pragma once

#ifdef	__cplusplus
extern "C" {
#endif


#if defined(ANTI_TAMPERING_ENABLED)
#include "md5.h"

// Slighltly obfuscated MD5 hashes of the files (don't want to make file tampering & cheating too easy for the first release)
#define FMDXHASHES				{ { 0xf3, 0x5e, 0x11, 0xb3, 0x30 }, \
								  { 0x6c, 0x31, 0x12, 0x00, 0xf2 }, \
								  { 0x6b, 0x85, 0x59, 0xc1, 0xdb }, \
								  { 0x4f, 0x7d, 0x68, 0x74, 0x3a }, \
								  { 0xc8, 0x87, 0x0a, 0x85, 0xd0 },	\
								  { 0x46, 0xc8, 0xf7, 0x61, 0x68 }, \
								  { 0x84, 0x1d, 0xc4, 0x82, 0xd8 }, \
								  { 0x03, 0xf0, 0xa7, 0xfc, 0xe3 }, \
								  { 0x98, 0x24, 0xf0, 0x26, 0xc8 }, \
								  { 0x60, 0xc1, 0xe3, 0x4a, 0xc0 }, \
								  { 0x59, 0x26, 0xc6, 0xbe, 0x68 }  }


// This inline performs an obfuscated MD5 check on file i
// Conveniently used to discreetly check for file tampering anytime during the game
extern void md5( unsigned char *input, int ilen, unsigned char output[16] );
extern u8  fmdxhash[NB_FILES][5];
static __inline bool integrity_check(u16 i)
{
	u8 md5hash[16];
	u8 blah = 0x5A;
	md5(fbuffer[i]+((i==LOADER)?LOADER_PADDING:0), fsize[i], md5hash);
//	printf("{ ");
	blah ^= md5hash[7];
//	printf("0x%02x, ", blah);
	if (blah != fmdxhash[i][0]) return false;
	blah ^= md5hash[12];
//	printf("0x%02x, ", blah);
	if (blah != fmdxhash[i][1]) return false;
	blah ^= md5hash[1];
//	printf("0x%02x, ", blah);
	if (blah != fmdxhash[i][2]) return false;
	blah ^= md5hash[13];
//	printf("0x%02x, ", blah);
	if (blah != fmdxhash[i][3]) return false;
	blah ^= md5hash[9];
//	printf("0x%02x }, \\\n", blah);
	if (blah != fmdxhash[i][4]) return false;
	return true;
}
#endif

#ifdef	__cplusplus
}
#endif
