// Read and populate the XML config file
// 
/////////////////////////////////////////////////////////////////////////////////
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

// Short(?)cut defines for the main program
#define KEY_FIRE				controls[key_fire].value
#define KEY_TOGGLE_WALK_RUN		controls[key_toggle_walk_run].value
#define KEY_PAUSE				controls[key_pause].value
#define KEY_INVENTORY_LEFT		controls[key_inventory_cycle_left].value
#define KEY_INVENTORY_RIGHT		controls[key_inventory_cycle_right].value
#define KEY_INVENTORY_PICKUP	controls[key_pickup].value
#define KEY_INVENTORY_DROP		controls[key_dropdown].value
#define KEY_SLEEP				controls[key_sleep].value
#define KEY_STOOGE				controls[key_stooge].value
#define KEY_ESCAPE				controls[key_escape].value
#define KEY_PRISONERS_LEFT		controls[key_prisoners_cycle_left].value
#define KEY_PRISONERS_RIGHT		controls[key_prisoners_cycle_right].value
#define KEY_BRITISH				controls[key_select_british].value
#define KEY_FRENCH				controls[key_select_french].value
#define KEY_AMERICAN			controls[key_select_american].value
#define KEY_POLISH				controls[key_select_polish].value
#define KEY_DIRECTION_LEFT		controls[key_direction_left].value
#define KEY_DIRECTION_RIGHT		controls[key_direction_right].value
#define KEY_DIRECTION_UP		controls[key_direction_up].value
#define KEY_DIRECTION_DOWN		controls[key_direction_down].value

// Non official debug keys
#define KEY_DEBUG_PRINT_POS		'p'
#define KEY_DEBUG_BONANZA		'#'
#define KEY_DEBUG_CATCH_HIM		'c'



/////////////////////////////////////////////////////////////////////////////////
// XML Node tables
//
// An example is worth a thousand words:
// INIT_XML_NODES(table, unsigned char, node1, node2, node3) ==>
//   enum { node1, node2, node3, _table_end };
//	  typedef struct { char* name; unsigned char value; } s_table_element
//   static char* _table_names = "node1, node2, node3";
//   s_table_element table[_node_end];
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

#if defined(INIT_XML_ACTUAL_INIT)
#define INIT_XML_NODES(table, type, ...)									\
	enum { __VA_ARGS__, _ ## table ## _end };								\
	typedef struct {char* name; type value;} s_ ## table ## _node_element ; \
	static char* _ ## table ## _names = #__VA_ARGS__;						\
	s_ ## table ## _node_element table[_ ## table ## _end];
#define INIT_XML_NODE_NAMES(table) {										\
	table[i=0].name = strtok (_ ## table ## _names, " ,\t");				\
	while (table[i++].name != NULL)											\
		table[i].name = strtok (NULL, " ,\t"); }		
#else
#define INIT_XML_NODES(table, type, ...)									\
	enum { __VA_ARGS__, _ ## table ## _end };								\
	typedef struct {char* name; type value;} s_ ## table ## _node_element ; \
	extern s_ ## table ## _node_element table[_ ## table ## _end];
#endif

// Debug printout of the node tree
#define PRINT_XML_NODES(table)												\
	for (i=0; i<_ ## table ## _end; i++) {									\
		printf("node[%d].name = %s\n", i, table[i].name);					\
		printf("node[%d].value = %X\n", i, table[i].value); }


/////////////////////////////////////////////////////////////////////////////////
// XML tables defintions
//

// General program options
INIT_XML_NODES(options, bool, skip_intro, enhanced_guards, picture_corners)
//#define opt_skip_intro			options[skip_intro].value
//#define opt_enhanced_guards		options[enhanced_guards].value
//#define opt_picture_corners		options[picture_corners].value

// User input mappings
INIT_XML_NODES(controls, u8, key_fire,		\
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
void init_controls(); 

#ifdef	__cplusplus
}
#endif

/* NO IMPL */

/*
#define INIT_XML_NODE_VALUES(table, type, ...)								\
	type table ## _value[_ ## table ## _end] = { __VA_ARGS__ };			
*/
//#define NB_OPTIONS 2

//#define VA_DIVIDER (NB_OPTIONS+1)
/*
#define INIT_XML_NODES(table, ...)									\
	enum { __VA_ARGS__, _ ## table ## _end };								
//	typedef struct { char* name; type value; } s_ ## table ## _node ;		
//	static char* _ ## table ## _tokens = #__VA_ARGS__;						
//	s_ ## table ## _node table [_ ## table ## _end ];
*/


/*
// The following allows to specifie multiple values for windows (first value)
// and PSP (second value)
#if defined(PSP)
#define SKIP_BEFORE strtok(NULL, WS)
#define SKIP_AFTER
#else
#define SKIP_BEFORE
#define SKIP_AFTER strtok(NULL, WS)
#endif

enum {NODE_CHAR, NODE_BOOL, NODE_INTEGER, NODE_FLOAT, NODE_ASCII};

#define set_xml_nodes(table, node_type) {									\
	table[i=0].name = strtok (_ ## table ## _tokens, _WS); SKIP_BEFORE;		\
	while (table[i].name != NULL) {											\
		switch (node_type) {												\
		case NODE_CHAR:														\
			table[i].value = *((u8*)strtok(NULL, _WS)); break;				\
		case NODE_INTEGER: break;											\
		case NODE_BOOL: break;												\
		default:															\
			table[i].value = strtok(NULL, _WS)); break; }; SKIP_AFTER;		\
		table ## _name[i++] = strtok (NULL, _WS); }		
*/

/*
#define INIT_XML_NODE_VALUES(table, type, ...)								\
	extern type table ## _value[_ ## table ## _end];


*/
