/**
 **  Escape from Colditz
 **
 **  Configuration helper functions (XML Parse, UTF-8 conversions, etc.)
 **
 **  UTF-8 to UTF-16 conversion adapted from unicode.org
 **  See http://unicode.org/Public/PROGRAMS/CVTUTF/ConvertUTF.c and
 **      http://unicode.org/Public/PROGRAMS/CVTUTF/ConvertUTF.h
 **
 **/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#if defined(WIN32)
#include <windows.h>
#include <gl/gl.h>
#include <gl/glu.h>
#include <gl/glut.h>
#elif defined(PSP)
#include <pspkernel.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glut.h>
#endif
#include "data-types.h"
#include "expat.h"
// Needs to be set before including conf.h
#define INIT_XML_ACTUAL_INIT
#include "conf.h"


#if defined(PSP)
#define SET_XML_NODE_DEFAULT(table, node_name, val1, val2) {			\
	for (i=0; i<_ ## table ## _end; i++) 								\
		if (strcmp(#node_name, table[i].name) == 0)						\
			table[i].value = val1; }
#else
#define SET_XML_NODE_DEFAULT(table, node_name, val1, val2) {			\
	for (i=0; i<_ ## table ## _end; i++) 								\
		if (strcmp(#node_name, table[i].name) == 0)						\
			table[i].value = val2; }
#endif




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

static const u32 offsetsFromUTF8[6] = { 0x00000000, 0x00003080, 0x000E2080, 
		     0x03C82080, 0xFA082080, 0x82082080 };


static bool isLegalUTF8(const char *source, int length) 
{
    unsigned char a;
	// Make sure we deal with UNSIGNED chars at all time
	const unsigned char *src = (unsigned char*)source;
    const unsigned char *srcptr = src+length;
    switch (length) {
    default: return false;
	// Everything else falls through when "true"...
    case 4: if ((a = (*--srcptr)) < 0x80 || a > 0xBF) return false;
    case 3: if ((a = (*--srcptr)) < 0x80 || a > 0xBF) return false;
    case 2: if ((a = (*--srcptr)) > 0xBF) return false;

	switch (*src) {
	    // no fall-through in this inner switch
	    case 0xE0: if (a < 0xA0) return false; break;
	    case 0xED: if (a > 0x9F) return false; break;
	    case 0xF0: if (a < 0x90) return false; break;
	    case 0xF4: if (a > 0x8F) return false; break;
	    default:   if (a < 0x80) return false;
	}

    case 1: if (*src >= 0x80 && *src < 0xC2) return false;
    }
    if (*src > 0xF4) return false;
    return true;
}


// Convert a single UTF8 sequence to a 16 bit UTF16, for a non zero 
// terminated string 'source' of length 'len'. Fill a 16 bit UTF16 in
// '*target' and returns the number of bytes consumed.
// If the target glyph is not valid Unicode, FFFF is returned
// If the target glyph is valid but falls outside our scope, FFFD is returned
// 
// Note that this is NOT a true UTF8 to UTF16 conversion function, as it 
// only returns UTF16 chars in the range [0000-FFFF], whereas true UTF16
// is [0000-10FFFF]
int utf8_to_u16_nz(const char* source, u16* target, size_t len) 
{
	unsigned long ch = 0;
	int extraBytesToRead;
	int pos = 0;
	// Make sure we deal with UNSIGNED chars at all time 
	const unsigned char *src = (unsigned char*)source;

	if (source == NULL)
	{
		*target = 0;
		return 0;
	}

	// Reading a \0 character here is fine
	extraBytesToRead = trailingBytesForUTF8[src[pos]];

	// Make sure we don't run out of stream...
	if (len < (size_t) extraBytesToRead)
	{
		*target = 0xFFFF;	// NOT a Unicode character
		return extraBytesToRead+1;
	}

	// Do this check whether lenient or strict 
	if (!isLegalUTF8(source, extraBytesToRead+1)) 
	{
		*target = 0xFFFF;	// NOT a Unicode character
	    return 1;			// only discard the first UTF-8 byte
	}

	switch (extraBytesToRead) {
	    case 5: ch += src[pos++]; ch <<= 6; // remember, illegal UTF-8 
	    case 4: ch += src[pos++]; ch <<= 6; // remember, illegal UTF-8 
	    case 3: ch += src[pos++]; ch <<= 6;
	    case 2: ch += src[pos++]; ch <<= 6;
	    case 1: ch += src[pos++]; ch <<= 6;
	    case 0: ch += src[pos++];
	}
	ch -= offsetsFromUTF8[extraBytesToRead];

	if (ch < 0xFFFC) 
		*target = ch & 0xFFFF;
	else
		*target = 0xFFFD;	// replacement character

	return extraBytesToRead + 1;
}


// Same as above, for NULL terminated strings
int utf8_to_u16(const char* source, u16* target) 
{
	size_t len;
	len = strlen(source);
	return utf8_to_u16_nz(source, target, len);
}

// Expat parsing of the XML conf file
static void XMLCALL characterData(void *userData, const char *s, int len)
{
	int i, consumed;
	u8* string = (u8*)s;
	u16 utf16;
	for (i=0; i<len; i++)
		switch(string[i])
		{
		// eliminate blanks
		case 0x20:
		case 0x0A:
			break;
		default:
			if (string[i] >= 0x80)
			{
				consumed = utf8_to_u16_nz(s+i, &utf16, len-i);
				printf("%02X", utf16);
				i += consumed;
			}
			else
				printf("%02X", string[i]);
			break;
		}
}

static void XMLCALL startElement(void *userData, const char *name, const char **atts)
{
  int i;
  int *depthPtr = (int *)userData;
  printf("\n");
  for (i = 0; i < *depthPtr; i++)
    putchar('\t');
  printf("%s ", name);
  *depthPtr += 1;
}

static void XMLCALL endElement(void *userData, const char *name)
{
  int *depthPtr = (int *)userData;
  *depthPtr -= 1;
}


int readconf(char* filename)
{
	// Expat variables
	char buf[BUFSIZ];
	int done;
	int depth = 0;
	XML_Parser parser; 
	FILE* fd;

	parser = XML_ParserCreate(NULL);
	XML_SetUserData(parser, &depth);
	XML_SetElementHandler(parser, startElement, endElement);
	XML_SetCharacterDataHandler(parser, characterData);

	if ((fd = fopen(filename, "rb")) == NULL)
	{
		printf("readconf: can't find '%s' - Aborting.\n", filename);
		return -1;
	}

	do 
	{
		int len = (int)fread(buf, 1, sizeof(buf), fd);
		done = len < sizeof(buf);
		if (XML_Parse(parser, buf, len, done) == XML_STATUS_ERROR) 
		{
			fprintf(stderr, "[%d] %s at line %lu\n", XML_GetErrorCode(parser),
				XML_ErrorString(XML_GetErrorCode(parser)),
				XML_GetCurrentLineNumber(parser));
			return 1;
		}
	} 
	while (!done);
	XML_ParserFree(parser);
	fclose(fd);

	return 0;
}


void init_controls()
{
	int i;
	// The second part of a node table init MUST occur at runtime
	INIT_XML_NODE_NAMES(controls);
	// Set defaults values. Can be skipped if relying on the external XML
	// The order is PSP, WIN
	SET_XML_NODE_DEFAULT(controls, key_fire, 'x', '5');
    SET_XML_NODE_DEFAULT(controls, key_toggle_walk_run, 'o', ' ');
    SET_XML_NODE_DEFAULT(controls, key_pause, 's', SPECIAL_KEY_F5);
    SET_XML_NODE_DEFAULT(controls, key_sleep, 'q', SPECIAL_KEY_F9);
    SET_XML_NODE_DEFAULT(controls, key_stooge, 'd', SPECIAL_KEY_F10);
    SET_XML_NODE_DEFAULT(controls, key_direction_left, 0, '4');
    SET_XML_NODE_DEFAULT(controls, key_direction_right, 0, '6');
    SET_XML_NODE_DEFAULT(controls, key_direction_up, 0, '8');
    SET_XML_NODE_DEFAULT(controls, key_direction_down, 0, '2');
    SET_XML_NODE_DEFAULT(controls, key_inventory_cycle_left, SPECIAL_KEY_LEFT, SPECIAL_KEY_LEFT);
    SET_XML_NODE_DEFAULT(controls, key_inventory_cycle_right, SPECIAL_KEY_RIGHT, SPECIAL_KEY_RIGHT);
    SET_XML_NODE_DEFAULT(controls, key_pickup, SPECIAL_KEY_UP, SPECIAL_KEY_UP);
    SET_XML_NODE_DEFAULT(controls, key_dropdown, SPECIAL_KEY_DOWN, SPECIAL_KEY_DOWN);
    SET_XML_NODE_DEFAULT(controls, key_escape, 'a', 0x1b);
    SET_XML_NODE_DEFAULT(controls, key_prisoners_cycle_left, SPECIAL_LEFT_MOUSE_BUTTON, 0);
    SET_XML_NODE_DEFAULT(controls, key_prisoners_cycle_right, SPECIAL_RIGHT_MOUSE_BUTTON, 0);
    SET_XML_NODE_DEFAULT(controls, key_select_british, 0, SPECIAL_KEY_F1);
    SET_XML_NODE_DEFAULT(controls, key_select_french, 0, SPECIAL_KEY_F2);
    SET_XML_NODE_DEFAULT(controls, key_select_american, 0, SPECIAL_KEY_F3);
    SET_XML_NODE_DEFAULT(controls, key_select_polish, 0, SPECIAL_KEY_F4);
	// Debug
	PRINT_XML_NODES(controls);
}