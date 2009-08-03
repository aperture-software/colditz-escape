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
 * 	eschew.c: Parsing of XML files and write back, definitions
 *  ---------------------------------------------------------------------------
 */

#pragma once

#ifdef	__cplusplus
extern "C" {
#endif

// User configurable options for XML maximu depth and attribute handling
#define XML_STACK_SIZE 16
#define XML_ATTRIBUTE_SEPARATOR	"_"

/*
	We need the following *SINGLE WORD* types to be defined, so that we can
	figure out the type of our XML node tables at runtime
*/
#define xml_boolean				int
#define xml_unsigned_char		unsigned char
#define xml_char				char
#define xml_unsigned_short		unsigned short
#define xml_short				short
#define xml_unsigned_int		unsigned int
#define xml_int					int
#define xml_unsigned_long		unsigned long
#define xml_long				long
#define xml_unsigned_long_long	unsigned long long
#define xml_long_long			long long
#define xml_float				float
#define xml_double				double
#define xml_string				char*
#define xml_node				xnode*
#define xml_illegal_type		void*

enum XML_TYPE {
	t_xml_boolean,
	t_xml_unsigned_char,
	t_xml_char,
	t_xml_unsigned_short,
	t_xml_short,
	t_xml_unsigned_int,
	t_xml_int,
	t_xml_unsigned_long,
	t_xml_long,
	t_xml_unsigned_long_long,
	t_xml_long_long,
	t_xml_float,
	t_xml_double,
	t_xml_string,
	t_xml_node,
	t_xml_illegal_type };

// XML attributes
typedef struct {
	char* name;
	char* value;
} s_xml_attr;

// The main XML node
typedef struct _xnode { 
	enum XML_TYPE node_type; 
	int node_count; 
	s_xml_attr attr;	
	char** _tokens;
	char*  id; 
	char** name;
	char** comment;
	struct _xnode **value;
	struct _xnode *next;} xnode;

// Useful access macros
#define XML_VALUE(tabid, i)		xml_##tabid.value[i]
#define XML_NAME(tabid, i)		xml_##tabid.name[i]
#define XML_COMMENT(tabid, i)	xml_##tabid.comment[i]

// XML default value initialization (optional)
#define SET_XML_NODE_DEFAULT(tabid, node_name, val) {							\
	for (i=0; i<xml_##tabid.node_count; i++) 									\
		if (strcmp(#node_name, XML_NAME(tabid, i)) == 0)						\
			XML_VALUE(tabid, i) = val; }

// XML node comment initialization (optional)
#define SET_XML_NODE_COMMENT(tabid, node_name, val) {							\
	for (i=0; i<xml_##tabid.node_count; i++) 									\
		if (strcmp(#node_name, XML_NAME(tabid, i)) == 0)						\
			XML_COMMENT(tabid, i) = val; }


/*
	XML Node tables definitions

	We'll use the trick, of using a single "init" define, documented at
	http://infocenter.arm.com/help/index.jsp?topic=/com.arm.doc.faqs/ka10134.html
	to ensure that the tables are declared as extern by default, and instantiated
	only once, in eschew.c 
*/

#if defined(INIT_XML_ACTUAL_INIT)

// We need a local index variable for the init macro
static int i;

// This macro defines an enum and sets the 2 variables used for tokenization
#define DEFINE_XML_NODES(nodid, ...)											\
	enum { __VA_ARGS__, _xml_##nodid##_end };									\
	static char* _xml_##nodid##_names[_xml_##nodid##_end];						\
	static char* _xml_##nodid##_tokens = #__VA_ARGS__;		

// Creates the xml_tabid[] with the nodes previously defined. 
#define CREATE_XML_TABLE(tabid, nodid, type)									\
	static type _xml_##tabid##_values[_xml_##nodid##_end];						\
	static char* _xml_##tabid##_comments[_xml_##nodid##_end];					\
	typedef struct {enum XML_TYPE node_type; int node_count; s_xml_attr attr;	\
		char** _tokens; char* id; char** name; char** comment;					\
		type* value; xml_node next;} s_xml_##tabid;								\
	s_xml_##tabid xml_##tabid = {t_##type, _xml_##nodid##_end, { NULL, NULL},	\
	&_xml_##nodid##_tokens, #tabid, _xml_##nodid##_names,						\
		_xml_##tabid##_comments, _xml_##tabid##_values, NULL };

// NB: No need to touch .id
#define SET_ATTRIBUTE(tabid)													\
	if (strtok(xml_##tabid.id, XML_ATTRIBUTE_SEPARATOR) != NULL) {				\
		xml_##tabid.attr.name = strtok(NULL, XML_ATTRIBUTE_SEPARATOR);			\
		xml_##tabid.attr.value = strtok(NULL, XML_ATTRIBUTE_SEPARATOR);	}

// A runtime initialization is necessary for the tokenization of the node names
// and their copying into the table
#define INIT_XML_TABLE(tabid) {													\
	if (xml_##tabid.name[0] == NULL) {											\
		xml_##tabid.name[i=0] = strtok(*(xml_##tabid._tokens), " ,\t");			\
		while (xml_##tabid.name[i++] != NULL)									\
			xml_##tabid.name[i] = strtok (NULL, " ,\t"); }						\
	SET_ATTRIBUTE(tabid) if (!link_table((xml_node)&xml_##tabid, &xml_root))	\
		printf("Could not find parent for table %s!!!\n", xml_##tabid.id); }

// At least one table must be defined as root (after it has been created)
// Note that _p_root_name MUST be initialized to NULL, else a sibling is created
#define SET_XML_ROOT(tabid)														\
	xml_node _p_root_table = NULL;	char* _p_root_name = #tabid;				\
	xnode xml_root = { t_xml_node, 1, {NULL, NULL}, NULL, "root",				\
			&_p_root_name, NULL, &_p_root_table, NULL };							

#else

#define DEFINE_XML_NODES(nodid, ...) 											\
	enum { __VA_ARGS__, _xml_##nodid##_end };	

#define CREATE_XML_TABLE(tabid, nodid, type)									\
	typedef struct {enum XML_TYPE node_type; int node_count; s_xml_attr attr;	\
		char** _tokens; char* id; char** name; char** comment; type* value;		\
		xml_node next;}	s_xml_##tabid;	extern s_xml_##tabid xml_##tabid; 

#define SET_XML_ROOT(table) extern xnode xml_root;

#endif

// Debug printout of a node tree
#define PRINT_XML_TABLE(tabid)													\
	for (i=0; i<xml_##tabid.node_count; i++) {									\
		printf("node[%d].name = %s\n", i, XML_NAME(tabid, i));					\
		printf("node[%d].value = %X\n", i, XML_VALUE(tabid, i)); }

int link_table(xml_node child, xml_node potential_parent);
int read_xml(const char* filename);
int write_xml(const char* filename);

#ifdef	__cplusplus
}
#endif

