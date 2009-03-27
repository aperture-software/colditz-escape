#ifndef _COLDITZ_UTILITIES_H
#define _COLDITZ_UTILITIES 1

#ifdef	__cplusplus
extern "C" {
#endif


//void cells_to_interleaved(u8* buffer, u32 size);
//void sprites_to_interleaved(u8* buffer, u32 bitplane_size);
void to_16bit_palette(u8 palette_index);
//void to_24bit_Palette(u8 palette_index);
//void to_48bit_Palette(u16 wPalette[3][16], u8 palette_index);
//void cells_to_RGB(u8* source, u8* dest, u32 size);
void cells_to_wGRAB(u8* source, u8* dest);
void load_all_files();
void display_room(u16 room_index);
void display_panel();
void rescale_buffer();
void get_properties();
#if !defined(PSP)
void glutPrintf(const char *fmt, ...);
#endif
void init_sprites();
void sprites_to_wGRAB();
int check_footprint(u16 room_index, int px, int p2y);


#ifdef	__cplusplus
}
#endif

#endif /* _COLDITZ_UTILITIES_H */
