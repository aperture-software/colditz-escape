#ifndef _COLDITZ_H
#define _COLDITZ 1

#ifdef	__cplusplus
extern "C" {
#endif

#if defined(WIN32)
// Disable the _CRT_SECURE_DEPRECATE warnings of VC++
#pragma warning(disable:4996)
#endif

// Define our msleep function
#if defined(WIN32)
//#include <Windows.h>
#define msleep(msecs) Sleep(msecs)
#elif defined(PSP)
#include <pspthreadman.h>
#define msleep(msecs) sceKernelDelayThread(1000*msecs); 
#else
#include <unistd.h>
#define	msleep(msecs) usleep(1000*msecs)
#endif

#if defined(__APPLE__)
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
#ifndef uint
#define uint unsigned int
#endif
#endif // __APPLE__

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
/*#if defined(PSP)
#define print(...)		pspDebugScreenPrintf(__VA_ARGS__)
#define perr(...)		pspDebugScreenPrintf(__VA_ARGS__)
#else
*/
#define perr(...)		fprintf(stderr, __VA_ARGS__)
#define print(...)		printf(__VA_ARGS__)
//#endif
#define printv(...)		if(opt_verbose) print(__VA_ARGS__)
#define perrv(...)		if(opt_verbose) perr(__VA_ARGS__)
#define printb(...)		if(opt_debug) print(__VA_ARGS__)
#define perrb(...)		if(opt_debug) perr(__VA_ARGS__)

#if !defined(bool)
#define bool int
#endif
#if !defined(true)
#define true (-1)
#endif
#if !defined(false)
#define false (0)
#endif



#define GLCHK(x)						\
do {									\
	GLint errcode;						\
		x;								\
		errcode = glGetError();			\
		if (errcode != GL_NO_ERROR) {					\
			print("%s (%d): GL error 0x%04x\n",			\
				__FUNCTION__, __LINE__, (uint) errcode);\
		}								\
	} while (0)


// # files we'll be dealing with
#define NB_FILES				8
// Some handy identifier to make code reader friendly
#define ROOMS					0
#define CELLS					1
#define PALETTES				2
#define LOADER					3
#define COMPRESSED_MAP			4
#define SPRITES					5
#define OBJECTS					6
#define PANEL					7
#define RED						0
#define GREEN					1
#define BLUE					2
// Never be short on filename sizes
#define NAME_SIZE				256			
#define FNAMES					{ "COLDITZ_ROOM_MAPS", "COLDITZ_CELLS", "PALS.BIN", "COLDITZ-LOADER",\
								  "COMPRESSED_MAP", "SPRITES.SPR", "OBS.BIN", "PANEL.RAW" }
#define FSIZES					{ 58828, 135944, 232, 56080, \
								  33508, 71056, 2056, 49152 }
#define ALT_LOADER				"SKR_COLD"
#define ALT_LOADER_SIZE			28820
#define OFFSETS_START			0x2684
#define ROOMS_START				0x2FE4
#define CM_TILES_START			0x5E80
// tiles that need overlay, from LOADER
#define SPECIAL_TILES_START		0x3EBA
#define NB_SPECIAL_TILES		0x16
#define OBS_TO_SPRITE_START		0x5D02
#define NB_OBS_TO_SPRITE		15
#define LOADER_DATA_START		0x10C
#define FFs_TO_IGNORE			7
#define MAX_OVERLAY				0x80
#define RGBA_SIZE				2
#define CMP_MAP_WIDTH			0x54
#define CMP_MAP_HEIGHT			0x48
// On compressed map (outside)
#define ROOM_OUTSIDE			0xFFFF
#define REMOVABLES_MASKS_START	0x86d8
#define REMOVABLES_MASKS_LENGTH	27
#define JOY_DEADZONE			450
// These are use to check if our footprint is out of bounds
#define TILE_MASKS_OFFSETS		0xA168
#define	TILE_MASKS_START		0xA9D8
#define TILE_MASKS_LENGTH		0x21B
#define SPRITE_FOOTPRINT		0x3FFC0000
#define TUNNEL_FOOTPRINT		0xFF000000
#define FOOTPRINT_HEIGHT		4


// Stupid VC++ doesn't know the basic formats it can actually use!
#if !defined(GL_UNSIGNED_SHORT_4_4_4_4_REV)
// NB: the _REV below is GRAB format, which is selected for 1:1 mapping on PSP
#define GL_UNSIGNED_SHORT_4_4_4_4_REV	0x8365
#endif
#if !defined(GL_CLAMP_TO_EDGE)
#define GL_CLAMP_TO_EDGE				0x812F
#endif

// PSP Screen will be our base def
#define PSP_SCR_WIDTH		480
#define PSP_SCR_HEIGHT		272

// Define a structure to hold the RGBA sprites
typedef struct
{
    u16 w;
	u16 h;
	// Politicaly correct w & h (power of twos, to keep the PSP happy)
	u16 corrected_w;
	u16 corrected_h;
	u16 x_offset;
	u8* data;
} s_sprite;

typedef struct
{
	int x;
	int y;
	u8 sid;
} s_overlay;


// Global variables
extern int	opt_verbose;
extern int	opt_debug;
extern int	stat;
extern int  debug_flag;
extern u8   *mbuffer;
extern u8   *fbuffer[NB_FILES];
extern FILE *fd;
extern u8   *rgbCells;
// Removable walls current bitmask
extern u32  rem_bitmask;



// Data specific global variables
extern u16  nb_rooms, nb_cells, nb_sprites, nb_objects;

extern char* fname[NB_FILES];
extern u32   fsize[NB_FILES];
extern int	gl_off_x, gl_off_y;
extern int	gl_width, gl_height;
extern int  prisoner_x, prisoner_2y;
extern int  last_p_x, last_p_y;
extern int  dx, d2y;
extern u8  prisoner_sid;
extern float  origin_x, origin_y;

extern u16  current_room_index;
extern s_sprite		*sprite;
extern s_overlay	*overlay; 
extern u8   overlay_index;

// Having  a global palette saves a lot of hassle
extern u8  bPalette[3][16];
extern u16 aPalette[16];

#ifdef	__cplusplus
}
#endif


#endif /* _COLDITZ */
