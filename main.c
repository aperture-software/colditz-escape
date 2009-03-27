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
int debug_flag			= 0;
int	opt_verbose			= 0;
int	opt_debug			= 0;
int opt_ghost			= 0;

// File stuff
FILE* fd				= NULL;
char* fname[NB_FILES]	= FNAMES;			// file name(s)
u32   fsize[NB_FILES]	= FSIZES;
u8*   fbuffer[NB_FILES];
u8*   mbuffer			= NULL;
u8*	  rgbCells			= NULL;
// GL Stuff
int	gl_off_x = 0, gl_off_y  = 0;
// OpenGL window size
int	gl_width, gl_height;
int prisoner_x = 900, prisoner_2y = 600;
//int prisoner_x = 32, prisoner_2y = 32;
int last_p_x = 0, last_p_y = 0;
int dx = 0, d2y = 0;
int jdx, jd2y;
u8	p_sid_base	 = 0x00;
// prisoner_run = 0x1F
// german_walk  = 0x37 (with rifle)
// german_run   = 0x57 (with rifle)
u8  prisoner_sid = 0x07; // 0x07;
float origin_x = 0, origin_y = -24.0;

bool key_down[256];

u16  current_room_index = ROOM_OUTSIDE;
u16  nb_rooms, nb_cells, nb_sprites, nb_objects;
u8   palette_index = 2;
s_sprite*	sprite;
s_overlay*	overlay;
u8   overlay_index;
u8   bPalette[3][16];
// remapped Amiga Palette
u16  aPalette[16];
u32  rem_bitmask = 0x0000003E;


// offsets to sprites according to joystick direction (y,x)
int directions[3][3] = { {6,7,8}, {5,0,1}, {4,3,2} };
int last_direction = 3;
int framecount = 0;

/* TO_DO: 
 * CRM: fix room 116's last exit to 0x0114
 * CRM: fix room 0's first and second exits to 0x0073 0x0001
 * CRM: fix init vector @ $50 from 0x100B0001 to 0x300B0001
 *
 * tile data tttt tttt t gg o xxxx 
 * t: tile #
 * g: lock grade (01 = lockpick, 10 = key 2, 11 = key 1)
 * o: door open flag
 * x: exit lookup number (in exit map [1-8])
 */


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
	glClear(GL_COLOR_BUFFER_BIT);
	display_room(current_room_index);
	display_panel();
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




// Handle keyboard standard keys 
// i.e. those that can be translated to ASCII
static void glut_keyboard(u8 key, int x, int y)
{
	key_down[key]=true;
}

#if defined(WIN32)
#define KEY_FRAME 12
#else
#define KEY_FRAME 8
#endif

void process_motion(void)
{
	u8 strip_base, strip_index;
	bool redisplay = false;
	int new_direction;


	// Check if we're allowed to go where we want
	if ( ((dx != 0) || (d2y != 0)) && (!opt_ghost) &&
		 (!check_footprint(current_room_index, prisoner_x + dx, prisoner_2y + d2y + 2)) )
	{	// can't go there
			dx = 0;
			d2y = 0;
	}

	/*  
	 * Below are the indexes of the relevant 3 sprites groups 
	 * in the 24 sprites strip (joystick position -> strip pos):
	 *    6 7 8
	 *    5 0 1
	 *    4 3 2  
	 */

	// Get the base strip index 
	new_direction = directions[d2y+1][dx+1];
	// NB: if d2y=0 & dx=0, new_dir = 0

	if (new_direction)
	{	// We're moving => animate sprite

		framecount++;
		prisoner_x += dx;
		prisoner_2y += d2y;

		if (last_direction == 0)
		{	// We just started moving
			framecount = 0;
			// pick up the sprite left of idle one
			prisoner_sid = p_sid_base + 3*(new_direction-1);
		}
		else
		{	// continuation of movement
			if (framecount % KEY_FRAME == 0) 
			{
				strip_base = 3*(new_direction-1);
				strip_index = (framecount / KEY_FRAME) % 4;
				if (strip_index == 3)
					strip_index = 1;	// 0 1 2 1 ...
				prisoner_sid = p_sid_base + strip_base + strip_index;
			}
		}
		redisplay = true;
	}
	else if (last_direction != 0)
	{	// We just stopped
		// the middle of the current 3 sprites is stopped pos
		prisoner_sid = p_sid_base + 3*(last_direction-1) + 1;	
		redisplay = true;
	}

	last_direction = new_direction;

	if (redisplay)
		glut_display();
}

static void glut_keyboard_up(u8 key, int x, int y)
{
	key_down[key]=false;
}

// process motion keys
static void glut_idle(void)
{	
	// Reset the motion
	dx = 0; 
	d2y = 0;	

#if !defined(PSP)
	if (key_down[27])
		exit(0);

	// Hey, GLUT, where's my bleeping callback on Windows?
	// NB: The routine is not called if there's no joystick
	//     and the force fumc does not exist on PSP
	glutForceJoystickFunc();	
#endif

	// Keyboard directions
	if ((key_down['4']) || (key_down['q']))
		dx = -1;
	else if ((key_down['6']) || (key_down['o']))
		dx = +1;
	if ((key_down['8']) || (key_down['d']))
		d2y = -1;
	else if ((key_down['2']) || (key_down['x']))
		d2y = +1;

	// Joystick motion overrides keys
	if (jdx)
		dx = jdx;
	if (jd2y)
		d2y = jd2y;

	process_motion();

	// Can't hurt to sleep a while if we're motionless, so that
	// we don't hammer down the CPU in a loop
	if ((dx == 0) && (d2y == 0))
		msleep(30);
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

// Handle keyboard special keys (non ASCII)
static void glut_special_keys(int key, int x, int y)
{
	switch (key) 
	{
	case GLUT_KEY_UP:
	case GLUT_KEY_DOWN:
		if (key == GLUT_KEY_UP)
		{
			if (current_room_index == ROOM_OUTSIDE)
				current_room_index = 0;
			else if (current_room_index < (nb_rooms-1))
				current_room_index++;
		}			
		if (key == GLUT_KEY_DOWN)
		{
			if (current_room_index == 0)
				current_room_index = ROOM_OUTSIDE;
			else if (current_room_index != ROOM_OUTSIDE)
				current_room_index--;
		}
		// Refresh
		glut_display();
		break;
	case GLUT_KEY_RIGHT:
	case GLUT_KEY_LEFT:
		// Change palette
		if ((key == GLUT_KEY_RIGHT) && (palette_index<6))
			palette_index++;
		if ((key == GLUT_KEY_LEFT) && (palette_index!=0))
			palette_index--;
		// Because we can't *PROPERLY* work with 4 bit indexed colours 
		// in GLUT, we have to recreate the whole cells buffer!
		to_16bit_palette(palette_index);
		cells_to_wGRAB(fbuffer[CELLS],rgbCells);
		sprites_to_wGRAB();
		// Refresh
		glut_display();
		break;
	}
}


/* Here we go! */
int main (int argc, char *argv[])
{

	// Flags
	int opt_skip			= 0;	// Skip TIFF Creation
	int opt_error 			= 0;	// getopt

	// General purpose
	u32  i;

	/*
     * Init
     */

#if defined(PSP)
	setup_callbacks();
	gl_width = PSP_SCR_WIDTH;
	gl_height = PSP_SCR_HEIGHT;
#else
	gl_width = 2*PSP_SCR_WIDTH;
	gl_height = 2*PSP_SCR_HEIGHT;
#endif


	glutInit(&argc, argv);

	// Let's clean up our buffers
	fflush(stdin);
	mbuffer    = NULL;
	for (i=0; i<NB_FILES; i++)
		fbuffer[i] = NULL;

	// Process commandline options (works for PSP too with psplink)
	while ((i = getopt (argc, argv, "vbgh")) != -1)
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
		case 'h':
		default:		// Unknown option
			opt_error++;
			break;
	}

	puts ("");
	puts ("Colditz v1.0 : Colditz map explorer");
	puts ("by Agent Smith,  July 2008");
	puts ("");

	if ( ((argc-optind) > 3) || opt_error)
	{
		puts ("usage: Colditz [-f] [-s] [-v] [device] [kernel] [general]");
		puts ("Most features are autodetected, but if you want to specify options:");
		puts ("If no device is given, DVRFlash will detect all DVR devices and exit");
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

	// Set global variables
	get_properties();

	// We're going to convert the cells array, from 2 pixels per byte (paletted)
	// to on RGB(A) word per pixel
	rgbCells = (u8*) aligned_malloc(fsize[CELLS]*2*RGBA_SIZE, 16);
	if (rgbCells == NULL)
	{
		perr("Could not allocate RGB Cells buffers\n");
		ERR_EXIT;
	}

	// Get a palette we can work with
	to_16bit_palette(palette_index);

	// Convert the cells to RGBA data
	cells_to_wGRAB(fbuffer[CELLS],rgbCells);

	// Do the same for overlay sprites
	init_sprites();
	sprites_to_wGRAB();

	// Now we can proceed with our display
	glutDisplayFunc(glut_display);
	glutReshapeFunc(glut_reshape);

	glutKeyboardFunc(glut_keyboard);
	glutKeyboardUpFunc(glut_keyboard_up);
	glutSpecialFunc(glut_special_keys);

	glutJoystickFunc(glut_joystick,30);	
	// This is what you get from using obsolete libraries
	// bloody joystick callback doesn't work on Windows,
	// so we have to stuff the movement handling in idle!!!
	glutIdleFunc(glut_idle);

	glutMainLoop();

	return 0;
}
