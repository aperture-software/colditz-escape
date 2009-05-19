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
#include <pspdebug.h>
#include <pspctrl.h>
#include <pspgu.h>
#include <psprtc.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glut.h>
#include "psp-setup.h"
#endif

#include "getopt.h"	
#include "colditz.h"
#include "low-level.h"
#include "utilities.h"


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


// File stuff
FILE* fd					= NULL;
char* fname[NB_FILES]		= FNAMES;			// file name(s)
u32   fsize[NB_FILES]		= FSIZES;
u8*   fbuffer[NB_FILES];
u8*   mbuffer				= NULL;
u8*	  rgbCells				= NULL;
u8*   static_image_buffer   = NULL;
char* iff_name[NB_IFFS]		= IFF_NAMES;
u16   iff_payload_w[NB_IFFS] = IFF_PAYLOAD_W;
u16   iff_payload_h[NB_IFFS] = IFF_PAYLOAD_H;

// Indicate whether we are running the game or displaying
// a static IFF image
bool  static_picture		= false;
// Current static image IFF to load and display
u16   current_iff = 0x0C;
// Used for fade in/fade out of static images
float fade_value = FADE_START_VALUE;
// 1 for fade in, -1 for fade out
int fade_direction = 0;

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

bool init_animations = true;
bool keep_message_on = false;
s_animation	animations[MAX_ANIMATIONS];
u8	nb_animations = 0;
// last time for Animations and rePosition of the guards
u64 last_atime, last_ptime, last_ctime;
u64 keep_message_mtime_start;
s_guybrush guybrush[NB_GUYBRUSHES];
s_event	events[NB_EVENTS];
u8	props[NB_NATIONS][NB_PROPS];
u8	selected_prop[NB_NATIONS];
u8	nb_room_props = 0;
u16	room_props[NB_OBSBIN];
u8	over_prop = 0, over_prop_id = 0;
u8  current_nation = 0;
u8	panel_chars[NB_PANEL_CHARS][8*8*2];
char* status_message;
bool paused = false;
u64  paused_time;
u8  hours_digit_h, hours_digit_l, minutes_digit_h, minutes_digit_l;
u8  tunexit_nr, tunexit_flags, tunnel_tool;
u8*	iff_image;

u16  nb_rooms, nb_cells, nb_objects;
u8	 palette_index = 4;
s_sprite*	sprite;
s_overlay*	overlay;
u8   overlay_index;
u8   bPalette[3][16];
// remapped Amiga Palette
u16  aPalette[32];
char nb_props_message[32] = "\499 * ";

// offsets to sprites according to joystick direction (dx,dy)
s16 directions[3][3] = { {3,2,4}, {0,DIRECTION_STOPPED,1}, {6,5,7} };
// reverse table for dx and dy
s16 dir_to_dx[8] = {-1, 1, 0, -1, 1, 0, -1, 1};
s16 dir_to_d2y[8] = {0, 0, -1, -1, -1, 1, 1, 1};
// The direct nation keys might not be sequencial on custom key mapping
u8 key_nation[NB_NATIONS+2] = {KEY_BRITISH, KEY_FRENCH, KEY_AMERICAN, KEY_POLISH, 
							 KEY_PRISONERS_LEFT, KEY_PRISONERS_RIGHT};


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


static void glut_display(void)
{
	// Always start with a clear to black
	glClear(GL_COLOR_BUFFER_BIT);

	// Display either the current game frame or a static picture
	if (static_picture)
		display_picture();
	else
	{	// In game => update room content and panel
		display_room();
		display_panel();
	}
#if !defined (PSP)
	rescale_buffer();
#endif
	glutSwapBuffers();
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
//			animations[prisoner_ani].direction = directions[d2y+1][dx+1];
			dx = 0;
			d2y = 0;
		}
	}
	// We're idle, but we might be trying to open a tunnel exit
	else if (read_key_once(KEY_FIRE))
	{
		// Start by checking where we stand
		check_footprint(0,0);
		if (tunexit_nr)
		{	// Tunnel it is
// TO_DO: direct exit when using shovel inside a tunnel
//        remove the need for a move on selet candle (issue a check_footprint?)

			// The opening of a tunnel exit is done in 2 times
			// 1. If the exit is closed, use the right tool (saw, shovel or pick-axe) to open it
			if (!(tunexit_flags & 0x10))
			{	// exit is closed
				if ( (opt_keymaster) || (selected_prop[current_nation] == tunnel_tool) )
				{	// Toggle the exit open and consume the relevant item
					printf("opening exit\n");
					consume_prop();
					toggle_exit(tunexit_nr-1);
				}
			}
			// If the exit is open and we're in a tunnel, just exit
			// Otherwise, consume a candle
			else if ( (opt_keymaster) || (in_tunnel) ||
				      (selected_prop[current_nation] == ITEM_CANDLE) )
			{
				printf("exit already open\n");
				if (!in_tunnel)
				// Only consume the candle
				// NB: works because we have a if !keymaster in the function
					consume_prop();
				// Toggle tunneling state
				prisoner_state ^= STATE_TUNNELING;
				switch_room(tunexit_nr-1, true);
			}
		}
	}

	// Get direction (which is used as an offset to pick the proper animation
	new_direction = directions[d2y+1][dx+1];
	// NB: if d2y=0 & dx=0, new_dir = DIRECTION_STOPPED

	if (new_direction != DIRECTION_STOPPED)
	{	// We're moving => animate sprite

		if (!(prisoner_state & STATE_MOTION))
		// we were stopped => make sure we start with the proper ani frame
			animations[prisoner_ani].framecount = 0;

		// Update our prisoner data
		prisoner_x += prisoner_speed*dx;
		prisoner_2y += prisoner_speed*d2y;
		prisoner_state |= STATE_MOTION;
		// Update the animation direction
		prisoner_dir = new_direction;
//		animations[prisoner_ani].direction = new_direction;
	}
	else if (prisoner_state & STATE_MOTION)
	{	// We just stopped
		prisoner_state ^= STATE_MOTION;
	}
}

// restore guybrush & animation parameters after a one shot ani
// this function expects the guybrush index as well as the previous ani_index
// to be concatenated in the lower 2 bytes of the parameter
void restore_params(u32 param)
{
	u8 brush, ani, previous_index;
	// extract the guybrush index
	brush = param & 0xFF;
	// extract the previous animation index
	previous_index = (param >> 8) & 0xFF;

	ani = guybrush[brush].ani_index;
	animations[ani].index = previous_index;
	animations[ani].framecount = 0;
	animations[ani].end_of_ani_function = NULL;
	// we always end up in stopped state after a one shot animation
	// for now we clear anything except tunneling
	guybrush[brush].state &= STATE_TUNNELING;
	//if (guybrush[brush].state & STATE_KNEEL)
}

// process motion keys
static void glut_idle(void)
{	
	u16 prop_offset;
	u8	prop_id, direction, i;
	u64	t;

	// Return to intro screen
	if (key_down[KEY_ESCAPE])
		exit(0);

	// We'll need the current time value for a bunch of stuff
	t = mtime();

	// Handle the displaying of static pictures, with fading
	if (static_picture)
	{
		// Make sure we initialize a pause if needed
		if (!paused)
		{
			// Pause the game and set static picture variables
			paused_time = t;
			fade_direction = 1;
			paused = true;
		}

		// Handle fading in/out of static pictures
		if (fade_direction)
		{
			fade_value += fade_direction*0.1f;

			// End of fading in
			if (fade_value > 1.0f)
			{
				fade_value = 1.0f;
				fade_direction = 0;
			}

			// End of fading out
			if (fade_value < 0.0f)
			{
				// for next fade_in
				fade_value = FADE_START_VALUE;
				fade_direction = 0;
				static_picture = false;
				// This makes sure we reset time variables and pause
				key_readonce[KEY_PAUSE] = false;
				key_down[KEY_PAUSE] = true;	
			}

			// Don't forget to display the image
			glut_display();
		}

		// Fire to exit the display of a static picture
		if (read_key_once(KEY_FIRE))
			fade_direction = -1;
	}
	else
	{
		// Toggle pause (if a static picture is not being displayed)
		if (read_key_once(KEY_PAUSE))
		{
			if (paused)
			{
				last_atime += (t - paused_time);
				last_ptime += (t - paused_time);
				last_ctime += (t - paused_time);
				paused = false;
			}
			else
			{
				paused_time = t;
				paused = true;
			}
		}
	}

	// Handle paused state (sleep & exit)
	if (paused)
	{
		// We should be able to sleep for a while
		msleep(PAUSE_DELAY);
		return;
	}

	// Handle animations and timed events on regular basis
	if ((t - last_atime) > ANIMATION_INTERVAL)
	{
		last_atime += ANIMATION_INTERVAL;	// closer to ideal rate

		// but leads to catchup effect when moving window
		for (i = 0; i < nb_animations; i++)
			animations[i].framecount++;

		// Clock minute tick?
		if ((t - last_ctime) > TIME_MARKER)
		{
			last_ctime += TIME_MARKER;
			minutes_digit_l++;
			if (minutes_digit_l == 10)
			{
				minutes_digit_h++;
				minutes_digit_l = 0;
				if (minutes_digit_h == 6)
				{
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
				}
			}
		}

		// Check for timed events
		timed_events(hours_digit_h*10+hours_digit_l, minutes_digit_h, minutes_digit_l);

		// Execute timed events, if any are in the queue
		for (i = 0; i<NB_EVENTS; i++)
		{
			if (events[i].function == NULL)
				continue;
			if (t > events[i].expiration_time)
			{	// Execute the timeout function
				events[i].function(events[i].parameter);
				// Make the event available again
				events[i].function = NULL;
			}
		}

		// Take care of message display
		if ((keep_message_on) && (t > keep_message_mtime_start + KEEP_MESSAGE_DURATION))
			keep_message_on = false;

		// Reset the status message if needed
		if (!keep_message_on)
			status_message = NULL;
	}

	// Reset the motion
	dx = 0; 
	d2y = 0;	

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
		printf("(px, p2y) = (%d, %d), room = %x, rem = %08X\n", prisoner_x, prisoner_2y, current_room_index, rem_bitmask);

	// Gimme some props
	if (read_key_once(KEY_DEBUG_BONANZA))
	{
		for (i=1; i<NB_PROPS; i++)
			props[current_nation][i] += 10;
	}

	// Walk/Run toggle
	if (read_key_once(KEY_TOGGLE_WALK_RUN))
	{
		if (prisoner_speed == 1)
		{
			prisoner_speed = 2;
			animations[prisoner_ani].index = RUN_ANI; // +GERMAN OFFSET
		}
		else
		{
			prisoner_speed = 1;
			animations[prisoner_ani].index = WALK_ANI;
		}
		animations[prisoner_ani].framecount = 0;
	}

	// The following keys are only handled if we are in a premissible state
	if (!(prisoner_state & MOTION_DISALLOWED))
	{

		// Prisoner selection: direct keys or left/right cycle keys
		for (i=0; i<(NB_NATIONS+2); i++)
			if (read_key_once(key_nation[i]))
			{
				// stop our current prisoner
				prisoner_state &= ~STATE_MOTION;
				if (i<NB_NATIONS)
					current_nation = i;
				else
					// we want to have +/-1, without going negative, and we
					// know that we are either at NB_NATIONS or NB_NATIONS+1
					// so the formula writes itself
					current_nation = (current_nation+(2*i)-1) % NB_NATIONS;
				init_animations = true;
				keep_message_on = false;
				set_room_props();
				break;
			}

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
				keep_message_on = true;
				keep_message_mtime_start = t;
			}
			else
				selected_prop[current_nation] = 0;

//			printf("selected prop = %02X\n", selected_prop[current_nation]);
		}

		// Inventory pickup/dropdown
		if ( ( (read_key_once(KEY_INVENTORY_PICKUP)) ||
			   (read_key_once(KEY_INVENTORY_DROP)) ) &&
			   (!(prisoner_state & STATE_TUNNELING)) )
		{
			prisoner_state |= STATE_KNEEL;
			// enqueue our 2 u8 parameters
			animations[prisoner_ani].end_of_ani_parameter = (current_nation & 0xFF) | 
				((animations[prisoner_ani].index << 8) & 0xFF00);
			animations[prisoner_ani].index = KNEEL_ANI;
//			animations[prisoner_ani].
			//animations[prisoner_ani].index = SHOT_ANI;
			// starting at -1 ensures that we'll go through frame 0
			// as we need to wait the for the ani counter to increase
			// for our first animation frame to show
			// NO LONGER TRUE DAMMIT!!!!
			animations[prisoner_ani].framecount = 0;
			animations[prisoner_ani].end_of_ani_function = restore_params;

			if (key_down[KEY_INVENTORY_PICKUP])
			{	// picking up
				if (over_prop)
				{
//					over_prop--;	// restore the zero based index
					prop_offset = room_props[over_prop-1];
					room_props[over_prop-1] = 0;
					// change the room index to an invalid one
					writeword(fbuffer[OBJECTS],prop_offset,ROOM_NO_PROP);
//					prop_id = readbyte(fbuffer[OBJECTS],prop_offset+7);
//					prop_id = over_prop_id;
					props[current_nation][over_prop_id]++;
					selected_prop[current_nation] = over_prop_id;
					update_props_message(over_prop_id);
					keep_message_on = true;
					keep_message_mtime_start = t;
//					for (i = 0; i< 16; i++)
//						printf(" %02X", props[current_nation][i]);
//					printf("\nselected prop = %02X\n", selected_prop[current_nation]);
				}
			}
			else
			{	// dropdown
				if (selected_prop[current_nation])
				{
//					prop_offset = room_props[over_prop];
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
							break;
						}
					}

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
	}

	// Joystick motion overrides keys
	if (jdx)
		dx = jdx;
	if (jd2y)
		d2y = jd2y;

	// This ensures that all the motions are in sync
	if ((t - last_ptime) > REPOSITION_INTERVAL)
	{
		last_ptime = t;
		// Update the guards positions (if not playing with guards disabled)
		if (!opt_no_guards && move_guards())
		{	// we have a collision with a guard => kill our motion
			// but before we do that, change our direction accordingly
			if (dx || d2y)
				prisoner_dir = directions[d2y+1][dx+1];
			dx = 0; d2y = 0;
		}
		// Upate our guy's position
		process_motion();
		// Always redisplay, as there might be a guard moving
		glut_display();
	}

	// Can't hurt to sleep a while if we're motionless, so that
	// we don't hammer down the CPU in a loop
//	if ((dx == 0) && (d2y == 0))
//		msleep(3);
		msleep(REPOSITION_INTERVAL/5);
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
}

static void glut_keyboard_up(u8 key, int x, int y)
{
	key_down[key] = false;
	key_readonce[key] = false;
}

static void glut_special_keys(int key, int x, int y)
{
//	printf("special key = %X\n", key);
	key_down[key + SPECIAL_KEY_OFFSET] = true;
}

static void glut_special_keys_up(int key, int x, int y)
{
	key_down[key + SPECIAL_KEY_OFFSET] = false;
	key_readonce[key + SPECIAL_KEY_OFFSET] = false;
}

static void glut_mouse_buttons(int button, int state, int x, int y)
{
//	printf("mouse button %X, %s\n", button, (state==GLUT_DOWN)?"down":"up");
	if (state == GLUT_DOWN)
		key_down[SPECIAL_MOUSE_BUTTON_BASE + button] = true;
	else
	{
		key_down[SPECIAL_MOUSE_BUTTON_BASE + button] = false;
		key_readonce[SPECIAL_MOUSE_BUTTON_BASE + button] = false;
	}
}


/* Here we go! */
int main (int argc, char *argv[])
{

	// Flags
	int opt_error 			= 0;	// getopt

	// General purpose
	u32  i,j;


#if defined(PSP)
	setup_callbacks();
	gl_width = PSP_SCR_WIDTH;
	gl_height = PSP_SCR_HEIGHT;
#else
	gl_width = 2*PSP_SCR_WIDTH;
	gl_height = 2*PSP_SCR_HEIGHT;
#endif

	// Initialize our timer values
	last_ctime = mtime();
	last_atime = last_ctime;
	last_ptime = last_ctime;

	// start time
	hours_digit_h = 0;
	hours_digit_l = 9;
	minutes_digit_h = 3;
	minutes_digit_l = 5;

	glutInit(&argc, argv);

	// Let's clean up our buffers
	fflush(stdin);
	mbuffer    = NULL;
	for (i=0; i<NB_FILES; i++)
		fbuffer[i] = NULL;

	// Process commandline options (works for PSP too with psplink)
	while ((i = getopt (argc, argv, "vbghs:")) != -1)
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

	// Need to have a working GL before we proceed
	glut_init();

	// Load the data
	load_all_files();

	// fix some of the files
	fix_files();

	// Allocate the bitmap image buffer (for displaying static IFF images)
	// The padded size of all our IFF textures is always 512x256
	static_image_buffer = (u8*) aligned_malloc(512*256*2,16);
	if (static_image_buffer == NULL)
	{
		perr("Could not allocate buffer for static images display\n");
		return 0;
	}

	// Set global variables
	get_properties();


	// clear the events array
	for (i=0; i< NB_EVENTS; i++)
		events[i].function = NULL;


	current_nation = 0;
	// English
	i = 0;
	guybrush[i].px = 940;
	guybrush[i].p2y = 630;
	guybrush[i].room = ROOM_OUTSIDE;
	guybrush[i].state = 0;
	guybrush[i].speed = 1;
	guybrush[i].direction = 0;
	guybrush[i].ext_bitmask = 0x8000001E;
	i++;

	// French
	guybrush[i].px = 404;
	guybrush[i].p2y = 707;
	guybrush[i].room = ROOM_OUTSIDE;
	guybrush[i].state = 0;
	guybrush[i].speed = 1;
	guybrush[i].direction = 0;
	guybrush[i].ext_bitmask = 0x8000007E;
	i++;

	// American
	guybrush[i].px = 1200;
	guybrush[i].p2y = 700;
	guybrush[i].room = ROOM_OUTSIDE;
	guybrush[i].state = 0;
	guybrush[i].speed = 1;
	guybrush[i].direction = 0;
	guybrush[i].ext_bitmask = 0x8000001E;
	i++;

	// Polish
	guybrush[i].px = 1000;
	guybrush[i].p2y = 600;
	guybrush[i].room = ROOM_OUTSIDE;
	guybrush[i].state = 0;
	guybrush[i].speed = 1;
	guybrush[i].direction = 0;
	guybrush[i].ext_bitmask = 0x8000001E;
	i++;

	for (j=0;j<NB_GUARDS;j++)
	{
//		guybrush[i].ani_index = i;
//		animations[i].index = GUARD_WALK_ANI;
		guybrush[i].px = readword(fbuffer[GUARDS],j*MENDAT_ITEM_SIZE + 2);
		guybrush[i].p2y = 2*readword(fbuffer[GUARDS],j*MENDAT_ITEM_SIZE);
		guybrush[i].room = readword(fbuffer[GUARDS],j*MENDAT_ITEM_SIZE+ 4);
//		printf("%x: (%d,%d) %x\n", j, guybrush[i].px, guybrush[i].p2y,guybrush[i].room);
		guybrush[i].state = 0;
		guybrush[i].speed = 1;
		guybrush[i].direction = 0;
		guybrush[i].wait = 0;
		i++;
	}
		
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

	// Don't forget to set the room props
	set_room_props();
	for (i = 0; i<NB_NATIONS; i++)
		selected_prop[i] = 0;

	// Now we can proceed with our display
	glutDisplayFunc(glut_display);
	glutReshapeFunc(glut_reshape);

	glutKeyboardFunc(glut_keyboard);
	glutKeyboardUpFunc(glut_keyboard_up);
	glutSpecialFunc(glut_special_keys);
	glutSpecialUpFunc(glut_special_keys_up);
	glutMouseFunc(glut_mouse_buttons);

	glutJoystickFunc(glut_joystick,30);	
	// This is what you get from using obsolete libraries
	// bloody joystick callback doesn't work on Windows,
	// so we have to stuff the movement handling in idle!!!
	glutIdleFunc(glut_idle);

	glutMainLoop();

	return 0;
}
