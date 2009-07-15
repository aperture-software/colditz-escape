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
	xml_node*	node;
} stack_el;

typedef struct {
    int skip;
    int depth;
	stack_el stack[XML_STACK_SIZE];
	// holder for the string conversion of the read value
	char value[256];
	// For intelligent white space detection and assignation. Allows a sequence 
	// of white spaces to be assigned as a value ifonly white spaces are present
	int white_space_value;
	int index;
} Parseinfo;


#if defined(PSP)
#define SET_XML_NODE_DEFAULT2(table, node_name, val1, val2) 			\
	SET_XML_NODE_DEFAULT(table, node_name, val1)
#else
#define SET_XML_NODE_DEFAULT2(table, node_name, val1, val2) 			\
	SET_XML_NODE_DEFAULT(table, node_name, val2)
#endif


/*
	UTF-8 conversion constants
*/
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


/*
	Convert a single UTF8 sequence to a 16 bit UTF16, for a non zero 
	terminated string 'source' of length 'len'. Fill a 16 bit UTF16 in
	'*target' and returns the number of bytes consumed.
	If the target glyph is not valid Unicode, FFFF is returned
	If the target glyph is valid but falls outside our scope, FFFD is returned

	Note that this is NOT a true UTF8 to UTF16 conversion function, as it 
	only returns UTF16 chars in the range [0000-FFFF], whereas true UTF16
	is [0000-10FFFF]
*/
int utf8_to_u16_nz(const char* source, unsigned short* target, size_t len) 
{
	unsigned long ch = 0;
	int extraBytesToRead;
	int pos = 0;
	// To make sure we deal with UNSIGNED chars at all time 
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
		*target = (unsigned short)ch;
	else
		*target = 0xFFFD;	// replacement character

	return extraBytesToRead + 1;
}


// Same as above, for NULL terminated strings
int utf8_to_u16(const char* source, u16* target) 
{
	return utf8_to_u16_nz(source, target, strlen(source));
}


/*
   XML parser
*/

// Return the node index of child node "name" from node "parent". -1 if not found
static __inline int get_node_index(const char* name, xml_node* parent)
{
	int i;
	for (i=0; i<parent->node_count; i++)
		if (strcmp(name, parent->name[i]) == 0)
			return i;
	return -1;
}

#define STACK_NODE(depth)	inf->stack[depth].node

// Init the  parsing user structure
void init_info(Parseinfo *info) {
    info->skip = 0;
    info->depth = 0;
	info->index = 0;
}

/*
// TO_DO: attribute recognition
xml_node* get_branch_node(Parseinfo *inf, int level)
{
	int i;
	xml_node* node;
	if (level == 0)
	{	// root node
		if (strcmp(inf->stack[0].name, xml_root.name[0]) == 0)
			return xml_root.value[0];
		else
			return NULL;
	}

	node = get_branch_node(inf, level-1);

	if (node == NULL)
		return NULL;

	for (i=0; i<node->node_count; i++)
		if (strcmp(inf->stack[level].name, node->name[i]) == 0)
			return node->value[i];

	return NULL;
}
*/

/*
	This function is used to create the XML tree by linking the tables previously 
	defined. No special cases needed for duplicates as each node/table has a unique
	name => a specific table can only ever have one parent.
*/
bool link_table(xml_node* child, xml_node* potential_parent)
{
	int i;

	// only bother if the potential_parent is a meta node
	if (potential_parent->node_type != type_xml_node_ptr)
		return false;

	// Explore all subnodes
	for (i=0; i<potential_parent->node_count; i++)
	{
		if (strcmp(*(child->id), potential_parent->name[i]) == 0)
		{
			potential_parent->value[i] = child;
			printf("link_table: matched child xml_%s with parent xml_%s[%s]\n", 
				*(child->id), *potential_parent->id, potential_parent->name[i]);
			return true;
		}
		// If the potential_parent has children nodes, see if these might not be the actual parent
		if ( (potential_parent->value[i] != NULL) && link_table(child, potential_parent->value[i]) )
			return true;
	}
	return false;
}


int look_for_orphans()
{
	return 0;
}


// Skip nodes that are undefined
bool skip(Parseinfo *inf, const char *name, const char **attr) 
{
	int index;

	if (inf->depth == 0)
		return (strcmp(name, xml_root.name[0]) != 0);

	// find out if "name" is one of the previous node's children
	index = get_node_index(name, STACK_NODE(inf->depth-1));
	if (index != -1)
	{	// Got a match for our name in the parent table
		// In case the parent is a meta-node, is this parent link actually pointing to a table?
		if ( (STACK_NODE(inf->depth-1)->node_type == type_xml_node_ptr) &&
			 (STACK_NODE(inf->depth-1)->value[index] == NULL) )
		{
			printf("skip: Orphan link found for node table xml_%s[%s]: Did you forget to create table xml_%s in your source?\n", 
				*STACK_NODE(inf->depth-1)->id, STACK_NODE(inf->depth-1)->name[index], 
				STACK_NODE(inf->depth-1)->name[index]);
			return true;
		}
		else
			return false;
	}
	// Not found
	return true;
}


// Parsing of the node values
static void XMLCALL characterData(void *userData, const char *s, int len)
{
	Parseinfo *inf = (Parseinfo *) userData;
	int i;
	u8* str = (u8*)s;
	if (inf->skip)
		return;
	for (i=0; i<len; i++)
		// copy the string data in our info structure
		switch(str[i])
		{
		/*		
			White space, as defined by section 2.3 of the XML specs (http://www.w3.org/TR/REC-xml)

			In this section we perform smart elimination of whitespaces: If the value is only
			whitespace, we use that value, otherwise whitespaces are eliminated.
			Not quite 'xml:space="preserve"' but close enough for our use
		*/
		case 0x20:
		case 0x09:
		case 0x0D:
		case 0x0A:
			if (inf->white_space_value)
				inf->value[inf->index++] = str[i];
			break;
		default:
			if (inf->white_space_value)
			{	// Non whitespace encountered => reset
				inf->white_space_value = 0;
				inf->index = 0;
			}
			inf->value[inf->index++] = str[i];
			break;
		}
}



// Parsing of a node name: start
static void XMLCALL start(void *userData, const char *name, const char **attr)
{
    Parseinfo *inf = (Parseinfo *) userData;
	int i;
	inf->stack[inf->depth].name = malloc(strlen(name)+1);
	// Consider the value is whitespace until proven otherwise
	inf->white_space_value = -1;
	strcpy(inf->stack[inf->depth].name, name);
	printf("stack[%d] = %s\n", inf->depth, inf->stack[inf->depth].name);
	if (inf->depth == 0)
		inf->stack[inf->depth].node = xml_root.value[0];
	else
	{
		i = get_node_index(name, STACK_NODE(inf->depth-1));
		if ((i != -1) && (STACK_NODE(inf->depth-1)->node_type == type_xml_node_ptr))
		{
			STACK_NODE(inf->depth) = STACK_NODE(inf->depth-1)->value[i];
			if (STACK_NODE(inf->depth) == NULL)
				printf("start: Orphan link for node table xml_%s[%s]: Did you forget to create table xml_%s in your source?\n", 
					*STACK_NODE(inf->depth-1)->id,  STACK_NODE(inf->depth-1)->name[i], 
					STACK_NODE(inf->depth-1)->name[i]);
		}

/*
		for (i=0; i<STACK_NODE(inf->depth-1)->node_count; i++)
		{
			if ( (strcmp(name, STACK_NODE(inf->depth-1)->name[i]) == 0) &&
				 (STACK_NODE(inf->depth-1)->node_type == type_xml_node_ptr) )
			{
				STACK_NODE(inf->depth) = STACK_NODE(inf->depth-1)->value[i];
//				printf("match[%d]: link to xml_%s is %X\n", inf->depth, inf->stack[inf->depth-1].node->name[i], inf->stack[inf->depth].node );
				if (STACK_NODE(inf->depth) == NULL)
					printf("start: Orphan link for node table xml_%s[%s]: Did you forget to create table xml_%s in your source?\n", 
						*STACK_NODE(inf->depth-1)->id,  STACK_NODE(inf->depth-1)->name[i], 
						STACK_NODE(inf->depth-1)->name[i]);
			}
		}
*/
	}
	inf->index = 0;
	inf->value[0] = 0;	// reinit for smart whitespace detection
}


// Parsing of a node name: end (this is where we fill our table with the node value)
static void XMLCALL end(void *userData, const char *name)
{
	int i, consumed;
	unsigned short utf16;
    Parseinfo *inf = (Parseinfo *) userData;

	if (inf->index != 0)
	{	// We have a regular value
		inf->value[inf->index] = 0;
//		printf("  reminder: table = xml_%s\n", *STACK_NODE(inf->depth-1)->id);
		switch(STACK_NODE(inf->depth-1)->node_type)
		{
		case type_u8:
			i = get_node_index(name, STACK_NODE(inf->depth-1));
			if (i == -1)
			{
				printf("assign value: could not find node named %s in xml_%s\n", name, *STACK_NODE(inf->depth-1)->id);
				return;
			}
			utf8_to_u16(inf->value, &utf16);
			if (utf16 > 0x100)
				printf("UTF16 sequence too high\n");
			((u8*)STACK_NODE(inf->depth-1)->value)[i] = (u8)(utf16&0xFF);
			printf("  u8: %02X\n", (u8)(utf16&0xFF));
			break;
		default:
			printf("  string: '%s'\n", inf->value);
			break;
		}

/*
			if (str[i] >= 0x80)
			{
				consumed = utf8_to_u16_nz(s+i, &utf16, len-i);
				printf("%02X", utf16);
				i += consumed;
			}
*/

		inf->index = 0;
	}

	// Free the string we allocated for the node name
	if (inf->stack[inf->depth].name != NULL)
	{
		free(inf->stack[inf->depth].name);
		inf->stack[inf->depth].name = NULL;
	}


/*
	else if (!inf->assigned)
	{	// Nothing was assigned yet, so we might want 
		printf("  0x%02X\n", inf->value[0]);
		inf->assigned = true;
	}
*/
}

void rawstart(void *userData, const char *name, const char **attr) 
{
    Parseinfo *inf = (Parseinfo *) userData;
//	printf("rawstart[%d].name = %s\n",  inf->depth, name);
    if (!inf->skip) 
	{
        if (skip(inf, name, attr)) 
		{
//			printf("  skipped!\n");
			inf->skip = inf->depth;
		}
        else 
            start(inf, name, attr); 
    }
//	else
//		printf("  skipped!\n");
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



int readconf(const char* filename)
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

	exit(1);
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
//	PRINT_XML_TABLE(controls); 
}