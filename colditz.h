#ifndef _COLDITZ_H
#define _COLDITZ_H

#ifdef	__cplusplus
extern "C" {
#endif

#if defined(WIN32)
// Disable the _CRT_SECURE_DEPRECATE warnings of VC++
#pragma warning(disable:4996)
#endif

// On the PSP, these are defined in psp-types.h
#if !defined(PSP)
#ifndef u8
#define u8 unsigned char
#endif
#ifndef u16
#define u16 unsigned short
#endif
#ifndef s16
#define s16 short
#endif
#ifndef u32
#define u32 unsigned long
#endif
#ifndef s32
#define s32 long
#endif
#ifndef u64
#define u64 unsigned long long
#endif
#ifndef uint
#define uint unsigned int
#endif
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
#define NB_FILES				11
// Some handy identifier to make code reader friendly
#define ROOMS					0
#define CELLS					1
#define PALETTES				2
#define LOADER					3
#define COMPRESSED_MAP			4
#define SPRITES					5
#define OBJECTS					6
#define SPRITES_PANEL			7
#define TUNNEL_IO				8
#define PANEL_BASE1				9
#define PANEL_BASE2				10
#define RED						0
#define GREEN					1
#define BLUE					2
// Never be short on filename sizes
#define NAME_SIZE				256			
#define FNAMES					{ "COLDITZ_ROOM_MAPS", "COLDITZ_CELLS", "PALS.BIN", "COLDITZ-LOADER",\
								  "COMPRESSED_MAP", "SPRITES.SPR", "OBS.BIN", "PANEL.BIN",  \
								  "TUNNELIODOORS.BIN", "panel_base1.raw", "panel_base2.raw" }
#define FSIZES					{ 58828, 135944, 232, 56080, \
								  33508, 71056, 2056, 11720, \
								  120, 6144, 24576 }
#define NB_NATIONS				4
#define PANEL_BASE1_W			64
#define PANEL_BASE2_W			256
#define PANEL_BASE_H			32
#define PANEL_OFF_X				79
#define PANEL_OFF_Y				3
#define ALT_LOADER				"SKR_COLD"
#define ALT_LOADER_SIZE			28820
#define OFFSETS_START			0x2684
#define ROOMS_START				0x2FE4
#define CM_TILES_START			0x5E80
// tiles that need overlay, from LOADER
#define SPECIAL_TILES_START		0x3EBA
#define NB_SPECIAL_TILES		0x16
//Sprites
#define NB_STANDARD_SPRITES		0xD1
// Panel sprites 
#define PANEL_FACES_OFFSET		0x1482
#define NB_PANEL_FACES			7
#define PANEL_FACES_W			16
#define PANEL_FACES_X			PANEL_OFF_X
#define PANEL_TOP_Y				(PSP_SCR_HEIGHT-PANEL_BASE_H+PANEL_OFF_Y)
#define PANEL_FLAGS_OFFSET		0x1082
#define NB_PANEL_FLAGS			NB_NATIONS
#define PANEL_FLAGS_W			32
#define PANEL_FLAGS_X			(PANEL_OFF_X+4*PANEL_FACES_W)
#define NB_PANEL_ITEMS			0x13
#define PANEL_ITEMS_W			32
#define PANEL_CLOCK_DIGITS_OFF	0x1802
#define NB_PANEL_CLOCK_DIGITS	11
#define PANEL_CLOCK_DIGITS_W	8
// sid of digit 0
#define PANEL_CLOCK_DIGITS_BASE	0xDC
#define PANEL_PROPS_X			(PANEL_FLAGS_X+PANEL_FLAGS_W)
// sid of prop 0 (empty prop)
#define PANEL_PROPS_BASE		0xE7
#define PANEL_STATE_X			(PANEL_PROPS_X+PANEL_ITEMS_W)
#define PANEL_CLOCK_HOURS_X		(PANEL_STATE_X+PANEL_ITEMS_W+16)
#define PANEL_CLOCK_MINUTES_X	(PANEL_CLOCK_HOURS_X+3*PANEL_CLOCK_DIGITS_W)
#define PANEL_ITEMS_OFFSET		0x1AC2
#define NB_PANEL_SPRITES		(NB_PANEL_FLAGS+NB_PANEL_FACES+NB_PANEL_CLOCK_DIGITS+NB_PANEL_ITEMS)
#define NB_SPRITES				(NB_STANDARD_SPRITES+NB_PANEL_SPRITES)
#define NB_PANEL_CHARS			59
#define PANEL_CHARS_OFFSET		0xF20
#define PANEL_CHARS_W			8
#define PANEL_CHARS_H			6
#define PANEL_CHARS_CORRECTED_H	8
#define PANEL_MESSAGE_X			(PANEL_FACES_X+5*PANEL_FACES_W)
#define PANEL_MESSAGE_Y			(PANEL_TOP_Y+16)
#define PANEL_CHARS_GRAD_BASE	0x98F1
#define PANEL_CHARS_GRAD_INCR	0x1101
#define GRAB_TRANSPARENT_COLOUR	0x0000
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
// Room index for picked objects
#define ROOM_NO_PROP			0x0258
#define REMOVABLES_MASKS_START	0x86D8
#define REMOVABLES_MASKS_LENGTH	27
#define JOY_DEADZONE			450
// These are use to check if our footprint is out of bounds
#define TILE_MASKS_OFFSETS		0xA168
#define	TILE_MASKS_START		0xA9D8
#define MASK_EMPTY				TILE_MASKS_START
#define MASK_FULL				(TILE_MASKS_START+0x2C0)
#define TILE_MASKS_LENGTH		0x21B
#define SPRITE_FOOTPRINT		0x3FFC0000
#define TUNNEL_FOOTPRINT		0xFF000000
#define FOOTPRINT_HEIGHT		4
// Exit checks
#define EXIT_TILES_LIST			0x392E
#define EXIT_MASKS_OFFSETS		0x3964
#define EXIT_MASKS_START		0x89C6
#define NB_EXITS				27
#define EXIT_CELLS_LIST			0x3E1A
#define NB_CELLS_EXITS			22
#define ROOMS_EXITS_BASE		0x0100
#define OUTSIDE_OVL_BASE		0x525E
#define OUTSIDE_OVL_NB			13
#define TUNNEL_OVL_NB			14
#define CMP_OVERLAYS			0x535C
// For our (magical) apparition into a new room after using an exit
#define HAT_RABBIT_OFFSET		0x3E46
#define CMP_RABBIT_OFFSET		0x4350
#define HAT_RABBIT_POS_START	0x3E72
// Time between animation frames, in ms
// 66 or 67 is about as close as we can get to the original game
#define ANIMATION_INTERVAL		120
#define DIRECTION_STOPPED		-1
// Animation data
#define ANIMATION_OFFSET_BASE	0x896A
#define REMOVE_ANIMATION		-1
#define WALK_ANI				0x00
#define RUN_ANI					0x01
#define EMERGE_ANI				0x02
#define KNEEL_ANI				0x03
#define SLEEP_ANI				0x04
#define WTF_ANI					0x05
#define GER_WALK_ANI			0x06
#define GER_RUN_ANI				0x07
#define FIREPLACE_ANI			0x08
#define DOOR_HORI_OPEN_ANI		0x09
#define DOOR_HORI_CLOSE_ANI		0x0a
#define DOOR_RIGHT_OPEN_ANI		0x0b
#define DOOR_RIGHT_CLOSE_ANI	0x0c
#define DOOR_LEFT_OPEN_ANI		0x0d
#define DOOR_LEFT_CLOSE_ANI		0x0e
#define CRAWL_ANI				0x0f
#define GER_CRAWL_ANI			0x10
#define SHOT_ANI				0x11
#define GER_SHOT_ANI			0x12
#define GER_SHOOTS_ANI			0x13
#define KNEEL2_ANI				0x14
#define KNEEL3_ANI				0x15
#define GER_KNEEL_ANI			0x16

#define STATE_WALK_SID			0xF6
#define STATE_RUN_SID			0xF7
#define STATE_CRAWL_SID			0xF8
#define STATE_STOOGE_SID		0xF9

#define JOY_FIRE				0x35
// How do we need to shift our whole room up so that the seams don't show
#define NORTHWARD_HO			28
// for our z index, for overlays
// Oh, and don't try to be smart and use 0x8000, because unless you do an
// explicit cast, you will get strange things line short variables set to
// MIN_Z never equating MIN_Z in comparisons
// gcc: comparison is always false due to limited range of data type
#define MIN_Z					-32768
// For data file patching
#define FIXED_CRM_VECTOR		0x50
//#define R116_EXITS
#define MAX_ANIMATIONS			64
#define MAX_CURRENTLY_ANIMATED	16
#define NB_ANIMATED_SPRITES		23
#define NB_GUYBRUSHES			2
// The current prisoner is always our first guybrush
#define PRISONER				0

// Our guy's states
#define STATE_STOP				0
#define STATE_MOVE				1
#define STATE_CRAWL				2
#define STATE_STOOGE			3
#define STATE_SLEEP				4
#define STATE_PICK				5
#define STATE_SHOOT				6
#define STATE_SHOT				7
#define STATE_SHOWER			8

// Nationalities
#define ENGLISH					0
#define FRENCH					1
#define AMERICAN				2
#define POLISH					3

// Props
#define NB_PROPS				16
#define NB_OBSBIN				0xBD
#define ITEM_NONE				0x00
#define ITEM_LOCKPICK			0x01
#define ITEM_KEY_ONE			0x02
#define ITEM_KEY_TWO			0x03
#define ITEM_PRISONERS_UNIFORM	0x04
#define ITEM_GUARDS_UNIFORM		0x05
#define ITEM_PASS				0x06
#define ITEM_SHOVEL				0x07
#define ITEM_PICKAXE			0x08
#define ITEM_SAW				0x09
#define ITEM_RIFLE				0x0A
#define ITEM_STONE				0x0B
#define ITEM_CANDLE				0x0C
#define ITEM_PAPERS				0x0D
#define ITEM_STETHOSCOPE		0x0E
// Alright, the next item doesn't really exists, but if you add it
// manually in the original game, you get to "drop" an inflatable 
// prisoner's dummy ;)
// In the orginal, the item's identified as "ROUND TOWER"
#define ITEM_INFLATABLE_DUMMY	0x0F

// Redefinition of GLUT's special keys
#define SPECIAL_KEY_OFFSET		0x80
#define SPECIAL_KEY_F1          (GLUT_KEY_F1 + SPECIAL_KEY_OFFSET)
#define SPECIAL_KEY_F2          (GLUT_KEY_F2 + SPECIAL_KEY_OFFSET)
#define SPECIAL_KEY_F3          (GLUT_KEY_F3 + SPECIAL_KEY_OFFSET)
#define SPECIAL_KEY_F4          (GLUT_KEY_F4 + SPECIAL_KEY_OFFSET)
#define SPECIAL_KEY_F5          (GLUT_KEY_F5 + SPECIAL_KEY_OFFSET)
#define SPECIAL_KEY_F6          (GLUT_KEY_F6 + SPECIAL_KEY_OFFSET)
#define SPECIAL_KEY_F7          (GLUT_KEY_F7 + SPECIAL_KEY_OFFSET)
#define SPECIAL_KEY_F8          (GLUT_KEY_F8 + SPECIAL_KEY_OFFSET)
#define SPECIAL_KEY_F9          (GLUT_KEY_F9 + SPECIAL_KEY_OFFSET)
#define SPECIAL_KEY_F10         (GLUT_KEY_F10 + SPECIAL_KEY_OFFSET)
#define SPECIAL_KEY_F11         (GLUT_KEY_F11 + SPECIAL_KEY_OFFSET)
#define SPECIAL_KEY_F12         (GLUT_KEY_F12 + SPECIAL_KEY_OFFSET)
#define SPECIAL_KEY_LEFT        (GLUT_KEY_LEFT + SPECIAL_KEY_OFFSET)
#define SPECIAL_KEY_UP          (GLUT_KEY_UP + SPECIAL_KEY_OFFSET)
#define SPECIAL_KEY_RIGHT       (GLUT_KEY_RIGHT + SPECIAL_KEY_OFFSET)
#define SPECIAL_KEY_DOWN        (GLUT_KEY_DOWN + SPECIAL_KEY_OFFSET)
#define SPECIAL_KEY_PAGE_UP     (GLUT_KEY_PAGE_UP + SPECIAL_KEY_OFFSET)
#define SPECIAL_KEY_PAGE_DOWN   (GLUT_KEY_PAGE_DOWN + SPECIAL_KEY_OFFSET)
#define SPECIAL_KEY_HOME        (GLUT_KEY_HOME + SPECIAL_KEY_OFFSET)
#define SPECIAL_KEY_END	        (GLUT_KEY_END	+ SPECIAL_KEY_OFFSET)
#define SPECIAL_KEY_INSERT      (GLUT_KEY_INSERT + SPECIAL_KEY_OFFSET)
   



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

// Define a structure to hold the standard RGBA sprites
typedef struct
{
    u16 w;
	u16 h;
	// Politicaly correct w & h (power of twos, to keep the PSP happy)
	u16 corrected_w;
	u16 corrected_h;
	u16 x_offset;
	s16 z_offset;
	u8* data;
} s_sprite;

typedef struct
{
	u16 w;
	u16 base;
	u32 offset;
} s_nonstandard;

typedef struct
{
	s16 x;
	s16 y;
	s16 z;
	u8 sid;
} s_overlay;

typedef struct
{
	u32	index;	// index for the ani in the LOADER table
	s32	framecount;
	/* For animated overlays, direction is one of:
	 *    5  6  7
	 *    4 -1  0 
	 *    3  2  1   */
	int	direction;
	u32 end_of_ani_parameter;
	void (*end_of_ani_function)(u32);
} s_animation;

typedef struct
{
	u16   room;
	s16   px;
	s16   p2y;
	s16   speed;
	u8	  state;
	u8	  ani_index;
} s_guybrush;

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
static u8 looping_animation[NB_ANIMATED_SPRITES] =
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

static u16 props_tile [0x213] = {
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

// Global variables
extern int	opt_verbose;
extern int	opt_debug;
extern int	opt_sid;
extern int	opt_play_as_the_safe;
extern int	stat;
extern int  debug_flag;
extern u8   *mbuffer;
extern u8   *fbuffer[NB_FILES];
extern FILE *fd;
extern u8   *rgbCells;
// Removable walls current bitmask
extern u32  rem_bitmask;
extern u8	props[NB_NATIONS][NB_PROPS];
extern u8	selected_prop[NB_NATIONS];
extern u8	nb_room_props;
extern u16	room_props[NB_OBSBIN];
extern u8	over_prop, over_prop_id;
extern u8	panel_chars[NB_PANEL_CHARS][8*8*2];



// Data specific global variables
extern u16  nb_rooms, nb_cells, nb_objects;

extern char* fname[NB_FILES];
extern u32   fsize[NB_FILES];
//extern int	gl_off_x, gl_off_y;
extern int	gl_width, gl_height;
//extern u8	prisoner_w, prisoner_h;
//extern int  prisoner_x, prisoner_2y;
#define prisoner_x guybrush[PRISONER].px
#define prisoner_2y guybrush[PRISONER].p2y
#define current_room_index guybrush[PRISONER].room
#define prisoner_speed guybrush[PRISONER].speed
#define prisoner_ani   guybrush[PRISONER].ani_index
#define prisoner_state guybrush[PRISONER].state
extern u8 current_nation;
extern int  last_p_x, last_p_y;
extern int  dx, d2y;
extern u8  prisoner_sid;

//extern u16  current_room_index;
extern s_sprite		*sprite;
extern s_overlay	*overlay; 
extern u8   overlay_index;

extern bool reset_animations;
extern bool key_down[256];
extern s_animation	animations[MAX_ANIMATIONS];
extern u8	nb_animations;
extern u64	last_mtime;
extern s_guybrush	guybrush[NB_GUYBRUSHES];


// Having  a global palette saves a lot of hassle
extern u8  bPalette[3][16];
extern u16 aPalette[16];

#ifdef	__cplusplus
}
#endif


#endif /* _COLDITZ */
