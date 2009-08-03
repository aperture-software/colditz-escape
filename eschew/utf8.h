/*
 *  Eschew - Even Simpler C-Heuristic Expat Wrapper
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
 * 	UTF-8 to UTF-16 conversion functions, adapted from unicode.org
 *  See http://unicode.org/Public/PROGRAMS/CVTUTF/ConvertUTF.c and
 *      http://unicode.org/Public/PROGRAMS/CVTUTF/ConvertUTF.h
 *  ---------------------------------------------------------------------------
 */

#pragma once

#ifdef	__cplusplus
extern "C" {
#endif

#define UNI_SUR_HIGH_START  (unsigned long)0xD800
#define UNI_SUR_HIGH_END    (unsigned long)0xDBFF
#define UNI_SUR_LOW_START   (unsigned long)0xDC00
#define UNI_SUR_LOW_END     (unsigned long)0xDFFF

typedef enum {
	strictConversion = 0,
	lenientConversion
} ConversionFlags;


/*
	Function prototypes
 */
int utf8_to_u16_nz(const char* source, unsigned short* target, size_t len);
int utf8_to_u16(const char* source, unsigned short* target);

#ifdef	__cplusplus
}
#endif

