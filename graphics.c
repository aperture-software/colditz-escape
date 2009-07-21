/**
 **  Escape from Colditz
 **
 **  Display functions
 **
 **/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(WIN32)
#include <windows.h>
#include <gl/gl.h>
#include <gl/glu.h>
#include <gl/glut.h>
#endif
#if defined(PSP)
#include <stdarg.h>
#include <pspkernel.h>
#include <pspdebug.h>
#include <pspgu.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glut.h>
#include <psp/psp-printf.h>
#endif

#include "data-types.h"
#include "low-level.h"
#include "colditz.h"
#include "eschew.h"
#include "conf.h"
#include "graphics.h"
#include "game.h"
#if defined(ANTI_TAMPERING_ENABLED)
#include "md5.h"
#endif

// varibles common to game & graphics
extern u8	remove_props[CMP_MAP_WIDTH][CMP_MAP_HEIGHT];
extern u8  overlay_order[MAX_OVERLAYS];
extern int	currently_animated[MAX_ANIMATIONS];
extern u16 room_x, room_y;
extern s16 tile_x, tile_y;
extern u32 offset;
#if defined(ANTI_TAMPERING_ENABLED)
extern u8  fmdxhash[NB_FILES][5];
#endif

// Whatever you do, you don't want local variables holding textures
GLuint* cell_texid;
GLuint* sprite_texid;
GLuint* chars_texid;
GLuint render_texid;
GLuint paused_texid[4];

s16	gl_off_x = 0, gl_off_y  = 0;	// GL display offsets
u8* background_buffer;				// (re)used for static pictures
u8  pause_rgb[3];					// colour for the pause screen borders

// Fatigue bar base sprite colours (4_4_4_4 GRAB)
u16 fatigue_colour[8] = {0x29F0, 0x4BF0, 0x29F0, 0x06F0, 
	0x16F1, GRAB_TRANSPARENT_COLOUR, GRAB_TRANSPARENT_COLOUR, GRAB_TRANSPARENT_COLOUR};

// For the outside map, because of the removable sections and the fact that
// having a removable section set for the tile does not guarantee that the 
// prop should be hidden (eg: bottom of a removable wall), we'll define a
// set of tile where props should always appear. We're actually defining 4
// prop disaplayable quadrants, deliminated by an X in the tile, with each 
// bit for a quadrant, starting with the left quadrant (lsb) and going
// clockwise
// For the time being, we're not going to use these, but just set the prop
// 'on' if nonzero, coz it's a bit of an overkill, compared to what the 
// original game did. Still this is an improvement on the original game's
// props handling, as if you dropped a prop in front of the chapel there
// it would simply vanish!

u16 props_tile [0x213] = {
// Well, C doesn't have binary constants, so, for maintainability reasons, 
// we'll use fake "decimal binary", which we're gonna convert to proper 
// bitmasks at init time. That is, if we're ever gonna use the actual masks...
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
	1111 };

// We'll use this 4x8 GRAB sprite as an indicator for guards fooled by a pass
// Can't use it directly as it needs to be 16 bit aligned on PSP
// Also padded to 8x8 because goddam PSP can't use anything less
u16 fooled_by_sprite[8][8] = {
	{0xbdf7,0xbdf7,0xbdf7,0xbdf7,0xff0f,0xff0f,0xff0f,0xff0f},
	{0xbdf7,0xbdf7,0x09f0,0xbdf7,0xff0f,0xff0f,0xff0f,0xff0f},
	{0xbdf7,0xbdf7,0xbdf7,0xbdf7,0xff0f,0xff0f,0xff0f,0xff0f},
	{0xbdf7,0x9bf5,0xbdf7,0xbdf7,0xff0f,0xff0f,0xff0f,0xff0f},
	{0xbdf7,0x9bf5,0xbdf7,0xbdf7,0xff0f,0xff0f,0xff0f,0xff0f},
	{0xbdf7,0x9bf5,0xbdf7,0xbdf7,0xff0f,0xff0f,0xff0f,0xff0f},
	{0xbdf7,0xbdf7,0xbdf7,0xbdf7,0xff0f,0xff0f,0xff0f,0xff0f},
	{0xff0f,0xff0f,0xff0f,0xff0f,0xff0f,0xff0f,0xff0f,0xff0f}
};	

// Set the gloabl textures properties
void set_textures()
{
	int	i;

	// Allocate the main background buffer, which is not tied to any file but shared
	// to display static images, etc...
	// The maximum size for our static image textures is 512x256x2 (IFF) or 512x512x3
	// (RAW RGB) bytes
	// NOTE that because we don't want to waste the 512x(512-272)x3 (=360 KB) extra
	// space on the PSP (the PSP can only deal with texture dimensions that are in 
	// power of twos), we are doing something really dodgy here, which is that we
	// pretend the extra 360K have been allocated, and let the system blatantly 
	// overflow our image buffer if it wants to (since it should never display that 
	// data anyway). Doesn't seem to be too much of an issue...
	// 
	// 
	// If you're really unhappy about this kind of practice, just allocate 512x512x3
	background_buffer = (u8*) aligned_malloc(512*PSP_SCR_HEIGHT*3,16);
//	static_image_buffer = (u8*) aligned_malloc(512*512*3,16);	
	if (background_buffer == NULL)
		printf("Could not allocate buffer for static images display\n");

	// Now that we have that buffer alloc'd, we can set the textures that will make 
	// use of it. This includes all the IFFs
	for (i=0; i<NB_IFFS; i++)
		texture[i].buffer = background_buffer;

	// Setup the backdrop cells
	// A backdrop cell is exactly 256 bytes (32*16*4bits)
	nb_cells = fsize[CELLS] / 0x100;
	cell_texid = malloc(sizeof(GLuint) * nb_cells);
	glGenTextures(nb_cells, cell_texid);

	if (readword(fbuffer[SPRITES],0) != (NB_STANDARD_SPRITES-1))
	{
		printf("Unexpected number of sprites\n");
		ERR_EXIT;
	}

	sprite_texid = malloc(sizeof(GLuint) * NB_SPRITES);
	glGenTextures(NB_SPRITES, sprite_texid);

	chars_texid = malloc(sizeof(GLuint) * NB_PANEL_CHARS);
	glGenTextures(NB_PANEL_CHARS, chars_texid);

	// Setup textures for the zoom and paused function 
	glGenTextures(1, &render_texid);
	for (i=0; i<4; i++)
		glGenTextures(1, &paused_texid[i]);

	// Load the panel & corner textures
	load_texture(&texture[PANEL_BASE1]);
	load_texture(&texture[PANEL_BASE2]);
	load_texture(&texture[PICTURE_CORNER]);
}


// Convert an Amiga 12 bit RGB colour palette to 16 bit GRAB
void to_16bit_palette(u8 palette_index, u8 transparent_index, u8 io_file)
{
	u32 i;
	u16 rgb, grab;

	int palette_start = palette_index * 0x20;

	// Read the palette
	if (opt_verbose)
		printf("Using Amiga Palette index: %d\n", palette_index);


	for (i=0; i<16; i++)		// 16 colours
	{
		rgb = readword(fbuffer[io_file], palette_start + 2*i);
		if (opt_verbose)
		{
			printf(" %03X", rgb); 
			if (i==7)
				printf("\n");
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
		printf("\n\n");
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
// to 16 bit GRAB destination
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
		glBindTexture(GL_TEXTURE_2D, cell_texid[i]);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 32, 16, 0, GL_RGBA, GL_UNSIGNED_SHORT_4_4_4_4_REV, 
			((u8*)rgbCells) + i*2*RGBA_SIZE*0x100);
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
		glBindTexture(GL_TEXTURE_2D, chars_texid[c]);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, PANEL_CHARS_W, 8, 0, GL_RGBA, 
			GL_UNSIGNED_SHORT_4_4_4_4_REV, (u8*)panel_chars[c]);
	}

}

// Get properties for panel sprites
s_panel_sprite get_panel_sprite(u16 sprite_index)
{
	static s_panel_sprite sp;

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
//		printf("sprite[%X] address = %08X\n", sprite_index, sprite_address);
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
//		printf("  w,h = %0X, %0X\n", sprite[sprite_index].w , sprite[sprite_index].h);
	}

	// Manual correction for the dying prisoners last animations, as these animations
	// are wider, and we use bottom-left instead of bottom right as origin
	sprite[0xab].x_offset = -16;
	sprite[0xac].x_offset = -16;
	sprite[0xaf].x_offset = -16;
	sprite[0xb0].x_offset = -16;

#if defined(ANTI_TAMPERING_ENABLED)
	if (!integrity_check(OBJECTS))
	{
		printf("Program failure\n");
		exit(1);
	}
#endif

	// We add the panel (nonstandard) sprites at the end of our exitsing sprite array
	for (sprite_index=NB_STANDARD_SPRITES; sprite_index<NB_SPRITES-NB_EXTRA_SPRITES; sprite_index++)
	{
		sprite[sprite_index].w = get_panel_sprite(sprite_index).w;
		sprite[sprite_index].corrected_w = sprite[sprite_index].w;
		sprite[sprite_index].h = 16;
		sprite[sprite_index].corrected_h = 16;
		sprite[sprite_index].x_offset = 1;
		sprite[sprite_index].data = aligned_malloc( RGBA_SIZE * 
			sprite[sprite_index].w * sprite[sprite_index].h, 16);
	}

	// We define the last nonstandard as sprite base for the fatigue bar
	sprite[PANEL_FATIGUE_SPRITE].w = 8;
	sprite[PANEL_FATIGUE_SPRITE].corrected_w = 8;
	sprite[PANEL_FATIGUE_SPRITE].h = 8;
	sprite[PANEL_FATIGUE_SPRITE].corrected_h = 8;
	sprite[PANEL_FATIGUE_SPRITE].x_offset = 1;
	sprite[PANEL_FATIGUE_SPRITE].data = aligned_malloc( RGBA_SIZE * 
			sprite[PANEL_FATIGUE_SPRITE].w * sprite[PANEL_FATIGUE_SPRITE].h, 16);

	// And one more for the fooled by one
	sprite[FOOLED_BY_SPRITE].w = 8;
	sprite[FOOLED_BY_SPRITE].corrected_w = 8;
	sprite[FOOLED_BY_SPRITE].h = 8;
	sprite[FOOLED_BY_SPRITE].corrected_h = 8;
	sprite[FOOLED_BY_SPRITE].x_offset = 0;
	sprite[FOOLED_BY_SPRITE].data = aligned_malloc( RGBA_SIZE * 
			sprite[FOOLED_BY_SPRITE].w * sprite[FOOLED_BY_SPRITE].h, 16);


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

	for (sprite_index=0; sprite_index<NB_SPRITES-NB_EXTRA_SPRITES; sprite_index++)
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
			{	// we're getting into panel overlays => switch to the panel palette
				// but before we do that, save the RGB index for the pause screen borders
				// Index 10 is the current border color
				pause_rgb[RED] = ((aPalette[10]>>8)&0xF)*0x11;
				pause_rgb[GREEN] = ((aPalette[10]>>12)&0xF)*0x11;
				pause_rgb[BLUE] = (aPalette[10]&0xF)*0x11;
				to_16bit_palette(0, 1, SPRITES_PANEL);
			}
			sbuffer = fbuffer[SPRITES_PANEL] + get_panel_sprite(sprite_index).offset + 
				8*sprite[sprite_index].w*(sprite_index-get_panel_sprite(sprite_index).base);
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
		glBindTexture(GL_TEXTURE_2D, sprite_texid[sprite_index]);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, sprite[sprite_index].corrected_w, 
			sprite[sprite_index].corrected_h, 0, GL_RGBA, GL_UNSIGNED_SHORT_4_4_4_4_REV,
			sprite[sprite_index].data);
	}

	// The fatigue and fooled_by sprites are initialized manually
	for (y=0; y<8; y++)
		for (x=0; x<8; x++)
			writeword(sprite[PANEL_FATIGUE_SPRITE].data, 16*y+2*x, fatigue_colour[y]);

	glBindTexture(GL_TEXTURE_2D, sprite_texid[PANEL_FATIGUE_SPRITE]);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, sprite[PANEL_FATIGUE_SPRITE].corrected_w,
		sprite[PANEL_FATIGUE_SPRITE].corrected_h, 0, GL_RGBA, GL_UNSIGNED_SHORT_4_4_4_4_REV,
		sprite[PANEL_FATIGUE_SPRITE].data);

	for (y=0; y<8; y++)
		for (x=0; x<8; x++)
			writeword(sprite[FOOLED_BY_SPRITE].data, 16*y+2*x, fooled_by_sprite[y][x]);

	glBindTexture(GL_TEXTURE_2D, sprite_texid[FOOLED_BY_SPRITE]);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, sprite[FOOLED_BY_SPRITE].corrected_w, 
		sprite[FOOLED_BY_SPRITE].corrected_h, 0, GL_RGBA, GL_UNSIGNED_SHORT_4_4_4_4_REV,
		sprite[FOOLED_BY_SPRITE].data);
}


// Display a sprite or cell, using the top left corner as the origin
static __inline void display_sprite(float x1, float y1, float w, float h, unsigned int texid) 
{
	float register x2, y2;

	x2 = x1 + w;
	y2 = y1 + h;

	glBindTexture(GL_TEXTURE_2D, texid);
	// Don't modify pixel colour ever
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

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


// That's right: we define 2 different display_sprite routines just
// to spare the processing time of an if condition for linear/nearest!
void display_sprite_linear(float x1, float y1, float w, float h, unsigned int texid) 
{
	float register x2, y2;

	x2 = x1 + w;
	y2 = y1 + h;

	glBindTexture(GL_TEXTURE_2D, texid);
	// Do linear interpolation. Looks better, but if you zoom, you have to zoom
	// the whole colour buffer, else the sprite seams will show
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

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

// Display room
void display_room()
{
// OK, I'll spare you the suspense: this is NOT optimized as hell!
// We are redrawing ALL the tiles and ALL overlays, for EACH FRAME!
// Yup, no scrolling or anything: just plain unoptimized brute force...
// But hey, the PSP can handle it, and so should a decent PC, so why bother?

	u16 tile_data;
	u32 raw_data;
	u16 rem_offset;
	s16 min_x, max_x, min_y, max_y;
	u16 tile_tmp, nb_tiles;
	u8  bit_index;
	s16 pixel_x, pixel_y;
	int u;

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

	// This sets the room_x, room_y and offset values
	set_room_xy(current_room_index);

	// No readtile() macros used here, for speed
	if (is_inside)
	{	// Standard room (inside)

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
				tile_data = readword((u8*)fbuffer[ROOMS], offset);

				display_sprite(pixel_x,pixel_y,32,16, 
					cell_texid[(tile_data>>7) + ((current_room_index>0x202)?0x1E0:0)]);

				// Display sprite overlay
				crm_set_overlays(pixel_x, pixel_y, tile_data & 0xFF80); 

				offset +=2;		// Read next tile
				pixel_x += 32;
			}
			pixel_y += 16;
		}


	}
	else
	{	// on compressed map (outside)

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
					nb_tiles = readword((u8*)fbuffer[COMPRESSED_MAP], CMP_TILES_START+rem_offset);
					// The rest of the data is a tile index (FF80), a bit index (1F), and 2 bits unused.
					// the later being used to check bits of an overlay bitmap longword
					for (u=nb_tiles; u!=0; u--)
					{
						tile_tmp = readword((u8*)fbuffer[COMPRESSED_MAP], CMP_TILES_START+rem_offset + 2*u);
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
	glBindTexture(GL_TEXTURE_2D, texture[current_picture].texid);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

	// The image
	glBegin(GL_TRIANGLE_FAN);
		glTexCoord2f(0.0f, 0.0f);
		glVertex2f((PSP_SCR_WIDTH-texture[current_picture].w)/2, (PSP_SCR_HEIGHT-texture[current_picture].h)/2);
		glTexCoord2f(1.0f, 0.0f);
		glVertex2f(512 + (PSP_SCR_WIDTH-texture[current_picture].w)/2, (PSP_SCR_HEIGHT-texture[current_picture].h)/2);
		glTexCoord2f(1.0, 1.0);
		glVertex2f(512 + (PSP_SCR_WIDTH-texture[current_picture].w)/2, 
			powerize(texture[current_picture].h) + (PSP_SCR_HEIGHT-texture[current_picture].h)/2);
		glTexCoord2f(0.0, 1.0);
		glVertex2f((PSP_SCR_WIDTH-texture[current_picture].w)/2, 
			powerize(texture[current_picture].h) + (PSP_SCR_HEIGHT-texture[current_picture].h)/2);
	glEnd();
}


// Display the game panel
void display_panel()
{
	float w, h;
	u16 i, sid;

	// Black rectangle (panel base) at the bottom
	glDisable(GL_TEXTURE_2D);		// Disable textures and set colour to black
	glColor3f(0.0f, 0.0f, 0.0f);	

	glBegin(GL_TRIANGLE_FAN);
		glVertex2f(0, PSP_SCR_HEIGHT-32);
		glVertex2f(PSP_SCR_WIDTH, PSP_SCR_HEIGHT-32);
		glVertex2f(PSP_SCR_WIDTH, PSP_SCR_HEIGHT);
		glVertex2f(0, PSP_SCR_HEIGHT);
	glEnd();

	// Restore for texturing
	glColor3f(fade_value, fade_value, fade_value);
	glEnable(GL_TEXTURE_2D);

	// Draw the 2 parts of our panel
 	glBindTexture(GL_TEXTURE_2D, texture[PANEL_BASE1].texid);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

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

	glBindTexture(GL_TEXTURE_2D, texture[PANEL_BASE2].texid);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

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

	// Because the original game wasn't designed for widescreen we have to
	// diagonally crop the area to keep some elements hidden
	// We either use textured picture corners for that, or black triangles
	if (opt_picture_corners)
	{	// Picture corners
		// Note that if you don't want to slow the game on PSP, the corner.raw
		// file that contains the texture MUST be 4 bit RGBA. If using standard
		// 8 bit RGBA, the blending will slow us down big time!
		w = (float) powerize(texture[PICTURE_CORNER].w);
		h = (float) powerize(texture[PICTURE_CORNER].h);

		glBindTexture(GL_TEXTURE_2D, texture[PICTURE_CORNER].texid);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

		// upper left
		glBegin(GL_TRIANGLE_FAN);
			glTexCoord2f(0.0f, 0.0f);
			glVertex2f(0.0f, 0.0f);
			glTexCoord2f(1.0f, 0.0f);
			glVertex2f(w, 0.0f);
			glTexCoord2f(1.0f, 1.0f);
			glVertex2f(w, h);
			glTexCoord2f(0.0f, 1.0f);
			glVertex2f(0.0f, h);
		glEnd();

		// upper right
		glBegin(GL_TRIANGLE_FAN);
			glTexCoord2f(0, 0);
			glVertex2f(PSP_SCR_WIDTH-0, 0);
			glTexCoord2f(1, 0);
			glVertex2f(PSP_SCR_WIDTH-w, 0);
			glTexCoord2f(1, 1);
			glVertex2f(PSP_SCR_WIDTH-w, h);
			glTexCoord2f(0, 1);
			glVertex2f(PSP_SCR_WIDTH-0, h);
		glEnd();

		// bottom right
		glBegin(GL_TRIANGLE_FAN);
			glTexCoord2f(0, 0);
			glVertex2f(PSP_SCR_WIDTH-0, PSP_SCR_HEIGHT-0);
			glTexCoord2f(1, 0);
			glVertex2f(PSP_SCR_WIDTH-w, PSP_SCR_HEIGHT-0);
			glTexCoord2f(1, 1);
			glVertex2f(PSP_SCR_WIDTH-w, PSP_SCR_HEIGHT-h);
			glTexCoord2f(0, 1);
			glVertex2f(PSP_SCR_WIDTH-0, PSP_SCR_HEIGHT-h);
		glEnd();

		// bottom left
		glBegin(GL_TRIANGLE_FAN);
			glTexCoord2f(0, 0);
			glVertex2f(0, PSP_SCR_HEIGHT-0);
			glTexCoord2f(1, 0);
			glVertex2f(w, PSP_SCR_HEIGHT-0);
			glTexCoord2f(1, 1);
			glVertex2f(w, PSP_SCR_HEIGHT-h);
			glTexCoord2f(0, 1);
			glVertex2f(0, PSP_SCR_HEIGHT-h);
		glEnd();
	}
	else
	{	// Use black triangles
		glDisable(GL_TEXTURE_2D);
		glColor3f(0.0f, 0.0f, 0.0f);	

		h = (float) (28-NORTHWARD_HO)+36;	// 36
		w = (float) 2*h;					// 72

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

		glColor3f(fade_value, fade_value, fade_value);
		glEnable(GL_TEXTURE_2D);
	}

	// Display our guys' faces
	for (i=0; i<4; i++)
	{
		if (guy(i).state & STATE_SHOT)
			sid = PANEL_FACE_SHOT;
		else if (p_event[i].escaped)
			sid = PANEL_FACE_FREE;
		else 
		{
			sid = 0xd5 + i;
			if ((guy(i).state & STATE_IN_PRISON) ||
				((guy(i).state & STATE_IN_PURSUIT) && ((game_time/1000)%2)))
				sid = PANEL_FACE_IN_PRISON;
		}
		display_sprite(PANEL_FACES_X+i*PANEL_FACES_W, PANEL_TOP_Y,
			sprite[sid].w, sprite[sid].h, sprite_texid[sid]);
	}

	// Display the currently selected nation's flag
	display_sprite(PANEL_FLAGS_X, PANEL_TOP_Y, sprite[PANEL_FLAGS_BASE_SID+current_nation].w,
		sprite[PANEL_FLAGS_BASE_SID+current_nation].h, sprite_texid[PANEL_FLAGS_BASE_SID+current_nation]);

	// Display the clock
	// (Unlike the original game, I like having the zero displayed on hour tens, always)
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

	// Display the currently selected prop
	sid = selected_prop[current_nation] + PANEL_PROPS_BASE;
	display_sprite(PANEL_PROPS_X, PANEL_TOP_Y,
			sprite[sid].w, sprite[sid].h, sprite_texid[sid]);

	// Display the fatigue bar
	display_sprite(PANEL_FATIGUE_X, PANEL_FATIGUE_Y,
		(prisoner_fatigue>>0xB), sprite[PANEL_FATIGUE_SPRITE].h, sprite_texid[PANEL_FATIGUE_SPRITE]);

	// Display close by prop or motion indicator
	if (over_prop_id)
		sid = over_prop_id + PANEL_PROPS_BASE;
	else
	{
		if (prisoner_state & STATE_TUNNELING)
			sid = STATE_CRAWL_SID;
		else if (prisoner_state & STATE_STOOGING)
			sid = STATE_STOOGE_SID;
		else
			sid = (prisoner_speed == 1)?STATE_WALK_SID:STATE_RUN_SID;
	}
	display_sprite(PANEL_STATE_X, PANEL_TOP_Y,
		sprite[sid].w, sprite[sid].h, sprite_texid[sid]);

	// Display the current status message
	display_message(status_message);
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


	if ((gl_width != PSP_SCR_WIDTH) && (gl_height != PSP_SCR_HEIGHT))
	{	
		glDisable(GL_BLEND);	// Better than having to use glClear()

		// First, we copy the whole buffer into a texture
		glBindTexture(GL_TEXTURE_2D, render_texid);
		glCopyTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 0, 0, PSP_SCR_WIDTH, PSP_SCR_HEIGHT, 0);

		// Then we change our viewport to the actual screen size
		glViewport(0, 0, gl_width, gl_height);

		// Now we change the projection, to the new dimensions
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
	    glOrtho(0, gl_width, gl_height, 0, -1, 1);

		// OK, now we can display the whole texture
		display_sprite(0, gl_height, gl_width, -gl_height, render_texid);

		// Finally, we restore the parameters
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		glOrtho(0, PSP_SCR_WIDTH, PSP_SCR_HEIGHT, 0, -1, 1);
		glViewport(0, 0, PSP_SCR_WIDTH, PSP_SCR_HEIGHT);

		glEnable(GL_BLEND);	// We'll need blending for the sprites, etc.

	}
}


// The couple of routines below handle the 4-way split pause screen
void create_pause_screen()
{
#define SPACER	8
	int i, restore_nation, restore_fade;
	float x,y,w,h;

	restore_fade = fade_value;
	fade_value = 1.0f;
	restore_nation = current_nation;
	w = powerize(PSP_SCR_WIDTH/2 - 2*SPACER);
	h = powerize(PSP_SCR_HEIGHT/2 - 2*SPACER);
	x = PSP_SCR_WIDTH/4 + SPACER;
	y = PSP_SCR_HEIGHT/4 + NORTHWARD_HO + 16 + SPACER;
	for (i=0; i<NB_NATIONS; i++)
	{
		current_nation = i;
		prisoner_state &= ~(STATE_MOTION|STATE_ANIMATED);
		prisoner_reset_ani = true;
		t_status_message_timeout = 0;
		status_message_priority = 0;
		set_room_props();
		glClear(GL_COLOR_BUFFER_BIT);
		display_room();
		// Copy the section of interest into one of our four paused textures
		glBindTexture(GL_TEXTURE_2D, paused_texid[i]);
		glCopyTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, x, y, w, h, 0);
	}
	current_nation = restore_nation;
	fade_value = restore_fade;
}

void display_pause_screen()
{
	int i, j;
	static float x_shift[2] = {7.0f, -7.0f};
	static float y_shift[2] = {7.0f, -7.0f};
	float x,y,w,h;
	w = PSP_SCR_WIDTH/2 - 2*SPACER;
	h = PSP_SCR_HEIGHT/2 - 2*SPACER;

	for (i=0; i<NB_NATIONS; i++)
	{
		j = (2*(i+1) + i/2)%4;	// we need to display in 2, 0, 3, 1 order because of texture 
								// overlap correction due to having to use powerized ones on PSP
		x = (j%2)*(PSP_SCR_WIDTH/2) + SPACER + x_shift[j%2] + 0.5;
		y = (j/2)*(PSP_SCR_HEIGHT/2) + (PSP_SCR_HEIGHT/2) - SPACER + y_shift[j/2] + 0.5;
		glDisable(GL_BLEND);	// textures won't appear on PSP without blend disabled (but works on Windows)
		display_sprite(x, y, powerize(w), -powerize(h), paused_texid[j]);
		glEnable(GL_BLEND);

		// Draw the border
		glDisable(GL_TEXTURE_2D);	// F...ing openGL a...oles!!! Why is it that as soon as you have textures
									// Enabled, you cannot draw plain COLOURED primitives any longer
		glColor3ub(pause_rgb[RED], pause_rgb[GREEN], pause_rgb[BLUE]);
		glBegin(GL_LINE_STRIP);		// doesn't look like pspGL handles LINE_LOOP
			glVertex2f(x, y);
			glVertex2f(x+w, y);
			glVertex2f(x+w, y-h);		
			glVertex2f(x, y-h);
			glVertex2f(x, y);
		glEnd();
		glColor3f(fade_value, fade_value, fade_value);
		glEnable(GL_TEXTURE_2D);
	}

	// now hide the overlap up and right, and between frames
	glDisable(GL_TEXTURE_2D);
	glColor3f(0.0f, 0.0f, 0.0f);

	// A couple of lines for in between frames
	glBegin(GL_LINES);
		glVertex2f(x-1, 0);
		glVertex2f(x-1, PSP_SCR_HEIGHT);
		glVertex2f(0, y+1);
		glVertex2f(PSP_SCR_WIDTH, y+1);
	glEnd();

	// And 2 black rectangles for up and right
	glBegin(GL_TRIANGLE_FAN);
		glVertex2f(0, 0);
		glVertex2f(PSP_SCR_WIDTH, 0);
		glVertex2f(PSP_SCR_WIDTH, y-h);
		glVertex2f(0, y-h);
	glEnd();

	glBegin(GL_TRIANGLE_FAN);
		glVertex2f(PSP_SCR_WIDTH-2*SPACER+2, 0);
		glVertex2f(PSP_SCR_WIDTH, 0);
		glVertex2f(PSP_SCR_WIDTH, PSP_SCR_HEIGHT);
		glVertex2f(PSP_SCR_WIDTH-2*SPACER+2, PSP_SCR_HEIGHT);
	glEnd();

	// Restore colour and stuff
	glColor3f(fade_value, fade_value, fade_value);
	glEnable(GL_TEXTURE_2D);
}




// Open and texturize an IFF image file. Modified from LBMVIEW V1.0b 
// http://www.programmersheaven.com/download/6394/download.aspx
bool load_iff(s_tex* tex)
{
	int i, y, bpl, bit_plane;
	char ch, cmp_type, color_depth;
	u8 uc, check_flags;
	u16	w, h, powerized_w;
	u32 id, len, l;
	u8  line_buf[8][512/8];	// bytes line buffers (8 bitplanes max, 512 pixels wide max)

	powerized_w = powerize(tex->w);	// Should be 512 always, but let's be generic here
	if (powerized_w > 512)
	{	// Because of line_buf above
		printf("Texture width must be lower than 512\n");
		return false;
	}

	if ((fd = fopen (tex->filename, "rb")) == NULL)
	{
		printf("Can't find file '%s'\n", tex->filename);
		return false;
	}

	// Check for 'FORM' tag
	if (freadl(fd) != IFF_FORM)
	{
		fclose(fd);
		printf("IFF_FORM not found\n");
		return false;
	}

	// Skip IFF Form length
	freadl(fd);						

	// Check for InterLeaved BitMap tag
	if (freadl(fd) != IFF_ILBM)
	{
		fclose(fd);
		printf("IFF_ILBM not found\n");
		return false;
	}

	// Check for BitMap Header
	if (freadl(fd) != IFF_BMHD)
	{
		fclose(fd);
		printf("IFF_BMHD not found\n");
		return false;
	}

	// Check header length
	if (freadl(fd) != 20)
	{
		fclose(fd);
		printf("Bad IFF header length\n");
		return false;
	}

	// Read width and height
	w = freadw(fd);
	if (w > 512)
	{
		fclose(fd);
		printf("IFF width must be lower than 512\n");
		return false;
	}
	if (w & 0x7)
	{
		fclose(fd);
		printf("IFF width must be a multiple of 8\n");
		return false;
	}
	h = freadw(fd);
	if (h > PSP_SCR_HEIGHT)
	{
		fclose(fd);
		printf("IFF height must be lower than %d\n", PSP_SCR_HEIGHT);
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
		printf("IFF: Colour depth must be lower than 5\n");
		return false;
	}

	// Skip masking type
	freadc(fd);

	// Get compression type
	cmp_type = freadc(fd);
	if ((cmp_type != 0) && (cmp_type != 1))
	{
		fclose(fd);
		printf("Unknown IFF compression method\n");
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
						memset(&line_buf[bit_plane][i], 0, (powerized_w/8)-i);
					}
					else
						// Uncompressed
						fread(&line_buf[bit_plane][0], 1, bpl, fd);
				}

				// OK, now we have our <color_depth> line buffers
				// Let's recombine those bits, and convert to GRAB from our palette
				line_interleaved_to_wGRAB((u8*)line_buf, 
					tex->buffer+powerized_w*y*2, powerized_w, 1, color_depth);

			}

			// We need to blank the extra padding we have, at least for
			// the PSP Screen height
			if (h < PSP_SCR_HEIGHT)
				memset(tex->buffer+powerized_w*h*2, 0, (PSP_SCR_HEIGHT-h)*powerized_w*2);

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
		return false;

	// The iff is good => we can set our texture
	glBindTexture(GL_TEXTURE_2D, tex->texid);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, powerized_w, powerize(tex->h),
		0, GL_RGBA, GL_UNSIGNED_SHORT_4_4_4_4_REV, tex->buffer);

	return true;
}


// Open and texturize a 16, 24 or 32 bpp RGB(A) RAW image 
bool load_raw_rgb(s_tex* tex, u8 pixel_size)
{
	u32 line_size, powerized_line_size;
	int i;
	u8* line_start;

	line_size = pixel_size*(tex->w);
	powerized_line_size = pixel_size*powerize(tex->w);

	if ((fd = fopen (tex->filename, "rb")) == NULL)
	{
		printf("Can't find file '%s'\n", tex->filename);
		return false;
	}

	line_start = tex->buffer;
	for (i=0; i<(tex->h); i++)
	{
		if (fread (line_start, 1, line_size, fd) != line_size)
		{
			printf("'%s': Read error while reading line %d\n", tex->filename, i);
			return false;
		}
		if (line_size < powerized_line_size)
			memset(line_start+line_size, 0, powerized_line_size-line_size);
		line_start += powerized_line_size;
	}

	fclose(fd);

	glBindTexture(GL_TEXTURE_2D, tex->texid);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	switch (pixel_size)
	{
	case 4:	// 8 bpp RGBA
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, powerize(tex->w), powerize(tex->h),
			0, GL_RGBA, GL_UNSIGNED_BYTE, tex->buffer);
		break;
	case 3:	// 8 bpp RGB
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, powerize(tex->w), powerize(tex->h),
			0, GL_RGB, GL_UNSIGNED_BYTE, tex->buffer);
		break;
	case 2: // 4 bpp GRAB
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, powerize(tex->w), powerize(tex->h),
			0, GL_RGBA, GL_UNSIGNED_SHORT_4_4_4_4_REV, tex->buffer);
		break;
	default:
		printf("load_raw_rgb: unexpected pixel size\n");
		return false;
		break;
	}
	
	return true;
}


// Load a texture from a file (RAW or IFF)
bool load_texture(s_tex* tex)
{
bool iff_file = false;
bool with_alpha = false;
bool is_rgb4 = false;
u32  file_size = 0;
u8   pixel_size = 0;	// in bytes
u16  powerized_w;

	// closest greater power of 2
	powerized_w = powerize(tex->w);
	
	// Make sure we have a valid texture
	if (tex->texid == 0)
		glGenTextures(1, &(tex->texid));

	// Does the file exist
	if ((fd = fopen (tex->filename, "rb")) == NULL)
	{
		printf("Can't find file '%s'\n", tex->filename);
		return false;
	}

	// Check if the file's an IFF
	if (freadl(fd) == IFF_FORM)
	{	// IFF file
		iff_file = true;
	}
	else	 
	{	// RAW file

		// Find out if there is an alpha channel to read by computing the size
	    fseek(fd, 0, SEEK_END);
		file_size = ftell(fd);

		pixel_size = file_size/((tex->w)*(tex->h));
		switch(pixel_size)
		{
		case 2:	// 4 bpp GRAB
			is_rgb4 = true;
			with_alpha = true;
			break;
		case 3:	// 8 bpp RGB
			is_rgb4 = false;
			with_alpha = false;
			break;
		case 4: // 8 bpp RGBA
			is_rgb4 = false;
			with_alpha = true;
			break;
		default:
			printf("Improper RAW file size for %s\n", tex->filename);
			return false;
			break;
		}

		if (pixel_size*(tex->w)*(tex->h) != file_size)
		{
			printf("Improper RAW file size for %s\n", tex->filename);
			return false;
		}
	}

	// We'll reopen the file in the load_iff/load_raw_rgb functions
	fclose(fd);

	// Let's fill our buffer and texturize then
	if (iff_file)
	{
		if (tex->buffer == NULL)
		{
			printf("Texture buffer for IFF file %s has not been initialized\n", tex->filename);
			return false;
		}
		if (!load_iff(tex))
			return false;
	}
	else
	{
		if (tex->buffer == NULL)
		{
			tex->buffer = aligned_malloc(pixel_size*powerized_w*powerize(tex->h), 16);
			if (tex->buffer == NULL)
			{
				printf("Could not allocate buffer for texture %s\n", tex->filename);
				return false;
			}
		}
		if (!load_raw_rgb(tex, pixel_size))
			return false;
	}
	return true;
}


