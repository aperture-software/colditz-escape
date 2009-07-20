/*
	Eschew = Even Simpler C-Heuristic Expat Wrapper

	* GOAL:
	This is a XML simple DOM constructor + runtime parser wrapper
	The goal here is to create XML node tables that are available at compilation 
	time (so that they can be referenced statically in the program) and provide
	the helper classes to populate these nodes from an XML file at runtime.
	This is typically used to set application options from an XML configuration 
	file.

	* ESCHEW vs SCEW, or why not use SCEW (Simple C Expat Wrapper) instead?
	1. Scew's node values are string only / we want to define any type
	2. Scew's nodes are only available at runtime => massive overhead everytime you
	   want to access a value, which you can't afford if you define your user input
	   controls in the XML. In Eschew, table values are directly accessible at 
	   compilation time, in their declared type, whilst being updated from the XML
	   at runtime
	3. Scew provide writing to XML, mutliple attribute handling and other features
	   which are not really required for a simple xml options readout operation

	Note that, after the XML file has been read to populate the tables, NO string
	resolution intervenes to access the table values (it's all done through enums)
	so after you called the macro to create the xml_keys_player_1 and declare its
	nodes (which includes "dir_up"), you can directly use in your source:

		if (current_key = xml_keys_player_1.value[dir_up])
			move_player_forward();

	* LIMITATIONS
	The following limitations are in effect when using this wrapper:
	- Only one atribute can be declared per node, which is then used to differen-
	  tiate between nodes bearing the same name. See example below for the "keys"
	  node
	- Child nodes of a specific branch must all be of the same type, i.e. no 
	  mixing of strings/integers/booleans within the same branch. However, boolean
	  values ("true", "enabled", "false", "off") will be detected within integer
	  values and set accordingly
	- node definitions must be issued from the top down, i.e. parent node before 
	  any of its children, for the macros to work
	- You cannot use underscores in your base table names, as underscores are used
	  to separate attributes properties (if this is too much of a limitation, this
	  can be changed by modifying the XML_ATTRIBUTE_SEPARATOR macro

	* EXAMPLE
	Let's suppose you want to make the following configuration options available
	to your program:

	<?xml version="1.0" encoding="utf-8"?>
	<config>
	  <resolution>
	    <scr_width>1280</scr_width>
		<scr_height>1024</scr_height>
		<fullscreen>1</fullscreen>
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

	With Eschew, you simply need to issue the following in one of your headers:

	DEFINE_XML_NODES(config_nodes, resolution, keys_player_1, keys_player_2, messages)
	CREATE_XML_TABLE(config, config_nodes, xml_node_ptr) 
	SET_XML_ROOT(config)

	DEFINE_XML_NODES(res_nodes, scr_width, scr_height, fullscreen)
	CREATE_XML_TABLE(resolution, res_nodes, int)

	DEFINE_XML_NODES(keys_nodes, dir_left, dir_right, dir_up, dir_down)
	CREATE_XML_TABLE(keys_player_1, keys_nodes, u8)
	CREATE_XML_TABLE(keys_player_2, keys_nodes, u8)

	DEFINE_XML_NODES(msg_nodes, msg1, msg2)
	CREATE_XML_TABLE(messages, msg_nodes, string)

	This results in the creation of the following enum and tables which you 
	can use	straight away
	enum { resolution, keys_player_1, keys_player_2, messages };
	enum { dir_left, dir_right, dir_up, dir_down };
	enum { msg1, msg2 };
	char*     xml_config.names[4];  // containing the names of the nodes
	xml_node* xml_config.values[4]; // pointer to the child tables
	char*     xml_resolution[3]; 
	int
		{ "resolution", "keys_player_1", "keys_player_2", "messages" };
	u8    xml_config.values[4] = 
		{ &xml_resolution, &xml_keys_player_1, &xml_keys_player2, &xml_messages };
	xml_keys_player_1 = [left, right, up, down];
	xml_keys_player_2 = [left, right, up, down];
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
#define xml_node_ptr			xml_node*
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
	t_xml_node_ptr,
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
typedef struct _xml_node { 
	enum XML_TYPE node_type; 
	int node_count; 
	s_xml_attr attr;	
	char** id; 
	char** name; 
	struct _xml_node **value; } xml_node;
#define xml_node_ptr xml_node*

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
	static char* _xml_my_name_##tabid = #tabid;									\
	typedef struct {enum XML_TYPE node_type; int node_count; s_xml_attr attr;	\
		char** id; char** name; type* value; } s_xml_##tabid;					\
	s_xml_##tabid xml_##tabid = {t_##type, _xml_##nodid##_end, { NULL, NULL},	\
	&_xml_##nodid##_tokens, _xml_##nodid##_names, _xml_##tabid##_values };

// NB: id has already been initialized, and will be truncated automatically if needed
#define SET_ATTRIBUTE(tabid)													\
	if (strtok(_xml_my_name_##tabid, XML_ATTRIBUTE_SEPARATOR) != NULL) {		\
		xml_##tabid.attr.name = strtok(NULL, XML_ATTRIBUTE_SEPARATOR);			\
		xml_##tabid.attr.value = strtok(NULL, XML_ATTRIBUTE_SEPARATOR);	}

// A runtime initialization is necessary for the tokenization of the node names
// and their copying into the table
// Before this function, id is set to the token list. After that, id is the name 
// of the table itsel
//TO_DO: init attribute names
#define INIT_XML_TABLE(tabid) {													\
	if (xml_##tabid.name[0] == NULL) {											\
		xml_##tabid.name[i=0] = strtok(*(xml_##tabid.id), " ,\t");				\
		while (xml_##tabid.name[i++] != NULL)									\
			xml_##tabid.name[i] = strtok (NULL, " ,\t"); }						\
	xml_##tabid.id = &_xml_my_name_##tabid;	SET_ATTRIBUTE(tabid)				\
	if (!link_table((xml_node*)&xml_##tabid, &xml_root))						\
		printf("could not find parent for %s!!!\n", *xml_##tabid.id); }


// At least one table must be defined as root (after it has been created)
#define SET_XML_ROOT(tabid)														\
	xml_node* _p_root_table = (xml_node*) &xml_##tabid;							\
	char* _p_root_name = #tabid; char* _p_root_id = "root";						\
	xml_node xml_root = { t_xml_node_ptr, 1, {NULL, NULL}, &_p_root_id,			\
			&_p_root_name, &_p_root_table };									

#else

#define DEFINE_XML_NODES(nodid, ...) 											\
	enum { __VA_ARGS__, _xml_##nodid##_end };	


#define CREATE_XML_TABLE(tabid, nodid, type)									\
	extern char* _xml_my_name_##tabid;											\
	typedef struct {enum XML_TYPE node_type; int node_count; s_xml_attr attr;	\
		char** id; char** name; type* value; } s_xml_##tabid;					\
	extern s_xml_##tabid xml_##tabid; 

#define SET_XML_ROOT(table) extern xml_node xml_root;

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
int read_xml(const char* filename);
int link_table(xml_node* child, xml_node* potential_parent);

#ifdef	__cplusplus
}
#endif

