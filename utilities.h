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
	((current_room_index == ROOM_OUTSIDE)?comp_readtile(x,y):room_readtile(x,y))

// Reads the exit index, which we need to get to the target room index
#define comp_readexit(x,y)			\
	((u32)(readlong((u8*)fbuffer[COMPRESSED_MAP], ((y)*room_x+(x))*4) & 0x1F))
#define room_readexit(x,y)			\
	((u32)(readword((u8*)(fbuffer[ROOMS]+ROOMS_START+offset),((y)*room_x+(x))*2) & 0x1F))
#define readexit(x,y)				\
	((current_room_index == ROOM_OUTSIDE)?comp_readexit(x,y):room_readexit(x,y))

// Returns the offset of the byte that describes the exit status (open/closed, key level...) 
#define room_get_exit_offset(x,y)	\
	((u32)ROOMS_START + offset + ((y)*room_x+(x))*2 + 1)
#define comp_get_exit_offset(x,y)	\
	(comp_readexit(x,y) << 3)
#define get_exit_offset(x,y)		\
	((current_room_index == ROOM_OUTSIDE)?comp_get_exit_offset(x,y):room_get_exit_offset(x,y))

// Toggle the exit open flag
#define toggle_open_flag(x_flags)	\
	x_flags = (x_flags & 0x10)?(x_flags & 0xEF):(x_flags | 0x10)

// Checks that an overlays is visible onscreen (with generous margins)
#define ignore_offscreen_x(ovl)		\
	if ((overlay[ovl].x < -64) || (overlay[ovl].x > (PSP_SCR_WIDTH+64)))	\
			continue
#define ignore_offscreen_y(ovl)		\
	if ((overlay[ovl].y < -64) || (overlay[ovl].y > (PSP_SCR_HEIGHT+64)))	\
			continue

#define get_guybrush_sid(x)					\
	((guybrush[x].state == STATE_STOP)?get_stop_animation_sid(guybrush[x].ani_index):get_animation_sid(guybrush[x].ani_index))

#define request_status_message(msg)	\
	if ((!keep_message_on) && (!status_message)) status_message = (char*)(msg)
#define force_status_message(msg)		\
	keep_message_on = false; status_message = (char*)(msg)



//void cells_to_interleaved(u8* buffer, u32 size);
//void sprites_to_interleaved(u8* buffer, u32 bitplane_size);
void to_16bit_palette(u8 palette_index, u8 transparent_index, u8 io_file);
//void to_16bit_palette(int palette_start, u8 io_file);
//void to_24bit_Palette(u8 palette_index);
//void to_48bit_Palette(u16 wPalette[3][16], u8 palette_index);
//void cells_to_RGB(u8* source, u8* dest, u32 size);
void cells_to_wGRAB(u8* source, u8* dest);
void load_all_files();
void display_room();
void display_panel();
void rescale_buffer();
void get_properties();
void move_guards();
#if !defined(PSP)
void glutPrintf(const char *fmt, ...);
#endif
void init_sprites();
void sprites_to_wGRAB();
int check_footprint(int dx, int d2y);
void switch_room(int exit, int dx, int dy);
void fix_files();
void set_room_props();




#ifdef	__cplusplus
}
#endif

#endif /* _COLDITZ_UTILITIES_H */
