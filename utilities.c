/**
 **  Colditz Maps Explorer
 **
 **  Utility functions
 **
 **/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include <gl/gl.h>
#include <gl/glu.h>
#include <gl/glut.h>
#include "colditz.h"
#include "utilities.h"

// Some global variable specific to utilities
int  underflow_flag = 0;
u32	compressed_size, checksum;
u8  obs_to_sprite[NB_OBS_TO_SPRITE];


/* The handy ones, in big endian mode */
u32 readlong(u8* buffer, u32 addr)
{
	return ((((u32)buffer[addr+0])<<24) + (((u32)buffer[addr+1])<<16) +
		(((u32)buffer[addr+2])<<8) + ((u32)buffer[addr+3]));
}

void writelong(u8* buffer, u32 addr, u32 value)
{
	buffer[addr]   = (u8)(value>>24);
	buffer[addr+1] = (u8)(value>>16);
	buffer[addr+2] = (u8)(value>>8);
	buffer[addr+3] = (u8)value;
}

u16 readword(u8* buffer, u32 addr)
{
	return ((((u16)buffer[addr+0])<<8) + ((u16)buffer[addr+1]));
}

void writeword(u8* buffer, u32 addr, u16 value)
{
	buffer[addr]   = (u8)(value>>8);
	buffer[addr+1] = (u8)value;
}


u8 readbyte(u8* buffer, u32 addr)
{
	return buffer[addr];
}

void writebyte(u8* buffer, u32 addr, u8 value)
{
	buffer[addr] = value;
}


// Prints a line of text on the top right corner
void glutPrintf(int line, const char *fmt, ...) 
{
	char		text[256];			// Holds Our String
	va_list		ap;					// Pointer To List Of Arguments
	int			w, h;
	char*		t;
	float		length;				// text length, in pixels
	const float coef = 0.09f;

	// Parses The String For Variables
	if (fmt==0) return;
	va_start(ap, fmt);	
	vsprintf(text, fmt, ap);
	va_end(ap);

    glMatrixMode(GL_PROJECTION);
	glPushMatrix();
    glLoadIdentity();
    w = glutGet(GLUT_WINDOW_WIDTH);
    h = glutGet(GLUT_WINDOW_HEIGHT);

	// Set up a 2d ortho projection that matches the window
    gluOrtho2D(-w/2,w/2,-h/2,h/2);
	// Get the pixel length
	length = glutStrokeLength(GLUT_STROKE_MONO_ROMAN, text)*coef;
	// Move text to top right corner
	glTranslatef(w/2-length-2, h/2-(line+1)*16.0f, 0.0f);  
    glScalef(coef, coef, 1.0f);
    glColor3f(1.0f, 1.0f, 1.0f);	// White colour
	glPushMatrix();	
	// Render each character
    t = text;
	while (*t) 
		glutStrokeCharacter(GLUT_STROKE_MONO_ROMAN, *t++); 
	// Apply the various transformations
    glPopMatrix();		
    glPopMatrix(); 
}


// Get one bit and read ahead if needed
u32 getbit(u32 *address, u32 *data)
{
	// Read one bit and rotate data
	u32 bit = (*data) & 1;
	(*data) >>= 1;
	if ((*data) == 0)
	{	// End of current bitstream? => read another longword
		(*data) = readlong(mbuffer, *address);
		checksum ^= (*data);
		if (opt_debug)
			printf("(-%X).l = %08X\n",(compressed_size-*address+LOADER_DATA_START+8), *data);
		(*address)-=4;
		// Lose the 1 bit marker on read ahead
		bit = (*data) & 1; 
		// Rotate data and introduce a 1 bit marker as MSb
		// This to ensure that zeros in high order bits are processed too
		(*data) = ((*data)>>1) | 0x80000000;
	}
	return bit;
}

// Get sequence of streamsize bits (in reverse order)
u32 getbitstream(u32 *address, u32 *data, u32 streamsize)
{
	u32 bitstream = 0;
	u32 i;
	for (i=0; i<streamsize; i++)
		bitstream = (bitstream<<1) | getbit (address, data);
	return bitstream;
}

// Decrement address by one byte and check for buffer underflow
void decrement(u32 *address)
{
	if (underflow_flag)
		printf("uncompress(): Buffer underflow error.\n");
	if ((*address)!=0)
		(*address)--;
	else
		underflow_flag = 1;
}

// Duplicate nb_bytes from address+offset to address
void duplicate(u32 *address, u32 offset, u32 nb_bytes)
{
	u32 i;
	if (offset == 0)
		printf("uncompress(): WARNING - zero offset value found for duplication\n");
	for (i=0; i<nb_bytes; i++)
	{
		writebyte(fbuffer[LOADER], (*address), readbyte(fbuffer[LOADER],(*address)+offset));
		decrement(address);
	}
}



// Uncompress loader data
int uncompress(u32 expected_size)
{
	u32 source = LOADER_DATA_START;
	u32 uncompressed_size, current;
	u32 dest, offset;
	u32 bit, nb_bits_to_process, nb_bytes_to_process;
	u32 j;
	compressed_size = readlong(mbuffer, source); 
	source +=4;
	uncompressed_size = readlong(mbuffer, source); 
	source +=4;
	if (uncompressed_size != expected_size)
	{
		printf("uncompress(): uncompressed data size does not match expected size\n");
		return -1;
	}
	checksum = readlong(mbuffer, source);	// There's a compression checksum
	source +=4;	// Keeping this last +/- 4 on source for clarity

	if (opt_verbose)
	{
		printf("  Compressed size=%X, uncompressed size=%X\n", 
			compressed_size, uncompressed_size);
	}

	source += (compressed_size-4);	// We read compressed data (long) starting from the end
	dest = uncompressed_size-1;		// We fill the uncompressed data (byte) from the end too

	current = readlong(mbuffer, source); 
	source -= 4;
	// Note that the longword above (last one) will not have the one bit marker
	// => Unlike other longwords, we might read ahead BEFORE all 32 bits
	// have been rotated out of the last longword of compressed data
	// (i.e., as soon as rotated long is zero)

	checksum ^= current;
	if (opt_debug)
		printf("(-%X).l = %08X\n", (compressed_size-source+LOADER_DATA_START+8), current);

	while (dest != 0)
	{
		// Read bit 0 of multiplier
		bit = getbit (&source, &current);
		if (bit)
		{	// bit0 = 1 => 3 bit multiplier
			// Read bits 1 and 2
			bit = getbitstream(&source, &current, 2);
			// OK, this is no longer a bit, but who cares?
			switch (bit)
			{
			case 2:	// mult: 011 (01 reversed  = 10)
				// Read # of bytes to duplicate (8 bit value)
				nb_bytes_to_process = getbitstream(&source, &current, 8)+1;
				// Read offset (12 bit value)
				offset = getbitstream(&source, &current, 12);
				duplicate(&dest, offset, nb_bytes_to_process);
				if (opt_debug)
					printf("  o mult=011: duplicated %d bytes at (start) offset %X to address %X\n", 
						nb_bytes_to_process, offset, dest+1);
				break;
			case 3:	// mult: 111
				// Read # of bytes to read and copy (8 bit value)
				nb_bytes_to_process = getbitstream(&source, &current, 8) + 9;
				// We add 8 above, because a [1-9] value 
				// would be taken care of by a 3 bit bitstream
				for (j=0; j<nb_bytes_to_process; j++)
				{	// Read and copy nb_bytes+1
					writebyte(fbuffer[LOADER], dest, (u8)getbitstream(&source, &current, 8));
					decrement(&dest);
				}
				if (opt_debug)
					printf("  o mult=111: copied %d bytes to address %X\n", nb_bytes_to_process, dest+1);
				break;
			default: // mult: x01
				// Read offset (9 or 10 bit value)
		        nb_bits_to_process = bit+9;
				offset = getbitstream(&source, &current, nb_bits_to_process);
				// Duplicate 2 or 3 bytes 
				nb_bytes_to_process = bit+3;
				duplicate(&dest, offset, nb_bytes_to_process);
				if (opt_debug)
					printf("  o mult=%d01: duplicated %d bytes at (start) offset %X to address %X\n", 
						bit&1, nb_bytes_to_process, offset, dest+1);
				break;
			}
		}
		else
		{	// bit0=0 => 2 bit multiplier
			bit = getbit (&source, &current);
			if (bit)
			{	// mult: 10
				// Read 8 bit offset
				offset = getbitstream(&source, &current, 8);
				// Duplicate 1 byte
				duplicate(&dest, offset, 2);
				if (opt_debug)
					printf("  o mult=10: duplicated 2 bytes at (start) offset %X to address %X\n", 
						offset, dest+1);
			}
			else
			{	// mult: 00
				// Read # of bytes to read and copy (3 bit value)
				nb_bytes_to_process = getbitstream(&source, &current, 3) + 1;
				for (j=0; j<nb_bytes_to_process; j++)
				{	// Read and copy nb_bytes+1
					writebyte(fbuffer[LOADER], dest, (u8)getbitstream(&source, &current, 8));
					decrement(&dest);
				}
				if (opt_debug)
					printf("  o mult=00: copied 2 bytes to address %X\n", dest+1);

			}
		} 
	}

	if (checksum != 0)
	{
		printf("uncompress(): checksum error\n");
		return -1;
	}
	return 0;
}


void load_all_files()
{
	size_t read;
	u32 i;
	int compressed_loader = 0;

	for (i=0; i<NB_FILES; i++)
	{
		// calloc is handy to get everything set to 0
		if ( (fbuffer[i] = (u8*) calloc(fsize[i], 1)) == NULL)
		{
			fprintf (stderr, "Could not allocate buffers\n");
			ERR_EXIT;
		}

		if ((fd = fopen (fname[i], "rb")) == NULL)
		{
			if (opt_verbose)
				perror ("fopen()");
			fprintf (stderr, "Can't find file '%s'\n", fname[i]);

			/* Take care of the compressed loader if present */
			if (i == LOADER)
			{
				// Uncompressed loader was not found
				// Maybe there's a compressed one?
				fprintf (stderr, "  Trying to use compressed loader '%s' instead\n",ALT_LOADER);
				if ((fd = fopen (ALT_LOADER, "rb")) == NULL)
				{
					printf ("  '%s' not found.\n", ALT_LOADER);
					ERR_EXIT;
				}
				// OK, file was found - let's allocated the compressed data buffer
				if ((mbuffer = (u8*) calloc(ALT_LOADER_SIZE, 1)) == NULL)
				{
					fprintf (stderr, "Could not allocate source buffer for uncompress\n");
					ERR_EXIT;
				}
				if (opt_verbose)
					printf("Reading file '%s'...\n", ALT_LOADER);
				read = fread (mbuffer, 1, ALT_LOADER_SIZE, fd);
				if (read != ALT_LOADER_SIZE)
				{
					if (opt_verbose)
						perror ("fread()");
					fprintf(stderr, "'%s': Unexpected file size or read error\n", ALT_LOADER);
					ERR_EXIT;
				}
				compressed_loader = 1;

				fprintf (stderr, "  Uncompressing...\n");
				if (uncompress(fsize[LOADER]))
				{
					printf("Decompression error\n");
					ERR_EXIT;
				}
				fprintf (stderr, "  OK. Now saving file as '%s'\n",fname[LOADER]);
				if ((fd = fopen (fname[LOADER], "wb")) == NULL)
				{
					if (opt_verbose)
						perror ("fopen()");
					fprintf (stderr, "Can't create file '%s'\n", fname[LOADER]);
					ERR_EXIT;
				}
				
				// Write file
				if (opt_verbose)
						printf("Writing file '%s'...\n", fname[LOADER]);
				read = fwrite (fbuffer[LOADER], 1, fsize[LOADER], fd);
				if (read != fsize[LOADER])
				{
					if (opt_verbose)
						perror ("fwrite()");
					fprintf(stderr, "'%s': Unexpected file size or write error\n", fname[LOADER]);
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
				printf("Reading file '%s'...\n", fname[i]);
			read = fread (fbuffer[i], 1, fsize[i], fd);
			if (read != fsize[i])
			{
				if (opt_verbose)
					perror ("fread()");
				fprintf(stderr, "'%s': Unexpected file size or read error\n", fname[i]);
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
	printf("nb_cells = %X\n", nb_rooms);

	nb_sprites = readword(fbuffer[SPRITES],0) + 1;
	printf("nb_sprites = %X\n", nb_sprites);

	nb_objects = readword(fbuffer[OBJECTS],0) + 1;
	printf("nb_objects = %X\n", nb_objects);
	for (i=0; i<NB_OBS_TO_SPRITE; i++)
	{
		obs_to_sprite[i] = readbyte(fbuffer[LOADER],OBS_TO_SPRITE_START+i);
		printf("obs_to_sprite[%d] = %X\n", i, obs_to_sprite[i]);
	}
}


// Reorganize cell bitplanes from interleaved lines (4*32 bits)
// to interleaved pixels (4 bits per pixel)
void cells_to_interleaved(u8* buffer, u32 size)
{
	u32 line[4];
	u8 byte = 0;
	u8 tmp;
	u32 index, i, j;
	int k;

	for (i=0; i<size; i+=16)	
	// 4 colours *4 bytes per line
	{
		for (j=0; j<4; j++)
			line[j] = readlong(buffer, i+j*4);
		index = 15;
		for (j=0; j<32; j++)	// 32 nibbles
		{
			if (!(j&0x01))	// don't lose the former nibble if j is odd (2 nibbles per byte)
				byte = 0;
			// if k is not a signed integer, we'll have issues with the following line
			for (k=3; k>=0; k--)
			// last bitplane is MSb dammit!!!
			{
				byte = (byte << 1) + (u8)(line[k] & 0x1);
				line[k] = line[k] >> 1;
			}
			if (j&0x01)		// if j is odd, we read 2 nibbles => time to write a byte
			{
				tmp = byte;
				byte = (byte << 4) | (tmp >> 4);
				writebyte(buffer, i+index, byte);
				index--;
			}
		}
	}
}


// Reorganize sprites from separate bitplanes (multiples of 16 bits)
// to interleaved pixels (4 bits per pixel)
void sprites_to_interleaved(u8* buffer, u32 bitplane_size)
{
	u8* sbuffer;
	u8  bitplane_byte[4];
	u32 interleaved;
	u32 i, j;

	sbuffer = (u8*) malloc(4*bitplane_size);

	// Yeah, I know, we could be smarter than allocate a buffer every time
	if (sbuffer == NULL)
	{
		fprintf (stderr, "remap_sprite: could not allocate sprite buffer\n");
		ERR_EXIT;
	}
	// First, let's copy the buffer
	for (i=0; i<4*bitplane_size; i++)
		sbuffer[i] = buffer[i];

	for (i=0; i<bitplane_size; i++)	
	{
		// Read one byte from each bitplane...
		for (j=0; j<4; j++)
			// bitplanes are in reverse order
			bitplane_byte[3-j] = readbyte(sbuffer, i+(j*bitplane_size));

		// ...and create the interleaved longword out of it
		interleaved = 0;
		for (j=0; j<32; j++)	
		{
			// You sure want to rotate BEFORE you add the last bit!
			interleaved = interleaved << 1;
			interleaved |= (bitplane_byte[j%4] >> ((31-j)/4)) & 1;
		}
		writelong(buffer,4*i,interleaved);
	}
	free(sbuffer);
}


// Convert an Amiga 12 bit colour palette to 24 bit
void to_24bit_Palette(u8 palette_index)
{
	u32 i;
	int colour;		// 0 = Red, 1 = Green, 2 = Blue
	u16 rgb;

	int palette_start = palette_index * 0x20;

	// Read the palette
	if (opt_verbose)
		printf("Using Amiga Palette index: %d\n", palette_index);
	for (i=0; i<16; i++)		// 16 colours
	{
		rgb = readword(fbuffer[PALETTES], palette_start + 2*i);
		if (opt_verbose)
		{
			printf(" %03X", rgb); 
			if (i==7)
				printf("\n");
		}
		for (colour=2; colour>=0; colour--)
		{
			bPalette[colour][i] = (rgb&0x000F) * 0x11;
			rgb = rgb>>4;
		}
	}
	if (opt_verbose)
		printf("\n\n");
}


// Convert an Amiga 12 bit colour palette to a 48 bit (TIFF) one
void to_48bit_Palette(u16 wPalette[3][16], u8 palette_index)
{
	u32 i;
	int colour;		// 0 = Red, 1 = Green, 2 = Blue
	u16 rgb;

	u16 palette_start = palette_index * 0x20;

	// Read the palette
	if (opt_verbose)
		printf("Using Amiga Palette index: %d\n", palette_index);
	for (i=0; i<16; i++)		// 16 colours
	{
		rgb = readword(fbuffer[PALETTES], palette_start + 2*i);
		if (opt_verbose)
		{
			printf(" %03X", rgb); 
			if (i==7)
				printf("\n");
		}
		for (colour=2; colour>=0; colour--)
		{
			wPalette[colour][i] = (rgb&0x000F) * 0x1111;
			rgb = rgb>>4;
		}
	}
	if (opt_verbose)
		printf("\n\n");
}


// Convert a 4 bit line-interleaved source to 24 bit RGB destination
void line_interleaved_to_RGB(u8* source, u8* dest, u16 w, u16 h)
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
				line_byte[3-k] = readbyte(source, 4*(wb*(h-1-i)) + k*wb + j);
			// Write 8 RGB(A) values
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
				// writebyte only uses pos once, so we can afford pos++
				writebyte(dest, pos++, bPalette[RED][colour_index]);
				writebyte(dest, pos++, bPalette[GREEN][colour_index]);
				writebyte(dest, pos++, bPalette[BLUE][colour_index]);
			}
		}
	}
}


// Convert a 1+4 bits (mask+colour) bitplane source
// to 24 bit RGBA destination
void bitplane_to_RGBA(u8* source, u8* dest, u16 w, u16 h)
{
	u16 bitplane_size;
	u8  colour_index;
	u16 i,j,k,wb;
	u8  bitplane_byte[5], mask_byte;
	u32 pos = 0;
//	int no_mask = 0;

	wb = w/8;	// width in bytes
	bitplane_size = h*wb; 

	for (i=0; i<bitplane_size; i++)	
	{
		// Read one byte from each bitplane...
		for (j=0; j<5; j++)
			// bitplanes are in reverse order for colour
			// and so is openGL's coordinate system for y
			bitplane_byte[4-j] = readbyte(source, 
				wb*(h-1-i/wb) + i%wb + (j*bitplane_size) );

		// For clarity
		mask_byte = bitplane_byte[4];

		// Write 8 RGBA quadruplets 
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
			writebyte(dest, pos++, bPalette[RED][colour_index]);
			writebyte(dest, pos++, bPalette[GREEN][colour_index]);
			writebyte(dest, pos++, bPalette[BLUE][colour_index]);
			// Alpha
			writebyte(dest, pos++, (mask_byte&0x80)?0xFF:0);
			mask_byte <<=1;
		}
	}
}


// Converts the room cells to RGB data we can handle
void cells_to_RGB(u8* source, u8* dest, u32 size)
{
	u32 i;

	// Convert each 32x16x4bit (=256 bytes) cell to RGB
	for (i=0; i<(size/256); i++)
		line_interleaved_to_RGB(source + (256*i), dest+(6*256*i), 32, 16);
}


// Initialize the sprite array
void init_sprites()
{
	u32 index = 2;	// We need to ignore the first word (nb of sprites)
	u16 sprite_index = 0;
	u16 sprite_w;	// width, in words
	u32 sprite_address;

	// Allocate the sprites and overlay arrays
	sprite = malloc(nb_sprites * sizeof(s_sprite));
	overlay = malloc(MAX_OVERLAY * sizeof(s_overlay));

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
//		printf("sprite[%X] address = %08X\n", sprite_index, sprite_address);
		// x size is given in words
		sprite_w = readword(fbuffer[SPRITES],sprite_address);
		sprite[sprite_index].w = 16*(sprite_w & 0x7FFF);
		sprite[sprite_index].h = readword(fbuffer[SPRITES],sprite_address+2);
		// According to MSb of sprite_w (=no_mask), we'll need to use RGBA or RGB
		sprite[sprite_index].type = (sprite_w & 0x8000)?GL_RGB:GL_RGBA;
		sprite[sprite_index].data = malloc( ((sprite_w & 0x8000)?3:4) * 
			sprite[sprite_index].w * sprite[sprite_index].h);
//		printf("  w,h = %0X, %0X\n", sprite[sprite_index].w , sprite[sprite_index].h);
	}
}


// Converts the sprites to RGB data we can handle
void sprites_to_RGB()
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
			printf("sprites_to_RGB: Integrity check failure on bitplane_size\n");

		// Source address
		sbuffer = fbuffer[SPRITES] + sprite_address + 8; 

		if (no_mask)
			// Bitplanes that have no mask are line-interleaved, like cells
			line_interleaved_to_RGB(sbuffer, sprite[sprite_index].data, w*8, h);
		else
			bitplane_to_RGBA(sbuffer, sprite[sprite_index].data, w*8, h);
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
			printf("  match: %04X, direction: %04X\n", tile1_data, sx);
		if (i >= (12*(NB_SPECIAL_TILES-4)))
		// The four last special tiles are exits. We need to check is they are open
		{
			// Get the exit data (same tile if tunnel, 2 rows down in door)
			tile2_data = readword(fbuffer[ROOMS], current_tile + 
				(i==(12*(NB_SPECIAL_TILES-1)))?0:(4*room_x));
//			printf("got exit: %04X\n", tile2_data);
			// Validity check
			if (!(tile2_data & 0x000F))
				printf("set_overlays: Integrity check failure on exit tile\n");
			// if the tile is an exit and the exit is open
			if (tile2_data & 0x0010)
			{
				if (opt_debug)
					printf("    exit open: ignoring overlay\n");
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
				printf("    ignored as %04X matches\n", tile2_data);
			continue;
		}
		sid = readword(fbuffer[LOADER], SPECIAL_TILES_START+i+4);
		overlay[overlay_index].sid = sid;
		if (opt_debug)
			printf("    overlay as %04X != %04X => %X\n", tile2_data, readword(fbuffer[LOADER], SPECIAL_TILES_START+i+2), sid);
		sy = readword(fbuffer[LOADER], SPECIAL_TILES_START+i+6);
		if (opt_debug)
			printf("    sx: %04X, sy: %04X\n", sx, sy);
		overlay[overlay_index].x = x + (int)sx - sprite[sid].w + (int)((sprite[sid].type==GL_RGBA)?1:16);
		overlay[overlay_index].y = y - (int)sy + 15;
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
//	u32 index = 2;	// We need to ignore the first word (nb of sprites)
	u16 i;

	for (i=0; i<(8*nb_objects); i+=8)
	{
		if (readword(fbuffer[OBJECTS],i+2) != current_room_index)
			continue;
		overlay[overlay_index].sid = obs_to_sprite[readword(fbuffer[OBJECTS],i+2+6)];
		overlay[overlay_index].x = gl_off_x + readword(fbuffer[OBJECTS],i+2+4) - 15;
		overlay[overlay_index].y = gl_height - gl_off_y - readword(fbuffer[OBJECTS],i+2+2) + 15;
		if (opt_debug)
			printf("  pickup object match: sid=%X\n", overlay[overlay_index].sid);
		overlay_index++;
	}
	
}

// Display all our overlays
void display_overlays()
{
	u8 i;

	for (i=0; i<overlay_index; i++)
	{
		glRasterPos2i(overlay[i].x,overlay[i].y);
		glDrawPixels(sprite[overlay[i].sid].w, sprite[overlay[i].sid].h,
			sprite[overlay[i].sid].type, GL_UNSIGNED_BYTE, 
			sprite[overlay[i].sid].data);
	}
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
		printf("\noffset[%03X] = %08X ", room_index, offset);
	if (offset == 0xFFFFFFFF)
	{
		// For some reason there is a break in the middle
		if (opt_verbose)
			printf("\n  IGNORED", room_index, offset);
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
		printf("(room_x=%X,room_y=%X)\n", room_x, room_y);
	gl_off_x = (gl_width - (room_x*32))/2;
	gl_off_y = 32 + (gl_height-(room_y*16))/2;

	// reset room overlays
	overlay_index = 0;

	// Before we do anything, let's set the pickable objects in
	// our overlay table (so that room overlays go on top of 'em)
	set_objects();

	// Read the tiles data
	for (tile_y=0; tile_y<room_y; tile_y++)
	{
		if (opt_verbose)
			printf("    ");	// Start of a line
		for(tile_x=0; tile_x<room_x; tile_x++)
		{
			pixel_x = gl_off_x+tile_x*32;
			pixel_y = gl_height-gl_off_y-(tile_y*16);

			// A tile is 32(x)*16(y)*4(bits) = 256 bytes
			// A specific room tile is identified by a word
			tile_data = readword((u8*)fbuffer[ROOMS], ROOMS_START+offset);

			// position ourselves to the right location
			glRasterPos2i(pixel_x,pixel_y);

			// Tile data is of the form (tile_index<<7) + exit_flags, with tile_index being 
			// the index in COLDITZ_CELLS (each tile occupying 0x100 bytes there)
			glDrawPixels(32,16,GL_RGB,GL_UNSIGNED_BYTE, ((u8*)rgbCells) + (tile_data>>7)*6*0x100 + 
				// Take care of the tunnels too below
				((room_index>0x202)?(6*0x1E000):0));
			// As we converted the cells from 4 to 24 bits (RGB) we need to multiply offsets by 6


			// Display sprite overlay
			set_overlays(pixel_x, pixel_y, ROOMS_START+offset, room_x);

			offset +=2;		// Read next tile

			if (opt_verbose)
				printf("%04X ", tile_data);
		}
		if (opt_verbose)
			printf("\n");
	}

	if (opt_debug)
		printf("\n");

	// Let add our guy
	overlay[overlay_index].sid = 3;
	overlay[overlay_index].x = gl_off_x + room_x*16;
	overlay[overlay_index++].y = gl_off_y;

	// Now that the background is done, and that we have the overlays, display the overlay sprites
	display_overlays();
}

