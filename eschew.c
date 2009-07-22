/*
	Eschew XML parser helper functions (XML Parse, UTF-8 conversions, etc.)
	See .h for details
	
	UTF-8 to UTF-16 conversion adapted from unicode.org
	See http://unicode.org/Public/PROGRAMS/CVTUTF/ConvertUTF.c and
	    http://unicode.org/Public/PROGRAMS/CVTUTF/ConvertUTF.h

*/



#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "expat.h"
#include "eschew.h"

/*
	Rrrrright, Microsoft: using underscores in what's meant to become
	standard library functions. Suuure, why not?
*/
#if defined(WIN32)
#define atoll _atoi64
#endif

// The root is defined in another source
extern xnode xml_root;

// Our stack structure
typedef struct {
	char*		name;
//	int			index;
//	s_xml_attr* attr;
	xml_node	node;
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

// To prevent gcc's warning 'array subscript is above array bounds'
#define NB_XML_BOOL_VALUES 3
static const char* xml_true_values[NB_XML_BOOL_VALUES] = { "true", "enabled", "on" };
static const char* xml_false_values[NB_XML_BOOL_VALUES] = { "false", "disabled", "off" };


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

static const unsigned long offsetsFromUTF8[6] = { 0x00000000, 0x00003080, 
	0x000E2080, 0x03C82080, 0xFA082080, 0x82082080 };


/*
	Checks UTF8 validity. Returns -1 if valid, 0 otherwise
*/
static int isLegalUTF8(const char *source, int length) 
{
    unsigned char a;
	// Make sure we deal with UNSIGNED chars at all time
	const unsigned char *src = (unsigned char*)source;
    const unsigned char *srcptr = src+length;
    switch (length) {
    default: return 0;
	// Everything else falls through when "true"...
    case 4: if ((a = (*--srcptr)) < 0x80 || a > 0xBF) return 0;
    case 3: if ((a = (*--srcptr)) < 0x80 || a > 0xBF) return 0;
    case 2: if ((a = (*--srcptr)) > 0xBF) return 0;

	switch (*src) {
	    // no fall-through in this inner switch
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
int utf8_to_u16(const char* source, unsigned short* target) 
{
	return utf8_to_u16_nz(source, target, strlen(source));
}


/*
   XML parser
*/

/*
	Convert string 'str' to boolean '*val'
	Returns 0 if string is not a boolean true or false value, 0 otherwise
*/
int xml_to_bool(const char* str, xml_boolean* val)
{
	int i;

	*val = 0;	// false by default
	for (i=0; i<NB_XML_BOOL_VALUES; i++)
		if (strcmp(xml_true_values[i], str) == 0)
		{
			*val = -1;
			return -1;
		}
	for (i=0; i<NB_XML_BOOL_VALUES; i++)
		if (strcmp(xml_false_values[i], str) == 0)
			return -1;

	return 0;
}

// static __inline

// Return the node index of child node "name" from node "parent". -1 if not found
static __inline int get_node_index(const char* name, xml_node parent)
{
	int i;
	for (i=0; i<parent->node_count; i++)
		if (strcmp(name, parent->name[i]) == 0)
			return i;
	return -1;
}

/*
// Return the node index of child node "name" from node "parent". -1 if not found
 int get_node_index(const char* name, xml_node parent, const char** attr)
{
	int i;
	for (i=0; i<parent->node_count; i++)
		if (strcmp(name, parent->name[i]) == 0)
		{
			if ((parent->node_type == t_xml_node) && (parent->value[i] != NULL))
				printf("name = %s, parent_name=%s, parent_attr = %s\n", name, parent->name[i], parent->value[i]->attr.name);
//				printf("name = %s, parent_name=%s, ptr = %X\n", name, parent->name[i], parent->value[i]);
			// Check attributes 
			if ((parent->node_type == t_xml_node) && (attr != NULL) && (attr[0]!=NULL) && (attr[1]!=NULL))
			{
				if (parent->value[i] == NULL)
					continue;
				if (parent->value[i]->attr.name == NULL)
					printf("attr name: match on NULL\n");
				else if ((strcmp(attr[0], parent->value[i]->attr.name) == 0) &&
					(strcmp(attr[1], parent->value[i]->attr.value) == 0))
					printf("attr name match on %s=%s\n", attr[0], attr[1]);
				else
				{
					printf("attr %s=%s - no match\n", attr[0], attr[1]);
					printf("PS: %s %s=%s\n", parent->name[i], parent->value[i]->attr.name, parent->value[i]->attr.value);
					continue;
				}
			}
			return i;
		}
	return -1;
}
*/

#define STACK_NODE(depth)	inf->stack[depth].node

// Init the  parsing user structure
void init_info(Parseinfo *info) {
    info->skip = 0;
    info->depth = 1;	// Must start at 1
	info->index = 0;
}

/*
// TO_DO: attribute recognition
xml_node get_branch_node(Parseinfo *inf, int level)
{
	int i;
	xml_node node;
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
	This function returns a node to the relevant sibling according to the attribute
	passed as parameter (if attribute is null, return the first sibling)
*/
xml_node get_sibling(xml_node startnode, const char** attr)
{
	xml_node curnode = startnode;
	if ((attr == NULL) || (attr[0] == NULL) || (attr[1] == NULL))
		return startnode;

	while(curnode != NULL)
	{
		if ((strcmp(attr[0], curnode->attr.name) == 0) &&
			(strcmp(attr[1], curnode->attr.value) == 0 ) )
			return curnode;
		curnode = curnode->next;
	}
	return NULL;
}


/*
	This function is used to create the XML tree by linking the tables previously 
	defined. No special cases needed for duplicates as each node/table has a unique
	name => a specific table can only ever have one parent.
*/
int link_table(xml_node child, xml_node potential_parent)
{
	int i;
	xml_node node;

	// only bother if the potential_parent is a meta node
	if (potential_parent->node_type != t_xml_node)
		return 0;

	// Explore all subnodes
	for (i=0; i<potential_parent->node_count; i++)
	{
		if (strcmp(child->id, potential_parent->name[i]) == 0)
		{
			printf("link_table: matched child xml_%s with parent xml_%s[%s]\n", 
				child->id, potential_parent->id, potential_parent->name[i]);

			// Same index is used for siblings, so fill out the siblings if needed
			if (potential_parent->value[i] == NULL)
				potential_parent->value[i] = child;
			else
			{
				node = potential_parent->value[i];
				while (node->next != NULL)
					node = node->next;
				node->next = child;
				printf("  SET SIBLING!\n");
			}

			return -1;
		}
		// If the potential_parent has children nodes, see if these might not be the actual parent
		if ( (potential_parent->value[i] != NULL) && link_table(child, potential_parent->value[i]) )
			return -1;
	}
	return 0;
}


int look_for_orphans()
{
	return 0;
}


// Find out if a node should be skipped. Returns true if skippable
int skip(Parseinfo *inf, const char *name, const char **attr) 
{
	int index;

	if (inf->depth <= 1)
		return (strcmp(name, xml_root.name[0]) != 0);

	// find out if "name" is one of the previous node's children
	index = get_node_index(name, STACK_NODE(inf->depth-1));
	if (index != -1)
	{	// Got a match for our name in the parent table
		// In case the parent is a meta-node, is this parent link actually pointing to a table?
		if ( (STACK_NODE(inf->depth-1)->node_type == t_xml_node) &&
			 (STACK_NODE(inf->depth-1)->value[index] == NULL) )
		{
			printf("skip: Orphan link found for node table xml_%s[%s]\n" 
				"Did you forget to create table xml_%s in your source?\n", 
				STACK_NODE(inf->depth-1)->id, STACK_NODE(inf->depth-1)->name[index], 
				STACK_NODE(inf->depth-1)->name[index]);
			return -1;
		}
		// In case the parent is a meta node, do we have the right sibling
		else if ( (STACK_NODE(inf->depth-1)->node_type == t_xml_node) &&
				  (get_sibling((xml_node) (STACK_NODE(inf->depth-1)->value[index]), attr) == NULL) )
			return -1;
		else
			return 0;
	}
	// Not found
	return -1;
}


// Parsing of the node values
static void XMLCALL characterData(void *userData, const char *s, int len)
{
	Parseinfo *inf = (Parseinfo *) userData;
	int i;
	unsigned char* str = (unsigned char*)s;
	if (inf->skip)
		return;
	for (i=0; i<len; i++)
		// copy the string data in our info structure
		switch(str[i])
		{
		/*		
			White spaces, as defined by section 2.3 of the XML specs (http://www.w3.org/TR/REC-xml)

			Our smart elimination of whitespaces is as follows: If the value is made of
			only whitespaces, we use that value, otherwise any leading and trailing whitespaces 
			are eliminated. Not quite 'xml:space="preserve"' but close enough for our use
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
	xml_node node;
	int i;
	inf->stack[inf->depth].name = malloc(strlen(name)+1);
	// Whitespace, until proven otherwise
	inf->white_space_value = -1;
	strcpy(inf->stack[inf->depth].name, name);
	printf("stack[%d] = %s\n", inf->depth, inf->stack[inf->depth].name);

	if (inf->depth <= 1)
		inf->stack[inf->depth].node = xml_root.value[0];
	else
	{
		i = get_node_index(name, STACK_NODE(inf->depth-1));
		// Spare the index value in our stack
//		inf->stack[inf->depth].index = i;
		if ((i != -1) && (STACK_NODE(inf->depth-1)->node_type == t_xml_node))
		{
			if (STACK_NODE(inf->depth-1)->value[i] == NULL)
			{
				printf("start: Orphan link for node table xml_%s[%s]\n"
					"Did you forget to create table xml_%s in your source?\n", 
					STACK_NODE(inf->depth-1)->id,  STACK_NODE(inf->depth-1)->name[i], 
					STACK_NODE(inf->depth-1)->name[i]);
				STACK_NODE(inf->depth) = NULL;
			}
			else
			{
				node = STACK_NODE(inf->depth-1)->value[i];
				// HERE!!!
				STACK_NODE(inf->depth) = STACK_NODE(inf->depth-1)->value[i];
			}
		}
	}
	inf->index = 0;
	inf->value[0] = 0;	// reinit for smart whitespace detection
}


// Parsing of a node name: end (this is where we fill our table with the node value)
static void XMLCALL end(void *userData, const char *name)
{
	int i;
	unsigned short utf16;
    Parseinfo *inf = (Parseinfo *) userData;
	xml_boolean tmp_bool;

	if (inf->index != 0)
	{	// We have a regular value
		inf->value[inf->index] = 0;		// Null terminate the string if needed
		// Get the index
		i = get_node_index(name, STACK_NODE(inf->depth-1));
		if (i == -1)
		{
			printf("assign value: could not find node named %s in xml_%s\n", name, STACK_NODE(inf->depth-1)->id);
			return;
		}

//		printf("  reminder: table = xml_%s\n", STACK_NODE(inf->depth-1)->id);
		switch(STACK_NODE(inf->depth-1)->node_type)
		{
		// BOOLEAN types
		case t_xml_boolean:
			if (xml_to_bool(inf->value, &tmp_bool)) 
				((xml_boolean*)STACK_NODE(inf->depth-1)->value)[i] = tmp_bool; 
			else
				printf("  %s: not a valid boolean value\n", inf->value);
			break;
		// BYTE types
		case t_xml_unsigned_char:
		case t_xml_char:
			utf8_to_u16(inf->value, &utf16);
			// TO_DO: check consumed
			if (utf16 > 0x100)
				printf("Unicode sequence truncated to fit single byte\n");
			XML_CAST_TO_BYTE(STACK_NODE(inf->depth-1)->value, i, utf16&0xFF, 
				STACK_NODE(inf->depth-1)->node_type); 
			printf("  u8: %02X\n", (unsigned char)(utf16&0xFF));
			break;
		// STRING types
		case t_xml_string:
			// Convert to string
			((xml_string*)STACK_NODE(inf->depth-1)->value)[i] = strdup(inf->value); 
			printf("  string: '%s'\n", inf->value);
			break;
		// FLOATING POINT types
		case t_xml_float:
		case t_xml_double:
			printf("  float: '%s'\n", inf->value);
			XML_CAST_TO_INTEGER(STACK_NODE(inf->depth-1)->value, i, 
				atof(inf->value), STACK_NODE(inf->depth-1)->node_type); 
			break;
		// INTEGER types
		case t_xml_unsigned_short:
		case t_xml_short:
		case t_xml_unsigned_int:
		case t_xml_int:
		case t_xml_unsigned_long:
		case t_xml_long:
		case t_xml_unsigned_long_long:
		case t_xml_long_long:
			// Check if the string value is a boolean
			if (xml_to_bool(inf->value, &tmp_bool))
			{
				printf("  boolean, set to %s\n", tmp_bool?"true":"false");
				((xml_boolean*)STACK_NODE(inf->depth-1)->value)[i] = tmp_bool; 
			}
			else	// Not a boolean
			{
				printf("  integer: '%s'\n", inf->value);
				XML_CAST_TO_INTEGER(STACK_NODE(inf->depth-1)->value, i, 
					atoll(inf->value), STACK_NODE(inf->depth-1)->node_type); 
			}
			break;
		// UNSUPPORTED
		default:
			printf("  Conversion of '%s' into table's %s type is not supported yet.\n", 
				inf->value, STACK_NODE(inf->depth-1)->id);
			break;
		}
		inf->index = 0;
	}

	// Free the string we allocated for the node name
	if (inf->stack[inf->depth].name != NULL)
	{
		free(inf->stack[inf->depth].name);
		inf->stack[inf->depth].name = NULL;
	}
}

// The calls to the raw function is used to skip nodes we don't want
void rawstart(void *userData, const char *name, const char **attr) 
{
    Parseinfo *inf = (Parseinfo *) userData;
//	printf("rawstart[%d].name = %s\n",  inf->depth, name);
//	if ((attr != NULL) && (attr[0] != NULL) && (attr[1] != NULL))
//		printf("attr: %s=%s\n", attr[0], attr[1]);
    if (!inf->skip) 
	{
        if (skip(inf, name, attr)) 
		{
//			printf("  skipped_d!\n");
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

/*
	Opens and reads the xml file "filename"
	Returns -1 on success, 0 otherwise
*/
int read_xml(const char* filename)
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
        return 0;
    }
    init_info(&info);
	XML_SetUserData(parser, &info);
    XML_SetElementHandler(parser, rawstart, rawend);

	XML_SetCharacterDataHandler(parser, characterData);

	if ((fd = fopen(filename, "rb")) == NULL)
	{
		printf("read_xml: can't find '%s' - Aborting.\n", filename);
		return 0;
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
			return 0;
		}
	} 
	while (!done);
	XML_ParserFree(parser);
	fclose(fd);

	return -1;
}
