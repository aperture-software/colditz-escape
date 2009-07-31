/*
 *  Colditz Escape! - Rewritten Engine for "Escape From Colditz"
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

// textures for static images, panel base, etc.
typedef struct
{
	char* filename;
	u16 w;
	u16 h;
	unsigned int texid;
	u8* buffer;
} s_tex;
extern s_tex texture[NB_TEXTURES];

// IFF tags & compression methods definitions
#define MAKE_ID(a,b,c,d) ((u32) ((a)<<24 | (b)<<16 | (c)<<8 | (d)))
#define IFF_ILBM	MAKE_ID('I','L','B','M')
#define IFF_FORM	MAKE_ID('F','O','R','M')
#define IFF_BMHD	MAKE_ID('B','M','H','D')
#define IFF_BODY	MAKE_ID('B','O','D','Y')
#define IFF_CMAP	MAKE_ID('C','M','A','P')
#define IFF_CMP_NODE			0
#define IFF_CMP_BYTERUN1		1

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


