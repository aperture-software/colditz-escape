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
// Tell VC++ to include the libs
#pragma comment(lib, "opengl32.lib")
#pragma comment(lib, "glu32.lib")
#pragma comment(lib, "glut32.lib")
#endif

#include "colditz.h"
#include "utilities.h"
//#include "tiffio.h"
#include "getopt.h"	

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


#if defined(GLUT_API_VERSION)
// This tells us if we're using glut
/**
 ** GLUT event handlers
 **/
void glut_init()
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
		glutPrintf(0, "[%d:%03X]", palette_index, current_room_index);
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
	int opt_force			= 0;

	// General purpose
	u32  i;
	static u16  tiffPalette[3][16];

	/*
     * Init
     */

	glutInit(&argc, argv);

	// Let's clean up our buffers
	fflush(stdin);
	mbuffer    = NULL;
	for (i=0; i<NB_FILES; i++)
		fbuffer[i] = NULL;

	// Process commandline options
	while ((i = getopt (argc, argv, "bfhsv")) != -1)
		switch (i)
	{
		case 'v':		// Print verbose messages
			opt_verbose = -1;
			break;
		case 'f':       // Force flashing
			opt_force++;
			break;
		case 'b':       // Debug mode (don't flash!)
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
	rgbCells = (u8*) calloc(fsize[CELLS]*6, 1);
	if (rgbCells == NULL)
	{
		fprintf (stderr, "Could not allocate RGB Cells buffers\n");
		ERR_EXIT;
	}

	// To expand to 24 bit RGB data
	to_24bit_Palette(palette_index);
	cells_to_RGB(fbuffer[CELLS],rgbCells,fsize[CELLS]);
//	nb_sprites = 3;
	init_sprites();
	sprites_to_RGB();

	// Get a palette to save TIFF
//	to_48bit_Palette(tiffPalette, palette_index);

	glut_init();

	// Display the first room
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	displayRoom(current_room_index);
	glutPrintf(0, "[%d:%03X]", palette_index, current_room_index);
//	glutSwapBuffers();
		
	glutDisplayFunc(glut_display);
	glutReshapeFunc(glut_reshape);
	//glutMouseFunc(mouse_button);
	//glutMotionFunc(mouse_motion);
	glutKeyboardFunc(glut_keyboard);
	glutSpecialFunc(glut_special_keys);

	//glutIdleFunc(idle);



	glutMainLoop();

//	glEnable(GL_DEPTH_TEST);
//	glClearColor(0.0, 0.0, 0.0, 1.0);
	// TO_DO: double size by enabling what follows

//	glEnable(0);


//#define _PROCESS_SPRITES
#ifdef _PROCESS_SPRITES
	u32 tile_offset = 0;					// Offsets to each rooms are given at
								// the beginning of the Rooms Map file
	int ignore = 0;				// We got to ignore a few FFFFFFFF offsets
	u16 room_x, room_y;
	u16 nb_tiles;
	u16 tile_data;
	char tiffname[NAME_SIZE];	// Hopefully we'll save a few TIFFs ;)
	TIFF* image;
	int no_mask = 0;


	// Read the number of sprites
	u32 index = 0;
	u16 sprite_index = 0;
	u16 sprite_x, sprite_y;
	u16 bitplane_size;
	u16 nb_sprites = readword(fbuffer[SPRITES],index) + 1;
	index+=2;
	u32 sprite_address = index + 4*(nb_sprites);
	for (sprite_index=0; sprite_index<nb_sprites; sprite_index++)
	{
		sprite_address += readlong(fbuffer[SPRITES],index);
		writelong(fbuffer[SPRITES],index,sprite_address);
		index+=4;
	}
	// Each sprite is prefixed by 2 words (x size in words, y size in pixels)
	// and one longword (size of one bitplane, in bytes)
	// NB: MSb on x size will be set if sprite is animated

	for (sprite_index=0; sprite_index<nb_sprites; sprite_index++)
	{
		sprite_address = readlong(fbuffer[SPRITES],2+4*sprite_index);
		printf("sprite[%X] address = %08X\n", sprite_index, sprite_address);
		// x size is given in words
		sprite_x = 16*(readword(fbuffer[SPRITES],sprite_address)&0x7FFF);
		sprite_x = readword(fbuffer[SPRITES],sprite_address);
		sprite_y = readword(fbuffer[SPRITES],sprite_address+2);
		printf("  x,y = %0X, %0X\n", sprite_x, sprite_y);
		bitplane_size = readword(fbuffer[SPRITES],sprite_address+6);
		printf("  bitplane_size = %0X\n", bitplane_size);

		// if MSb is set, we have 4 bitplanes instead of 5
		no_mask = (sprite_x & 0x8000);

		// Reoganize sprites from separate bitplanes to interleaved bitplane bits
		sprites_to_interleaved(((u8*)fbuffer[SPRITES]) + sprite_address + 8 + (no_mask?0:bitplane_size),  bitplane_size);
		sprite_x = 16*(sprite_x&0x7FFF);


		if (!opt_skip)
		{
			// Initialize the TIFF file for the current room
			sprintf(tiffname, "sprite_%02X.tif", sprite_index);
			image = TIFFOpen(tiffname, "w");
			if (image == NULL)
			{
				fprintf(stderr, "Unable to create file %s\n", tiffname);
				ERR_EXIT;
			}
			if (opt_verbose)
				printf("Created file '%s'...\n", tiffname);

			// We need to set some values for basic tags before we can add any data
			TIFFSetField(image, TIFFTAG_IMAGEWIDTH, sprite_x);
			TIFFSetField(image, TIFFTAG_IMAGELENGTH, sprite_y);

			// 16 bit palette
			TIFFSetField(image, TIFFTAG_BITSPERSAMPLE, 4);
			TIFFSetField(image, TIFFTAG_SAMPLESPERPIXEL, 1);
			TIFFSetField(image, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);
			TIFFSetField(image, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_PALETTE);
			TIFFSetField(image, TIFFTAG_COLORMAP, palette[0], palette[1], palette[2]);
			TIFFSetField(image, TIFFTAG_ROWSPERSTRIP, sprite_y);

			TIFFWriteEncodedStrip(image, 0, ((u8*)fbuffer[SPRITES]) + sprite_address + 8 + (no_mask?0:bitplane_size), bitplane_size*4);
			TIFFClose(image);

		}

	}

#endif



#ifdef _PROCESS_COMPRESSED_MAP

// COMPRESSED MAP
	room_x = 0x54;
	room_y = 0x44;
	u8* over = (u8*) calloc(room_x*room_y, 1);
	for (int a=0; a<room_x; a++)
		for (int b=0; b<room_y; b++)
			writebyte(over,b*room_x+a,0);
	u32 room_index = 0x400;
	
		if (!opt_skip)
		{
			// Initialize the TIFF file for the current room
			sprintf(tiffname, "room_%03X.tif", room_index);
			image = TIFFOpen(tiffname, "w");
			if (image == NULL)
			{
				fprintf(stderr, "Unable to create file %s\n", tiffname);
				ERR_EXIT;
			}
			if (opt_verbose)
				printf("Created file '%s'...\n", tiffname);

			// We need to set some values for basic tags before we can add any data
//			TIFFSetField(image, TIFFTAG_IMAGEWIDTH, 32*room_x);
//			TIFFSetField(image, TIFFTAG_IMAGELENGTH, 16*room_y);
			TIFFSetField(image, TIFFTAG_IMAGEWIDTH, 32*0x54);
			TIFFSetField(image, TIFFTAG_IMAGELENGTH, 16*0x44);

			// 16 bit palette
			TIFFSetField(image, TIFFTAG_BITSPERSAMPLE, 4);
			TIFFSetField(image, TIFFTAG_SAMPLESPERPIXEL, 1);
			TIFFSetField(image, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);
			TIFFSetField(image, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_PALETTE);
			TIFFSetField(image, TIFFTAG_COLORMAP, palette[0], palette[1], palette[2]);
			// We'll use tiling, since you're so kind as to ask ;)
			TIFFSetField(image, TIFFTAG_TILEWIDTH, 32);
			TIFFSetField(image, TIFFTAG_TILELENGTH, 16);
		}

		// Read the tiles data
//		for (int tile_y=(room_y-1); tile_y>=0; tile_y--)
		int u;
		int tile_y;
		int tile_x;
		for (tile_y=0; tile_y<room_y; tile_y++)
		{
			printf("    ");	// Start of a line
			for(tile_x=0; tile_x<room_x; tile_x++)
			{
				tile_offset = readword((u8*)fbuffer[COMPRESSED_MAP], (tile_y*room_x+tile_x)*4);
				if (tile_offset == 0)
					tile_data = 0;
				else
				{
					nb_tiles = readword((u8*)fbuffer[COMPRESSED_MAP], CM_TILES_START+tile_offset);
					tile_offset +=2;
					for (u=0;u<nb_tiles;u++)
						tile_data = readword((u8*)fbuffer[COMPRESSED_MAP], CM_TILES_START+tile_offset+2*u);
					tile_offset +=(2*nb_tiles);
				}
				if (!opt_skip)
				{
					// CRM Tile data is of the form
					// Tile data is of the form (tile_index<<7) + some_exit_flags, with tile_index being 
					// the index in COLDITZ_CELLS (each tile occupying 0x100 bytes there)
					if (!readbyte(over, tile_x+tile_y*room_y))
					{
						TIFFWriteRawTile(image , tile_y*room_x + tile_x, ((u8*)fbuffer[CELLS]) + ((tile_data & 0xFF80) >>7)*0x100, 0x100);
						writebyte(over, tile_x+tile_y*room_y,1);
					}
				}
				//offset +=4;		// Read next tile
				printf("%04X ", tile_data & 0xFF80);
//				if (readbyte(over, tile_x+tile_y*room_y))
//					printf("OVERWRITE!!!\n");
//				else
//					writebyte(over, tile_x+tile_y*room_y,1);
			}
			printf("\n");
		}

		printf("went to %X\n", (tile_y*room_x+tile_x)*4);

		if (!opt_skip)
		// Close the image file
			TIFFClose(image);

#endif


//#define _PROCESS_ROOM_MAP
#ifdef _PROCESS_ROOM_MAP
	/*
	 * Process rooms
	 */
	u32 offset;					// Offsets to each rooms are given at
								// the beginning of the Rooms Map file
	int ignore = 0;				// We got to ignore a few FFFFFFFF offsets
	u16 room_x, room_y, tile_data;
	char tiffname[NAME_SIZE];	// Hopefully we'll save a few TIFFs ;)
	TIFF* image;

	for (u32 room_index=9; room_index<10 ; room_index++)
	{	
		// Read the offset
		offset = readlong((u8*)fbuffer[0], OFFSETS_START+4*room_index);
		if (offset == 0xFFFFFFFF)
		{	// For some reason there is a break in the middle
			ignore++;
			if (ignore > FFs_TO_IGNORE)
				break;
			else
			{
				printf("\noffset[%03X] = %08X (IGNORING)\n", room_index, offset);
				continue;
			}
		}

		// Now that we have the offset, let's look at the room
		printf("\noffset[%03X] = %08X ", room_index, offset);
		// The 2 first words are the room Y and X dimension, in tha order
		room_y = readword((u8*)fbuffer[0], ROOMS_START+offset);
		offset +=2;
		room_x = readword((u8*)fbuffer[0], ROOMS_START+offset);
		offset +=2;
		printf("(room_x=%X,room_y=%X)\n", room_x, room_y);

		if (!opt_skip)
		{
			// Initialize the TIFF file for the current room
			sprintf(tiffname, "room_%03X.tif", room_index);
			image = TIFFOpen(tiffname, "w");
			if (image == NULL)
			{
				fprintf(stderr, "Unable to create file %s\n", tiffname);
				break;
			}
			if (opt_verbose)
				printf("Created file '%s'...\n", tiffname);

			// We need to set some values for basic tags before we can add any data
			TIFFSetField(image, TIFFTAG_IMAGEWIDTH, 32*room_x);
			TIFFSetField(image, TIFFTAG_IMAGELENGTH, 16*room_y);
			// 16 bit palette
			TIFFSetField(image, TIFFTAG_BITSPERSAMPLE, 4);
			TIFFSetField(image, TIFFTAG_SAMPLESPERPIXEL, 1);
			TIFFSetField(image, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);
			TIFFSetField(image, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_PALETTE);
			TIFFSetField(image, TIFFTAG_COLORMAP, tiffPalette[RED], tiffPalette[GREEN], tiffPalette[BLUE]);
			// We'll use tiling, since you're so kind as to ask ;)
			TIFFSetField(image, TIFFTAG_TILEWIDTH, 32);
			TIFFSetField(image, TIFFTAG_TILELENGTH, 16);
		}
		else
		{
			// Initialise the GL window for the current room
		}

		// Read the tiles data
		for (int tile_y=0; tile_y<room_y; tile_y++)
		{
			printf("    ");	// Start of a line
			for(int tile_x=0; tile_x<room_x; tile_x++)
			{
				// A tile is 32(x)*16(y)*4(bits) = 256 bytes
				// A specific room tile is identified by a word
				tile_data = readword((u8*)fbuffer[0], ROOMS_START+offset);
				if (!opt_skip)
				{
					// Tile data is of the form (tile_index<<7) + some_exit_flags, with tile_index being 
					// the index in COLDITZ_CELLS (each tile occupying 0x100 bytes there)
					TIFFWriteRawTile(image , tile_y*room_x + tile_x, ((u8*)fbuffer[CELLS]) + (tile_data>>7)*0x100 + 
						// Take care of the tunnels too below
						((room_index>0x202)?0x1E000:0), 0x100);
				}
				// position ourselves to the right location
//				glPointSize(2.0);
				glRasterPos2i(gl_off_x+tile_x*32,gl_height-gl_off_y-(tile_y*16));
				glDrawPixels(32,16,GL_RGB,GL_UNSIGNED_BYTE,
					((u8*)cells) + (tile_data>>7)*0x600 +((room_index>0x202)?(6*0x1E000):0));

				offset +=2;		// Read next tile
				printf("%04X ", tile_data);
			}
			printf("\n");
		}

		if (!opt_skip)
		// Close the image file
			TIFFClose(image);

	}
#endif

	return 0;
}
