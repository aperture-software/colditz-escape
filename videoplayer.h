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
 *  videoplayer.h: wrapper for PSP (PMP) & Windows (DirectShow) video playout
 *  ---------------------------------------------------------------------------
 */
#pragma once

#ifdef __cplusplus
extern "C" {
#endif
#include <stdbool.h>

// Call once on startup to initialize video playback
bool video_init();

// Playback file
bool video_play(char* filename);

// Stop the playback and free all resources
void video_stop();

// Check if playback is still running
bool video_isplaying();

#ifdef __cplusplus
}
#endif
