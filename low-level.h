/*
 *  Colditz Escape - Rewritten Engine for "Escape From Colditz"
 *  copyright (C) 2008-2017 Aperture Software
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

#include <stdint.h>
#include <stdbool.h>

// Define our msleep function
#if defined(WIN32)
#define msleep(msecs) Sleep(msecs)
#include <windows.h>
static __inline uint64_t mtime(void)
{	// Because MS uses a 32 bit value, this counter will reset every 49 days or so
	// Hope you won't be playing the game while it resets...
	return timeGetTime();
}
#elif defined(PSP)
#include <pspthreadman.h>
#include <psprtc.h>
#include <pspctrl.h>
#define msleep(msecs) sceKernelDelayThread(1000*msecs)
static __inline uint64_t mtime(void)
{
	uint64_t current_tick;
	sceRtcGetCurrentTick(&current_tick);
	return current_tick/1000;
}
#elif defined(__linux__)
#include <unistd.h>
#include <time.h>
#define msleep(msecs) usleep(1000*msecs)
static __inline uint64_t mtime(void)
{
	struct timespec tp;
	clock_gettime(CLOCK_MONOTONIC, &tp); //TODO : check for returned value
	return ((uint64_t)tp.tv_sec)*1000LL + ((uint64_t)tp.tv_nsec)/1000000LL;
}
#endif

// Some fixes for windows
#if defined(WIN32) || defined(__MSDOS__)
#define NULL_FD fopen("NUL", "w")
#else
#define NULL_FD fopen("/dev/null", "w")
#endif

#if defined(PSP)
#define LEAVE			free_data(); back_to_kernel()
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
#else
#define LEAVE			free_data(); exit(0)
#define FATAL			free_data(); exit(1)
#endif
#define ERR_EXIT		do {if (fd!=NULL) fclose(fd); fflush(stdout); FATAL;} while(0)

extern bool opt_verbose;
extern bool opt_debug;

#if defined(PSP_ONSCREEN_STDOUT)
#define perr(...)		printf(__VA_ARGS__)
#else
#define perr(...)		fprintf(stderr, __VA_ARGS__)
#endif
#define print(...)		printf(__VA_ARGS__)
#define printv(...)		if(opt_verbose) print(__VA_ARGS__)
#define perrv(...)		if(opt_verbose) perr(__VA_ARGS__)
#define printb(...)		if(opt_debug) print(__VA_ARGS__)
#define perrb(...)		if(opt_debug) perr(__VA_ARGS__)

// size of an array
#define SIZE_A(ar)		(sizeof(ar)/sizeof(ar[0]))

// dealloc macros
#define SFREE(p)		do {free(p); p=NULL;} while(0)
#define SAFREE(p)		do {aligned_free(p); p=NULL;} while(0)

// concatenate 2 words into a long
#define to_long(msw,lsw)			\
	((((uint32_t)(msw))<<16) | ((uint16_t)(lsw)))

// check if a sprite (s) is overflowing on a mask (m)
#define collision(s,m)				\
	( (((m)^s)&(m)) != (m) )

// rerurn true if a bit of a sprite (s) is set over a clear bit of the mask (m)
#define inverted_collision(s,m)		\
	collision(s,~m)

// The handy ones, in big endian mode
static __inline uint32_t readlong(uint8_t* buffer, uint32_t addr)
{
	return ((((uint32_t)buffer[addr+0])<<24) + (((uint32_t)buffer[addr+1])<<16) +
		(((uint32_t)buffer[addr+2])<<8) + ((uint32_t)buffer[addr+3]));
}

static __inline uint32_t freadlong(FILE* f)
{
	uint8_t b, i;
	uint32_t r = 0;
	for (i=0; i<4; i++)
	{
		fread(&b, 1, 1, f);
		r <<= 8;
		r |= b;
	}
	return r;
}

static __inline void writelong(uint8_t* buffer, uint32_t addr, uint32_t value)
{
	buffer[addr]   = (uint8_t)(value>>24);
	buffer[addr+1] = (uint8_t)(value>>16);
	buffer[addr+2] = (uint8_t)(value>>8);
	buffer[addr+3] = (uint8_t)value;
}

static __inline uint32_t read24(uint8_t* buffer, uint32_t addr)
{
	return ((((uint32_t)buffer[addr+0])<<16) + (((uint32_t)buffer[addr+1])<<8) +
		((uint32_t)buffer[addr+2]));
}

static __inline uint16_t readword(uint8_t* buffer, uint32_t addr)
{
	return ((((uint16_t)buffer[addr+0])<<8) + ((uint16_t)buffer[addr+1]));
}

static __inline uint16_t freadword(FILE* f)
{
	uint8_t b,i;
	uint16_t r = 0;
	for (i=0; i<2; i++)
	{
		fread(&b, 1, 1, f);
		r <<= 8;
		r |= b;
	}
	return r;
}

static __inline void writeword(uint8_t* buffer, uint32_t addr, uint16_t value)
{
	buffer[addr]   = (uint8_t)(value>>8);
	buffer[addr+1] = (uint8_t)value;
}

static __inline uint8_t readbyte(uint8_t* buffer, uint32_t addr)
{
	return buffer[addr];
}

static __inline uint8_t freadbyte(FILE* f)
{
	uint8_t b = 0;
	fread(&b, 1, 1, f);
	return b;
}

static __inline void writebyte(uint8_t* buffer, uint32_t addr, uint8_t value)
{
	buffer[addr] = value;
}

// The well known K&R method to count bits
static __inline int count_bits(uint32_t n)
{
  int c;
  for (c = 0; n; c++)
    n &= n - 1; // clear the least significant bit set
  return c;
}


// Prototypes
uint16_t powerize(uint16_t n);
int uncompress(uint32_t expected_size);
void *aligned_malloc(size_t bytes, size_t alignment);
void aligned_free(void *ptr);
const char *to_binary(uint32_t x);
int ppDecrunch(uint8_t *src, uint8_t *dest, uint8_t *offset_lens, uint32_t src_len, uint32_t dest_len, uint8_t skip_bits);

#ifdef	__cplusplus
}
#endif
