/*
 *  Colditz Escape - Rewritten Engine for "Escape From Colditz"
 *  copyright (C) 2008-2009 Aperture Software
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
 *  graphics.h: Game runtime display definitions
 *  ---------------------------------------------------------------------------
 */


#pragma once

#ifdef	__cplusplus
extern "C" {
#endif

// IFF tags & compression methods definitions
#define MAKE_ID(a,b,c,d) ((uint32_t) ((a)<<24 | (b)<<16 | (c)<<8 | (d)))
#define IFF_ILBM	MAKE_ID('I','L','B','M')
#define IFF_FORM	MAKE_ID('F','O','R','M')
#define IFF_BMHD	MAKE_ID('B','M','H','D')
#define IFF_BODY	MAKE_ID('B','O','D','Y')
#define IFF_CMAP	MAKE_ID('C','M','A','P')
#define IFF_CMP_NODE			0
#define IFF_CMP_BYTERUN1		1

// textures for static images, panel base, etc.
typedef struct
{
	char* filename;
	uint16_t w;
	uint16_t h;
	unsigned int texid;
	uint8_t* buffer;
} s_tex;


/*
 *	Graphics globals we export
 */
extern s_tex		texture[NB_TEXTURES];
extern s_sprite		*sprite;
extern s_overlay	*overlay;
extern uint8_t		overlay_index;
extern int16_t		gl_off_x, gl_off_y;
extern int16_t		last_p_x, last_p_y;
extern int			selected_menu_item, selected_menu;
extern char*		menus[NB_MENUS][NB_MENU_ITEMS];
extern bool			enabled_menus[NB_MENUS][NB_MENU_ITEMS];

/*
 *	Public prototypes
 */
void free_gfx();
void to_16bit_palette(uint8_t palette_index, uint8_t transparent_index, uint8_t io_file);
void cells_to_wGRAB(uint8_t* source, uint8_t* dest);
void display_sprite_linear(float x1, float y1, float w, float h, unsigned int texid);
void display_room();
void display_picture();
void display_panel();
void rescale_buffer();
void create_savegame_list();
void display_menu_screen();
void create_pause_screen();
void display_pause_screen();
void set_textures();
void init_sprites();
void sprites_to_wGRAB();
bool load_texture(s_tex *tex);
void display_tunnel_area();
void display_fps(uint64_t frames_duration, uint64_t nb_frames);
bool init_shaders();

#ifdef	__cplusplus
}
#endif
