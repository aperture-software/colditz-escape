#ifndef _COLDITZ_H
#define _COLDITZ_H

#ifdef	__cplusplus
extern "C" {
#endif

#if defined(WIN32)
// Disable the _CRT_SECURE_DEPRECATE warnings of VC++
#pragma warning(disable:4996)
#endif

// General compilation options for the program
#define CHEATMODE_ENABLED
#define ANTI_TAMPERING_ENABLED
#define DEBUG_KEYS_ENABLED

// Stupid VC++ doesn't know the basic GL formats it can actually use!
#if !defined(GL_UNSIGNED_SHORT_4_4_4_4_REV)
// NB: the _REV below is GRAB format, which is selected for 1:1 mapping on PSP
#define GL_UNSIGNED_SHORT_4_4_4_4_REV	0x8365
#endif
#if !defined(GL_CLAMP_TO_EDGE)
#define GL_CLAMP_TO_EDGE				0x812F
#endif

// LIST OF ABREVIATIONS:
// CRM = Colditz Room Maps => data used for inside rooms
// CMP = CoMPressed map => data used for outisde


//
// Global defines
/////////////////////////////////////////////////////////////////

#define APPNAME					"colditz"

// The PSP Screen dimensions will be our base def
#define PSP_SCR_WIDTH			480
#define PSP_SCR_HEIGHT			272

// # data files from the original game
#define NB_FILES				11
// # files need reload on a new game
#define NB_FILES_TO_RELOAD		4
// Some handy identifier to make code reader friendly
#define ROOMS					0
#define COMPRESSED_MAP			1
#define OBJECTS					2
#define TUNNEL_IO				3
#define SPRITES_PANEL			4
#define GUARDS					5
#define CELLS					6
#define PALETTES				7
#define LOADER					8
#define SPRITES					9
#define ROUTES					10

#define RED						0
#define GREEN					1
#define BLUE					2
#define ALPHA					3
// Size of our internal RGBA format, in bytes
#define RGBA_SIZE				2
// Never be too short on filename sizes
#define NAME_SIZE				128			
#define FNAMES					{ "COLDITZ_ROOM_MAPS",			\
								  "COMPRESSED_MAP",				\
								  "OBS.BIN",					\
								  "TUNNELIODOORS.BIN",			\
								  "PANEL.BIN",					\
								  "MENDAT.BIN",					\
								  "COLDITZ_CELLS",				\
								  "PALS.BIN",					\
								  "COLDITZ-LOADER",				\
								  "SPRITES.SPR",				\
								  "ROUTES.BIN" }
#define FSIZES					{ 58828,		\
								  33508,		\
								  2056,			\
								  120,			\
								  11720,		\
								  1288,			\
								  135944,		\
								  232,			\
								  56080,		\
								  71056,		\
								  13364 }
// Most versions of Colditz archived on the net use the Skid Row loader
#define ALT_LOADER				"SKR_COLD"
#define ALT_LOADER_SIZE			28820

// Our little intro
#if defined(PSP)
#define APERTURE_VIDEO			"aperture.pmp"
#else
#define APERTURE_VIDEO			"aperture.avi"
#endif


/*
#define FMD5HASHES				{ { 0x0c, 0x4f, 0xeb, 0x19, 0xfc, 0x53, 0xaf, 0xa9, 0x03, 0x83, 0x24, 0xc1, 0xad, 0xa2, 0x1c, 0xe9 }, \
								  { 0xd8, 0x23, 0x9a, 0x3e, 0x68, 0xe4, 0x6f, 0x36, 0x5f, 0xf2, 0x4d, 0xca, 0x5d, 0x12, 0xfb, 0x52 }, \
								  { 0x15, 0xdc, 0x6b, 0xa1, 0x39, 0x2c, 0x9a, 0x31, 0x66, 0x1a, 0xd3, 0x78, 0xee, 0x98, 0x11, 0x62 }, \
								  { 0x24, 0x15, 0x8a, 0xe9, 0x52, 0x7d, 0x92, 0x15, 0xab, 0x4e, 0x00, 0x00, 0x32, 0x1c, 0x53, 0x75 }, \
								  { 0x10, 0xd9, 0x97, 0xad, 0x03, 0x5a, 0x4c, 0xde, 0x46, 0x5a, 0x82, 0xd9, 0x99, 0x46, 0xbe, 0x81 }, \
								  { 0x8c, 0x3f, 0x01, 0xde, 0x56, 0xf9, 0x9d, 0x1c, 0x3c, 0x09, 0x05, 0x84, 0x8e, 0x96, 0x66, 0xa8 }, \
								  { 0x0a, 0x57, 0x16, 0x00, 0x7c, 0x53, 0x2f, 0x59, 0xf4, 0x1f, 0x1c, 0xd9, 0xf3, 0x5b, 0x79, 0xd1 }, \
								  { 0x5c, 0xd4, 0xa6, 0x75, 0x8b, 0xe9, 0xf9, 0xc2, 0xff, 0xee, 0xa6, 0x72, 0xbc, 0xd6, 0x05, 0x61 }, \
								  { 0x35, 0x22, 0x3d, 0x00, 0x68, 0x2f, 0x2d, 0x3a, 0x8f, 0x8a, 0x77, 0xa7, 0xa1, 0xa9, 0x71, 0x06 }, \
								  { 0xcb, 0xe0, 0x09, 0xbe, 0x17, 0x15, 0xae, 0x03, 0xbf, 0xd6, 0x03, 0x91, 0x7f, 0x78, 0xe5, 0x67 }, \
								  { 0xb7, 0x8d, 0xbf, 0x3c, 0xdd, 0xa7, 0xfc, 0x92, 0x9a, 0x55, 0x56, 0xd2, 0x4f, 0x8f, 0x82, 0xb3 } }
*/

#if defined(ANTI_TAMPERING_ENABLED)
// Slighltly obfuscated MD5 hashes of the files (don't want to make file tampering & cheating too easy for the first prized release)
#define FMDXHASHES				{ { 0xf3, 0x5e, 0x11, 0xb3, 0x30 }, \
								  { 0x6c, 0x31, 0x12, 0x00, 0xf2 }, \
								  { 0x6b, 0x85, 0x59, 0xc1, 0xdb }, \
								  { 0x4f, 0x7d, 0x68, 0x74, 0x3a }, \
								  { 0x84, 0x1d, 0xc4, 0x82, 0xd8 }, \
								  { 0x46, 0xc8, 0xf7, 0x61, 0x68 }, \
								  { 0x03, 0xf0, 0xa7, 0xfc, 0xe3 }, \
								  { 0x98, 0x24, 0xf0, 0x26, 0xc8 }, \
								  { 0x60, 0xc1, 0xe3, 0x4a, 0xc0 }, \
								  { 0x59, 0x26, 0xc6, 0xbe, 0x68 }, \
								  { 0xc8, 0x87, 0x0a, 0x85, 0xd0 } }
#endif

// Textures that will be used for various images
#define NB_TEXTURES				21
#define NB_IFFS					19
#define TEXTURES				{	{ "PIC.1(SOLITARY)", 320, 192, 0, NULL },					\
									{ "PIC.1(SOLITARY)FREE", 320, 192, 0, NULL },				\
									{ "PIC.2(APPELL)", 320, 192, 0, NULL },						\
									{ "PIC.3(SHOT)", 320, 192, 0, NULL },						\
									{ "PIC.4(FREE-1)", 320, 192, 0, NULL },						\
									{ "PIC.5(CURFEW)", 320, 192, 0, NULL },						\
									{ "PIC.6(EXERCISE)", 320, 192, 0, NULL },					\
									{ "PIC.7(CONFINED)", 320, 192, 0, NULL },					\
									{ "PIC.A(FREE-ALL)", 320, 192, 0, NULL },					\
									{ "PIC.B(GAME-OVER)", 320, 192, 0, NULL },					\
									{ "PIC.A(FREE-ALL)TEXT", 320, 192, 0, NULL },				\
									{ "PIC.B(GAME-OVER)TEXT", 295, 192, 0, NULL },				\
									{ "STARTSCREEN0", 320, 200, 0, NULL },						\
									{ "STARTSCREEN1", 320, 200, 0, NULL },						\
									{ "STARTSCREEN2", 320, 200, 0, NULL },						\
									{ "STARTSCREEN3", 320, 200, 0, NULL },						\
									{ "STARTSCREEN4", 320, 200, 0, NULL },						\
									{ "PIC.8(PASS)", 320, 192, 0, NULL },						\
									{ "PIC.9(PAPERS)", 320, 192, 0, NULL },						\
									{ "panel_base1.raw", 64, 32, 0, NULL},						\
									{ "panel_base2.raw", 256, 32, 0, NULL}						}
															

// handy identifier for images
#define TO_SOLITARY				0
#define FROM_SOLITARY			1
#define PRISONER_SHOT			3
#define PRISONER_FREE			4
#define PRISONER_FREE_ALL		8
#define GAME_OVER				9
#define PRISONER_FREE_ALL_TEXT	10
#define GAME_OVER_TEXT			11
#define INTRO_SCREEN_START		12
#define INTRO_SCREEN_END		16
#define REQUIRE_PASS			17
#define REQUIRE_PAPERS			18
#define PANEL_BASE1				19
#define PANEL_BASE2				20
#define NO_PICTURE				-1


// Loader table containing the IFF indexes to use for various events
#define IFF_INDEX_TABLE			0x00007A80

// Music mods
#define NB_MODS					4
#define MOD_NAMES				{ "LOADTUNE.MOD", "GAMEOVER.MUS", "WHENWIN.MUS", "aperture-software.mod" }
#define MOD_LOADTUNE			0
#define MOD_GAMEOVER			1
#define MOD_WHENWIN				2
#define MOD_APERTURE			3
#define PP_LOADTUNE_NAME		"LOADTUNE.MUS"
#define PP_LOADTUNE_SIZE		66956

// If we place our loader at address 0x80, we won't have to convert the pointers from the disassembly
#define LOADER_PADDING			0x00000080
#define NB_NATIONS				4
#define NB_GUARDS				0x3D
#define MENDAT_ITEM_SIZE		0x14

#define CRM_OFFSETS_START		0x00002684
#define CRM_ROOMS_START			0x00002FE4
#define CMP_TILES_START			0x00005E80
// tiles that need overlay, from LOADER
#define SPECIAL_TILES_START		0x00003F3A
#define NB_SPECIAL_TILES		0x16
#define FIREPLACE_TILE			0xE080
#define TUNNEL_TILE_ADDON		0x1E0
//Sprites
#define NB_STANDARD_SPRITES		0xD1

// Positions for the PANEL
#define PANEL_BASE1_W			64
#define PANEL_BASE2_W			256
#define PANEL_BASE_H			32
#define PANEL_OFF_X				79
#define PANEL_OFF_Y				3
#define INITIAL_PALETTE_INDEX	4
// Panel sprites 
#define PANEL_FACES_OFFSET		0x00001482
#define NB_PANEL_FACES			7
#define PANEL_FACES_W			16
#define PANEL_FACES_X			PANEL_OFF_X
#define PANEL_FACE_IN_PRISON	0xD9
#define PANEL_FACE_SHOT			0xDA
#define PANEL_FACE_FREE			0xDB
#define PANEL_TOP_Y				(PSP_SCR_HEIGHT-PANEL_BASE_H+PANEL_OFF_Y)
#define PANEL_FLAGS_OFFSET		0x00001082
#define NB_PANEL_FLAGS			NB_NATIONS
#define PANEL_FLAGS_BASE_SID	0xD1
#define PANEL_FLAGS_W			32
#define PANEL_FLAGS_X			(PANEL_OFF_X+4*PANEL_FACES_W)
#define NB_PANEL_ITEMS			0x13
#define PANEL_ITEMS_W			32
#define PANEL_CLOCK_DIGITS_OFF	0x00001802
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
#define PANEL_ITEMS_OFFSET		0x00001AC2
#define NB_PANEL_SPRITES		(NB_PANEL_FLAGS+NB_PANEL_FACES+NB_PANEL_CLOCK_DIGITS+NB_PANEL_ITEMS+1)
#define NB_SPRITES				(NB_STANDARD_SPRITES+NB_PANEL_SPRITES)
// The fatigue bar base is the last sprite
#define PANEL_FATIGUE_SPRITE	(NB_SPRITES-1)
#define NB_PANEL_CHARS			59
#define PANEL_CHARS_OFFSET		0x00000F20
#define PANEL_CHARS_W			8
#define PANEL_CHARS_H			6
#define PANEL_CHARS_CORRECTED_H	8
#define PANEL_MESSAGE_X			(PANEL_FACES_X+5*PANEL_FACES_W)
#define PANEL_MESSAGE_Y			(PANEL_TOP_Y+16)
#define PANEL_CHARS_GRAB_BASE	0x000098F1
#define PANEL_CHARS_GRAB_INCR	0x1101
#define MESSAGE_BASE			0x00007F12
#define EXIT_MESSAGE_BASE		MESSAGE_BASE
#define PROPS_MESSAGE_BASE		(MESSAGE_BASE+16)
#define	ROOM_DESC_BASE			0x0000BCB4
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
#define AUTHORIZED_BASE			0x000020EE
#define NB_AUTHORIZED_POINTERS	7
#define AUTHORIZED_NATION_BASE	0x0000210A
#define GRAB_TRANSPARENT_COLOUR	0x0000
#define OBS_TO_SPRITE_START		0x00005D82
#define NB_OBS_TO_SPRITE		15
#define LOADER_DATA_START		0x0000010C
//#define FFs_TO_IGNORE			7
#define MAX_OVERLAYS			0x80
#define CMP_MAP_WIDTH			0x54
#define CMP_MAP_HEIGHT			0x48
// On compressed map (outside)
#define ROOM_OUTSIDE			0xFFFF
// Lower index of tunnels
#define ROOM_TUNNEL				0x0203
// Room index for picked objects
#define ROOM_NO_PROP			0x0258
#define REMOVABLES_MASKS_START	0x00008758
#define REMOVABLES_MASKS_LENGTH	27
#define JOY_DEADZONE			450
// These are use to check if our footprint is out of bounds
#define TILE_MASKS_OFFSETS		0x0000A1E8
#define	TILE_MASKS_START		0x0000AA58
#define MASK_EMPTY				TILE_MASKS_START
#define MASK_FULL				(TILE_MASKS_START+0x2C0)
#define TILE_MASKS_LENGTH		0x21B
#define SPRITE_FOOTPRINT		0x3FFC0000
#define TUNNEL_FOOTPRINT		0xFF000000
#define FOOTPRINT_HEIGHT		4
// Exit checks
#define EXIT_TILES_LIST			0x000039AE
#define EXIT_MASKS_OFFSETS		0x000039E4
#define EXIT_MASKS_START		0x00008A46
#define NB_EXITS				27
#define NB_TUNNEL_EXITS			7
#define IN_TUNNEL_EXITS_START	5
#define TUNNEL_EXIT_TILES_LIST	0x00002AEA
#define TUNNEL_EXIT_TOOLS_LIST	0x00002AF8
#define EXIT_CELLS_LIST			0x00003E9A
#define NB_CELLS_EXITS			22
#define ROOMS_EXITS_BASE		0x00000100
#define OUTSIDE_OVL_BASE		0x000052DE
#define OUTSIDE_OVL_NB			13
#define TUNNEL_OVL_NB			14
#define CMP_OVERLAYS			0x000053DC
// For our (magical) apparition into a new room after using an exit
#define HAT_RABBIT_OFFSET		0x00003EC6
#define CMP_RABBIT_OFFSET		0x000043D0
#define HAT_RABBIT_POS_START	0x00003EF2
#define INITIAL_POSITION_BASE	0x0000773A
#define SOLITARY_POSITION_BASE	0x0000292C
// Time between animation frames, in ms
// 66 or 67 is about as close as we can get to the original game
#define ANIMATION_INTERVAL		120
// 20 ms provides the same speed (for patrols) as on the Amiga
#define	REPOSITION_INTERVAL		15
// defines how long a transition takes on animated picture effects
#define TRANSITION_DURATION		1000
// How long should we sleep when paused or between each fade step (ms)
#define PAUSE_DELAY				40
// Muhahahahahaha!!! Fear not, mere mortals, for I'll...
//#define TIME_MARKER				20000
#define TIME_MARKER				10000
// NB: This is the duration of a game minute, in ms
#define SOLITARY_DURATION		100000
// How long should we keep a static picture on, in ms
#define PICTURE_TIMEOUT			12000
// Time we should keep our inventory messages, in ms
#define	PROPS_MESSAGE_TIMEOUT	2000
#define CHEAT_MESSAGE_TIMEOUT	2000
#define NO_MESSAGE_TIMEOUT		0

#define NB_SFXS					5
#define SFX_TABLE_START			0x0000CA3E
#define SFX_ADDRESS_START		0x0000CA6A
#define SFX_DOOR				0
#define SFX_WTF					1
#define SFX_SAW					2
#define SFX_FOOTSTEPS			3
#define SFX_SHOOT				4

// How long should the guard remain blocked (in nb of route steps)
// default of the game is 0x64
#define BLOCKED_GUARD_TIMEOUT	0xC0
#define WALKING_PURSUIT_TIMEOUT	0x64
#define RUNNING_PURSUIT_TIMEOUT	0x64
#define SHOOTING_GUARD_TIMEOUT	0x14
// Timed events from LOADER (roll call, palette change)
#define TIMED_EVENTS_BASE		0x00002BDE
// First timed event of the game
#define TIMED_EVENTS_INIT		0x00002C1A
// Palette change type
#define TIMED_EVENT_PALETTE		0xFFFF	
// Rollcall check
#define TIMED_EVENT_ROLLCALL_CHECK	1
#define DIRECTION_STOPPED		-1

// Animation data
#define ANIMATION_OFFSET_BASE	0x000089EA
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

// these sprites are information messages for the panel
#define STATE_WALK_SID			0xF6
#define STATE_RUN_SID			0xF7
#define STATE_CRAWL_SID			0xF8
#define STATE_STOOGE_SID		0xF9

// doubt we'll need more than simultaneously enqueued events in all
#define NB_EVENTS				4

// How much we need to shift our screen so the seams don't show
#define NORTHWARD_HO			28

// Our minimal z index, for overlays
// Oh, and don't try to be smart and use 0x8000, because unless you do an
// explicit cast, you will get strange things line short variables that you
// explicitely set to MIN_Z never equating MIN_Z in comparisons
// NB: gcc will issue a "comparison is always false due to limited range of data type"
#define MIN_Z					-32768
// For data file patching
#define FIXED_CRM_VECTOR		0x50

// For animations that are NOT guybrushes (guybrushes embed their own animation struct)
#define MAX_ANIMATIONS			0x20
#define MAX_CURRENTLY_ANIMATED	MAX_ANIMATIONS
#define NB_ANIMATED_SPRITES		23
#define NB_GUYBRUSHES			(NB_NATIONS + NB_GUARDS)

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
#define GAME_STATE_GAME_WON		32
#define GAME_STATE_PICTURE_LOOP	64
#define GAME_STATE_CUTSCENE		128

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

// States for the displaying of static pictures
#define GAME_FADE_OUT_START		0
#define GAME_FADE_OUT			1
#define PICTURE_FADE_IN_START	2
#define PICTURE_FADE_IN			3
#define PICTURE_WAIT_START		4
#define PICTURE_WAIT			5
#define PICTURE_FADE_OUT_START	6
#define PICTURE_FADE_OUT		7
#define GAME_FADE_IN_START		8
#define GAME_FADE_IN			9
#define PICTURE_EXIT			10
//#define NB_PICTURE_STATES		11
//#define FADE_INCREMENT			0.05f

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

#if defined(DEBUG_KEYS_ENABLED)
#define KEY_DEBUG_PRINT_POS		'p'
#define KEY_DEBUG_BONANZA		'#'
#define KEY_DEBUG_CATCH_HIM		'c'
#endif


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

// Sound FX
typedef struct 
{
	u32				address;
	u16				length;
	u16				psp_length;
	u16				frequency;
	u8				volume;
	short*			upconverted_address;
	unsigned long	upconverted_length;
} s_sfx;

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
extern u16	game_state;
extern float fade_value;
extern int current_picture;


// Data specific global variables
extern u16  nb_rooms, nb_cells, nb_objects;

extern char* fname[NB_FILES];
extern u32   fsize[NB_FILES];
//extern char* iff_name[NB_IFFS];
extern char* mod_name[NB_MODS];
//extern u16   iff_payload_w[NB_IFFS];
//extern u16   iff_payload_h[NB_IFFS];
//extern char* raw_name[NB_RAWS];
extern int	gl_width, gl_height;
extern u8 current_nation;
extern s_sfx sfx[NB_SFXS];
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
extern u64	game_time, last_atime, last_ptime, last_ctime, t_last;
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
		t_status_message_timeout = game_time + timeout_duration;	
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
