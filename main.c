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
#include <pspctrl.h>
#include <pspdisplay.h>
#include <pspgu.h>
#include <psprtc.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glut.h>
#include "psp/vram.h"
//#include "psp/graphics.h"
#endif

#include "getopt.h"	
#include "colditz.h"
#include "utilities.h"


typedef struct 
{
	unsigned short u, v;
	unsigned short color;
	short x, y, z;
} Vertex;

/*
 *  Global variables, set to static to avoid name confusion, e.g. with stat()
 */

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
int origin_x = 0, origin_y = 0;
//int zoom_level = 150;
int zoom_level = 150;


u16  current_room_index = 0;
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

#if defined(PSP)
//static unsigned int __attribute__((aligned(16))) list[262144];
//static unsigned short __attribute__((aligned(16))) pixels[PSP_BUF_WIDTH*PSP_SCR_HEIGHT];
/* Define the module info section */
PSP_MODULE_INFO("colditz", 0, 1, 1);

/* Define the main thread's attribute value (optional) */
PSP_MAIN_THREAD_ATTR(THREAD_ATTR_USER | THREAD_ATTR_VFPU);
//PSP_MAIN_THREAD_ATTR(PSP_THREAD_ATTR_USER);
// Leave 256 KB for threads
PSP_HEAP_SIZE_KB(-256);
//PSP_HEAP_SIZE_MAX();

/* Exit callback */
int exitCallback(int arg1, int arg2, void *common) {
	sceKernelExitGame();
	return 0;
}

/* Callback thread */
int callbackThread(SceSize args, void *argp) {
	int cbid;

	cbid = sceKernelCreateCallback("Exit Callback", (void*) exitCallback, NULL);
	sceKernelRegisterExitCallback(cbid);
	sceKernelSleepThreadCB();

	return 0;
}

/* Sets up the callback thread and returns its thread id */
int setupCallbacks(void) {
	int thid = 0;

	thid = sceKernelCreateThread("update_thread", callbackThread, 0x11, 0xFA0, 0, 0);
	if (thid >= 0) {
		sceKernelStartThread(thid, 0, 0);
	}
	return thid;
}
#endif


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

	GLCHK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
	GLCHK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
}


// Handle keyboard standard keys 
// i.e. those that can be translated to ASCII
static void glut_keyboard(u8 key, int x, int y)
{
	switch (key)
	{
	case 27:
		exit(0);
		break;
	}
}

// Handle keyboard special keys (non ASCII)
static void glut_special_keys(int key, int x, int y)
{
	switch (key) 
	{
	case GLUT_KEY_UP:
	case GLUT_KEY_DOWN:
		if ((key == GLUT_KEY_UP) && (current_room_index < (nb_rooms-1)))
			current_room_index++;
		if ((key == GLUT_KEY_DOWN) && (current_room_index != 0))
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

//	GLCHK(glOrtho(0, 240, 136, 0, -1, 1));
	GLCHK(glOrtho(0, PSP_SCR_WIDTH*100/zoom_level, PSP_SCR_HEIGHT*100/zoom_level, 0, -1, 1));
	origin_x = PSP_SCR_WIDTH/2 - (PSP_SCR_WIDTH/2) * zoom_level/100;
	origin_y = PSP_SCR_HEIGHT/2 - (PSP_SCR_HEIGHT/2) * zoom_level/100;

	GLCHK(glMatrixMode(GL_MODELVIEW));
	GLCHK(glLoadIdentity());

	gl_width=w;
	gl_height=h;
}


static void glut_display(void)
{
//	GLCHK(glShadeModel(GL_SMOOTH));

	GLCHK(glClear(GL_COLOR_BUFFER_BIT));

	GLCHK(glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE));

	GLCHK(glMatrixMode(GL_MODELVIEW));
	GLCHK(glLoadIdentity());

	displayRoom(current_room_index);

	glutSwapBuffers();
	// glutPostRedisplay();

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
	setupCallbacks();
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

	glut_init();

	// Load the data
	load_all_files();
	getProperties();

	// Reorganize cells from interleaved bitplane lines to interleaved bitplane bits
//	cells_to_interleaved(fbuffer[CELLS],fsize[CELLS]);

	// And then create a new cell buffer
	// TO_DO (change back to 6 and ARGB to RGB)
	rgbCells = (u8*) aligned_malloc(fsize[CELLS]*2*RGBA_SIZE, 16);
	if (rgbCells == NULL)
	{
		perr("Could not allocate RGB Cells buffers\n");
		ERR_EXIT;
	}

	to_16bit_Palette(palette_index);
	printf("cells_to_wGRAB\n");
	cells_to_wGRAB(fbuffer[CELLS],rgbCells);
	printf("cells_to_wGRAB2\n");
	init_sprites();
	sprites_to_wGRAB();

	// Display the first room
//	glClear(GL_COLOR_BUFFER_BIT);
//	displayRoom(current_room_index);
#if !defined(PSP)
	glutPrintf("[%d:%03X]", palette_index, current_room_index);
#endif
//	glutSwapBuffers();

	glutDisplayFunc(glut_display);
	glutReshapeFunc(glut_reshape);
	//glutMouseFunc(mouse_button);
	//glutMotionFunc(mouse_motion);
	glutKeyboardFunc(glut_keyboard);
	glutSpecialFunc(glut_special_keys);

	glutMainLoop();

	return 0;
}
