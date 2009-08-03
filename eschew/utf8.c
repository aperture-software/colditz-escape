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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "utf8.h"

static const int halfShift			= 10; /* used for shifting by 10 bits */
static const unsigned long halfBase = 0x0010000UL;
static const unsigned long halfMask = 0x3FFUL;

/*
 * Once the bits are split out into bytes of UTF-8, this is a mask OR-ed
 * into the first byte, depending on how many bytes follow.  There are
 * as many entries in this table as there are UTF-8 sequence types.
 * (I.e., one byte sequence, two byte... etc.). Remember that sequencs
 * for *legal* UTF-8 will be 4 or fewer bytes total.
 */
static const unsigned char firstByteMark[7] = { 
	0x00, 0x00, 0xC0, 0xE0, 0xF0, 0xF8, 0xFC };

static const char trailingBytesForUTF8[256] = {
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
    2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2, 3,3,3,3,3,3,3,3,4,4,4,4,5,5,5,5
};

static const unsigned long offsetsFromUTF8[6] = { 
	0x00000000, 0x00003080, 0x000E2080, 0x03C82080, 0xFA082080, 0x82082080 };


/*
 * Checks UTF8 validity. Returns -1 if valid, 0 otherwise
 */
static int isLegalUTF8(const char *source, int length) 
{
    unsigned char a;
	/* Make sure we deal with UNSIGNED chars at all times */
	const unsigned char *src = (unsigned char*)source;
    const unsigned char *srcptr = src+length;
    switch (length) {
    default: return 0;
	/* Everything else falls through when "true"... */
    case 4: if ((a = (*--srcptr)) < 0x80 || a > 0xBF) return 0;
    case 3: if ((a = (*--srcptr)) < 0x80 || a > 0xBF) return 0;
    case 2: if ((a = (*--srcptr)) > 0xBF) return 0;

	switch (*src) {
	    /* no fall-through in this inner switch */
	    case 0xE0: if (a < 0xA0) return 0; break;
	    case 0xED: if (a > 0x9F) return 0; break;
	    case 0xF0: if (a < 0x90) return 0; break;
	    case 0xF4: if (a > 0x8F) return 0; break;
	    default:   if (a < 0x80) return 0;
	}

    case 1: if (*src >= 0x80 && *src < 0xC2) return 0;
    }
    if (*src > 0xF4) return 0;
    return -1;
}


/*
 * Convert a single UTF8 sequence to a 16 bit UTF16, for a non zero 
 * terminated string 'source' of length 'len'. Fill a 16 bit UTF16 in
 * '*target' and returns the number of bytes consumed.
 * If the target glyph is not valid Unicode, FFFF is returned
 * If the target glyph is valid but falls outside our scope, FFFD is returned
 *
 * Note that this is NOT a true UTF8 to UTF16 conversion function, as it 
 * only returns UTF16 chars in the range [0000-FFFF], whereas true UTF16
 * is [0000-10FFFF]
 */
int utf8_to_u16_nz(const char* source, unsigned short* target, size_t len) 
{
	unsigned long ch = 0;
	int extraBytesToRead;
	int pos = 0;
	/* To make sure we deal with UNSIGNED chars at all times */
	const unsigned char *src = (unsigned char*)source;

	if (source == NULL)
	{
		*target = 0;
		return 0;
	}

	/* Reading a \0 character here is fine */
	extraBytesToRead = trailingBytesForUTF8[src[pos]];

	/* Make sure we don't run out of stream... */
	if (len < (size_t) extraBytesToRead)
	{
		*target = 0xFFFF;	// NOT a Unicode character
		return extraBytesToRead+1;
	}

	/* Do this check whether lenient or strict */
	if (!isLegalUTF8(source, extraBytesToRead+1)) 
	{
		*target = 0xFFFF;	/* NOT a Unicode character */
	    return 1;			/* only discard the first UTF-8 byte */
	}

	switch (extraBytesToRead) {
	    case 5: ch += src[pos++]; ch <<= 6; /* remember, illegal UTF-8 */
	    case 4: ch += src[pos++]; ch <<= 6; /* remember, illegal UTF-8 */
	    case 3: ch += src[pos++]; ch <<= 6;
	    case 2: ch += src[pos++]; ch <<= 6;
	    case 1: ch += src[pos++]; ch <<= 6;
	    case 0: ch += src[pos++];
	}
	ch -= offsetsFromUTF8[extraBytesToRead];

	if (ch < 0xFFFC) 
		*target = (unsigned short)ch;
	else
		*target = 0xFFFD;	/* replacement character */

	return extraBytesToRead + 1;
}


/*
 * Same as above, for NULL terminated strings
 */
int utf8_to_u16(const char* source, unsigned short* target) 
{
	return utf8_to_u16_nz(source, target, strlen(source));
}


/*
 * Converts the source_len characters UTF-16 string in source to a null terminated UTF-8 string in target
 * Returns the number of characters read from source. If this number is different from source_len, there
 * was an error
*/
size_t utf16_to_utf8(const unsigned int* source, const size_t source_len, 
				  char* targetc, size_t max_target_len) 
{
	size_t source_pos = 0;
	size_t target_pos = 0;
	unsigned long ch = 0;
	unsigned long ch2 = 0;
	unsigned short bytesToWrite = 0;
	const unsigned long byteMask = 0xBF;
	const unsigned long byteMark = 0x80;
	const int flags = strictConversion;
	unsigned char *target = (unsigned char*)targetc;

    while (source_pos < source_len) 
	{
		ch = source[source_pos++];
		/* If we have a surrogate pair, convert to UTF32 first. */
		if (ch >= UNI_SUR_HIGH_START && ch <= UNI_SUR_HIGH_END) 
		{
			/* If the 16 bits following the high surrogate are in the source buffer... */
			if (source_pos < source_len) 
			{
				ch2 = source[source_pos];
				/* If it's a low surrogate, convert to UTF32. */
				if (ch2 >= UNI_SUR_LOW_START && ch2 <= UNI_SUR_LOW_END) 
				{
					ch = ((ch - UNI_SUR_HIGH_START) << halfShift) +
						(ch2 - UNI_SUR_LOW_START) + halfBase;
					++source_pos;
				} else if (flags == strictConversion) 
				{ /* it's an unpaired high surrogate */
					--source_pos; /* return to the illegal value itself */
					break;
				}
			} 
			else 
			{ /* We don't have the 16 bits following the high surrogate. */
				--source_pos; /* return to the high surrogate */
				break;
			}
		} 
		else if (flags == strictConversion) 
		{
			/* UTF-16 surrogate values are illegal in UTF-32 */
			if (ch >= UNI_SUR_LOW_START && ch <= UNI_SUR_LOW_END) 
			{
				--source_pos; /* return to the illegal value itself */
				break;
			}
		}
		/* Figure out how many bytes the result will require */
		if (ch < (unsigned long)0x80) 
			bytesToWrite = 1;
		else if (ch < (unsigned long)0x800) 
			bytesToWrite = 2;
		else if (ch < (unsigned long)0x10000) 
			bytesToWrite = 3;
		else if (ch < (unsigned long)0x110000) 
			bytesToWrite = 4;
		else 
		{
			bytesToWrite = 3;
			ch = 0xFFFD;		/* Replacement char */
		}

		target_pos += bytesToWrite;
		if (target_pos >= max_target_len) 
		{
			break;
		}
		switch (bytesToWrite) 
		{ /* note: everything falls through. */
			case 4: target[--target_pos] = (unsigned char)((ch | byteMark) & byteMask); ch >>= 6;
			case 3: target[--target_pos] = (unsigned char)((ch | byteMark) & byteMask); ch >>= 6;
			case 2: target[--target_pos] = (unsigned char)((ch | byteMark) & byteMask); ch >>= 6;
			case 1: target[--target_pos] = (unsigned char) (ch | firstByteMark[bytesToWrite]);
		}
		target_pos += bytesToWrite;
    }
	target[target_pos] = 0;
    return source_pos;
}


