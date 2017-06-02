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
 *  soundplayer.h: header for Colditz MOD and SFX audio engine
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

#include <stdbool.h>

// The NoteData structure stores the information for a single note and/or effect.
typedef struct {
    int sample_num;		// The sample number (ie "instrument") to play, 1->31
    int period_index;	// This effectively stores the frequency to play the
    // sample, although it's actually an index into
    // PeriodTable.
    int effect;		// Contains the effect code
    int effect_parms;	// Used to store control parameters for the
    // various effects
} NoteData;

// RowData stores all the information for a single row. If there are 8 tracks
// in this mod then this structure will be filled with 8 elements of NoteData,
// one for each track.
typedef struct {
    int numnotes;
    NoteData *note;
} RowData;

// Pattern contains all the information for a single pattern.
// It is filled with 64 elements of type RowData.
typedef struct {
    int numrows;
    RowData *row;
} Pattern;

// The Sample structure is used to store all the information for a single
// sample, or "instrument". Most of the member's should be self-explanatory.
// The "data" member is an array of bytes containing the raw sample data
// in 8-bit signed format.
typedef struct {
    char szName[100];
    int nLength, nFineTune, nVolume, nLoopStart, nLoopLength, nLoopEnd;
    int data_length;
    char *data;
} Sample;


// TrackData is used to store ongoing information about a particular track.
typedef struct {
    int sample;		// The current sample being played (0 for none)
    int pos;		// The current playback position in the sample,
    // stored in fixed-point format
    int period_index;	// Which note to play, stored as an index into the
    // PeriodTable array
    int period;		// The period number that period_index corresponds to
    // (needed for various effects)
    float freq;		// This is the actual frequency used to do the mixing.
    // It's a combination of the value calculated from the
    // period member and an "adjustment" made by various effects.
    int volume;		// Volume that this track is to be mixed at
    int mixvol;		// This is the actual volume used to do the mixing.
    // It's a combination of the volume member and an
    // "adjustment" made by various effects.
    int porta;		// Used by the porta effect, this stores the note we're
    // porta-ing (?) to
    int portasp;		// The speed at which to porta
    int vibspe;		// Vibrato speed
    int vibdep;		// Vibrato depth
    int tremspe;		// Tremolo speed
    int tremdep;		// Tremolo depth
    int panval;		// Pan value....this player doesn't actually do panning,
    // so this member is ignored (but included for future use)
    int sinepos;		// These next two values are pointers to the sine table.
    // They're used to do
    int sineneg;		// various effects.
} TrackData;

// Function prototypes
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
