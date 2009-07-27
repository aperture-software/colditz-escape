/*
 *  Colditz Escape! - Rewritten Engine for "Escape From Colditz"
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
 *  conf.c: XML/Eschew configuration wrapper
 *  ---------------------------------------------------------------------------
 */

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


void init_xml()
{
	// The second part of a node table init MUST occur at runtime
	INIT_XML_TABLE(config);
	INIT_XML_TABLE(options);
#if defined(PSP)
	INIT_XML_TABLE(controls_target_psp);
#else
	INIT_XML_TABLE(controls_target_windows);
#endif

	// Set defaults values. Can be skipped if relying on the external XML
	SET_XML_NODE_DEFAULT(options, skip_intro, false);
	SET_XML_NODE_DEFAULT(options, enhanced_guard_handling, true);
	SET_XML_NODE_DEFAULT(options, picture_corners, true);

#if defined(PSP)
#define SET_CONTROLS_DEFAULT(key, val1, val2) 							\
	SET_XML_NODE_DEFAULT(controls_target_psp, key, val1)
#else
#define SET_CONTROLS_DEFAULT(key, val1, val2) 							\
	SET_XML_NODE_DEFAULT(controls_target_windows, key, val2)
#endif

	// The order is PSP, WIN
	SET_CONTROLS_DEFAULT(key_fire, 'x', '5');
    SET_CONTROLS_DEFAULT(key_toggle_walk_run, 'o', ' ');
    SET_CONTROLS_DEFAULT(key_pause, 's', SPECIAL_KEY_F5);
    SET_CONTROLS_DEFAULT(key_sleep, 'q', SPECIAL_KEY_F9);
    SET_CONTROLS_DEFAULT(key_stooge, 'd', SPECIAL_KEY_F10);
    SET_CONTROLS_DEFAULT(key_direction_left, 0, '4');
    SET_CONTROLS_DEFAULT(key_direction_right, 0, '6');
    SET_CONTROLS_DEFAULT(key_direction_up, 0, '8');
    SET_CONTROLS_DEFAULT(key_direction_down, 0, '2');
    SET_CONTROLS_DEFAULT(key_inventory_cycle_left, SPECIAL_KEY_LEFT, SPECIAL_KEY_LEFT);
    SET_CONTROLS_DEFAULT(key_inventory_cycle_right, SPECIAL_KEY_RIGHT, SPECIAL_KEY_RIGHT);
    SET_CONTROLS_DEFAULT(key_pickup, SPECIAL_KEY_UP, SPECIAL_KEY_UP);
    SET_CONTROLS_DEFAULT(key_dropdown, SPECIAL_KEY_DOWN, SPECIAL_KEY_DOWN);
    SET_CONTROLS_DEFAULT(key_escape, 'a', 0x1b);
    SET_CONTROLS_DEFAULT(key_prisoners_cycle_left, SPECIAL_LEFT_MOUSE_BUTTON, 0);
    SET_CONTROLS_DEFAULT(key_prisoners_cycle_right, SPECIAL_RIGHT_MOUSE_BUTTON, 0);
    SET_CONTROLS_DEFAULT(key_select_british, 0, SPECIAL_KEY_F1);
    SET_CONTROLS_DEFAULT(key_select_french, 0, SPECIAL_KEY_F2);
    SET_CONTROLS_DEFAULT(key_select_american, 0, SPECIAL_KEY_F3);
    SET_CONTROLS_DEFAULT(key_select_polish, 0, SPECIAL_KEY_F4);

	// Debug
//	PRINT_XML_TABLE(controls); 
}