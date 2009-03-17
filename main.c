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

// File stuff
FILE* fd				= NULL;
char* fname[NB_FILES]	= FNAMES;			// file name(s)
u32   fsize[NB_FILES]	= FSIZES;
u8*   fbuffer[NB_FILES];
u8*   mbuffer			= NULL;
u8*	  rgbCells			= NULL;
// GL Stuff
int	gl_off_x = 0, gl_off_y  = 0;
int	gl_width = PSP_SCR_WIDTH, gl_height = PSP_SCR_HEIGHT;
int prisoner_x = 0, prisoner_y = 0;
u8  prisoner_sid = 0x07;
float origin_x = 0, origin_y = 0;

float zoom_level = 200.0;


u16  current_room_index = 0x00;
u16  nb_rooms, nb_cells, nb_sprites, nb_objects;
u8   palette_index = 2;
s_sprite*	sprite;
s_overlay*	overlay;
u8   overlay_index;
u8   bPalette[3][16];
// remapped Amiga Palette
u16  aPalette[16];

//GLuint texid;
GLuint* cell_texid;
GLuint* sprite_texid;

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
    GLCHK(glOrtho(0, gl_width, gl_height, 0, -1, 1));
    GLCHK(glMatrixMode(GL_MODELVIEW)); 

	// Setup transparency
	GLCHK(glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA));
	GLCHK(glEnable(GL_BLEND));

	// Disable depth
	GLCHK(glDisable(GL_DEPTH_TEST));
//	GLCHK(glEnable(GL_DEPTH_TEST));
	GLCHK(glEnable(GL_TEXTURE_2D));
}


// Handle keyboard standard keys 
// i.e. those that can be translated to ASCII
static void glut_keyboard(u8 key, int x, int y)
{
	int dx = 0, dy = 0;
	u8 strip_base, strip_index;
	bool redisplay = false;
	int new_direction;

	switch (key)
	{
	case 27:
		exit(0);
		break;
	case 'w':
		dy = -1;
		break;
	case 'a':
		dx = -1;
		break;
	case 'd':
		dx = 1;
		break;
	case 's':
		dy = 1;
	}

	framecount++;
	if (framecount%2)
		return;

	/*  
	 * Below are the index of the relevant 3 sprites group 
	 * in the 24 sprites strip (joystick position -> strip pos):
	 *    6 7 8
	 *    5 0 1
	 *    4 3 2  
	 */

	// Get the base strip index 
	new_direction = directions[dy+1][dx+1];

	if ((new_direction == 0) && (last_direction != 0))
	{	// we stopped
		prisoner_sid = 3*(last_direction-1) + 1;	// the middle of 3 sprites is stopped pos
		redisplay = true;
	}

	if (new_direction != 0)
	{	// We're moving => animate sprite
		if (last_direction == 0)
		{	// We just started moving
			framecount = 0;
			// pick up the sprite left of idle one
			prisoner_sid = 3*(new_direction-1);
		}
		else
		{	// continuation of movement
//			framecount++;
#define KEY_FRAME 12
			if (framecount % KEY_FRAME == 0) 
			{
				strip_base = 3*(new_direction-1);
				strip_index = (framecount / KEY_FRAME) % 4;
				if (strip_index == 3)
					strip_index = 1;	// 0 1 2 1 ...
				prisoner_sid = strip_base + strip_index;
			}
		}
		redisplay = true;
	}

//	if (dx || dy)
//		print("dx = %d, dy = %d, last = %d, new = %d, sid = %X\n", dx, dy, last_direction, new_direction, prisoner_sid);
	last_direction = new_direction;

	prisoner_x += dx;
	prisoner_y += dy;

	if (redisplay)
	{
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		displayRoom(current_room_index);
		glutSwapBuffers();
	}
//	print("x = %d, y = %d\n", x, y);

}

static void glut_joystick(uint buttonMask, int x, int y, int z)
{
	int dx = 0, dy = 0;
	u8 strip_base, strip_index;
	bool redisplay = false;
	int new_direction;

	framecount++;
	printf("%d\n", framecount);
	if (framecount%2)
		return;

	// compute x and y displacements
	if (x>250)
		dx = 1;
	if (x<-250)
		dx = -1;
	if (y>250)
		dy = 1;
	if (y<-250)
		dy = -1;

	/*  
	 * Below are the index of the relevant 3 sprites group 
	 * in the 24 sprites strip (joystick position -> strip pos):
	 *    6 7 8
	 *    5 0 1
	 *    4 3 2  
	 */

	// Get the base strip index 
	new_direction = directions[dy+1][dx+1];

	if ((new_direction == 0) && (last_direction != 0))
	{	// we stopped
		prisoner_sid = 3*(last_direction-1) + 1;	// the middle of 3 sprites is stopped pos
		redisplay = true;
	}

	if (new_direction != 0)
	{	// We're moving => animate sprite
		if (last_direction == 0)
		{	// We just started moving
			framecount = 0;
			// pick up the sprite left of idle one
			prisoner_sid = 3*(new_direction-1);
		}
		else
		{	// continuation of movement
//			framecount++;
#define KEY_FRAME 12
			if (framecount % KEY_FRAME == 0) 
			{
				strip_base = 3*(new_direction-1);
				strip_index = (framecount / KEY_FRAME) % 4;
				if (strip_index == 3)
					strip_index = 1;	// 0 1 2 1 ...
				prisoner_sid = strip_base + strip_index;
			}
		}
		redisplay = true;
	}

//	if (dx || dy)
//		print("dx = %d, dy = %d, last = %d, new = %d, sid = %X\n", dx, dy, last_direction, new_direction, prisoner_sid);
	last_direction = new_direction;

	prisoner_x += dx;
	prisoner_y += dy;

	if (redisplay)
	{
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		displayRoom(current_room_index);
		glutSwapBuffers();
	}
//	print("x = %d, y = %d\n", x, y);
}

// Handle keyboard special keys (non ASCII)
static void glut_special_keys(int key, int x, int y)
{
	switch (key) 
	{
	case GLUT_KEY_UP:
	case GLUT_KEY_DOWN:
		if ((key == GLUT_KEY_UP) && 
			((current_room_index < (nb_rooms-1)) || (current_room_index == 0xFFFF)))
			current_room_index++;
		if ((key == GLUT_KEY_DOWN) && (current_room_index != 0xFFFF))
			current_room_index--;
		// Refresh
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		displayRoom(current_room_index);
		glutSwapBuffers();
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
		to_16bit_Palette(palette_index);
		cells_to_wGRAB(fbuffer[CELLS],rgbCells);
		sprites_to_wGRAB();
		// Refresh
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		displayRoom(current_room_index);
		glutSwapBuffers();
		break;
	}
}

// 
static void glut_reshape (int w, int h)
{
	GLCHK(glViewport(0, 0, w, h));

	GLCHK(glMatrixMode(GL_PROJECTION));
	GLCHK(glLoadIdentity());

	GLCHK(glOrtho(0, (PSP_SCR_WIDTH*100)/zoom_level, (PSP_SCR_HEIGHT*100)/zoom_level, 0, -1, 1));

	// As far as the projection is concerned, the zoomed width is still the constant PSP_SCR_WIDTH
	// Thus we need to compute how the projection "sees" the viewport width (width*100/zoom) after
	// the zoom, and we can find out how we need to move our origin
	// Note: this zoom only works for FIXED window size
	origin_x = PSP_SCR_WIDTH/2.0f * (100.0f/zoom_level - 1.0f);
	origin_y = PSP_SCR_HEIGHT/2.0f * (100.0f/zoom_level - 1.0f);

	GLCHK(glMatrixMode(GL_MODELVIEW));
	GLCHK(glLoadIdentity());

	gl_width=w;
	gl_height=h;
}


static void glut_display(void)
{
	GLCHK(glClear(GL_COLOR_BUFFER_BIT));

	GLCHK(glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE));

	GLCHK(glMatrixMode(GL_MODELVIEW));
	GLCHK(glLoadIdentity());

	displayRoom(current_room_index);

	glutSwapBuffers();

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
#endif
	glutInit(&argc, argv);

	// Let's clean up our buffers
	fflush(stdin);
	mbuffer    = NULL;
	for (i=0; i<NB_FILES; i++)
		fbuffer[i] = NULL;

	// Process commandline options (works for PSP too with psplink)
	while ((i = getopt (argc, argv, "bhsv")) != -1)
		switch (i)
	{
		case 'v':		// Print verbose messages
			opt_verbose = -1;
			break;
		case 'b':       // Debug mode
			opt_debug = -1;
			break;
		case 's':		// skip generation of TIFF files
			opt_skip = -1;
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
	getProperties();

	// We're going to convert the cells array, from 2 pixels per byte (paletted)
	// to on RGB(A) word per pixel
	rgbCells = (u8*) aligned_malloc(fsize[CELLS]*2*RGBA_SIZE, 16);
	if (rgbCells == NULL)
	{
		perr("Could not allocate RGB Cells buffers\n");
		ERR_EXIT;
	}

	// Get a palette we can work with
	to_16bit_Palette(palette_index);

	// Convert the cells to RGBA data
	cells_to_wGRAB(fbuffer[CELLS],rgbCells);

	// Do the same for overlay sprites
	init_sprites();
	sprites_to_wGRAB();

	// Now we can proceed with our display
	glutDisplayFunc(glut_display);
	glutReshapeFunc(glut_reshape);
	glutJoystickFunc(glut_joystick, 30);	
	// This is what you get from using obsolete libraries
	// bloody callback doesn't work on Windows!!!
	//glutMouseFunc(mouse_button);
	//glutMotionFunc(mouse_motion);
	glutKeyboardFunc(glut_keyboard);
	glutSpecialFunc(glut_special_keys);

	glutMainLoop();

	return 0;
}
