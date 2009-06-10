#ifndef _COLDITZ_UTILITIES_H
#define _COLDITZ_UTILITIES_H

#ifdef	__cplusplus
extern "C" {
#endif


/* These macro will come handy later on */

// Reads the tile index at (x,y) from compressed map and rotate
#define comp_readtile(x,y)			\
	((u32)(readlong((u8*)fbuffer[COMPRESSED_MAP], ((y)*room_x+(x))*4) & 0x1FF00) >> 8)
#define room_readtile(x,y)			\
	((u32)(readword((u8*)(fbuffer[ROOMS]+ROOMS_START+offset),((y)*room_x+(x))*2) & 0xFF80) >> 7)
#define readtile(x,y)				\
	(is_outside?comp_readtile(x,y):room_readtile(x,y))

// Reads the exit index, which we need to get to the target room index
#define comp_readexit(x,y)			\
	((u32)(readlong((u8*)fbuffer[COMPRESSED_MAP], ((y)*room_x+(x))*4) & 0x1F))
#define room_readexit(x,y)			\
	((u32)(readword((u8*)(fbuffer[ROOMS]+ROOMS_START+offset),((y)*room_x+(x))*2) & 0x1F))
#define readexit(x,y)				\
	(is_outside?comp_readexit(x,y):room_readexit(x,y))

// Returns the offset of the byte that describes the exit status (open/closed, key level...) 
#define room_get_exit_offset(x,y)	\
	((u32)ROOMS_START + offset + ((y)*room_x+(x))*2 + 1)
#define comp_get_exit_offset(x,y)	\
	(comp_readexit(x,y) << 3)
#define get_exit_offset(x,y)		\
	(is_outside?comp_get_exit_offset(x,y):room_get_exit_offset(x,y))

// Toggle the exit open flag
#define toggle_open_flag(x_flags)  x_flags ^= 0x10

// Checks that an overlay is visible onscreen (with generous margins)
#define ignore_offscreen_x(ovl)		\
	if ((overlay[ovl].x < -64) || (overlay[ovl].x > (PSP_SCR_WIDTH+64)))	\
			continue
#define ignore_offscreen_y(ovl)		\
	if ((overlay[ovl].y < -64) || (overlay[ovl].y > (PSP_SCR_HEIGHT+64)))	\
			continue

#define get_guybrush_sid(x)					\
	( ( (guybrush[x].state & STATE_ANIMATED) && (!(guybrush[x].state & STATE_BLOCKED)) ) ?	\
	get_animation_sid(x, true):get_stop_animation_sid(x, true))

#define safe_nb_animations_increment() {	\
	if (nb_animations <= (MAX_ANIMATIONS-1))\
		nb_animations++;					\
	else									\
		printf("Too many animations!\n");	}

#define safe_overlay_index_increment() {	\
	if (overlay_index <= (MAX_OVERLAYS-1))	\
		overlay_index++;					\
	else									\
	printf("Too many overlays!\n");			}	


// For load_iff()
#define IFF_FORM        0x464F524D    // 'FORM' - IFF FORM structure  
#define IFF_ILBM        0x494C424D    // 'ILBM' - interleaved bitmap
#define IFF_BMHD        0x424D4844    // 'BMHD' - bitmap header
#define IFF_CMAP        0x434D4150    // 'CMAP' - color map (palette)
#define IFF_BODY        0x424F4459    // 'BODY' - bitmap data

static __inline u32 freadl(FILE* f)
{
	u8	b,i;
	u32 r = 0;
	for (i=0; i<4; i++)
	{
		fread(&b,1,1,f);
		r <<= 8;
		r |= b;
	}
	return r;
}
static __inline u16 freadw(FILE* f)
{
	u8	b,i;
	u16 r = 0;
	for (i=0; i<2; i++)
	{
		fread(&b,1,1,f);
		r <<= 8;
		r |= b;
	}
	return r;
}
static __inline u8 freadc(FILE* f)
{
	u8	b = 0;
	fread(&b,1,1,f);
	return b;
}


// Public prototypes
//
//////////////////////////////////////////////////////////////////////
void to_16bit_palette(u8 palette_index, u8 transparent_index, u8 io_file);
void cells_to_wGRAB(u8* source, u8* dest);
void load_all_files();
void reload_files();
void display_room();
void display_picture();
void display_panel();
void rescale_buffer();
void set_global_properties();
int move_guards();
void init_sprites();
void sprites_to_wGRAB();
void toggle_exit(u32 exit_nr);
s16 check_footprint(s16 dx, s16 d2y);
bool check_guard_footprint(u8 g, s16 dx, s16 d2y);
void switch_room(s16 exit, bool tunnel_io);
void fix_files(bool reload);
void set_room_props();
void timed_events(u16 hours, u16 minutes_high, u16 minutes_low);
int  load_iff(u8 iff_id);
bool load_raw_rgb(int w, int h, char* filename);
void check_on_prisoners();
void newgame_init(bool reload);
void play_sfx(int sfx_id);
void depack_loadtune();

#ifdef	__cplusplus
}
#endif

#endif /* _COLDITZ_UTILITIES_H */
