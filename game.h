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
 *  game.h: Game runtime definitions
 *  ---------------------------------------------------------------------------
 */

#pragma once

#ifdef	__cplusplus
extern "C" {
#endif

/*
 *	These game macro will come handy later on
 */

// Reads the tile index at (x,y) from compressed map and rotate
#define comp_readtile(x,y)			\
	((u32)(readlong((u8*)fbuffer[COMPRESSED_MAP], ((y)*room_x+(x))*4) & 0x1FF00) >> 8)
#define room_readtile(x,y)			\
	((u32)(readword((u8*)(fbuffer[ROOMS]+offset),((y)*room_x+(x))*2) & 0xFF80) >> 7)
#define readtile(x,y)				\
	(is_outside?comp_readtile(x,y):room_readtile(x,y))

// Reads the exit index, which we need to get to the target room index
#define comp_readexit(x,y)			\
	((u32)(readlong((u8*)fbuffer[COMPRESSED_MAP], ((y)*room_x+(x))*4) & 0x1F))
#define room_readexit(x,y)			\
	((u32)(readword((u8*)(fbuffer[ROOMS]+offset),((y)*room_x+(x))*2) & 0x1F))
#define readexit(x,y)				\
	(is_outside?comp_readexit(x,y):room_readexit(x,y))

// Returns the offset of the byte that describes the exit status (open/closed, key level...)
#define room_get_exit_offset(x,y)	\
	(offset + ((y)*room_x+(x))*2 + 1)
#define comp_get_exit_offset(x,y)	\
	(comp_readexit(x,y) << 3)
#define get_exit_offset(x,y)		\
	(is_outside?comp_get_exit_offset(x,y):room_get_exit_offset(x,y))

// Toggle the exit open flag
#define toggle_open_flag(x_flags)  x_flags ^= 0x10

// Checks that an overlay is visible onscreen (with generous margins)
#define is_offscreen_x(x) ((x < -64) || (x > (PSP_SCR_WIDTH+64)))
#define is_offscreen_y(y) ((y < -64) || (y > (PSP_SCR_HEIGHT+64)))
#define ignore_offscreen_x(ovl)	{if is_offscreen_x(overlay[ovl].x) continue;}
#define ignore_offscreen_y(ovl)	{if is_offscreen_y(overlay[ovl].y) continue;}

// Get the current animated SID
#define get_guybrush_sid(x)												\
	( (((guybrush[x].state & (STATE_MOTION|STATE_ANIMATED))				\
	&& (!(guybrush[x].state & STATE_BLOCKED))) && !paused)?				\
	get_animation_sid(x, true):get_stop_animation_sid(x, true))

// Safe increments for our stacks
#define safe_nb_animations_increment() {	\
	if (nb_animations <= (MAX_ANIMATIONS-1))\
		nb_animations++;					\
	else									\
		perr("Too many animations!\n");	}

#define safe_overlay_index_increment() {	\
	if (overlay_index <= (MAX_OVERLAYS-1))	\
		overlay_index++;					\
	else									\
		perr("Too many overlays!\n");		}


// A few definitions to make prop handling and status messages more readable
extern u64  t_status_message_timeout;
static __inline void set_status_message(void* msg, int priority, u64 timeout_duration)
{
	if (priority >= status_message_priority)
	{
		t_status_message_timeout = game_time + timeout_duration;
		status_message = (char*)(msg);
		status_message_priority = priority;
	}
}

static __inline void consume_prop()
{	// we can use an __inline here because we deal with globals
	if (!opt_keymaster)
	{	// consume the prop
		props[current_nation][selected_prop[current_nation]]--;
		if (props[current_nation][selected_prop[current_nation]] == 0)
		// display the empty box if last prop
			selected_prop[current_nation] = 0;
	}
}

#define update_props_message(prop_id)												\
	nb_props_message[1] = (props[current_nation][prop_id] / 10) + 0x30;				\
	nb_props_message[2] = (props[current_nation][prop_id] % 10) + 0x30;				\
	strcpy(nb_props_message+6, (char*) fbuffer[LOADER] + readlong(fbuffer[LOADER],	\
		PROPS_MESSAGE_BASE + 4*(prop_id-1)) + 1);

#define show_prop_count()															\
	update_props_message(selected_prop[current_nation]);							\
	set_status_message(nb_props_message, 1, PROPS_MESSAGE_TIMEOUT)


/*
 *	Global variables
 */
extern u8			nb_animations;
extern s_animation	animations[MAX_ANIMATIONS];
extern s_guybrush	guybrush[NB_GUYBRUSHES];


/*
 *	Public prototypes
 */
void free_data();
void load_all_files();
void reload_files();
void newgame_init();
bool save_game(char* save_name);
bool load_game(char* load_name);
void depack_loadtune();
void set_room_props();
void set_sfxs();
int  move_guards();
void toggle_exit(u32 exit_nr);
s16  check_footprint(s16 dx, s16 d2y);
s16  check_tunnel_io();
bool check_guard_footprint(u8 g, s16 dx, s16 d2y);
void switch_nation(u8 new_nation);
void switch_room(s16 exit, bool tunnel_io);
void fix_files(bool reload);
void timed_events(u16 hours, u16 minutes_high, u16 minutes_low);
void check_on_prisoners();
void play_sfx(int sfx_id);
void go_to_jail(u32 p);
void set_room_xy(u16 room);
void set_props_overlays();
void crm_set_overlays(s16 x, s16 y, u16 current_tile);
void cmp_set_overlays();
void removable_walls();
void add_guybrushes();
void sort_overlays(u8 a[], u8 n);
void play_cluck();
void thriller_toggle();

#ifdef	__cplusplus
}
#endif

