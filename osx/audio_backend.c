/*
 *  Colditz Escape - Rewritten Engine for "Escape From Colditz"
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
 *  audio_backend.c: OSX template audio backend
 *  ---------------------------------------------------------------------------
 */

#include "low-level.h"
#include "audio_backend.h"

bool audio_backend_init(void)
{
    return false;
}

bool audio_backend_set_voice(int voice, void* data, int size, unsigned int frequency,
                             unsigned int bits_per_sample, bool stereo)
{
    return false;
}
bool audio_backend_set_voice_callback(int voice, audio_backend_voice_callback_t callback, void* pdata,
                                      unsigned int frequency, unsigned int bits_per_sample, bool stereo)
{
    return false;
}

bool audio_backend_release(void)
{
    return false;
}

bool audio_backend_start_voice(int voice)
{
    return false;
}

bool audio_backend_stop_voice(int voice)
{
    return false;
}

bool audio_backend_set_voice_volume(int voice, float volume)
{
    return false;
}

bool audio_backend_release_voice(int voice)
{
    return false;
}
