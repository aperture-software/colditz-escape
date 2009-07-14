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

typedef struct {
	char*		name;
	s_xml_attr* attr;
} stack_el;

typedef struct {
    int skip;
    int depth;
	stack_el stack[256];
	// holder for the string conversion of the read value
	char value[256];
	int  index;
} Parseinfo;


#if defined(PSP)
#define SET_XML_NODE_DEFAULT2(table, node_name, val1, val2) 			\
	SET_XML_NODE_DEFAULT(table, node_name, val1)
#else
#define SET_XML_NODE_DEFAULT2(table, node_name, val1, val2) 			\
	SET_XML_NODE_DEFAULT(table, node_name, val2)
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


// Init our parsing user structure
void init_info(Parseinfo *info) {
    info->skip = 0;
    info->depth = 0;
	info->index = 0;
}
/*
xml_node* get_branch_node(Parseinfo *inf, int level)
{
	xml_node* node;
	if (level == 0)
	{
		if (strcmp(inf->stack[0].name, xml_root_name) == 0)
			return xml_root;
		else
			return NULL;
	}
	node = get_branch_node(inf, level-1);
	for (i=0; i<GET_NODE_COUNT(node); i++)
	{
		if (strcmp(inf->stack[0], xml_root_name)
	if (node == NULL)
		return NULL;


}
*/


// Skip nodes that are undefined
bool skip(Parseinfo *inf, const char *name, const char **attr) 
{
	int i;
/*
	if (inf->depth == 0)
		return (strcmp(name, xml_root_name) != 0);
	else
		getbranch(inf, name, inf->depth -1);
*/	
	switch (inf->depth) 
	{
	case 0:
	{
//		blah = GET_NODE_NAME(&xml_root, type_xml_node, 0);
//		return true;	
		// Check that the rootnode matches our definition
//		GET_NODE_NAME(ptr, xml_type, i) (char*)(((xml_node*)&xml_root)->nodes +			
//			0*(sizeof(char*) + xml_type_size[xml_type]))
		printf("got %s on root\n", GET_NODE_NAME(&xml_root, type_xml_node_ptr, 0));
		printf("xml_config[%d].name = %s. size = %d\n", controls, GET_NODE_NAME(&xml_config, type_xml_node_ptr, 2), s_xml_el_size[type_xml_node_ptr]);

		exit(0);
		return (strcmp(name, GET_NODE_NAME(&xml_root, type_xml_node_ptr, 0)) != 0);
	}
/*	case 1:
		// Ignore any 1st level nodename that is not in our 1st level table
		for (i=0; i<GET_NODE_COUNT(config); i++)
			// xml_config[] is the name of our root node table
			if (strcmp(name, NODE(config, i).name) == 0)
				return false;
		return true;
*/	default:
		return false;
	}
}


// Parsing of the node values
static void XMLCALL characterData(void *userData, const char *s, int len)
{
	Parseinfo *inf = (Parseinfo *) userData;
	int i, consumed;
	u8* str = (u8*)s;
	u16 utf16;
	if (inf->skip)
		return;
	for (i=0; i<len; i++)
		switch(str[i])
		{
		// eliminate blanks
		case 0x20:
		case 0x0A:
			// TO_DO: smart elimination of whitespaces
			if (inf->index == 0)
			{
				inf->value[0] = i;
				break;
			}
		// copy the string data in our info structure
		default:
			inf->value[inf->index++] = str[i];
//			printf(" %c\n", str[i]);
/*
			if (str[i] >= 0x80)
			{
				consumed = utf8_to_u16_nz(s+i, &utf16, len-i);
				printf("%02X", utf16);
				i += consumed;
			}
			else
				printf("%c", str[i]);*/
			break;
		}
}

// Parsing of a node name: start
static void XMLCALL start(void *userData, const char *name, const char **attr)
{
    Parseinfo *inf = (Parseinfo *) userData;
	inf->stack[inf->depth].name = malloc(strlen(name)+1);
	strcpy(inf->stack[inf->depth].name, name);
	printf("stack[%d] = %s\n", inf->depth, inf->stack[inf->depth].name);
	inf->index = 0;
	inf->value[0] = 0;	// for smart whitespace detection
}

/*
void* get_current_table(Parseinfo *inf)
{
	int i;
	for(i=0; i< curtable
}
*/
// Parsing of a node name: end (this is where we fill our table with the node value)
static void XMLCALL end(void *userData, const char *name)
{
    Parseinfo *inf = (Parseinfo *) userData;
	// 1. find the 
//	for (i=0;
//	table = inf->stack

	// Free the string we allocate
	if (inf->stack[inf->depth].name != NULL)
	{
		free(inf->stack[inf->depth].name);
		inf->stack[inf->depth].name = NULL;
	}
	if (inf->index != 0)
	{
		inf->value[inf->index] = 0;
		printf("  %s\n", inf->value);
		inf->index = 0;
	}
}

void rawstart(void *userData, const char *name, const char **attr) 
{
    Parseinfo *inf = (Parseinfo *) userData;
//	printf("rawstart[%d].name = %s\n",  inf->depth, name);
    if (!inf->skip) 
	{
        if (skip(inf, name, attr)) 
			inf->skip = inf->depth;
        else 
            start(inf, name, attr); 
    }
    inf->depth++;	
}

void rawend(void *userData, const char *name) 
{
    Parseinfo *inf = (Parseinfo *) userData;
    inf->depth--;
    if (!inf->skip)
        end(inf, name);
	if (inf->skip == inf->depth) 
        inf->skip = 0;
}  



int readconf(char* filename)
{
	// Expat variables
	char buf[BUFSIZ];
	int done;
	XML_Parser parser; 
	FILE* fd;
	Parseinfo info;

	parser = XML_ParserCreate(NULL);
    if (parser == NULL) {
        fprintf(stderr, "Could not allocate memory for parser\n");
        return -1;
    }
    init_info(&info);
	XML_SetUserData(parser, &info);
    XML_SetElementHandler(parser, rawstart, rawend);

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


void init_xml_config()
{
	int i;
	// The second part of a node table init MUST occur at runtime
	INIT_XML_TABLE(config);
	INIT_XML_TABLE(options);
	INIT_XML_TABLE(controls);

	// Set defaults values. Can be skipped if relying on the external XML
	SET_XML_NODE_DEFAULT(options, skip_intro, false);
	SET_XML_NODE_DEFAULT(options, enhanced_guards, true);
	SET_XML_NODE_DEFAULT(options, picture_corners, true);
	// The order is PSP, WIN
	SET_XML_NODE_DEFAULT2(controls, key_fire, 'x', '5');
    SET_XML_NODE_DEFAULT2(controls, key_toggle_walk_run, 'o', ' ');
    SET_XML_NODE_DEFAULT2(controls, key_pause, 's', SPECIAL_KEY_F5);
    SET_XML_NODE_DEFAULT2(controls, key_sleep, 'q', SPECIAL_KEY_F9);
    SET_XML_NODE_DEFAULT2(controls, key_stooge, 'd', SPECIAL_KEY_F10);
    SET_XML_NODE_DEFAULT2(controls, key_direction_left, 0, '4');
    SET_XML_NODE_DEFAULT2(controls, key_direction_right, 0, '6');
    SET_XML_NODE_DEFAULT2(controls, key_direction_up, 0, '8');
    SET_XML_NODE_DEFAULT2(controls, key_direction_down, 0, '2');
    SET_XML_NODE_DEFAULT2(controls, key_inventory_cycle_left, SPECIAL_KEY_LEFT, SPECIAL_KEY_LEFT);
    SET_XML_NODE_DEFAULT2(controls, key_inventory_cycle_right, SPECIAL_KEY_RIGHT, SPECIAL_KEY_RIGHT);
    SET_XML_NODE_DEFAULT2(controls, key_pickup, SPECIAL_KEY_UP, SPECIAL_KEY_UP);
    SET_XML_NODE_DEFAULT2(controls, key_dropdown, SPECIAL_KEY_DOWN, SPECIAL_KEY_DOWN);
    SET_XML_NODE_DEFAULT2(controls, key_escape, 'a', 0x1b);
    SET_XML_NODE_DEFAULT2(controls, key_prisoners_cycle_left, SPECIAL_LEFT_MOUSE_BUTTON, 0);
    SET_XML_NODE_DEFAULT2(controls, key_prisoners_cycle_right, SPECIAL_RIGHT_MOUSE_BUTTON, 0);
    SET_XML_NODE_DEFAULT2(controls, key_select_british, 0, SPECIAL_KEY_F1);
    SET_XML_NODE_DEFAULT2(controls, key_select_french, 0, SPECIAL_KEY_F2);
    SET_XML_NODE_DEFAULT2(controls, key_select_american, 0, SPECIAL_KEY_F3);
    SET_XML_NODE_DEFAULT2(controls, key_select_polish, 0, SPECIAL_KEY_F4);
	// Debug
	PRINT_XML_TABLE(controls); 
}