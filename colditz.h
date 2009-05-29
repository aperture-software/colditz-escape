#ifndef _COLDITZ_H
#define _COLDITZ_H

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


#define CHEATMODE_ENABLED

// # files we'll be dealing with
#define NB_FILES				13
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
#define GUARDS					11
#define ROUTES					12
#define RED						0
#define GREEN					1
#define BLUE					2
// Never be too short on filename sizes
#define NAME_SIZE				128			
#define FNAMES					{ "COLDITZ_ROOM_MAPS", "COLDITZ_CELLS", "PALS.BIN", "COLDITZ-LOADER",	\
								  "COMPRESSED_MAP", "SPRITES.SPR", "OBS.BIN", "PANEL.BIN",				\
								  "TUNNELIODOORS.BIN", "panel_base1.raw", "panel_base2.raw",			\
								  "MENDAT.BIN", "ROUTES.BIN" }
#define FSIZES					{ 58828, 135944, 232, 56080,	\
								  33508, 71056, 2056, 11720,	\
								  120, 6144, 24576,				\
								  1288, 13364 }

// Static IFF images (intro, events, gameover, etc)
#define NB_IFFS					19
#define IFF_NAMES				{ "PIC.1(SOLITARY)", "PIC.1(SOLITARY)FREE", "PIC.2(APPELL)", "PIC.3(SHOT)",	\
								  "PIC.4(FREE-1)", "PIC.5(CURFEW)", "PIC.6(EXERCISE)", "PIC.7(CONFINED)",	\
								  "PIC.A(FREE-ALL)", "PIC.B(GAME-OVER)", "PIC.A(FREE-ALL)TEXT",				\
								  "PIC.B(GAME-OVER)TEXT", "STARTSCREEN0", "STARTSCREEN1", "STARTSCREEN2",	\
								  "STARTSCREEN3", "STARTSCREEN4", "PIC.8(PASS)", "PIC.9(PAPERS)" }
// Additional Static RGB RAW images. Those images all have a 480x272 PSP dimension
#define NB_RAWS					1
#define RAW_NAMES				{ "aperture-software.raw" }
// Music mods
#define NB_MODS					1
#define MOD_NAMES				{ "LOADTUNE.MOD" }
#define TO_SOLITARY				0
#define FROM_SOLITARY			1
#define PRISONER_SHOT			3
#define PRISONER_FREE			4
#define PRISONER_FREE_ALL		8
#define GAME_OVER				9
#define PRISONER_FREE_ALL_TEXT	10
#define GAME_OVER_TEXT			11
#define REQUIRE_PASS			17
#define REQUIRE_PAPERS			18
#define APERTURE_SOFTWARE		(0+ NB_IFFS)
#define NO_PICTURE				0xFFFF
#define IFF_PAYLOAD_W			{ 320, 320, 320, 320, 320, 320, 320, 320, 320, 320,	\
								  320, 320, 320, 320, 320, 320, 320, 320, 320 }
#define IFF_PAYLOAD_H			{ 192, 192, 192, 192, 192, 192, 192, 192, 192, 192,	\
								  192, 192, 200, 200, 200, 200, 200, 192, 192 }
// Loader table containing the IFF indexes to use for various events
#define IFF_INDEX_TABLE			0x7A80

// If we start the loader at address 0x80, we won't have to convert the pointers
#define LOADER_PADDING			0x80
#define NB_NATIONS				4
#define NB_GUARDS				0x3D
#define MENDAT_ITEM_SIZE		0x14
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
#define SPECIAL_TILES_START		0x3F3A
#define NB_SPECIAL_TILES		0x16
#define FIREPLACE_TILE			0xE080
#define TUNNEL_TILE_ADDON		0x1E0
//Sprites
#define NB_STANDARD_SPRITES		0xD1
// Panel sprites 
#define PANEL_FACES_OFFSET		0x1482
#define NB_PANEL_FACES			7
#define PANEL_FACES_W			16
#define PANEL_FACES_X			PANEL_OFF_X
#define PANEL_FACE_IN_PRISON	0xD9
#define PANEL_FACE_SHOT			0xDA
#define PANEL_FACE_FREE			0xDB
#define PANEL_TOP_Y				(PSP_SCR_HEIGHT-PANEL_BASE_H+PANEL_OFF_Y)
#define PANEL_FLAGS_OFFSET		0x1082
#define NB_PANEL_FLAGS			NB_NATIONS
#define PANEL_FLAGS_BASE_SID	0xD1
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
#define PANEL_FATIGUE_X			(PANEL_CLOCK_MINUTES_X + 32)
#define PANEL_FATIGUE_Y			(PANEL_TOP_Y+10)
// 0x2C000 = 88 pixels * 0x800
#define MAX_FATIGUE				0x2C000
#define HOURLY_FATIGUE_INCREASE	0x1000
#define PANEL_ITEMS_OFFSET		0x1AC2
#define NB_PANEL_SPRITES		(NB_PANEL_FLAGS+NB_PANEL_FACES+NB_PANEL_CLOCK_DIGITS+NB_PANEL_ITEMS+1)
#define NB_SPRITES				(NB_STANDARD_SPRITES+NB_PANEL_SPRITES)
// The fatigue bar base is the last sprite
#define PANEL_FATIGUE_SPRITE	(NB_SPRITES-1)
#define NB_PANEL_CHARS			59
#define PANEL_CHARS_OFFSET		0xF20
#define PANEL_CHARS_W			8
#define PANEL_CHARS_H			6
#define PANEL_CHARS_CORRECTED_H	8
#define PANEL_MESSAGE_X			(PANEL_FACES_X+5*PANEL_FACES_W)
#define PANEL_MESSAGE_Y			(PANEL_TOP_Y+16)
#define PANEL_CHARS_GRAB_BASE	0x98F1
#define PANEL_CHARS_GRAB_INCR	0x1101
#define MESSAGE_BASE			0x7F12
#define EXIT_MESSAGE_BASE		MESSAGE_BASE
#define PROPS_MESSAGE_BASE		(MESSAGE_BASE+16)
#define	ROOM_DESC_BASE			0xBCB4
#define TUNNEL_MSG_ID			0x35
#define	COURTYARD_MSG_ID		0x36
// Boundaries for courtyard authorized access (ROM:00002160)
#define COURTYARD_MIN_X			0x300
#define COURTYARD_MAX_X			0x5A0
#define COURTYARD_MIN_Y			0xD0
#define COURTYARD_MAX_Y			0x200
// Boundaries for succesful escape! (ROM:00001E6E)
#define ESCAPE_MIN_X			0x20
#define ESCAPE_MAX_X			0xA40
#define ESCAPE_MIN_Y			0x08
#define ESCAPE_MAX_Y			0x470
// Where to send someone we don't want to see around us again
#define GET_LOST_X				5000;
#define GET_LOST_Y				5000;
// This loader section defines the list of authorized rooms (through their 
// message ID) after certain events (appel, exercise, confined)
#define AUTHORIZED_BASE			0x20EE
#define NB_AUTHORIZED_POINTERS	7
#define AUTHORIZED_NATION_BASE	0x210A
#define GRAB_TRANSPARENT_COLOUR	0x0000
#define OBS_TO_SPRITE_START		0x5D82
#define NB_OBS_TO_SPRITE		15
#define LOADER_DATA_START		0x10C
#define FFs_TO_IGNORE			7
#define MAX_OVERLAYS			0x80
#define RGBA_SIZE				2
#define CMP_MAP_WIDTH			0x54
#define CMP_MAP_HEIGHT			0x48
// On compressed map (outside)
#define ROOM_OUTSIDE			0xFFFF
// Lower index of tunnels
#define ROOM_TUNNEL				0x0203
// Room index for picked objects
#define ROOM_NO_PROP			0x0258
#define REMOVABLES_MASKS_START	0x8758
#define REMOVABLES_MASKS_LENGTH	27
#define JOY_DEADZONE			450
// These are use to check if our footprint is out of bounds
#define TILE_MASKS_OFFSETS		0xA1E8
#define	TILE_MASKS_START		0xAA58
#define MASK_EMPTY				TILE_MASKS_START
#define MASK_FULL				(TILE_MASKS_START+0x2C0)
#define TILE_MASKS_LENGTH		0x21B
#define SPRITE_FOOTPRINT		0x3FFC0000
#define TUNNEL_FOOTPRINT		0xFF000000
#define FOOTPRINT_HEIGHT		4
// Exit checks
#define EXIT_TILES_LIST			0x39AE
#define EXIT_MASKS_OFFSETS		0x39E4
#define EXIT_MASKS_START		0x8A46
#define NB_EXITS				27
#define NB_TUNNEL_EXITS			7
#define IN_TUNNEL_EXITS_START	5
#define TUNNEL_EXIT_TILES_LIST	0x2AEA
#define TUNNEL_EXIT_TOOLS_LIST	0x2AF8
#define EXIT_CELLS_LIST			0x3E9A
#define NB_CELLS_EXITS			22
#define ROOMS_EXITS_BASE		0x0100
#define OUTSIDE_OVL_BASE		0x52DE
#define OUTSIDE_OVL_NB			13
#define TUNNEL_OVL_NB			14
#define CMP_OVERLAYS			0x53DC
// For our (magical) apparition into a new room after using an exit
#define HAT_RABBIT_OFFSET		0x3EC6
#define CMP_RABBIT_OFFSET		0x43D0
#define HAT_RABBIT_POS_START	0x3EF2
#define INITIAL_POSITION_BASE	0x773A
#define SOLITARY_POSITION_BASE	0x292C
// Time between animation frames, in ms
// 66 or 67 is about as close as we can get to the original game
#define ANIMATION_INTERVAL		120
// 20 ms provides the same speed (for patrols) as on the Amiga
#define	REPOSITION_INTERVAL		15
// How long should we sleep when paused or between each fade step (ms)
#define PAUSE_DELAY				40
// Muhahahahahaha!!! Fear not, mere mortals, for I will...
//#define TIME_MARKER				20000
#define TIME_MARKER				10000
// NB: This is the duration of a game minute, in ms
#define SOLITARY_DURATION		100000
// How long should we keep a static picture on, in ms
#define PICTURE_TIMEOUT			5000
// Time we should keep our inventory messages, in ms
#define	PROPS_MESSAGE_TIMEOUT	2000
#define CHEAT_MESSAGE_TIMEOUT	2000
#define NO_MESSAGE_TIMEOUT		0


// How long should the guard remain blocked (innb of route steps)
// default of the game is 0x64
#define BLOCKED_GUARD_TIMEOUT	0xC0
#define WALKING_PURSUIT_TIMEOUT	0x64
#define RUNNING_PURSUIT_TIMEOUT	0x64
#define SHOOTING_GUARD_TIMEOUT	0x14
// Timed events from LOADER (roll call, palette change)
#define TIMED_EVENTS_BASE		0x2BDE
// First timed event of the game
#define TIMED_EVENTS_INIT		0x2C1A
// Palette change type
#define TIMED_EVENT_PALETTE		0xFFFF	
// Rollcall check
#define TIMED_EVENT_ROLLCALL_CHECK	1
#define DIRECTION_STOPPED		-1
// Animation data
#define ANIMATION_OFFSET_BASE	0x89EA
// sids for animation removal or no display
#define REMOVE_ANIMATION_SID	-1
#define WALK_ANI				0x00
#define RUN_ANI					0x01
#define EMERGE_ANI				0x02
#define KNEEL_ANI				0x03
#define SLEEP_ANI				0x04
#define WTF_ANI					0x05
#define GUARD_WALK_ANI			0x06
#define GUARD_RUN_ANI			0x07
#define FIREPLACE_ANI			0x08
#define DOOR_HORI_OPEN_ANI		0x09
#define DOOR_HORI_CLOSE_ANI		0x0a
#define DOOR_RIGHT_OPEN_ANI		0x0b
#define DOOR_RIGHT_CLOSE_ANI	0x0c
#define DOOR_LEFT_OPEN_ANI		0x0d
#define DOOR_LEFT_CLOSE_ANI		0x0e
#define CRAWL_ANI				0x0f
#define GUARD_CRAWL_ANI			0x10
#define SHOT_ANI				0x11
#define GUARD_SHOT_ANI			0x12
#define GUARD_SHOOTS_ANI		0x13
#define KNEEL2_ANI				0x14
#define KNEEL3_ANI				0x15
#define GUARD_KNEEL_ANI			0x16

#define STATE_WALK_SID			0xF6
#define STATE_RUN_SID			0xF7
#define STATE_CRAWL_SID			0xF8
#define STATE_STOOGE_SID		0xF9

// doubt we'll need more than simultaneously enqueued events in all
#define NB_EVENTS				4

// How much we need to shift our screen so the seams don't show
#define NORTHWARD_HO			28
// for our z index, for overlays
// Oh, and don't try to be smart and use 0x8000, because unless you do an
// explicit cast, you will get strange things line short variables that you
// explicitely set to MIN_Z never equating MIN_Z in comparisons
// NB: gcc will issue a "comparison is always false due to limited range of data type"
#define MIN_Z					-32768
// For data file patching
#define FIXED_CRM_VECTOR		0x50
//#define R116_EXITS
// For animations that are NOT guybrushes (guybrushes embed their own animation struct)
#define MAX_ANIMATIONS			0x20
#define MAX_CURRENTLY_ANIMATED	MAX_ANIMATIONS
#define NB_ANIMATED_SPRITES		23
// indiactes that no guybrush is associated to an animation
#define NO_GUYBRUSH				-1
#define NB_GUYBRUSHES			(NB_NATIONS + NB_GUARDS)
// The current prisoner is always our first guybrush
//#define PRISONER				0

// Our guy's states
// bit 1 = move/stop
// bit 2 = in_tunnel
// bit 3 = stooge
// bit 4 = blocked
// bit 5 = sleep
// bit 6 = shower
// bit 7 = shooting
// bit 8 = shot
/*
#define STATE_STOP				0
#define STATE_MOVE				1
#define STATE_CRAWL				2
#define STATE_STOOGE			3
#define STATE_SLEEP				4
#define STATE_PICK				5
#define STATE_SHOOT				6
#define STATE_SHOT				7
#define STATE_SHOWER			8
#define STATE_BLOCKED_STOP		9
#define STATE_BLOCKED_MOVE		10
*/
// Motion related states
#define STATE_MOTION			1
#define STATE_TUNNELING			2
#define STATE_STOOGE			4
#define STATE_BLOCKED			8
#define STATE_KNEEL				16
#define STATE_SHOT				32
#define STATE_IN_PURSUIT		64
#define STATE_SLEEPING			128
#define STATE_IN_PRISON			256
// Useful masks
#define MOTION_DISALLOWED		(~(STATE_MOTION|STATE_TUNNELING|STATE_STOOGE|STATE_IN_PURSUIT|STATE_SHOT|STATE_IN_PRISON))
#define KNEEL_DISALLOWED		(~(STATE_MOTION|STATE_STOOGE|STATE_IN_PURSUIT|STATE_IN_PRISON))
#define STATE_ANIMATED			(STATE_MOTION|STATE_KNEEL)

// Game states
#define GAME_STATE_ACTION		1
#define GAME_STATE_PAUSED		2
#define GAME_STATE_STATIC_PIC	4
#define GAME_STATE_INTRO		8
#define GAME_STATE_GAME_OVER	16

// Nationalities
#define BRITISH					0
#define FRENCH					1
#define AMERICAN				2
#define POLISH					3
#define GUARD					4

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
// manually in the original game, you get to drop an "inflatable" 
// prisoner's dummy ;)
// In the orginal, the item's identified as "ROUND TOWER"
#define ITEM_INFLATABLE_DUMMY	0x0F

// Because the colours are 4 or 5 bits, starting a fade at 0 is 
// not advisable
//#define FADE_START_VALUE		0.1f

// States for the displaying of static pictures
#define GAME_FADE_OUT		0
#define PICTURE_FADE_IN		1
#define PICTURE_WAIT		2
#define PICTURE_FADE_OUT	3
#define GAME_FADE_IN		4
#define PICTURE_EXIT		5
#define NB_PICTURE_STATES	6
#define FADE_INCREMENT		0.05f

// Redefinition of GLUT's special keys so that they fit in our key table
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
// Same for mouse buttons (PSP's triggers are mapped to mouse buttons)
// The GLUT_KEYs above map to [01-0C] & [64-6C], and the mouse buttons range
// from 0 to 2, so we can safely use...
#define SPECIAL_MOUSE_BUTTON_BASE	0x90
#define SPECIAL_LEFT_MOUSE_BUTTON	(SPECIAL_MOUSE_BUTTON_BASE+GLUT_LEFT_BUTTON)
#define SPECIAL_MIDDLE_MOUSE_BUTTON	(SPECIAL_MOUSE_BUTTON_BASE+GLUT_MIDDLE_BUTTON)
#define SPECIAL_RIGHT_MOUSE_BUTTON	(SPECIAL_MOUSE_BUTTON_BASE+GLUT_RIGHT_BUTTON)

#define KEY_INVENTORY_LEFT		SPECIAL_KEY_LEFT
#define KEY_INVENTORY_RIGHT		SPECIAL_KEY_RIGHT
#define KEY_INVENTORY_PICKUP	SPECIAL_KEY_UP	
#define KEY_INVENTORY_DROP		SPECIAL_KEY_DOWN

// Key mappings
#if defined(PSP)
// q = square, d = triangle, s = select, a = start
#define KEY_FIRE				'x'
#define KEY_TOGGLE_WALK_RUN		'o'
#define KEY_SLEEP				'q'
#define KEY_STOOGE				'd'
#define KEY_ESCAPE				'a'
#define KEY_PAUSE				's'
#define KEY_PRISONERS_LEFT		SPECIAL_LEFT_MOUSE_BUTTON
#define KEY_PRISONERS_RIGHT		SPECIAL_RIGHT_MOUSE_BUTTON
// The following are unused on the PSP
#define KEY_BRITISH				0
#define KEY_FRENCH				0
#define KEY_AMERICAN			0
#define KEY_POLISH				0
#define KEY_DIRECTION_LEFT		0
#define KEY_DIRECTION_RIGHT		0
#define KEY_DIRECTION_UP		0
#define KEY_DIRECTION_DOWN		0
#else
#define KEY_FIRE				'5'
#define KEY_TOGGLE_WALK_RUN		' '
#define KEY_STOOGE				'x'
#define KEY_ESCAPE				27
#define KEY_PAUSE				SPECIAL_KEY_F5
#define KEY_SLEEP				SPECIAL_KEY_F9
#define KEY_PRISONERS_LEFT		0
#define KEY_PRISONERS_RIGHT		0
// The following are unused on the PSP
#define KEY_BRITISH				SPECIAL_KEY_F1
#define KEY_FRENCH				SPECIAL_KEY_F2
#define KEY_AMERICAN			SPECIAL_KEY_F3
#define KEY_POLISH				SPECIAL_KEY_F4
#define KEY_DIRECTION_LEFT		'4'
#define KEY_DIRECTION_RIGHT		'6'
#define KEY_DIRECTION_UP		'8'
#define KEY_DIRECTION_DOWN		'2'
#endif

#define KEY_DEBUG_PRINT_POS		'p'
#define KEY_DEBUG_BONANZA		'#'
#define KEY_DEBUG_CATCH_HIM		'c'





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
	s16 x_offset;
	s16 z_offset;
	u8* data;
} s_sprite;

// for nonstandtard sprites (panel, etc)
typedef struct
{
	u16 w;
	u16 base;
	u32 offset;
} s_nonstandard;

// For room overlays (props, bed, stairs, etc)
typedef struct
{
	s16 x;
	s16 y;
	s16 z;
	u8  sid;
} s_overlay;

// Animated sprites data
typedef struct
{
	u32	index;	// index for the ani in the LOADER table
	s32	framecount;
//	s16	guybrush_index;
	u32 end_of_ani_parameter;
	void (*end_of_ani_function)(u32);
} s_animation;

// Timed events
typedef struct
{
	u64	expiration_time;
	u32 parameter;
	void (*function)(u32);
} s_event;

// prisoners or guards
typedef struct
{
	u16				room;
	s16				px;
	s16				p2y;
	s16				speed;
	/* For animated overlays, direction is one of:
	 *    5  6  7
	 *    4 -1  0 
	 *    3  2  1   */
	s16				direction;
	u32				ext_bitmask;
	u16				state;
	s_animation		animation;
//	u8	  ani_index;
	bool			reset_animation;
	bool			is_dressed_as_guard;
	bool			is_onscreen;
	u32				go_on;
	u16				wait;
} s_guybrush;

// Event related states (apply to prisoners only)
typedef struct
{
	bool require_pass;
	bool require_papers;
	bool to_solitary;
	bool unauthorized;
	bool display_shot;
	bool killed;
	bool escaped;
	u32  fatigue;
	u32  solitary_countdown;
	u8   caught_by;
} s_prisoner_event;


// Global variables
extern int	opt_verbose;
extern int	opt_debug;
extern int	opt_sid;
extern int	opt_play_as_the_safe;
extern int	opt_keymaster;
extern int	opt_thrillerdance;
extern int	opt_no_guards;
extern bool	opt_haunted_castle;
extern int	nb_escaped;
extern int	stat;
extern int  debug_flag;
extern u8   *mbuffer;
extern u8   *fbuffer[NB_FILES];
extern FILE *fd;
extern u8   *rgbCells;
extern u8*  static_image_buffer;
// Removable walls current bitmask
extern u32  rem_bitmask;
extern u8	props[NB_NATIONS][NB_PROPS];
extern u8	selected_prop[NB_NATIONS];
extern s_prisoner_event p_event[NB_NATIONS];
extern u8	nb_room_props;
extern u16	room_props[NB_OBSBIN];
extern u8	over_prop, over_prop_id;
extern u8	panel_chars[NB_PANEL_CHARS][8*8*2];
extern char* status_message;
extern int	 status_message_priority;
extern s16 directions[3][3], dir_to_dx[8], dir_to_d2y[8];
extern u8  hours_digit_h, hours_digit_l, minutes_digit_h, minutes_digit_l;
extern u8  palette_index;
//extern u16 current_iff;
//extern bool  static_picture;
extern u16	game_state;
extern float fade_value;


// Data specific global variables
extern u16  nb_rooms, nb_cells, nb_objects;

extern char* fname[NB_FILES];
extern u32   fsize[NB_FILES];
extern char* iff_name[NB_IFFS];
extern char* mod_name[NB_MODS];
extern u16   iff_payload_w[NB_IFFS];
extern u16   iff_payload_h[NB_IFFS];
extern char* raw_name[NB_RAWS];
extern int	gl_width, gl_height;
//extern u8	prisoner_w, prisoner_h;
//extern int  prisoner_x, prisoner_2y;
extern u8 current_nation;
#define prisoner_x			guybrush[current_nation].px
#define prisoner_2y			guybrush[current_nation].p2y
#define current_room_index	guybrush[current_nation].room
#define prisoner_speed		guybrush[current_nation].speed
#define prisoner_ani		guybrush[current_nation].animation
#define prisoner_reset_ani	guybrush[current_nation].reset_animation
#define prisoner_state		guybrush[current_nation].state
#define prisoner_dir		guybrush[current_nation].direction
#define rem_bitmask			guybrush[current_nation].ext_bitmask
#define guy(i)				guybrush[i]
#define guard(i)			guy(i+NB_NATIONS)
#define in_tunnel			(prisoner_state&STATE_TUNNELING)
#define is_dead				(prisoner_state&STATE_SHOT)
#define has_escaped			p_event[current_nation].escaped
#define is_outside			(current_room_index==ROOM_OUTSIDE)
#define is_inside			(current_room_index!=ROOM_OUTSIDE)
#define prisoner_as_guard	guybrush[current_nation].is_dressed_as_guard
#define prisoner_fatigue	p_event[current_nation].fatigue
extern s16  last_p_x, last_p_y;
extern s16  dx, d2y;
extern u8  prisoner_sid;

extern s_sprite		*sprite;
extern s_overlay	*overlay; 
extern u8   overlay_index;

extern bool init_animations;
extern bool is_fire_pressed;
extern char nb_props_message[32];
extern u64	t;
extern u64  t_status_message_timeout;


extern bool key_down[256], key_readonce[256];
static __inline bool read_key_once(u8 k)
{	
	if (key_down[k])
	{
		if (key_readonce[k])
			return false;
		key_readonce[k] = true;
		return true;
	}
	return false;
}




// A few defintions to make prop handling and status messages more readable
static __inline void set_status_message(void* msg, int priority, u64 timeout_duration)	
{
	if (priority >= status_message_priority)
	{
		t_status_message_timeout = t + timeout_duration;	
		status_message = (char*)(msg);
		status_message_priority = priority;
	}
}

static __inline void consume_prop()
{	// we can use an __inline here because we deal with globals
	if (!opt_keymaster)
	{	// consume the prop
		props[current_nation][selected_prop[current_nation]]--;
		if (props[current_nation][selected_prop[current_nation]] == 0)
		// display the empty box if last prop
			selected_prop[current_nation] = 0;
	}
}

#define update_props_message(prop_id)												\
	nb_props_message[1] = (props[current_nation][prop_id] / 10) + 0x30;				\
	nb_props_message[2] = (props[current_nation][prop_id] % 10) + 0x30;				\
	strcpy(nb_props_message+6, (char*) fbuffer[LOADER] + readlong(fbuffer[LOADER],	\
		PROPS_MESSAGE_BASE + 4*(prop_id-1)) + 1);									

#define show_prop_count()															\
	update_props_message(selected_prop[current_nation]);							\
	set_status_message(nb_props_message, 1, PROPS_MESSAGE_TIMEOUT)




extern s_animation	animations[MAX_ANIMATIONS];
extern u8	nb_animations;
extern s_guybrush	guybrush[NB_GUYBRUSHES];
extern s_event		events[NB_EVENTS];


// Having  a global palette saves a lot of hassle
extern u8  bPalette[3][16];
extern u16 aPalette[32];	// This palette is 32 instead of 16, because we also use
							// it to load 5 bpp IFF images

void static_screen(u8 iff_id, void (*func)(u32), u32 param);


#ifdef	__cplusplus
}
#endif


#endif /* _COLDITZ */
