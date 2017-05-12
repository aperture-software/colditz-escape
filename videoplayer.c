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
 *  videoplayer.c: wrapper for PSP (PMP) & Windows (DirectShow) video playout
 *  ---------------------------------------------------------------------------
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "data-types.h"
#include "videoplayer.h"

#if defined(WIN32)
#include "colditz.h"
#include "win32/wmp.h"

bool video_init()
{
    return wmp_init(APPNAME);
}

bool video_play(char* filename)
{
    return wmp_play(filename);
}

void video_stop()
{
    wmp_stop();
}

bool video_isplaying()
{
    return wmp_isplaying();
}
#elif defined(PSP)
#include "psp/pmp.h"

bool video_init()
{
    return (pmp_init() == NULL);
}

bool video_play(char* filename)
{
    return (pmp_play(filename, 1, -1) == NULL);
}

void video_stop()
{
    pmp_stop();
}

// Check if playback is still running
bool video_isplaying()
{
    return pmp_isplaying();
}
#endif
