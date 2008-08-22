/**
 **  Escape from Colditz
 **
 **  Utility functions
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
#elif defined(PSP)
#include <stdarg.h>
#include <pspkernel.h>
#include <pspdebug.h>
#include <pspgu.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glut.h>
#endif

#include "colditz.h"
#include "utilities.h"
#include "low-level.h"

extern GLuint* cell_texid;
extern GLuint* sprite_texid;

u8  obs_to_sprite[NB_OBS_TO_SPRITE];


void load_all_files()
{
	size_t read;
	u32 i;
	int compressed_loader = 0;

	for (i=0; i<NB_FILES; i++)
	{
		if ( (fbuffer[i] = (u8*) aligned_malloc(fsize[i], 16)) == NULL)
		{
			perr("Could not allocate buffers\n");
			ERR_EXIT;
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
}


// Get some properties (max/min/...) according to file data
void getProperties()
{
	u16 room_index;
	u32 ignore = 0;
	u32 offset;
	u8  i;

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
	print("nb_rooms = %X\n", nb_rooms);

	// A backdrop cell is exactly 256 bytes
	nb_cells = fsize[CELLS] / 0x100;
	cell_texid = malloc(sizeof(GLuint) * nb_cells);
	GLCHK(glGenTextures(nb_cells, cell_texid));
	print("nb_cells = %X\n", nb_cells);

	nb_sprites = readword(fbuffer[SPRITES],0) + 1;
	sprite_texid = malloc(sizeof(GLuint) * nb_sprites);
	GLCHK(glGenTextures(nb_sprites, sprite_texid));
	print("nb_sprites = %X\n", nb_sprites);

	nb_objects = readword(fbuffer[OBJECTS],0) + 1;
	print("nb_objects = %X\n", nb_objects);
	for (i=0; i<NB_OBS_TO_SPRITE; i++)
		obs_to_sprite[i] = readbyte(fbuffer[LOADER],OBS_TO_SPRITE_START+i);
}


// Convert an Amiga 12 bit RGB colour palette to 16 bit GRAB
void to_16bit_Palette(u8 palette_index)
{
	u32 i;
	u16 rgb, grab;

	int palette_start = palette_index * 0x20;

	// Read the palette
	if (opt_verbose)
		print("Using Amiga Palette index: %d\n", palette_index);


	for (i=0; i<16; i++)		// 16 colours
	{
		rgb = readword(fbuffer[PALETTES], palette_start + 2*i);
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
		grab |= 0x00F0;
		// 3) Set Green
		grab |= (rgb << 8) & 0xF000;
		// 4) Write in the palette
		aPalette[i] = grab;
	}
	if (opt_verbose)
		print("\n\n");
}


// Convert a 4 bit line-interleaved source to 16 bit RGBA (GRAB) destination
void line_interleaved_to_wGRAB(u8* source, u8* dest, u16 w, u16 h)
{
	u8 colour_index;
	u32 i,j,l,pos;
	int k;
	u32 wb;
	u8 line_byte[4];

	// the width of interest to us is the one in bytes.
	wb = w/8;

	// We'll write sequentially to the destination
	pos = 0;
	for (i=0; i<h; i++)
	{	// h lines to process
		for (j=0; j<wb; j++)
		{	// wb bytes per line
			for (k=0; k<4; k++)
				// Read one byte from each of the 4 lines (starting from max y for openGL)
				line_byte[3-k] = readbyte(source, 4*(wb*i) + k*wb + j);
			// Write 8 RGBA values
			for (k=0; k<8; k++)
			{
				colour_index = 0;
				// Get the palette colour index and rotate the line bytes
				for (l=0; l<4; l++)
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
		line_interleaved_to_wGRAB(source + (256*i), dest+(2*RGBA_SIZE*256*i), 32, 16);
		GLCHK(glBindTexture(GL_TEXTURE_2D, cell_texid[i]));
		GLCHK(glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 32, 16, 0, GL_RGBA, GL_UNSIGNED_SHORT_4_4_4_4_REV, 
			((u8*)rgbCells) + i*2*RGBA_SIZE*0x100));
	}

}


// Initialize the sprite array
void init_sprites()
{
	u32 index = 2;	// We need to ignore the first word (nb of sprites)
	u16 sprite_index = 0;
	u16 sprite_w;	// width, in words
	u32 sprite_address;

	// Allocate the sprites and overlay arrays
	sprite = aligned_malloc(nb_sprites * sizeof(s_sprite), 16);
	overlay = aligned_malloc(MAX_OVERLAY * sizeof(s_overlay), 16);

	// First thing we do is populate the sprite offsets at the beginning of the table
	sprite_address = index + 4* (readword(fbuffer[SPRITES],0) + 1);
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
}


// Converts the sprites to 16 bit GRAB data we can handle
void sprites_to_wGRAB()
{
	u16 sprite_index;
	u16 bitplane_size;
	u32 sprite_address;
	u8* sbuffer;
	u16 w,h;
	int no_mask = 0;

	for (sprite_index=0; sprite_index<nb_sprites; sprite_index++)
	{
		// Get the base in the original Colditz sprite file
		sprite_address = readlong(fbuffer[SPRITES],2+4*sprite_index);

		// if MSb is set, we have 4 bitplanes instead of 5
		w = readword(fbuffer[SPRITES],sprite_address);
		no_mask = w & 0x8000;
		w *= 2;		// width in bytes
		h = sprite[sprite_index].h;

		bitplane_size = readword(fbuffer[SPRITES],sprite_address+6);
		if (bitplane_size != w*h)
			print("sprites_to_wGRAB: Integrity check failure on bitplane_size\n");

		// Source address
		sbuffer = fbuffer[SPRITES] + sprite_address + 8; 

		if (no_mask)
			// Bitplanes that have no mask are line-interleaved, like cells
			line_interleaved_to_wGRAB(sbuffer, sprite[sprite_index].data, sprite[sprite_index].w, h);
		else
			bitplane_to_wGRAB(sbuffer, sprite[sprite_index].data, sprite[sprite_index].w,
				sprite[sprite_index].corrected_w, h);

		// Now that we have data in a GL readable format, let's texturize it!
		GLCHK(glBindTexture(GL_TEXTURE_2D, sprite_texid[sprite_index]));
		GLCHK(glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, sprite[sprite_index].corrected_w, 
			sprite[sprite_index].corrected_h, 0, GL_RGBA, GL_UNSIGNED_SHORT_4_4_4_4_REV,
			sprite[sprite_index].data));

	}
}


// Populates the overlay table
void set_overlays(int x, int y, u32 current_tile, u16 room_x)
{
	u16 tile1_data, tile2_data;
	u16 i;
	short sx, sy;
	u16 sid;	// sprite index

	// read current tile
	tile1_data = readword(fbuffer[ROOMS], current_tile) & 0xFF80;
	for (i=0; i<(12*NB_SPECIAL_TILES); i+=12)
	{
		if (readword(fbuffer[LOADER], SPECIAL_TILES_START+i) != tile1_data)
			continue;
		sx = readword(fbuffer[LOADER], SPECIAL_TILES_START+i+8);
		if (opt_debug)
			print("  match: %04X, direction: %04X\n", tile1_data, sx);
		if (i >= (12*(NB_SPECIAL_TILES-4)))
		// The four last special tiles are exits. We need to check is they are open
		{
			// Get the exit data (same tile if tunnel, 2 rows down in door)
			tile2_data = readword(fbuffer[ROOMS], current_tile + 
				(i==(12*(NB_SPECIAL_TILES-1)))?0:(4*room_x));
//			print("got exit: %04X\n", tile2_data);
			// Validity check
			if (!(tile2_data & 0x000F))
				print("set_overlays: Integrity check failure on exit tile\n");
			// if the tile is an exit and the exit is open
			if (tile2_data & 0x0010)
			{
				if (opt_debug)
					print("    exit open: ignoring overlay\n");
				// The second check on exits is always an FA00, thus we can safely
				break;
			}
		}
			 
		if (sx < 0)
			tile2_data = readword(fbuffer[ROOMS], current_tile-2) & 0xFF80;
		else
			tile2_data = readword(fbuffer[ROOMS], current_tile+2) & 0xFF80;
		// ignore if special tile that follows is matched
		if (readword(fbuffer[LOADER], SPECIAL_TILES_START+i+2) == tile2_data)
		{
			if (opt_debug)
				print("    ignored as %04X matches\n", tile2_data);
			continue;
		}
		sid = readword(fbuffer[LOADER], SPECIAL_TILES_START+i+4);
		overlay[overlay_index].sid = sid;
		if (opt_debug)
			print("    overlay as %04X != %04X => %X\n", tile2_data, readword(fbuffer[LOADER], SPECIAL_TILES_START+i+2), sid);
		sy = readword(fbuffer[LOADER], SPECIAL_TILES_START+i+6);
		if (opt_debug)
			print("    sx: %04X, sy: %04X\n", sx, sy);
		overlay[overlay_index].x = x + (int)sx - (int)sprite[sid].w + (int)(sprite[sid].x_offset);
		overlay[overlay_index].y = y + (int)sy - (int)sprite[sid].h + 1;
		overlay_index++;
		// No point in looking for overlays any further if we met our match 
		// UNLESS this is a double bed overlay, in which case the same tile
		// need to be checked for a double match (in both in +x and -x)
		if (tile1_data != 0xEF00)
			break;
	}
}


// Read the pickable objects from obs.bin
void set_objects()
{
	u16 i;

	for (i=0; i<(8*nb_objects); i+=8)
	{
		if (readword(fbuffer[OBJECTS],i+2) != current_room_index)
			continue;
		overlay[overlay_index].sid = obs_to_sprite[readword(fbuffer[OBJECTS],i+2+6)];
		overlay[overlay_index].x = gl_off_x + readword(fbuffer[OBJECTS],i+2+4) - 15;
		overlay[overlay_index].y = gl_off_y + readword(fbuffer[OBJECTS],i+2+2) - 3;
		if (opt_debug)
			print("  pickup object match: sid=%X\n", overlay[overlay_index].sid);
		overlay_index++;
	}
	
}


#define GLCHK1(x) {x;}
void displaySprite(float x, float y, float w, float h, GLuint texid) 
{
	float x1, x2, y1, y2;

	x1 = origin_x + x;
	x2 = origin_x + x + w;
	y1 = origin_y + y;
	y2 = origin_y + y + h;

	GLCHK1(glBindTexture(GL_TEXTURE_2D, texid));

	GLCHK1(glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
	GLCHK1(glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
	
	// If we don't set clamp, our tiling will show
#if defined(WIN32)
	// For some reason GL_CLAMP_TO_EDGE on Win achieves the same as GL_CLAMP on PSP
//	GLCHK1(glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
//	GLCHK1(glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));
	GLCHK1(glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP));
	GLCHK1(glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP));
#elif defined(PSP)
	GLCHK(glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP));
	GLCHK(glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP));
#endif 

	// pspgl does not implement QUADS
	GLCHK1(glBegin(GL_TRIANGLE_FAN));

//	GLCHK(glColor3f(1.0f, 0.0f, 0.0f));
	GLCHK(glTexCoord2f(0.0f, 0.0f));
	GLCHK(glVertex3f(x1, y1, 0.0f));

//	GLCHK(glColor3f(0.0f, 1.0f, 0.0f));
	GLCHK(glTexCoord2f(0.0f, 1.0f));
	GLCHK(glVertex3f(x1, y2, 0.0f));

//	GLCHK(glColor3f(0.0f, 0.0f, 1.0f));
	GLCHK(glTexCoord2f(1.0f, 1.0f));
	GLCHK(glVertex3f(x2, y2, 0.0f));

//	GLCHK(glColor3f(1.0f, 1.0f, 1.0f));
	GLCHK(glTexCoord2f(1.0f, 0.0f));
	GLCHK(glVertex3f(x2, y1, 0.0f));

	GLCHK1(glEnd());

}

// Display all our overlays
void display_overlays()
{
	u8 i;

	for (i=0; i<overlay_index; i++)
		displaySprite(overlay[i].x, overlay[i].y, sprite[overlay[i].sid].corrected_w, 
			sprite[overlay[i].sid].corrected_h, sprite_texid[overlay[i].sid]);
}

// Display room
void displayRoom(u16 room_index)
{
	u32 offset;					// Offsets to each rooms are given at
								// the beginning of the Rooms Map file
	u16 room_x, room_y, tile_data;
	int tile_x, tile_y;
	int pixel_x, pixel_y;

	// Read the offset
	offset = readlong((u8*)fbuffer[ROOMS], OFFSETS_START+4*room_index);
	if (opt_verbose)
		print("\noffset[%03X] = %08X ", room_index, (uint)offset);
	if (offset == 0xFFFFFFFF)
	{
		// For some reason there is a break in the middle
		if (opt_verbose)
			print("\n  IGNORED");
		return;
	}

	// Now that we have the offset, let's look at the room

	// The 2 first words are the room Y and X dimension (in tiles),
	// in that order
	room_y = readword((u8*)fbuffer[ROOMS], ROOMS_START+offset);
	offset +=2;
	room_x = readword((u8*)fbuffer[ROOMS], ROOMS_START+offset);
	offset +=2;
	if (opt_verbose)
		print("(room_x=%X,room_y=%X)\n", room_x, room_y);
	gl_off_x = (gl_width - (room_x*32))/2 - prisoner_x;
	gl_off_y = (gl_height - (room_y*16))/2 - prisoner_y;

	// reset room overlays
	overlay_index = 0;

	// Before we do anything, let's set the pickable objects in
	// our overlay table (so that room overlays go on top of 'em)
	set_objects();

	// Read the tiles data
	for (tile_y=0; tile_y<room_y; tile_y++)
	{
		if (opt_verbose)
			print("    ");	// Start of a line
		for(tile_x=0; tile_x<room_x; tile_x++)
		{
			pixel_x = gl_off_x+tile_x*32;
			pixel_y = gl_off_y+tile_y*16;


			// A tile is 32(x)*16(y)*4(bits) = 256 bytes
			// A specific room tile is identified by a word
			tile_data = readword((u8*)fbuffer[ROOMS], ROOMS_START+offset);

			displaySprite(pixel_x,pixel_y,32,16, 
				cell_texid[(tile_data>>7) + ((room_index>0x202)?0x1E0:0)]);

			// Display sprite overlay
			set_overlays(pixel_x, pixel_y, ROOMS_START+offset, room_x);

			offset +=2;		// Read next tile

			if (opt_verbose)
				print("%04X ", tile_data);
		}
		if (opt_verbose)
			print("\n");
	}

	if (opt_debug)
		print("\n");


	// Let's add our guy
//	overlay[overlay_index].sid = 0x85;	// tunnel cover
	overlay[overlay_index].sid = prisoner_sid;	// tunnel cover

	overlay[overlay_index].x = PSP_SCR_WIDTH/2; // - origin_x;
	overlay[overlay_index++].y = PSP_SCR_HEIGHT/2; // - origin_y;
//	overlay[overlay_index].x = gl_off_x + room_x*16;
//	overlay[overlay_index++].y = gl_off_y + room_y*8;

	// Now that the background is done, and that we have the overlays, display the overlay sprites
	display_overlays();

//	displaySprite(100,100,32,16,cell_texid[4]);
//	displaySprite(32,0,32,16,cell_texid[5]);


}

