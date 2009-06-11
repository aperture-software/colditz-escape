/**
 **  Colditz Maps Explorer
 **
 **  
 **
 **/

/** For glut, you need the dll in the exec location or system32
 ** and to compile, you need:
 ** - glut32.lib in C:\Program Files\Microsoft SDKs\Windows\v6.0A\Lib
 ** - glut.h in C:\Program Files\Microsoft SDKs\Windows\v6.0A\Include\gl 
 **
 ** The libs and header can be had from http://www.opengl.org/resources/libraries/glut/glutdlls37beta.zip
 **/
#include <stdio.h>
#include <stdlib.h>	
#include <string.h>
#include <math.h>

#if defined(WIN32)
#include <windows.h>
#include <gl/gl.h>
#include <gl/glu.h>
#include <gl/glut.h>
// Tell VC++ to include the GL libs
#pragma comment(lib, "opengl32.lib")
#pragma comment(lib, "glu32.lib")
#pragma comment(lib, "glut32.lib")
#elif defined(PSP)
#include <pspkernel.h>
#include <pspdebug.h>
#include <pspdisplay.h>
//#include <pspsdk.h>
#include <pspctrl.h>
#include <pspsyscon.h>
#include <pspgu.h>
#include <psprtc.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glut.h>
#include <pspaudiolib.h>
#include "psp/psp-setup.h"
#include "psp/psp-printf.h"
#endif

#include "getopt.h"	
#include "data-types.h"
#include "low-level.h"
#include "utilities.h"
#include "soundplayer.h"
#include "colditz.h"


// Global variables

// Flags
int debug_flag				= 0;
int	opt_verbose				= 0;
int	opt_debug				= 0;
int opt_ghost				= 0;
int opt_play_as_the_safe	= 0;
// Who needs keys?
int opt_keymaster			= 0;
// "'coz this is triller..."
int opt_thrillerdance		= 0;
// Force a specific sprite ID for our guy
// NB: must be init to -1
int opt_sid					= -1;
// Kill the guards
int	opt_no_guards			= 0;
// Is the castle haunted by the ghost of shot prisoners
bool opt_haunted_castle		= true;
// Number of escaped prisoners
int nb_escaped				= 0;
// Current static picture to show
u8  current_picture			= INTRO_SCREEN_START;
// Why is this global again?
bool new_game				= false;
//
bool reload_needed			= false;
// Our second local pause variable, to help with the transition
bool display_paused			= false;

#if defined(PSP)
int psp_printf_mode			= 0;
#endif




// File stuff
FILE* fd					= NULL;
char* fname[NB_FILES]		= FNAMES;			// file name(s)
u32   fsize[NB_FILES]		= FSIZES;
u8*   fbuffer[NB_FILES];
u8*   mbuffer				= NULL;
u8*	  rgbCells				= NULL;
u8*   static_image_buffer   = NULL;
char* iff_name[NB_IFFS]		= IFF_NAMES;
char* mod_name[NB_MODS]		= MOD_NAMES;
u16   iff_payload_w[NB_IFFS] = IFF_PAYLOAD_W;
u16   iff_payload_h[NB_IFFS] = IFF_PAYLOAD_H;
char* raw_name[NB_RAWS]		= RAW_NAMES;
s_sfx sfx[NB_SFXS];

// Used for fade in/fade out of static images
float fade_value = 1.0f;
// 1 for fade in, -1 for fade out
int fade_direction = 1;	// make sure it is initialized to 1

// OpenGL window size
int	gl_width, gl_height;
//u8	prisoner_h = 0x23, prisoner_w = 0x10;
//int prisoner_x = 20, prisoner_2y = 20;
//int prisoner_x = 900, prisoner_2y = 600;
//int prisoner_x = 1339, prisoner_2y = 895;
//int prisoner_x = 0, prisoner_2y = 0;
s16 last_p_x = 0, last_p_y = 0;
s16 dx = 0, d2y = 0;
s16 jdx, jd2y;
//u8	p_sid_base	 = 0x00;
// prisoner_run = 0x1F
// german_walk  = 0x37 (with rifle)
// german_run   = 0x57 (with rifle)
//u8  prisoner_sid = 0x07; // 0x07;

// Could use a set of flags, but more explicit this way
bool key_down[256], key_readonce[256];

#if defined (CHEATMODE_ENABLED)
// We don't want to pollute the key_readonce table for cheat keys, yet
// we need to check if the cheat keys have been pressed once
bool key_cheat_readonce[256];
u8 last_key_used = 0;
static __inline bool read_cheat_key_once(u8 k)
{	
	if (key_down[k])
	{
		if (key_cheat_readonce[k])
			return false;
		key_cheat_readonce[k] = true;
		return true;
	}
	return false;
}
typedef struct
{
	u8  nb_keys;
	u8  cur_pos;
	u8* keys;
} s_cheat_sequence;
#define NB_CHEAT_SEQUENCES 3
#define CHEAT_PROP_BONANZA	0
#define CHEAT_KEYMASTER		1
#define CHEAT_NOGUARDS		2
static u8 sequence0[4]  = {KEY_INVENTORY_LEFT, KEY_INVENTORY_LEFT, KEY_INVENTORY_RIGHT, KEY_INVENTORY_RIGHT};
static u8 sequence1[9]  = {'k', 'e', 'y', 'm', 'a', 's', 't', 'e', 'r'};
static u8 sequence2[11] = {'d', 'i', 'e', ' ', 'd', 'i', 'e', ' ', 'd', 'i', 'e'};
s_cheat_sequence cheat_sequence[NB_CHEAT_SEQUENCES] = {
	{4, 0, sequence0}, {9, 0, sequence1}, {11, 0, sequence2}
};
#endif

bool init_animations = true;
bool is_fire_pressed = false;
s_animation	animations[MAX_ANIMATIONS];
u8	nb_animations = 0;
// last time for Animations and rePosition of the guards
// NB: atime = animation time, ptime = position time, ctime = ???
u64 game_time, program_time, last_atime, last_ptime, last_ctime;
u64 t_last, t_status_message_timeout;
int	 status_message_priority;
s_guybrush guybrush[NB_GUYBRUSHES];
s_event	events[NB_EVENTS];
u8	props[NB_NATIONS][NB_PROPS];
u8	selected_prop[NB_NATIONS];
s_prisoner_event p_event[NB_NATIONS];
u8	nb_room_props = 0;
u16	room_props[NB_OBSBIN];
u8	over_prop = 0, over_prop_id = 0;
u8  current_nation = 0;
u8	panel_chars[NB_PANEL_CHARS][8*8*2];
char* status_message;
u16 game_state;
//u64 paused_time, suspended_time;
u8  hours_digit_h, hours_digit_l, minutes_digit_h, minutes_digit_l;
u8  tunexit_nr, tunexit_flags, tunnel_tool;
u8*	iff_image;
bool found;
void (*static_screen_func)(u32) = NULL;
u32  static_screen_param;

u16  nb_rooms, nb_cells, nb_objects;
u8	 palette_index = INITIAL_PALETTE_INDEX;
s_sprite*	sprite;
s_overlay*	overlay;
u8   overlay_index;
u8   bPalette[3][16];
// remapped Amiga Palette
u16  aPalette[32];	
char nb_props_message[32] = "\499 * ";

// static picture
u64 picture_t;
u8 picture_state;
float fade_step[NB_PICTURE_STATES] = { -FADE_INCREMENT, FADE_INCREMENT, 0.0f,
									   -FADE_INCREMENT, FADE_INCREMENT, 0.0f };


// offsets to sprites according to joystick direction (dx,dy)
s16 directions[3][3] = { {3,2,4}, {0,DIRECTION_STOPPED,1}, {6,5,7} };
// reverse table for dx and dy
s16 dir_to_dx[8] = {-1, 1, 0, -1, 1, 0, -1, 1};
s16 dir_to_d2y[8] = {0, 0, -1, -1, -1, 1, 1, 1};
// The direct nation keys might not be sequencial on custom key mapping
u8 key_nation[NB_NATIONS+2] = {KEY_BRITISH, KEY_FRENCH, KEY_AMERICAN, KEY_POLISH, 
							 KEY_PRISONERS_LEFT, KEY_PRISONERS_RIGHT};

u8* aper_blurb = NULL;
GLuint aper_blurb_texid;


/**
 ** GLUT event handlers
 **/
static void glut_init()
{
	// Use Glut to create a window
	glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_ALPHA);
	glutInitWindowSize(gl_width, gl_height);
	glutInitWindowPosition(0, 0); 
	glutCreateWindow( __FILE__ );

	GLCHK(glShadeModel(GL_SMOOTH));		// set by default

	GLCHK(glMatrixMode(GL_PROJECTION));
	GLCHK(glLoadIdentity());
	// We'll set top left corner to be (0,0) for 2D
    GLCHK(glOrtho(0, PSP_SCR_WIDTH, PSP_SCR_HEIGHT, 0, -1, 1));
	GLCHK(glMatrixMode(GL_MODELVIEW));

	// Set our default viewport
	GLCHK(glViewport(0, 0, PSP_SCR_WIDTH, PSP_SCR_HEIGHT));

	// Setup transparency
	GLCHK(glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA));
	GLCHK(glEnable(GL_BLEND));

	// Disable depth
	GLCHK(glDisable(GL_DEPTH_TEST));
	GLCHK(glEnable(GL_TEXTURE_2D));

	// Clear both buffers (this is needed on PSP)
	GLCHK(glClear(GL_COLOR_BUFFER_BIT));
	glutSwapBuffers();
	GLCHK(glClear(GL_COLOR_BUFFER_BIT));

	// Define the scissor area, outside of which we don't want to draw
//	GLCHK(glScissor(0,32,PSP_SCR_WIDTH,PSP_SCR_HEIGHT-32));
//	GLCHK(glEnable(GL_SCISSOR_TEST));

//	eglCreatePbufferSurface( null, null, null );

}

u16 ani_w = 1024;
u16 ani_x, ani_y;



// Our display routine. 
static void glut_display(void)
{
#if defined(PSP_ONSCREEN_STDOUT)
	// If we printf a message on the PSP we don't allow glut to take over
	if (psp_printf_mode == 0) 
	{
#endif
//	puts("psp_printf_mode = 0");
		// Always start with a clear to black
		glClear(GL_COLOR_BUFFER_BIT);

		// Display either the current game frame or a static picture
		if (game_state & GAME_STATE_STATIC_PIC)
		{
			display_picture();
			if (ani_w > 256)
			{
				ani_w-=4;
				ani_x = 209-(ani_w-256);
				ani_y = 181-(ani_w-256)/4;
			}
			display_sprite_linear(ani_x, ani_y, ani_w, ani_w/4, aper_blurb_texid);
		}
		else
		{	// In game => update room content and panel
			display_room();
			display_panel();
		} 
#if !defined (PSP)
		// Rescale the screen on Windows
		rescale_buffer();
#endif
		glutSwapBuffers();
#if defined (PSP)
	} 
#endif
}

// 
static void glut_reshape (int w, int h)
{
//	u32 gl_crop_width, gl_crop_height;

	gl_width=w;
	gl_height=h;
	
//	gl_crop_width = (32.0f * gl_width/PSP_SCR_WIDTH);
//	gl_crop_height = (16.0f * gl_height/PSP_SCR_HEIGHT);

	// Don't ask me why you need to use TWO scales of coordinates 
	// in the same formula to get the right amount of cropping
//	glScissor(32,32, (gl_width-32)-gl_crop_width,(gl_height-32));
//	glScissor(0,32, gl_width,(gl_height-32));

	glut_display();
}


// Update the game and program timers
void update_timers()
{
	u64 t, delta_t;
#if defined(PSP_ONSCREEN_STDOUT)
	SceCtrlData pad;
#endif	
	// Find out how much time elapsed since last call
	t = mtime();
	delta_t = t - t_last;
	t_last = t;

	if (delta_t > 1000)
		printf("NOTICE: more than one second elapsed since last time update\n");
	
#if defined(PSP_ONSCREEN_STDOUT)
	if (psp_printf_mode != 0) 
	{
		// Keep message on screen as long as X is not pressed
		sceCtrlReadBufferPositive(&pad, 1);
		if(pad.Buttons & PSP_CTRL_START)
		{
			// Wait for depress
			while (pad.Buttons & PSP_CTRL_START)
				sceCtrlReadBufferPositive(&pad, 1);
			psp_printf_mode = 0;
		}
	}
	else
	{	// Don't update the program timer while we're printing stuff
#endif
		program_time += delta_t;
		if (game_state & GAME_STATE_ACTION)
			game_time += delta_t;
#if defined(PSP_ONSCREEN_STDOUT)
	}
#endif
}


// As it names indicates
void process_motion(void)
{
	s16 new_direction;
	s16 exit;
	
	if (prisoner_state & MOTION_DISALLOWED)
	{	// Only a few states will allow motion 
		dx=0;
		d2y=0;
	}

	// Check if we're allowed to go where we want
	if ((dx != 0) || (d2y != 0))
	{
		exit = check_footprint(dx*prisoner_speed, d2y*prisoner_speed);
		if (exit != -1)
		{	// if -1, we move normally
			// in all other cases, we need to stop (even on sucessful exit)
			if (exit>0)
			{
//				printf("exit[%d], from room[%X]\n", exit-1, current_room_index);
				switch_room(exit-1, false);
				// keep_message_on = false;	// we could do without this
			}
			// Change the last direction so that we use the right sid for stopping
			prisoner_dir = directions[d2y+1][dx+1];

			// "Freeze!"
			dx = 0;
			d2y = 0;
		}
	}

	// Get direction (which is used as an offset to pick the proper animation
	new_direction = directions[d2y+1][dx+1];
	// NB: if d2y=0 & dx=0, new_dir = DIRECTION_STOPPED

	if (new_direction != DIRECTION_STOPPED)
	{	// We're moving => animate sprite

		if (!(prisoner_state & STATE_MOTION))
		// we were stopped => make sure we start with the proper ani frame
			prisoner_ani.framecount = 0;

		// Update our prisoner data
		// Update the fatigue
		if (prisoner_state & STATE_TUNNELING)
			prisoner_fatigue += 0x28;
		else
		{
			if (prisoner_fatigue >= MAX_FATIGUE)
			{
				prisoner_speed = 1;
				prisoner_ani.index = prisoner_as_guard?GUARD_WALK_ANI:WALK_ANI; 
			}
			prisoner_fatigue += (prisoner_speed==1)?1:4;
		}
		if (prisoner_fatigue > MAX_FATIGUE)
			prisoner_fatigue = MAX_FATIGUE;

		prisoner_x += prisoner_speed*dx;
		prisoner_2y += prisoner_speed*d2y;

		prisoner_state |= STATE_MOTION;
		// Update the animation direction
		prisoner_dir = new_direction;
	}
	else if (prisoner_state & STATE_MOTION)
	{	// We just stopped
			prisoner_state ^= STATE_MOTION;
	}
	else if (prisoner_state & STATE_SLEEPING)
	{	// Decrease fatigue
		if (prisoner_fatigue >= 2)
			prisoner_fatigue -= 2;
	}
}

// restore guybrush & animation parameters after a one shot ani
// this function expects the guybrush index as well as the previous ani_index
// to be concatenated in the lower 2 bytes of the parameter
void restore_params(u32 param)
{
	u8 brush, previous_index;
	// extract the guybrush index
	brush = param & 0xFF;
	// extract the previous animation index
	previous_index = (param >> 8) & 0xFF;

	guybrush[brush].animation.index = previous_index;
	guybrush[brush].animation.framecount = 0;
	guybrush[brush].animation.end_of_ani_function = NULL;
	// we always end up in stopped state after a one shot animation
	guybrush[brush].state &= ~(STATE_MOTION|STATE_KNEEL);
}


// This is the main game loop
static void glut_idle_game(void)
{	
	u16 prop_offset;
	u8	prop_id, direction, i, j;
	s16 exit_nr;

	// We'll need the current time value for a bunch of stuff
	update_timers();

	// Return to intro screen
	if (key_down[KEY_ESCAPE])
	{
		//game_state = GAME_STATE_INTRO | GAME_STATE_STATIC_PIC;
		exit(0);
	}

#if defined (CHEATMODE_ENABLED)
	// Check cheat sequences
	if (read_cheat_key_once(last_key_used))
	{
		for (i=0; i<NB_CHEAT_SEQUENCES; i++)
		{
			if (cheat_sequence[i].keys[cheat_sequence[i].cur_pos] == last_key_used)
			{	// The right key was read
				cheat_sequence[i].cur_pos++;
				if (cheat_sequence[i].cur_pos == cheat_sequence[i].nb_keys)
				// We completed cheat sequence #i
				{
					switch(i)
					{
					case CHEAT_PROP_BONANZA:
						for (j=1; j<NB_PROPS-1; j++)
							props[current_nation][j] += 10;
//						status_message = "1234567890123456789012345678";
						set_status_message("    ENJOY YOUR PROPS ;)     ", 3, CHEAT_MESSAGE_TIMEOUT);
						break;
					case CHEAT_KEYMASTER:
						opt_keymaster = ~opt_keymaster;
						break;
					default:
						set_status_message("        DIE DIE DIE         ", 3, CHEAT_MESSAGE_TIMEOUT);
						break;
					}
					printf("CHEAT[%d] activated!\n", i);
					cheat_sequence[i].cur_pos = 0;
				}
			}
			else
				// Reset the cheat sequence index
				cheat_sequence[i].cur_pos = 0;
		}
	}
#endif

	// Handle the pausing of the game 
	if (read_key_once(KEY_PAUSE))
		game_state ^= GAME_STATE_PAUSED;

	// No need to push it further if paused
	if (game_state & GAME_STATE_PAUSED)
	{
		// We should be able to sleep for a while
		msleep(PAUSE_DELAY);
		return;
	}

	// Handle timed events (including animations)
	if ((game_time - last_atime) > ANIMATION_INTERVAL)
	{
		last_atime += ANIMATION_INTERVAL;	// closer to ideal rate

		// but leads to catchup effect when moving window
		for (i = 0; i < nb_animations; i++)
			animations[i].framecount++;
		for (i = 0; i < NB_GUYBRUSHES; i++)
			guy(i).animation.framecount++;

		// Clock minute tick?
		if ((game_time - last_ctime) > TIME_MARKER)
		{
			last_ctime += TIME_MARKER;
			minutes_digit_l++;
			if (minutes_digit_l == 10)
			{
				minutes_digit_h++;
				minutes_digit_l = 0;
				if (minutes_digit_h == 6)
				{	// +1 hour
					hours_digit_l++;
					minutes_digit_h = 0;
					if (hours_digit_l == 10)
					{
						hours_digit_h++;
						hours_digit_l = 0;
					}
					if ((hours_digit_l == 4) && (hours_digit_h == 2))
					{
						hours_digit_l = 0;
						hours_digit_h = 0;
					}
					// As per the original game, we also add HOURLY_FATIGUE_INCREASE
					// for each hour spent awake
					for (i=0; i<NB_NATIONS; i++)
						if (!(guy(i).state & STATE_SLEEPING))
							p_event[i].fatigue += HOURLY_FATIGUE_INCREASE;
				}
			}

			// Check for timed events  
			timed_events(hours_digit_h*10+hours_digit_l, minutes_digit_h, minutes_digit_l);
			// If we got an event with a static pic, cancel the rest
			if (game_state & GAME_STATE_STATIC_PIC)
				return;
		}


		// Execute timed events, if any are in the queue
		for (i = 0; i<NB_EVENTS; i++)
		{
			if (events[i].function == NULL)
				continue;
			if (game_time > events[i].expiration_time)
			{	// Execute the timeout function
				events[i].function(events[i].parameter);
				// Make the event available again
				events[i].function = NULL;
			}
		}

		// Take care of message display
		if (game_time > t_status_message_timeout)
			status_message_priority = 0;
	}

	// Prisoner selection: direct keys or left/right cycle keys
	for (i=0; i<(NB_NATIONS+2); i++)
		if (read_key_once(key_nation[i]) && (current_nation != i))
		{
			// stop any motion or animation)
			// TO_DO: move this ion utilities
			// if there was any end of ani function, execute it
			if (prisoner_ani.end_of_ani_function != NULL)
			{	// execute the end of animation function (toggle exit)
				prisoner_ani.end_of_ani_function(prisoner_ani.end_of_ani_parameter);
				prisoner_ani.end_of_ani_function = NULL;
			}
			prisoner_state &= ~(STATE_MOTION|STATE_ANIMATED|STATE_KNEEL);

			if (i<NB_NATIONS)
				current_nation = i;
			else
				// we want to have +/-1, without going negative, and we
				// know that we are either at NB_NATIONS or NB_NATIONS+1
				// so the formula writes itself
				current_nation = (current_nation+(2*i)-1) % NB_NATIONS;
			prisoner_state &= ~(STATE_MOTION|STATE_ANIMATED|STATE_KNEEL);
			prisoner_reset_ani = true;
			t_status_message_timeout = 0;
			status_message_priority = 0;
			set_room_props();
			break;
		}

	// Reset the motion
	dx = 0; 
	d2y = 0;	

	// What follows only makes sense if the prisoner is still playable
	if (has_escaped || is_dead)
		// Tired of confusing brackets and indentations? So am I.
		goto update_motion;

#if !defined(PSP)
	// Hey, GLUT, where's my bleeping callback on Windows?
	// NB: The routine is not called if there's no joystick
	//     and the force func does not exist on PSP
	glutForceJoystickFunc();	
#endif

	// Alternate direction keys (non Joystick)
	if (key_down[KEY_DIRECTION_LEFT])
		dx = -1;
	else if (key_down[KEY_DIRECTION_RIGHT])
		dx = +1;
	if (key_down[KEY_DIRECTION_UP])
		d2y = -1;
	else if (key_down[KEY_DIRECTION_DOWN])
		d2y = +1;

	// Display our current position
	if (read_key_once(KEY_DEBUG_PRINT_POS))
	{
		printf("(px, p2y) = (%d, %d), room = %x, rem = %08X\n", prisoner_x, prisoner_2y, current_room_index, rem_bitmask);
		printf("game_time = %lld, program_time = %lld\n", game_time, program_time);
	}

	// Gimme some props!!!
	if (read_key_once(KEY_DEBUG_BONANZA))
	{
		for (i=1; i<NB_PROPS-1; i++)
			props[current_nation][i] += 10;
	}

	if (read_key_once(KEY_DEBUG_CATCH_HIM))
		prisoner_state ^= STATE_IN_PURSUIT;

	// Walk/Run toggle
	if ( read_key_once(KEY_TOGGLE_WALK_RUN) && (!in_tunnel) && 
		// Not checking for the following leads to issues
	     ( (prisoner_ani.index == RUN_ANI) || 
		   (prisoner_ani.index == WALK_ANI) ||
		   (prisoner_ani.index == GUARD_RUN_ANI) ||
		   (prisoner_ani.index == GUARD_WALK_ANI) ) 
	   )
	{
		if  ((prisoner_speed == 1) && (prisoner_fatigue < MAX_FATIGUE) )
		{
			prisoner_speed = 2;
			prisoner_ani.index = prisoner_as_guard?GUARD_RUN_ANI:RUN_ANI; 
		}
		else
		{
			prisoner_speed = 1;
			prisoner_ani.index = prisoner_as_guard?GUARD_WALK_ANI:WALK_ANI; 
		}
	}

	// We're idle, but we might be trying to open a tunnel exit, or use a prop
	if (read_key_once(KEY_FIRE))
	{
		// We need to set this variable as we might check if fire is pressed 
		// in various subroutines below
		is_fire_pressed = true;

		// Handle tunnel I/O through a check_footprint() call
		exit_nr = check_footprint(0,0);		// 0,0 indicates tunnel I/O
		if (exit_nr)
		{	// We just went through a tunnel exit
			// => Toggle tunneling state
			prisoner_state ^= STATE_TUNNELING;
			switch_room(exit_nr-1, true);
		}
		else
		{	// Are we trying to use some non tunnel I/O related prop?
			switch(selected_prop[current_nation])
			{
			case ITEM_GUARDS_UNIFORM:
				if (!prisoner_as_guard)
				{	// Only makes sense if we're not already dressed as guard
					prisoner_as_guard = true;
					prisoner_reset_ani = true;
					consume_prop();
					show_prop_count();
				}
				break;
			case ITEM_PRISONERS_UNIFORM:
				if (prisoner_as_guard)
				{	// Only makes sense if we're dressed as guard
					prisoner_as_guard = false;
					prisoner_reset_ani = true;
					consume_prop();
					show_prop_count();
				}
				break;
			case ITEM_STONE:
				break;
			default:
				break;
			}
		}
	}

	// The following keys are only handled if we are in a premissible state
	if (!(prisoner_state & MOTION_DISALLOWED))
	{

		// Inventory cycle
		if ( (read_key_once(KEY_INVENTORY_LEFT)) ||
			 (read_key_once(KEY_INVENTORY_RIGHT)) ) 
		{
			prop_id = selected_prop[current_nation];
			direction = key_down[KEY_INVENTORY_LEFT]?0x0F:1;
			do
				prop_id = (prop_id + direction) & 0x0F;
			while ( (!props[current_nation][prop_id]) && (prop_id != selected_prop[current_nation]) );
			if (props[current_nation][prop_id])
			// we found a non empty item
			{
				selected_prop[current_nation] = prop_id;
				// Display our props count
				update_props_message(prop_id);
				show_prop_count();
			}
			else
				selected_prop[current_nation] = 0;
		}

		// Inventory pickup/dropdown
		if ( ( (read_key_once(KEY_INVENTORY_PICKUP)) ||
			   (read_key_once(KEY_INVENTORY_DROP)) ) &&
			   (!(prisoner_state & STATE_TUNNELING)) )
		{
			prisoner_state |= STATE_KNEEL;
			// enqueue our 2 u8 parameters
			prisoner_ani.end_of_ani_parameter = (current_nation & 0xFF) | 
				((prisoner_ani.index << 8) & 0xFF00);
			prisoner_ani.index = prisoner_as_guard?GUARD_KNEEL_ANI:KNEEL_ANI;
			// Make sure we go through frame 0
			prisoner_ani.framecount = 0;
			prisoner_ani.end_of_ani_function = restore_params;

			if (key_down[KEY_INVENTORY_PICKUP])
			{	// picking up
				if (over_prop)
				{
					prop_offset = room_props[over_prop-1];
					room_props[over_prop-1] = 0;
					// change the room index to an invalid one
					writeword(fbuffer[OBJECTS],prop_offset,ROOM_NO_PROP);
					props[current_nation][over_prop_id]++;
					selected_prop[current_nation] = over_prop_id;
					show_prop_count();
				}
			}
			else
			{	// dropdown
				if (selected_prop[current_nation])
				{
					found = false;
					over_prop_id = selected_prop[current_nation];
					// OK, now we'll look for an picked object space in obs.bin to store 
					// our data
					for (prop_offset=2; prop_offset<(8*nb_objects+2); prop_offset+=8)
					{
						if (readword(fbuffer[OBJECTS],prop_offset) == ROOM_NO_PROP)
						{	// There should always be at least one
							// Add the prop to our current room
							room_props[nb_room_props] = prop_offset; 
							nb_room_props++;
							// Write down the relevant value in obs.bin
							// 1. Room number
							writeword(fbuffer[OBJECTS],prop_offset,current_room_index);
							// 2. x & y pos
							writeword(fbuffer[OBJECTS],prop_offset+4, prisoner_x + 15);
							writeword(fbuffer[OBJECTS],prop_offset+2, prisoner_2y/2 + 3);
							// 3. object id
							writeword(fbuffer[OBJECTS],prop_offset+6, over_prop_id);
							found = true;
							break;
						}
					}
					if (!found)		// Somebody's cheating!
						printf("Could not find any free prop variable => discarding prop.\n");

					props[current_nation][over_prop_id]--;
					if (props[current_nation][over_prop_id] == 0)
					// display the empty box if last prop
						selected_prop[current_nation] = 0;
					// don't care to much about reconstructing over_prop, as the next redisplay
					// will take care of it
					over_prop = 0;
					over_prop_id = 0;
				}
			}
		}

		if (read_key_once(KEY_STOOGE))
			play_sfx(0);
	}

	// Joystick motion overrides keys
	if (jdx)
		dx = jdx;
	if (jd2y)
		d2y = jd2y;

// CPU branches FTW!
update_motion:

	// This ensures that all the motions are in sync
	if ((game_time - last_ptime) > REPOSITION_INTERVAL)
	{
		last_ptime = game_time;
	
		// Update the guards positions (if not playing with guards disabled)
		if (!opt_no_guards && move_guards())
		{	// we have a collision with a guard => kill our motion
			// but before we do that, change our direction accordingly
			if (dx || d2y)
				prisoner_dir = directions[d2y+1][dx+1];
			dx = 0; d2y = 0;
			prisoner_state &= ~STATE_MOTION;
		}
		else 
		// Update our guy's position
			process_motion();
		// Do we have something going on with a prisoner (request, caught, release...)
		check_on_prisoners();
		// Only reset the fire action AFTER we processed motion
		is_fire_pressed = false;
		// Redisplay, as there might be a guard moving
		glut_display();
	}

	// Can't hurt to sleep a while if we're motionless, so that
	// we don't hammer down the CPU in a loop
//	if ((dx == 0) && (d2y == 0))
//		msleep(3);
		msleep(REPOSITION_INTERVAL/5);
}


// We'll use a different idle function for static picture
static void glut_idle_static_pic(void)
{
	// As usual, we'll need the current time value for a bunch of stuff
	update_timers();

	if ((game_state & GAME_STATE_INTRO) && read_key_once(KEY_FIRE))
	{	// Exit intro => start new game
		mod_release();
		// Reload the changed files if needed
		newgame_init(reload_needed);
		t_last = mtime();
		if (!reload_needed)
			// We'll need a reload from now on
			reload_needed = true;
		picture_state = PICTURE_FADE_OUT;
		game_state = GAME_STATE_STATIC_PIC;
		current_picture = INTRO_SCREEN_START;	// for next restart
	}
	else if ((game_state & GAME_STATE_GAME_OVER) && (!(picture_state == GAME_FADE_OUT)) 
		&& read_key_once(KEY_FIRE))
	{	// Exit game over/game won => Intro
		mod_release();
		picture_state = PICTURE_FADE_OUT;
		game_state = GAME_STATE_INTRO | GAME_STATE_STATIC_PIC | GAME_STATE_PICTURE_LOOP;
		current_picture = INTRO_SCREEN_START-1;
	}

	// Start by incrementing/decrementing fade value
	fade_value += fade_step[picture_state];

	// Have we reached a fade boundary
	if ((fade_value > 1.0f) || (fade_value < 0.0f))
	{
		fade_value = (fade_value < 0.0f)?0.0f:1.0f;
		picture_state++;
	}
/*
	printf("    fade_value = %f\n", fade_value);
	printf("    picture_state = %d\n", picture_state);
	printf("    fade_step = %f\n",  fade_step[picture_state]);
*/

	// Act on the various game states
	switch(picture_state)
	{
	case GAME_FADE_OUT:
		game_state &= ~GAME_STATE_ACTION;
		break;
	case PICTURE_FADE_IN:
		game_state |= GAME_STATE_STATIC_PIC;
		picture_t = program_time;
		// We use the picture fade in to load a new MOD if needed
		if (!is_mod_playing())
		{
			if (game_state & GAME_STATE_INTRO)
			{
				if (mod_init(mod_name[MOD_LOADTUNE]))
					mod_play();
				else
					printf("Failed to load Intro tune\n");
			}
			else if (game_state & GAME_STATE_GAME_OVER)
			{
				if (mod_init(mod_name[MOD_GAMEOVER]))
					mod_play();
				else
					printf("Failed to load Game Over tune\n");
			}
			else if (game_state & GAME_STATE_GAME_WON)
			{
				if (mod_init(mod_name[MOD_WHENWIN]))
					mod_play();
				else
					printf("Failed to load winning tune\n");
			}

		}
		break;
	case PICTURE_WAIT:
		if (read_key_once(KEY_FIRE) || 
			( (!(game_state & GAME_STATE_GAME_OVER)) && (program_time-picture_t > PICTURE_TIMEOUT)))
		{
			picture_state++;
			// Add the picture loop flag here on game won
			if (game_state & GAME_STATE_GAME_WON)
				game_state |= GAME_STATE_PICTURE_LOOP;
		}
		// We might want to call a function to update the prisoner's status at this stage
		if (static_screen_func != NULL)
		{
			static_screen_func(static_screen_param);
			static_screen_func = NULL;
		}
		break;
	case PICTURE_FADE_OUT:
		break;
	case GAME_FADE_IN:
		if (game_state & GAME_STATE_PICTURE_LOOP)
		{
			// No game fades for picture loop sequences
//			picture_state = PICTURE_FADE_IN;
			if (game_state & GAME_STATE_INTRO)
			{
				current_picture++;
				if (current_picture > INTRO_SCREEN_END)
				{
					current_picture = INTRO_SCREEN_START;
					// Add a 1 second black screen before the first intro screen
	//				glClear(GL_COLOR_BUFFER_BIT);
	//				glutSwapBuffers();
	//				msleep(1000);
				}
				static_screen(current_picture, NULL, 0);
			}
			else if (game_state & GAME_STATE_GAME_WON)
			{	// Switch to second & last GAME_WON picture
				// We add the GAME_OVER flag so that we can reuse the GAME OVER exit code
				game_state |= GAME_STATE_GAME_OVER;
				static_screen(PRISONER_FREE_ALL, NULL, 0);
			}
			else
				printf("Missing static_screen() call in static pic loop!\n");
		}
		else	
			game_state &= ~GAME_STATE_STATIC_PIC;
		break;
	case PICTURE_EXIT:
		game_state |= GAME_STATE_ACTION;
		// Set glut_idle to our main game loop
		glutIdleFunc(glut_idle_game);
		break;
	default:
		printf("glut_idle_static_pic(): should never be here!\n");
		break;
	}

	// Don't forget to display the image
	glut_display();

	// We should be able to sleep for a while
	msleep(PAUSE_DELAY);
}


// This function handles the displaying of static screens
// The last 2 parameters are for a function callback while we are in
// the middle of a static picture
void static_screen(u8 picture_id, void (*func)(u32), u32 param)
{
bool loaded_ok;

	// We need to store the current picture for idle_static_pic()
	current_picture = picture_id;

	// This function call will be initiated in idle_static_pic()
	static_screen_func = func;
	static_screen_param = param;

	// if the picture_id >= NB_IFFS then we need to load an display an
	// RGB RAW (with
	if (picture_id >= NB_IFFS)
		// The pictures are always padded to 512 in w
		loaded_ok = load_raw_rgb(PSP_SCR_WIDTH, PSP_SCR_HEIGHT, raw_name[picture_id-NB_IFFS]);
	else
		loaded_ok = load_iff(picture_id);
	if (!loaded_ok)
	{	// There was a problem loading the file
		if (static_screen_func != NULL)
		{	// If we don't execute the function, we might get stuck
			static_screen_func(static_screen_param);
			static_screen_func = NULL;
		}
		return;
	}

	// start with the following picture state
	if (game_state & GAME_STATE_PICTURE_LOOP)
	{
		picture_state = PICTURE_FADE_IN;
		fade_value = 0.0f;
	}
	else
	{
		picture_state = GAME_FADE_OUT;
		fade_value = 1.0f;
		// Switch to a different game fucntion
		glutIdleFunc(glut_idle_static_pic);
	}

}


static void glut_joystick(uint buttonMask, int x, int y, int z)
{
	// compute x and y displacements
	if (x>JOY_DEADZONE)
		jdx = 1;
	else if (x<-JOY_DEADZONE)
		jdx = -1;
	else 
		jdx = 0;
	if (y>JOY_DEADZONE)
		jd2y = 1;
	else if (y<-JOY_DEADZONE)
		jd2y = -1;
	else jd2y = 0;
}

// Keyboard handling
static void glut_keyboard(u8 key, int x, int y)
{
//	printf("key = %X (%c)\n", key, key);
	key_down[key] = true;
#if defined (CHEATMODE_ENABLED)
	last_key_used = key;
#endif

}

static void glut_keyboard_up(u8 key, int x, int y)
{
	key_down[key] = false;
	key_readonce[key] = false;
#if defined (CHEATMODE_ENABLED)
	key_cheat_readonce[key] = false;
#endif
}

static void glut_special_keys(int key, int x, int y)
{
	key_down[key + SPECIAL_KEY_OFFSET] = true;
#if defined (CHEATMODE_ENABLED)
	last_key_used = key + SPECIAL_KEY_OFFSET;
#endif

}

static void glut_special_keys_up(int key, int x, int y)
{
	key_down[key + SPECIAL_KEY_OFFSET] = false;
	key_readonce[key + SPECIAL_KEY_OFFSET] = false;
#if defined (CHEATMODE_ENABLED)
	key_cheat_readonce[key + SPECIAL_KEY_OFFSET] = false;
#endif
}

static void glut_mouse_buttons(int button, int state, int x, int y)
{
	if (state == GLUT_DOWN)
	{
		key_down[SPECIAL_MOUSE_BUTTON_BASE + button] = true;
#if defined (CHEATMODE_ENABLED)
		last_key_used = SPECIAL_MOUSE_BUTTON_BASE + button;
#endif
	}
	else
	{
		key_down[SPECIAL_MOUSE_BUTTON_BASE + button] = false;
		key_readonce[SPECIAL_MOUSE_BUTTON_BASE + button] = false;
#if defined (CHEATMODE_ENABLED)
		key_cheat_readonce[SPECIAL_MOUSE_BUTTON_BASE + button] = false;
#endif
	}
}


/* Here we go! */
int main (int argc, char *argv[])
{

	// Flags
	int opt_error 			= 0;	// getopt
	int opt_sfx				= 0;

	// General purpose
	u32  i;



#if defined(PSP)
	setup_callbacks();
//	pspDebugInstallStdoutHandler(pspDebugScreenPrintData);
	gl_width = PSP_SCR_WIDTH;
	gl_height = PSP_SCR_HEIGHT;
#else
	gl_width = 2*PSP_SCR_WIDTH;
	gl_height = 2*PSP_SCR_HEIGHT;
#endif

	// Well, we're supposed to call that blurb
	glutInit(&argc, argv);

	// Need to have a working GL before we proceed. This is our own init() function
	glut_init();

	// Let's clean up our buffers
	fflush(stdin);
	mbuffer    = NULL;
	for (i=0; i<NB_FILES; i++)
		fbuffer[i] = NULL;

	// Process commandline options (works for PSP too with psplink)
	while ((i = getopt (argc, argv, "vbghs:f:")) != -1)
		switch (i)
	{
		case 'v':		// Print verbose messages
			opt_verbose = -1;
			break;
		case 'b':       // Debug mode
			opt_debug = -1;
			break;
		case 'g':		// walk through walls
			opt_ghost = -1;
			break;
		case 's':
			sscanf(optarg, ("%x"), &opt_sid); 
			break;
		case 'f':
			sscanf(optarg, ("%d"), &opt_sfx); 
			break;
		case 'h':
		default:		// Unknown option
			opt_error++;
			break;
	}

	puts ("");
	puts ("Colditz v1.0 : Escape from Colditz port");
	puts ("by Aperture Software,  May 2009");
	puts ("");

	if ( ((argc-optind) > 3) || opt_error)
	{
		puts ("usage: Colditz [-f] [-s] [-v] [device] [kernel] [general]");
		puts ("Most features are autodetected, but if you want to specify options:");
		puts ("                -f : force");
		puts ("                -s : skip TIFF image creation");
		puts ("                -v : verbose");
		puts ("");
		exit (1);
	}
//	opt_verbose = -1;


	// Allocate the bitmap image buffer (for displaying static IFF or RAW RGB 
	// images). The padded size of all our static image textures is always 
	// 512x256x2 (IFF) or 512x512x3 (RAW RGB) bytes
	// NOTE that because we don't want to waste the 512x(512-272)x3 (=360 KB) extra
	// space on the PSP (the PSP can only deal with texture dimensions that are in 
	// power of twos), we are doing something really dodgy here, which is that we
	// pretend the extra 360K have been allocated, and let the system blatantly 
	// overflow our image buffer if it wants to (since it should never display that 
	// data anyway). As a matter of fact, this is why we allocate this buffer first.
	// 
	// 
	// If you're really unhappy about this kind of practice, just allocate 512x512x3
	static_image_buffer = (u8*) aligned_malloc(512*PSP_SCR_HEIGHT*3,16);
//	static_image_buffer = (u8*) aligned_malloc(512*512*3,16);	
	if (static_image_buffer == NULL)
	{
		printf("Could not allocate buffer for static images display\n");
		return 0;
	}

	// Load the data. If it's the first time we run the game, we might have
	// to uncompress LOADTUNE.MUS (PowerPack) and SKR_COLD (custom compression)
	load_all_files();
	depack_loadtune();

	// Some of the files need patching (this was done too in the original game!)
	fix_files(false);

	// Set global variables
	t_last = mtime();
	program_time = 0;
	game_time = 0;
	set_global_properties();

	// We might want some sound
	if (!audio_init())
		printf("Could not Initialize audio\n");
		
	// We're going to convert the cells array, from 2 pixels per byte (paletted)
	// to on RGB(A) word per pixel
	rgbCells = (u8*) aligned_malloc(fsize[CELLS]*2*RGBA_SIZE, 16);
	if (rgbCells == NULL)
	{
		perr("Could not allocate RGB Cells buffers\n");
		ERR_EXIT;
	}

	// Get a palette we can work with
	to_16bit_palette(palette_index, 0xFF, PALETTES);

	// Convert the cells to RGBA data
	cells_to_wGRAB(fbuffer[CELLS],rgbCells);

	// Do the same for overlay sprites
	init_sprites();
	sprites_to_wGRAB();

	aper_blurb = load_raw_rgba(256, 64, "blurb-drop.raw", &aper_blurb_texid);

	// We start with the Intro state
	game_state = GAME_STATE_INTRO | GAME_STATE_STATIC_PIC | GAME_STATE_PICTURE_LOOP;
	static_screen(APERTURE_SOFTWARE, NULL, 0);
//	static_screen(INTRO_SCREEN_START, NULL, 0);


	// Now we can proceed with setting up our display
	glutDisplayFunc(glut_display);
	glutReshapeFunc(glut_reshape);

	glutKeyboardFunc(glut_keyboard);
	glutKeyboardUpFunc(glut_keyboard_up);
	glutSpecialFunc(glut_special_keys);
	glutSpecialUpFunc(glut_special_keys_up);
	glutMouseFunc(glut_mouse_buttons);

	glutJoystickFunc(glut_joystick,30);	
	// This is what you get from using obsolete libraries!
	// bloody joystick callback doesn't work on Windows,
	// so we have to stuff the movement handling in idle!!!
	glutIdleFunc(glut_idle_static_pic);

	glutMainLoop();


	return 0;
}
