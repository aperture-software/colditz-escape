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
 *  audio_backend.c: PSP audio backend
 *  ---------------------------------------------------------------------------
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <pspkernel.h>
#include <pspaudio.h>
#include <pspaudiolib.h>
#include "psp/printf.h"
#include "audio_backend.h"

#define PSP_PLAYBACK_FREQ 44100

bool audio_backend_init(void)
{
    return (pspAudioInit() >= 0);
}

bool audio_backend_release(void)
{
    pspAudioEnd();
    return true;
}

bool audio_backend_set_voice(int voice, void* data, int size, unsigned int frequency,
                             unsigned int bits_per_sample, bool stereo)
{
    if (frequency != PSP_PLAYBACK_FREQ)
    {
        fprintf(stderr, "play_sfx: frequency must be %d for PSP SFX\n", PSP_PLAYBACK_FREQ);
        return false;
    }
    if (bits_per_sample != 16)
    {
        fprintf(stderr, "play_sfx: samples must be 16 bit for PSP SFX\n");
        return false;
    }
    if (size >= PSP_AUDIO_SAMPLE_MAX)
    {
        fprintf(stderr, "play_sfx: Cannot play samples of more than %d bytes\n", PSP_AUDIO_SAMPLE_MAX);
        return false;
    }
    if (sceAudioChReserve(voice, size, PSP_AUDIO_FORMAT_MONO) < 0)
        return false;
    if (sceAudioOutput(voice, PSP_AUDIO_VOLUME_MAX, data) < 0)
        return false;
    return true;
}

bool audio_backend_set_voice_callback(int voice, AudioVoiceCallback_t callback, void* pdata,
                                      unsigned int frequency, unsigned int bits_per_sample, bool stereo)
{
    if (frequency != PSP_PLAYBACK_FREQ)
    {
        fprintf(stderr, "play_sfx: frequency must be %d for PSP SFX\n", PSP_PLAYBACK_FREQ);
        return false;
    }
    if (bits_per_sample != 16)
    {
        fprintf(stderr, "play_sfx: samples must be 16 bit for PSP SFX\n");
        return false;
    }
    pspAudioSetChannelCallback(voice, callback, NULL);
    return true;
}

bool audio_backend_start_voice(int voice)
{
    // Nothing to do on this platform
    return true;
}

bool audio_backend_stop_voice(int voice)
{
    pspAudioSetChannelCallback(voice, NULL, NULL);
    return true;
}

bool audio_backend_set_voice_volume(int voice, float volume)
{
    return (sceAudioChangeChannelVolume(voice, 
            volume*PSP_VOLUME_MAX, volume*PSP_VOLUME_MAX) >= 0);
}

bool audio_backend_release_voice(int voice)
{
    return (sceAudioChRelease(voice) >= 0);
}
