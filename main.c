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
 *  main.c: Game engine entrypoint and GLUT functions
 *  ---------------------------------------------------------------------------
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#if defined(WIN32)
#include <windows.h>
#include <GL/glu.h>
#include "GL/wglext.h"
// Tell VC++ to include the GL libs
#pragma comment(lib, "opengl32.lib")
#pragma comment(lib, "glu32.lib")
// Glew is only required for the GLSL HQnX shaders
#pragma comment(lib, "glew32s.lib")
// Prevents a potential "LINK : fatal error LNK1104: cannot open file 'libc.lib'" error with Glew
#pragma comment(linker, "/NODEFAULTLIB:libc.lib")
#elif defined(PSP)
#include <psppower.h>
#include "psp/setup.h"
#include "psp/printf.h"
#endif

#include "getopt.h"
#include "low-level.h"
#include "colditz.h"
#include "graphics.h"
#include "game.h"
#include "soundplayer.h"
#include "conf.h"
#include "anti-tampering.h"

// Global variables

#if defined(PSP_ONSCREEN_STDOUT)
// What the bleep is so hard about flattening ALL known global names from ALL objects
// & libs into a big happy pool before linking and figuring out where to pick it up?!?
void glut_idle_suspended(void);		// Needs a proto for init below
void (*work_around_stupid_linkers_glut_idle_suspended)(void) = glut_idle_suspended;
void (*work_around_stupid_linkers_glutIdleFunc)(void (*func)(void)) = glutIdleFunc;
#endif

// Flags
int debug_flag					= 0;
bool opt_verbose				= false;
// Console debug
bool opt_debug					= false;
// Skip intro
bool opt_skip_intro				= false;
// Additional oncreen debug info
bool opt_onscreen_debug			= false;
bool opt_display_fps			= false;
// Do I hear a safecracker?
bool opt_play_as_the_safe[NB_NATIONS]
                                = {false, false, false, false};
// indifferent guards
bool opt_meh					= false;
// Use half size (i.e. original) resolution on Windows
bool opt_halfsize				= false;
// Who needs keys?
bool opt_keymaster				= false;
// "'coz this is triller!..."
bool opt_thrillerdance			= false;
// Force a specific sprite ID for our guy
// NB: must be init to -1
int opt_sid						= -1;
// Kill the guards
bool opt_no_guards				= false;
// Is the castle haunted by the ghost of shot prisoners
bool opt_haunted_castle			= false;	/* NOT IMPLEMENTED */
// Current static picture to show
int current_picture				= INTRO_SCREEN_START;
// Our second local pause variable, to help with the transition
bool display_paused				= false;
// we might need to suspend the game for videos, debug, etc
bool game_suspended				= false;
// We need to share these between game & gfx
bool init_animations			= true;
bool is_fire_pressed			= false;
// Used for fade in/fade out of static images
float fade_value				= 0.0f;
// false for fade in, true for fade out
bool fade_out					= false;
// Save config.xml?
bool config_save				= false;
// Is the GPU recent enough to support GLSL shaders
bool opt_has_shaders			= false;
// Prevents double consumption of keys while opening a door
bool can_consume_key			= true;


// We'll need this to retrieve our glutIdle function after a suspended state
// (but make sure we don't change idle if already suspended, which can happen on PSP printf)
#define glutIdleFunc_save(f) {if (!game_suspended) glutIdleFunc(f); restore_idle = f;}

// File stuff
FILE* fd					= NULL;
char* fname[NB_FILES]		= FNAMES;		// file name(s)
uint32_t fsize[NB_FILES]	= FSIZES;
uint8_t* fbuffer[NB_FILES] = { NULL };
uint8_t* rbuffer			= NULL;
uint8_t* mbuffer			= NULL;
uint8_t* rgbCells			= NULL;
uint8_t* static_image_buffer = NULL;
s_tex texture[NB_TEXTURES]	= TEXTURES;
char* mod_name[NB_MODS]		= MOD_NAMES;
const char confname[]		= "config.ini";
#if defined(ANTI_TAMPERING_ENABLED)
const uint8_t fmd5hash[NB_FILES][16] = FMD5HASHES;
#endif


// OpenGL window size
int		gl_width, gl_height;
int16_t	last_p_x = 0, last_p_y = 0;
int16_t	dx = 0, d2y = 0;
int16_t	jdx, jd2y;
// Key modifiers for glut
int		glut_mod;

// Key handling
bool	key_down[256], key_readonce[256];
static	__inline bool read_key_once(uint8_t k)
{
    if (key_down[k])
    {
        if (key_readonce[k])
            return false;
        key_readonce[k] = true;
        return true;
    }
    return false;
}


#if defined (CHEATMODE_ENABLED)
// We don't want to pollute the key_readonce table for cheat keys, yet
// we need to check if the cheat keys have been pressed once
bool key_cheat_readonce[256];
uint8_t last_key_used = 0;
static __inline bool read_cheat_key_once(uint8_t k)
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
    uint8_t  nb_keys;
    uint8_t  cur_pos;
    uint8_t* keys;
} s_cheat_sequence;

// Cheat definitions
static uint8_t konami[] = {SPECIAL_KEY_UP, SPECIAL_KEY_UP, SPECIAL_KEY_DOWN, SPECIAL_KEY_DOWN,
                           SPECIAL_KEY_LEFT, SPECIAL_KEY_RIGHT, SPECIAL_KEY_LEFT, SPECIAL_KEY_RIGHT};
static uint8_t namiko[] = {SPECIAL_KEY_DOWN, SPECIAL_KEY_DOWN, SPECIAL_KEY_UP, SPECIAL_KEY_UP,
                           SPECIAL_KEY_RIGHT, SPECIAL_KEY_LEFT, SPECIAL_KEY_RIGHT, SPECIAL_KEY_LEFT};
// NB: I have seen NO EVIDENCE WHATSOEVER in the disassembly that the first 3 cheats below were
// ever implemented in the original game. But just for the sake of it...
static uint8_t want_it_all[] = "i want everything";
static uint8_t keymaster[]   = "keymaster";
static uint8_t die_die[]     = "die die die";
static uint8_t no_cake[]     = "the cake is a lie";
static uint8_t as_safe[]     = "safecracker";
static uint8_t thriller[]    = "thrillerdance";
s_cheat_sequence cheat_sequence[] = {
    {SIZE_A(konami), 0, konami},
    {SIZE_A(want_it_all)-1, 0, want_it_all},
    {SIZE_A(keymaster)-1, 0, keymaster},
    {SIZE_A(die_die)-1, 0, die_die},
    {SIZE_A(no_cake)-1, 0, no_cake},
    {SIZE_A(as_safe)-1, 0, as_safe},
    {SIZE_A(thriller)-1, 0, thriller},
    {SIZE_A(namiko), 0, namiko}
};
#define NB_CHEAT_SEQUENCES	SIZE_A(cheat_sequence)
#define CHEAT_KONAMI		0
#define CHEAT_PROP_BONANZA	1
#define CHEAT_KEYMASTER		2
#define CHEAT_NOGUARDS		3
#define NO_CAKE_FOR_YOU		4
#define CHEAT_SAFECRACKER	5
#define THRILLERDANCE		6
#define NAMIKO				7	// Poor PSP owners, can't access any valuable cheat!
#endif

bool		found;
uint64_t	game_time, program_time, last_atime, last_ptime, last_ctime;
uint64_t	t_last, t_status_message_timeout, transition_start;
uint64_t	picture_t;
char*		status_message;
int			status_message_priority;
s_event		events[NB_EVENTS];
s_prisoner_event p_event[NB_NATIONS];
uint8_t		nb_room_props = 0;
uint8_t		props[NB_NATIONS][NB_PROPS];
uint8_t		selected_prop[NB_NATIONS];
uint16_t	room_props[NB_OBSBIN];
uint8_t		over_prop = 0, over_prop_id = 0;
char		nb_props_message[32] = "\499 * ";
uint8_t		current_nation = 0;
uint16_t	game_state;
uint8_t		hours_digit_h, hours_digit_l, minutes_digit_h, minutes_digit_l;
uint8_t		tunexit_nr, tunexit_flags, tunnel_tool;
uint8_t*	iff_image;
void		(*static_screen_func)(uint32_t) = NULL;
void		(*restore_idle)(void) = NULL;
uint32_t	static_screen_param;
uint16_t	nb_objects;
uint8_t		palette_index = INITIAL_PALETTE_INDEX;
uint8_t		picture_state;
// offsets to sprites according to joystick direction (dx,dy)
const int16_t directions[3][3] = { {3,2,4}, {0,8,1}, {6,5,7} };
// reverse table for dx and dy
const int16_t dir_to_dx[9]  = {-1, 1, 0, -1, 1, 0, -1, 1, 0};
const int16_t dir_to_d2y[9] = {0, 0, -1, -1, -1, 1, 1, 1, 0};
const int16_t invert_dir[9] = {1, 0, 5, 7, 6, 2, 4, 3, 8};
// The direct nation keys might not be sequencial on custom key mapping
uint8_t		key_nation[NB_NATIONS+2];


/*
 *	Internal Prototypes
 */
static void glut_idle_static_pic(void);


// Update the game and program timers
void update_timers()
{
    uint64_t register t, delta_t;
    // Find out how much time elapsed since last call
    t = mtime();
    delta_t = t - t_last;
    t_last = t;

    if (delta_t > 1000)
    {
        // We probably gone out of a suspended state from the PSP or PC
        // Don't screw up our game timers then
        delta_t = 0;
        printv("update_timers: more than one second elapsed since last time update\n");
    }

    program_time += delta_t;
    // Only update game_time when we're actually playing
    if ((game_state & GAME_STATE_ACTION) && (fade_value == 1.0f))
        game_time += delta_t;
}

#if defined(WIN32)
// Returns true if WGL extension 'extension_name' is supported
bool WGLExtensionSupported(const char *extension_name)
{
    PFNWGLGETEXTENSIONSSTRINGEXTPROC _wglGetExtensionsStringEXT = NULL;

    // Get a pointer to wglGetExtensionsStringEXT function
    _wglGetExtensionsStringEXT = (PFNWGLGETEXTENSIONSSTRINGEXTPROC)
        wglGetProcAddress("wglGetExtensionsStringEXT");

    return (strstr(_wglGetExtensionsStringEXT(), extension_name) != NULL);
}

// Set VSYNC
void set_vsync(bool enable)
{
    PFNWGLSWAPINTERVALEXTPROC       wglSwapIntervalEXT = NULL;
    PFNWGLGETSWAPINTERVALEXTPROC    wglGetSwapIntervalEXT = NULL;

    if (!WGLExtensionSupported("WGL_EXT_swap_control"))
    {
        printb("VSYNC is not supported on this platform\n");
        return;
    }

    // Extension is supported, init pointers.
    wglSwapIntervalEXT = (PFNWGLSWAPINTERVALEXTPROC)wglGetProcAddress("wglSwapIntervalEXT");

    // A swap interval of 1 tells the GPU to wait for one v-blank
    // before swapping the front and back buffers.
    wglSwapIntervalEXT(enable?1:0);
}
#endif

/*
 *	GLUT event handlers
 */
static void glut_init()
{
    // Use Glut to create a window
    glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_ALPHA);
    glutInitWindowSize(gl_width, gl_height);
    glutInitWindowPosition(0, 0);
    glutCreateWindow(APPNAME);

#if !defined(PSP)
    // Remove that pesky cursor
    glutSetCursor(GLUT_CURSOR_NONE);
#endif

    glShadeModel(GL_SMOOTH);		// set by default

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    // We'll set top left corner to be (0,0) for 2D
    glOrtho(0, PSP_SCR_WIDTH, PSP_SCR_HEIGHT, 0, -1, 1);
    glMatrixMode(GL_MODELVIEW);

    // Set our default viewport
    glViewport(0, 0, PSP_SCR_WIDTH, PSP_SCR_HEIGHT);

    // Setup transparency
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_BLEND);

    // Disable depth
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_TEXTURE_2D);

    // Clear both buffers (this is needed on PSP)
    glClear(GL_COLOR_BUFFER_BIT);
    glutSwapBuffers();
    glClear(GL_COLOR_BUFFER_BIT);
//	glutSwapBuffers();

    // Define the scissor area, outside of which we don't want to draw
//	GLCHK(glScissor(0,32,PSP_SCR_WIDTH,PSP_SCR_HEIGHT-32));
//	GLCHK(glEnable(GL_SCISSOR_TEST));

//	eglCreatePbufferSurface( null, null, null );

}


// Our display routine.
static void glut_display(void)
{
    // For onscreen FPS debug
    static uint64_t frames_duration = 0;
    static uint64_t plast = 0;
    static uint64_t nb_frames = 0;
    static uint64_t lptime = 0;
    static uint64_t ptime = 0;

    // Don't mess with our screen buffer if we're suspended and diplay()
    // is called.
    if (game_suspended)
        return;

    // Always start with a clear to black
    glClear(GL_COLOR_BUFFER_BIT);

    // Display either the current game frame or a static picture
    if ((game_state & GAME_STATE_STATIC_PIC) && (!game_menu))
    {
        if (paused)
            display_pause_screen();
        else
            display_picture();
    }
    else
    {	// In game => update room content and panel
        display_room();
        if (in_tunnel && opt_enhanced_tunnels)
            display_tunnel_area();
        display_panel();
        // Should we display the game menu?
        if ((game_menu) && (picture_state != GAME_FADE_IN) )
            display_menu_screen();
    }

    if (opt_display_fps)
    {
        // NB: Having the (blocking) glFinish() on will shoot the CPU usage
        // through the roof! Don't use in normal operations
        glFinish();
        plast = ptime;
        ptime = mtime();
        if (ptime/100 == lptime/100)
        {	// same 10th of a sec
            frames_duration += ptime-plast;
            nb_frames++;
            display_fps(0,0);	// display old value
        }
        else
        {	// new 10th of a sec => update
            lptime = ptime;
            display_fps(frames_duration, nb_frames);
            frames_duration = ptime-plast;
            nb_frames = 1;
        }
    }

#if !defined (PSP)
    // Rescale the screen
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

    glutPostRedisplay();
}


// As its names indicates
void process_motion(void)
{
    int16_t new_direction;
    int16_t exit_nr;

    if (prisoner_state & MOTION_DISALLOWED)
    {	// Only a few states will allow motion
        dx=0;
        d2y=0;
    }

    // Check if we're allowed to go where we want
    if ((dx != 0) || (d2y != 0))
    {
        exit_nr = check_footprint(dx*prisoner_speed, d2y*prisoner_speed);
        if (exit_nr != -1)
        {	// if -1, we move normally
            // in all other cases, we need to stop (even on sucessful exit)
            if (exit_nr > 0)
            {
//              printb("exit[%d], from room[%X]\n", exit_nr-1, current_room_index);
                switch_room(exit_nr-1, false);
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
}

/*
 *	restore guybrush & animation parameters after a one shot ani
 *	this function expects the guybrush index as well as the previous ani_index
 *	to be concatenated in the lower 2 bytes of the parameter
 */
void restore_params(uint32_t param)
{
    uint8_t brush, previous_index;
    // extract the guybrush index
    brush = param & 0xFF;
    // extract the previous animation index
    previous_index = (param >> 8) & 0xFF;

    if (brush >= NB_GUYBRUSHES)
    {
        perr("restore_params: brush #%d is out of range!\n", brush);
        return;
    }

    guybrush[brush].animation.index = previous_index;
    guybrush[brush].animation.framecount = 0;
    guybrush[brush].animation.end_of_ani_function = NULL;
    // we always end up in stopped state after a one shot animation
    guybrush[brush].state &= ~(STATE_MOTION|STATE_ANIMATED|STATE_KNEELING);
    // This is necessary for the tunnel opening animations
    prisoner_reset_ani = true;
}


// Act on user input (keys, joystick)
void user_input()
{
    uint16_t prop_offset;
    uint8_t  prop_id, direction, i, j;
    int16_t  exit_nr;
    uint8_t  cur_prop;

#if !defined(PSP)
    // Hey, GLUT, where's my bleeping callback on Windows?
    // NB: The routine is not called if there's no joystick
    //     and the force func does not exist on PSP
    glutForceJoystickFunc();
#endif

    // Access the menu
    if (read_key_once(KEY_ESCAPE))
    {
        game_state |= GAME_STATE_MENU;
        selected_menu = MAIN_MENU;
        selected_menu_item = FIRST_MENU_ITEM;
        picture_state = GAME_FADE_OUT_START;
        // Switch to a different idle function
        glutIdleFunc_save(glut_idle_static_pic);
        config_save = false;
    }

    // Handle the pausing of the game
    if (read_key_once(KEY_PAUSE))
    {
        game_state |= GAME_STATE_PAUSED;
        picture_state = GAME_FADE_OUT_START;
        // Switch to a different idle function
        glutIdleFunc_save(glut_idle_static_pic);
    }

    // Prisoner selection: direct keys or left/right cycle keys
    for (i=0; i<(NB_NATIONS+2); i++)
        if (read_key_once(key_nation[i]))
        {
            // Unpause the game if required
            if (game_state & GAME_STATE_PAUSED)
                game_state ^= GAME_STATE_PAUSED|GAME_STATE_ACTION;

            if (i<NB_NATIONS)
                switch_nation(i);
            else
                // we want to have +/-1, without going negative, and we
                // know that we are either at NB_NATIONS or NB_NATIONS+1
                // so the formula writes itself as:
                switch_nation((current_nation+(2*i)-1) % NB_NATIONS);
            break;
        }

#if defined (CHEATMODE_ENABLED)
    // Check cheat sequences
    if ((!opt_original_mode) && read_cheat_key_once(last_key_used))
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
                    case CHEAT_KONAMI:
                        set_status_message("     NICE TRY... BUT NO     ", 3, CHEAT_MESSAGE_TIMEOUT);
                        break;
                    case CHEAT_PROP_BONANZA:
                        for (j=1; j<NB_PROPS-1; j++)
                            props[current_nation][j] += 10;
                        set_status_message("    ENJOY YOUR PROPS ;)     ", 3, CHEAT_MESSAGE_TIMEOUT);
                        break;
                    case NO_CAKE_FOR_YOU:
                        set_status_message("  NO: >YOU< ARE THE LIE!!!  ", 3, CHEAT_MESSAGE_TIMEOUT);
                        break;
                    case NAMIKO:
                    case THRILLERDANCE:
                        if (!opt_thrillerdance)
                            set_status_message("  'COZ THIS IS THRILLER!... ", 3, CHEAT_MESSAGE_TIMEOUT);
                        opt_thrillerdance = !opt_thrillerdance;
                        thriller_toggle();
                        break;
                    case CHEAT_KEYMASTER:
                        opt_keymaster = !opt_keymaster;
                        break;
                    case CHEAT_NOGUARDS:
                        opt_no_guards = !opt_no_guards;
                        break;
                    case CHEAT_SAFECRACKER:
                        opt_play_as_the_safe[current_nation] = !opt_play_as_the_safe[current_nation];
                        break;
                    default:
                        break;
                    }
                    play_cluck();
                    printv("CHEAT[%d] activated!\n", i);
                    cheat_sequence[i].cur_pos = 0;
                }
            }
            else
                // Reset the cheat sequence index
                cheat_sequence[i].cur_pos = 0;
        }
    }
#endif

    if (read_key_once('#'))
        opt_display_fps = !opt_display_fps;

#if defined(DEBUG_ENABLED)
// Non official debug keys
#define KEY_DEBUG_PRINT_POS		'p'
#define KEY_OSD					']'
#define KEY_NEXT_SID			'+'
#define KEY_PREV_SID			'-'
    // Display our current position
    if (read_key_once(KEY_DEBUG_PRINT_POS))
    {
        printf("game_time = %lld, program_time = %lld, state = 0x%02X\n",
            game_time, program_time, prisoner_state);
        printf("room = 0x%03X, (px, p2y) = (%d, %d), rem_bitmask = 0x%08X\n",
            current_room_index, prisoner_x, prisoner_2y, rem_bitmask);
    }

    if (read_key_once(KEY_OSD))
        opt_onscreen_debug = !opt_onscreen_debug;

    if (opt_sid >= 0)
    {
        if (read_key_once(KEY_NEXT_SID))
            opt_sid++;
        if (read_key_once(KEY_PREV_SID))
            opt_sid--;
    }
#endif

    // Above are all the keys allowed if the prisoner has not already escaped or died, thus...
    if (has_escaped || is_dead)
        return;

    // Walk/Run toggle
    if (read_key_once(KEY_TOGGLE_WALK_RUN) && (!in_tunnel) &&
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
        prisoner_ani.framecount = 0;
    }

    // Toggle stooge
    if (read_key_once(KEY_STOOGE))
        prisoner_state ^= STATE_STOOGING;

    // Even if we're idle, we might be trying to open a tunnel exit, or use a prop
    if (read_key_once(KEY_ACTION))
    {
        // We need to set this variable as we might check if fire is pressed
        // in various subroutines below
        is_fire_pressed = true;

        // Handle tunnel I/O through a check_footprint(0,0) call immediately followed
        // by a check_tunnel_io
        check_footprint(0,0);
        exit_nr = check_tunnel_io();

        if (exit_nr > 0)
        {	// We just went through a tunnel exit
            // => Toggle tunneling state
            prisoner_state ^= STATE_TUNNELING;
            switch_room(exit_nr-1, true);
        }
        else if (exit_nr < 0)
        {	// We used a prop to open a tunnel => cue in animation
            prisoner_state |= STATE_ANIMATED;
            // enqueue our 2 u8 parameters
            prisoner_ani.end_of_ani_parameter = (current_nation & 0xFF) |
                ((prisoner_ani.index << 8) & 0xFF00);
            prisoner_ani.index = prisoner_as_guard?GUARD_KNEEL_ANI:KNEEL_ANI;
            // Make sure we go through frame 0
            prisoner_ani.framecount = 0;
            prisoner_ani.end_of_ani_function = restore_params;
        }
        else
        {	// Are we trying to use some non tunnel I/O related prop?
            switch(cur_prop = selected_prop[current_nation])
            {
            case ITEM_GUARDS_UNIFORM:
            case ITEM_PRISONERS_UNIFORM:
                if ( (!in_tunnel) &&
                     ( ((cur_prop == ITEM_GUARDS_UNIFORM) && (!prisoner_as_guard)) ||
                       ((cur_prop == ITEM_PRISONERS_UNIFORM) && (prisoner_as_guard) ) ) )
                {
                    prisoner_as_guard = (cur_prop == ITEM_GUARDS_UNIFORM);
                    consume_prop();
                    show_prop_count();
                    // Set the animation for changing into guard's clothes
                    prisoner_state |= STATE_ANIMATED+STATE_KNEELING;
                    prisoner_ani.end_of_ani_parameter = (current_nation & 0xFF) |
                        ((prisoner_ani.index << 8) & 0xFF00);
                    prisoner_ani.index = (cur_prop == ITEM_GUARDS_UNIFORM)?
                        INTO_GUARDS_UNI_ANI:INTO_PRISONERS_UNI_ANI;
                    prisoner_ani.framecount = 0;
                    prisoner_ani.end_of_ani_function = restore_params;
                    // The original game leaves us turned away at the end of the animation
                    prisoner_dir = 2;
                }
                break;
            case ITEM_STONE:
                consume_prop();
                show_prop_count();
                p_event[current_nation].thrown_stone = true;
                break;
            default:
                break;
            }
        }
    }

    if ( (prisoner_state & STATE_SLEEPING) && read_key_once(KEY_SLEEP) )
    {	// Out of bed
        prisoner_state &= ~STATE_SLEEPING;
        prisoner_x = readword(fbuffer[LOADER],INITIAL_POSITION_BASE+10*current_nation+2)-16;
        prisoner_2y = 2*readword(fbuffer[LOADER],INITIAL_POSITION_BASE+10*current_nation)-8;
        prisoner_reset_ani = true;
    }
    else if (!(prisoner_state & MOTION_DISALLOWED))
    {	// The following keys are only handled if we are in a premissible state

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
            prisoner_state |= STATE_ANIMATED|STATE_KNEELING;
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
                            writeword(fbuffer[OBJECTS],prop_offset+4, prisoner_x + 16);
                            writeword(fbuffer[OBJECTS],prop_offset+2, prisoner_2y/2 + 4);
                            // 3. object id
                            writeword(fbuffer[OBJECTS],prop_offset+6, over_prop_id);
                            found = true;
                            break;
                        }
                    }
                    if (!found)		// Somebody's cheating!
                        perr("Could not find any free prop variable => discarding prop.\n");

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
            return;
        }

        // Sleep
        if ( (read_key_once(KEY_SLEEP)) &&
             (current_room_index == readword(fbuffer[LOADER],INITIAL_POSITION_BASE+10*current_nation+4)) &&
             (!(prisoner_state & STATE_SLEEPING)) )
        {	// Go to bed
            prisoner_x = readword(fbuffer[LOADER],INITIAL_POSITION_BASE+10*current_nation+8) - 9;
            prisoner_2y = 2*readword(fbuffer[LOADER],INITIAL_POSITION_BASE+10*current_nation+6) - 28;
            prisoner_state |= STATE_SLEEPING;
            prisoner_ani.index = SLEEP_ANI;
            prisoner_ani.framecount = 0;
            prisoner_ani.end_of_ani_function = NULL;
        }

        if (read_key_once(KEY_STOOGE))
            play_sfx(0);
    }

    //
    // Finally, we handle motion
    //
    if (key_down[KEY_DIRECTION_LEFT])
        dx = -1;
    else if (key_down[KEY_DIRECTION_RIGHT])
        dx = +1;

    if (key_down[KEY_DIRECTION_UP])
        d2y = -1;
    else if (key_down[KEY_DIRECTION_DOWN])
        d2y = +1;

    // Joystick motion overrides keys
    if (jdx)
        dx = jdx;
    if (jd2y)
        d2y = jd2y;
}


// This is the main game loop
static void glut_idle_game(void)
{
    uint8_t i;

    // Reset the motion
    dx = 0;
    d2y = 0;

    // They say it's good practice to poke the sleeping GL monster a bit
    glFlush();

    // We'll need the current time value for a bunch of stuff
    update_timers();

    // Read & process user input
    user_input();

    // No need to push it further if paused
    if (game_state & GAME_STATE_PAUSED)
    {
        glutPostRedisplay();
        // We should be able to sleep for a while
        msleep(PAUSE_DELAY);
        return;
    }

    // Handle timed events (including animations)
    if ((game_time - last_atime) > ANIMATION_INTERVAL)
    {
//		last_atime += ANIMATION_INTERVAL;	// closer to ideal rate but leads
                                            // to catchup effect when moving window
        last_atime = game_time;

        for (i = 0; i < nb_animations; i++)
            animations[i].framecount++;
        for (i = 0; i < NB_GUYBRUSHES; i++)
            guy(i).animation.framecount++;

        // Panel clock minute tick?
        if ((game_time - last_ctime) > TIME_MARKER)
        {
//			last_ctime += TIME_MARKER;
            last_ctime = game_time;
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
                        if (!(guy(i).state & STATE_SLEEPING) && (!p_event[i].killed))
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
        glutPostRedisplay();
    }

    // Don't hammer down the CPU
    else if (game_time - last_ptime - REPOSITION_INTERVAL > QUANTUM_OF_SOLACE)
        msleep(QUANTUM_OF_SOLACE);
}

// Menu navigation
#define TOGGLE_OPTION(section, option) {                                        \
    bool opt = !iniparser_getboolean(config, #section ":" #option, false);      \
    iniparser_set(config, #section ":" #option, opt?"1":"0");                   \
    config_save=true; }
#define ROTATE_OPTION(section, option, nb_values) {                             \
    char val_str[2] = { '0', 0 };                                               \
    int val = iniparser_getint(config, #section ":" #option, 2);                \
    val= (val+(key_down[SPECIAL_KEY_LEFT]?(nb_values)-1:1))%(nb_values);        \
    val_str[0] = '0'+val; iniparser_set(config, #section ":" #option, val_str); \
    config_save = true; }

void process_menu()
{
    bool cancel_selected = read_key_once(KEY_CANCEL);
    char save_name[] = "colditz_00.sav";
#if !defined(PSP)
    static int old_w=2*PSP_SCR_WIDTH, old_h=2*PSP_SCR_HEIGHT;
#endif

    // Menu navigation (up or down)
    if (read_key_once(SPECIAL_KEY_UP) || read_key_once(KEY_DIRECTION_UP))
    {
        do
            selected_menu_item = (selected_menu_item+NB_MENU_ITEMS-1)%NB_MENU_ITEMS;
        while (!enabled_menus[selected_menu][selected_menu_item]);
    }
    if (read_key_once(SPECIAL_KEY_DOWN) || read_key_once(KEY_DIRECTION_DOWN))
    {
        do
            selected_menu_item = (selected_menu_item+1)%NB_MENU_ITEMS;
        while (!enabled_menus[selected_menu][selected_menu_item]);
    }

    if (read_key_once(KEY_ACTION) || read_key_once(0x0D) || read_key_once(' ') ||
        read_key_once(SPECIAL_KEY_LEFT) || read_key_once(SPECIAL_KEY_RIGHT) || cancel_selected)
    {
        switch (selected_menu)
        {
        case MAIN_MENU:
            if (cancel_selected)
                selected_menu_item = MENU_RETURN;
            switch(selected_menu_item)
            {
            case MENU_RETURN:
                picture_state = GAME_FADE_IN_START;
                break;
            case MENU_RESTART:
                newgame_init();
                break;
            case MENU_LOAD:
            case MENU_SAVE:
                create_savegame_list();
                if (selected_menu_item == MENU_LOAD)
                    selected_menu = LOAD_MENU;
                else
                    selected_menu = SAVE_MENU;
                selected_menu_item = FIRST_MENU_ITEM;
                break;
            case MENU_OPTIONS:
                selected_menu = OPTIONS_MENU;
                selected_menu_item = FIRST_MENU_ITEM;
                break;
            case MENU_EXIT:
                if ((config_save) && (!write_conf(confname)))
                    perr("Error rewritting %s.\n", confname);
                LEAVE;
                break;
            default:
                break;
            }
            break;
        case OPTIONS_MENU:
            if (cancel_selected)
                selected_menu_item = MENU_BACK_TO_MAIN;
            switch(selected_menu_item)
            {
            case MENU_BACK_TO_MAIN:
                if ((config_save) && (!write_conf(confname)))
                    perr("Error rewritting %s.\n", confname);
                selected_menu = MAIN_MENU;
                selected_menu_item = FIRST_MENU_ITEM;
                break;
            case MENU_ENHANCEMENTS:
                TOGGLE_OPTION(options, enhanced_guards);
                TOGGLE_OPTION(options, enhanced_tunnels);
                break;
// The following are platform specific
#if defined(WIN32)
            case MENU_VSYNC:
                TOGGLE_OPTION(options, vsync);
                set_vsync(opt_vsync);
                break;
#endif
#if !defined(PSP)
            case MENU_SMOOTHING:
                ROTATE_OPTION(options, gl_smoothing, 2+(opt_has_shaders?NB_SHADERS:0));
                break;
            case MENU_FULLSCREEN:
                TOGGLE_OPTION(options, fullscreen);
                if (opt_fullscreen)
                {
                    old_w = gl_width;
                    old_h = gl_height;
                    glutFullScreen();
                }
                else
                    glutReshapeWindow(old_w, old_h);
                break;
#endif
            case MENU_PICTURE_CORNERS:
                TOGGLE_OPTION(options, picture_corners);
                break;
            case MENU_ORIGINAL_MODE:
                TOGGLE_OPTION(options, original_mode);
                if (opt_original_mode)
                {
                    SET_CONFIG_BOOLEAN(options, enhanced_guards, false);
                    SET_CONFIG_BOOLEAN(options, enhanced_tunnels, false);
                    enabled_menus[OPTIONS_MENU][MENU_ENHANCEMENTS] = 0;
                    enabled_menus[MAIN_MENU][MENU_SAVE] = 0;
                    enabled_menus[MAIN_MENU][MENU_LOAD] = 0;
                }
                else
                {
                    enabled_menus[OPTIONS_MENU][MENU_ENHANCEMENTS] = 1;
                    enabled_menus[MAIN_MENU][MENU_SAVE] = 1;
                    enabled_menus[MAIN_MENU][MENU_LOAD] = 1;
                    play_cluck();
                }
                break;
            default:
                break;
            }
            break;
        case SAVE_MENU:
        case LOAD_MENU:
            if (cancel_selected)
                selected_menu_item = MENU_BACK_TO_MAIN;
            if (selected_menu_item == MENU_BACK_TO_MAIN)
            {
                selected_menu = MAIN_MENU;
                selected_menu_item = FIRST_MENU_ITEM;
            }
            else
            {
                sprintf(save_name, "colditz_%02d.sav", selected_menu_item-3);
                if (selected_menu == LOAD_MENU)
                {
                    if (!load_game(save_name))
                    {
                        menus[LOAD_MENU][selected_menu_item] = "<LOAD FAILED!>";
                        // If we couldn't load, better reset the whole lot
                        newgame_init();
                    }
                }
                else
                {
                    if(!save_game(save_name))
                    {
                        menus[SAVE_MENU][selected_menu_item] = "<SAVE FAILED!>";
                        // Try to delete the failed file
                        remove(save_name);
                    }
                    else	// Update the saved games display
                        create_savegame_list();
                }
            }
            break;
        default:
            break;
        }
    }
}


// We'll use a different idle function for static picture
static void glut_idle_static_pic(void)
{
    float min_fade;

    min_fade = (game_menu)?MIN_MENU_FADE:0.0f;
    // As usual, we'll need the current time value for a bunch of stuff
    update_timers();

#if !defined(PSP)
    glutForceJoystickFunc();
#endif

    if (game_menu)
        process_menu();

    if ((intro) && read_key_once(last_key_used))
    {	// Exit intro => start new game
        mod_release();
        newgame_init();
        picture_state = PICTURE_FADE_OUT_START;
        game_state = GAME_STATE_STATIC_PIC;
        last_key_used = 0;
    }
    else if (game_over && (picture_state > GAME_FADE_OUT) && (picture_state < PICTURE_FADE_OUT_START)
        && read_key_once(last_key_used))
    {	// Exit game over/game won => Intro
        mod_release();
        picture_state = PICTURE_FADE_OUT_START;
        game_state |= GAME_STATE_PICTURE_LOOP;
        last_key_used = 0;
    }

    if ( (picture_state%2) && (picture_state != PICTURE_WAIT) )
    {	// All the non "START" states (odd) need to slide the fade value, except for "PICTURE_WAIT"
        if (transition_start == 0)
            perr("glut_idle_static_pic: should never see me!\n");
        fade_value = (float)(program_time-transition_start)/TRANSITION_DURATION;
        if (fade_out)
            fade_value = 1.0f - fade_value;
        else
            fade_value += min_fade;
        if ((fade_value >= 1.0f) || (fade_value <= min_fade))
        {
            fade_value = (fade_value <= min_fade)?min_fade:1.0f;
            picture_state++;
            transition_start = 0;
        }
    }

    // Act on the various game states
    switch(picture_state)
    {
    case GAME_FADE_OUT_START:
        game_state &= ~GAME_STATE_ACTION;
        transition_start = program_time;
        fade_out = true;
        picture_state++;
        break;
    case GAME_FADE_OUT:
        break;
    case PICTURE_FADE_IN_START:
        game_state |= GAME_STATE_STATIC_PIC;
        if (paused) {
#if defined(PSP)
            // For some reason, the graphics on PSP are all garbled the first time
            // we create the pause screen.
            // And since I can't be bothered to try to figure the intricacies of
            // GLUT/OpenGL on PSP any longer, just apply an obvious workaround.
            static bool first_time = true;
            if (first_time)
            {
                first_time = false;
                create_pause_screen();
            }
#endif
            // We use the picture fade in to create the pause screen if paused
            create_pause_screen();
        }
        else if (game_menu)
            picture_state++;	// Skip picture fade
        else
        {
            // Load a new MOD if needed...
            if (!is_mod_playing())
            {
                if (intro)
                {
                    if (mod_init(mod_name[MOD_LOADTUNE]))
                        mod_play();
                    else
                        printf("Failed to load Intro tune\n");
                }
                else if (game_over)
                {
                    if (mod_init(mod_name[MOD_GAMEOVER]))
                        mod_play();
                    else
                        printf("Failed to load Game Over tune\n");
                }
                else if (game_won)
                {
                    if (mod_init(mod_name[MOD_WHENWIN]))
                        mod_play();
                    else
                        printf("Failed to load winning tune\n");
                }
            }
        }
        transition_start = program_time;
        fade_out = false;
        picture_state++;
        break;
    case PICTURE_FADE_IN:
        break;
    case PICTURE_WAIT_START:
        if ((!paused) && (!game_menu))
        {
            // Set the timeout start for pictures
            picture_t = program_time;
            // We might want to call a function to update the prisoner's status at this stage
            if (static_screen_func != NULL)
            {
                static_screen_func(static_screen_param);
                static_screen_func = NULL;
            }
            // Add the picture loop flag here on game won
            if (game_state & GAME_STATE_GAME_WON)
                game_state |= GAME_STATE_PICTURE_LOOP;
        }
        picture_state++;
        break;
    case PICTURE_WAIT:
        if (game_menu)
        {
            if (read_key_once(KEY_ESCAPE))
            {
                if ((config_save) && (!write_conf(confname)))
                    perr("Error rewritting %s.\n", confname);
                picture_state = GAME_FADE_IN_START;
            }
        }
        else if (read_key_once(last_key_used) ||
            ( (!game_over) && (!paused) && (program_time-picture_t > PICTURE_TIMEOUT)))
        {	// Any key or timeout
            picture_state++;
            last_key_used = 0;
        }
        break;
    case PICTURE_FADE_OUT_START:
        transition_start = program_time;
        fade_out = true;
        picture_state++;
        break;
    case PICTURE_FADE_OUT:
        break;
    case GAME_FADE_IN_START:
        transition_start = program_time;
        fade_out = false;
        picture_state++;
        if (paused)
            switch_nation(current_nation);
        if (game_state & GAME_STATE_PICTURE_LOOP)
        {
            if (intro)
            {	// Cycle through Intro screens
                current_picture++;
                if (current_picture > INTRO_SCREEN_END)
                    current_picture = INTRO_SCREEN_START;
                // NB: static screen will reset picture_state to the right one
                static_screen(current_picture, NULL, 0);
            }
            else if (game_over)
            {
                game_state = GAME_STATE_INTRO | GAME_STATE_PICTURE_LOOP | GAME_STATE_STATIC_PIC;
                static_screen(INTRO_SCREEN_START, NULL, 0);
            }
            else if (game_won)
            {	// Switch to second & last GAME_WON picture
                // We add the GAME_OVER flag so that we can reuse the GAME OVER exit code
                game_state |= GAME_STATE_GAME_OVER;
                static_screen(PRISONER_FREE_ALL, NULL, 0);
            }
            else
                perr("Missing static_screen() call in static pic loop!\n");
        }
        else if (current_picture == REQUIRE_PAPERS)
        {	// We should probably have used a picture loop, but this function's a mess anyway ;)
            static_screen(TO_SOLITARY, go_to_jail, current_nation);
            picture_state = PICTURE_FADE_IN_START;
        }
        else
            game_state &= ~GAME_STATE_STATIC_PIC;
        break;
    case GAME_FADE_IN:
        break;
    case PICTURE_EXIT:
        game_state |= GAME_STATE_ACTION;
        if (paused)
            game_state &= ~GAME_STATE_PAUSED;
        if (game_menu)
            game_state &= ~GAME_STATE_MENU;
        // Set glut_idle to our main game loop
        glutIdleFunc_save(glut_idle_game);
        break;
    default:
        perr("glut_idle_static_pic(): should never be here!\n");
        break;
    }

    // Don't forget to display the image
    glutPostRedisplay();

    // We should be able to sleep for a while
    // Except if we're in a picture transition on the PSP, as it's too slow otherwise
#if defined(PSP)
    if ((fade_value == 0.0) || (fade_value == 1.0))
#endif
        msleep(PAUSE_DELAY);
}


// And another one when the game is in suspended state (e.g. debug output)
// that one can't be static as it needs to be defined externally
void glut_idle_suspended(void)
{

#if !defined(PSP)
    glutForceJoystickFunc();
#endif

    // If we didn't get out, wait for any key
    if ((!game_suspended) || read_key_once(last_key_used))
    {
        t_last = mtime();
        glutIdleFunc(restore_idle);
        game_suspended = false;
    }

    msleep(PAUSE_DELAY);
}

/*
 *	This function handles the displaying of static screens
 *	The last 2 parameters are for a function callback while we are in
 *	the middle of a static picture
 */
void static_screen(uint8_t picture_id, void (*func)(uint32_t), uint32_t param)
{
    if (game_suspended)
        return;

    // We need to store the current picture for idle_static_pic()
    current_picture = picture_id;

    // This function call will be initiated in idle_static_pic()
    static_screen_func = func;
    static_screen_param = param;

    if (!load_texture(&texture[picture_id]))
    {	// There was a problem loading the file
        if (static_screen_func != NULL)
        {	// If we don't execute the function, we might get stuck
            static_screen_func(static_screen_param);
            static_screen_func = NULL;
        }
        if (intro)
            ERR_EXIT;
        return;
    }

    // start with the following picture state
    if (game_state & GAME_STATE_PICTURE_LOOP)
        picture_state = PICTURE_FADE_IN_START;
    else
    {
        picture_state = GAME_FADE_OUT_START;
        // Switch to a different idle function
        glutIdleFunc_save(glut_idle_static_pic);
    }

    // Reset the "any" key
    last_key_used = 0;

}

#if defined(XBOX360_CONTROLLER_SUPPORT)
#define ASSIGN_X360(key, button)                                    \
    if ((buttonMask & button) && !(last_buttonMask & button))       \
    {                                                               \
        key_down[key] = true;                                       \
        last_key_used = key;                                        \
    }                                                               \
    else if (!(buttonMask & button) && (last_buttonMask & button))  \
    {                                                               \
        key_down[key] = false;                                      \
        key_readonce[key] = false;                                  \
        key_cheat_readonce[key] = false;                            \
    }

static void parse_xbox360_controller(unsigned int buttonMask)
{
    // Keep a copy of last mask to track button up
    static unsigned int last_buttonMask = 0;

    if (buttonMask != last_buttonMask)
    {
        ASSIGN_X360(KEY_ACTION, XBOX360_CONTROLLER_BUTTON_A);
        ASSIGN_X360(KEY_CANCEL, XBOX360_CONTROLLER_BUTTON_B);
        ASSIGN_X360(KEY_SLEEP, XBOX360_CONTROLLER_BUTTON_X);
        ASSIGN_X360(KEY_INVENTORY_PICKUP, XBOX360_CONTROLLER_DPAD_UP);
        ASSIGN_X360(KEY_INVENTORY_DROP, XBOX360_CONTROLLER_DPAD_DOWN);
        ASSIGN_X360(KEY_INVENTORY_LEFT, XBOX360_CONTROLLER_DPAD_LEFT);
        ASSIGN_X360(KEY_INVENTORY_RIGHT, XBOX360_CONTROLLER_DPAD_RIGHT);
        ASSIGN_X360(KEY_TOGGLE_WALK_RUN, XBOX360_CONTROLLER_BUTTON_RT);
        ASSIGN_X360(KEY_STOOGE, XBOX360_CONTROLLER_BUTTON_LT);
        ASSIGN_X360(KEY_PRISONER_LEFT, XBOX360_CONTROLLER_BUTTON_LB);
        ASSIGN_X360(KEY_PRISONER_RIGHT, XBOX360_CONTROLLER_BUTTON_RB);
        ASSIGN_X360(KEY_ESCAPE, XBOX360_CONTROLLER_BUTTON_START);
        ASSIGN_X360(KEY_PAUSE, XBOX360_CONTROLLER_BUTTON_BACK);
        last_buttonMask = buttonMask;
    }
}
#endif

// Input handling
static void glut_joystick(unsigned int buttonMask, int x, int y, int z)
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

#if defined(XBOX360_CONTROLLER_SUPPORT)
    // Remap the analog trigger input to virtual buttons
    if (z>JOY_DEADZONE)
        buttonMask |= XBOX360_CONTROLLER_BUTTON_LT;
    else if (z<-JOY_DEADZONE)
        buttonMask |= XBOX360_CONTROLLER_BUTTON_RT;
    parse_xbox360_controller(buttonMask);
#endif
#if defined(PSP)
    // The PSP has the bad habit of powering the LCD down EVEN if the analog stick is in use
    if ((jdx != 0) || (jd2y != 0))
        scePowerTick(0);
#endif
}

// These macro handle the readout of the Ctrl, Alt, Shift modifiers
// Only relevant for platforms with actual keyboard
#if !defined(PSP)
#define KEY_MOD(mod)													\
    key_down[SPECIAL_KEY_##mod] = (glut_mod & GLUT_ACTIVE_##mod) != 0
#define SET_MODS { glut_mod = glutGetModifiers();						\
    KEY_MOD(SHIFT); KEY_MOD(CTRL); KEY_MOD(ALT); }
#else
#define SET_MODS
#endif

static void glut_keyboard(uint8_t key, int x, int y)
{
    key_down[key] = true;
    last_key_used = key;
    SET_MODS;
}

static void glut_keyboard_up(uint8_t key, int x, int y)
{
    key_down[key] = false;
    key_readonce[key] = false;
#if defined (CHEATMODE_ENABLED)
    key_cheat_readonce[key] = false;
#endif
    SET_MODS;
}

static void glut_special_keys(int key, int x, int y)
{
    int converted_key;
    converted_key = (key < GLUT_KEY_LEFT)?key-GLUT_KEY_F1+SPECIAL_KEY_OFFSET1:key-GLUT_KEY_LEFT+SPECIAL_KEY_OFFSET2;
    key_down[converted_key] = true;
    last_key_used = converted_key;
    SET_MODS;
}

static void glut_special_keys_up(int key, int x, int y)
{
    int converted_key;
    converted_key = (key < GLUT_KEY_LEFT)?key-GLUT_KEY_F1+SPECIAL_KEY_OFFSET1:key-GLUT_KEY_LEFT+SPECIAL_KEY_OFFSET2;
    key_down[converted_key] = false;
    key_readonce[converted_key] = false;
#if defined (CHEATMODE_ENABLED)
    key_cheat_readonce[converted_key] = false;
#endif
    SET_MODS;
}

static void glut_mouse_buttons(int button, int state, int x, int y)
{
    int converted_key;
    converted_key = SPECIAL_MOUSE_BUTTON_BASE + button - GLUT_LEFT_BUTTON;
    if (state == GLUT_DOWN)
    {
        key_down[converted_key] = true;
        last_key_used = converted_key;
    }
    else
    {
        key_down[converted_key] = false;
        key_readonce[converted_key] = false;
#if defined (CHEATMODE_ENABLED)
        key_cheat_readonce[converted_key] = false;
#endif
    }
    SET_MODS;
}


/* Here we go! */
#if (defined(WIN32) && !defined(_DEBUG))
// If we don't use WinMain, the latest DX will create a console
int __stdcall WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
// argc & argv are provided by msvcrt.dll, as __argc & __argv for Windows apps
#define argc __argc
#define argv __argv
#else
int main (int argc, char *argv[])
#endif
{
#if defined(DEBUG_ENABLED)
    const char* getopt_str = "hbnvs:";
    const char* usage = "[-h][-b][-n][-v][-s <sprite_id>]";
#else
    const char* getopt_str = "hnv";
    const char* usage = "[-h][-n][-v]";
#endif
    // Flags
    int opt_error = 0;	// getopt
    // General purpose
    uint32_t i;

#if defined(PSP)
    setup_callbacks();
#endif

    // A little cleanup
    fflush(stdin);
    for (i=0; i<NB_FILES; i++)
        fbuffer[i] = NULL;

    // Process commandline options (works for PSP too with psplink)
    while ((i = getopt (argc, argv, getopt_str)) != -1)
        switch (i)
    {
        case 'n':		// Skip intro
            opt_skip_intro = true; 
            break;
        case 'v':		// Print verbose messages
            opt_verbose = true;
            break;
#if defined(DEBUG_ENABLED)
        case 'b':		// Debug mode
            opt_debug = true;
            break;
        case 's':		// debug SID (sprite) test
            IGNORE_RETVAL(sscanf(optarg, ("%x"), &opt_sid));
            break;
#endif
        case 'h':		// Half size
            opt_halfsize = true;
            break;
        default:		// Unknown option
            opt_error++;
            break;
    }
#if !defined(PSP)
    printf("\nColditz Escape %s\n", VERSION);
    printf("by Aperture Software - 2009-2017\n\n");
#endif
    if ( ((argc-optind) > 3) || opt_error)
    {
        printf("usage: %s %s\n\n", argv[0], usage);
        ERR_EXIT;
    }

#if defined(PSP)
    gl_width = PSP_SCR_WIDTH;
    gl_height = PSP_SCR_HEIGHT;
#else
    gl_width = (opt_halfsize?1:2)*PSP_SCR_WIDTH;
    gl_height = (opt_halfsize?1:2)*PSP_SCR_HEIGHT;
#endif

#if defined(__linux__)
    // This is supposed to force VSYNC for nVidia cards on Linux
    putenv((char *) "__GL_SYNC_TO_VBLANK=1");
#endif

    // Well, we're supposed to call that blurb
    glutInit(&argc, argv);

    // Need to have a working GL before we proceed. This is our own init() function
    glut_init();

//	remove(confname);
    if (!read_conf(confname))
    {	// config.ini not found => try to create one
        printb("'%s' not found. Creating a new one...\n", confname);
        fflush(stdout);
        if (!set_conf_defaults())
        {
            perr("Could not set configuration defaults\n");
        }
        else if (!write_conf(confname))
        {
            perr("Could not create '%s'\n", confname);
        }
    }
    if (opt_original_mode)
    {
        SET_CONFIG_BOOLEAN(options, enhanced_guards, false);
        enabled_menus[OPTIONS_MENU][MENU_ENHANCEMENTS] = 0;
        enabled_menus[MAIN_MENU][MENU_SAVE] = 0;
        enabled_menus[MAIN_MENU][MENU_LOAD] = 0;
    }
#if !defined(PSP)
    opt_has_shaders = init_shaders();
    if (!opt_has_shaders)
        SET_CONFIG_BOOLEAN(options, gl_smoothing, 0);
#if defined(WIN32)
    set_vsync(opt_vsync);
#endif
#endif

#if !defined(PSP)
    if (opt_fullscreen)
        glutFullScreen();
#endif

    // Now that we have our config set, we can initialize some controls values
    key_nation[0] = KEY_PRISONER_BRITISH;
    key_nation[1] = KEY_PRISONER_FRENCH;
    key_nation[2] = KEY_PRISONER_AMERICAN;
    key_nation[3] = KEY_PRISONER_POLISH;
    key_nation[4] = KEY_PRISONER_LEFT;
    key_nation[5] = KEY_PRISONER_RIGHT;

    // Load the data. If it's the first time the game is ran, we might have
    // to uncompress LOADTUNE.MUS (PowerPack) and SKR_COLD (custom compression)
    load_all_files();
#if defined(ANTI_TAMPERING_ENABLED)
    for (i=0; i<NB_FILES; i++)
        if (!integrity_check(i))
        {
            perr("Integrity check failure on file '%s'\n", fname[i]);
            ERR_EXIT;
        }
#endif
    depack_loadtune();

    // Some of the files need patching (this was done too in the original game!)
    fix_files(false);

    set_textures();
    set_sfxs();

    // Set global variables
    t_last = mtime();
    srand((unsigned int)t_last);
    program_time = 0;
    game_time = 0;

    // We might want some sound
    if (!audio_init())
        perr("Could not Initialize audio\n");

    // We're going to convert the cells array, from 2 pixels per byte (paletted)
    // to on RGB(A) word per pixel
    rgbCells = (uint8_t*) aligned_malloc(fsize[CELLS]*2*RGBA_SIZE, 16);
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
    sprites_to_wGRAB();	// Must be called after init sprite

    if (opt_skip_intro)
    {
        fade_value = 1.0f;
        glColor3f(fade_value, fade_value, fade_value);
        game_state = GAME_STATE_ACTION;
        glutIdleFunc_save(glut_idle_game);
        newgame_init();
    }
    else
    {
    // We'll start with the Intro state
    // DO NOT call static_screen() here, as it messes up with any earlier psp/printf
        game_state = GAME_STATE_INTRO | GAME_STATE_STATIC_PIC | GAME_STATE_PICTURE_LOOP;
        current_picture = INTRO_SCREEN_START;
        picture_state = PICTURE_FADE_IN_START;
        if (!load_texture(&texture[INTRO_SCREEN_START]))
        {
            perr("Could not load INTRO screen\n");
            ERR_EXIT;
        }
        glutIdleFunc(glut_idle_static_pic);
        last_key_used = 0;
    }

    // Now we can proceed with setting up our display
    glutDisplayFunc(glut_display);
    glutReshapeFunc(glut_reshape);

    glutKeyboardFunc(glut_keyboard);
    glutKeyboardUpFunc(glut_keyboard_up);
    glutSpecialFunc(glut_special_keys);
    glutSpecialUpFunc(glut_special_keys_up);
    glutMouseFunc(glut_mouse_buttons);

    glutJoystickFunc(glut_joystick,30);

    glutMainLoop();

    return 0;
}
