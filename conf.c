/*
 *  Colditz Escape - Rewritten Engine for "Escape From Colditz"
 *  copyright (C) 2008-2017 Aperture Software
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
#include <stdbool.h>
#if defined(WIN32)
#include <windows.h>
#elif defined(PSP)
#include <pspkernel.h>
#endif
/*
 * This constant *MUST* be defined before including eschew.h,
 * *IN* the source where you do the runtime initialization
 */
#define INIT_XML_ACTUAL_INIT
#include "eschew/eschew.h"
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
}

void set_xml_defaults()
{
    SET_XML_NODE_COMMENT(config, controls, " About our key mappings:\n"
"       Standard key = regular (lowercase) ASCII code\n"
"       0x7F         = Del\n"
"       [0x80-0x8B]  = [F1-F12]\n"
"       [0x8C-0x8F]  = [Left, Up, Right, Down]\n"
"       [0x90-0x94]  = [PgUp, PgDn, Home, End, Insert]\n"
"       [0x95-0x97]  = [Shift, Ctrl, Alt] (*)\n"
"       [0x98-0x9A]  = [Mouse Left, Mouse Middle, Mouse Right]\n"
"       [0xE0-0xFF]  = [Reserved - DO NOT USE]\n"
"       (*) Because of GLUT's limitations, these key events are ONLY detected\n"
"       in conjuction with other keypresses, and cannot be used as standalone\n"
"       see https://www.opengl.org/resources/libraries/glut/spec3/node73.html\n\n"
"       Now with regards to the PSP GLUT key mappings conversions\n"
"       [X] = 'x'\n"
"       [O] = 'o'\n"
"       [Square] = 'q'\n"
"       [Triangle] = 'd'\n"
"       [Select] = 's'\n"
"       [Start] = 'a'\n"
"       [Left, Up, Right, Down] = same as above\n"
"       [Left Trigger] = Mouse Left (see above for the actual code)\n"
"       [Right Trigger] = Mouse Right (see above for the actual code) ")

    // Set defaults values. Can be skipped if relying on the external XML
    SET_XML_NODE_DEFAULT(options, vsync, true);
    SET_XML_NODE_COMMENT(options, vsync,
        " enable VSYNC ");
    SET_XML_NODE_DEFAULT(options, fullscreen, false);
    SET_XML_NODE_COMMENT(options, fullscreen,
        " resize to fullscreen ");
    SET_XML_NODE_DEFAULT(options, enhanced_guards, true);
    SET_XML_NODE_COMMENT(options, enhanced_guards,
        " have guards remember when they've seen a pass ");
    SET_XML_NODE_DEFAULT(options, enhanced_tunnels, true);
    SET_XML_NODE_COMMENT(options, enhanced_tunnels,
        " limited field of vision in tunnels ");
    SET_XML_NODE_DEFAULT(options, picture_corners, true);
    SET_XML_NODE_COMMENT(options, picture_corners,
        " display texturized picture corners, rather than black triangles ");
#if defined(PSP)
    SET_XML_NODE_DEFAULT(options, gl_smoothing, 0);
#else
    SET_XML_NODE_DEFAULT(options, gl_smoothing, 5);
#endif
    SET_XML_NODE_COMMENT(options, gl_smoothing,
        " type of graphic smoothing: none, linear, hq2x, hq4x, 5xbr, sabr ");
    SET_XML_NODE_DEFAULT(options, joy_deadzone, 450);
    SET_XML_NODE_COMMENT(options, joy_deadzone,
        " joystick deadzone");
    SET_XML_NODE_DEFAULT(options, original_mode, false);
    SET_XML_NODE_COMMENT(options, original_mode,
        " Bwak! Bwaaak! Chicken!!! ");


#if defined(PSP)
#define SET_CONTROLS_DEFAULT(key, val1, val2) 							\
    SET_XML_NODE_DEFAULT(controls_target_psp, key, val1)
#else
#define SET_CONTROLS_DEFAULT(key, val1, val2) 							\
    SET_XML_NODE_DEFAULT(controls_target_windows, key, val2)
#endif

    // The order is PSP, WIN
    SET_CONTROLS_DEFAULT(key_action, 'x', '5');
    SET_CONTROLS_DEFAULT(key_cancel, 0, 0);
    SET_CONTROLS_DEFAULT(key_toggle_walk_run, 'o', ' ');
    SET_CONTROLS_DEFAULT(key_pause, 's', SPECIAL_KEY_F5);
    SET_CONTROLS_DEFAULT(key_sleep, 'q', SPECIAL_KEY_F9);
    SET_CONTROLS_DEFAULT(key_stooge, 'd', SPECIAL_KEY_F10);
    SET_CONTROLS_DEFAULT(key_direction_left, 0, '4');
    SET_CONTROLS_DEFAULT(key_direction_right, 0, '6');
    SET_CONTROLS_DEFAULT(key_direction_up, 0, '8');
    SET_CONTROLS_DEFAULT(key_direction_down, 0, '2');
    SET_CONTROLS_DEFAULT(key_inventory_cycle_left, SPECIAL_LEFT_MOUSE_BUTTON, SPECIAL_KEY_LEFT);
    SET_CONTROLS_DEFAULT(key_inventory_cycle_right, SPECIAL_RIGHT_MOUSE_BUTTON, SPECIAL_KEY_RIGHT);
    SET_CONTROLS_DEFAULT(key_pickup, SPECIAL_KEY_UP, SPECIAL_KEY_UP);
    SET_CONTROLS_DEFAULT(key_dropdown, SPECIAL_KEY_DOWN, SPECIAL_KEY_DOWN);
    SET_CONTROLS_DEFAULT(key_escape, 'a', 0x1b);
    SET_CONTROLS_DEFAULT(key_prisoners_cycle_left, SPECIAL_KEY_LEFT, 0);
    SET_CONTROLS_DEFAULT(key_prisoners_cycle_right, SPECIAL_KEY_RIGHT, 0);
    SET_CONTROLS_DEFAULT(key_select_british, 0, SPECIAL_KEY_F1);
    SET_CONTROLS_DEFAULT(key_select_french, 0, SPECIAL_KEY_F2);
    SET_CONTROLS_DEFAULT(key_select_american, 0, SPECIAL_KEY_F3);
    SET_CONTROLS_DEFAULT(key_select_polish, 0, SPECIAL_KEY_F4);

    // Debug
//  PRINT_XML_TABLE(controls_target_windows);
}
