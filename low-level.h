#ifndef _COLDITZ_LOW_LEVEL_H
#define _COLDITZ_LOW_LEVEL_H

#ifdef	__cplusplus
extern "C" {
#endif

// Define our msleep function
#if defined(WIN32)
//#include <Windows.h>
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
#define FREE_BUFFERS	{int _buf; for (_buf=0;_buf<NB_FILES;_buf++) aligned_free(fbuffer[_buf]); aligned_free(mbuffer);}
#define ERR_EXIT		{FREE_BUFFERS; if (fd != NULL) fclose(fd); fflush(stdin); exit(0);}
#define perr(...)		fprintf(stderr, __VA_ARGS__)
#define print(...)		printf(__VA_ARGS__)
#define printv(...)		if(opt_verbose) print(__VA_ARGS__)
#define perrv(...)		if(opt_verbose) perr(__VA_ARGS__)
#define printb(...)		if(opt_debug) print(__VA_ARGS__)
#define perrb(...)		if(opt_debug) perr(__VA_ARGS__)


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

static __inline u32 read24(u8* buffer, u32 addr)
{
	return ((((u32)buffer[addr+0])<<16) + (((u32)buffer[addr+1])<<8) +
		((u32)buffer[addr+2]));
}

static __inline void writelong(u8* buffer, u32 addr, u32 value)
{
	buffer[addr]   = (u8)(value>>24);
	buffer[addr+1] = (u8)(value>>16);
	buffer[addr+2] = (u8)(value>>8);
	buffer[addr+3] = (u8)value;
}

static __inline u16 readword(u8* buffer, u32 addr)
{
	return ((((u16)buffer[addr+0])<<8) + ((u16)buffer[addr+1]));
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

static __inline void writebyte(u8* buffer, u32 addr, u8 value)
{
	buffer[addr] = value;
}

//u32 readlong(u8* buffer, u32 addr);
//u32 read24(u8* buffer, u32 addr);
//void writelong(u8* buffer, u32 addr, u32 value);
//u16 readword(u8* buffer, u32 addr);
//void writeword(u8* buffer, u32 addr, u16 value);
//u8 readbyte(u8* buffer, u32 addr);
//void writebyte(u8* buffer, u32 addr, u8 value);
u16 powerize(u16 n);
int uncompress(u32 expected_size);
void *aligned_malloc(size_t bytes, size_t alignment);
void aligned_free(void *ptr);
const char *to_binary(u32 x);
void ppdepack(u8 *packed, u8 *depacked, u32 plen, u32 unplen);

#ifdef	__cplusplus
}
#endif

#endif /* _COLDITZ_LOW_LEVEL_H */
