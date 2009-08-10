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
 * 	eschew.c: Parsing of XML files and write back functions
 *  ---------------------------------------------------------------------------
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "expat.h"
#include "ConvertUTF.h"
#include "eschew.h"

/*
 *	Rrrrright, Microsoft: using underscores in what's meant to become
 *	standard library functions. Suuure, why not?
 */
#if defined(WIN32)
#define atoll _atoi64
#endif


/*
 *	We'll use these cast macros to either assign a table value to a variable or
 *	assign a variable to a table value. Thus the recursive part (REC) will be
 *	defined before the macros are set to be used (see further down)
 */
#define XML_CAST_TO_BOOLEAN(tab, idx, val, typ)									\
REC(tab, idx, val, typ, xml_boolean, printf("illegal cast to boolean\n"))

#define XML_CAST_TO_STRING(tab, idx, val, typ)									\
REC(tab, idx, val, typ, xml_string, printf("illegal cast to string\n"))

#define XML_CAST_TO_BYTE(tab, idx, val, typ)									\
REC(tab, idx, val, typ, xml_unsigned_char,										\
 REC(tab, idx, val, typ, xml_char, printf("illegal cast to byte"))				\
)

// Parentheses - always a great fun (See also: "Reasons why NOBODY uses Lisp")
#define XML_CAST_TO_INTEGER(tab, idx, val, typ)									\
REC(tab, idx, val, typ, xml_unsigned_short,										\
 REC(tab, idx, val, typ, xml_short,												\
  REC(tab, idx, val, typ, xml_unsigned_int,										\
   REC(tab, idx, val, typ, xml_int,												\
    REC(tab, idx, val, typ, xml_unsigned_long,									\
     REC(tab, idx, val, typ, xml_long,											\
      REC(tab, idx, val, typ, xml_unsigned_long_long,							\
       REC(tab, idx, val, typ, xml_long_long, printf("illegal cast to integer\n"))\
	  )																			\
	 )																			\
	)																			\
   )																			\
  )																				\
 )																				\
)

#define XML_CAST_TO_FLOATING(tab, idx, val, typ)								\
REC(tab, idx, val, typ, xml_float,												\
 REC(tab, idx, val, typ, xml_double, printf("illegal cast to floating point\n"))\
)

// This table, defined in ConvertUTF.c, provides the length of a multibyte UTF8 sequence
extern const char trailingBytesForUTF8[256];

// The root is defined in another source
extern xnode xml_root;

// Our stack structure
typedef struct {
	char*		name;
	char*		comment;
	xml_node	node;
} stack_el;

typedef struct {
    int skip;
    int depth;
	stack_el stack[XML_STACK_SIZE];
	// holder for the string conversion of the read value
	char value[XML_MAX_STRING_LENGTH];
	// For intelligent white space detection and assignation. Allows a sequence 
	// of white spaces to be assigned as a value if only white spaces are present
	int white_space_value;
	int index;
} Parseinfo;

#define STACK_NODE(depth)		inf->stack[depth].node
#define STACK_NODE_SAFE(depth)	(((depth)<1)?NULL:STACK_NODE(depth))

// Boolean values we recognize. These are recognized within integer arrays as well
#define NB_XML_BOOL_VALUES	3
static const char* xml_true_values[NB_XML_BOOL_VALUES] = { "true", "enabled", "on" };
static const char* xml_false_values[NB_XML_BOOL_VALUES] = { "false", "disabled", "off" };

/*
 *	Convert a single UTF8 sequence to a 16 bit UTF16, for the zero terminated 
 *	string 'source'. Fill a 16 bit UTF16 in '*target' and returns the number 
 *	of bytes consumed.
 *	If the target glyph is not valid Unicode, FFFF is returned
 *	If the target glyph is valid but falls outside our scope, FFFD is returned
 *
 *	Note that this is NOT a true UTF8 to UTF16 conversion function, as it 
 *	only returns UTF16 chars in the range [0000-FFFF], whereas true UTF16
 *	is [0000-10FFFF] with surrogate pairs
 */
size_t single_utf8_to_utf16(const UTF8* source, UTF16* target) 
{
	UTF16	utf16[2];
	// Pointers to the "current" position in source and target
	const UTF8* p_utf8 = source;
	UTF16*	p_utf16 = utf16;
	ConversionResult res;

	res = ConvertUTF8toUTF16 (&p_utf8, &p_utf8[trailingBytesForUTF8[p_utf8[0]]+1], 
							  &p_utf16, &utf16[1], 0);
	switch(res)
	{
	case conversionOK:
		// Since we return a single word, we don't handle surrogate pairs
		if (p_utf16 != &utf16[1])
			*target = 0xFFFD;	// replacement character
		else
			*target = utf16[0];
		break;
	default:
		*target = 0xFFFF;	// Invalid
	break;
	}

	return (p_utf8 - (UTF8*)source)/sizeof(UTF8);
}


/*
 *	Convert string 'str' to boolean '*val'
 * Returns 0 if string is not a boolean true or false value, 0 otherwise
 */
int xml_to_bool(const char* str, xml_boolean* val)
{
	int i;

	*val = 0;	// false by default
	for (i=0; i<NB_XML_BOOL_VALUES; i++)
		if (strcmp(xml_true_values[i], str) == 0)
		{
			*val = ~0;
			return -1;
		}
	for (i=0; i<NB_XML_BOOL_VALUES; i++)
		if (strcmp(xml_false_values[i], str) == 0)
			return -1;

	return 0;
}

// Return the node index of child node "name" from node "parent". -1 if not found
static __inline int get_node_index(const char* name, xml_node parent)
{
	int i;
	if (parent == NULL)
		return -1;
	for (i=0; i<parent->node_count; i++)
		if (strcmp(name, parent->name[i]) == 0)
			return i;
	return -1;
}

// Init the  parsing user structure
void init_info(Parseinfo *info) 
{
	int i;
    info->skip = 0;
    info->depth = 1;	// Must start at 1
	info->index = 0;
	for (i=0; i<XML_STACK_SIZE; i++)
	{	// Think your init variables will be zeroed by VC++? Think again!
		info->stack[i].comment = NULL;
		info->stack[i].name = NULL;
	}
}

/*
 *	This function returns a node to the relevant sibling according to the attribute
 *	passed as parameter (if attribute is null, return the first sibling)
 */
xml_node get_sibling(xml_node startnode, const char** attr)
{
	xml_node curnode = startnode;
	if ((attr == NULL) || (attr[0] == NULL) || (attr[1] == NULL))
		return startnode;

	while(curnode != NULL)
	{
		if ((strcmp(attr[0], curnode->attr[0]) == 0) &&
			(strcmp(attr[1], curnode->attr[1]) == 0 ) )
			return curnode;
		curnode = curnode->next;
	}
	return NULL;
}

/*
 *	This function is used to create the XML tree by linking the tables previously 
 *	defined. No special cases needed for duplicates as each node/table has a unique
 *	name (see attributes notes) => a specific table can only ever have one parent.
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
			// Same index is used for siblings, so fill out the siblings if needed
			if (potential_parent->value[i] == NULL)
				potential_parent->value[i] = child;
			else
			{
				node = potential_parent->value[i];
				while (node->next != NULL)
					node = node->next;
				node->next = child;
			}

			return -1;
		}
		// If the potential_parent has children nodes, see if these might not be the actual parent
		if ( (potential_parent->value[i] != NULL) && link_table(child, potential_parent->value[i]) )
			return -1;
	}
	return 0;
}


// Find out if a node should be skipped. Returns true if skippable
int skip(Parseinfo *inf, const char *name, const char **attr) 
{
	int index;

	if (inf->depth <= 1)
		return (strcmp(name, xml_root.name[0]) != 0);

	// find out if "name" is one of the previous node's children
	index = get_node_index(name, STACK_NODE_SAFE(inf->depth-1));
	if (index != -1)
	{	// Got a match for our name in the parent table
		// In case the parent is a meta-node, is this parent link actually pointing to a table?
		if ( (STACK_NODE(inf->depth-1)->node_type == t_xml_node) &&
			 (STACK_NODE(inf->depth-1)->value[index] == NULL) )
		{
			fprintf(stderr, "eschew.skip: Orphan link found for node table xml_%s[%s]\n" 
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
static void XMLCALL read_value(void *userData, const char *s, int len)
{
	Parseinfo *inf = (Parseinfo *) userData;
	int i, cp_len;
//	unsigned char* str = (unsigned char*)s;
	if (inf->skip)
		return;

	/*		
	 *	White spaces, as defined by section 2.3 of the XML specs (http://www.w3.org/TR/REC-xml)
	 *
	 *	Our smart elimination of whitespaces is as follows: If the value is made of only 
	 *  whitespaces, we use that value, otherwise any leading and some trailing whitespaces 
	 *	are eliminated. Not quite 'xml:space="preserve"' but close enough for our use
	 */
	for (i=0; i<len; i++)
	{
		if ((s[i]!=0x20)&&(s[i]!=0x09)&&(s[i]!=0x0D)&&(s[i]!=0x0A))
			break;
	}

	if (i==len)
	{	// Whitespace only section
		cp_len = len;
		if (inf->index == 0)
			inf->white_space_value = -1;
		else	// Don't add more white spaces to what we already have
			return;
	}
	else
	{	// Contains regular characters
		cp_len = len-i;
		if (inf->white_space_value)
		// We might have a leading whitespaces leftover from last call
			inf->index = 0;
		inf->white_space_value = 0;
	}

	if (inf->index + cp_len > XML_MAX_STRING_LENGTH-2)
	{	// Don't overflow our string buffer!
		fprintf(stderr, "eschew.read_value: Truncated XML value.\n");
		cp_len = XML_MAX_STRING_LENGTH-2;
	}
	strncpy(&inf->value[inf->index], &s[len-cp_len], cp_len);
	inf->index += cp_len;
}


// Copy comments to our current stack for later processing (in assign)
static void XMLCALL copy_comments(void *userData, const char *s)
{
    Parseinfo *inf = (Parseinfo *) userData;
	inf->stack[inf->depth].comment = strdup(s);
}

// Parsing of a node name: start
static void XMLCALL start(void *userData, const char *name, const char **attr)
{
    Parseinfo *inf = (Parseinfo *) userData;
	xml_node node;
	int i;
	inf->stack[inf->depth].name = calloc(strlen(name)+1,1);
	// Whitespace, until proven otherwise
	inf->white_space_value = -1;
	strcpy(inf->stack[inf->depth].name, name);

	if (inf->depth <= 1)
		inf->stack[inf->depth].node = xml_root.value[0];
	else
	{
		i = get_node_index(name, STACK_NODE_SAFE(inf->depth-1));

		if ((i != -1) && (STACK_NODE(inf->depth-1)->node_type == t_xml_node))
		{
			if (STACK_NODE(inf->depth-1)->value[i] == NULL)
			{
				fprintf(stderr, "eschew.start: Orphan link for node table xml_%s[%s]\n"
					"              Did you forget to create table xml_%s in your source?\n", 
					STACK_NODE(inf->depth-1)->id,  STACK_NODE(inf->depth-1)->name[i], 
					STACK_NODE(inf->depth-1)->name[i]);
				STACK_NODE(inf->depth) = NULL;
			}
			else
			{
				node = STACK_NODE(inf->depth-1)->value[i];
				STACK_NODE(inf->depth) = get_sibling(STACK_NODE(inf->depth-1)->value[i], attr);
			}
		}
	}
	inf->index = 0;
	inf->value[0] = 0;	// reinit for smart whitespace detections
} 


// Define the XML cast recursive to assign a table element from a variable
#ifdef	REC
#undef	REC
#endif
#define REC(tab, idx, val, typ, def, rem)										\
	if (typ == t_##def) ((def*)tab)[idx] = (def)(val); else rem

// Parsing of a node name: end (this is where we fill our table with the node value)
static void XMLCALL assign(void *userData, const char *name)
{
	int i;
	UTF16 utf16;
	size_t consumed;
    Parseinfo *inf = (Parseinfo *) userData;
	xml_boolean tmp_bool;

	// Handle comments
	if (inf->stack[inf->depth].comment != NULL)
	{
		// Get the index
		i = get_node_index(name, STACK_NODE_SAFE(inf->depth-1));
		// NB: Only the first comment at the node level of multiple attribute nodes is kept
		if ((i != -1) && (STACK_NODE(inf->depth-1)->comment[i] == NULL))
			STACK_NODE(inf->depth-1)->comment[i] = inf->stack[inf->depth].comment;
		else	// If the comment wasn't assigned, we need to free its strdup'd string
			free(inf->stack[inf->depth].comment);
		inf->stack[inf->depth].comment = NULL;
	}

	// Handle values
	if (inf->index != 0)
	{	// We have a regular value

		inf->value[inf->index] = 0;		// Null terminate the string if needed

		// Get the index
		i = get_node_index(name, STACK_NODE_SAFE(inf->depth-1));

		if (i == -1)
		{
			if (!inf->white_space_value)
			{	// If it's a white space, it's not an error
				if (STACK_NODE_SAFE(inf->depth-1) != NULL)
					fprintf(stderr, "eschew.assign: Could not find node named %s in table xml_%s\n", 
						name, STACK_NODE(inf->depth-1)->id);
				else
					fprintf(stderr, "eschew.assign: Tried to assign a value to the root node!\n");
			}
			return;
		}

		switch(STACK_NODE(inf->depth-1)->node_type)
		{
		// BOOLEAN types
		case t_xml_boolean:
			if (xml_to_bool(inf->value, &tmp_bool)) 
				((xml_boolean*)STACK_NODE(inf->depth-1)->value)[i] = tmp_bool; 
			else
				fprintf(stderr, "eschew.assign: '%s' is not a valid boolean value\n", inf->value);
			break;
		// BYTE types
		case t_xml_unsigned_char:
		case t_xml_char:
			consumed = single_utf8_to_utf16((UTF8*)inf->value, &utf16);
			if (utf16 > 0x100)
				fprintf(stderr, "eschew.assign: Truncated Unicode value to fit single byte\n");
			XML_CAST_TO_BYTE(STACK_NODE(inf->depth-1)->value, i, utf16&0xFF, 
				STACK_NODE(inf->depth-1)->node_type); 
			break;
		// STRING types
		case t_xml_string:
			// Convert to string
			((xml_string*)STACK_NODE(inf->depth-1)->value)[i] = strdup(inf->value); 
			break;
		// FLOATING POINT types
		case t_xml_float:
		case t_xml_double:
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
				((xml_boolean*)STACK_NODE(inf->depth-1)->value)[i] = tmp_bool; 
			}
			else	// Not a boolean
			{
				XML_CAST_TO_INTEGER(STACK_NODE(inf->depth-1)->value, i, 
					atoll(inf->value), STACK_NODE(inf->depth-1)->node_type); 
			}
			break;
		// Someone is trying to assign a direct value to a meta node
		case t_xml_node:
			// We do get some remnants white space values here from Expat (due to XML formatting)
			// Only alert the user on non whitespaces.
			if (!inf->white_space_value)
			{
				fprintf(stderr, "eschew.assign: Invalid value '%s' assigned to meta-node <%s", 
					inf->value, STACK_NODE(inf->depth)->id);
				if (STACK_NODE(inf->depth)->attr[0] != NULL)
					fprintf(stderr, " %s=\"%s\"", STACK_NODE(inf->depth)->attr[0], STACK_NODE(inf->depth)->attr[1]);
				fprintf(stderr, ">\n");
			}
			break;
		// UNSUPPORTED
		default:
			fprintf(stderr, "eschew.assign: Conversion of '%s' into table %s's type is not supported yet.\n", 
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
        assign(inf, name);
	if (inf->skip == inf->depth) 
        inf->skip = 0;
}  

/*
 *	Open and parse the xml file "filename"
 *	Returns -1 on success, 0 otherwise
 */
int read_xml(const char* filename)
{
	// Expat variables
	char buf[BUFSIZ];
	int done;
	XML_Parser parser; 
	FILE* fd;
	Parseinfo info;

	if ((fd = fopen(filename, "rb")) == NULL)
		return 0;

	parser = XML_ParserCreate(NULL);
    if (parser == NULL) {
		fprintf(stderr, "eschew.read_xml: Could not allocate memory for parser\n");
        return 0;
    }
    init_info(&info);
	XML_SetUserData(parser, &info);
    XML_SetElementHandler(parser, rawstart, rawend);
	XML_SetCommentHandler(parser, copy_comments);

	XML_SetCharacterDataHandler(parser, read_value);

	do 
	{
		int len = (int)fread(buf, 1, sizeof(buf), fd);
		done = len < sizeof(buf);
		if (XML_Parse(parser, buf, len, done) == XML_STATUS_ERROR) 
		{
			fprintf(stderr, "expat error: [%d] %s at line %lu\n", XML_GetErrorCode(parser),
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

// Define the XML cast recursive to assign a variable from a table element
#ifdef	REC
#undef	REC
#endif
#define REC(tab, idx, val, typ, def, rem)										\
	if (typ == t_##def) val = ((def*)tab)[idx]; else rem

// For human readibility
#define XML_NB_INDENT_SPACES	2
#define XML_INDENT				for(i=0;i<current_indentation;i++) fputc(' ', fd)

// Recursive function that writes XML nodes into a file
void write_node(FILE* fd, xml_node xn)
{
	int i, n;
	static int current_indentation = 0;
	xml_unsigned_long_long	lval = 0;
	xml_boolean				bval = 0;
	xml_unsigned_char		cval = 0;
	xml_double				fval = 0;
	xml_string				sval = 0;
	xml_node				nval = NULL;

	if (xn == NULL)
		return;

	XML_INDENT;	fprintf(fd, "<%s", xn->id);
	if (xn->attr[0] != NULL)
		fprintf(fd, " %s=\"%s\"", xn->attr[0], xn->attr[1]);
	fprintf(fd, ">\n");
	current_indentation += XML_NB_INDENT_SPACES;
	for (n=0; n<xn->node_count; n++)
	{
		if (xn->comment[n] != NULL)
		{
			XML_INDENT; 
			fprintf(fd, "<!--%s-->\n", xn->comment[n]);
		}
		switch(xn->node_type)
		{
		case t_xml_node:
			nval = xn->value[n];
			while (nval != NULL)
			{
				write_node(fd, nval);
				nval = nval->next;
			}
			break;
		case t_xml_boolean:
			XML_CAST_TO_BOOLEAN(xn->value, n, bval, xn->node_type);
			XML_INDENT; 
			fprintf(fd, "<%s>%s</%s>\n", xn->name[n], 
				(bval?xml_true_values[0]:xml_false_values[0]), xn->name[n]);
			break;
		case t_xml_unsigned_char:
		case t_xml_char:
			XML_CAST_TO_BYTE(xn->value, n, cval, xn->node_type);
			XML_INDENT; 
			fprintf(fd, "<%s>", xn->name[n]);
			if ((cval < 0x20) || (cval >= 0x7F))
				fprintf(fd, "&#x%02X;", cval);
			else
				fprintf(fd, "%c", cval);
			fprintf(fd, "</%s>\n", xn->name[n]);
			break;
		case t_xml_unsigned_short:
		case t_xml_short:
		case t_xml_unsigned_int:
		case t_xml_int:
		case t_xml_unsigned_long:
		case t_xml_long:
		case t_xml_unsigned_long_long:
		case t_xml_long_long:
			XML_CAST_TO_INTEGER(xn->value, n, lval, xn->node_type);
			XML_INDENT; 
			fprintf(fd, "<%s>%lld</%s>\n", xn->name[n], lval, xn->name[n]);
			break;
		case t_xml_float:
		case t_xml_double:
			XML_CAST_TO_FLOATING(xn->value, n, fval, xn->node_type);
			XML_INDENT; 
			fprintf(fd, "<%s>%f</%s>\n", xn->name[n], fval, xn->name[n]);
			break;
		case t_xml_string:
			XML_CAST_TO_STRING(xn->value, n, sval, xn->node_type);
			XML_INDENT; 
			fprintf(fd, "<%s>%s</%s>\n", xn->name[n], (sval==NULL)?"":sval, xn->name[n]);
			break;
		default:
			fprintf(stderr, "eschew.write_node: Conversion to xml type %d not yet implemented\n", xn->node_type);
			// Let's write a blank value nonetheless
			XML_INDENT; 
			fprintf(fd, "<%s>undefined</%s>\n", xn->name[n], xn->name[n]);
			break;
		}
	}
	current_indentation -= XML_NB_INDENT_SPACES;
	XML_INDENT;	fprintf(fd, "</%s>\n", xn->id);
}

/*
 *	Write the XML file "filename" using the current tables' data
 *	If filename is not provided, writes to stdout
 *	Returns -1 on success, 0 otherwise
 */
int write_xml(const char* filename)
{
	FILE* fd;

	// NULL or empty string => use stdout
	if ((filename == NULL) || (strlen(filename) == 0))
		fd = stdout;
	else if ((fd = fopen(filename, "wb")) == NULL)
		return 0;

	fprintf(fd, "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n");
	fprintf(fd, "<!--Generated by Eschew XML Expat Wrapper v%s-->\n", ESCHEW_VERSION);
	write_node(fd, xml_root.value[0]);

	if (fd != stdout)
		fclose(fd);

	return -1;
}
