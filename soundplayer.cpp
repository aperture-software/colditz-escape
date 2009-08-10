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
 *  soundplayer.cpp: SFX and MOD Player Implementation in C++ for PSP and Win
 *  Copied almost 100% from PSP SDK's "PSP ModPlayer v1.0" by adresd:
 *     http://svn.pspdev.org/listing.php?repname=pspware&path=%2Ftrunk%2FPSPMediaCenter%2Fcodec%2Fmod%2F&rev=0&sc=0
 *  ---------------------------------------------------------------------------
 */

#if defined(WIN32)
#define _WIN32_DCOM
#define _CRT_SECURE_NO_WARNINGS 1 
#pragma warning(disable:4995)
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#if defined(PSP)
#include <pspkernel.h>
#include <pspaudio.h>
#include <pspaudiolib.h>
#include "psp/psp-printf.h"
#endif

#if defined(WIN32)
#include <windows.h>
// If you don't include initguid.h before xaudio2.h, you'll get
// error LNK2001: unresolved external symbol _CLSID_XAudio2
#include <initguid.h>
#include <xaudio2.h>
#include <strsafe.h>
#include <shellapi.h>
#include <mmsystem.h>
#include <conio.h>
#include "win32/winXAudio2.h"
#endif

#include "data-types.h"
#include "low-level.h"
#include "soundplayer.h"
#include "modplayeri.h"
#include "modtables.h"

// As a sample is being mixed into the buffer its position pointer is updated.
// The position pointer is a 32-bit integer that is used to store a fixed-point
// number. The following constant specifies how many of the bits should be used
// for the fractional part.
#define FRAC_BITS 10

// MAximum number of channels we'll handle
#define NB_CHANNELS	4

#define AMIGA_VOLUME_MAX 64


//////////////////////////////////////////////////////////////////////
// Function Prototypes
//////////////////////////////////////////////////////////////////////
static void SetMasterVolume(int volume);
static int  ReadModWord(unsigned char *data, int index);
static void DoTremalo(int track);
static void DoVibrato(int track);
static void DoPorta(int track);
static void SlideVolume(int track, int amount);
static int  VolumeTable[65][256];
static void UpdateEffects();
static void UpdateRow();
static bool MixChunk(int numsamples, short *buffer);
static bool MixSubChunk(short *buffer, int numsamples);
static void ModPlayCallback(void *_buf2, unsigned int length, void *pdata);


//////////////////////////////////////////////////////////////////////
// Global local variables
//////////////////////////////////////////////////////////////////////

// The following variables contain the music data, ie they don't change value until you load a new file
static char m_szName[200];
static int m_nSongLength;

static int m_nOrders_num;
static int *m_nOrders;
static int m_Patterns_num;
static Pattern *m_Patterns;
static int m_Samples_num;
static Sample *m_Samples;

// The following variables are maintained and updated by the tracker during playback
static int m_nSpeed;		// Speed of mod being played
static int m_nOrder;		// Current order being played
static int m_nRow;		// Current row being played
static int m_nTick;		// Current tick number (there are "m_nSpeed"
// ticks between each row)
static int BPM_RATE;		// Adjusted master BPM for refresh rate
static int m_nBPM;		// Beats-per-minute...controls length of 
// each tick
static int m_nSamplesLeft;	// Number of samples left to mix for the 
// current tick
static int m_nNumTracks;	// Number of tracks in this mod

static int m_TrackDat_num;
static TrackData *m_TrackDat;	// Stores info for each track being played
static RowData *m_CurrentRow;	// Pointer to the current row being played
static bool m_bPlaying = false;	// Set to true when a mod is being played
static u8 *data;
int size = 0;



//////////////////////////////////////////////////////////////////////
// These are the public functions
//////////////////////////////////////////////////////////////////////
// This could be used to set a different channel/voice for mod playout
static int mod_channel = 0;
static int sfx_channel = 0;

// Play a MONO sound sample. channel = -1 => autoallocated 
bool play_sample(int channel, unsigned int volume, void *address, unsigned int length, unsigned int frequency, unsigned int bits_per_sample)
{
	int play_channel;
	if (channel == -1)
	{
		play_channel = sfx_channel;
		sfx_channel = (sfx_channel+1)%NB_CHANNELS;
	}
	else
		play_channel = channel;

#if defined(PSP)
	if (frequency != PLAYBACK_FREQ)
	{
		fprintf(stderr, "play_sfx: frequency must be %d for PSP SFX\n", PLAYBACK_FREQ);
		return false;
	}
	if (bits_per_sample != 16)
	{
		fprintf(stderr, "play_sfx: samples must be 16 bit for PSP SFX\n");
		return false;
	}
	// Release channel if needed
	sceAudioChRelease(play_channel);
	sceAudioChReserve(play_channel, length, PSP_AUDIO_FORMAT_MONO);
	sceAudioOutput(play_channel, PSP_AUDIO_VOLUME_MAX, address);
	sceAudioChangeChannelVolume(play_channel, volume * PSP_VOLUME_MAX/AMIGA_VOLUME_MAX, volume * PSP_VOLUME_MAX/AMIGA_VOLUME_MAX);

#endif
#if defined(WIN32)
	winXAudio2ReleaseVoice(play_channel);
	// Our SFXs are always mono
	winXAudio2SetVoice(play_channel, (BYTE*) address, length, frequency, bits_per_sample, false);
	winXAudio2SetVoiceVolume(play_channel, (float) volume/AMIGA_VOLUME_MAX);
	winXAudio2StartVoice(play_channel);
#endif
	return true;
}


#if defined(PSP)
// This upconverts an 8 bit PCM file @ any frequency to 16 bit/PLAYBACK_FREQ  
void psp_upsample(short **upconverted_address, unsigned long *upconverted_length, char *src_sample, 
				  unsigned long src_numsamples, unsigned short src_frequency)
{
	int src_fracpos = 0;
	int dst_pos = 0;
	int delta_pos = (int) (src_frequency * (1 << FRAC_BITS) / PLAYBACK_FREQ);
	int src_fraclength = src_numsamples << FRAC_BITS;
	int dst_length = (src_fraclength - 1) / delta_pos + 1;
	int dst_length_aligned = PSP_AUDIO_SAMPLE_ALIGN(dst_length);
	short* dst_sample = (short*) aligned_malloc(2*dst_length_aligned, 64);

	for (dst_pos = 0; dst_pos < dst_length; dst_pos++) 
	{
		// Mix this sample in and update our position
	#ifdef OVERSAMPLE
		int sample1 = src_sample[src_fracpos >> FRAC_BITS] << 8;
		int sample2 = src_sample[(src_fracpos >> FRAC_BITS) + 1] << 8;
		int frac1 = src_fracpos & ((1 << FRAC_BITS) - 1);
		int frac2 = (1 << FRAC_BITS) - frac1;
		dst_sample[dst_pos] = ((sample1 * frac2) + (sample2 * frac1)) >> FRAC_BITS;
	#else
		dst_sample[dst_pos] = (src_sample[src_fracpos >> FRAC_BITS]) << 8;
	#endif
		src_fracpos += delta_pos;
	}

	*upconverted_address = dst_sample;
	*upconverted_length = dst_length_aligned;
}
#endif


//fillOutputBuffer(void* buffer, unsigned int samplesToWrite, void* userData)
static void ModPlayCallback(void *_buf2, unsigned int length, void *pdata)
{
	short *_buf = (short *)_buf2;
	if (m_bPlaying == true) {	//  Playing , so mix up a buffer
		MixChunk(length, _buf);
	} else {			//  Not Playing , so clear buffer
		{
			unsigned int count;
			for (count = 0; count < length * 2; count++)
				*(_buf + count) = 0;
		}
	}
}


// Initialize the audio engine
bool audio_init()
{
 if (!audio_release())
	 fprintf(stderr, "audio_release error\n");
#if defined(PSP)
	if (pspAudioInit())
		return false;
#endif
#if defined(WIN32)
	if (!winXAudio2Init())
		return false;
#endif
	return true;
}


// Release the audio engine
bool audio_release()
{
#if defined(PSP)
	pspAudioEnd();
#endif
#if defined(WIN32)
	if (!winXAudio2Release())
		return false;
#endif
	return true;
}


// Terminate MOD playout
void mod_release()
{
    int i;
    int row;

    mod_stop();
#if defined(PSP)
    pspAudioSetChannelCallback(mod_channel, 0, 0);
#endif
#if defined(WIN32)
	winXAudio2ReleaseVoice(mod_channel);
#endif

	// Tear down all the mallocs done
    //free the file itself
    if (data)
        free(data);
    // Free patterns
    for (i = 0; i < m_Patterns_num; i++) {
        for (row = 0; row < 64; row++)
            free(m_Patterns[i].row[row].note);
        free(m_Patterns[i].row);
    }
    // Free orders
    free(m_nOrders);

    // Free samples
    for (i = 1; i < m_Samples_num; i++)
        free(m_Samples[i].data);
    free(m_Samples);

    // Free tracks
    free(m_TrackDat);
}

bool is_mod_playing()
{
	return m_bPlaying;
}

//  This is the initialiser and module loader
//  This is a general call, which loads the module from the 
//  given address into the modplayer
//
//  It basically loads into an internal format, so once this function
//  has returned the buffer at 'data' will not be needed again.
bool mod_init(char *filename)
{
    int i, numpatterns, row, note;
    int index = 0;
    int numsamples;
    char modname[21];
    FILE* fd;

    m_bPlaying = false;

	if ((fd = fopen (filename, "rb")) != NULL) {
        //  opened file, so get size now
        fseek(fd, 0, SEEK_END);
		size = ftell(fd);
		fseek(fd, 0, SEEK_SET);
        data = (unsigned char *) malloc(size + 8);
        memset(data, 0, size + 8);
        if (data != 0) {	// Read file in
			fread(data, 1, size, fd);
        } else {
			fclose(fd);
            return false;
        }
        // Close file
		fclose(fd);
    } else 	// we couldn't open the file
        return false;

#if defined(PSP)
    pspAudioSetChannelCallback(mod_channel, ModPlayCallback, NULL);
#endif
#if defined(WIN32)
	// Release the voice first
	winXAudio2ReleaseVoice(0);
	if (!winXAudio2SetVoiceCallback(mod_channel, ModPlayCallback, NULL, PLAYBACK_FREQ, 16, true))
		return false;
#endif

    BPM_RATE = 130;
    //  Set default settings
    numsamples = 32;
    m_nNumTracks = 4;
    //  Check for diff types of mod
    if ((data[1080] == 'M') && (data[1081] == '.') && (data[1082] == 'K') && (data[1083] == '.'));
    else if ((data[1080] == 'F') && (data[1081] == 'L') && (data[1082] == 'T') && (data[1083] == '4'));
    else if ((data[1080] == 'F') && (data[1081] == 'L') && (data[1082] == 'T') && (data[1083] == '8'))
        m_nNumTracks = 8;
    else if ((data[1080] == '6') && (data[1081] == 'C') && (data[1082] == 'H') && (data[1083] == 'N'))
        m_nNumTracks = 6;
    else if ((data[1080] == '8') && (data[1081] == 'C') && (data[1082] == 'H') && (data[1083] == 'N'))
        m_nNumTracks = 8;
    else
        numsamples = 16;

    //  Setup the trackdata structure
    m_TrackDat_num = m_nNumTracks;
    m_TrackDat = (TrackData *) malloc(m_TrackDat_num * sizeof(TrackData));

    // Get the name
    memcpy(modname, &data[index], 20);
    modname[20] = 0;
    strcpy(m_szName, modname);
    index += 20;

    // Read in all the instrument headers - mod files have 31, sample #0 is ignored
    m_Samples_num = numsamples;
    m_Samples = (Sample *) malloc(m_Samples_num * sizeof(Sample));
	// BUGFIX: If you don't memset the first Sample to 0, 
	// all kind of bad things can happen on Win32!!!
	memset(m_Samples, 0, sizeof(Sample));
    for (i = 1; i < numsamples; i++) {
        // Read the sample name
        char samplename[23];
        memcpy(samplename, &data[index], 22);
        samplename[22] = 0;
        strcpy(m_Samples[i].szName, samplename);
        index += 22;

        // Read remaining info about sample
        m_Samples[i].nLength = ReadModWord(data, index);
        index += 2;
        m_Samples[i].nFineTune = (int) (unsigned char) *(data + index);
        index++;
        if (m_Samples[i].nFineTune > 7)
            m_Samples[i].nFineTune -= 16;
        m_Samples[i].nVolume = (int) (unsigned char) *(data + index);
        index++;
        m_Samples[i].nLoopStart = ReadModWord(data, index);
        index += 2;
        m_Samples[i].nLoopLength = ReadModWord(data, index);
        index += 2;
        m_Samples[i].nLoopEnd = m_Samples[i].nLoopStart + m_Samples[i].nLoopLength;

        // Fix loop end in case it goes too far
        if (m_Samples[i].nLoopEnd > m_Samples[i].nLength)
            m_Samples[i].nLoopEnd = m_Samples[i].nLength;
    }

    // Read in song data
    m_nSongLength = (int) (unsigned char) *(data + index);
    index++;
    index++;			// Skip over this byte, it's no longer used

    numpatterns = 0;
    m_nOrders_num = 128;
    m_nOrders = (int *) malloc(m_nOrders_num * sizeof(int));
    for (i = 0; i < 128; i++) {
        m_nOrders[i] = (int) (unsigned char) *(data + index);
        index++;
        if (m_nOrders[i] > numpatterns)
            numpatterns = m_nOrders[i];
    }
    numpatterns++;
    index += 4;			// skip over the identifier

    // Load in the pattern data
    m_Patterns_num = numpatterns;
    m_Patterns = (Pattern *) malloc(m_Patterns_num * sizeof(Pattern));
    for (i = 0; i < numpatterns; i++) {
        // Set the number of rows for this pattern, for mods it's always 64
        m_Patterns[i].numrows = 64;
        m_Patterns[i].row = (RowData *) malloc(m_Patterns[i].numrows * sizeof(RowData));

        // Loop through each row
        for (row = 0; row < 64; row++) {
            // Set the number of notes for this pattern
            m_Patterns[i].row[row].numnotes = m_nNumTracks;
            m_Patterns[i].row[row].note = (NoteData *) malloc(m_Patterns[i].row[row].numnotes * sizeof(NoteData));

            // Loop through each note
            for (note = 0; note < m_nNumTracks; note++) {
                int b0, b1, b2, b3, period;
                // Get the 4 bytes for this note
                b0 = (int) (unsigned char) *(data + index);
                b1 = (int) (unsigned char) *(data + index + 1);
                b2 = (int) (unsigned char) *(data + index + 2);
                b3 = (int) (unsigned char) *(data + index + 3);
                index += 4;

                // Parse them
                period = ((b0 & 0x0F) << 8) | b1;
                if (period)
                    //m_Patterns[i].row[row].note[note].period_index = (int)((log(856) - log(period)) / log(1.007246412224) + 8);// ??
                    m_Patterns[i].row[row].note[note].period_index = Period_Log_Lookup[period];
                else
                    m_Patterns[i].row[row].note[note].period_index = -1;

                m_Patterns[i].row[row].note[note].sample_num = (b0 & 0xF0) | (b2 >> 4);
                m_Patterns[i].row[row].note[note].effect = b2 & 0x0F;
                m_Patterns[i].row[row].note[note].effect_parms = b3;
            }
        }
    }

    // Load in the sample data
    for (i = 1; i < numsamples; i++) {
        int length;
        m_Samples[i].data_length = m_Samples[i].nLength;
        m_Samples[i].data = (char *) malloc(m_Samples[i].data_length + 1);
		memset(m_Samples[i].data, 0, m_Samples[i].data_length + 1);

        if (m_Samples[i].nLength) {
            memcpy(&m_Samples[i].data[0], &data[index], m_Samples[i].nLength);
        }
        index += m_Samples[i].nLength;

        // Duplicate the last byte, we'll need an extra one in order to safely anti-alias
        length = m_Samples[i].nLength;
        if (length > 0) {
            m_Samples[i].data[length] = m_Samples[i].data[length - 1];

            if (m_Samples[i].nLoopLength > 2)
                m_Samples[i].data[m_Samples[i].nLoopEnd] = m_Samples[i].data[m_Samples[i].nLoopStart];
        }
    }
    //  Set volume to full ready to play
    SetMasterVolume(64);
    m_bPlaying = false;

    return true;
}

// Starts playing a MOD
bool mod_play()
{
    int track;
    // See if I'm already playing
    if (m_bPlaying)
        return false;

    // Reset all track data
    for (track = 0; track < m_nNumTracks; track++) {
        m_TrackDat[track].sample = 0;
        m_TrackDat[track].pos = 0;
        m_TrackDat[track].period_index = 0;
        m_TrackDat[track].period = 0;
        m_TrackDat[track].volume = 0;
        m_TrackDat[track].mixvol = 0;
        m_TrackDat[track].porta = 0;
        m_TrackDat[track].portasp = 0;
        m_TrackDat[track].vibspe = 0;
        m_TrackDat[track].vibdep = 0;
        m_TrackDat[track].tremspe = 0;
        m_TrackDat[track].tremdep = 0;
        m_TrackDat[track].panval = 0;
        m_TrackDat[track].freq = 0;
        m_TrackDat[track].sinepos = 0;
        m_TrackDat[track].sineneg = 0;
    }

    // Get ready to play
    m_nSpeed = 6;
    m_nBPM = BPM_RATE;
    m_nSamplesLeft = 0;
    m_nOrder = 0;
    m_nRow = 0;
    m_CurrentRow = &m_Patterns[m_nOrders[m_nOrder]].row[m_nRow];
    m_nTick = 0;


	m_bPlaying = true;

#if defined(WIN32)
	winXAudio2StartVoice(0);
#endif

	return true;
}


void mod_pause()
{
	m_bPlaying = !m_bPlaying;
}


bool mod_stop()
{
    m_bPlaying = false;
#if defined(WIN32)
	winXAudio2StopVoice(0);
#endif
    return true;
}


//////////////////////////////////////////////////////////////////////
// Functions - Local and not public
//////////////////////////////////////////////////////////////////////

// This function mixes an entire chunk of sound which is then 
// to be sent to the sound driver, in this case the IOP module.
static bool MixChunk(int numsamples, short *buffer)
{
    // Calculate the number of samples per beat
    //  48000 / (125 * 2 / 5) = 48000/ 50 = 960
    int samples_per_beat = PLAYBACK_FREQ / (m_nBPM * 2 / 5);

    int thiscount;
    // Keep looping until we've filled the buffer
    int tickdata = 0;
    int samples_to_mix = numsamples;

    while (samples_to_mix) {
        // Only move on to the next tick if we finished mixing the last
        if (!m_nSamplesLeft) {
            // If we're on tick 0 then update the row
            if (m_nTick == 0) {
                // Get this row
                m_CurrentRow = &m_Patterns[m_nOrders[m_nOrder]].row[m_nRow];

                // Set up for next row (effect might change these values later)
                m_nRow++;
                if (m_nRow >= 64) {
                    m_nRow = 0;
                    m_nOrder++;
                    if (m_nOrder >= m_nSongLength)
                        m_nOrder = 0;
                }
                // Now update this row
                UpdateRow();
            }
            // Otherwise, all we gotta do is update the effects
            else {
                UpdateEffects();
            }

            // Move on to next tick
            m_nTick++;
            if (m_nTick >= m_nSpeed)
                m_nTick = 0;

            // Set the number of samples to mix in this chunk
            m_nSamplesLeft = samples_per_beat;
        }
        // Ok, so we know that we gotta mix 'm_nSamplesLeft' samples into
        // this buffer, see how much room we actually got
        thiscount = m_nSamplesLeft;
        if (thiscount > samples_to_mix)
            thiscount = samples_to_mix;

        // Make a note that we've added this amount
        m_nSamplesLeft -= thiscount;
        samples_to_mix -= thiscount;

        // Now mix it!
        MixSubChunk(&buffer[tickdata * 2], thiscount);
        tickdata += thiscount;
    }
    return true;
}

// This function is called whenever a new row is encountered. 
// It loops through each track, check's it's appropriate NoteData structure 
// and updates the track accordingly.
static void UpdateRow()
{
    int neworder = m_nOrder;
    int newrow = m_nRow;
    int track;

    // Loop through each track
    for (track = 0; track < m_nNumTracks; track++) {
        // Get note data
        NoteData *note = &m_CurrentRow->note[track];

        // Make a copy of each value in the NoteData structure so they'll 
        // be easier to work with (less typing)
        int sample = note->sample_num;
        int period = note->period_index;
        int effect = note->effect;
        int eparm = note->effect_parms;
        int eparmx = eparm >> 4;	// effect parameter x
        int eparmy = eparm & 0xF;	// effect parameter y

        // Are we changing the sample being played?
        if (sample > 0) {
            m_TrackDat[track].sample = sample;
            m_TrackDat[track].volume = m_Samples[sample].nVolume;
            m_TrackDat[track].mixvol = m_TrackDat[track].volume;
            if ((effect != 3) && (effect != 5))
                m_TrackDat[track].pos = 0;
        }
        // Are we changing the frequency being played?
        if (period >= 0) {
            // Remember the note
            m_TrackDat[track].period_index = note->period_index;

            // If not a porta effect, then set the channels frequency to the
            // looked up amiga value + or - any finetune
            if ((effect != 3) && (effect != 5)) {
                int notenum = m_TrackDat[track].period_index + m_Samples[m_TrackDat[track].sample].nFineTune;
                if (notenum < 0)
                    notenum = 0;
                if (notenum > NUMNOTES - 1)
                    notenum = NUMNOTES - 1;
                m_TrackDat[track].period = PeriodTable[notenum];
            }
            // If there is no sample number or effect then we reset the position
            if ((sample == 0) && (effect == 0))
                m_TrackDat[track].pos = 0;

            // Now reset a few things
            m_TrackDat[track].vibspe = 0;
            m_TrackDat[track].vibdep = 0;
            m_TrackDat[track].tremspe = 0;
            m_TrackDat[track].tremdep = 0;
            m_TrackDat[track].sinepos = 0;
            m_TrackDat[track].sineneg = 0;
        }
        // Process any effects - need to include 1, 2, 3, 4 and A
        switch (note->effect) {
            // Arpeggio
        case 0x00:
            break;		// tick effect

            // Porta Up
        case 0x01:
            break;		// tick effect

            // Porta Down
        case 0x02:
            break;		// tick effect

            // Porta to Note (3) and Porta + Vol Slide (5)
        case 0x03:
        case 0x05:
			m_TrackDat[track].porta = PeriodTable[m_TrackDat[track].period_index + m_Samples[sample].nFineTune];
            if (eparm > 0 && effect == 0x3)
                m_TrackDat[track].portasp = eparm;
            break;

            // Vibrato
        case 0x04:
            if (eparmx > 0)
                m_TrackDat[track].vibspe = eparmx;
            if (eparmy > 0)
                m_TrackDat[track].vibdep = eparmy;
            break;

            // Vibrato + Vol Slide
        case 0x06:
            break;		// tick effect

            // Tremolo
        case 0x07:
            if (eparmx > 0)
                m_TrackDat[track].tremspe = eparmx;
            if (eparmy > 0)
                m_TrackDat[track].tremdep = eparmy;
            break;

            // Pan - not supported in the mixing yet
        case 0x08:
            if (eparm == 0xa4)
                m_TrackDat[track].panval = 7;
            else
                m_TrackDat[track].panval = (eparm >> 3) - 1;
            if (m_TrackDat[track].panval < 0)
                m_TrackDat[track].panval = 0;
            break;

            // Sample offset
        case 0x09:
            m_TrackDat[track].pos = note->effect_parms << (FRAC_BITS + 8);
            break;

            // Volume Slide
        case 0x0A:
            break;		// tick effect

            // Jump To Pattern
        case 0x0B:
            neworder = note->effect_parms;
            if (neworder >= m_nSongLength)
                neworder = 0;
            newrow = 0;

            // Set Volume
        case 0x0C:
            m_TrackDat[track].volume = note->effect_parms;
            SlideVolume(track, 0);
            m_TrackDat[track].mixvol = m_TrackDat[track].volume;
            break;

            // Break from current pattern
        case 0x0D:
            newrow = eparmx * 10 + eparmy;
            if (newrow > 63)
                newrow = 0;
            neworder = m_nOrder + 1;
            if (neworder >= m_nSongLength)
                neworder = 0;
            break;

            // Extended effects
        case 0x0E:
            switch (eparmx) {
                // Set filter
            case 0x00:
                break;		// not supported

                // Fine porta up
            case 0x01:
                m_TrackDat[track].period -= eparmy;
                break;

                // Fine porta down
            case 0x02:
                m_TrackDat[track].period += eparmy;
                break;

                // Glissando 
            case 0x03:
                break;		// not supported


                // Set vibrato waveform
            case 0x04:
                break;		// not supported

                // Set finetune
            case 0x05:
                m_Samples[sample].nFineTune = eparmy;
                if (m_Samples[sample].nFineTune > 7)
                    m_Samples[sample].nFineTune -= 16;
                break;

                // Pattern loop
            case 0x6:
                break;		// not supported

                // Set tremolo waveform
            case 0x07:
                break;		// not supported

                // Pos panning - not supported in the mixing yet
            case 0x08:
                m_TrackDat[track].panval = eparmy;
                break;

                // Retrig Note
            case 0x09:
                break;		// tick effect

                // Fine volside up
            case 0x0A:
                SlideVolume(track, eparmy);
                m_TrackDat[track].mixvol = m_TrackDat[track].volume;
                break;

                // Fine volside down
            case 0xB:
                SlideVolume(track, -eparmy);
                m_TrackDat[track].mixvol = m_TrackDat[track].volume;
                break;

                // Cut note
            case 0x0C:
                break;		// tick effect

                // Delay note
            case 0x0D:
                break;		// not supported

                // Pattern delay
            case 0x0E:
                break;		// not supported

                // Invert loop
            case 0x0F:
                break;		// not supported
            }
            break;

            // Set Speed
        case 0x0F:
            if (eparm < 0x20)
                m_nSpeed = note->effect_parms;
            else
                m_nBPM = note->effect_parms;
            break;

        default:
            break;
        }

        // If we have something playing then set the frequency
        if (m_TrackDat[track].period > 0)
            m_TrackDat[track].freq = Period2Freq(m_TrackDat[track].period);
    }

    // Update our row and orders
    m_nRow = newrow;
    m_nOrder = neworder;
}

static bool MixSubChunk(short *buffer, int numsamples)
{
    int i, track;
    // Setup left and right channels
    short *left;
    short *right;
    left = buffer;
    right = buffer + 1;
    // Set up a mixing buffer and clear it
    for (i = 0; i < numsamples * 2; i++)
        buffer[i] = 0;

    // Loop through each channel and process note data
    for (track = 0; track < m_nNumTracks; track++) {
        // Make sure I'm actually playing something
        if (m_TrackDat[track].sample <= 0)
            continue;

        // Make sure this sample actually contains sound data
        if (!m_Samples[m_TrackDat[track].sample].data_length)
            continue;
        {
            // Set up for the mix loop
            short *mixed = ((track & 3) == 0)
                || ((track & 3) == 3) ? left : right;
            unsigned char *sample = (unsigned char *) &m_Samples[m_TrackDat[track].sample].data[0];
            int nLength = m_Samples[m_TrackDat[track].sample].nLength << FRAC_BITS;
            int nLoopLength = m_Samples[m_TrackDat[track].sample].nLoopLength << FRAC_BITS;
            int nLoopEnd = m_Samples[m_TrackDat[track].sample].nLoopEnd << FRAC_BITS;
            int finetune = m_Samples[m_TrackDat[track].sample].nFineTune;
            int notenum = m_TrackDat[track].period_index + finetune;
            float freq = m_TrackDat[track].freq;
            int pos = m_TrackDat[track].pos;
            int deltapos = (int) (freq * (1 << FRAC_BITS) / PLAYBACK_FREQ);
            int *VolumeTablePtr = VolumeTable[m_TrackDat[track].mixvol];
            int mixpos = 0;
            int samples_to_mix = numsamples;
            notenum = notenum < 0 ? 0 : notenum >= NUMNOTES ? NUMNOTES - 1 : notenum;

            while (samples_to_mix) {
                int thiscount;

                // If I'm a looping sample then I need to check if it's time to 
                // loop back. I also need to figure out how many samples I can mix
                // before I need to loop again
                if (nLoopEnd > (2 << FRAC_BITS)) {
                    if (pos >= nLoopEnd)
                        pos -= nLoopLength;
                    thiscount = min(samples_to_mix, (nLoopEnd - pos - 1) / deltapos + 1);
                    // above returns the smaller parameter

                    samples_to_mix -= thiscount;
                }
                // If I'm not a looping sample then mix until I'm done playing 
                // the entire sample
                else {
                    // If we've already reached the end of the sample then forget it
                    if (pos >= nLength)
                        thiscount = 0;
                    else
                        thiscount = min(numsamples, (nLength - pos - 1) / deltapos + 1);
                    samples_to_mix = 0;
                }

                // Inner Loop start
                for (i = 0; i < thiscount; i++) {
                    // Mix this sample in and update our position
#ifdef OVERSAMPLE
                    //  This smooths the sound a bit by oversampling, but
                    //  uses up more cpu (only a bit)
                    int sample1 = VolumeTablePtr[sample[pos >> FRAC_BITS]];
                    int sample2 = VolumeTablePtr[sample[(pos >> FRAC_BITS) + 1]];
                    int frac1 = pos & ((1 << FRAC_BITS) - 1);
                    int frac2 = (1 << FRAC_BITS) - frac1;
                    mixed[mixpos] += ((sample1 * frac2) + (sample2 * frac1)) >> FRAC_BITS;
                    mixpos += 2;
#else
                    //  This is normal plain mixing
                    mixed[mixpos] += VolumeTablePtr[sample[pos >> FRAC_BITS]];
                    mixpos += 2;
#endif
                    pos += deltapos;
                }
                // Inner Loop end
            }
            // Save current position
            m_TrackDat[track].pos = pos;
        }
    }

    return true;
}

static void UpdateEffects()
{
    int track;
    // Loop through each channel
    for (track = 0; track < m_nNumTracks; track++) {
        // Get note data
        NoteData *note = &m_CurrentRow->note[track];

        // Parse it
        int effect = note->effect;	// grab the effect number
        int eparm = note->effect_parms;	// grab the effect parameter
        int eparmx = eparm >> 4;	// grab the effect parameter x
        int eparmy = eparm & 0xF;	// grab the effect parameter y

        // Process it
        switch (effect) {
            // Arpeggio
        case 0x00:
            if (eparm > 0) {
                int notenum, period;
                switch (m_nTick % 3) {
                case 0:
                    period = m_TrackDat[track].period;
                    break;
                case 1:
                    notenum =
                        m_TrackDat[track].period_index + 8 * eparmx + m_Samples[m_TrackDat[track].sample].nFineTune;
                    period = PeriodTable[notenum];
                    break;
                case 2:
                    notenum =
                        m_TrackDat[track].period_index + 8 * eparmy + m_Samples[m_TrackDat[track].sample].nFineTune;
                    period = PeriodTable[notenum];
                    break;
                default:
                    period = 0;
                    break;
                }
                m_TrackDat[track].freq = Period2Freq(period);
            }
            break;


            // Porta up
        case 0x01:
            m_TrackDat[track].period -= eparm;	// subtract freq
            if (m_TrackDat[track].period < 54)
                m_TrackDat[track].period = 54;	// stop at C-5
            m_TrackDat[track].freq = Period2Freq(m_TrackDat[track].period);
            break;

            // Porta down
        case 0x02:
            m_TrackDat[track].period += eparm;	// add freq
            m_TrackDat[track].freq = Period2Freq(m_TrackDat[track].period);
            break;

            // Porta to note
        case 0x03:
            DoPorta(track);
            break;

            // Vibrato
        case 0x04:
            DoVibrato(track);
            break;

            // Porta + Vol Slide
        case 0x05:
            DoPorta(track);
            SlideVolume(track, eparmx - eparmy);
            m_TrackDat[track].mixvol = m_TrackDat[track].volume;
            break;

            // Vibrato + Vol Slide
        case 0x06:
            DoVibrato(track);
            SlideVolume(track, eparmx - eparmy);
            m_TrackDat[track].mixvol = m_TrackDat[track].volume;
            break;

            // Tremolo
        case 0x07:
            DoTremalo(track);
            break;

            // Pan
        case 0x08:
            break;		// note effect

            // Sample offset
        case 0x09:
            break;		// note effect

            // Volume slide
        case 0x0A:
            SlideVolume(track, eparmx - eparmy);
            m_TrackDat[track].mixvol = m_TrackDat[track].volume;
            break;

            // Jump To Pattern
        case 0x0B:
            break;		// note effect

            // Set Volume
        case 0x0C:
            break;		// note effect

            // Pattern Break
        case 0x0D:
            break;		// note effect

            // Extended effects
        case 0x0E:
            switch (eparmx) {
                // Retrig note
            case 0x9:
                break;		// not supported

                // Cut note
            case 0xC:
                if (m_nTick == eparmy) {
                    m_TrackDat[track].volume = 0;
                    m_TrackDat[track].mixvol = m_TrackDat[track].volume;
                }
                break;

                // Delay note
            case 0xD:
                break;		// not supported

                // All other Exy effects are note effects
            }

            // Set Speed
        case 0x0F:
            break;		// note effect
        }
    }
}

static void SlideVolume(int track, int amount)
{
    amount += m_TrackDat[track].volume;
    m_TrackDat[track].volume = amount < 0 ? 0 : amount > 64 ? 64 : amount;
}

static void DoPorta(int track)
{
    if (m_TrackDat[track].period < m_TrackDat[track].porta) {
        m_TrackDat[track].period += m_TrackDat[track].portasp;
        if (m_TrackDat[track].period > m_TrackDat[track].porta)
            m_TrackDat[track].period = m_TrackDat[track].porta;
    } else if (m_TrackDat[track].period > m_TrackDat[track].porta) {
        m_TrackDat[track].period -= m_TrackDat[track].portasp;
        if (m_TrackDat[track].period < m_TrackDat[track].porta)
            m_TrackDat[track].period = m_TrackDat[track].porta;
    }
    m_TrackDat[track].freq = Period2Freq(m_TrackDat[track].period);
}

static void DoVibrato(int track)
{
    int vib = m_TrackDat[track].vibdep * sintab[m_TrackDat[track].sinepos] >> 7;	// div 128
    if (m_TrackDat[track].sineneg == 0)
        m_TrackDat[track].freq = Period2Freq(m_TrackDat[track].period + vib);
    else
        m_TrackDat[track].freq = Period2Freq(m_TrackDat[track].period - vib);
    m_TrackDat[track].sinepos += m_TrackDat[track].vibspe;
    if (m_TrackDat[track].sinepos > 31) {
        m_TrackDat[track].sinepos -= 32;
        m_TrackDat[track].sineneg = ~m_TrackDat[track].sineneg;	// flip pos/neg flag
    }
}

static void DoTremalo(int track)
{
    int vib = m_TrackDat[track].tremdep * sintab[m_TrackDat[track].sinepos] >> 6;	// div64
    if (m_TrackDat[track].sineneg == 0) {
        if (m_TrackDat[track].volume + vib > 64)
            vib = 64 - m_TrackDat[track].volume;
        m_TrackDat[track].mixvol = m_TrackDat[track].volume + vib;
    } else {
        if (m_TrackDat[track].volume - vib < 0)
            vib = m_TrackDat[track].volume;
        m_TrackDat[track].mixvol = m_TrackDat[track].volume + vib;
    }

    m_TrackDat[track].sinepos += m_TrackDat[track].tremspe;
    if (m_TrackDat[track].sinepos > 31) {
        m_TrackDat[track].sinepos -= 32;
        m_TrackDat[track].sineneg = ~m_TrackDat[track].sineneg;	// flip pos/neg flag
    }
}

// Sets the master volume for the mod being played. 
// The value should be from 0 (silence) to 64 (max volume)
static void SetMasterVolume(int volume)
{
    int i, j;
    for (i = 0; i < 65; i++)
        for (j = 0; j < 256; j++)
            VolumeTable[i][j] = volume * i * (int) (char) j / 64;
}

// 16-bit word values in a mod are stored in the Motorola 
// Most-Significant-Byte-First format. 
// They're also stored at half their actual value, thus doubling their range. 
// This function accepts a pointer to such a word and returns it's integer value
// NOTE: relic from pc testing.
static int ReadModWord(unsigned char *data, int index)
{
    int byte1 = (int) (unsigned char) *(data + index);
    int byte2 = (int) (unsigned char) *(data + index + 1);
    return ((byte1 * 256) + byte2) * 2;
}
