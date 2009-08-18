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
 *  soundplayer.h: headers for Colditz MOD and SFX audio engine
 *  ---------------------------------------------------------------------------
 */
#pragma once

// This #define is used to convert an Amiga period number to a frequency. 
// The frequency returned is the frequency that the sample should be played at.
//  3579545.25f / 428 = 8363.423 Hz for Middle C (PAL)
#define Period2Freq(period) (3579545.25f / (period))

// These next few lines determine how the sound will be mixed. 
// Set PLAYBACK_FREQ to whatever you want. 
#define PLAYBACK_FREQ 44100

// OVERSAMPLE can be commented out to disable that function 
// (takes up less CPU time, but doesnt sound as good). 
#define OVERSAMPLE

#ifdef __cplusplus
extern "C" {
#endif


    bool audio_init();
    bool audio_release();
    bool mod_init(char *filename);
    void mod_release();
    bool mod_play();
    bool is_mod_playing();
    void mod_pause();
    bool mod_stop();
    bool play_sample(int channel, unsigned int volume, void *address, unsigned int length, 
        unsigned int frequency, unsigned int bits_per_sample, bool loop);
    bool play_loop(unsigned int volume, void *address, unsigned int length, 
        unsigned int frequency, unsigned int bits_per_sample);
    void stop_loop();
#if defined(PSP)
    bool psp_upsample(short **dst_address, unsigned long *dst_length, char *src_sample, 
                      unsigned long src_numsamples, unsigned short src_frequency);
#endif
#ifdef __cplusplus
}
#endif

