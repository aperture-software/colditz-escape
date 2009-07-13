/*
	Read and populate the XML config file

	This is an XML simple DOM constructor + runtime parser wrapper
	The goal here is to create XML node tables that are available at compilation 
	time (so that they can be referenced statically in the program) and provide
	the helper classes to populate these nodes from an XML file at runtime.
	This is typically used to set application options from an XML configuration 
	file.

	Note that, after the XML file has been read to populate the tables, NO string
	resolution intervenes to access the table values (it's all done through enum
	so you can actually manually use, which is what the XML_VAL() macro defines: 
		((s_xml_keys*) xml_keys_player_1.nodes)[right].value
	In the statement above, "right" is the value of an enum and NOT a string!

	The following limitations are in effect when using this wrapper:
	- Regardless of where they are placed in the XML tree, no two XML nodes can 
	  have the same name (as every node name is part of a specific enum), 
	- Child nodes of a specific branch must all be of the same type
	- Using multiple instances of the same node is possible (except for root) but
	  each node must be identified by a unique attribute within the same level 
	  branch (see example below for multiple instances of the "keys" node)
 
	It is meant to help setup and make the link between compilation time option
	tables, and these options definition


	eg:
	<?xml version="1.0" encoding="utf-8"?>
	<config>
	  <resolution>
	    <width>1280</width>
		<height>1024</height>
		<fullscreen>1</fullscreen>
	  </resolution>
	  <keys player="1">
	    <left>&#x8C;</left>
		<right>&#x8E;</right>
		<up>&#x8D;</up>
		<down>&#x8F;</down>
	  </keys>
	  <keys player="2">
	    <left>a</left>
		<right>d</right>
		<up>w</up>
		<down>s</down>
	  </keys>
	  <messages lang="en">
	    <msg1>ha ha! pwnd!</msg1>
		<msg2>%s has left the game</msg2>
	  </messages>
    </config>

	In your program, you should then use the following macros to initialize and
	instantiate the tables:
	SET_XML_TABLE_ROOT(config);
	CREATE_XML_TABLE(config, xml_node, resolution, keys_player_1, keys_player_2)
	CREATE_XML_TABLE(resolution, int, width, height, fullscreen)
	CREATE_XML_TABLE_WITH_ATTR(keys, player, 1, int, left, right, up, down)
	CREATE_XML_TABLE_WITH_ATTR(keys, player, 2, int, left, right, up, down)

	This will create the following tables
	xml_node xml_config[3] = [resolution, keys_player_1, keys_player_2];
	int xml_resolution[3] = [width, height, fullscreen];
	int xml_keys_player_1[4] = [left, right, up, down];
	int xml_keys_player_2[4] = {left, right, up, down};




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

#define XML_VALUE(table, id)	xml_##table.node[id].value
#define XML_NAME(table, id)		xml_##table.node[id].name

// Short(?)cut defines for the main program
#define KEY_FIRE				XML_VALUE(controls, key_fire)
#define KEY_TOGGLE_WALK_RUN		xml_controls.node[key_toggle_walk_run].value
#define KEY_PAUSE				xml_controls.node[key_pause].value
#define KEY_INVENTORY_LEFT		xml_controls.node[key_inventory_cycle_left].value
#define KEY_INVENTORY_RIGHT		xml_controls.node[key_inventory_cycle_right].value
#define KEY_INVENTORY_PICKUP	xml_controls.node[key_pickup].value
#define KEY_INVENTORY_DROP		xml_controls.node[key_dropdown].value
#define KEY_SLEEP				xml_controls.node[key_sleep].value
#define KEY_STOOGE				xml_controls.node[key_stooge].value
#define KEY_ESCAPE				xml_controls.node[key_escape].value
#define KEY_PRISONERS_LEFT		xml_controls.node[key_prisoners_cycle_left].value
#define KEY_PRISONERS_RIGHT		xml_controls.node[key_prisoners_cycle_right].value
#define KEY_BRITISH				xml_controls.node[key_select_british].value
#define KEY_FRENCH				xml_controls.node[key_select_french].value
#define KEY_AMERICAN			xml_controls.node[key_select_american].value
#define KEY_POLISH				xml_controls.node[key_select_polish].value
#define KEY_DIRECTION_LEFT		xml_controls.node[key_direction_left].value
#define KEY_DIRECTION_RIGHT		xml_controls.node[key_direction_right].value
#define KEY_DIRECTION_UP		xml_controls.node[key_direction_up].value
#define KEY_DIRECTION_DOWN		xml_controls.node[key_direction_down].value

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
#define pointer  void*
#define string	 char*
#define xml_node void*
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
	type_pointer,
	type_xml_node
};

/*
	SET_XML_TABLE(table, type, ...)
	INSTANTIATE_XML_TABLE(table, attr, value)
	CREATE_XML_TABLE_SINGLE()
	CREATE_XML_TABLE_DUPLICATE_BASE()
	CREATE_XML_TABLE_DUPLICATE()
 */

typedef struct {
	char* name;
	char* value;
} s_xml_attr;

#if defined(INIT_XML_ACTUAL_INIT)
#define CREATE_XML_TABLE(table, type, ...)										\
	enum { __VA_ARGS__, _xml_##table##_end };									\
	static char* _xml_##table##_names = #__VA_ARGS__;							\
	static int _xml_##table##_type = type_##type;								\
	typedef struct {char* name; type value;} s_xml_##table##_element ;			\
	typedef struct {enum XML_TYPE node_type; int node_count; s_xml_attr attr;	\
		s_xml_##table##_element node[_xml_##table##_end];} s_xml_##table;		\
	s_xml_##table xml_##table;
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
#define SET_XML_ROOT(table)	static char* xml_root = #table;
//	xml_##table.attr.name = _xml_##table_attr_name;								\
//	xml_##table.attr.value = _xml_##table_attr_value;							\

#define INIT_XML_TABLE(table) {													\
	xml_##table.node_type = _xml_##table##_type;								\
	xml_##table.node_count = _xml_##table##_end;								\
	xml_##table.node[i=0].name = strtok (_xml_##table##_names, " ,\t");			\
	while (xml_##table.node[i++].name != NULL)									\
		xml_##table.node[i].name = strtok (NULL, " ,\t"); }		
#else
#define CREATE_XML_TABLE(table, type, ...)										\
	enum { __VA_ARGS__, _xml_##table##_end };									\
	extern int _xml_##table##_type;												\
	typedef struct {char* name; type value;} s_xml_##table##_element ;			\
	typedef struct {enum XML_TYPE node_type; int node_count;					\
		s_xml_##table##_element node[_xml_##table##_end];} s_xml_##table;		\
	extern s_xml_##table xml_##table;
#define SET_XML_ROOT(table) char* xml_root;
#endif


// Debug printout of the node tree
#define PRINT_XML_TABLE(table)												\
	for (i=0; i<_xml_##table##_end; i++) {									\
		printf("node[%d].name = %s\n", i, xml_##table.node[i].name);		\
		printf("node[%d].value = %X\n", i, xml_##table.node[i].value); }



/////////////////////////////////////////////////////////////////////////////////
// XML tables definitions
//

// root node
CREATE_XML_TABLE(config, xml_node, runtime,		\
				 options,						\
				 controls)
SET_XML_ROOT(config)


// General program options
CREATE_XML_TABLE(options, bool, skip_intro,	\
				 enhanced_guards,			\
				 picture_corners)

// User input mappings
CREATE_XML_TABLE(controls, u8, key_fire,	\
			   key_toggle_walk_run,			\
			   key_pause,					\
			   key_sleep,					\
			   key_stooge,					\
			   key_direction_left,			\
			   key_direction_right,			\
			   key_direction_up,			\
			   key_direction_down,			\
			   key_inventory_cycle_left,	\
			   key_inventory_cycle_right,	\
			   key_pickup,					\
			   key_dropdown,				\
			   key_escape,					\
			   key_prisoners_cycle_left,	\
			   key_prisoners_cycle_right,	\
			   key_select_british,			\
			   key_select_french,			\
			   key_select_american,			\
			   key_select_polish)


/////////////////////////////////////////////////////////////////////////////////
// Function prototypes
//
int utf8_to_u16_nz(const char* source, u16* target, size_t len);
int utf8_to_u16(const char* source, u16* target);
int readconf(char* filename);
void init_xml_config(); 

#ifdef	__cplusplus
}
#endif

