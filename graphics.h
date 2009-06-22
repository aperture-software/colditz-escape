#ifndef _GRAPHICS_H
#define _GRAPHICS_H

#ifdef	__cplusplus
extern "C" {
#endif

// textures for static images, panel base, etc.
typedef struct
{
	char* filename;
	u16 w;
	u16 h;
	unsigned int texid;
	u8* buffer;
} s_tex;

// load_iff() stuff
//
//////////////////////////////////////////////////////////////////////
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

extern s_tex texture[NB_TEXTURES];


// Public prototypes
//
//////////////////////////////////////////////////////////////////////
void to_16bit_palette(u8 palette_index, u8 transparent_index, u8 io_file);
void cells_to_wGRAB(u8* source, u8* dest);
void display_sprite_linear(float x1, float y1, float w, float h, unsigned int texid) ;
void display_room();
void display_picture();
void display_panel();
void rescale_buffer();
void create_pause_screen();
void display_pause_screen();
void set_textures();
void init_sprites();
void sprites_to_wGRAB();
bool load_texture(s_tex *tex);

#ifdef	__cplusplus
}
#endif

#endif 
