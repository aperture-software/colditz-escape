/*
 *  Colditz Escape! - Rewritten Engine for "Escape From Colditz"
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
 *  colditz.h: Game engine global definitions
 *  ---------------------------------------------------------------------------
 */


#pragma once

#ifdef	__cplusplus
extern "C" {
#endif

// General compilation options for the program
#define CHEATMODE_ENABLED
#define ANTI_TAMPERING_ENABLED

#define IGNORE_RETVAL(expr) do { (void)(expr); } while(0)

/*
 *	LIST OF ABREVIATIONS:
 *	CRM = Colditz Room Maps => data used for inside rooms
 *	CMP = CoMPressed map => data used for outisde
 */

/*
 *	Global defines
 */

#define APPNAME					"colditz"
// NB: Make sure you use capital V for version as we don't have lowercase
// in our menu font (where we display this version as well)
#define VERSION					"V0.9.4"
#define COLDITZ_URL				"HTTP://SITES.GOOGLE.COM/SITE/COLDITZESCAPE"

/*
 * Graphics
 */
// The PSP Screen dimensions will be our base def
#define PSP_SCR_WIDTH			480
#define PSP_SCR_HEIGHT			272
// How much should we shift our screen so the seams don't show
#define NORTHWARD_HO			28

// Color handling
#define RED						0
#define GREEN					1
#define BLUE					2
#define ALPHA					3
// Size of our internal RGBA format, in bytes
#define RGBA_SIZE				2
#define GRAB_TRANSPARENT_COLOUR	0x0000

/*
 *	Files
 */
// # data files from the original game
#define NB_FILES				11
// # files that need reload on a new game
#define NB_FILES_TO_RELOAD		4
// # file buffers that go into a savegame
#define NB_FILES_TO_SAVE		5
// Some handy identifier for files to make code reader friendly
#define ROOMS					0
#define COMPRESSED_MAP			1
#define OBJECTS					2
#define TUNNEL_IO				3
#define GUARDS					4
#define ROUTES					5
#define SPRITES_PANEL			6
#define CELLS					7
#define PALETTES				8
#define LOADER					9
#define SPRITES					10

// Files definitions
#define FNAMES					{ "COLDITZ_ROOM_MAPS",	\
								  "COMPRESSED_MAP",		\
								  "OBS.BIN",			\
								  "TUNNELIODOORS.BIN",	\
								  "MENDAT.BIN",			\
								  "ROUTES.BIN",			\
								  "PANEL.BIN",			\
								  "COLDITZ_CELLS",		\
								  "PALS.BIN",			\
								  "COLDITZ-LOADER",		\
								  "SPRITES.SPR"			}
#define FSIZES					{ 58828,				\
								  33508,				\
								  2056,					\
								  120,					\
								  1288,					\
								  13364,				\
								  11720,				\
								  135944,				\
								  232,					\
								  56080,				\
								  71056					}
// Most of the archive versions from the net use the Skid Row loader
#define ALT_LOADER				"SKR_COLD"
#define ALT_LOADER_SIZE			28820
#define ALT_LOADER_SIZE2        27940

// Static images
#define NB_TEXTURES				24
#define NB_IFFS					19
// Couple differences between Win & PSP
#if defined(PSP)
#define S2_NAME					"STARTSCREEN2-PSP"
#define APERTURE_VIDEO			"aperture.pmp"
#else
#define S2_NAME					"STARTSCREEN2"
#define APERTURE_VIDEO			"aperture.avi"
#endif
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
									{ S2_NAME, 320, 200, 0, NULL },								\
									{ "STARTSCREEN3", 320, 200, 0, NULL },						\
									{ "STARTSCREEN4", 320, 200, 0, NULL },						\
									{ "PIC.8(PASS)", 320, 192, 0, NULL },						\
									{ "PIC.9(PAPERS)", 320, 192, 0, NULL },						\
									{ "panel_base1.raw", 64, 32, 0, NULL},						\
									{ "panel_base2.raw", 256, 32, 0, NULL},						\
									{ "corner.raw", 74, 37, 0, NULL},							\
									{ "tunnel-vision.raw", 128, 128, 0, NULL},					\
								}

// Handy identifier for images
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
#define PICTURE_CORNER			21
#define TUNNEL_VISION			22
#define NO_PICTURE				-1

// Music mods
#define NB_MODS					3
#define MOD_NAMES				{ "LOADTUNE.MOD", "GAMEOVER.MUS", "WHENWIN.MUS" }
#define MOD_LOADTUNE			0
#define MOD_GAMEOVER			1
#define MOD_WHENWIN				2
// Most archives will use the PPacked version of LOADTUNE
#define PP_LOADTUNE_NAME		"LOADTUNE.MUS"
#define PP_LOADTUNE_SIZE		66956
// SFX data
#define NB_SFXS					5
#define SFX_TABLE_START			0x0000CA3E
#define SFX_ADDRESS_START		0x0000CA6A
#define SFX_DOOR				0
#define SFX_WTF					1
#define SFX_SAW					2
#define SFX_FOOTSTEPS			3
#define SFX_SHOOT				4


// If we place our loader at address 0x80, we won't have to convert the pointers from the disassembly ;)
#define LOADER_PADDING			0x00000080
#define MENDAT_ITEM_SIZE		0x14
#define INITIAL_PALETTE_INDEX	2


// Compressed and indoors maps data
#define CRM_OFFSETS_START		0x00002684
#define CRM_ROOMS_START			0x00002FE4
#define CMP_TILES_START			0x00005E80
#define CMP_MAP_WIDTH			0x54
#define CMP_MAP_HEIGHT			0x48


// tiles that need overlay, from LOADER
#define SPECIAL_TILES_START		0x00003F3A
#define NB_SPECIAL_TILES		0x16
#define FIREPLACE_TILE			0xE080
#define TUNNEL_TILE_ADDON		0x1E0


// Nations & guards
#define NB_NATIONS				4
#define NB_GUARDS				0x3D
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


/*
 *	PANEL related data
 */
#define PANEL_BASE1_W			64
#define PANEL_BASE2_W			256
#define PANEL_BASE_H			32
#define PANEL_OFF_X				79
#define PANEL_OFF_Y				3
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
#define NB_PANEL_SPRITES		(NB_PANEL_FLAGS+NB_PANEL_FACES+NB_PANEL_CLOCK_DIGITS+NB_PANEL_ITEMS+2)
#define NB_EXTRA_SPRITES		2
#define NB_SPRITES				(NB_STANDARD_SPRITES+NB_PANEL_SPRITES)
// The fatigue bar base is the last sprite
#define PANEL_FATIGUE_SPRITE	(NB_SPRITES-1)
#define FOOLED_BY_SPRITE		(NB_SPRITES-2)
#define PASS_SPRITE				0x9F
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


/*
 *	Menu stuff
 */
#define MIN_MENU_FADE			0.4f
#define MENU_MARKER				0x20
#define NB_MENUS				4
#define NB_MENU_ITEMS			10
#define FIRST_MENU_ITEM			3
#define MAIN_MENU				0
#define OPTIONS_MENU			1
#define SAVE_MENU				2
#define LOAD_MENU				3
#define ABOUT_MENU				4
#define NB_SAVEGAMES			6
// Items for the main menu
#define MENU_RETURN				3
#define MENU_RESTART			4
#define MENU_SAVE				5
#define MENU_LOAD				6
#define MENU_OPTIONS			7
#define MENU_EXIT				9
// Items for the options menu
#define MENU_BACK_TO_MAIN		3
#define MENU_SKIP_INTRO			4
#define MENU_PICTURE_CORNERS	5
#define MENU_SMOOTHING			6
#define MENU_FULLSCREEN			7
#define MENU_ENHANCEMENTS		8
#define MENU_ORIGINAL_MODE		9


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

// Where to send someone we don't want to see around again
#define GET_LOST_X				5000
#define GET_LOST_Y				5000

// This loader section defines the list of authorized rooms (through their
// message ID) after certain events (appel, exercise, confined)
#define AUTHORIZED_BASE			0x000020EE
#define NB_AUTHORIZED_POINTERS	7
#define AUTHORIZED_NATION_BASE	0x0000210A

#define NB_OBS_TO_SPRITE		15
#define OBS_TO_SPRITE_START		0x00005D82
#define LOADER_DATA_START		0x0000010C
// On compressed map (outside)
#define ROOM_OUTSIDE			0xFFFF
// Lower index of tunnels
#define ROOM_TUNNEL				0x0203
// Room index for picked objects
#define ROOM_NO_PROP			0x0258
#define REMOVABLES_MASKS_START	0x00008758
#define REMOVABLES_MASKS_LENGTH	27

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

// For data file patching
#define FIXED_CRM_VECTOR		0x50

// Joystick center position
#define DIRECTION_STOPPED		8



/*
 *	Time related
 */
// Stack for time delayed events. Doubt we'll need more than that
#define NB_EVENTS				32
// Time between animation frames, in ms
#define ANIMATION_INTERVAL		120
// 20 ms provides the same speed (for patrols) as on the Amiga
#define	REPOSITION_INTERVAL		15
// defines how long a transition takes on animated picture effects
#define TRANSITION_DURATION		1000
// How long should we sleep when paused
#define PAUSE_DELAY				40
// Minimum amount of sleep we are entitling ourselves to, in ms
#define QUANTUM_OF_SOLACE		2
// Muhahahahahaha!!! Fear not, mere mortals, for I'll...
#define TIME_MARKER				10000
// NB: This is the duration of a game minute, in ms
#define SOLITARY_DURATION		300000
// How long should we keep a static picture on, in ms
#define PICTURE_TIMEOUT			12000
// Time we should keep our inventory messages, in ms
#define	PROPS_MESSAGE_TIMEOUT	2000
#define CHEAT_MESSAGE_TIMEOUT	2000
#define NO_MESSAGE_TIMEOUT		0
// How long does a pass provide immunity, in ms
#define PASS_GRACE_PERIOD		3000

// Loader table containing the IFF indexes to use for various events
#define IFF_INDEX_TABLE			0x00007A80

// How long should the guard remain blocked (in nb of route steps)
// default of the game is 0x64
#define BLOCKED_GUARD_TIMEOUT	0xC0
#define WALKING_PURSUIT_TIMEOUT	100
#define RUNNING_PURSUIT_TIMEOUT	100
#define SHOOTING_GUARD_TIMEOUT	20
#define STONE_THROWN_TIMEOUT	150
#define RESET_GUARD_MAX_TIMEOUT	500
#define RESET_GUARD_MIN_TIMEOUT	100
#define PRE_PURSUIT_TIMEOUT		20
// Timed events from LOADER (roll call, palette change)
#define TIMED_EVENTS_BASE		0x00002BDE
// First timed event of the game
#define TIMED_EVENTS_INIT		0x00002C1A
// Palette change type
#define TIMED_EVENT_PALETTE		0xFFFF
// Rollcall check
#define TIMED_EVENT_ROLLCALL_CHECK	1
// For pursuit states
#define NO_TARGET				-1

/*
 *	Overlays and animation related
 */
#define MAX_OVERLAYS			0x80
#define NB_STANDARD_SPRITES		0xD1
// Our minimal z index, for overlays
// Oh, and don't try to be smart and use 0x8000, because unless you do an
// explicit cast, you will get strange things line short variables that you
// explicitely set to MIN_Z never equating MIN_Z in comparisons
// NB: gcc will issue a "comparison is always false due to limited range of data type"
#define MIN_Z					-32768

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
#define INTO_GUARDS_UNI_ANI		0x14
#define INTO_PRISONERS_UNI_ANI	0x15
#define GUARD_KNEEL_ANI			0x16

// these sprites are information messages for the panel
#define STATE_WALK_SID			0xF6
#define STATE_RUN_SID			0xF7
#define STATE_CRAWL_SID			0xF8
#define STATE_STOOGE_SID		0xF9

// For animations that are NOT guybrushes (guybrushes embed their own animation struct)
#define MAX_ANIMATIONS			0x20
#define MAX_CURRENTLY_ANIMATED	MAX_ANIMATIONS
#define NB_ANIMATED_SPRITES		23
#define NB_GUYBRUSHES			(NB_NATIONS + NB_GUARDS)

/*
 *	Game related states
 */
// guybrushes states
#define STATE_MOTION			0x0001
#define STATE_ANIMATED			0x0002
#define STATE_SLEEPING			0x0004
#define STATE_STOOGING			0x0008
#define STATE_TUNNELING			0x0010
#define STATE_IN_PRISON			0x0020
#define STATE_IN_PURSUIT		0x0040
#define STATE_BLOCKED			0x0080
#define STATE_AIMING			0x0100
#define STATE_SHOT				0x0200
#define STATE_RESUME_ROUTE_WAIT	0x0400
#define STATE_RESUME_ROUTE		0x0800
#define STATE_KNEELING			0x1000

// Useful masks
#define MOTION_DISALLOWED		(STATE_SLEEPING|STATE_SHOT|STATE_KNEELING)
#define DEVIATED_FROM_ROUTE		(STATE_IN_PURSUIT|STATE_RESUME_ROUTE|STATE_RESUME_ROUTE_WAIT)

// Game states
#define GAME_STATE_ACTION		1
#define GAME_STATE_PAUSED		2
#define GAME_STATE_STATIC_PIC	4
#define GAME_STATE_INTRO		8
#define GAME_STATE_GAME_OVER	16
#define GAME_STATE_GAME_WON		32
#define GAME_STATE_PICTURE_LOOP	64
#define GAME_STATE_CUTSCENE		128
#define GAME_STATE_MENU			256

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


/*
 *	Global structs
 */

// Structure to hold the standard RGBA sprites
typedef struct
{
    u16 w;
	u16 h;
	// Politicaly correct w & h (power of twos, to keep the PSP happy)
	u16 corrected_w;
	u16 corrected_h;
	s16 x_offset;
	s16 y_offset;
	s16 z_offset;
	u8* data;
} s_sprite;

// for nonstandtard sprites (panel, etc)
typedef struct
{
	u16 w;
	u16 base;
	u32 offset;
} s_panel_sprite;

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

// Sound FXs
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

// Guybrushes (prisoners or guards)
typedef struct
{
	u16				room;					// Room index
	s16				px;
	s16				p2y;
	s16				speed;					// Walk = 1, Run = 2
	/* For animated overlays, direction is one of:
	 *    3  2  4
	 *    0  8  1
	 *    6  5  7   */
	s16				direction;
	u16				state;					// Motion related state (see above)
	u32				ext_bitmask;			// Removable walls bitmask
	s_animation		animation;
	bool			reset_animation;
	bool			is_dressed_as_guard;
	bool			is_onscreen;
	// Guard activity variables
	bool			reinstantiate;
	bool			resume_motion;
	bool			blocked_by_prisoner;
	u32				go_on;
	u32				spent_in_room;
	u16				wait;
	s16				target;
	s16				resume_px;
	s16				resume_p2y;
	s16				resume_direction;
	bool			fooled_by[NB_NATIONS];
} s_guybrush;

// Event related states (applies to prisoners only)
typedef struct
{
	bool require_pass;
	bool require_papers;
	bool to_solitary;
	bool unauthorized;
	bool display_shot;
	bool killed;
	bool escaped;
	bool thrown_stone;
	u64	 pass_grace_period_expires;
	u32  fatigue;
	u64  solitary_release;
} s_prisoner_event;


/*
 *	Defines, passing as globals (originally global variables)
 */
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
#define paused				(game_state & GAME_STATE_PAUSED)
#define game_over			(game_state & GAME_STATE_GAME_OVER)
#define intro				(game_state & GAME_STATE_INTRO)
#define game_won			(game_state & GAME_STATE_GAME_WON)
#define menu				(game_state & GAME_STATE_MENU)


/*
 *	Actual global variables
 */

// General program options, including cheats
extern bool		opt_verbose;
extern bool		opt_debug;
extern bool		opt_onscreen_debug;
extern int		opt_sid;
extern bool		opt_play_as_the_safe[NB_NATIONS];
extern bool		opt_keymaster;
extern bool		opt_thrillerdance;
extern bool		opt_no_guards;
extern bool		opt_meh;
extern bool		opt_haunted_castle;
extern bool		opt_glsl_enabled;

// Global variables
extern bool		init_animations;
extern bool		is_fire_pressed;
extern bool		can_consume_key;
extern u8		*mbuffer;	// Generic TMP buffer
extern u8		*fbuffer[NB_FILES];
extern u8		*rbuffer;
extern FILE		*fd;		// Generic file descriptor
extern u8		*rgbCells;	// Cells table
extern u8		*static_image_buffer;
extern u8		props[NB_NATIONS][NB_PROPS];
extern u8		selected_prop[NB_NATIONS];
extern s_prisoner_event p_event[NB_NATIONS];
extern u8		nb_room_props;
extern u16		room_props[NB_OBSBIN];
extern u8		over_prop, over_prop_id;
extern char		*status_message;
extern int		status_message_priority;
extern const s16 directions[3][3], dir_to_dx[9], dir_to_d2y[9], invert_dir[9];
extern u8		hours_digit_h, hours_digit_l, minutes_digit_h, minutes_digit_l;
extern u8		palette_index;	// Current palette
extern u16		game_state;
extern float	fade_value;
extern int		current_picture;
extern u16		nb_objects;
extern char		*fname[NB_FILES];
extern u32		fsize[NB_FILES];
extern char		*mod_name[NB_MODS];
extern int		gl_width, gl_height;
extern u8		current_nation;
extern char		nb_props_message[32];
extern u64		game_time, last_atime, last_ptime, last_ctime, t_last;
extern s_event	events[NB_EVENTS];

/*
 *	Prototypes
 */
void static_screen(u8 iff_id, void (*func)(u32), u32 param);


#ifdef	__cplusplus
}
#endif

