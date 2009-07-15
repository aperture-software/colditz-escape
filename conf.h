/*
	Eschew = Even Simpler C Heuristic Expat Wrapper

	* GOAL:
	This is a XML simple DOM constructor + runtime parser wrapper
	The goal here is to create XML node tables that are available at compilation 
	time (so that they can be referenced statically in the program) and provide
	the helper classes to populate these nodes from an XML file at runtime.
	This is typically used to set application options from an XML configuration 
	file.

	* ESCHEW vs SCEW: Why not use SCEW (Simple C Expat Wrapper)?
	1. Scew's node values are string only / we want to define any type
	2. Scew's nodes are only available at runtime => massive overhead everytime you
	   want to access a value, which you can't afford if you define your user input
	   controls in the XML. In Eschew, table values are directly accessible at 
	   compilation time, in their declared type, whilst being updated from the XML
	   at runtime
	3. Scew provide writing to XML, mutliple attribute hnadling and other features
	   which are not really required for a simple xml options readout operation

	Note that, after the XML file has been read to populate the tables, NO string
	resolution intervenes to access the table values (it's all done through enums)
	so after you called the macro to create the xml_keys_player_1 and declare its
	nodes (which includes "dir_up"), you can directly use in your source:

		if (current_key = xml_keys_player_1.value[dir_up])
			jump();

	* LIMITATIONS
	The following limitations are in effect when using this wrapper:
	- Only one atribute can be declared per node, which is then used to differen-
	  tiate between nodes bearing the same name. See example below for the "keys"
	  node
	- Child nodes of a specific branch must all be of the same type (i.e. no 
	  mixing of strings/integers/booleans within the same branch)
	- node definitions must be issued from the top down, i.e. parent node before 
	  any of its children

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




// which is meant to be is only intended for XML conf files, i.e. an XML file
// that is only read, but NEVER written into by our application.
// The DOM-like C structures are also NOT created dynamically at runtime, but 
// instead need to be setup during the application creation. All child nodes
// must have the same type
// 
/////////////////////////////////////////////////////////////////////////////////
*/

#pragma once

#if defined(WIN32)
// Disable the _CRT_SECURE_DEPRECATE warnings of VC++
#pragma warning(disable:4996)
#endif

#ifdef	__cplusplus
extern "C" {
#endif

#define XML_STACK_SIZE 16

/////////////////////////////////////////////////////////////////////////////////
// Custom key mappings
//
// At the end of the day, below are the tables of interest to y'all:
// Standard key = regular (lowercase) ASCII code
// 0x7F         = Del
// [0x80-0x8B]  = [F1-F12] 
// [0x8C-0x8F]  = [Left, Up, Right, Down]
// [0x90-0x94]  = [PgUp, PgDn, Home, End, Insert] 
// [0x95-0x97]  = [Shift, Ctrl, Alt] (*)
// [0x98-0x9A]  = [Mouse Left, Mouse Middle, Mouse Right]
// (*) Because of GLUT's limitations, these key events are ONLY detected
// in conjuction with other keypresses, and cannot be used as standalone
// see http://www.nabble.com/Re%3A-Status-of-modifier-keys-(shift-ctrl-alt)-p6983221.html
// 
// The PSP GLUT key mappings from pspgl are as follows:
// [X] = 'x'
// [O] = 'o'
// [Square] = 'q'
// [Triangle] = 'd'
// [Select] = 's'
// [Start] = 'a'
// [Left, Up, Right, Down] = same as above
// [Left Trigger] = Mouse Left (see above for the actual code)
// [Right Trigger] = Mouse Right (see above for the actual code)
//
// Now, with regards to the overall ugliness of this section, it is ugly because
// of Unicode's total idiocy on not wanting to add common keyboard keys to their 
// charts. Otherwise we would have gone Unicode all the way and dropped the GLUT
// codes & required redefinition.
// As per http://www.mail-archive.com/unicode@unicode.org/msg27037.html:
// "All this is out of topic for Unicode which is only concerned by the encoding
//  of characters, not the scancodes, or virtual keys used by function keys like
//  Arrows or F1 or even the Enter key (which is not by itself mapped directly
//  to a CR Unicode character but first as a virtual key with keyboard state
//  flags or mode), or the definition of bits in keyboard state modes: they are
//  not characters, do not generate plain-text information, so it's normal that
//  they have no Unicode code point associated with them!"
//
// When you have the scope to encode every single character on the planet, and
// you ALREADY have charts that include device control characters, why on earth
// don't you want to define a set of keyboard codes that everyone can agree on.
// Ever heard about my friend, De Facto?!?
// And if you want a rationale for this, how about a glyphset that describes 
// keypresses/mousepresses (using common input devices) for vision impaired
// users for instance?
// 

// We'll use the [0x80-0x9F] section of ASCII for our special codes
#define SPECIAL_KEY_OFFSET1		0x80
// Follows the order in which they are defined in the glut headers
#define SPECIAL_KEY_F1          (GLUT_KEY_F1 - GLUT_KEY_F1 + SPECIAL_KEY_OFFSET1)
#define SPECIAL_KEY_F2          (GLUT_KEY_F2 - GLUT_KEY_F1 + SPECIAL_KEY_OFFSET1)
#define SPECIAL_KEY_F3          (GLUT_KEY_F3 - GLUT_KEY_F1 + SPECIAL_KEY_OFFSET1)
#define SPECIAL_KEY_F4          (GLUT_KEY_F4 - GLUT_KEY_F1 + SPECIAL_KEY_OFFSET1)
#define SPECIAL_KEY_F5          (GLUT_KEY_F5 - GLUT_KEY_F1 + SPECIAL_KEY_OFFSET1)
#define SPECIAL_KEY_F6          (GLUT_KEY_F6 - GLUT_KEY_F1 + SPECIAL_KEY_OFFSET1)
#define SPECIAL_KEY_F7          (GLUT_KEY_F7 - GLUT_KEY_F1 + SPECIAL_KEY_OFFSET1)
#define SPECIAL_KEY_F8          (GLUT_KEY_F8 - GLUT_KEY_F1 + SPECIAL_KEY_OFFSET1)
#define SPECIAL_KEY_F9          (GLUT_KEY_F9 - GLUT_KEY_F1 + SPECIAL_KEY_OFFSET1)
#define SPECIAL_KEY_F10         (GLUT_KEY_F10 - GLUT_KEY_F1 + SPECIAL_KEY_OFFSET1)
#define SPECIAL_KEY_F11         (GLUT_KEY_F11 - GLUT_KEY_F1 + SPECIAL_KEY_OFFSET1)
#define SPECIAL_KEY_F12         (GLUT_KEY_F12 - GLUT_KEY_F1 + SPECIAL_KEY_OFFSET1)
#define SPECIAL_KEY_OFFSET2		(SPECIAL_KEY_F12 + 1)
#define SPECIAL_KEY_LEFT        (GLUT_KEY_LEFT - GLUT_KEY_LEFT + SPECIAL_KEY_OFFSET2)
#define SPECIAL_KEY_UP          (GLUT_KEY_UP - GLUT_KEY_LEFT + SPECIAL_KEY_OFFSET2)
#define SPECIAL_KEY_RIGHT       (GLUT_KEY_RIGHT - GLUT_KEY_LEFT + SPECIAL_KEY_OFFSET2)
#define SPECIAL_KEY_DOWN        (GLUT_KEY_DOWN - GLUT_KEY_LEFT + SPECIAL_KEY_OFFSET2)
#define SPECIAL_KEY_PAGE_UP     (GLUT_KEY_PAGE_UP - GLUT_KEY_LEFT + SPECIAL_KEY_OFFSET2)
#define SPECIAL_KEY_PAGE_DOWN   (GLUT_KEY_PAGE_DOWN - GLUT_KEY_LEFT + SPECIAL_KEY_OFFSET2)
#define SPECIAL_KEY_HOME        (GLUT_KEY_HOME - GLUT_KEY_LEFT + SPECIAL_KEY_OFFSET2)
#define SPECIAL_KEY_END	        (GLUT_KEY_END - GLUT_KEY_LEFT + SPECIAL_KEY_OFFSET2)
#define SPECIAL_KEY_INSERT      (GLUT_KEY_INSERT - GLUT_KEY_LEFT + SPECIAL_KEY_OFFSET2)
#define SPECIAL_KEY_SHIFT		(SPECIAL_KEY_INSERT + 1)
#define SPECIAL_KEY_CTRL		(SPECIAL_KEY_INSERT + 2)
#define SPECIAL_KEY_ALT			(SPECIAL_KEY_INSERT + 3)
// Same for mouse buttons (PSP L&R triggers are mapped to mouse buttons in GLUT)
#define SPECIAL_MOUSE_BUTTON_BASE	(SPECIAL_KEY_ALT + 1)
#define SPECIAL_LEFT_MOUSE_BUTTON	(GLUT_LEFT_BUTTON - GLUT_LEFT_BUTTON + SPECIAL_MOUSE_BUTTON_BASE)
#define SPECIAL_MIDDLE_MOUSE_BUTTON	(GLUT_MIDDLE_BUTTON - GLUT_LEFT_BUTTON + SPECIAL_MOUSE_BUTTON_BASE)
#define SPECIAL_RIGHT_MOUSE_BUTTON	(GLUT_RIGHT_BUTTON - GLUT_LEFT_BUTTON + SPECIAL_MOUSE_BUTTON_BASE)



// Non official debug keys
#define KEY_DEBUG_PRINT_POS		'p'
#define KEY_DEBUG_BONANZA		'#'
#define KEY_DEBUG_CATCH_HIM		'c'

//#define opt_skip_intro			options.node[skip_intro].value
//#define opt_enhanced_guards		options.node[enhanced_guards].value
//#define opt_picture_corners		options.node[picture_corners].value


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
// only once, in conf.c 
//

// We need the following *SINGLE WORD* types to be defined, so that we can
// figure out the type of our XML node tables at runtime
//#define pointer  void*
#define string	 char*

#define xml_unsigned_char		unsigned char
#define xml_unsigned_char_ptr	unsigned char*
#define xml_char				char
#define xml_char_ptr			char*
#define xml_unsigned_short		unsigned short
#define xml_unsigned_short_ptr	unsigned short*
#define xml_short				short
#define xml_short_ptr			short*
#define xml_int					int
#define xml_int_ptrs			int*
#define xml_unsigned_int		unsigned int
#define xml_unsigned_int_ptr	unsigned int*


// Ask the user to create a def file? But then what of the case switch
#blah(yada, yodo)
#define yada_yodo				yada yodo
#define type_yada_yodo

case type_yada_yodo				(yada yodo)value


enum XML_TYPE {
	type_bool,
	type_u8,
	type_s8,
	type_u16,
	type_s16,
	type_u32,
	type_s32,
	type_u64,
	type_s64,
	type_string,
//	type_pointer,
	type_xml_node_ptr,
};

typedef struct {
	char* name;
	char* value;
} s_xml_attr;


typedef struct _xml_node { enum XML_TYPE node_type; int node_count; s_xml_attr attr;	\
		char** id; char** name; struct _xml_node **value; } xml_node;
#define xml_node_ptr xml_node*

// We need to keep a linked list of all the xml tables defined so that we can link them
// to one another at runtime (NB: we could probably do without it, but we'd have to ask
// people to define their XML tables starting from the bottom => not desirable)
typedef struct _xml_list { char* name; xml_node* table; struct _xml_list *next; } xml_list;



/*


#define GET_NODE_NAME(ptr, xml_type, i) \
((char*)(*((xml_node_el**)((xml_node*)ptr)->nodes))) + i*s_xml_el_size[xml_type]
 

//(((xml_node*)ptr)->nodes)

//GET_NODE_NAME(&xml_root, type_xml_node, 0) 
//(char*)(((xml_node*)&xml_root)->nodes +	0*(sizeof(char*) + xml_type_size[type_xml_node]))

#define GET_CHILD_NODE(ptr, i) ((xml_node_el*)((xml_node*)ptr)->nodes)[i].value

*/

//#define GET_NODE_NAME(ptr, i) (*(xml_node*)ptr).name[i]
	


/* working
#define CREATE_XML_TABLE(table, type, ...)										\
	enum { __VA_ARGS__, _xml_##table##_end };									\
	static char* _xml_##table##_names = #__VA_ARGS__;							\
	static int _xml_##table##_type = type_##type;								\
	typedef struct {char* name; type value;} s_xml_##table##_element ;			\
	typedef struct {enum XML_TYPE node_type; int node_count; s_xml_attr attr;	\
		s_xml_##table##_element node[_xml_##table##_end];} s_xml_##table;		\
	static s_xml_##table xml_##table;

#define SET_XML_TABLE(table, type, ...)										\
	enum { __VA_ARGS__, _xml_##table##_end };									\
	static char* _xml_##table##_names = #__VA_ARGS__;							\
	static int _xml_##table##_type = type_##type;								\
	typedef struct {char* name; type value;} s_xml_##table##_element ;			\
	typedef struct {enum XML_TYPE node_type; int node_count; s_xml_attr attr;	\
		s_xml_##table##_element node[_xml_##table##_end];} s_xml_##table;		
#define INSTANTIATE_XML_TABLE_MULTIPLE(table, attr, value)						\
	static char* _xml_##table_##value_attr_value = #value;						\
	static char* _xml_##table_##value_attr_name = #attr;						\
	s_xml_##table xml_##table_##value;
#define INSTANTIATE_XML_TABLE_SINGLE(table)										\
	static char* _xml_##table_attr_value = NULL;								\
	static char* _xml_##table_attr_name = NULL;									\
	s_xml_##table xml_##table_##value;



#else
#define CREATE_XML_TABLE(table, type, ...)										\
	enum { __VA_ARGS__, _xml_##table##_end };									\
	extern int _xml_##table##_type;												\
	typedef struct {char* name; type value;} s_xml_##table##_element ;			\
	typedef struct {enum XML_TYPE node_type; int node_count;					\
		s_xml_##table##_element node[_xml_##table##_end];} s_xml_##table;		\
	extern s_xml_##table xml_##table;




*/

#define XML_VALUE(tabid, i)			xml_##tabid.value[i]
#define XML_NAME(tabid, i)			xml_##tabid.name[i]

// Short(?)cut defines for the main program
#define KEY_FIRE				XML_VALUE(controls, key_fire)
#define KEY_TOGGLE_WALK_RUN		XML_VALUE(controls, key_toggle_walk_run)
#define KEY_PAUSE				XML_VALUE(controls, key_pause)
#define KEY_INVENTORY_LEFT		XML_VALUE(controls, key_inventory_cycle_left)
#define KEY_INVENTORY_RIGHT		XML_VALUE(controls, key_inventory_cycle_right)
#define KEY_INVENTORY_PICKUP	XML_VALUE(controls, key_pickup)
#define KEY_INVENTORY_DROP		XML_VALUE(controls, key_dropdown)
#define KEY_SLEEP				XML_VALUE(controls, key_sleep)
#define KEY_STOOGE				XML_VALUE(controls, key_stooge)
#define KEY_ESCAPE				XML_VALUE(controls, key_escape)
#define KEY_PRISONERS_LEFT		XML_VALUE(controls, key_prisoners_cycle_left)
#define KEY_PRISONERS_RIGHT		XML_VALUE(controls, key_prisoners_cycle_right)
#define KEY_BRITISH				XML_VALUE(controls, key_select_british)
#define KEY_FRENCH				XML_VALUE(controls, key_select_french)
#define KEY_AMERICAN			XML_VALUE(controls, key_select_american)
#define KEY_POLISH				XML_VALUE(controls, key_select_polish)
#define KEY_DIRECTION_LEFT		XML_VALUE(controls, key_direction_left)
#define KEY_DIRECTION_RIGHT		XML_VALUE(controls, key_direction_right)
#define KEY_DIRECTION_UP		XML_VALUE(controls, key_direction_up)
#define KEY_DIRECTION_DOWN		XML_VALUE(controls, key_direction_down)

#define SET_XML_NODE_DEFAULT(tabid, node_name, val) {							\
	for (i=0; i<xml_##tabid.node_count; i++) 									\
		if (strcmp(#node_name, XML_NAME(tabid, i)) == 0)						\
			XML_VALUE(tabid, i) = val; }


#if defined(INIT_XML_ACTUAL_INIT)

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
	s_xml_##tabid xml_##tabid = {type_##type, _xml_##nodid##_end, { NULL, NULL},\
		&_xml_##nodid##_tokens, _xml_##nodid##_names, _xml_##tabid##_values };	

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
	xml_##tabid.id = &_xml_my_name_##tabid;										\
	if (!link_table((xml_node*)&xml_##tabid, &xml_root))						\
		printf("could not find parent for %s!!!\n", *xml_##tabid.id); }

// At least one table must be defined as root (after it has been created)
#define SET_XML_ROOT(tabid)														\
	xml_node* _p_root_table = (xml_node*) &xml_##tabid;							\
	char* _p_root_name = #tabid; char* _p_root_id = "root";						\
	xml_node xml_root = { type_xml_node_ptr, 1, {NULL, NULL}, &_p_root_id,		\
			&_p_root_name, &_p_root_table };									

#else

#define DEFINE_XML_NODES(nodid, ...)											\
	enum { __VA_ARGS__, _xml_##nodid##_end };	

#define CREATE_XML_TABLE(tabid, nodid, type)									\
	typedef struct {char* name; type value;} s_xml_##tabid##_el;				\
	typedef struct {enum XML_TYPE node_type; int node_count; s_xml_attr attr;	\
		char** tokens; char** name; type* value; } s_xml_##tabid;				\
	extern s_xml_##tabid xml_##tabid; 

#define SET_XML_ROOT(table) extern xml_node xml_root;

#endif


// Debug printout of the node tree
#define PRINT_XML_TABLE(tabid)													\
	for (i=0; i<xml_##tabid.node_count; i++) {									\
		printf("node[%d].name = %s\n", i, XML_NAME(tabid, i));					\
		printf("node[%d].value = %X\n", i, XML_VALUE(tabid, i)); }



/////////////////////////////////////////////////////////////////////////////////
// XML tables definitions
//

// root node
DEFINE_XML_NODES(config_nodes, runtime,		\
				 options,					\
				 controls)
CREATE_XML_TABLE(config, config_nodes, xml_node_ptr) 
SET_XML_ROOT(config)

// General program options
DEFINE_XML_NODES(options_nodes, skip_intro,	\
				 enhanced_guards,			\
				 picture_corners)
CREATE_XML_TABLE(options, options_nodes, bool)

// User input mappings
DEFINE_XML_NODES(controls_nodes, key_fire,	
				 key_toggle_walk_run,		\
				 key_pause,					\
				 key_sleep,					\
				 key_stooge,				\
				 key_direction_left,		\
				 key_direction_right,		\
				 key_direction_up,			\
				 key_direction_down,		\
				 key_inventory_cycle_left,	\
				 key_inventory_cycle_right,	\
				 key_pickup,				\
				 key_dropdown,				\
				 key_escape,				\
				 key_prisoners_cycle_left,	\
				 key_prisoners_cycle_right,	\
				 key_select_british,		\
				 key_select_french,			\
				 key_select_american,		\
				 key_select_polish)
CREATE_XML_TABLE(controls, controls_nodes, u8)

/////////////////////////////////////////////////////////////////////////////////
// Function prototypes
//
int utf8_to_u16_nz(const char* source, u16* target, size_t len);
int utf8_to_u16(const char* source, u16* target);
int readconf(const char* filename);
void init_xml_config(); 

#ifdef	__cplusplus
}
#endif

