/**
 **  Escape from Colditz
 **
 **  Utility functions
 **
 **/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if !defined(PSP)
#include <windows.h>
#include <gl/gl.h>
#include <gl/glu.h>
#include <gl/glut.h>
#else
#include <stdarg.h>
#include <pspkernel.h>
#include <pspdebug.h>
#include <pspgu.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glut.h>
#endif

#include "data-types.h"
#include "colditz.h"
#include "utilities.h"
#include "low-level.h"
#include "modplayer.h"


/* Whatever you do, you don't want local variables holding textures */
GLuint* cell_texid;
GLuint* sprite_texid;
GLuint* chars_texid;
GLuint panel1_texid, panel2_texid;
GLuint render_texid, picture_texid;

u16 picture_w, picture_h;

/* Some more globals */
u8  obs_to_sprite[NB_OBS_TO_SPRITE];
u8	remove_props[CMP_MAP_WIDTH][CMP_MAP_HEIGHT];
u8  overlay_order[MAX_OVERLAYS], b[MAX_OVERLAYS/2+1];
int	currently_animated[MAX_ANIMATIONS];
u32 exit_flags_offset;
bool tunnel_toggle;
// Pointer to the message ID list of the currently allowed rooms
u32 authorized_ptr;
s16	gl_off_x = 0, gl_off_y  = 0;
char panel_message[256];
u32 next_timed_event_ptr = TIMED_EVENTS_INIT;
u8*	iff_image;

// Fatigue bar base sprite colours (4_4_4_4 GRAB)
u16 fatigue_colour[8] = {0x29F0, 0x4BF0, 0x29F0, 0x06F0, 
	0x16F1, GRAB_TRANSPARENT_COLOUR, GRAB_TRANSPARENT_COLOUR, GRAB_TRANSPARENT_COLOUR};

// These are the offsets to the solitary cells doors for each prisoner
// We use them to make sure the doors are closed after leaving a prisoner in
u32 solitary_cells_door_offset[NB_NATIONS][2] = { {0x3473, 0x34C1}, {0x3FD1, 0x3F9F}, 
												  {0x92A1, 0x3FA9}, {0x92C3, 0x3FAD} };

// Bummer! The way they setup their animation overlays and the way I 
// do it to be more efficient means I need to define a custom table
// to find out animations that loop
/*
ROM:000089EA animation_data: dc.l walk_ani           ; DATA XREF: display_sprites+AEo
ROM:000089EA                                         ; #00
ROM:000089EE                 dc.l run_ani            ; #04
ROM:000089F2                 dc.l emerge_ani         ; #08
ROM:000089F6                 dc.l kneel_ani          ; #0C
ROM:000089FA                 dc.l sleep_ani          ; #10
ROM:000089FE                 dc.l wtf_ani            ; #14
ROM:00008A02                 dc.l ger_walk_ani       ; #18
ROM:00008A06                 dc.l ger_run_ani        ; #1C
ROM:00008A0A                 dc.l fireplace_ani      ; #20
ROM:00008A0E                 dc.l door1_ani          ; #24
ROM:00008A12                 dc.l door2_ani          ; #28
ROM:00008A16                 dc.l door3_ani          ; #2C
ROM:00008A1A                 dc.l door4_ani          ; #30
ROM:00008A1E                 dc.l door5_ani          ; #34
ROM:00008A22                 dc.l door6_ani          ; #38
ROM:00008A26                 dc.l crawl_ani          ; #3C
ROM:00008A2A                 dc.l ger_crawl_ani      ; #40
ROM:00008A2E                 dc.l shot_ani           ; #44
ROM:00008A32                 dc.l ger_shot_ani       ; #48
ROM:00008A36                 dc.l ger_shoot_ani      ; #4C
ROM:00008A3A                 dc.l kneel2_ani         ; #50
ROM:00008A3E                 dc.l kneel3_ani         ; #54
ROM:00008A42                 dc.l ger_kneel_ani      ; #58
*/
u8 looping_animation[NB_ANIMATED_SPRITES] =
	{	1, 1, 0, 0, 1, 0, 1, 1, 1, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0 };

// For the outside map, because of the removable sections and the fact that
// having a removable section set for the tile does not guarantee that the 
// prop should be hidden (eg: bottom of a removable wall), we'll define a
// set of tile where props should always appear. We're actually defining 4
// prop disaplayable quadrants, deliminated by an X in the tile, with each 
// bit for a quadrant, starting with the left quadrant (lsb) and going
// clockwise
// For the time being, we're not going to use these, but just set the prop
// 'on' if nonzero, coz it's a bit of an overkill, compare to what the 
// original game did. Still this is an improvement on the original game's
// props handling, as if you dropped a prop in front of the chapel in the
// original, it would simply disappear!

u16 props_tile [0x213] = {
// Well, C doesn't have binary constants, so, for maintainability reasons, 
// we'll use fake "decimal binary" constants, which we'll just convert to
// proper bitmasks at init time, if we're ever gonna use the actual masks
	0000,1111,1111,1111,1111,1111,1111,1111,1111,1111,	// 0000
	1111,1111,1111,1111,1111,1111,1111,1111,1111,1111,	// 0500
	1111,1111,1111,1111,1111,1111,1111,1111,1111,1111,	// 0A00
	1111,1111,1111,1111,1111,1111,1111,1111,1111,1111,	// 0F00
	1111,1111,1111,1111,1111,1111,1111,1111,1111,1111,	// 1400
	1111,1111,1111,1111,1111,1111,0000,0000,0000,0000,	// 1900
	1111,0000,0000,0000,0000,1111,0000,0000,0000,0000,	// 1E00
	0000,0000,0000,0000,0000,0000,0000,0000,0000,0000,	// 2300
	1111,0000,1111,0000,0000,0000,1111,1111,1111,1111,	// 2800
	1111,1111,1111,1111,1111,1111,0000,0000,0000,0000,	// 2D00
	1111,1111,1111,1111,1111,0000,0000,0000,1111,1111,	// 3200
	1111,1111,1111,1111,1111,0000,1111,1111,1111,1111,	// 3700
	0000,0000,0000,0000,0000,0000,0000,0000,0000,0000,	// 3C00
	0000,0000,0000,0000,0000,0000,0000,0000,0000,0000,	// 4100
	1111,1111,1111,1111,1111,1111,1111,1111,0000,0000,	// 4600
	0000,0000,0000,0000,1111,1111,0000,0000,0000,1111,	// 4B00
	0000,0000,1111,1111,1111,1111,0000,0000,1111,0000,	// 5000
	0011,0110,0000,0000,0000,0000,1111,0000,0000,0000,	// 5500
	0000,0000,0000,0000,1111,1111,1111,0000,0000,0000,	// 5A00
	0000,0000,0000,0000,1111,1111,0000,1111,0000,1111,	// 5F00
	0000,0000,0000,0110,0000,1111,0000,1111,0000,0000,	// 6400
	1111,1111,1111,0000,0000,1111,0000,0000,1111,0110,	// 6900
	0000,0000,0000,0011,1111,0000,0000,0000,0000,0000,	// 6E00
	1111,1111,1111,0000,0000,1111,1111,0000,0000,0000,	// 7300
	0000,0000,1111,1111,0000,0000,0000,0000,0000,0000,	// 7800
	0000,0000,0000,0000,0000,0000,0000,0000,0000,0000,	// 7D00
	0000,0000,0000,0000,0000,0000,0000,0000,0000,0000,	// 8200
	0000,0000,0000,0000,0000,0000,0000,0000,0000,0000,	// 8700
	0000,0000,0000,0000,0000,0000,0000,0000,1111,0000,	// 8C00
	0000,0000,0000,0000,0000,0000,0000,0000,1111,0000,	// 9100
	0000,0000,0000,0000,0000,0000,0000,0000,0000,0000,	// 9600
	0000,0000,0000,0000,0000,0000,0000,0000,0110,0000,	// 9B00
	0000,0000,0000,0000,0000,0000,0000,0000,0000,0000,	// A000
	0000,0011,0011,0011,0011,0110,0000,0000,0000,0000,	// A500
	0100,0011,0000,0100,0011,0110,0000,0000,0000,1111,	// AA00
	0110,0000,0000,0000,0000,0000,0000,0000,1111,0000,	// AF00
	0000,1111,0000,0000,0000,0000,0000,0000,0000,0000,	// B400
	1111,1111,0000,0000,0000,0000,0000,0000,0000,0000,	// B900
	1111,0000,0000,1111,1111,1111,1111,1111,1111,1111,	// BE00
	1111,0000,0000,0000,0000,1111,1111,0000,0000,0000,	// C300
	1111,1111,0000,0000,0000,1111,0000,0000,0000,0000,	// C800
	1111,1111,1111,1111,0000,0000,0000,0000,0000,1111,	// CD00
	0000,1111,0000,0000,1111,0000,0000,0000,0000,0000,	// D200
	0000,0000,1111,1111,0000,0000,0000,0000,0000,0000,	// D700
	0000,0000,0000,1111,0000,1111,1111,0000,0000,0000,	// DC00
	1111,0000,0000,0000,1111,1111,1111,0000,1111,1111,	// E100
	1111,0000,1111,0000,1111,1111,1111,1111,1111,1111,	// E600
	1111,1111,0000,1111,0000,0000,1111,1111,1111,1111,	// EB00
	0000,1111,1111,1111,1111,1111,1111,1111,1111,1111,	// tunnels, line 1
	1111,1111,1111,1111,1111,1111,1111,1111,1111,1111,	// tunnels, line 2
	1111,1111,1111,1111,1111,1111,1111,1111,1111,1111,	// tunnels, line 3
	1111,1111,1111,1111,1111,1111,1111,1111,1111,1111,	// tunnels, line 4
	1111,1111,1111,1111,1111,1111,1111,1111,1111,1111,	// tunnels, line 5
	1111};

void load_all_files()
{
	size_t read;
	u32 i;
	int compressed_loader = 0;

	// We need a little padding of the loader to keep the offsets happy
	fsize[LOADER] += LOADER_PADDING;

	for (i=0; i<NB_FILES; i++)
	{
		if ( (fbuffer[i] = (u8*) aligned_malloc(fsize[i], 16)) == NULL)
		{
			perr("Could not allocate buffers\n");
			ERR_EXIT;
		}

		if (i==LOADER)
		{
			fbuffer[LOADER] += LOADER_PADDING;
			fsize[LOADER] -= LOADER_PADDING;
		}

		if ((fd = fopen (fname[i], "rb")) == NULL)
		{
			if (opt_verbose)
				perror ("fopen()");
			perr("Can't find file '%s'\n", fname[i]);

			/* Take care of the compressed loader if present */
			if (i == LOADER)
			{
				// Uncompressed loader was not found
				// Maybe there's a compressed one?
				perr("  Trying to use compressed loader '%s' instead\n",ALT_LOADER);
				if ((fd = fopen (ALT_LOADER, "rb")) == NULL)
				{
					print("  '%s' not found.\n", ALT_LOADER);
					ERR_EXIT;
				}
				// OK, file was found - let's allocated the compressed data buffer
				if ((mbuffer = (u8*) aligned_malloc(ALT_LOADER_SIZE, 16)) == NULL)
				{
					perr("Could not allocate source buffer for uncompress\n");
					ERR_EXIT;
				}
				if (opt_verbose)
					print("Reading file '%s'...\n", ALT_LOADER);
				read = fread (mbuffer, 1, ALT_LOADER_SIZE, fd);
				if (read != ALT_LOADER_SIZE)
				{
					if (opt_verbose)
						perror ("fread()");
					perr("'%s': Unexpected file size or read error\n", ALT_LOADER);
					ERR_EXIT;
				}
				compressed_loader = 1;

				perr("  Uncompressing...\n");
				if (uncompress(fsize[LOADER]))
				{
					perr("Decompression error\n");
					ERR_EXIT;
				}
				perr("  OK. Now saving file as '%s'\n",fname[LOADER]);
				if ((fd = fopen (fname[LOADER], "wb")) == NULL)
				{
					if (opt_verbose)
						perror ("fopen()");
					perr("Can't create file '%s'\n", fname[LOADER]);
					ERR_EXIT;
				}
				
				// Write file
				if (opt_verbose)
						print("Writing file '%s'...\n", fname[LOADER]);
				read = fwrite (fbuffer[LOADER], 1, fsize[LOADER], fd);
				if (read != fsize[LOADER])
				{
					if (opt_verbose)
						perror ("fwrite()");
					perr("'%s': Unexpected file size or write error\n", fname[LOADER]);
					ERR_EXIT;
				}				
			}
			else 
				ERR_EXIT;
		}
	
		// Read file (except in the case of a compressed loader)
		if (!((i == LOADER) && (compressed_loader)))
		{
			if (opt_verbose)
				print("Reading file '%s'...\n", fname[i]);
			read = fread (fbuffer[i], 1, fsize[i], fd);
			if (read != fsize[i])
			{
				if (opt_verbose)
					perror ("fread()");
				perr("'%s': Unexpected file size or read error\n", fname[i]);
				ERR_EXIT;
			}
		}

		fclose (fd);
		fd = NULL;
	}

	// OK, now we can reset our LOADER's start address
	fbuffer[LOADER] -= LOADER_PADDING;
}


// Get some properties (max/min/...) according to file data
void get_properties()
{
	u16 room_index;
	u32 ignore = 0;
	u32 offset;
	u8  i,j;

	// Get the number of rooms
	for (room_index=0; ;room_index++)
	{	
		// Read the offset
		offset = readlong((u8*)fbuffer[ROOMS], OFFSETS_START+4*room_index);
		if (offset == 0xFFFFFFFF)
		{	// For some reason there is a break in the middle
			ignore++;
			if (ignore > FFs_TO_IGNORE)
				break;
		}
	}
	nb_rooms = room_index;
//	print("nb_rooms = %X\n", nb_rooms);

	// A backdrop cell is exactly 256 bytes (32*16*4bits)
	nb_cells = fsize[CELLS] / 0x100;
	cell_texid = malloc(sizeof(GLuint) * nb_cells);
	GLCHK(glGenTextures(nb_cells, cell_texid));
//	print("nb_cells = %X\n", nb_cells);

//	nb_sprites = readword(fbuffer[SPRITES],0) + 1;
	if (readword(fbuffer[SPRITES],0) != (NB_STANDARD_SPRITES-1))
	{
		perr("Unexpected number of sprites\n");
		ERR_EXIT;
	}

	sprite_texid = malloc(sizeof(GLuint) * NB_SPRITES);
	GLCHK(glGenTextures(NB_SPRITES, sprite_texid));
//	print("nb_sprites = %X\n", NB_SPRITES);

	chars_texid = malloc(sizeof(GLuint) * NB_PANEL_CHARS);
	GLCHK(glGenTextures(NB_PANEL_CHARS, chars_texid));
//	print("nb_chars = %X\n", NB_PANEL_CHARS);

	nb_objects = readword(fbuffer[OBJECTS],0) + 1;
//	print("nb_objects = %X\n", nb_objects);
	for (i=0; i<NB_OBS_TO_SPRITE; i++)
		obs_to_sprite[i] = readbyte(fbuffer[LOADER],OBS_TO_SPRITE_START+i);

	// This will be needed to hide the pickable objects on the outside map
	// if the removable walls are set
	for (i=0; i<CMP_MAP_WIDTH; i++)
		for (j=0; j<CMP_MAP_HEIGHT; j++)
			remove_props[i][j] = 0;

	// We need to initialize the current route pos offset (0x0E)
	for (i=0; i<NB_GUARDS; i++)
		// simply copy over the route start offset (0x06)
		writelong(fbuffer[GUARDS], i*MENDAT_ITEM_SIZE+0x0E,
			readlong(fbuffer[GUARDS], i*MENDAT_ITEM_SIZE+0x06));

	// Set our textures for panel, zoom and iff images
	glGenTextures( 1, &panel1_texid );
	glGenTextures( 1, &panel2_texid );
	glGenTextures( 1, &render_texid );
	glGenTextures( 1, &picture_texid );

	// The panel textures have already been loaded, so we map them
	glBindTexture(GL_TEXTURE_2D, panel1_texid);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, PANEL_BASE1_W, PANEL_BASE_H, 0,
		GL_RGB, GL_UNSIGNED_BYTE, fbuffer[PANEL_BASE1]);

	glBindTexture(GL_TEXTURE_2D, panel2_texid);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, PANEL_BASE2_W, PANEL_BASE_H, 0,
		GL_RGB, GL_UNSIGNED_BYTE, fbuffer[PANEL_BASE2]);
}

// init_variables
void init_variables()
{
	// start time
	hours_digit_h = 0;
	hours_digit_l = 9;
	minutes_digit_h = 3;
	minutes_digit_l = 0;

	minutes_digit_h = 5;
	minutes_digit_l = 4;

	next_timed_event_ptr = TIMED_EVENTS_INIT;

	// First rollcall
	hours_digit_h = 0;
	hours_digit_l = 5;
	minutes_digit_h = 5;
	minutes_digit_l = 4;

	next_timed_event_ptr = 0x2BE8;

	// Current event is #3 (confined to quarters)
	authorized_ptr = readlong(fbuffer[LOADER],AUTHORIZED_BASE+4*3);
}


// Convert an Amiga 12 bit RGB colour palette to 16 bit GRAB
void to_16bit_palette(u8 palette_index, u8 transparent_index, u8 io_file)
{
	u32 i;
	u16 rgb, grab;

	int palette_start = palette_index * 0x20;

	// Read the palette
	if (opt_verbose)
		print("Using Amiga Palette index: %d\n", palette_index);


	for (i=0; i<16; i++)		// 16 colours
	{
		rgb = readword(fbuffer[io_file], palette_start + 2*i);
		if (opt_verbose)
		{
			print(" %03X", rgb); 
			if (i==7)
				print("\n");
		}
		// OK, we need to convert our rgb to grab
		// 1) Leave the R&B values as they are
		grab = rgb & 0x0F0F;
		// 2) Set Alpha to no transparency
		if (i != transparent_index)
			grab |= 0x00F0;
		// 3) Set Green
		grab |= (rgb << 8) & 0xF000;
		// 4) Write in the palette
		aPalette[i] = grab;
	}
	if (opt_verbose)
		print("\n\n");
}


// Convert a <bpp> bits line-interleaved source to 16 bit RGBA (GRAB) destination
// bpp parameter = bits per pixels, a.k.a. colour depth
// Assumes w to be a multiple of 8, and bpp < 8 as well
void line_interleaved_to_wGRAB(u8* source, u8* dest, u16 w, u16 h, u8 bpp)
{
	u8 colour_index;
	u32 i,j,l,pos;
	int k;
	u32 wb;
	u8 line_byte[8];

	// the width of interest to us is the one in bytes.
	wb = w/8;

	// We'll write sequentially to the destination
	pos = 0;
	for (i=0; i<h; i++)
	{	// h lines to process
		for (j=0; j<wb; j++)
		{	// wb bytes per line
			for (k=0; k<bpp; k++)
				// Read one byte from each of the <bpp> lines (starting from max y for openGL)
				line_byte[bpp-k-1] = readbyte(source, bpp*(wb*i) + k*wb + j);
			// Write 8 RGBA values
			for (k=0; k<8; k++)
			{
				colour_index = 0;
				// Get the palette colour index and rotate the line bytes
				for (l=0; l<bpp; l++)
				{
					colour_index <<= 1;
					colour_index |= (line_byte[l]&0x80)?1:0;
					line_byte[l] <<= 1;
				}
				// Alpha is always set to 0
				writeword(dest, pos, aPalette[colour_index]);
				pos += 2;
			}
		}
	}
}


// Convert a 1+4 bits (mask+colour) bitplane source
// to 16 bit RGBA (GRAB) destination
void bitplane_to_wGRAB(u8* source, u8* dest, u16 w, u16 ext_w, u16 h)
{
	u16 bitplane_size;
	u8  colour_index;
	u16 i,j,k,wb,ext_wb;
	u8  bitplane_byte[5], mask_byte;
	u32 pos = 0;

	wb = w/8;	// width in bytes
	ext_wb = ext_w/8;
	bitplane_size = h*wb; 

	for (i=0; i<bitplane_size; i++)	
	{
		// Read one byte from each bitplane...
		for (j=0; j<5; j++)
			// bitplanes are in reverse order for colour
			// and so is openGL's coordinate system for y
			bitplane_byte[4-j] = readbyte(source, i + (j*bitplane_size) );

		// For clarity
		mask_byte = bitplane_byte[4];

		// Write 8 RGBA words 
		for (k=0; k<8; k++)
		{

			colour_index = 0;
			// Get the palette colour index and rotate the bitplane bytes
			for (j=0; j<4; j++)
			{
				colour_index <<= 1;
				colour_index |= (bitplane_byte[j]&0x80)?1:0;
				bitplane_byte[j] <<= 1;
			}
			// Alpha is in 3rd position, and needs to be cleared on empty mask
			writeword(dest, pos, aPalette[colour_index] & ((mask_byte&0x80)?0xFFFF:0xFF0F));
			pos += 2;
			// Takes care of padding in width
			while ((u16)(pos%(2*ext_w))>=(2*w))
				pos +=2;	// calloced to zero, so just skim
			mask_byte <<=1;
		}
	}
}


// Converts the room cells to RGB data we can handle
void cells_to_wGRAB(u8* source, u8* dest)
{
	u32 i;

	// Convert each 32x16x4bit (=256 bytes) cell to RGB
	for (i=0; i<nb_cells; i++)
	{
		line_interleaved_to_wGRAB(source + (256*i), dest+(2*RGBA_SIZE*256*i), 32, 16, 4);
		GLCHK(glBindTexture(GL_TEXTURE_2D, cell_texid[i]));
		GLCHK(glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 32, 16, 0, GL_RGBA, GL_UNSIGNED_SHORT_4_4_4_4_REV, 
			((u8*)rgbCells) + i*2*RGBA_SIZE*0x100));
	}

}

// Create the sprites for the panel text characters
void init_panel_chars()
{
	u8 c, y, x, m, b;

	for (c = 0; c < NB_PANEL_CHARS; c++)
	{
		// first line is transparent
		for (x=0; x<PANEL_CHARS_W; x++)
			writeword((u8*)panel_chars[c],2*x,GRAB_TRANSPARENT_COLOUR);

		// 6 lines of 1 byte each
		for (y = 1; y < PANEL_CHARS_H+1; y++)
		{
			b = readbyte(fbuffer[SPRITES_PANEL], PANEL_CHARS_OFFSET + c*PANEL_CHARS_H + y-1);
			for (x=0,m=0x80; m!=0; x++,m>>=1)
			{	// each line is one byte exactly
				if (b&m)
					// The bits are inverted => anything set is to be transparent
					writeword((u8*)panel_chars[c], y*16 + 2*x, 0x0000);
				else
					writeword((u8*)panel_chars[c], y*16 + 2*x, 
					// Respect the nice incremental colour graduation
						PANEL_CHARS_GRAB_BASE + (y-1)*PANEL_CHARS_GRAB_INCR);					
			}
		}
		// last line is transparent too
		for (x=0; x<PANEL_CHARS_W; x++)
			writeword((u8*)panel_chars[c],y*16 + 2*x,GRAB_TRANSPARENT_COLOUR);

		// Convert to textures
		GLCHK(glBindTexture(GL_TEXTURE_2D, chars_texid[c]));
		GLCHK(glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, PANEL_CHARS_W, 8, 0, GL_RGBA, 
			GL_UNSIGNED_SHORT_4_4_4_4_REV, (u8*)panel_chars[c]));
	}

}

// Get properties for panel sprites
s_nonstandard nonstandard(u16 sprite_index)
{
	s_nonstandard sp;

	// Panel flags (32x16)
	if (sprite_index < NB_STANDARD_SPRITES + NB_PANEL_FLAGS)
	{
		sp.w =  32;
		sp.base = NB_STANDARD_SPRITES;
		sp.offset = PANEL_FLAGS_OFFSET;
	}
	// Panel's prisoner faces (16x16)
	else if (sprite_index < NB_STANDARD_SPRITES + NB_PANEL_FLAGS + NB_PANEL_FACES)
	{
		sp.w = 16;
		sp.base = NB_STANDARD_SPRITES + NB_PANEL_FLAGS;
		sp.offset = PANEL_FACES_OFFSET;
	}
	// Clock digits (8x16)
	else if (sprite_index < NB_STANDARD_SPRITES + NB_PANEL_FLAGS + NB_PANEL_FACES + 
		NB_PANEL_CLOCK_DIGITS)
	{
		sp.w = 8;
		sp.base = NB_STANDARD_SPRITES + NB_PANEL_FLAGS + NB_PANEL_FACES;
		sp.offset = PANEL_CLOCK_DIGITS_OFF;
	}
	// inventory props + status (32x16)
	else if (sprite_index < NB_STANDARD_SPRITES + NB_PANEL_FLAGS + NB_PANEL_FACES + 
		NB_PANEL_CLOCK_DIGITS + NB_PANEL_ITEMS)
	{
		sp.w = 32;
		sp.base = NB_STANDARD_SPRITES + NB_PANEL_FLAGS + NB_PANEL_FACES + NB_PANEL_CLOCK_DIGITS;
		sp.offset = PANEL_ITEMS_OFFSET;
	}
	// Never gonna happen... or will it???
	else
	{
		sp.w = 0;
		sp.base = 0;
		sp.offset = 0;
	}

	return sp;
}


// Initialize the sprite array
void init_sprites()
{
	u32 index = 2;	// We need to ignore the first word (nb of sprites)
	u16 sprite_index = 0;
	u16 sprite_w;	// width, in words
	u32 sprite_address;


	// Allocate the sprites and overlay arrays
	sprite = aligned_malloc(NB_SPRITES * sizeof(s_sprite), 16);
	overlay = aligned_malloc(MAX_OVERLAYS * sizeof(s_overlay), 16);

	// First thing we do is populate the standard sprite offsets at the beginning of the table
	sprite_address = index + 4* (readword(fbuffer[SPRITES],0) + 1);
	for (sprite_index=0; sprite_index<NB_STANDARD_SPRITES; sprite_index++)
	{
		sprite_address += readlong(fbuffer[SPRITES],index);
		writelong(fbuffer[SPRITES],index,sprite_address);
		index+=4;
	}
	// Each sprite is prefixed by 2 words (x size in words, y size in pixels)
	// and one longword (size of one bitplane, in bytes)
	// NB: MSb on x size will be set if sprite is animated
	for (sprite_index=0; sprite_index<NB_STANDARD_SPRITES; sprite_index++)
	{
		sprite_address = readlong(fbuffer[SPRITES],2+4*sprite_index);
//		print("sprite[%X] address = %08X\n", sprite_index, sprite_address);
		// x size is given in words
		sprite_w = readword(fbuffer[SPRITES],sprite_address);
		// w is fine as it's either 2^4 or 2^5
		sprite[sprite_index].w = 16*(sprite_w & 0x7FFF);
		sprite[sprite_index].corrected_w = powerize(sprite[sprite_index].w);
		// h will be problematic as pspgl wants a power of 2
		sprite[sprite_index].h = readword(fbuffer[SPRITES],sprite_address+2);
		sprite[sprite_index].corrected_h = powerize(sprite[sprite_index].h);
		
		// According to MSb of sprite_w (=no_mask), we'll need to use RGBA or RGB
//		sprite[sprite_index].type = (sprite_w & 0x8000)?GL_RGB:GL_RGBA;
		// There's an offset to position the sprite depending on the mask's presence
		sprite[sprite_index].x_offset = (sprite_w & 0x8000)?16:1;
		sprite[sprite_index].data = aligned_malloc( RGBA_SIZE * 
			sprite[sprite_index].corrected_w * sprite[sprite_index].corrected_h, 16);
//		print("  w,h = %0X, %0X\n", sprite[sprite_index].w , sprite[sprite_index].h);
	}

	// Manual correction for the dying prisoners last animations, as these animations
	// are wider, and we use bottom-left instead of bottom right as origin
	sprite[0xab].x_offset = -16;
	sprite[0xac].x_offset = -16;
	sprite[0xaf].x_offset = -16;
	sprite[0xb0].x_offset = -16;

	// We add the panel (nonstandard) sprites at the end of our exitsing sprite array
	for (sprite_index=NB_STANDARD_SPRITES; sprite_index<NB_SPRITES-1; sprite_index++)
	{
		sprite[sprite_index].w = nonstandard(sprite_index).w;
		sprite[sprite_index].corrected_w = nonstandard(sprite_index).w;
		sprite[sprite_index].h = 16;
		sprite[sprite_index].corrected_h = 16;
		sprite[sprite_index].x_offset = 1;
		sprite[sprite_index].data = aligned_malloc( RGBA_SIZE * 
			sprite[sprite_index].w * sprite[sprite_index].h, 16);
	}

	// We define the last nonstandard as sprite base for the fatigue bar
	// Dammit!!! - the PSP can't handle 1 pixel wide textures properly, so we need
	// to make it 8 pixels large
	sprite[sprite_index].w = 8;
	sprite[sprite_index].corrected_w = 8;
	sprite[sprite_index].h = 8;
	sprite[sprite_index].corrected_h = 8;
	sprite[sprite_index].x_offset = 1;
	sprite[sprite_index].data = aligned_malloc( RGBA_SIZE * 
			sprite[sprite_index].w * sprite[sprite_index].h, 16);

	// We use a different sprite array for status message chars
	init_panel_chars();
}




// Converts the sprites to 16 bit GRAB data we can handle
void sprites_to_wGRAB()
{
	u16 sprite_index;
	u32 sprite_address;
	u8* sbuffer;
	int no_mask = 0;
	int x,y;

	for (sprite_index=0; sprite_index<NB_SPRITES-1; sprite_index++)
	{
		// Standard sprites (from SPRITES.SPR)
		if (sprite_index < NB_STANDARD_SPRITES)
		{
			// Get the base in the original Colditz sprite file
			sprite_address = readlong(fbuffer[SPRITES],2+4*sprite_index);

			// if MSb is set, we have 4 bitplanes instead of 5
			no_mask = readword(fbuffer[SPRITES],sprite_address) & 0x8000;

			// Populate the z_offset, which we'll use later on to decide the z position
			// of the overlays. We substract h because we use top left corner rather than
			// bottom right as in original game (speed up computations for later)
			sprite[sprite_index].z_offset = readword(fbuffer[SPRITES],sprite_address+4) - 
				sprite[sprite_index].h;
			
			// Compute the source address
			sbuffer = fbuffer[SPRITES] + sprite_address + 8; 
		}
		// Panel (nonstandard sprites)
		else 
		{
			if (sprite_index == NB_STANDARD_SPRITES)
			// we're getting into panel overlays => switch to the panel palette
				to_16bit_palette(0, 1, SPRITES_PANEL);
			sbuffer = fbuffer[SPRITES_PANEL] + nonstandard(sprite_index).offset + 
				8*sprite[sprite_index].w*(sprite_index-nonstandard(sprite_index).base);
			no_mask = 1;
		}

		if (no_mask)
		{
			// Bitplanes that have no mask are line-interleaved, like cells
			line_interleaved_to_wGRAB(sbuffer, sprite[sprite_index].data, 
				sprite[sprite_index].w, sprite[sprite_index].h, 4);
			// A sprite with no mask should always display under anything else
			sprite[sprite_index].z_offset = MIN_Z;
		}
		else
			// bitplane interleaved with mask
			bitplane_to_wGRAB(sbuffer, sprite[sprite_index].data, sprite[sprite_index].w,
				sprite[sprite_index].corrected_w, sprite[sprite_index].h);

		// Now that we have data in a GL readable format, let's texturize it!
		GLCHK(glBindTexture(GL_TEXTURE_2D, sprite_texid[sprite_index]));
		GLCHK(glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, sprite[sprite_index].corrected_w, 
			sprite[sprite_index].corrected_h, 0, GL_RGBA, GL_UNSIGNED_SHORT_4_4_4_4_REV,
			sprite[sprite_index].data));
	}

	// The last sprite (fatigue) is initialized manually
	for (y=0; y<8; y++)
		for (x=0; x<8; x++)
			writeword(sprite[sprite_index].data, 16*y+2*x, fatigue_colour[y]);

	GLCHK(glBindTexture(GL_TEXTURE_2D, sprite_texid[sprite_index]));
	GLCHK(glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, sprite[sprite_index].w, 
		sprite[sprite_index].h, 0, GL_RGBA, GL_UNSIGNED_SHORT_4_4_4_4_REV,
		sprite[sprite_index].data));

}

// Returns the last frame of an animation (usually the centered position)
int get_stop_animation_sid(u8 index, bool is_guybrush)
{
	u8 frame;
	int sid;
	u32 ani_base;
	s16 dir;
	s_animation* p_ani;

	// Pointer to the animation structure
	p_ani = is_guybrush?&guybrush[index].animation:&animations[index];
	// Our index will tell us which animation sequence we use (walk, run, kneel, etc.)
	ani_base = readlong(fbuffer[LOADER], ANIMATION_OFFSET_BASE + 4*p_ani->index);
	// Guybrushes animations need to handle a direction, others do not
	dir = is_guybrush?guybrush[index].direction:0;
	// With the direction and animation base, we can get to the base SID of the ani sequence
	sid = readbyte(fbuffer[LOADER], ani_base + 0x0A + dir);
	// find out the index of the last animation frame
	frame = readbyte(fbuffer[LOADER], ani_base) - 1;
	sid += readbyte(fbuffer[LOADER], readlong(fbuffer[LOADER], ani_base + 0x06) + frame);
	return sid;
}

// Returns an animation frame
// index is either the animation[] array index (standard overlays) or the guybrush[] array index
int get_animation_sid(u8 index, bool is_guybrush)
{
	u8 frame, sid_increment;
	int sid;
	u32 ani_base;
	s32 nb_frames;
	s16 dir;
	s_animation* p_ani;

	// Pointer to the animation structure
	p_ani = is_guybrush?&guybrush[index].animation:&animations[index];
	// read the base sid
	ani_base = readlong(fbuffer[LOADER], ANIMATION_OFFSET_BASE + 4*p_ani->index);
	dir = is_guybrush?guybrush[index].direction:0;
	sid = readbyte(fbuffer[LOADER], ani_base + 0x0A + dir);
//	printf("framecount = %d\n", p_ani->framecount);
	nb_frames = readbyte(fbuffer[LOADER], ani_base);	// offset 0 is nb frames max
//	printf("sid base = %x, nb_frames = %d\n", sid, nb_frames);
//	printf("ani_index = %d\n", p_ani->index);


	if ( (!(looping_animation[p_ani->index])) && (p_ani->framecount >= nb_frames) )
	{	// end of one shot animations
		frame = nb_frames - 1;	// 0 indexed
		if (p_ani->end_of_ani_function != NULL)
		{	// execute the end of animation function (toggle exit)
			p_ani->end_of_ani_function(p_ani->end_of_ani_parameter);
			p_ani->end_of_ani_function = NULL;
		}
	}
	else
	{	// one shot (non end) or loop
		frame = p_ani->framecount % nb_frames;
	}
//	printf("nb_frames = %d, framecount = %d\n", nb_frames, animations[index].framecount);
	sid_increment = readbyte(fbuffer[LOADER], 
		readlong(fbuffer[LOADER], ani_base + 0x06) + frame);
//	printf("frame = %d, increment = %x\n", frame, sid_increment);
	if (sid_increment == 0xFF)
	{	// play a sound 
		// sound = yada +1;
		sid_increment = readbyte(fbuffer[LOADER], 
			readlong(fbuffer[LOADER], ani_base + 0x06) + frame + 2);
		p_ani->framecount += 2;
	}
	if (sid_increment & 0x80)
		sid = REMOVE_ANIMATION_SID;
	else
		sid += sid_increment;
//	if (sid != -1)
//	if (animations[index].index == GER_WALK_ANI)
//		printf("sid_increment = %x, framecount = %d\n", sid_increment, animations[index].framecount);
	return sid;
}


// Populates the tile overlays, if we are on Colditz Rooms Map
void crm_set_overlays(s16 x, s16 y, u16 current_tile, u32 tile_offset, u16 room_x)
{
	u16 tile2_data;
	u16 i;
	s16 sx, sy;
	u16 sid;
	int animated_sid;	// sprite index

	animated_sid = 0;	// 0 is a valid sid, but not for overlays, so we 
						// can use it as "false" flag
	// read current tile
	for (i=0; i<(12*NB_SPECIAL_TILES); i+=12)
	{
		if (readword(fbuffer[LOADER], SPECIAL_TILES_START+i) != current_tile)
			continue;

		if (current_tile == FIREPLACE_TILE)
		{	// The fireplace is the only animated overlay we need to handle beside exits
			if (init_animations)
			{	// Setup animated tiles, if any
				currently_animated[0] = nb_animations;
				animations[nb_animations].index = FIREPLACE_ANI;
				animations[nb_animations].framecount = 0;
				animations[nb_animations].end_of_ani_function = NULL;
				safe_nb_animations_increment();
			}
			// Even if there's more than one fireplace per room, their sids will match
			// so we can use currently_animated[0] for all of them. Other room animations
			// will go at currently_animated[1+]
			animated_sid = get_animation_sid(currently_animated[0], false);
		}

		sx = readword(fbuffer[LOADER], SPECIAL_TILES_START+i+8);
		if (opt_debug)
			print("  match: %04X, direction: %04X\n", current_tile, sx);
		if (i >= (12*(NB_SPECIAL_TILES-4)))
		// The four last special tiles are exits. We need to check is they are open
		{
			// Get the exit data (same tile if tunnel, 2 rows down if door)
			tile2_data = readword(fbuffer[ROOMS], tile_offset + 
			// careful that ? take precedence over +, so if you don't put the
			// whole ?: increment in parenthesis, you have a problem
				((i==(12*(NB_SPECIAL_TILES-1)))?0:(4*room_x)));

			// Validity check
			if (!(tile2_data & 0x000F))
				// This is how I know that we can use the exit # as ani index
				// and leave index 0 for the fireplace ani
				print("set_overlays: Integrity check failure on exit tile\n");

			// The door might be in animation
			if ((currently_animated[tile2_data & 0x000F] >= 0) && 
				(currently_animated[tile2_data & 0x000F] < 0x70))
				// the trick of using the currently_animated table to find the door 
				// direction works because the exit sids are always > 0x70
				animated_sid = get_animation_sid(currently_animated[tile2_data & 0x000F], false);
			else
				currently_animated[tile2_data & 0x000F] = readword(fbuffer[LOADER], SPECIAL_TILES_START+i+4);
	
			// if the tile is an exit and the exit is open
			if (tile2_data & 0x0010)
			{	// door open
				if (opt_debug)
					print("    exit open: ignoring overlay\n");
				// The second check on exits is always an FA00, thus we can safely
				break;
			}
		}
			 
		if (sx < 0)
			tile2_data = readword(fbuffer[ROOMS], tile_offset-2) & 0xFF80;
		else
			tile2_data = readword(fbuffer[ROOMS], tile_offset+2) & 0xFF80;
		// ignore if special tile that follows is matched
		if (readword(fbuffer[LOADER], SPECIAL_TILES_START+i+2) == tile2_data)
		{
			if (opt_debug)
				print("    ignored as %04X matches\n", tile2_data);
			continue;
		}

		if (animated_sid == REMOVE_ANIMATION_SID)
		// ignore
			continue;

		sid = (animated_sid)?animated_sid:readword(fbuffer[LOADER], SPECIAL_TILES_START+i+4);
		overlay[overlay_index].sid = sid;

		if (opt_debug)
			print("    overlay as %04X != %04X => %X\n", tile2_data, 
				readword(fbuffer[LOADER], SPECIAL_TILES_START+i+2), sid);
		sy = readword(fbuffer[LOADER], SPECIAL_TILES_START+i+6);
		if (opt_debug)
			print("    sx: %04X, sy: %04X\n", sx, sy);

		overlay[overlay_index].x = x + sx - sprite[sid].w + sprite[sid].x_offset;
		overlay[overlay_index].y = y + sy - sprite[sid].h + 1;

		// No need to bother if the overlay is offscreen (with generous margins)
		if ((overlay[overlay_index].x < -64) || (overlay[overlay_index].x > (PSP_SCR_WIDTH+64)))
			continue;
		if ((overlay[overlay_index].y < -64) || (overlay[overlay_index].y > (PSP_SCR_HEIGHT+64)))
			continue;

		// Update the z index according to our current y pos
		if (sprite[sid].z_offset == MIN_Z)
			overlay[overlay_index].z = MIN_Z;
		else
			// PSP_SCR_HEIGHT/2 is our actual prisoner position on screen
			overlay[overlay_index].z = overlay[overlay_index].y - sprite[sid].z_offset 
				- PSP_SCR_HEIGHT/2 + NORTHWARD_HO - 2; 
//		printf("z[%x] = %d\n", sid, overlay[overlay_index].z); 
		safe_overlay_index_increment();
		// No point in looking for overlays any further if we met our match 
		// UNLESS this is a double bed overlay, in which case the same tile
		// needs to be checked for a double match (in both in +x and -x)
		if (current_tile != 0xEF00)
			break;
	}
}


// Populates the tile overlays, if we are on the CoMPressed map
void cmp_set_overlays()
{
	u16 i;
	u32 bitset, offset;
	short sx, sy;
	u16 tile_x, tile_y;
	u8	exit_nr;
	int sid;	// sprite index
	u16 room_x = CMP_MAP_WIDTH;
	u8 io_file = ROOMS;	// We'll need to switch to TUNNEL_IO midway through

	for (i=0; i<(4*OUTSIDE_OVL_NB+4*TUNNEL_OVL_NB); i+=4)
	{
		if (i==(4*OUTSIDE_OVL_NB))
			io_file = TUNNEL_IO;	// switch IO file

		// The relevant bit (byte[0]) from the bitmask must be set 
		bitset = 1 << (readbyte(fbuffer[LOADER], OUTSIDE_OVL_BASE+i));
		if (!(rem_bitmask & bitset))
			continue;
		// But only if the bit identified by byte[1] is not set
		bitset = 1 << (readbyte(fbuffer[LOADER], OUTSIDE_OVL_BASE+i+1));
		if (rem_bitmask & bitset)
			continue;

		// OK, now we know that our removable section is meant to show an exit

		// First, let's grab the base sid
		offset = readbyte(fbuffer[LOADER], OUTSIDE_OVL_BASE+i+3) << 3;
		sid = readword(fbuffer[LOADER],CMP_OVERLAYS+offset+4);
//		overlay[overlay_index].sid = sid;

		// Next read the pixel shifts on the tile
		sx = readword(fbuffer[LOADER],CMP_OVERLAYS+offset+2);
		sy = readword(fbuffer[LOADER],CMP_OVERLAYS+offset);

		// Then add the tile position, as identified in the 8 bytes data at the beginning of 
		// the Colditz Rooms Map or Tunnel_IO files,
		offset = readbyte(fbuffer[LOADER], OUTSIDE_OVL_BASE+i+2) << 3;
		// check if the exit is open. This is indicated with bit 12 of the first word
		if (readword(fbuffer[io_file],offset) & 0x1000)
			continue;

		tile_x = readword(fbuffer[io_file],offset+6);
		tile_y = readword(fbuffer[io_file],offset+4);

		sx += tile_x * 32;
		sy += tile_y * 16;

		// Don't forget the displayable area offset
		overlay[overlay_index].x = gl_off_x + sx - sprite[sid].w + sprite[sid].x_offset;
		ignore_offscreen_x(overlay_index);	// Don't bother if offscreen
		overlay[overlay_index].y = gl_off_y + sy - sprite[sid].h + 1;
		ignore_offscreen_y(overlay_index);	// Don't bother if offscreen

		// OK, now let's deal with potential door animations
		if (i<(4*OUTSIDE_OVL_NB))
		{	// we're dealing with a door overlay, possibly animated
			// Get the exit_nr (which we need for animated overlays)
			exit_nr = readexit(tile_x, tile_y);

			if ((currently_animated[exit_nr] >= 0) && (currently_animated[exit_nr] < 0x70))
			// get the current animation frame on animated overlays
				sid = get_animation_sid(currently_animated[exit_nr], false);
			else
			// if it's not animated, set the sid in the table, so we can find out
			// our type of exit later on
				currently_animated[exit_nr] = sid;
		}

		if (sid == REMOVE_ANIMATION_SID)	// ignore doors that have ended their animation cycle
			continue;

		// Now we have our definitive sid
		overlay[overlay_index].sid = sid;

		// Update the z index according to our current y pos
		if (sprite[sid].z_offset == MIN_Z)
			overlay[overlay_index].z = MIN_Z;
		else
			// PSP_SCR_HEIGHT/2 is our actual prisoner position on screen
			overlay[overlay_index].z = overlay[overlay_index].y - sprite[sid].z_offset 
				- PSP_SCR_HEIGHT/2 + NORTHWARD_HO -3; 

		safe_overlay_index_increment();
	}
}


// Read the props (pickable objects) from obs.bin
// For efficiency reasons, this is only done when switching room
void set_room_props()
{
	u16 prop_offset;

	nb_room_props = 0;
	for (prop_offset=2; prop_offset<(8*nb_objects+2); prop_offset+=8)
	{
		if (readword(fbuffer[OBJECTS],prop_offset) != current_room_index)
			continue;
			
		room_props[nb_room_props] = prop_offset; 
		nb_room_props++;
	}
}


// Set the props overlays
void set_props_overlays()
{
	u8 u;
	u32 prop_offset;
	u16 x, y;

	// reset the stand over prop
	over_prop = 0;
	over_prop_id = 0;
	for (u=0; u<nb_room_props; u++)
	{
		prop_offset = room_props[u];

		if (prop_offset == 0)
		// we might have picked the prop since last time
			continue;

		overlay[overlay_index].sid = obs_to_sprite[readword(fbuffer[OBJECTS],prop_offset+6)];

		// Man, this positioning of sprites sure is a fucking mess,
		// with weird offsets having to be manually added everywhere!
		x = readword(fbuffer[OBJECTS],prop_offset+4) - 15;
		y = readword(fbuffer[OBJECTS],prop_offset+2) - (is_outside?4:3);

		overlay[overlay_index].x = gl_off_x + x;
		ignore_offscreen_x(overlay_index);
		overlay[overlay_index].y = gl_off_y + y;
		ignore_offscreen_y(overlay_index);

		// We also take this oppportunity to check if we stand over a prop
		if ( (prisoner_x >= x-9) && (prisoner_x < x+8) && 
			 (prisoner_2y/2 >= y-9) && (prisoner_2y/2 < y+8) ) 
		{
			over_prop = u+1;	// 1 indexed
			over_prop_id = readbyte(fbuffer[OBJECTS],prop_offset+7);
			// The props message takes precedence
			set_status_message(fbuffer[LOADER] + readlong(fbuffer[LOADER], 
				PROPS_MESSAGE_BASE + 4*(over_prop_id-1)), 1, PROPS_MESSAGE_TIMEOUT);
//			printf("over_prop = %x, over_prop_id = %x\n", over_prop, over_prop_id);
		}

		// all the props should appear behind overlays, expect the ones with no mask
		// (which are always set at MIN_Z)
		overlay[overlay_index].z = MIN_Z+1;

		// Because of the removable walls we have a special case for the CMP_MAP
		if (!(is_outside && (remove_props[x/32][y/16])))
			// Don't add overlay if covered by a wall
			safe_overlay_index_increment();
	}
}

// Display a sprite or cell, using the top left corner as the origin
void display_sprite(float x1, float y1, float w, float h, GLuint texid) 
{
	float x2, y2;

	x2 = x1 + w;
	y2 = y1 + h;

	glBindTexture(GL_TEXTURE_2D, texid);

	// Don't modify pixel colour ever
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	// Do linear interpolation. Looks better, but if you zoom, you have to zoom
	// the whole colour buffer, else the sprite seams will show
//	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
//	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

	// If we don't set clamp, our tiling will show
#if defined(PSP)
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
#else
	// For some reason GL_CLAMP_TO_EDGE on Win achieves the same as GL_CLAMP on PSP
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
#endif 

	// pspGL does not implement QUADS
	glBegin(GL_TRIANGLE_FAN);

	glTexCoord2f(0.0f, 0.0f);
	glVertex3f(x1, y1, 0.0f);

	glTexCoord2f(0.0f, 1.0f);
	glVertex3f(x1, y2, 0.0f);

	glTexCoord2f(1.0f, 1.0f);
	glVertex3f(x2, y2, 0.0f);

	glTexCoord2f(1.0f, 0.0f);
	glVertex3f(x2, y1, 0.0f);

	glEnd();
}

// We need a sort to reorganize our overlays according to z
// We'll use "merge" sort (see: http://www.sorting-algorithms.com/) here,
// but we probably could have gotten away with "shell" sort
void sort_overlays(u8 a[], u8 n)
{
	u8 m,i,j,k;

	if (n < 2)
		return;

	// split in half
	m = n >> 1;

	// recursive sorts
	sort_overlays(a, m);
	sort_overlays(a+m, n-m); 

	// merge sorted sub-arrays using temp array
	// b = copy of a[1..m]
	for (i=0; i<m; i++)
		b[i] = a[i];

	i = 0; j = m; k = 0;
	while ((i < m) && (j < n))
		a[k++] = (overlay[a[j]].z < overlay[b[i]].z) ? a[j++] : b[i++];
		// => invariant: a[1..k] in final position
	while (i < m)
		a[k++] = b[i++];
		// => invariant: a[1..k] in final position
}


// Display all our overlays
void display_overlays()
{
	u8 i, j;

	// OK, first we need to reorganize our overlays according to the z position
	for (i=0; i<overlay_index; i++)
		overlay_order[i] = i;	// dummy indexes

	// Recursive "merge" sort
	sort_overlays(overlay_order, overlay_index);

/*
	for (j=0; j<overlay_index; j++)
	{
		i = overlay_order[j];
		printf("overlay[%d].sid =%x, z = %d (order: %d)\n", i, overlay[i].sid, overlay[i].z, j);
	}
*/
	for (j=0; j<overlay_index; j++)
	{
		i = overlay_order[j];
		display_sprite(overlay[i].x, overlay[i].y, sprite[overlay[i].sid].corrected_w, 
			sprite[overlay[i].sid].corrected_h, sprite_texid[overlay[i].sid]);
//		printf("ovl(%d,%d), sid = %X\n", overlay[i].x, overlay[i].y, overlay[i].sid);
	}
}


void removable_walls()
{	// Get the current removable walls mask to apply

	int tile_x, tile_y;
	u16 tile_data;
	u32 cmp_data;
	u32 tmp_bitmask;
	u8  bit_index;
	u32 dir_offset;
	int rdx, rdy;
	u8  cmp_x, cmp_y;

	rdx = prisoner_x - last_p_x;
	rdy = (prisoner_2y/2) - last_p_y; 
	// If no motion, exit
	if ((rdx == 0) && (rdy == 0))
		return;

	// Compute the tile on which we stand
	tile_y = prisoner_2y / 32;

	// Sanity checks
	if (tile_y < 0)
		tile_y = 0;
	if (tile_y >= CMP_MAP_HEIGHT)
		tile_y = CMP_MAP_HEIGHT-1;

	tile_x = prisoner_x / 32;
	if (tile_x < 0)
		tile_x = 0;
	if (tile_x >= CMP_MAP_WIDTH)
		tile_x = CMP_MAP_WIDTH-1;

	// Read a longword in the first part of the compressed map
	// The compressed map elements are of the form
	// OOOO OOOO OOOO OOOT TTTT TTTT IIII II DD
	// where:
	// OOO OOOO OOOO OOOO is the index for overlay tile (or 0 for tiles without cmp map overlays)
	// T TTTT TTTT        is the base tile index (tile to display with all overlays removed)
	// II IIII            is the removable_mask index to use when positionned on this tile
	//                    (REMOVABLES_MASKS_LENGTH possible values)
	// DD                 is the index for the direction subroutine to pick
	cmp_data = readlong((u8*)fbuffer[COMPRESSED_MAP], (tile_y*CMP_MAP_WIDTH+tile_x)*4);

	tile_data = (cmp_data & 0x1FF00) >> 1;
	
	bit_index = ((u8)cmp_data) & 0xFC;
	if (bit_index == 0)
		return;

	// direction "subroutine" to use (diagonal, horizontal...)
	dir_offset = ((u8)cmp_data) & 0x03;

	// read the mask with relevant removable turned on, associated to the tile
	tmp_bitmask = readlong((u8*)fbuffer[LOADER],  REMOVABLES_MASKS_START + bit_index);
	
	if (tile_data <= 0x480)
	// ignore if blank or exit
	// nb: if it is an exit, lower 5 bits are the exit number
		return;

	switch (tile_data)
	// ignore tunnel exits
		case 0x5100: case 0x5180: case 0x6100: case 0x6180:	
			return;

	// direction "subroutines":
	if ((dir_offset == 0) && (rdy > 0))
	{	// moving down and having crossed the horizontal
		// boundary (set at the tile top)
		// => turn the relevant removable visible
		rem_bitmask = tmp_bitmask;
	}
	if ((dir_offset == 1) && (rdy < 0))
	{	// moving up and having crossed the horizontal
		// boundary (set at tile bottom)
		// => turn the relevant removable invisible
		tmp_bitmask &= ~(1 << (bit_index >> 2));	// don't forget to rotate dammit!
		rem_bitmask = tmp_bitmask;
	}
	if (dir_offset == 2)
	{	// check the crossing of a bottom-left to top-right diagonal
		cmp_x = 0xF - ((prisoner_x/2) & 0xF);	// need to invert x
		cmp_y = (prisoner_2y/2) & 0xF;
		if ( ((rdx > 0) && (rdy == 0)) || (rdy > 0) )
		{	// into the bottom "quadrant"
			if (cmp_x <= cmp_y)
			{	// turn removable on
				rem_bitmask = tmp_bitmask;
			}
		}
		else
		{	// into the top "quadrant"
			if (cmp_x >= cmp_y)
			{	// turn removable off
				tmp_bitmask &= ~(1 << (bit_index >> 2));
				rem_bitmask = tmp_bitmask;
			}
		}
	}
	if (dir_offset == 3)
	{	// check the crossing of a top-left to bottom-right diagonal
		cmp_x = (prisoner_x/2) & 0xF;
		cmp_y = (prisoner_2y/2) & 0xF;
		if ( ((rdx < 0) && (rdy == 0)) || (rdy > 0) )
		{	// into the bottom "quadrant"
			if (cmp_x <= cmp_y)
			{	// turn removable on
				rem_bitmask = tmp_bitmask;
			}
		}
		else
		{	// into the top "quadrant"
			if (cmp_x >= cmp_y)
			{	// turn removable off
				tmp_bitmask &= ~(1 << (bit_index >> 2));
				rem_bitmask = tmp_bitmask;
			}
		}
	}
}

void add_guybrushes()
{
u8 i, sid;

	// Add our current prisoner's animation
	if (prisoner_reset_ani)
	{	// Set the animation for our prisoner
		if (in_tunnel)
		{
			prisoner_speed = 1;
			prisoner_ani.index = prisoner_as_guard?GUARD_CRAWL_ANI:CRAWL_ANI;
		}
		else if (prisoner_state & STATE_SHOT)
			prisoner_ani.index = prisoner_as_guard?GUARD_SHOT_ANI:SHOT_ANI;
		else if (prisoner_speed == 1)
			prisoner_ani.index = prisoner_as_guard?GUARD_WALK_ANI:WALK_ANI; 
		else
			prisoner_ani.index = prisoner_as_guard?GUARD_RUN_ANI:RUN_ANI; 

		prisoner_ani.framecount = 0;
		prisoner_ani.end_of_ani_function = NULL;
		guy(current_nation).reset_animation = false;
	}

	// Always display our main guy
	sid = get_guybrush_sid(current_nation);
	overlay[overlay_index].sid = (opt_sid == -1)?sid:opt_sid;	

	// If you uncomment the lines below, you'll get confirmation that our position 
	// computations are right to position our guy to the middle of the screen
//overlay[overlay_index].x = gl_off_x + guybrush[PRISONER].px + sprite[sid].x_offset;
	overlay[overlay_index].y = gl_off_y + guybrush[current_nation].p2y/2 - sprite[sid].h + (in_tunnel?11:5);
	overlay[overlay_index].x = PSP_SCR_WIDTH/2 + sprite[sid].x_offset - (in_tunnel?24:0);  
//	overlay[overlay_index].y = PSP_SCR_HEIGHT/2 - NORTHWARD_HO - 32; 

	// Our guy's always at the center of our z-buffer
	overlay[overlay_index].z = 0;
	// Who cares about optimizing for one guy!
	if(!(p_event[current_nation].escaped))
		// Ignore this overlay if our guy is free
		safe_overlay_index_increment();
	/*
		overlay[overlay_index].x = PSP_SCR_WIDTH/2;  
		overlay[overlay_index].y = PSP_SCR_HEIGHT/2 - NORTHWARD_HO - 32; 
		overlay[overlay_index].sid = sid + 0x37;
		overlay[overlay_index].z = -1;
		overlay_index++;
	*/


	// Now add all the other guys
	for (i=0; i< (opt_no_guards?NB_NATIONS:NB_GUYBRUSHES); i++)
	{
		// Our current guy has already been taken care of above
		if (i==current_nation)
			continue;

		// Everybody is offscreen by default. NB: this is only used for guards, 
		// so don't care if the prisoner's onscreen status is wrong
		guy(i).is_onscreen = false;

		// Guy already on the loose?
		if ((i<NB_NATIONS) && (p_event[i].escaped))
			continue;

		// Guybrush's probably blowing his foghorn in the library again
		if (guy(i).room != current_room_index)
			continue;

		// How I wish there was an easy way to explain these small offsets we add
		// NB: The positions we compute below are still missing the sprite dimensions
		// which we will only add at the end. They are just good enough for ignore_offscreen()
		overlay[overlay_index].x = gl_off_x + guy(i).px; // + sprite[sid].x_offset;
		ignore_offscreen_x(overlay_index);	// Don't bother if offscreen
		overlay[overlay_index].y = gl_off_y + guy(i).p2y/2 + 5; //  - sprite[sid].h + 5;
		ignore_offscreen_y(overlay_index);	// Don't bother if offscreen

		// If the guy's under a removable wall, we ignore him too
		if (is_outside && (remove_props[guy(i).px/32][(guy(i).p2y+4)/32]))
			// TO_DO: check for an actual props SID?
			continue;

		// Ideally, we would remove animations that have gone offscreen here, but 
		// there's little performance to be gained in doing so, so we don't
		// We do set the onscreen flag though
		guy(i).is_onscreen = true;

//		printf("guard(%x).is_onscreen\n", i-4);

		// First we check if the relevant guy's animation was ever initialized
		if (guy(i).reset_animation)
		{	// We need to initialize that guy's animation
			// TO_DO: better thriller dance keeping the german's uniforms
//			guy(i).ani_index = (opt_thrillerdance)?prisoner_ani:nb_animations;

			if ((i<NB_NATIONS) && (guy(i).state & STATE_SHOT))
				// Might have a stiff guy to display
				guy(i).animation.index = ((guy(i).is_dressed_as_guard)?GUARD_SHOT_ANI:SHOT_ANI);
			else if (guy(i).state&STATE_TUNNELING)
				guy(i).animation.index = ((guy(i).is_dressed_as_guard)?GUARD_CRAWL_ANI:CRAWL_ANI);
			else if (guy(i).speed == 1)
				guy(i).animation.index = ((guy(i).is_dressed_as_guard)?GUARD_WALK_ANI:WALK_ANI); 
			else
				guy(i).animation.index = ((guy(i).is_dressed_as_guard)?GUARD_RUN_ANI:RUN_ANI); 
			guy(i).animation.framecount = 0;
			guy(i).animation.end_of_ani_function = NULL;
			guy(i).reset_animation = false;
		}

		// OK, now we're good to add the overlay sprite
		sid = get_guybrush_sid(i);
		overlay[overlay_index].sid = sid;

		// And now that we have the sprite attributes, we can add the final position adjustments
		if (i < NB_NATIONS)
		{	// prisoners
			overlay[overlay_index].x += sprite[sid].x_offset - ((guy(i).state&STATE_TUNNELING)?24:0);
			overlay[overlay_index].y -= sprite[sid].h - ((guy(i).state&STATE_TUNNELING)?6:0);
		}
		else
		{	// guards
			overlay[overlay_index].x += sprite[sid].x_offset - 16;
			overlay[overlay_index].y -= sprite[sid].h + 4;
		}
		overlay[overlay_index].z = overlay[overlay_index].y - sprite[sid].z_offset 
				- PSP_SCR_HEIGHT/2 + NORTHWARD_HO - 3; 
		safe_overlay_index_increment();
	}

// Let's add our guy
/*	// TO_DO: REMOVE THIS DEBUG FEATURE
	overlay[overlay_index].sid = (opt_sid == -1)?prisoner_sid:opt_sid;	
	// 0x85 = tunnel board, 0x91 = safe
	overlay[overlay_index].x = PSP_SCR_WIDTH/2;  
	overlay[overlay_index].y = PSP_SCR_HEIGHT/2 - NORTHWARD_HO - 32; 
	// Our guy's always at the center of our z-buffer
	overlay[overlay_index].z = 0;
	overlay_index++;
*/
	if (opt_play_as_the_safe)
	{
		overlay[overlay_index].sid = 0x91;	
		overlay[overlay_index].x = PSP_SCR_WIDTH/2 - 10;  
		overlay[overlay_index].y = PSP_SCR_HEIGHT/2 - NORTHWARD_HO - 32 - (((dx==0)&&(d2y==0))?8:12); 
		overlay[overlay_index].z = 0;
		safe_overlay_index_increment();
	}
}

void prisoner_killed(u32 p)
{
	p_event[p].display_shot = true;
	// Prevent the sprite from being animated
	guy(p).state = STATE_SHOT;
	// This will reinstantiate the guard when we switch screens
	guard(p_event[p].caught_by).state = STATE_IN_PURSUIT;
}


static __inline bool guard_close_by(i, pos_x, pos_2y)
{
s16 dx, dy;
	dx = pos_x+16 - guard(i).px;
	dy = pos_2y+8 - guard(i).p2y;
	if ( ((dx>=0 && dx<=144) || (dx<0 && dx>=-144)) &&
		 ((dy>=0 && dy<=160)  || (dy<0 && dy>-160)) )
		 return true;
	return false;
}


static __inline bool guard_collision(i, pos_x, pos_2y)
{
s16 dx, dy;
	dx = pos_x+16 - guard(i).px;
	dy = pos_2y+8 - guard(i).p2y;
	if ( ((dx>=0 && dx<=10) || (dx<0 && dx>=-10)) &&
		 ((dy>=0 && dy<=8)  || (dy<0 && dy>-8)) )
		 return true;
	return false;
}


int move_guards()
{
	int i, p, dir_x, dir_y;
	u32 route_pos;
	u16 route_data;
	bool continue_parent;
	bool but_i_just_got_out;
	bool still_have_a_prisoner_to_chase;

	int	kill_motion = 0;


	for (i=0; i<NB_GUARDS; i++)
	{
		// 0a. We'll use this variable to break this loop from a child loop if needed
		continue_parent = false;

		// 0b. This one is to make sure that we execute at least one more step from 
		// the route at the end of the blocked timeout 
		// (prevents the blocking of guards by unattended prisoners)
		but_i_just_got_out = false;
/*
		if (guard(i).is_onscreen)
			printf("is_onscreen(%x)\n", i);
*/
		// 1. Check if we have a collision between our current prisoner and the guard
		//    (and kill our motion as a result)
		p = current_nation;
		if ((guard(i).room == current_room_index) && guard_collision(i, guy(p).px, guy(p).p2y))
			kill_motion = -1;

		// 2. Deal with guards that are currently being blocked by a prisoner
		if (guard(i).state & STATE_BLOCKED)
		{
			// Did our blocking counter just reach zero
			if (guard(i).wait == 0)
			{
				// Is our prisoner blocked but still trying to get out at the end of the guard's pause?
				if ((kill_motion) && (dx || d2y))
				{	
					// Prevent blocking (butter guard!)
					kill_motion = 0;
					continue;
				}

				// Nicely restore to STATE_MOVE or STATE_STOP
				guard(i).state ^= STATE_BLOCKED;
				but_i_just_got_out = true;

				// Not issuing a continue here allows us to progress one step further
				// even if blocked
			}
			else
			{	// decrement our counter, and ignore motion
				guard(i).wait--;
				continue;
			}
		}

		still_have_a_prisoner_to_chase = false;
		// 3. Check for an event with one of the prisoners
		for (p = 0; p<NB_NATIONS; p++)
		{
			// Don't bother if prisoner's dead or escaped
			if ( (guy(p).state & STATE_SHOT) || (p_event[p].escaped) )
				continue;

			// Do we have a prisoner in sight?
			if ( (guard(i).room == guy(p).room) && guard_close_by(i, guy(p).px, guy(p).p2y) )
			{
				// Don't do jack if we're already in the middle of something
				if (p_event[p].require_pass || p_event[p].to_solitary)
					continue;

				// Is that prisoner supposed to be here?
				if ( (!(guy(p).state & STATE_IN_PURSUIT)) && (p_event[p].unauthorized) )
				{	// spotted!
					guy(p).state |= STATE_IN_PURSUIT;
					// Might be a prisoner on the lose 
					if (guy(p).state & STATE_IN_PRISON)
					{	// prison break: the prisoner left his cell!
						guy(p).state &= ~STATE_IN_PRISON;
						p_event[p].solitary_countdown = 0;
					}
				}

				// Should we be in pursuit of this prisoner?
				if (guy(p).state & STATE_IN_PURSUIT)
				{
					still_have_a_prisoner_to_chase = true;

					if (!(guard(i).state & STATE_IN_PURSUIT))
					{	// Start walking towards prisoner
						// Indicate that we deviate from the normal flight path
						guard(i).state |= STATE_IN_PURSUIT;
						// Set guard to walk
						guard(i).speed = 1;
						guard(i).wait = WALKING_PURSUIT_TIMEOUT;
						guard(i).reset_animation = true;
					}
					else if ((guard(i).speed == 1) && (guard(i).wait == 0))
					{	// Start running towards prisoner
						guard(i).speed = 2;
						guard(i).wait = RUNNING_PURSUIT_TIMEOUT;
						guard(i).reset_animation = true;
					}
					else if ((guard(i).speed == 2) && (guard(i).wait == 0))
					{	// We were running, now we're pissed off => License to kill
						if (guy(p).state & STATE_MOTION)
						{	// Moving prisoners make good targets
							// Stop the guard and set shooting animation
							guard(i).state &= ~STATE_MOTION;
							guard(i).state |= STATE_SHOT|STATE_ANIMATED;
							guard(i).animation.index = GUARD_SHOOTS_ANI;
							guard(i).animation.framecount = 0;
							// Stop the prisoner and set shot animation
							guy(p).state = STATE_SHOT|STATE_ANIMATED;
							guy(p).animation.end_of_ani_parameter = p;
							guy(p).animation.end_of_ani_function = prisoner_killed;
							guy(p).animation.index = guy(p).is_dressed_as_guard?GUARD_SHOT_ANI:SHOT_ANI;
							guy(p).animation.framecount = 0;
							// Set the event flags
							p_event[p].unauthorized = false;
							p_event[p].caught_by = i;
						}
						else
						{	// The prisoner has stopped => give him another chance
							// Restart the run counter
							guard(i).wait = RUNNING_PURSUIT_TIMEOUT;
						}
					}
					else if (guard(i).state & STATE_SHOT)
						// The prisoner is supposed to be dead now
						still_have_a_prisoner_to_chase = false;
					else if (guard(i).wait != 0)
						guard(i).wait--;

					// Have we caught up with our guy?
					if (guard_collision(i, guy(p).px, guy(p).p2y))
					{
						if (guy(p).is_dressed_as_guard)
						{	// Ask for a pass
							p_event[p].require_pass = true;
							// We'll need this for check_on_prisoners()
							p_event[p].caught_by = i;
						}
						else
							p_event[p].to_solitary = true;
						// The following ensures that we reset the guard when offscreen
						guard(i).state &= ~STATE_MOTION;
						still_have_a_prisoner_to_chase = false;
						break;
					}

					// Update the guard's direction (also applies when shooting)
					dir_x = (guard(i).px - guy(p).px - 16)/2;
					// If we don't divide by 2 here, we'll have jerky motion on pursuit
					dir_y = (guard(i).p2y - guy(p).p2y - 8)/2;

					if (dir_x != 0)
						dir_x = (dir_x>0)?-1:1;
					dir_x++;

					if (dir_y !=0)
						dir_y = (dir_y>0)?-1:1;
					dir_y++;

					guard(i).direction = directions[dir_y][dir_x];

					if (!(guard(i).state & STATE_SHOT))
					{
						// "After him!"
						guard(i).go_on = 1;
						guard(i).state |= STATE_MOTION;
					}

				}
				else
				{	// Prisoner is not flagged as suspicious... yet
					// is a prisoner in the same room & within a range of our guard
					if (guard_collision(i, guy(p).px, guy(p).p2y) 
						  // Also, have NOT just exited a blocked timeout loop
						  && (!but_i_just_got_out))
					{
						// Setup the blocked counter
						guard(i).wait = BLOCKED_GUARD_TIMEOUT;
						// And indicate that we are stopped
						guard(i).state |= STATE_BLOCKED;
						// Ah shoot, we need to continue the parent "for" loop
						continue_parent = true;
						// Break this loop then
						break;
					}
				}
			}
		}

		// Messy, but works
		if (continue_parent)
			continue;

		// Do we need to reinstantiate the guard?
		if ((guard(i).state & STATE_IN_PURSUIT) && (!still_have_a_prisoner_to_chase))
		{	// We were in pursuit but lost our target
			if (guard(i).is_onscreen)
			{	// Don't reinstanciate if onscreen. Just freeze
				if (guard(i).state & STATE_MOTION)
				{	// Set animation to stopped
					guard(i).state &= ~(STATE_MOTION|STATE_ANIMATED);
					guard(i).reset_animation = true;
				}
				continue;
			}
			else
				// Reinstantiate guard
				route_data = 0xFFFF;
		}
		else
		{
			// Go_on > 0 indicates that we don't need to read the route data
			if (guard(i).go_on > 0)
			{	// continue in the same direction
				guard(i).go_on--;

				// We want to move
				if (guard(i).state & STATE_MOTION)
				{	// Is there an obstacle in the way?
					dir_x = guard(i).speed * dir_to_dx[guard(i).direction];
					dir_y = guard(i).speed * dir_to_d2y[guard(i).direction];
					if ((guard(i).state & STATE_IN_PURSUIT) && (!check_guard_footprint(i, dir_x, dir_y)))
					{	// Only check footprint when in pursuit
						// check_guard_footprint() returns false if we can't go there
						guard(i).state &= ~(STATE_MOTION|STATE_ANIMATED);
						guard(i).reset_animation = true;
					}
					else
					{	// Just move
						guard(i).px += dir_x;
						guard(i).p2y += dir_y;
					}
				}
				continue;
			}

			// Change in route => get our current route position
			route_pos = readlong(fbuffer[GUARDS], i*MENDAT_ITEM_SIZE + 0x0E);

			// Read the first word
			route_data = readword(fbuffer[ROUTES], route_pos);
		}

		if (route_data == 0xFFFF)
		{	// repeat => back to start of route
			route_pos = readlong(fbuffer[GUARDS], i*MENDAT_ITEM_SIZE + 0x06);
			route_data = readword(fbuffer[ROUTES], route_pos);
			guard(i).px = readword(fbuffer[GUARDS],i*MENDAT_ITEM_SIZE + 2);
			guard(i).p2y = 2*readword(fbuffer[GUARDS],i*MENDAT_ITEM_SIZE);
			guard(i).room = readword(fbuffer[GUARDS],i*MENDAT_ITEM_SIZE + 4);
			guard(i).state = 0;
			guard(i).speed = 1;
			guard(i).reset_animation = true;	// reset the animation
		}

		if (route_data & 0x8000)
		{	// absolute positioning (eg. when switching rooms)
			guard(i).room = readword(fbuffer[ROUTES], route_pos + 2);
			guard(i).px = readword(fbuffer[ROUTES], route_pos + 6);
			guard(i).p2y = 2*readword(fbuffer[ROUTES], route_pos + 4);
			route_pos +=10;
		}
		else
		{	// standard route action
			guard(i).go_on = route_data; // How long do we need to keep at it
			route_data =  readword(fbuffer[ROUTES], route_pos + 2);
			if (route_data == 0xFFFF)
				// stopped state (pause)
				guard(i).state &= ~(STATE_MOTION|STATE_ANIMATED);
			else
			{	// motion state
				guard(i).direction = route_data;
				guard(i).state |= STATE_MOTION|STATE_ANIMATED;
				// Change our position
				guard(i).px += guard(i).speed * dir_to_dx[guard(i).direction];
				guard(i).p2y += guard(i).speed * dir_to_d2y[guard(i).direction];
			}
			route_pos += 4;
		}

		// save the new current position
		writelong(fbuffer[GUARDS], i*MENDAT_ITEM_SIZE+0x0E, route_pos);

	}

	return kill_motion;
}


// Display room
void display_room()
{
// OK, I'll spare you the suspense: this is NOT optimized as hell!
// We are redrawing ALL the tiles and ALL overlays, for EACH FRAME!
// Yup, no scrolling or anything: just plain unoptimized brute force...
// But hey, the PSP can handle it, and so should a decent PC, so why bother?

	u32 offset;	
	u16 room_x, room_y, tile_data;
	u32 raw_data;
	u16 rem_offset;
	s16 min_x, max_x, min_y, max_y;
	u16 tile_tmp, nb_tiles;
	u8  bit_index;
	s16 tile_x, tile_y;
	s16 pixel_x, pixel_y;
	int u;

//	printf("prisoner (x,y) = (%d,%d)\n", prisoner_x, prisoner_2y/2);

	glColor3f(fade_value, fade_value, fade_value);

	if (init_animations)
	{	// We might have to init the room animations after a room switch or nationality change
		// Reset all animations
		// TODO_execute all end of ani functions
		for (u=0; u<MAX_CURRENTLY_ANIMATED; u++)
			currently_animated[u] = -1;	// We use -1, as 0 is a valid index
		// Reset 
		nb_animations = 0;
		for (u=0; u<NB_GUYBRUSHES; u++)
			guybrush[u].reset_animation = true;
	}

	// Compute GL offsets (position of 0,0 corner of the room wrt center of the screen) 
	gl_off_x = PSP_SCR_WIDTH/2 - prisoner_x;
	gl_off_y = PSP_SCR_HEIGHT/2 - (prisoner_2y/2) - NORTHWARD_HO;


	// reset room overlays
	overlay_index = 0;

	// Update the room description message (NB: we need to do that before the props
	// overlay call, if we want a props message override
	if (is_outside)
	{	// Outside
		set_status_message(fbuffer[LOADER] + readlong(fbuffer[LOADER], MESSAGE_BASE + 
			4*COURTYARD_MSG_ID), 0, NO_MESSAGE_TIMEOUT);
	}
	else if (current_room_index < ROOM_TUNNEL)
	{	// Standard room
		set_status_message(fbuffer[LOADER] + readlong(fbuffer[LOADER], MESSAGE_BASE + 
			4*(readbyte(fbuffer[LOADER], ROOM_DESC_BASE	+ current_room_index))), 0, NO_MESSAGE_TIMEOUT);
	}
	else
	{	// Tunnel
		set_status_message(fbuffer[LOADER] + readlong(fbuffer[LOADER], MESSAGE_BASE + 
			4*TUNNEL_MSG_ID), 0, NO_MESSAGE_TIMEOUT);
	}


	// Before we do anything, let's set the pickable objects in
	// our overlay table (so that room overlays go on top of 'em)
	set_props_overlays();

	// No readtile() macros used here, for speed
	if (is_inside)
	{	// Standard room (inside)
		// Read the offset
		offset = readlong((u8*)fbuffer[ROOMS], OFFSETS_START+4*current_room_index);
		if (offset == 0xFFFFFFFF)
		// For some reason there is a gap in the middle of the rooms data
		// This shouldn't matter, unless you set the room manually
			return;

		// Now that we have the offset, let's look at the room

		// The 2 first words are the room Y and X dimension (in tiles),
		// in that order
		room_y = readword((u8*)fbuffer[ROOMS], ROOMS_START+offset);
		offset +=2;
		room_x = readword((u8*)fbuffer[ROOMS], ROOMS_START+offset);
		offset +=2;

		// Read the tiles data
		pixel_y = gl_off_y;	// A little optimization can't hurt
		for (tile_y=0; tile_y<room_y; tile_y++)
		{
			pixel_x = gl_off_x;
			for(tile_x=0; tile_x<room_x; tile_x++)
			{
				// A tile is 32(x)*16(y)*4(bits) = 256 bytes
				// A specific room tile is identified by a word

				/*
				 * tile_data  = tttt tttt tggo xxxx 
				 * t: tile #
				 * g: lock grade (01 = lockpick, 10 = key 2, 11 = key 1)
				 * o: door open flag
				 * x: exit lookup number (in exit map [1-8])
				*/
				tile_data = readword((u8*)fbuffer[ROOMS], ROOMS_START+offset);

				display_sprite(pixel_x,pixel_y,32,16, 
					cell_texid[(tile_data>>7) + ((current_room_index>0x202)?0x1E0:0)]);

				// Display sprite overlay
				crm_set_overlays(pixel_x, pixel_y, tile_data & 0xFF80, ROOMS_START+offset, room_x);

				offset +=2;		// Read next tile
				pixel_x += 32;
			}
			pixel_y += 16;
		}


	}
	else
	{	// on compressed map (outside)
		room_x = CMP_MAP_WIDTH;
		room_y = CMP_MAP_HEIGHT;

		// Since we're outside, take care of removable sections
		removable_walls();

		// These are the min/max tile boundary computation for PSP screen
		// according to our cropped section
		min_y = prisoner_2y/32 - 8;
		if (min_y < 0)
			min_y = 0;

		max_y = prisoner_2y/32 + 10;
		if (max_y > room_y)
			max_y = room_y;

		min_x = prisoner_x/32 - 8;
		if (min_x < 0)
			min_x = 0;

		max_x = prisoner_x/32 + 9;
		if (max_x > room_x)
			max_x = room_x;

		// Read the tiles data
		pixel_y = gl_off_y+min_y*16;
		for (tile_y=min_y; tile_y<max_y; tile_y++)
		{
			offset = (tile_y*room_x+min_x)*4;
			pixel_x = gl_off_x+32*min_x;
			for(tile_x=min_x; tile_x<max_x; tile_x++)
			{
				/* Read a longword in the first part of the compressed map
				 * The compressed map elements are of the form
				 * OOOO OOOO OOOO OOOT TTTT TTTT IIII IIDD
				 * where:
				 * OOO OOOO OOOO OOOO is the index for overlay tile (or 0 for tiles without cmp map overlays)
				 * T TTTT TTTT is the base tile index (tile to display with all overlays removed)
				 * II IIII is the removable_mask index to use when positionned on this tile
				 *         (REMOVABLES_MASKS_LENGTH possible values)
				 * DD is the index for the direction subroutine to pick
				 *
				 * NB: in the case of an exit (T TTTT TTTT < 0x900), IIII IIDD is the exit index
				 */

				raw_data = readlong((u8*)fbuffer[COMPRESSED_MAP], offset);
				tile_data = (u16)(raw_data>>1) & 0xFF80;

				// For the time being, we'll reset the removable boolean for props
				remove_props[tile_x][tile_y] = 0;

				// If the first 15 bits of this longword are zero, then we have a simple tile, 
				// with remainder 17 being the tile data 
				rem_offset = (raw_data >> 16) & 0xFFFE;
				// First word (with mask 0xFFFE) indicates if we have a simple tile or not

				if (rem_offset != 0)
				// If the first 15 bits are not null, we have a complex sequence, 
				// which we must read in second part of the compressed map, 
				// the 15 bits being the offset from start of second part
				{
					// The first word read is the number of overlapping tiles
					// overlapping tiles occur when there might be a wall hiding a walkable section
					nb_tiles = readword((u8*)fbuffer[COMPRESSED_MAP], CM_TILES_START+rem_offset);
					// The rest of the data is a tile index (FF80), a bit index (1F), and 2 bits unused.
					// the later being used to check bits of an overlay bitmap longword
					for (u=nb_tiles; u!=0; u--)
					{
						tile_tmp = readword((u8*)fbuffer[COMPRESSED_MAP], CM_TILES_START+rem_offset + 2*u);
						bit_index = tile_tmp & 0x1F;
						if ( (1<<bit_index) & rem_bitmask )
						{
							tile_data = tile_tmp;
							// Do we need to hide the props beneath?
							if (!props_tile[tile_data>>7])
								remove_props[tile_x][tile_y] = 1;
							break;
						}
					}
				}

				// At last, we have a tile we can display
				display_sprite(pixel_x,pixel_y,32,16, 
					cell_texid[(tile_data>>7)]);

				// Display sprite overlay
//				set_overlays(pixel_x, pixel_y, tile_data, offset, room_x);

				offset += 4;
				pixel_x += 32;
			}
			pixel_y += 16;
		}
		// On the compressed map, we set all the overlays in one go
		cmp_set_overlays();
	}

	// Add all our guys
	add_guybrushes();

	// Now that the background is done, and we have all the overlays, display the overlay sprites
	display_overlays();

	// Make sure we only reset the overlay animations once
	if (init_animations)
		init_animations = false;

	// We'll need that for next run
	last_p_x = prisoner_x;
	last_p_y = prisoner_2y/2; 
}


// Display location and other strings on panel
void display_message(char string[])
{
	char c;
	int pos = 0, i = 0;

	if (string == NULL)
		return;

	if (string[0] < 0x20)
	{
		pos = string[0]+1;
		i = 1;
	}
	while ((c = string[i++]))
	{
		display_sprite(PANEL_MESSAGE_X+8*pos, PANEL_MESSAGE_Y,
			PANEL_CHARS_W, PANEL_CHARS_CORRECTED_H, chars_texid[c-0x20]);
		pos++;
	}
}







// Display a static picture (512x256x16 GRAB texture) that was previously
// loaded from an IFF file. 
void display_picture()
{
	// NB, we don't need to clear the screen to black, as this is done 
	// before calling this function

	// Set white to the current fade_value for fading effects
	glColor3f(fade_value, fade_value, fade_value);	

	// Display the current IFF image
	glBindTexture(GL_TEXTURE_2D, picture_texid);

//	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
//	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

	// The image
	glBegin(GL_TRIANGLE_FAN);

	glTexCoord2f(0.0f, 0.0f);
	glVertex2f((PSP_SCR_WIDTH-picture_w)/2, (PSP_SCR_HEIGHT-picture_h)/2);

	glTexCoord2f(1.0f, 0.0f);
	glVertex2f(512 + (PSP_SCR_WIDTH-picture_w)/2, (PSP_SCR_HEIGHT-picture_h)/2);

	glTexCoord2f(1.0, 1.0);
	glVertex2f(512 + (PSP_SCR_WIDTH-picture_w)/2, 512 + (PSP_SCR_HEIGHT-picture_h)/2);

	glTexCoord2f(0.0, 1.0);
	glVertex2f((PSP_SCR_WIDTH-picture_w)/2, 512 + (PSP_SCR_HEIGHT-picture_h)/2);

	glEnd();
}


// Display Panel
void display_panel()
{
	u8 w,h;
	u16 i, sid;

	glColor3f(0.0f, 0.0f, 0.0f);	// Set the colour to black

	glDisable(GL_BLEND);	// Needed for black objects to show

	// Because the original game wasn't designed for widescreen
	// we have to diagonally crop the area to keep some elements hidden
	// TO_DO: add some texture, to make it look like an old photograph or something
	h = (28-NORTHWARD_HO)+36;
	w = 2*h;
	glBegin(GL_TRIANGLES);

	glVertex2f(0, 0);
	glVertex2f(w, 0);
	glVertex2f(0, h);

	glVertex2f(PSP_SCR_WIDTH, 0);
	glVertex2f(PSP_SCR_WIDTH-w, 0);
	glVertex2f(PSP_SCR_WIDTH, h);

	glVertex2f(PSP_SCR_WIDTH, PSP_SCR_HEIGHT-32);
	glVertex2f(PSP_SCR_WIDTH-w, PSP_SCR_HEIGHT-32);
	glVertex2f(PSP_SCR_WIDTH, PSP_SCR_HEIGHT-32-h);

	glVertex2f(0, PSP_SCR_HEIGHT-32);
	glVertex2f(w, PSP_SCR_HEIGHT-32);
	glVertex2f(0, PSP_SCR_HEIGHT-32-h);

	glEnd();

	// Black rectangle (panel base) at the bottom
	glBegin(GL_TRIANGLE_FAN);

	glVertex2f(0, PSP_SCR_HEIGHT-32);
	glVertex2f(PSP_SCR_WIDTH, PSP_SCR_HEIGHT-32);
	glVertex2f(PSP_SCR_WIDTH, PSP_SCR_HEIGHT);
	glVertex2f(0, PSP_SCR_HEIGHT);

	glEnd();

	// Restore colour
	glColor3f(fade_value, fade_value, fade_value);

	// Draw the 2 parts of our panel
 	glBindTexture(GL_TEXTURE_2D, panel1_texid);

	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

//	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
//	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

	// pspGL does not implement QUADS
	glBegin(GL_TRIANGLE_FAN);

	glTexCoord2f(0.0f, 0.0f);
	glVertex2f(PANEL_OFF_X, PSP_SCR_HEIGHT-PANEL_BASE_H+PANEL_OFF_Y );

	glTexCoord2f(1.0f, 0.0f);
	glVertex2f(PANEL_OFF_X+PANEL_BASE1_W, PSP_SCR_HEIGHT-PANEL_BASE_H+PANEL_OFF_Y );

	glTexCoord2f(1.0, 1.0);
	glVertex2f(PANEL_OFF_X+PANEL_BASE1_W, PSP_SCR_HEIGHT+PANEL_OFF_Y);

	glTexCoord2f(0.0, 1.0);
	glVertex2f(PANEL_OFF_X, PSP_SCR_HEIGHT+PANEL_OFF_Y);

	glEnd();


 	glBindTexture(GL_TEXTURE_2D, panel2_texid);

	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

//	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
//	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

	// pspGL does not implement QUADS
	glBegin(GL_TRIANGLE_FAN);

	glTexCoord2f(0.0f, 0.0f);
	glVertex2f(PANEL_OFF_X+PANEL_BASE1_W, PSP_SCR_HEIGHT-PANEL_BASE_H+PANEL_OFF_Y );

	glTexCoord2f(1.0f, 0.0f);
	glVertex2f(PANEL_OFF_X+PANEL_BASE1_W+PANEL_BASE2_W, PSP_SCR_HEIGHT-PANEL_BASE_H+PANEL_OFF_Y );

	glTexCoord2f(1.0, 1.0);
	glVertex2f(PANEL_OFF_X+PANEL_BASE1_W+PANEL_BASE2_W, PSP_SCR_HEIGHT+PANEL_OFF_Y);

	glTexCoord2f(0.0, 1.0);
	glVertex2f(PANEL_OFF_X+PANEL_BASE1_W, PSP_SCR_HEIGHT+PANEL_OFF_Y);

	glEnd();

	glEnable(GL_BLEND);	// We'll need blending for the sprites, etc.

	// Display our guy's faces
	for (i=0; i<4; i++)
	{
		if (guy(i).state & STATE_SHOT)
			sid = PANEL_FACE_SHOT;
		else if (p_event[i].escaped)
			sid = PANEL_FACE_FREE;
		else
		{
			sid = 0xd5 + i;
			if ( (guy(i).state & STATE_IN_PRISON) || 
				 ( (guy(i).state & STATE_IN_PURSUIT) && ((t/1000)%2) ) )
				sid = PANEL_FACE_IN_PRISON;
		}
		display_sprite(PANEL_FACES_X+i*PANEL_FACES_W, PANEL_TOP_Y,
			sprite[sid].w, sprite[sid].h, sprite_texid[sid]);
	}

	// Display the currently selected nation's flag
	display_sprite(PANEL_FLAGS_X, PANEL_TOP_Y, sprite[PANEL_FLAGS_BASE_SID+current_nation].w,
		sprite[PANEL_FLAGS_BASE_SID+current_nation].h, sprite_texid[PANEL_FLAGS_BASE_SID+current_nation]);

	// Display the clock
	// Unlike the original game, I like having the zero displayed on hour tens, always
	sid = PANEL_CLOCK_DIGITS_BASE + hours_digit_h;
	display_sprite(PANEL_CLOCK_HOURS_X, PANEL_TOP_Y,
			sprite[sid].w, sprite[sid].h, sprite_texid[sid]);

	// Hours, units
	sid = PANEL_CLOCK_DIGITS_BASE + hours_digit_l;
	display_sprite(PANEL_CLOCK_HOURS_X + PANEL_CLOCK_DIGITS_W, PANEL_TOP_Y,
			sprite[sid].w, sprite[sid].h, sprite_texid[sid]);

	// Minute, tens
	sid = PANEL_CLOCK_DIGITS_BASE + minutes_digit_h;
	display_sprite(PANEL_CLOCK_MINUTES_X, PANEL_TOP_Y,
			sprite[sid].w, sprite[sid].h, sprite_texid[sid]);

	// Minutes, units
	sid = PANEL_CLOCK_DIGITS_BASE + minutes_digit_l;
	display_sprite(PANEL_CLOCK_MINUTES_X + PANEL_CLOCK_DIGITS_W, PANEL_TOP_Y,
			sprite[sid].w, sprite[sid].h, sprite_texid[sid]);

	sid = selected_prop[current_nation] + PANEL_PROPS_BASE;
	display_sprite(PANEL_PROPS_X, PANEL_TOP_Y,
			sprite[sid].w, sprite[sid].h, sprite_texid[sid]);

	// Display the fatigue bar
	display_sprite(PANEL_FATIGUE_X, PANEL_FATIGUE_Y,
		(prisoner_fatigue>>0xB), sprite[PANEL_FATIGUE_SPRITE].h, sprite_texid[PANEL_FATIGUE_SPRITE]);

	// Display the props data
	if (over_prop_id)
	{
		sid = over_prop_id + PANEL_PROPS_BASE;
	}
	else
	{
		if (prisoner_state & STATE_TUNNELING)
			sid = STATE_CRAWL_SID;
		else if (prisoner_state & STATE_STOOGE)
			sid = STATE_STOOGE_SID;
		else
			sid = (prisoner_speed == 1)?STATE_WALK_SID:STATE_RUN_SID;
	}
	display_sprite(PANEL_STATE_X, PANEL_TOP_Y,
		sprite[sid].w, sprite[sid].h, sprite_texid[sid]);

	// Display the current status message
	display_message(status_message);
}


// Handle timed events (palette change, rollcalls, ...)
void timed_events(u16 hours, u16 minutes_high, u16 minutes_low)
{
	u16 event_data;
	u16 p, iff_id;

	// Read the hour (or reset marker)
	event_data = readword(fbuffer[LOADER], next_timed_event_ptr);

	// Negative => reset
	if (event_data & 0x8000)
		next_timed_event_ptr = TIMED_EVENTS_BASE;
		// NB: we'll use the next comparison to safely return on above event

	if (event_data != hours)
		return;

	// Read the minutes tens
	event_data = readword(fbuffer[LOADER], next_timed_event_ptr+2);
	if (event_data != minutes_high)
		return;

	// Read the minutes units
	event_data = readword(fbuffer[LOADER], next_timed_event_ptr+4);
	if (event_data != minutes_low)
		return;

	// Time match => deal with the event
	event_data = readword(fbuffer[LOADER], next_timed_event_ptr+6);

	// Change the palette
	if (event_data == TIMED_EVENT_PALETTE)
	{
		palette_index = readbyte(fbuffer[LOADER], next_timed_event_ptr+9);
		to_16bit_palette(palette_index, 0xFF, PALETTES);
		cells_to_wGRAB(fbuffer[CELLS],rgbCells);
		sprites_to_wGRAB();
		next_timed_event_ptr += 10;
	}
	else
	{	// Rollcall, etc.
///		printf("got event %04X\n", event_data);
		// Each event changes the list of authorized rooms
		authorized_ptr = readlong(fbuffer[LOADER],AUTHORIZED_BASE+4*event_data);

		// Get the relevant picture index (for events with static images) 
		// according to the IFF_INDEX_TABLE in the loader
		iff_id = readword(fbuffer[LOADER], IFF_INDEX_TABLE + 2*event_data);
///		printf("iff to load = %x\n", current_iff);
		if (iff_id != 0xFFFF)
			static_screen(iff_id, NULL, 0);
		else if (event_data == TIMED_EVENT_ROLLCALL_CHECK)
		{	// This is the actual courtyard rollcall check
			for (p=0; p<NB_NATIONS; p++)
			{
				if ( (!(guy(p).state & STATE_IN_PRISON)) && (guy(p).room != ROOM_OUTSIDE) )
					// neither outside nor in prison => catch him!
					guy(p).state |= STATE_IN_PURSUIT;
			}
		}
		next_timed_event_ptr += 8;
	}
}



// Here is the long sought after "zooming the ****ing 2D colour buffer" function.
// What a £$%^&*&^ing bore!!! And none of this crap works on PSP anyway!
void rescale_buffer()
{
// using the buffer as a texture, is the ONLY WAY I COULD FIND TO GET A ZOOM
// THAT WORKS PROPERLY IN OPENGL!!! (i.e. without showing artefacts around overlay sprites)
// Seriously guys, if you're doing 2D with sprites, you'll waste DAYS trying
// to figure out a bloody solution to zoom the lousy colour buffer, because 
// if you think, with all the GPU acceleration, there should be an easy way to
// achieve that crap, you couldn't be more wrong!
//	float old_x, old_y;

	if ((gl_width != PSP_SCR_WIDTH) && (gl_height != PSP_SCR_HEIGHT))
	{	
		glDisable(GL_BLEND);	// Better than having to use glClear()

		// First, we copy the whole buffer into a texture
		glBindTexture(GL_TEXTURE_2D,render_texid);

		glCopyTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 0, 0, PSP_SCR_WIDTH, PSP_SCR_HEIGHT, 0);

		// Then we change our viewport to the actual screen size
		glViewport(0, 0, gl_width, gl_height);

		// Now we change the projection, to the new dimensions
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
	    glOrtho(0, gl_width, gl_height, 0, -1, 1);

		// OK, now we can display the whole texture
		display_sprite(0,gl_height,gl_width,-gl_height,render_texid);

		// Finally, we restore the parameters
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		glOrtho(0, PSP_SCR_WIDTH, PSP_SCR_HEIGHT, 0, -1, 1);
		glViewport(0, 0, PSP_SCR_WIDTH, PSP_SCR_HEIGHT);

		glEnable(GL_BLEND);	// We'll need blending for the sprites, etc.

	}
}

// Open a closed door, or close an open door
// Also uses exit_flags_offset and tunnel_toggle, which are a global variable for utilities
void toggle_exit(u32 exit_nr)
{
// On the compressed map, we use either the ROOMS or TUNNEL_IO file 
// depending on the exit type. We use the previously set tunnel_toggle 
// to find out if we're dealing with a tunnel (see check_footprint())
#define ROOMS_TUNIO (tunnel_toggle?TUNNEL_IO:ROOMS)
	u32 offset;
	u16 exit_index;	// exit index in destination room
	u16 room_x, room_y, tile_data;
	int tile_x, tile_y;
	bool found;
	u8	exit_flags;
	u16 target_room_index;

	if (is_outside)
	{	
		// Toggle the exit we are facing 
		exit_flags = readbyte(fbuffer[ROOMS_TUNIO], exit_flags_offset);
		toggle_open_flag(exit_flags);
		writebyte(fbuffer[ROOMS_TUNIO], exit_flags_offset, exit_flags);

		// Get target:
		// If we are on the compressed map, we need to read 2 words (out of 4)
		// from beginning of the ROOMS_MAP file or from TUNNEL_IO if tunnelling
		offset = exit_nr << 3;	// skip 8 bytes
		target_room_index = readword((u8*)fbuffer[ROOMS_TUNIO], offset) & 0x7FF;
		exit_index = readword((u8*)fbuffer[ROOMS_TUNIO], offset+2);
	}
	else
	{	
		// Toggle the exit we are facing 
		exit_flags = readbyte(fbuffer[ROOMS], exit_flags_offset);
		toggle_open_flag(exit_flags);
		writebyte(fbuffer[ROOMS], exit_flags_offset, exit_flags);

		// Get target by reading from the ROOMS_EXIT_BASE data
		exit_index = (exit_nr&0xF)-1;
		offset = current_room_index << 4;
		// Now the real clever trick here is that the exit index of the room you 
		// just left and the exit index of the one you go always match.
		// Thus, we know where we should get positioned on entering the room
		target_room_index = readword((u8*)fbuffer[ROOMS], ROOMS_EXITS_BASE + offset 
			+ 2*exit_index);
	}

	exit_index++;	// zero based to one based

	if (target_room_index & 0x8000)	
	{	// outside destination (compressed map)
		room_x = CMP_MAP_WIDTH;		// keep our readtile macros happy
		// NB: The ground floor rooms are in [00-F8]
		offset = target_room_index & 0xF8;
		// set the mirror door to open
		exit_flags = readbyte(fbuffer[ROOMS_TUNIO], offset);
		toggle_open_flag(exit_flags);
		writebyte(fbuffer[ROOMS_TUNIO], offset, exit_flags);
	}
	else
	{	// inside destination (colditz_room_map)
		// Get the room dimensions
		offset = readlong((u8*)fbuffer[ROOMS], OFFSETS_START+4*target_room_index);
		room_y = readword((u8*)fbuffer[ROOMS], ROOMS_START+offset);
		offset +=2;
		room_x = readword((u8*)fbuffer[ROOMS], ROOMS_START+offset);
		offset +=2;

		// Read the tiles data
		found = false;	// easier this way, as tile_x/y won't need adjusting
		for (tile_y=0; (tile_y<room_y)&&(!found); tile_y++)
		{
			for(tile_x=0; (tile_x<room_x)&&(!found); tile_x++)
			{
				tile_data = readword((u8*)fbuffer[ROOMS], ROOMS_START+offset);
				if ((tile_data & 0xF) == exit_index)
				{
					found = true;
					// open exit
					exit_flags = tile_data & 0xFF;
					toggle_open_flag(exit_flags);
					writebyte(fbuffer[ROOMS], ROOMS_START+offset+1, exit_flags);
					break;
				}
				offset +=2;		// Read next tile
			}
			if (found)
				break;
		}
	}
}


void enqueue_event(void (*f)(u32), u32 p, u64 delay)
{
	u8 i;

	// find an empty event to use
	for (i=0; i< NB_EVENTS; i++)
		if (events[i].function == NULL)
			break;

	if (i == NB_EVENTS)
	{
		perr("Couldn't enqueue event!!!\n");
		return;
	}

	events[i].function = f;
	events[i].parameter = p;
	events[i].expiration_time = mtime() + delay;
}


// Checks if the prisoner can go to (px,p2y) and initiates door/tunnel I/O
// Returns:
// non zero if allowed (-1 if not an exit, or the exit number)
// 0 if not allowed
// TO_DO: remove gotit debug code
// Be mindful that the dx, d2y used here are not the same as the global values from main!
// Note that if (dx,d2y) = (0,0) we'll return a tunnel exit nr rather than a regular exit
s16 check_footprint(s16 dx, s16 d2y)
{
	u32 tile, tile_mask, exit_mask, offset=0;
	u32 ani_offset;
	u32 footprint = SPRITE_FOOTPRINT;
	u16 room_x, room_y;
	u16 mask_y;
	// maks offsets for upper-left, upper-right, lower-left, lower_right tiles
	u32 mask_offset[4];	// tile boundary
	u32 exit_offset[4];	// exit boundary
	s16 tile_x, tile_y, exit_dx[2];
	u8 i,u,sid;
	s16 gotit = -1;
	s16 px, p2y;
	u8	exit_flags;
	u8	exit_nr;
	u8  tunexit_tool[4] = {ITEM_NONE, ITEM_NONE, ITEM_NONE, ITEM_NONE};
	bool check_for_tunnel_exits;


	/*
	 * To check the footprint, we need to set 4 quadrants of masks
	 * in case our rectangular footprint spans more than a single tile
	 */

	// This function handles tunnel I/O so initiate a few things
	check_for_tunnel_exits = ((dx == 0) && (d2y == 0));
	tunnel_toggle=false;

	if (in_tunnel)
		footprint = TUNNEL_FOOTPRINT;

	if (is_outside)
	{	// on compressed map
		room_x = CMP_MAP_WIDTH;
		room_y = CMP_MAP_HEIGHT;
	}
	else
	{	// in a room (inside)
		offset = readlong((u8*)fbuffer[ROOMS], OFFSETS_START+4*current_room_index);
		if (offset == 0xFFFFFFFF)
			return -1;
		room_y = readword((u8*)fbuffer[ROOMS], ROOMS_START+offset);
		offset +=2;
		room_x = readword((u8*)fbuffer[ROOMS], ROOMS_START+offset);
		offset +=2;		// remember offset is used in readtile/readexit and needs
						// to be constant from there on
	}

	// Compute the tile on which we try to stand
	px = prisoner_x + dx - (in_tunnel?16:0);
	p2y = prisoner_2y + 2*d2y - 1;
	tile_y = p2y / 32;
	tile_x = px / 32;
//	printf("org (x,y) = (%X,%X)\n", tile_x, tile_y);

	// check if we are trying to overflow our room left or up
	if ((px<0) || (p2y<0))
		return 0;

	// y and y+1
	for (i=0; i<2; i++)
	{
		// Set the left mask offset (tile_x, tile_y(+1)) index, converted to a long offset
		tile = readtile(tile_x, tile_y) + (in_tunnel?TUNNEL_TILE_ADDON:0);
		// Dunno why they reset the tile index for tunnels in the original game

		// Get the exit mask, if we stand on an exit
		// If we are not on an exit tile we'll use the empty mask from TILE_MASKS 
		// NB: This is why we add the ####_MASKS_STARTs here, as we might mix EXIT and TILE
		exit_offset[2*i] = MASK_EMPTY;

		for (u=0; u<NB_EXITS; u++)
		{
			if (readword((u8*)fbuffer[LOADER], EXIT_TILES_LIST + 2*u) == tile)
			{	
///				printf("1. got_exit %04X - offset = %04X (%d, %d) px = %d\n", tile << 7, readword((u8*)fbuffer[LOADER], EXIT_MASKS_OFFSETS+2*u),tile_x, tile_y, px);
				exit_offset[2*i] = EXIT_MASKS_START + 
					readword((u8*)fbuffer[LOADER], EXIT_MASKS_OFFSETS+2*u);
//				printf("1. exit_offset[%d] = EXIT_MASKS_START + %X\n", 2*i, readword((u8*)fbuffer[LOADER], EXIT_MASKS_OFFSETS+2*u));

				exit_dx[i] = 0;
				break;
			}
		}

		// Check for tunnel exits, and set the appropriate tool
		// NB: we need to do that even when not specifically checking for tunnel exits
		//     to make sure tunnel I/O tiles are walkable (MASK_FULL) as their default
		//     mask is not
		// TO_DO: fix the mask offsets for tunnel exits and speed up things by using check_for_tunnel_exits
		for (u=0; u<NB_TUNNEL_EXITS; u++)
		{
			if (readword((u8*)fbuffer[LOADER], TUNNEL_EXIT_TILES_LIST + 2*u) == tile)
			{	
				tunexit_tool[2*i] = readbyte((u8*)fbuffer[LOADER], TUNNEL_EXIT_TOOLS_LIST + 2*u + 1);
				break;
			}
		}

		if (u<IN_TUNNEL_EXITS_START)
		// a tunnel exit is always walkable (even open), as per the original game
		// NB: This is not necessary for within tunnel exits, where the default mask works fine
			mask_offset[2*i] = MASK_FULL;
		else
			// Regular
			mask_offset[2*i] = TILE_MASKS_START + readlong((u8*)fbuffer[LOADER], TILE_MASKS_OFFSETS+(tile<<2));

		// Set the upper right mask offset
		if ((px&0x1F) < 16)
		{	// we're on the left handside of the tile
			mask_offset[2*i+1] = mask_offset[2*i] + 2;	// Just shift 16 bits on the same tile
			exit_offset[2*i+1] = exit_offset[2*i] + 2;
		}
		else
		{	// right handside = > need to lookup the adjacent 
			// (tile_x+1, tile_y(+1)) mask
			mask_offset[2*i] += 2;	// first, we need to offset our first quadrant
			exit_offset[2*i] += 2;

			if ((tile_x+1) < room_x)
			{	// only read adjacent if it exists (i.e. < room_x)
				tile = readtile(tile_x+1, tile_y) + (in_tunnel?0x1E0:0);

				// Get the exit mask, if we stand on an exit
				exit_offset[2*i+1] = MASK_EMPTY;

				for (u=0; u<NB_EXITS; u++)
				{
					if (readword((u8*)fbuffer[LOADER], EXIT_TILES_LIST + 2*u) == tile)
					{	
///						printf("2. got_exit %04X - offset = %04X (%d, %d) px = %d\n", tile << 7, readword((u8*)fbuffer[LOADER], EXIT_MASKS_OFFSETS+2*u),tile_x, tile_y, px);
//						printf("got_exit %04X - offset = %04X\n", tile << 7, readword((u8*)fbuffer[LOADER], EXIT_MASKS_OFFSETS+2*u));
						exit_offset[2*i+1] = EXIT_MASKS_START + 
							readword((u8*)fbuffer[LOADER], EXIT_MASKS_OFFSETS+2*u);
//				printf("exit_offset[%d] = EXIT_MASKS_START + %X\n", 2*i+1, readword((u8*)fbuffer[LOADER], EXIT_MASKS_OFFSETS+2*u));

						exit_dx[i] = 1;
						break;
					}
				}

				// Check for tunnel exits, and set the appropriate tool
				for (u=0; u<NB_TUNNEL_EXITS; u++)
				{
					if (readword((u8*)fbuffer[LOADER], TUNNEL_EXIT_TILES_LIST + 2*u) == tile)
					{	
						tunexit_tool[2*i+1] = readbyte((u8*)fbuffer[LOADER], TUNNEL_EXIT_TOOLS_LIST + 2*u + 1);
						break;
					}
				}

				if (u<IN_TUNNEL_EXITS_START)
				// a tunnel exit is always walkable (even open), as per the original game
				// NB: This is not necessary for within tunnel exits, where the default mask works fine
					mask_offset[2*i+1] = MASK_FULL;
				else
					mask_offset[2*i+1] = TILE_MASKS_START + readlong((u8*)fbuffer[LOADER], TILE_MASKS_OFFSETS+(tile<<2));
			}
			else	
			{
				exit_offset[2*i+1] = MASK_EMPTY;
				mask_offset[2*i+1] = MASK_EMPTY;
			}
		}
		tile_y++;	// process lower tiles
	}

//	for (i=0; i<4; i++)
//		printf("exit_offset[%d] = %08X\n", i, exit_offset[i]);

	// OK, now we have our 4 mask offsets
	mask_y = (p2y & 0x1E)<<1;	// one mask line is 4 bytes (and p2y is already 2*py)

	mask_offset[0] += mask_y;	// start at the right line
	mask_offset[1] += mask_y;

	exit_offset[0] += mask_y;	// start at the right line
	exit_offset[1] += mask_y;

	footprint >>= (px & 0x0F);	// rotate our footprint according to our x pos
///	printf("%s %s [%d]\n", to_binary(footprint), to_binary(footprint), (px&0x1f));


	// Check if we are standing on a tunnel exit and set the global variables accordingly
	// If a tunnel exit tool is set, we have a winner
	if (check_for_tunnel_exits)
	{
		tunnel_toggle = true;
		for (u=0; u<3; u++)
		{
			if (check_for_tunnel_exits && (tunexit_tool[u] != ITEM_NONE))
			{
				// The line below defines the boundaries we'll use for tunnel exits
				// I'd say we do a much better job than in the original game, as exiting 
				// tunnels was a complete pain there
				if (
					 ( ( (!in_tunnel) && (((u == 0) && (px%32 <24 )) || ((u == 1) && (px%32 >=24))) ) ||
					   ( ( in_tunnel) && ((u == 0) || (u == 2)) ) ) && (is_fire_pressed)
					)
				{
					// We need to spare the exit offset value
					// NB1: tile_y was incremented twice in the main for loop
					// NB2: exit_flags_offset is a global that will be used in toggle_exit()
					exit_flags_offset = get_exit_offset(tile_x+(u%2),tile_y-2+(u/2));
					exit_flags = readbyte(fbuffer[is_inside?ROOMS:TUNNEL_IO], exit_flags_offset);
					// as for regular exits, because we use tunexit_nr as a boolean, we start at +1
					exit_nr = readexit(tile_x+(u%2),tile_y-2+(u/2)) + 1;
	//				printf("setting tunexit_nr = %x, tunexit_flags = %x, tunnel_tool = %d\n", tunexit_nr, tunexit_flags, tunnel_tool);

					if (!(exit_flags & 0x10))
					{	// Exit is closed => check for the right prop
						if ( (opt_keymaster) || (selected_prop[current_nation] == tunexit_tool[u]) )
						{	// Toggle the exit open and consume the relevant item
							consume_prop();		// doesn't consume if opt_keymaster
							show_prop_count();

							toggle_exit(exit_nr-1);
							if (in_tunnel) 
								// If we're in a tunnel and used the shovel, we exit directly
								return exit_nr;
							else
								// return 0 as we don't want to switch room just yet
								return 0;
						}
					}
					else if ( (opt_keymaster) || (in_tunnel) ||
							  (selected_prop[current_nation] == ITEM_CANDLE) )
					{	// Exit is open and we're all set to get through it
						if (!in_tunnel)
						{
							// Only consume the candle (because of the if !opt_keymaster in fn)
							consume_prop();
							show_prop_count();
						}
						return exit_nr;
					}
				}
				break;
			}
		}
		// No need to push it further if we are only checking for tunnels
		return 0;
	}

	// Not tunnel I/O => we check collisions and regular exits for multiple py's
	for (i=0; i<FOOTPRINT_HEIGHT; i++)
	{
		tile_mask = to_long(readword((u8*)fbuffer[LOADER], mask_offset[0]),	
			readword((u8*)fbuffer[LOADER], mask_offset[1]));

		exit_mask = to_long(readword((u8*)fbuffer[LOADER], exit_offset[0]),
			readword((u8*)fbuffer[LOADER], exit_offset[1]));


///	printf("%s ",to_binary(exit_mask));
///	printf("%s\n", to_binary(tile_mask));
//		printf("%08X\n",exit_mask);


		// see low_level.h for the collisions macros
		if inverted_collision(footprint,tile_mask)
		{
			// we have an exit perhaps
			if (collision(footprint,exit_mask))
			{
				// We need to spare the exit offset value
				exit_flags_offset = get_exit_offset(tile_x+exit_dx[0],tile_y-2);
				exit_flags = readbyte(fbuffer[ROOMS], exit_flags_offset);

				// Is the exit open?
				if ((!(exit_flags & 0x10)) && (exit_flags & 0x60))
				{	// exit is closed
					if (is_fire_pressed)
					{
						if ((opt_keymaster) || 
							// do we have the right key selected
							(selected_prop[current_nation] == ((exit_flags & 0x60) >> 5)))
						{
							// enqueue the door opening animation
							exit_nr = (u8) readexit(tile_x+exit_dx[0],tile_y-2) & 0x1F;
							// The trick is we use currently_animated[] to store our door sids
							// even if not yet animated, so that we can quickly access the
							// right animation data, rather than exhaustively compare tiles
							sid = currently_animated[exit_nr];
							ani_offset = 0;
							switch(sid)
							{	// let's optimize this a bit
							case 0x76:	// door_left
								ani_offset += 0x02;		// +2 because of door close ani
							case 0x78:	// door right
								ani_offset += 0x02;
							case 0x71:	// horizontal door 
								ani_offset += DOOR_HORI_OPEN_ANI;
								currently_animated[exit_nr] = nb_animations;
								animations[nb_animations].index = ani_offset;
								animations[nb_animations].framecount = 0;
								animations[nb_animations].end_of_ani_function = &toggle_exit;
								animations[nb_animations].end_of_ani_parameter = exit_nr;
								safe_nb_animations_increment();
								break;
							default:	// not an exit we should animate
								// just enqueue the toggle exit event
								enqueue_event(&toggle_exit, exit_nr, 3*ANIMATION_INTERVAL);
								break;
							}
							consume_prop();
						}
						// For now, the exit is still closed, so we return failure to progress further
						return 0;
					}
					else
					{
						// Display the key grade message
						set_status_message(fbuffer[LOADER] + readlong(fbuffer[LOADER], EXIT_MESSAGE_BASE + 
							((exit_flags & 0x60) >> 3)), 2, NO_MESSAGE_TIMEOUT);
						// Return failure if we can't exit
						return 0;
					}
				}

				// +1 as exits start at 0
				return(readexit(tile_x+exit_dx[0],tile_y-2)+1);
			}
//			gotit = 0;
			return 0;
		}
		mask_y+=4;
		// Do we need to change tile in y?
		for (u=0;u<2;u++)
		{
			if (mask_y == 0x40)
			{	// went over the tile boundary
				// => replace upper mask offsets with lower
				mask_offset[u] = mask_offset[u+2];
				exit_offset[u] = exit_offset[u+2];
				// We need an array for dx as we may have 2 exits on opposite quadrants (room 118 for instance)
				exit_dx[0] = exit_dx[1];
			}
			else
			{	// just scroll one mask line down
				mask_offset[u] +=4;
				exit_offset[u] +=4;
			}
		}
	}
//	return gotit;
	return -1;
}

bool check_guard_footprint(u8 g, s16 dx, s16 d2y)
{
	u32 tile, tile_mask, offset=0;
	u32 footprint = SPRITE_FOOTPRINT;
	u16 room_x, room_y;
	u16 mask_y;
	// maks offsets for upper-left, upper-right, lower-left, lower_right tiles
	u32 mask_offset[4];	// tile boundary
	s16 tile_x, tile_y;
	u8 i,u;
	s16 gx, g2y;

	/*
	 * To check the footprint, we need to set 4 quadrants of masks
	 * in case our rectangular footprint spans more than a single tile
	 */

	if (guard(g).room == ROOM_OUTSIDE)
	{	// on compressed map
		room_x = CMP_MAP_WIDTH;
		room_y = CMP_MAP_HEIGHT;
	}
	else
	{	// in a room (inside)
		offset = readlong((u8*)fbuffer[ROOMS], OFFSETS_START+4*guard(g).room);
		if (offset == 0xFFFFFFFF)
			return -1;
		room_y = readword((u8*)fbuffer[ROOMS], ROOMS_START+offset);
		offset +=2;
		room_x = readword((u8*)fbuffer[ROOMS], ROOMS_START+offset);
		offset +=2;		// remember offset is used in readtile/readexit and needs
						// to be constant from there on
	}

	// Compute the tile on which we try to stand
	gx = guard(g).px + dx - 16;
	g2y = guard(g).p2y + 2*d2y - 1 -4;
	tile_y = g2y / 32;
	tile_x = gx / 32;
//	printf("org (x,y) = (%X,%X)\n", tile_x, tile_y);

	// check if we are trying to overflow our room left or up
	if ((gx<0) || (g2y<0))
		return 0;

	// y and y+1
	for (i=0; i<2; i++)
	{
		// Set the left mask offset (tile_x, tile_y(+1)) index, converted to a long offset
		tile = readtile(tile_x, tile_y);
		mask_offset[2*i] = TILE_MASKS_START + readlong((u8*)fbuffer[LOADER], TILE_MASKS_OFFSETS+(tile<<2));

		// Set the upper right mask offset
		if ((gx&0x1F) < 16)
		// we're on the left handside of the tile
			mask_offset[2*i+1] = mask_offset[2*i] + 2;	// Just shift 16 bits on the same tile
		else
		{	// right handside = > need to lookup the adjacent 
			// (tile_x+1, tile_y(+1)) mask
			mask_offset[2*i] += 2;	// first, we need to offset our first quadrant

			if ((tile_x+1) < room_x)
			{	// only read adjacent if it exists (i.e. < room_x)
				tile = readtile(tile_x+1, tile_y);
				mask_offset[2*i+1] = TILE_MASKS_START + readlong((u8*)fbuffer[LOADER], TILE_MASKS_OFFSETS+(tile<<2));
			}
			else	
				mask_offset[2*i+1] = MASK_EMPTY;
		}
		tile_y++;	// process lower tiles
	}


	// OK, now we have our 4 mask offsets
	mask_y = (g2y & 0x1E)<<1;	// one mask line is 4 bytes (and g2y is already 2*gy)

	mask_offset[0] += mask_y;	// start at the right line
	mask_offset[1] += mask_y;

	footprint >>= (gx & 0x0F);	// rotate our footprint according to our x pos
///	printf("%s %s [%d]\n", to_binary(footprint), to_binary(footprint), (px&0x1f));

	// Not tunnel I/O => we check collisions and regular exits for multiple py's
	for (i=0; i<FOOTPRINT_HEIGHT; i++)
	{
		tile_mask = to_long(readword((u8*)fbuffer[LOADER], mask_offset[0]),	
			readword((u8*)fbuffer[LOADER], mask_offset[1]));

///	printf("%s\n", to_binary(tile_mask));

		// see low_level.h for the collisions macros
		if inverted_collision(footprint,tile_mask)
			return false;

		mask_y+=4;
		// Do we need to change tile in y?
		for (u=0;u<2;u++)
		{
			if (mask_y == 0x40)
				// went over the tile boundary
				// => replace upper mask offsets with lower
				mask_offset[u] = mask_offset[u+2];
			else
				// just scroll one mask line down
				mask_offset[u] +=4;
		}
	}
	return true;
}



void switch_room(s16 exit_nr, bool tunnel_io)
{
	u32 offset;
	u16 exit_index;	// exit index in destination room
	u16 room_x, room_y, tile_data;
	s16 tile_x, tile_y;
	u32 u;
	bool found;
	s16 pixel_x, pixel_y;
	u8  bit_index;

	// Let's get through
	if (is_outside)
	{	// If we're on the compressed map, we need to read 2 words (out of 4)
		// from beginning of the ROOMS_MAP file
		offset = exit_nr << 3;	// skip 8 bytes
		current_room_index = readword((u8*)fbuffer[tunnel_io?TUNNEL_IO:ROOMS], offset) & 0x7FF;
		exit_index = readword((u8*)fbuffer[tunnel_io?TUNNEL_IO:ROOMS], offset+2);
	}
	else
	{	// indoors => read from the ROOMS_EXIT_BASE data
		exit_index = (exit_nr&0xF)-1;
		offset = current_room_index << 4;
		// Now the real clever trick here is that the exit index of the room you 
		// just left and the exit index of the one you go always match.
		// Thus, we know where we should get positioned on entering the room
		current_room_index = readword((u8*)fbuffer[ROOMS], ROOMS_EXITS_BASE + offset 
			+ 2*exit_index);
	}

	// Since we're changing room, reset all animations
//	for (u=0; u<MAX_CURRENTLY_ANIMATED; u++)
//		currently_animated[u] = 0;
	// We should always keep at least the animation for our guy
//	nb_animations = 1;
//	nb_animations = 0;
	init_animations = true;
	
	exit_index++;	// zero based to one based
//	printf("          to room[%X] (exit_index = %d)\n", current_room_index, exit_index);

	// OK, we have now officially changed room, but we still need to position our guy
	if (current_room_index & 0x8000)	// MSb from ROOMS_EXIT_BASE data means going out
										// anything else is inside
	{	// going outside
		room_x = CMP_MAP_WIDTH;		// keep our readtile macros happy

		// If we're outside, we need to set the removable mask according to our data's MSB
		bit_index = (current_room_index >> 8) & 0x7C;
		rem_bitmask = readlong((u8*)fbuffer[LOADER],  REMOVABLES_MASKS_START + bit_index);

		// Now, use the tile index (LSB) as an offset to our (x,y) pos
		// NB: The ground floor rooms are in [00-F8]
		offset = current_room_index & 0xF8;
		tile_y = readword((u8*)fbuffer[tunnel_io?TUNNEL_IO:ROOMS], offset+4);
		tile_x = readword((u8*)fbuffer[tunnel_io?TUNNEL_IO:ROOMS], offset+6);

		// Now that we're done, switch to our actual outbound marker
		current_room_index = ROOM_OUTSIDE;

		// Finally, we need to adjust our pos, through the rabbit offset table
		// But only if we're not doing tunnel_io
		if (!tunnel_io)
		{
			tile_data = ((readtile(tile_x,tile_y) & 0xFF) << 1) - 2;	// first exit tile is 1, not 0
			offset = readword((u8*)fbuffer[LOADER], CMP_RABBIT_OFFSET + tile_data);
		}
	}
	else
	{	// going inside, or still inside
		// Get the room dimensions
		offset = readlong((u8*)fbuffer[ROOMS], OFFSETS_START+4*current_room_index);
		room_y = readword((u8*)fbuffer[ROOMS], ROOMS_START+offset);
		offset +=2;
		room_x = readword((u8*)fbuffer[ROOMS], ROOMS_START+offset);
		offset +=2;

		// Read the tiles data
		found = false;	// easier this way, as tile_x/y won't need adjusting
		for (tile_y=0; (tile_y<room_y)&&(!found); tile_y++)
		{
			for(tile_x=0; (tile_x<room_x)&&(!found); tile_x++)
			{
				tile_data = readword((u8*)fbuffer[ROOMS], ROOMS_START+offset);
				if ((tile_data & 0xF) == exit_index)
				{
					found = true;
					break;
				}
				offset +=2;		// Read next tile
			}
			if (found)
				break;
		}

		if (!found)
		{	// Better exit than go LHC and create a black hole
			perr("Error: Exit lookup failed\n");
			ERR_EXIT;
		}

		// We have our exit position in tiles. Now, depending 
		// on the exit type, we need to add a small position offset
		if (!tunnel_io)
		{	// but only if we're not doing a tunnel io
			tile_data &= 0xFF80;
			offset = 0;		// Should never fail (famous last words). Zero in case it does
			for (u=0; u<NB_CELLS_EXITS; u++)
			{
				if (readword((u8*)fbuffer[LOADER], EXIT_CELLS_LIST + 2*u) == tile_data)
				{	
					offset = readword((u8*)fbuffer[LOADER], HAT_RABBIT_OFFSET + 2*u);
					break;
				}
			}
		}
	}

	// Read the pixel adjustment
	if (!tunnel_io)
	{
		pixel_x = (s16)(readword((u8*)fbuffer[LOADER], HAT_RABBIT_POS_START + offset+2));
		pixel_y = (s16)(readword((u8*)fbuffer[LOADER], HAT_RABBIT_POS_START + offset));
	}
	else if (prisoner_state&STATE_TUNNELING)
	{	// Entering a tunnel
		pixel_x = 20;
		pixel_y = -32;
	}
	else
	{	// Exiting a tunnel
		pixel_x = 16;
		pixel_y = -32; //2;
	}

	prisoner_x = tile_x*32 + pixel_x; 
	prisoner_2y = tile_y*32 + 2*pixel_y + 2*0x20 - 2; 

	// Don't forget to (re)set the room props
	set_room_props();
}


// The next 2 functions are called while displaying the go to / released from
// solitary static screen, so that the prisoner position has been switched
// when we fade in on the game
void go_to_jail(u32 p)
{
	int prop;

	guy(p).state &= ~STATE_IN_PURSUIT;
	guy(p).state |= STATE_IN_PRISON;

	// Make sure the jail doors are closed when we leave the prisoner in!
	writebyte(fbuffer[ROOMS], solitary_cells_door_offset[p][0], 
		readbyte(fbuffer[ROOMS], solitary_cells_door_offset[p][0]) & 0xEF);
	writebyte(fbuffer[ROOMS], solitary_cells_door_offset[p][1], 
		readbyte(fbuffer[ROOMS], solitary_cells_door_offset[p][1]) & 0xEF);

	// Set our guy in the cell
	guy(p).room = readword(fbuffer[LOADER],SOLITARY_POSITION_BASE+8*p);
	guy(p).p2y = 2*readword(fbuffer[LOADER],SOLITARY_POSITION_BASE+8*p+2)-2;
	guy(p).px = readword(fbuffer[LOADER],SOLITARY_POSITION_BASE+8*p+4)-2;
	if (!opt_keymaster)
	{	// Bye bye props!
		for (prop = 0; prop<NB_PROPS; prop++)
			props[p][prop] = 0;
		// display the empty box
		selected_prop[p] = 0;
		// Strip of guard uniform if any
		if (guy(p).is_dressed_as_guard)
		{	// Only makes sense if we're dressed as guard
			guy(p).is_dressed_as_guard = false;
			guy(p).reset_animation = true;
		}
	}

	// Don't forget to (re)set the room props
	set_room_props();
}


void out_of_jail(u32 p)
{
	guy(p).state &= ~STATE_IN_PRISON;

	guy(p).px = readword(fbuffer[LOADER],INITIAL_POSITION_BASE+10*p+2);
	guy(p).p2y = 2*readword(fbuffer[LOADER],INITIAL_POSITION_BASE+10*p);
	guy(p).room = readword(fbuffer[LOADER],INITIAL_POSITION_BASE+10*p+4);

	// Don't forget to (re)set the room props
	set_room_props();
}

// Have a look at what our prisoner are doing
// After pic is a boolean indicating that a static picture event has just
// been acknowledged
void check_on_prisoners()
{
	int p;
	u16 i;
	bool authorized_id;
	u8 room_desc_id;
	int game_over;

	// Check escape condition (for current prisoner only)
	if ( (!has_escaped) && (current_room_index == ROOM_OUTSIDE) && 
		 ( (prisoner_x < ESCAPE_MIN_X) || (prisoner_x > ESCAPE_MAX_X) ||
		   (prisoner_2y < (2*ESCAPE_MIN_Y)) || (prisoner_2y > (2*ESCAPE_MAX_Y)) ) )
	{
		if (props[current_nation][ITEM_PAPERS])
		{
			nb_escaped++;
			if (nb_escaped >= NB_NATIONS)
			{
				static_screen(PRISONER_FREE_ALL_TEXT, NULL, 0);
				static_screen(PRISONER_FREE_ALL, NULL, 0);
			}
			else
			{
				static_screen(PRISONER_FREE, NULL, 0);
				p_event[current_nation].escaped = true;
			}
		}
		else
		{
			static_screen(REQUIRE_PAPERS, NULL, 0);
			p_event[current_nation].to_solitary = true;
		}
	}

	game_over = 0;
	// Game over condition
	for(p=0; p<NB_NATIONS; p++)
	{
		if ( (p_event[p].killed) || (p_event[p].escaped) || 
			 (guy(p).state & STATE_IN_PRISON) )
			 game_over++;
	}
	if (game_over == NB_NATIONS)
	{
		static_screen(GAME_OVER_TEXT, NULL, 0);
		return;
	}


	for(p=0; p<NB_NATIONS; p++)
	{
		if ((p_event[p].escaped) || (p_event[p].killed))
			continue;
		else if (p_event[p].to_solitary)
		{	// Sent to jail
			static_screen(TO_SOLITARY, go_to_jail, p);
			p_event[p].to_solitary = false;
			// Start solitary countdown
			p_event[p].solitary_countdown = SOLITARY_DURATION;
		}
		else if (p_event[p].require_pass)
		{
			static_screen(REQUIRE_PASS, NULL, 0);
			p_event[p].require_pass = false;
			if (props[p][ITEM_PASS] != 0)
			{
				guy(p).state &= ~STATE_IN_PURSUIT;
//				selected_prop[p] = ITEM_PASS;		// Doing this is bothersome if
				consume_prop();						// we have to re-cycle in a hurry
//				show_prop_count();					// let's comment 2 lines out
				// If pass handling was successful, we reset the guard's route
				writelong(fbuffer[GUARDS], p_event[p].caught_by*MENDAT_ITEM_SIZE+0x0E, 
					readlong(fbuffer[GUARDS], p_event[p].caught_by*MENDAT_ITEM_SIZE+0x06));
				// You got our pass, now get lost (prevents multiple pass requests)
				guard(p_event[p].caught_by).px = GET_LOST_X;
				guard(p_event[p].caught_by).p2y = GET_LOST_Y;
				// This ensures that we'll reinstantiate the guard
				guard(p_event[p].caught_by).go_on = 0;
			}
			else
				p_event[p].to_solitary = true;
		}
		else if (p_event[p].display_shot)
		{
			static_screen(PRISONER_SHOT, NULL, 0);
			p_event[p].display_shot = false;
			p_event[p].killed = true;
		}
		else if (p_event[p].solitary_countdown)
		{	// Guy's in solitary => decrement counter
			p_event[p].solitary_countdown--;

			if (guy(p).room == readword(fbuffer[LOADER],SOLITARY_POSITION_BASE+8*p))
			{	// Still in his cell?
				if (p_event[p].solitary_countdown == 0)
				// "Freeeeeeeedom!"
					static_screen(FROM_SOLITARY, out_of_jail, p);
			}
			else
			{	// Jailbreak!
				// We'll keep showing the guy behind bars until the guards 
				// come to release (in which case pursuit mode is activated)
				if (p_event[p].solitary_countdown == 0)
					guy(p).state &= ~STATE_IN_PRISON;
				else
					guy(p).state |= STATE_IN_PURSUIT;
			}
		}
		else
		{	// Common motion checks

			// Check if we are authorised in our current pos		
			if (guy(p).room == ROOM_OUTSIDE)
				room_desc_id = COURTYARD_MSG_ID;
			else if (guy(p).room < ROOM_TUNNEL)
				room_desc_id = readbyte(fbuffer[LOADER], ROOM_DESC_BASE	+ guy(p).room);
			else
				room_desc_id = TUNNEL_MSG_ID;

			// Now that we have the room desc ID, we can check if it's in the 
			// currently authrorized list
			p_event[p].unauthorized = true;
			for (i=1; i<=readword(fbuffer[LOADER], authorized_ptr)+1; i++)
			{
				authorized_id = readword(fbuffer[LOADER], authorized_ptr+2*i);
				if (authorized_id == 0xFFFF)
					// prisoner's quarters
					authorized_id = readbyte(fbuffer[LOADER], AUTHORIZED_NATION_BASE+p);

				if (authorized_id == room_desc_id)
				{	// if there's a match, we're allowed here
					p_event[p].unauthorized = false;
					// Additional boundary check for courtyard
					if ( (guy(p).room == ROOM_OUTSIDE) && (
						 (guy(p).px < COURTYARD_MIN_X) || (guy(p).px > COURTYARD_MAX_X) ||
						 (guy(p).p2y < (2*COURTYARD_MIN_Y)) || (guy(p).p2y > (2*COURTYARD_MAX_Y)) ) )
						p_event[p].unauthorized = true;
					break;
				}
			}
		}

	}
}

// Looks like the original programmer found that some of the data files had issues
// but rather than fixing the files, they patched them in the loader... go figure!
void fix_files()
{
	u8 i;
	u32 mask;
	//00001C10                 move.b  #$30,(fixed_crm_vector).l ; '0'
	writebyte(fbuffer[ROOMS],FIXED_CRM_VECTOR,0x30);
	//00001C18                 move.w  #$73,(exits_base).l ; 's' ; fix room #0's exits
	writeword(fbuffer[ROOMS],ROOMS_EXITS_BASE,0x0073);
	//00001C20                 move.w  #1,(exits_base+2).l
	writeword(fbuffer[ROOMS],ROOMS_EXITS_BASE+2,0x0001);
	//00001C28                 move.w  #$114,(r116_exits+$E).l ; fix room #116's last exit (0 -> $114)
	writeword(fbuffer[ROOMS],ROOMS_EXITS_BASE+(0x116<<4),0x0114);
	//00001C30                 subq.w  #1,(obs_bin).l  ; number of obs.bin items (BB)
	//00001C30                                          ; 187 items in all...

	// OK, because we're not exactly following the exact exit detection routines from the original game
	// (we took some shortcuts to make things more optimized) upper stairs landings are a major pain in 
	// the ass to handle, so we might as well take this opportunity to do a little patching of our own...
	for (i=9; i<16; i++)
	{	// Offset 0x280 is the intermediate right stairs landing
		mask = readlong((u8*)fbuffer[LOADER], EXIT_MASKS_START + 0x280 + 4*i);
		// eliminating the lower right section of the exit mask seems to do the job
		mask &= 0xFFFF8000;
		writelong((u8*)fbuffer[LOADER], EXIT_MASKS_START + 0x280 + 4*i, mask);
	}

	// The 3rd tunnel removable masks for overlays on the compressed map were not designed 
	// for widescreen, and as such the tunnel will show open if we don't patch it.
	writebyte(fbuffer[LOADER], OUTSIDE_OVL_BASE+0x38+1, 0x05);

	// For some reason, they reset the tile index for tunnels, instead of starting at 
	// 0x1E0 (TUNNEL_TILE_ADDON) as they should have done. Because we use the more
	// logical approach for efficiency, we need to patch a couple of words
	writeword(fbuffer[LOADER], TUNNEL_EXIT_TILES_LIST+2*IN_TUNNEL_EXITS_START,
		readword(fbuffer[LOADER], TUNNEL_EXIT_TILES_LIST+2*IN_TUNNEL_EXITS_START) + TUNNEL_TILE_ADDON);
	writeword(fbuffer[LOADER], TUNNEL_EXIT_TILES_LIST+2*IN_TUNNEL_EXITS_START+2,
		readword(fbuffer[LOADER], TUNNEL_EXIT_TILES_LIST+2*IN_TUNNEL_EXITS_START+2) + TUNNEL_TILE_ADDON);


	// I've always wanted to have the stethoscope!
	writeword(fbuffer[OBJECTS],32,0x000E);
}

// Open and texturize an IFF image file. Modified from LBMVIEW V1.0b 
// http://www.programmersheaven.com/download/6394/download.aspx
bool load_iff(u8 iff_id)
{
	int i, y, bpl, bit_plane;
	char ch, cmp_type, color_depth;
	u8 uc, check_flags;
	u16	w, h;
	u32 id, len, l;
	u8  line_buf[8][512/8];	// bytes line buffers (8 bitplanes max, 512 pixels wide)

	if ((fd = fopen (iff_name[iff_id], "rb")) == NULL)
	{
		if (opt_verbose)
			perror ("fopen()");
		perr("Can't find file '%s'\n", iff_name[iff_id]);
		return false;
	}

	// Check for 'FORM' tag
	if (freadl(fd) != IFF_FORM)
	{
		fclose(fd);
		perr("IFF_FORM not found\n");
		return false;
	}

	// Skip IFF Form length
	freadl(fd);						

	// Check for InterLeaved BitMap tag
	if (freadl(fd) != IFF_ILBM)
	{
		fclose(fd);
		perr("IFF_ILBM not found\n");
		return false;
	}

	// Check for BitMap Header
	if (freadl(fd) != IFF_BMHD)
	{
		fclose(fd);
		perr("IFF_BMHD not found\n");
		return false;
	}

	// Check header length
	if (freadl(fd) != 20)
	{
		fclose(fd);
		perr("Bad IFF header length\n");
		return false;
	}

	// Read width and height
	w = freadw(fd);
	if (w > 512)
	{
		fclose(fd);
		perr("IFF width must be lower than 512\n");
		return false;
	}
	if (w & 0x7)
	{
		fclose(fd);
		perr("IFF width must be a multiple of 8\n");
		return false;
	}
	h = freadw(fd);
	if (h > PSP_SCR_HEIGHT)
	{
		fclose(fd);
		perr("IFF height must be lower than %d\n", PSP_SCR_HEIGHT);
		return false;
	}

	// Discard initial x and y pos
	freadw(fd);
	freadw(fd);

	// Read colour depth
	color_depth = freadc(fd);
	if (color_depth > 5)
	{
		fclose(fd);
		perr("IFF: Colour depth must be lower than 5\n");
		return false;
	}

	// Skip masking type
	freadc(fd);

	// Get compression type
	cmp_type = freadc(fd);
	if ((cmp_type != 0) && (cmp_type != 1))
	{
		fclose(fd);
		perr("Unknown IFF compression method\n");
		return false;
	}

	// Skip the bytes we're not interested in
	freadc(fd);				// skip unused field
	freadw(fd);				// skip transparent color
	freadc(fd);				// skip x aspect ratio
	freadc(fd);				// skip y aspect ratio
	freadw(fd);				// skip default page width
	freadw(fd);				// skip default page height

	check_flags = 0;

	do  // We'll use cycle to skip possible junk      
	{   //  chunks: ANNO, CAMG, GRAB, DEST, TEXT etc.
		id = freadl(fd);
		switch(id)
		{
		case IFF_CMAP:
			len = freadl(fd) / 3;
			for (l=0; l<len; l++)
			{
				aPalette[l]  = ((u16)freadc(fd) & 0xF0) << 4;	// Red
				aPalette[l] |= ((u16)freadc(fd) & 0xF0) << 8;	// Green
				aPalette[l] |= ((u16)freadc(fd) & 0xF0) >> 4;	// Blue
				aPalette[l] |= 0x00F0;							// Alpha
			}
			check_flags |= 1;				// flag "palette read" 
			break;

		case IFF_BODY:
			freadl(fd);						// skip BODY size 

			// Calculate bytes per line. As our width is always a multiple of 8
			// no special cases are needed
			bpl = w >> 3;  

			for (y = 0; y < h; y++)
			{
				for (bit_plane = 0; bit_plane < color_depth; bit_plane++)
				{
					if (cmp_type)
					{	// Compressed
						i = 0;
						while (i < bpl)
						{
							uc = freadc(fd);
							if (uc < 128)
							{
								uc++;
								fread(&line_buf[bit_plane][i], 1, uc, fd);
								i += uc;
							} else
								if (uc > 128)
								{
									uc = 257 - uc;
									ch = freadc(fd);
									memset(&line_buf[bit_plane][i], ch, uc);
									i += uc;
								}
								// 128 (0x80) means NOP - no operation  
						}
						// Set our image extra bytes to colour index 0
						memset(&line_buf[bit_plane][i], 0, (512/8)-i);
					}
					else
						// Uncompressed
						fread(&line_buf[bit_plane][0], 1, bpl, fd);
				}

				// OK, now we have our <color_depth> line buffers
				// Let's recombine those bits, and convert to GRAB from our palette
				line_interleaved_to_wGRAB((u8*)line_buf, 
					static_image_buffer+512*y*2, 512, 1, color_depth);

			}

			// We need to blank the extra padding we have, at least for
			// the PSP Screen height
			if (h < PSP_SCR_HEIGHT)
				memset(static_image_buffer+512*h*2, 0, (PSP_SCR_HEIGHT-h)*512*2);

			check_flags |= 2;       // flag "bitmap read" 
			break;

		default:					// Skip unused chunks  
			len = freadl(fd);		// nb of bytes to skip
			for (l=0; l<len; l++)
				freadc(fd);
		}

	// Exit from loop if we are at the end of file, 
	// or if both palette and bitmap have been loaded
	} while ((check_flags != 3) && (!feof(fd)));

	fclose(fd);
	fd = NULL;

	if (check_flags != 3)
	{
		if (check_flags & 2)
			aligned_free(b);
		return false;
	}

	// The iff is good => we can set our texture
	glBindTexture(GL_TEXTURE_2D, picture_texid);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 512, 512, 0, 
		GL_RGBA, GL_UNSIGNED_SHORT_4_4_4_4_REV, static_image_buffer);

	// Also don't forget to update the global picture dimension variables
	picture_w = iff_payload_w[iff_id];
	picture_h = iff_payload_h[iff_id];

	return true;
}

// Open and texturize an RGB RAW image 
bool load_raw_rgb(int w, int h, char* filename)
{

	if ((fd = fopen (filename, "rb")) == NULL)
	{
		if (opt_verbose)
			perror ("fopen()");
		perr("Can't find file '%s'\n", filename);
		return false;
	}

	if (fread (static_image_buffer, 1, 512*h*3, fd) != (512*h*3))
	{
		if (opt_verbose)
			perror ("fread()");
		perr("'%s': Unexpected file size or read error\n", filename);
		return false;
	}

	fclose (fd);
	fd = NULL;

	glBindTexture(GL_TEXTURE_2D, picture_texid);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 512, 512, 0,
		GL_RGB, GL_UNSIGNED_BYTE, static_image_buffer);

	picture_w = w;
	picture_h = h;


	return true;

}

void init_mod()
{
	Mod_Init();
}

void play_mod(char* mod_name)
{
	Mod_Load(mod_name);
	Mod_Play();
}

void stop_mod()
{
	Mod_Stop();
//	Mod_FreeTune();
}