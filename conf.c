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
 *  conf.c: Iniparser configuration wrapper
 *  ---------------------------------------------------------------------------
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "low-level.h"
#include "conf.h"

dictionary* config = NULL;
#if !defined(PSP)
uint8_t key_default[KEY_MAX] = { 
    '5', 0x00, 0x1b, ' ',
    SPECIAL_KEY_F5, SPECIAL_KEY_F9, SPECIAL_KEY_F10,
    '4', '6', '8', '2',
    SPECIAL_KEY_LEFT, SPECIAL_KEY_RIGHT, SPECIAL_KEY_UP, SPECIAL_KEY_DOWN,
    0x00, 0x00,
    SPECIAL_KEY_F1, SPECIAL_KEY_F2, SPECIAL_KEY_F3, SPECIAL_KEY_F4 };
#else
uint8_t key_default[KEY_MAX] = {
    'x', 'o', 'a', 'o',
    's', 'd', 'q',
    0x00, 0x00, 0x00, 0x00,
    SPECIAL_KEY_LEFT, SPECIAL_KEY_RIGHT, SPECIAL_KEY_UP, SPECIAL_KEY_DOWN,
    SPECIAL_LEFT_MOUSE_BUTTON, SPECIAL_RIGHT_MOUSE_BUTTON,
    0x00, 0x00, 0x00, 0x00 };
#endif

void free_conf(void)
{
    iniparser_freedict(config);
    config = NULL;
}

bool read_conf(const char* filename)
{
    config = iniparser_load(filename);
    return (config != NULL);
}

bool write_conf(const char* filename)
{
    FILE* fd;

    if (config == NULL)
        return false;

    // NULL or empty string => use stdout
    if ((filename == NULL) || (strlen(filename) == 0))
        fd = stdout;
    else if ((fd = fopen(filename, "w")) == NULL)
    {
        perr("Could not save '%s'\n", filename);
        return false;
    }
    iniparser_dump_ini(config, fd);
    if (fd != stdout)
        fclose(fd);
    return true;
}

static __inline void set_key_default(const char* optname, enum keys key)
{
    char str[] = "'0'";
    // If possible, insert the character (between single quotes) instead of its hex value
    if ((key_default[key] >= 0x20) && (key_default[key] <= 0x7E))
    {
        str[1] = key_default[key];
        iniparser_set(config, optname, str);
    }
    else
        iniparser_set_char(config, optname, key_default[key]);
}

#define SET_KEY_DEFAULT(key) set_key_default("controls:" # key, key)

bool set_conf_defaults(void)
{
    config = dictionary_new(0);
    if (config == NULL)
        return false;

    // Options
    iniparser_set(config, "options", NULL);
    iniparser_set(config, "options:vsync", "1");
    iniparser_set(config, "options:fullscreen", "0");
    iniparser_set(config, "options:enhanced_guards", "1");
    iniparser_set(config, "options:enhanced_tunnels", "1");
    iniparser_set(config, "options:picture_corners", "1");
    iniparser_set(config, "options:joy_deadzone", "450");
    iniparser_set(config, "options:original_mode", "0");
#if defined(PSP) || defined(__APPLE__)
    iniparser_set(config, "options:gl_smoothing", "0");
#else
    iniparser_set(config, "options:gl_smoothing", "5");
#endif

    // Controls
    iniparser_set(config, "controls", NULL);
    SET_KEY_DEFAULT(key_action);
    SET_KEY_DEFAULT(key_cancel);
    SET_KEY_DEFAULT(key_escape);
    SET_KEY_DEFAULT(key_toggle_walk_run);
    SET_KEY_DEFAULT(key_pause);
    SET_KEY_DEFAULT(key_sleep);
    SET_KEY_DEFAULT(key_stooge);
    SET_KEY_DEFAULT(key_direction_left);
    SET_KEY_DEFAULT(key_direction_right);
    SET_KEY_DEFAULT(key_direction_up);
    SET_KEY_DEFAULT(key_direction_down);
    SET_KEY_DEFAULT(key_inventory_left);
    SET_KEY_DEFAULT(key_inventory_right);
    SET_KEY_DEFAULT(key_inventory_pickup);
    SET_KEY_DEFAULT(key_inventory_dropdown);
    SET_KEY_DEFAULT(key_prisoner_left);
    SET_KEY_DEFAULT(key_prisoner_right);
    SET_KEY_DEFAULT(key_prisoner_british);
    SET_KEY_DEFAULT(key_prisoner_french);
    SET_KEY_DEFAULT(key_prisoner_american);
    SET_KEY_DEFAULT(key_prisoner_polish);

    return true;
}

