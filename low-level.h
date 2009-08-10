/*
 *  Colditz Escape! - Rewritten Engine for "Escape From Colditz"
 *  copyright (C) 2008-2009 Aperture Software 
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 *  ---------------------------------------------------------------------------
 *  low-level.h: low level helper functions definitions
 *  ---------------------------------------------------------------------------
 */
#pragma once

#ifdef	__cplusplus
extern "C" {
#endif

// Define our msleep function
#if defined(WIN32)
#define msleep(msecs) Sleep(msecs)
#include <Windows.h>
static __inline u64 mtime(void)
{	// Because MS uses a 32 bit value, this counter will reset every 49 days or so
	// Hope you won't be playing the game while it resets...
	return timeGetTime(); 
}
#elif defined(PSP)
#include <pspthreadman.h>
#include <psprtc.h>
#include <pspctrl.h>
#define msleep(msecs) sceKernelDelayThread(1000*msecs)
static __inline u64 mtime(void)
{
	u64 blahtime; 
	sceRtcGetCurrentTick(&blahtime);
	return blahtime/1000;
}
#else
#include <unistd.h>
#define	msleep(msecs) usleep(1000*msecs)
#endif


// Some fixes for windows
#if defined(WIN32) || defined(__MSDOS__)
#define NULL_FD fopen("NUL", "w")
#else
#define NULL_FD fopen("/dev/null", "w")
#endif



// Handy macro for exiting. xbuffer or fd = NULL is no problemo 
// (except for lousy Visual C++, that will CRASH on fd = NULL!!!!)
//#define FREE_BUFFERS	{int _buf; for (_buf=0;_buf<NB_FILES;_buf++) aligned_free(fbuffer[_buf]); aligned_free(mbuffer);}
//#define ERR_EXIT		{FREE_BUFFERS; if (fd != NULL) fclose(fd); fflush(stdin); exit(0);}
// On Windows and PSP, exiting the application will automatically free allocated memory blocks
// so we don't bother freeing any buffers here
#if defined(WIN32)
#define LEAVE			exit(0)
#define FATAL			exit(1)
#elif defined(PSP)
#define LEAVE			back_to_kernel()
#if defined(PSP_ONSCREEN_STDOUT)
// No immediate exit on PSP as we might want to display the error
extern void back_to_kernel (void);

// Wait for a (new) keypress (can't use idle_supended as we can't continue on error)
static __inline void psp_any_key() 
{
	unsigned int initialButtons;
	SceCtrlData pad;

	sceCtrlReadBufferPositive(&pad, 1);
	initialButtons = pad.Buttons;

	while (pad.Buttons == initialButtons)
	{
		sceCtrlReadBufferPositive(&pad, 1);
		if (pad.Buttons == 0)
			initialButtons = 0;
	}
}
#define FATAL_MSG		"\n\n\t\tFATAL ERROR: Press any key to exit the program\n"
#define FATAL			{ printf(FATAL_MSG); psp_any_key(); back_to_kernel(); }
#else
// no screen stdout => immediate exit
#define FATAL			back_to_kernel()
#endif
#endif
#define ERR_EXIT		{if (fd!=NULL) fclose(fd); RECORD(0xDEAD); RECORD(0); fflush(stdout); FATAL;}
#define perr(...)		fprintf(stderr, __VA_ARGS__)
#define print(...)		printf(__VA_ARGS__)
#define printv(...)		if(opt_verbose) print(__VA_ARGS__)
#define perrv(...)		if(opt_verbose) perr(__VA_ARGS__)
#define printb(...)		if(opt_debug) print(__VA_ARGS__)
#define perrb(...)		if(opt_debug) perr(__VA_ARGS__)

// size of an array
#define SIZE_A(ar)		(sizeof(ar)/sizeof(ar[0]))

// concatenate 2 words into a long 
#define to_long(msw,lsw)			\
	((((u32)(msw))<<16) | ((u16)(lsw)))

// check if a sprite (s) is overflowing on a mask (m)
#define collision(s,m)				\
	( (((m)^s)&(m)) != (m) )

// rerurn true if a bit of a sprite (s) is set over a clear bit of the mask (m)
#define inverted_collision(s,m)		\
	collision(s,~m)

// The handy ones, in big endian mode
static __inline u32 readlong(u8* buffer, u32 addr)
{
	return ((((u32)buffer[addr+0])<<24) + (((u32)buffer[addr+1])<<16) +
		(((u32)buffer[addr+2])<<8) + ((u32)buffer[addr+3]));
}

static __inline u32 freadlong(FILE* fd)
{
	u8	b, i;
	u32 r = 0;
	for (i=0; i<4; i++)
	{
		fread(&b, 1, 1, fd);
		r <<= 8;
		r |= b;
	}
	return r;
}

static __inline void writelong(u8* buffer, u32 addr, u32 value)
{
	buffer[addr]   = (u8)(value>>24);
	buffer[addr+1] = (u8)(value>>16);
	buffer[addr+2] = (u8)(value>>8);
	buffer[addr+3] = (u8)value;
}

static __inline u32 read24(u8* buffer, u32 addr)
{
	return ((((u32)buffer[addr+0])<<16) + (((u32)buffer[addr+1])<<8) +
		((u32)buffer[addr+2]));
}

static __inline u16 readword(u8* buffer, u32 addr)
{
	return ((((u16)buffer[addr+0])<<8) + ((u16)buffer[addr+1]));
}

static __inline u16 freadword(FILE* fd)
{
	u8	b,i;
	u16 r = 0;
	for (i=0; i<2; i++)
	{
		fread(&b, 1, 1, fd);
		r <<= 8;
		r |= b;
	}
	return r;
}

static __inline void writeword(u8* buffer, u32 addr, u16 value)
{
	buffer[addr]   = (u8)(value>>8);
	buffer[addr+1] = (u8)value;
}

static __inline u8 readbyte(u8* buffer, u32 addr)
{
	return buffer[addr];
}

static __inline u8 freadbyte(FILE* fd)
{
	u8	b = 0;
	fread(&b, 1, 1, fd);
	return b;
}

static __inline void writebyte(u8* buffer, u32 addr, u8 value)
{
	buffer[addr] = value;
}

// The well known K&R method to count bits
static __inline int count_bits(u32 n) 
{     
  int c; 
  for (c = 0; n; c++) 
    n &= n - 1; // clear the least significant bit set
  return c;
}


// Prototypes
u16 powerize(u16 n);
int uncompress(u32 expected_size);
void *aligned_malloc(size_t bytes, size_t alignment);
void aligned_free(void *ptr);
const char *to_binary(u32 x);
int ppDecrunch(u8 *src, u8 *dest, u8 *offset_lens, u32 src_len, u32 dest_len, u8 skip_bits);

#ifdef	__cplusplus
}
#endif

