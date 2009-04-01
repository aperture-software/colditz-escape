#ifndef _COLDITZ_UTILITIES_H
#define _COLDITZ_UTILITIES 1

#ifdef	__cplusplus
extern "C" {
#endif


/* These macro will come handy later on */

// Reads the tile index at (x,y) from compressed map and rotate
#define comp_readtile(x,y)			\
	((u32)(readlong((u8*)fbuffer[COMPRESSED_MAP], ((y)*room_x+(x))*4) & 0x1FF00) >> 8)
#define room_readtile(x,y)			\
	((u32)(readword((u8*)(fbuffer[ROOMS]+ROOMS_START+offset),((y)*room_x+(x))*2) & 0xFF80) >> 7)
// Converts a tile index to a longword offset
#define readtile(x,y)				\
	((current_room_index == ROOM_OUTSIDE)?comp_readtile(x,y):room_readtile(x,y))

#define comp_readexit(x,y)			\
	((u32)(readlong((u8*)fbuffer[COMPRESSED_MAP], ((y)*room_x+(x))*4) & 0x1F))
#define room_readexit(x,y)			\
	((u32)(readword((u8*)(fbuffer[ROOMS]+ROOMS_START+offset),((y)*room_x+(x))*2) & 0x1F))
// Converts a tile index to a longword offset
#define readexit(x,y)				\
	((current_room_index == ROOM_OUTSIDE)?comp_readexit(x,y):room_readexit(x,y))


//void cells_to_interleaved(u8* buffer, u32 size);
//void sprites_to_interleaved(u8* buffer, u32 bitplane_size);
void to_16bit_palette(u8 palette_index);
//void to_24bit_Palette(u8 palette_index);
//void to_48bit_Palette(u16 wPalette[3][16], u8 palette_index);
//void cells_to_RGB(u8* source, u8* dest, u32 size);
void cells_to_wGRAB(u8* source, u8* dest);
void load_all_files();
void display_room();
void display_panel();
void rescale_buffer();
void get_properties();
#if !defined(PSP)
void glutPrintf(const char *fmt, ...);
#endif
void init_sprites();
void sprites_to_wGRAB();
int check_footprint(int dx, int d2y);
void switch_room(int exit, int dx, int dy);
void fix_files();


#ifdef	__cplusplus
}
#endif

#endif /* _COLDITZ_UTILITIES_H */
