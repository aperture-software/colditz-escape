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
 *  conf.h: Iniparser configuration wrapper. Also user input definitions
 *  ---------------------------------------------------------------------------
 */

#pragma once

#include <stdbool.h>
#include "iniparser.h"

#if defined(PSP)
#include <GL/glut.h>
#else
#include "GL/freeglut.h"
#endif

#ifdef	__cplusplus
extern "C" {
#endif

extern dictionary* config;

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
#define SPECIAL_KEY_OFFSET1			0x80
// Follows the order in which they are defined in the glut headers
#define SPECIAL_KEY_F1				(GLUT_KEY_F1 - GLUT_KEY_F1 + SPECIAL_KEY_OFFSET1)
#define SPECIAL_KEY_F2				(GLUT_KEY_F2 - GLUT_KEY_F1 + SPECIAL_KEY_OFFSET1)
#define SPECIAL_KEY_F3				(GLUT_KEY_F3 - GLUT_KEY_F1 + SPECIAL_KEY_OFFSET1)
#define SPECIAL_KEY_F4				(GLUT_KEY_F4 - GLUT_KEY_F1 + SPECIAL_KEY_OFFSET1)
#define SPECIAL_KEY_F5				(GLUT_KEY_F5 - GLUT_KEY_F1 + SPECIAL_KEY_OFFSET1)
#define SPECIAL_KEY_F6				(GLUT_KEY_F6 - GLUT_KEY_F1 + SPECIAL_KEY_OFFSET1)
#define SPECIAL_KEY_F7				(GLUT_KEY_F7 - GLUT_KEY_F1 + SPECIAL_KEY_OFFSET1)
#define SPECIAL_KEY_F8				(GLUT_KEY_F8 - GLUT_KEY_F1 + SPECIAL_KEY_OFFSET1)
#define SPECIAL_KEY_F9				(GLUT_KEY_F9 - GLUT_KEY_F1 + SPECIAL_KEY_OFFSET1)
#define SPECIAL_KEY_F10				(GLUT_KEY_F10 - GLUT_KEY_F1 + SPECIAL_KEY_OFFSET1)
#define SPECIAL_KEY_F11				(GLUT_KEY_F11 - GLUT_KEY_F1 + SPECIAL_KEY_OFFSET1)
#define SPECIAL_KEY_F12				(GLUT_KEY_F12 - GLUT_KEY_F1 + SPECIAL_KEY_OFFSET1)
#define SPECIAL_KEY_OFFSET2			(SPECIAL_KEY_F12 + 1)
#define SPECIAL_KEY_LEFT			(GLUT_KEY_LEFT - GLUT_KEY_LEFT + SPECIAL_KEY_OFFSET2)
#define SPECIAL_KEY_UP				(GLUT_KEY_UP - GLUT_KEY_LEFT + SPECIAL_KEY_OFFSET2)
#define SPECIAL_KEY_RIGHT			(GLUT_KEY_RIGHT - GLUT_KEY_LEFT + SPECIAL_KEY_OFFSET2)
#define SPECIAL_KEY_DOWN			(GLUT_KEY_DOWN - GLUT_KEY_LEFT + SPECIAL_KEY_OFFSET2)
#define SPECIAL_KEY_PAGE_UP			(GLUT_KEY_PAGE_UP - GLUT_KEY_LEFT + SPECIAL_KEY_OFFSET2)
#define SPECIAL_KEY_PAGE_DOWN		(GLUT_KEY_PAGE_DOWN - GLUT_KEY_LEFT + SPECIAL_KEY_OFFSET2)
#define SPECIAL_KEY_HOME			(GLUT_KEY_HOME - GLUT_KEY_LEFT + SPECIAL_KEY_OFFSET2)
#define SPECIAL_KEY_END				(GLUT_KEY_END - GLUT_KEY_LEFT + SPECIAL_KEY_OFFSET2)
#define SPECIAL_KEY_INSERT			(GLUT_KEY_INSERT - GLUT_KEY_LEFT + SPECIAL_KEY_OFFSET2)
#define SPECIAL_KEY_SHIFT			(SPECIAL_KEY_INSERT + 1)
#define SPECIAL_KEY_CTRL			(SPECIAL_KEY_INSERT + 2)
#define SPECIAL_KEY_ALT				(SPECIAL_KEY_INSERT + 3)
// Same for mouse buttons (PSP L&R triggers are mapped to mouse buttons in GLUT)
#define SPECIAL_MOUSE_BUTTON_BASE	(SPECIAL_KEY_ALT + 1)
#define SPECIAL_LEFT_MOUSE_BUTTON	(GLUT_LEFT_BUTTON - GLUT_LEFT_BUTTON + SPECIAL_MOUSE_BUTTON_BASE)
#define SPECIAL_MIDDLE_MOUSE_BUTTON	(GLUT_MIDDLE_BUTTON - GLUT_LEFT_BUTTON + SPECIAL_MOUSE_BUTTON_BASE)
#define SPECIAL_RIGHT_MOUSE_BUTTON	(GLUT_RIGHT_BUTTON - GLUT_LEFT_BUTTON + SPECIAL_MOUSE_BUTTON_BASE)

enum keys {
    key_action = 0,
    key_cancel,
    key_escape,
    key_toggle_walk_run,
    key_pause,
    key_sleep,
    key_stooge,
    key_direction_left,
    key_direction_right,
    key_direction_up,
    key_direction_down,
    key_inventory_left,
    key_inventory_right,
    key_inventory_pickup,
    key_inventory_dropdown,
    key_prisoner_left,
    key_prisoner_right,
    key_prisoner_british,
    key_prisoner_french,
    key_prisoner_american,
    key_prisoner_polish,
    KEY_MAX
};

extern uint8_t key_default[KEY_MAX];

static __inline uint8_t read_keyval(const char* keyname, int fallback)
{
    const char* str = iniparser_getstring(config, keyname, "0");
    // Straight characters are enclosed in single quotes
    if (str[0] == '\'')
        return (uint8_t)str[1];
    // Not a straight char => hex or integer value
    return (uint8_t)iniparser_getint(config, keyname, fallback);
}
#define KEYVAL(key) read_keyval("controls:" #key, 0xe0 + key)

// Short(?)cut defines for the main program
#define KEY_ACTION					KEYVAL(key_action)
#define KEY_CANCEL					KEYVAL(key_cancel)
#define KEY_TOGGLE_WALK_RUN			KEYVAL(key_toggle_walk_run)
#define KEY_PAUSE					KEYVAL(key_pause)
#define KEY_INVENTORY_LEFT			KEYVAL(key_inventory_left)
#define KEY_INVENTORY_RIGHT			KEYVAL(key_inventory_right)
#define KEY_INVENTORY_PICKUP		KEYVAL(key_inventory_pickup)
#define KEY_INVENTORY_DROP			KEYVAL(key_inventory_dropdown)
#define KEY_SLEEP					KEYVAL(key_sleep)
#define KEY_STOOGE					KEYVAL(key_stooge)
#define KEY_ESCAPE					KEYVAL(key_escape)
#define KEY_PRISONER_LEFT			KEYVAL(key_prisoner_left)
#define KEY_PRISONER_RIGHT			KEYVAL(key_prisoner_right)
#define KEY_PRISONER_BRITISH		KEYVAL(key_prisoner_british)
#define KEY_PRISONER_FRENCH			KEYVAL(key_prisoner_french)
#define KEY_PRISONER_AMERICAN		KEYVAL(key_prisoner_american)
#define KEY_PRISONER_POLISH			KEYVAL(key_prisoner_polish)
#define KEY_DIRECTION_LEFT			KEYVAL(key_direction_left)
#define KEY_DIRECTION_RIGHT			KEYVAL(key_direction_right)
#define KEY_DIRECTION_UP			KEYVAL(key_direction_up)
#define KEY_DIRECTION_DOWN			KEYVAL(key_direction_down)

// XBox 360 Controller definitions
#if !defined(PSP)
#define XBOX360_CONTROLLER_SUPPORT
#endif

#define XBOX360_CONTROLLER_BUTTON_A		0x00000001
#define XBOX360_CONTROLLER_BUTTON_B		0x00000002
#define XBOX360_CONTROLLER_BUTTON_X		0x00000004
#define XBOX360_CONTROLLER_BUTTON_Y		0x00000008
#define XBOX360_CONTROLLER_BUTTON_LB	0x00000010
#define XBOX360_CONTROLLER_BUTTON_RB	0x00000020
#define XBOX360_CONTROLLER_BUTTON_BACK	0x00000040
#define XBOX360_CONTROLLER_BUTTON_START	0x00000080
#define XBOX360_CONTROLLER_BUTTON_LJOY	0x00000100
#define XBOX360_CONTROLLER_BUTTON_RJOY	0x00000200
#define XBOX360_CONTROLLER_BUTTON_LT	0x00010000
#define XBOX360_CONTROLLER_BUTTON_RT	0x00020000
#define XBOX360_CONTROLLER_DPAD_UP		0x80000000
#define XBOX360_CONTROLLER_DPAD_DOWN	0x40000000
#define XBOX360_CONTROLLER_DPAD_RIGHT	0x20000000
#define XBOX360_CONTROLLER_DPAD_LEFT	0x10000000

#define GET_CONFIG_BOOLEAN(section, option) iniparser_getboolean(config, #section ":" #option, false)
#define GET_CONFIG_INTEGER(section, option) iniparser_getint(config, #section ":" #option, 0)
#define SET_CONFIG_BOOLEAN(section, option, value) iniparser_set(config, #section ":" #option, value?"1":"0");
#define SET_CONFIG_INTEGER_ZERO(section, option, value) iniparser_set(config, #section ":" #option, "0");

#define opt_picture_corners			GET_CONFIG_BOOLEAN(options, picture_corners)
#define opt_enhanced_guards			GET_CONFIG_BOOLEAN(options, enhanced_guards)
#define opt_enhanced_tunnels		GET_CONFIG_BOOLEAN(options, enhanced_tunnels)
#define opt_vsync					GET_CONFIG_BOOLEAN(options, vsync)
#define opt_gl_smoothing			GET_CONFIG_INTEGER(options, gl_smoothing)
#define opt_fullscreen				GET_CONFIG_BOOLEAN(options, fullscreen)
#define JOY_DEADZONE				GET_CONFIG_INTEGER(options, joy_deadzone)
#define opt_original_mode			GET_CONFIG_BOOLEAN(options, original_mode)

/*
 * Set the internal config to default
 */
bool set_conf_defaults(void);

/*
 * Open and parse the ini file "filename"
 * Returns true on success, false otherwise
 */
bool read_conf(const char* filename);

/*
 * Write the ini file "filename" using the current dictionary
 * Returns true on success, false otherwise
 */
bool write_conf(const char* filename);

/*
 * Free dynamically allocated data that needs to be released
 */
void free_conf(void);

#ifdef	__cplusplus
}
#endif
