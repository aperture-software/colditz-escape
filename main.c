/**
 **  Colditz Maps Explorer
 **
 **  
 **
 **/

#include <stdio.h>
#include "getopt.h"		// Stupid Windows will complain
#include <stdlib.h>		// if this goes after stdlib.h
#include <string.h>
#include <math.h>
#include "tiffio.h"

// Define our msleep function
#ifdef _WIN32
#include <Windows.h>
#define msleep(msecs) Sleep(msecs)
#else
#include <unistd.h>
#define	msleep(msecs) usleep(1000*msecs)
#endif

#if __APPLE__
#include <inttypes.h>
#define u8  uint8_t
#define u16 uint16_t
#define u32 uint32_t
#else // ! __APPLE__
#ifndef u8
#define u8 unsigned char
#endif
#ifndef u16
#define u16 unsigned short
#endif
#ifndef u32
#define u32 unsigned long
#endif
#endif // __APPLE__

// Some fixes for windows
#if (_WIN32 || __MSDOS__)
#define NULL_FD fopen("NUL", "w")
#else
#define NULL_FD fopen("/dev/null", "w")
#endif

// # files we'll be dealing with
#define nb_files			6
#define ROOMS				0
#define CELLS				1
#define PALS_BIN			2
#define LOADER				3
#define COMPRESSED_MAP      4
#define SPRITES             5
// Never be short on filename sizes
#define NAME_SIZE			256			
#define FNAMES				{ "COLDITZ_ROOM_MAPS", "COLDITZ_CELLS", "PALS.BIN", "COLDITZ-LOADER",\
							  "COMPRESSED_MAP", "SPRITES.SPR" }
#define FSIZES				{ 58828, 135944, 232, 56080, \
							  33508, 71056 }	
#define ALT_LOADER          "SKR_COLD"
#define ALT_LOADER_SIZE		28820
#define OFFSETS_START		0x2684
#define ROOMS_START			0x2FE4
#define CM_TILES_START      0x5E80
#define LOADER_DATA_START	0x10C
#define FFs_TO_IGNORE		7

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


// Handy macro for exiting. xbuffer or fd = NULL is no problemo 
// (except for lousy Visual C++, that will CRASH on fd = NULL!!!!)
#define FREE_BUFFERS	{for (i=0;i<nb_files;i++) free(fbuffer[i]); free(mbuffer);}
#define ERR_EXIT		{FREE_BUFFERS; if (fd != NULL) fclose(fd); fflush(stdin); exit(1);}
// The infamous Linux/DOS stdin fix
#define FLUSHER			{while(getchar() != 0x0A);}

// Global variables, set to static to avoid name confusion, e.g. with stat()
static int	opt_verbose		= 0;
static int	opt_debug		= 0;
static int	stat			= 0;
static int  debug_flag		= 0;
static int  underflow_flag	= 0;
static u8   *mbuffer		= NULL;
static u8   *fbuffer[nb_files] = {NULL, NULL, NULL};
// Need to place it up here for debug
u32			compressed_size, checksum;


/* The handy ones, IN BIG ENDIAN MODE THIS TIME!!!*/
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
	for (u32 i=0; i<streamsize; i++)
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
	if (offset == 0)
		printf("uncompress(): WARNING - zero offset value found for duplication\n");
	for (u32 i=0; i<nb_bytes; i++)
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
	// Note that the longword above will not have the one bit marker
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

// Reorganize bitplanes from interleaved lines (32 bits)
// to interleaved pixels (4 bits)
void remap_bitplanes(u8* buffer, u32 size)
{
	u32 line[4];
	u8 byte = 0;
	u8 tmp;
	u32 index, i, j;
	int k;
	for (i=0; i<size; i+=16)	
	// 4*4 bytes per line
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


// Reorganize sprites from bitplanes (multiples of 16 bits)
// to interleaved pixels (4 bits)
void remap_sprite(u8* buffer, u32 bitplane_size)
{
	u8* sbuffer;
	u8  bitplane_byte[4];
	u32 interleaved;
	u32 index, i, j;

//	if ( (sbuffer = (u8*) calloc(4*bitplane_size, 1)) == NULL)
	if ( (sbuffer = (u8*) malloc(4*bitplane_size)) == NULL)
	{
		fprintf (stderr, "remap_sprite: could not allocate sprite buffer\n");
		exit(1);
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

/* Here we go! */
int main (int argc, char *argv[])
{
	// The line below only works because of "const" above
	char* fname[nb_files]	= FNAMES;			// file name(s)
	u32  fsize[nb_files]	= FSIZES;
	// PALS.BIN, offset $40
	static u16  palette[3][16] = {
		{ 0x0000, 0x9999, 0x8888, 0x6666, 0x4444, 0x0000, 0x0000, 0xAAAA, 0x4444, 0x0000, 0xAAAA, 0x7777, 0x5555, 0x3333, 0x2222, 0x1F1F} ,
	    { 0x0000, 0x7777, 0x5555, 0x3333, 0x1111, 0x0000, 0x4444, 0x0000, 0x6666, 0x4444, 0xAAAA, 0x7777, 0x5555, 0x3333, 0x2222, 0x0606} ,
	    { 0x0000, 0x4444, 0x2222, 0x0000, 0x0000, 0x6666, 0x9999, 0x0000, 0x0000, 0x0000, 0xAAAA, 0x7777, 0x5555, 0x3333, 0x2222, 0x0000}
	};
	
	// Flags
	int opt_skip			= 0;	// Skip TIFF Creation
	int opt_error 			= 0;	// getopt
	int opt_force			= 0;
	int compressed_loader   = 0;

	// General purpose
	u32  i;
	size_t read;
	FILE *fd = NULL;

	/*
     * Init
     */
	fflush(stdin);
	mbuffer    = NULL;


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

//	if (argv[optind] == NULL)
//		detected= 0;


	for (i=0; i<nb_files; i++)
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


	// Reoganize cells from interleaved bitplane lines to interleaved bitplane bits
	remap_bitplanes(fbuffer[CELLS],fsize[CELLS]);




	// Read the palette
	int pal_start = 0xc0;
	int colour;		// 0 = Red, 1 = Green, 2 = Blue
	u16 rgb;
	if (opt_verbose)
		printf("Using Amiga Palette:\n");
	for (i=0; i<16; i++)		// 16 colours
	{
		rgb = readword(fbuffer[2], pal_start + 2*i);
		if (opt_verbose)
		{
			printf(" %03X", rgb); 
			if (i==7)
				printf("\n");
		}
		for (colour=2; colour>=0; colour--)
		{
			palette[colour][i] = (rgb&0x000F) * 0x1111;
			rgb = rgb>>4;
		}
	}
	if (opt_verbose)
		printf("\n\n");


	/*
	 * Process compressed map
	 */
	u32 tile_offset = 0;					// Offsets to each rooms are given at
								// the beginning of the Rooms Map file
	int ignore = 0;				// We got to ignore a few FFFFFFFF offsets
	u16 room_x, room_y;
	u16 nb_tiles;
	u16 tile_data;
	char tiffname[NAME_SIZE];	// Hopefully we'll save a few TIFFs ;)
	TIFF* image;
	int no_mask = 0;

#define _PROCESS_SPRITES
#ifdef _PROCESS_SPRITES
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
		remap_sprite(((u8*)fbuffer[SPRITES]) + sprite_address + 8 + (no_mask?0:bitplane_size),  bitplane_size);
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
					tile_data = 0;	// WRONG needs to be set to full long & 0x1FF00 if 0
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

	for (u32 room_index=0; ; room_index++)
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
			TIFFSetField(image, TIFFTAG_COLORMAP, palette[0], palette[1], palette[2]);
			// We'll use tiling, since you're so kind as to ask ;)
			TIFFSetField(image, TIFFTAG_TILEWIDTH, 32);
			TIFFSetField(image, TIFFTAG_TILELENGTH, 16);
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
