/**
 **  Escape from Colditz
 **
 **  Configuration helper functions 
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
/*
	This constant *MUST* be defined before including eschew.h,
	*IN* the source where you will do the runtime initialization
*/
#define INIT_XML_ACTUAL_INIT
#include "eschew.h"
#include "conf.h"

#if defined(PSP)
#define SET_XML_NODE_DEFAULT2(table, node_name, val1, val2) 			\
	SET_XML_NODE_DEFAULT(table, node_name, val1)
#else
#define SET_XML_NODE_DEFAULT2(table, node_name, val1, val2) 			\
	SET_XML_NODE_DEFAULT(table, node_name, val2)
#endif


void init_xml_config()
{
	// The second part of a node table init MUST occur at runtime
	INIT_XML_TABLE(config);
	INIT_XML_TABLE(options);
	INIT_XML_TABLE(controls);
	INIT_XML_TABLE(controls_target_psp);

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