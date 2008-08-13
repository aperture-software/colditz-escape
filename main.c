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
int	gl_off_x = 32, gl_off_y  = 16;
int	gl_width = 480, gl_height = 270;

u16  current_room_index = 0;
u16  nb_rooms, nb_sprites, nb_objects;
u8   palette_index = 2;
s_sprite*	sprite;
s_overlay*	overlay;
u8   overlay_index;
u8   bPalette[3][16];


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
static unsigned int __attribute__((aligned(16))) list[262144];
static unsigned short __attribute__((aligned(16))) pixels[PSP_BUF_WIDTH*PSP_SCR_HEIGHT];
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

#if defined(PSP)
void ge_init()
{
	uint x,y;
	uint* row;
	float u,v;
	int i, j, val;
	int sliceWidth;
	Vertex* vertices;
	float curr_fps;
	struct timeval last_time;

	float curr_ms = 1.0f;
	struct timeval time_slices[16];
	void* framebuffer = 0;

	void* fbp0 = getStaticVramBuffer(PSP_BUF_WIDTH,PSP_SCR_HEIGHT,GU_PSM_8888);
	void* fbp1 = getStaticVramBuffer(PSP_BUF_WIDTH,PSP_SCR_HEIGHT,GU_PSM_8888);
	void* zbp = getStaticVramBuffer(PSP_BUF_WIDTH,PSP_SCR_HEIGHT,GU_PSM_4444);


	sceGuInit();

	sceGuStart(GU_DIRECT,list);
	sceGuDrawBuffer(GU_PSM_8888,fbp0,PSP_BUF_WIDTH);
	sceGuDispBuffer(PSP_SCR_WIDTH,PSP_SCR_HEIGHT,fbp1,PSP_BUF_WIDTH);
	sceGuDepthBuffer(zbp,PSP_BUF_WIDTH);
	sceGuOffset(2048 - (PSP_SCR_WIDTH/2),2048 - (PSP_SCR_HEIGHT/2));
	sceGuViewport(2048,2048,PSP_SCR_WIDTH,PSP_SCR_HEIGHT);
	sceGuDepthRange(65535,0);
	sceGuScissor(0,0,PSP_SCR_WIDTH,PSP_SCR_HEIGHT);
	sceGuEnable(GU_SCISSOR_TEST);
	sceGuFrontFace(GU_CW);
	sceGuClear(GU_COLOR_BUFFER_BIT|GU_DEPTH_BUFFER_BIT);
	sceGuFinish();
	sceGuSync(0,0);

	sceDisplayWaitVblankStart();
	sceGuDisplay(1);

	val = 0;

	for (y = 0; y < 272; ++y)
	{
		row = &pixels[y * 512];
		for (x = 0; x < 480; ++x)
		{
			row[x] = x * y;
		}
	}


	sceKernelDcacheWritebackAll();

	while (1)
	{
		sceGuStart(GU_DIRECT,list);

		// copy image from ram to vram

		sceGuCopyImage(GU_PSM_8888,0,0,32,16,32,((u8*)rgbCells) + 4*8*0x100,100,100,512,(void*)(0x04000000+(u32)framebuffer));
		sceGuCopyImage(GU_PSM_8888,0,0,32,16,32,((u8*)rgbCells) + 5*8*0x100,132,100,512,(void*)(0x04000000+(u32)framebuffer));
		sceGuCopyImage(GU_PSM_8888,0,0,32,16,32,((u8*)rgbCells) + 6*8*0x100,164,100,512,(void*)(0x04000000+(u32)framebuffer));
		//		sceGuCopyImage(GU_PSM_8888,0,0,480,272,512,pixels,0,0,512,(void*)(0x04000000+(u32)framebuffer));
		sceGuTexSync();

		sceGuFinish();
		sceGuSync(0,0);

		curr_fps = 1.0f / curr_ms;

//		sceDisplayWaitVblankStart();
		framebuffer = sceGuSwapBuffers();

		pspDebugScreenSetXY(0,0);
		pspDebugScreenPrintf("%d.%03d",(int)curr_fps,(int)((curr_fps-(int)curr_fps) * 1000.0f));

		// simple frame rate counter

		gettimeofday(&time_slices[val & 15],0);

		val++;

		if (!(val & 15))
		{
			last_time = time_slices[0];
			i;

			curr_ms = 0;
			for (i = 1; i < 16; ++i)
			{
				struct timeval curr_time = time_slices[i];

				if (last_time.tv_usec > curr_time.tv_usec)
				{
					curr_time.tv_sec++;
					curr_time.tv_usec-=1000000;
				}

				curr_ms += ((curr_time.tv_usec-last_time.tv_usec) + (curr_time.tv_sec-last_time.tv_sec) * 1000000) * (1.0f/1000000.0f);

				last_time = time_slices[i];
			}
			curr_ms /= 15.0f;
		}
	}

//	sceDisplayWaitVblankStart();
//	sceGuDisplay(GU_TRUE);


//	sceGuCopyImage(GU_PSM_8888,0,0,32,16,32,((u8*)rgbCells) + 18*0x100,0,0,32,fbp0+0x4000000);


/*
	sceKernelDcacheWritebackInvalidateAll();
	sceGuStart(GU_DIRECT, list);
	sceGuTexImage(0, 32, 16, 32, ((u8*)rgbCells) + 24*0x100);
	u = 1.0f / 32.0f;
	v = 1.0f / 16.0f;
	sceGuTexScale(u, v);
	
	j = 0;
	while (j < 32) {
		vertices = (Vertex*) sceGuGetMemory(2 * sizeof(Vertex));
		sliceWidth = 64;
		if (j + sliceWidth > 32) sliceWidth = 32 - j;
		vertices[0].u = 32 + j;
		vertices[0].v = 16;
		vertices[0].x = 0 + j;
		vertices[0].y = 0;
		vertices[0].z = 0;
		vertices[1].u = 32 + j + sliceWidth;
		vertices[1].v = 16 + 16;	// height
		vertices[1].x = 0 + j + sliceWidth;
		vertices[1].y = 0 + 16;	// height
		vertices[1].z = 0;
		sceGuDrawArray(GU_SPRITES, GU_TEXTURE_16BIT | GU_VERTEX_16BIT | GU_TRANSFORM_2D, 2, 0, vertices);
		j += sliceWidth;
	}
	
*/


}
#endif


#if !defined(PSP)	// This tells us if we're using glut
/**
 ** GLUT event handlers
 **/
void ge_init()
{
	// OK, now we're ready to display some stuff onscreen
	glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE);
	glutInitWindowSize(gl_width, gl_height);
	glutCreateWindow("Colditz Explorer");

	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_BLEND);
//	glDisable(GL_COLOR_MATERIAL);

	glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, gl_width, 0, gl_height, -1, 1);



//    glMatrixMode(GL_MODELVIEW); 
//    glLoadIdentity(); 
}

// Handle keyboard standard keys 
// i.e. those that can be translated to ASCII
void glut_keyboard(u8 key, int x, int y)
{
	switch (key)
	{
	case 27:
		exit(0);
		break;
	}
}

// Handle keyboard special keys (non ASCII)
void glut_special_keys(int key, int x, int y)
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
		glutPrintf("[%d:%03X]", palette_index, current_room_index);
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
		// in openGL, we have to recreate the whole cells buffer!
		to_24bit_Palette(palette_index);
		cells_to_RGB(fbuffer[CELLS],rgbCells,fsize[CELLS]);
		sprites_to_RGB();
		// Refresh
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		displayRoom(current_room_index);
		glutPrintf(0, "[%d:%03X]", palette_index, current_room_index);
		glutSwapBuffers();
		break;
	}
}

// 
void glut_reshape (int w, int h)
{
  gl_width=w;
  gl_height=h;
  glViewport(0,0,w,h);
}


void glut_display(void)
{

	//Create some nice colours (3 floats per pixel) from data -10..+10
//	float* pixels = new float[size*3];
//	for(int i=0;i<size;i++) {
//	colour(10.0-((i*20.0)/size),&pixels[i*3]);
//	} 

//	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	//http://msdn2.microsoft.com/en-us/library/ms537062.aspx
	//glDrawPixels writes a block of pixels to the framebuffer.

//	glDrawPixels(gl_width,gl_height,GL_RGB,GL_FLOAT,pixels);

//	glPointSize(2);
//	glBegin(GL_POINTS);
//	glEnd();
	glutSwapBuffers();
}
#endif


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
	pspDebugScreenInit();
#else
	glutInit(&argc, argv);
#endif

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

	// Load the data
	load_all_files();
	getProperties();

	// Reorganize cells from interleaved bitplane lines to interleaved bitplane bits
//	cells_to_interleaved(fbuffer[CELLS],fsize[CELLS]);

	// And then create a new cell buffer
	// TO_DO (change back to 6 and ARGB to RGB)
	rgbCells = (u8*) aligned_malloc(fsize[CELLS]*8, 16);
//	rgbCells = (u8*) calloc(fsize[CELLS]*6, 1);
	if (rgbCells == NULL)
	{
		perr("Could not allocate RGB Cells buffers\n");
		ERR_EXIT;
	}

	// To expand to 24 bit RGB data
	to_24bit_Palette(palette_index);
	cells_to_RGB(fbuffer[CELLS],rgbCells,fsize[CELLS]);
//	cells_to_ARGB(fbuffer[CELLS],rgbCells,fsize[CELLS]);
//	nb_sprites = 3;
	init_sprites();
	sprites_to_RGB();


	ge_init();
#if !defined(PSP)

	// Display the first room
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	displayRoom(current_room_index);
	glutPrintf("[%d:%03X]", palette_index, current_room_index);
//	glutSwapBuffers();
		
	glutDisplayFunc(glut_display);
	glutReshapeFunc(glut_reshape);
	//glutMouseFunc(mouse_button);
	//glutMotionFunc(mouse_motion);
	glutKeyboardFunc(glut_keyboard);
	glutSpecialFunc(glut_special_keys);

	//glutIdleFunc(idle);

	glutMainLoop();
#else
    sceKernelSleepThread();
#endif
//	glEnable(GL_DEPTH_TEST);
//	glClearColor(0.0, 0.0, 0.0, 1.0);
	// TO_DO: double size by enabling what follows

//	glEnable(0);

	return 0;
}
