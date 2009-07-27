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
 */

/*
                             USAGE NOTES
	* GOAL:
	This is a XML simple DOM tables constructor + runtime parser wrapper
	The goal here is to create XML node tables that are available at compilation 
	time (so that they can be referenced statically in the program) and provide
	the helper classes to populate these nodes from an XML file at runtime.
	Eschew is typically used to set application options from an XML conf file.

	* ESCHEW vs SCEW, or why not use SCEW (Simple C Expat Wrapper) instead?
	1. Scew's node values are string only, whereas Eschew can define and populate 
	   any type
	2. Scew's nodes are only available at runtime => massive overhead everytime you
	   access a simple value. If you define user input controls in the XML, this is
	   not a viable option. In Eschew on the other hand, table values are *directly*
	   accessible at compilation time, in their declared type, whilst still being 
	   populated from the XML at runtime
	3. Scew provide writing to XML, mutliple attribute handling and other features
	   which are not really required for a simple xml options readout operation
	3. As the nodes you define are automatically added as enums, you must make sure
	   that your node names don't conflict with keywords or variable names

	Once again, the main advantage of Eschew is that, after the XML file has been 
	read, NO string	or cycle consuming resolution intervenes to access the table 
	values. They are available like any other regular C array of values (with the
	node names defined as enum for you to facilitate access). 
	For instance, if you use the Eschew macros to create the xml_keys_player_1 
	XML table (which includes the node "dir_up"), you can directly use in your 
	source:

		if (current_key = xml_keys_player_1.value[dir_up])
			move_player_forward();

	* LIMITATIONS
	The following limitations are in effect when using this XML wrapper:
	- Only one atribute can be declared per node, which is used to differentiate 
	  between nodes of the same name. See example below for the "keys" node
	- Child nodes of a specific branch must all be of the same type (because they
	  all belong to the same C table, which of course can only have one type in C)
	  However, if you declare an integer xml tablein Eschew, boolean values like
	  "true", "enabled", "on", "false", "disable", "off" will be detected and set 
	  accordingly (i.e. false -> 0, true -> non zero)
	- node definitions must follow the logical order, i.e. parent nodes must be 
	  instantiated before of their children, for the macros to work.
	- You cannot use underscores in your base table names, as underscores are used
	  to separate attributes properties (but if you feel this is too much of a 
	  limitation, you can change the XML_ATTRIBUTE_SEPARATOR definition to set the
	  attribute separator to something else). You are free to use underscore in
	  node names.
	- You must use one of the xml_#### types when defining your table. If you look
	  at the .h, you will see that there are just a redefinition for what you'd 
	  expect (eg: xml_unsigned_char is a redefintion of "unsigned char"). The 
	  reason is that any type used for your defintions must be a single word.

	* EXAMPLE
	Suppose you want to make the following configuration options available to your 
	program:

	<?xml version="1.0" encoding="utf-8"?>
	<config>
	  <resolution>
	    <scr_width>1280</scr_width>
		<scr_height>1024</scr_height>
		<fullscreen>disabled</fullscreen>
	  </resolution>
	  <keys player="1">
	    <dir_left>&#x8C;</dir_left>
		<dir_right>&#x8E;</dir_right>
		<dir_up>&#x8D;</dir_up>
		<dir_down>&#x8F;</dir_down>
	  </keys>
	  <keys player="2">
	    <left>a</left>
		<right>d</right>
		<up>w</up>
		<down>s</down>
	  </keys>
	  <messages>
	    <msg1>ha ha! pwnd!</msg1>
		<msg2>%s has left the game</msg2>
	  </messages>
    </config>

	With Eschew, you simply need to create a header with the following:

	DEFINE_XML_NODES(config_nodes, resolution, keys, messages)
	// Table ot tables => xml_node type
	CREATE_XML_TABLE(config, config_nodes, xml_node) 
	// config is the root node => declare it as such
	SET_XML_ROOT(config)

	DEFINE_XML_NODES(res_nodes, scr_width, scr_height, fullscreen)
	// Integers or booleans => xml_int
	CREATE_XML_TABLE(resolution, res_nodes, xml_int)

	DEFINE_XML_NODES(keys_nodes, dir_left, dir_right, dir_up, dir_down)
	// Single character values => xml_unsigned_char
	CREATE_XML_TABLE(keys_player_1, keys_nodes, xml_unsigned_char)
	CREATE_XML_TABLE(keys_player_2, keys_nodes, xml_unsigned_char)

	DEFINE_XML_NODES(msg_nodes, msg1, msg2)
	// Strings => xml_string
	CREATE_XML_TABLE(messages, msg_nodes, xml_string)


	This results in the creation of the following tables which you can
	use	straight away (with the content initialized at runtime)

	int				resolution.value[3] = {1280, 1024, 0};
	unsigned char	keys_player_1.value[4] = {0x8C, 0x8E, 0x8D, 0x8F};
	unsigned char	keys_player_2.value[4] = {'a', 'd', 'w', 's'};
	char*			messages.values[2] = { "ha ha! pwnd!", "%s has left the game"};

	The following enums are also automatically set

	enum { resolution, keys_player_1, keys_player_2, messages };
	enum { dir_left, dir_right, dir_up, dir_down };
	enum { msg1, msg2 };

	If needed, the names for the nodes are also available in 
	xml_<table_name>.name[i], as well as the parent table (xml_config)


	Then in the C counterpart:

	#define INIT_XML_ACTUAL_INIT
	#include "eschew.h"
	#include "the_header_you_created_above.h"

	void init_xml_config()
	{
		INIT_XML_TABLE(config);
		INIT_XML_TABLE(resolution);
		INIT_XML_TABLE(keys_player_1);
		INIT_XML_TABLE(keys_player_2);
		INIT_XML_TABLE(messages);
	}

	Call the function above at runtime, call read_xml() with the name of 
	your XML config file and make sure you include your 2 headers (without
	the ACTUAL_INIT define) in all the source you plan to use your tables,
	That's it!

*/

#pragma once

#ifdef	__cplusplus
extern "C" {
#endif

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


// Recursive part of the XML_CAST macro
#define REC(tab, idx, val, typ, def, rem)										\
	if (typ == t_##def) ((def*)tab)[idx] = (def)(val); else rem

// Parentheses - always a great fun (also the reason why NOBODY uses Lisp)
#define XML_CAST_TO_BYTE(tab, idx, val, typ)									\
REC(tab, idx, val, typ, xml_unsigned_char,										\
 REC(tab, idx, val, typ, xml_char, printf("illegal cast\n"))					\
)

#define XML_CAST_TO_INTEGER(tab, idx, val, typ)									\
REC(tab, idx, val, typ, xml_unsigned_short,										\
 REC(tab, idx, val, typ, xml_short,												\
  REC(tab, idx, val, typ, xml_unsigned_int,										\
   REC(tab, idx, val, typ, xml_int,												\
    REC(tab, idx, val, typ, xml_unsigned_long,									\
     REC(tab, idx, val, typ, xml_long,											\
      REC(tab, idx, val, typ, xml_unsigned_long_long,							\
       REC(tab, idx, val, typ, xml_long_long, printf("illegal cast\n"))			\
	  )																			\
	 )																			\
	)																			\
   )																			\
  )																				\
 )																				\
)

#define XML_CAST_TO_FLOATING(tab, idx, val, typ)								\
REC(tab, idx, val, typ, xml_float,												\
 REC(tab, idx, val, typ, xml_double, printf("illegal cast\n"))					\
)

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
	struct _xnode **value;
	struct _xnode *next;} xnode;


// Useful access macros
#define XML_VALUE(tabid, i)	xml_##tabid.value[i]
#define XML_NAME(tabid, i)	xml_##tabid.name[i]

// XML default value initialization (optional)
#define SET_XML_NODE_DEFAULT(tabid, node_name, val) {							\
	for (i=0; i<xml_##tabid.node_count; i++) 									\
		if (strcmp(#node_name, XML_NAME(tabid, i)) == 0)						\
			XML_VALUE(tabid, i) = val; }


/////////////////////////////////////////////////////////////////////////////////
// XML Node tables
//
// An example is worth a thousand words:
// INIT_XML_NODES(table, u8, node1, node2, node3) ==>
//   enum { node1, node2, node3, _xml_table_end };
//	 typedef struct { char* name; u8 value; } s_xml_table_element
//   static char* _table_names = "node1, node2, node3";
//	 int _xml_table_type = type_u8;
//   s_xml_table_element xml_table[_xml_table_end];
//
// Then at runtime, use SET_XML_NODE_NAMES() to break down the _table_names
// string into "node1", "node2", "node3" string tokens, with which we then 
// populate the table[].name values.
//
// We also use the smart trick documented at
// http://infocenter.arm.com/help/index.jsp?topic=/com.arm.doc.faqs/ka10134.html
// to ensure that the tables are declared as extern by default, and instantiated
// only once, in eschew.c 
//

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
	typedef struct {enum XML_TYPE node_type; int node_count; s_xml_attr attr;	\
		char** _tokens; char* id; char** name; type* value; xml_node next;}		\
		s_xml_##tabid;															\
	s_xml_##tabid xml_##tabid = {t_##type, _xml_##nodid##_end, { NULL, NULL},	\
	&_xml_##nodid##_tokens, #tabid, _xml_##nodid##_names, _xml_##tabid##_values,\
		NULL };

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
			&_p_root_name, &_p_root_table, NULL };									

#else

#define DEFINE_XML_NODES(nodid, ...) 											\
	enum { __VA_ARGS__, _xml_##nodid##_end };	

#define CREATE_XML_TABLE(tabid, nodid, type)									\
	typedef struct {enum XML_TYPE node_type; int node_count; s_xml_attr attr;	\
		char** _tokens; char* id; char** name; type* value; xml_node next;}		\
		s_xml_##tabid;	extern s_xml_##tabid xml_##tabid; 

#define SET_XML_ROOT(table) extern xnode xml_root;

#endif

// Debug printout of a node tree
#define PRINT_XML_TABLE(tabid)													\
	for (i=0; i<xml_##tabid.node_count; i++) {									\
		printf("node[%d].name = %s\n", i, XML_NAME(tabid, i));					\
		printf("node[%d].value = %X\n", i, XML_VALUE(tabid, i)); }

/*
	Function prototypes
 */
int utf8_to_u16_nz(const char* source, unsigned short* target, size_t len);
int utf8_to_u16(const char* source, unsigned short* target);
int link_table(xml_node child, xml_node potential_parent);
int read_xml(const char* filename);

#ifdef	__cplusplus
}
#endif

