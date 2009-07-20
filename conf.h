/*
	configuration file handling
*/

#pragma once

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

// Non official debug keys
#define KEY_DEBUG_PRINT_POS		'p'
#define KEY_DEBUG_BONANZA		'#'
#define KEY_DEBUG_CATCH_HIM		'c'

//#define opt_skip_intro			options.node[skip_intro].value
//#define opt_enhanced_guards		options.node[enhanced_guards].value
//#define opt_picture_corners		options.node[picture_corners].value

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
CREATE_XML_TABLE(options, options_nodes, xml_int)

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
CREATE_XML_TABLE(controls, controls_nodes, xml_unsigned_char)
CREATE_XML_TABLE(controls_target_psp, controls_nodes, xml_unsigned_char)


void init_xml_config(); 

#ifdef	__cplusplus
}
#endif

