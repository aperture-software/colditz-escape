// modplayer.c: Module Player Implementation in C for Sony PSP and Windows
//
// based on the PSP SDK "PSP ModPlayer v1.0" by adresd
//
// Much of the information in this file (particularly the code used to do 
// the effects) came from Brett Paterson's MOD Player Tutorial.
// Also contains some code by Mark Feldman, which is not subject to a license
// of any kind.
//
// This is not the most efficient bit of code in the world and there are a lot
// of optimisations that could be done, but it is released as a working 
// modplayer for PSP which can be used and expanded upon by others.
// I would ask that anyone who expands or improves it considers releasing
// an updated version of the source, as a courtesy.
//
// This code is released with no implied warranty or assurance that it works
// it is not subject to any GPL or suchlike license, so use and enjoy.
//
//                   -- adresd
////////////////////////////////////////////////////////////////////////////


#if defined(PSP)
#include <pspkernel.h>
#include <pspaudiolib.h>
#endif

#if defined(WIN32)
#define _WIN32_DCOM
#define _CRT_SECURE_NO_WARNINGS 1 
#pragma warning(disable:4995)
#include <windows.h>
// If you don't include that "£$%^&^%$ thing before xaudio2.h, you'll get
// error LNK2001: unresolved external symbol _CLSID_XAudio2
#include <initguid.h>
#include <xaudio2.h>
#include <strsafe.h>
#include <shellapi.h>
#include <mmsystem.h>
#include <conio.h>
#include "win32/winXAudio2.h"
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "data-types.h"
#include "modplayer.h"
#include "modplayeri.h"
#include "modtables.h"

//#include "SDKwavefile.h"


// As a sample is being mixed into the buffer it's position pointer is updated.
// The position pointer is a 32-bit integer that is used to store a fixed-point
// number. The following constant specifies how many of the bits should be used
// for the fractional part.
#define FRAC_BITS 10

#define NUMCHANNELS 2

// This #define is used to convert an Amiga period number to a frequency. 
// The freqency returned is the frequency that the sample should be played at.
//  3579545.25f / 428 = 8363.423 Hz for Middle C (PAL)
#define Period2Freq(period) (3579545.25f / (period))

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
static bool m_bPlaying;		// Set to true when a mod is being played
static u8 *data;
int size = 0;


/*
//////////////////////////////////////////////////////////////////////
// Windows specific
//////////////////////////////////////////////////////////////////////
#if defined(WIN32)
//#define BUFFER_SIZE 65536
//#define NB_BUFFERS 2
// We'll use double simple double buffering to fill our MOD data
static bool even_buffer = true;
short  _buf_data[NB_BUFFERS*BUFFER_SIZE*2];
short* buf_data[NB_BUFFERS] = {_buf_data, _buf_data+2*BUFFER_SIZE};
XAUDIO2_BUFFER buffer[NB_BUFFERS];
IXAudio2* pXAudio2 = NULL;
IXAudio2MasteringVoice* pMasteringVoice = NULL;
IXAudio2SourceVoice* pSourceVoice;

class VoiceCallback : public IXAudio2VoiceCallback
{
public:
	//Called when the voice starts playing a new buffer
	STDMETHOD_(void, OnBufferStart) (THIS_ void* pBufferContext) 
	{ 
		// update the next buffer we'll submit
		int buffer_to_fill = even_buffer?1:0;
//		printf("OnBufferStart, to_fill %d\n", buffer_to_fill);
		ModPlayCallback(buf_data[buffer_to_fill], BUFFER_SIZE/2, NULL);
		pSourceVoice->SubmitSourceBuffer( &(buffer[buffer_to_fill]) ); 
		even_buffer = !even_buffer;
	}

    //Unused methods are stubs
	STDMETHOD_(void, OnStreamEnd)() { }; 
	STDMETHOD_(void, OnVoiceProcessingPassEnd)() { }
    STDMETHOD_(void, OnVoiceProcessingPassStart)(UINT32 SamplesRequired) {    }
    STDMETHOD_(void, OnBufferEnd)(void * pBufferContext)    { }
    STDMETHOD_(void, OnLoopEnd)(void * pBufferContext) {    }
    STDMETHOD_(void, OnVoiceError)(void * pBufferContext, HRESULT Error) { }
};

// Create the source voice callbacks
static	VoiceCallback voiceCallback;

#endif
*/



//////////////////////////////////////////////////////////////////////
// These are the public functions
//////////////////////////////////////////////////////////////////////
static int modplayint_channel;

bool Mod_EndOfStream()
{
    return false;
}

void Mod_GetTimeString(char *dest)
{
    *dest = '\0';
    //HH:MM:SS
    sprintf(dest, "%02d:%02d:%02d", m_nOrder, m_nRow, m_nTick);
}

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

bool Mod_Init()
{
    m_bPlaying = false;
#if defined(PSP)
    pspAudioSetChannelCallback(1, ModPlayCallback,0);
#endif
#if defined(WIN32)
	if (!winXAudio2SetVoiceCallback(1, ModPlayCallback,0))
		return false;
#endif
#if defined(YARGL)
	HRESULT hr = S_OK;
	//	winX2AudioSetChannelCallback(1, 

	// Get a handle to the XAudio engine
	CoInitializeEx( NULL, COINIT_MULTITHREADED );
    if( FAILED( XAudio2Create(&pXAudio2, 0) ) )
    {
        printf( "Failed to init XAudio2 engine\n" );
        CoUninitialize();
        return false;
    }

	// Create the master voice
    pMasteringVoice = NULL;
    if( FAILED( pXAudio2->CreateMasteringVoice( &pMasteringVoice ) ) )
    {
        printf( "Failed creating mastering voice\n" );
        SAFE_RELEASE( pXAudio2 );
        CoUninitialize();
        return false;
    }

    // Set the format of the source voice
	WAVEFORMATEX* pwfx = ( WAVEFORMATEX* )new CHAR[ sizeof( WAVEFORMATEX ) ];
	pwfx->wFormatTag = WAVE_FORMAT_PCM;
	pwfx->wBitsPerSample = 16;
	pwfx->cbSize = 0;
	pwfx->nChannels = NUMCHANNELS;
	pwfx->nSamplesPerSec = PLAYBACK_FREQ;
	pwfx->nBlockAlign = pwfx->wBitsPerSample * pwfx->nChannels / 8;
	pwfx->nAvgBytesPerSec = pwfx->nSamplesPerSec * pwfx->nBlockAlign;

/*
/// DEBUG
    WCHAR strFilePath[MAX_PATH];
    if( FAILED( hr = FindMediaFileCch( strFilePath, MAX_PATH, L"LOADTUNE.WAV" ) ) )
    {
        wprintf( L"Failed to find media file\n" );
        return hr;
    }

    //
    // Read in the wave file
    //
    CWaveFile wav;
    if( FAILED( hr = wav.Open( strFilePath, NULL, WAVEFILE_READ ) ) )
    {
        wprintf( L"Failed reading WAV file: %#X (%s)\n", hr, strFilePath );
        return hr;
    }

    // Get format of wave file
    WAVEFORMATEX* pwfx = wav.GetFormat();

    // Calculate how many bytes and samples are in the wave
    DWORD cbWaveSize = wav.GetSize();

    // Read the sample data into memory
    BYTE* pbWaveData = new BYTE[ cbWaveSize ];

    if( FAILED( hr = wav.Read( pbWaveData, cbWaveSize, &cbWaveSize ) ) )
    {
        wprintf( L"Failed to read WAV data: %#X\n", hr );
        SAFE_DELETE_ARRAY( pbWaveData );
        return hr;
    }
/// end of debug

*/

	// Create the source voice and set it to have buffer callback
    if( FAILED( hr = pXAudio2->CreateSourceVoice( &pSourceVoice, pwfx,
		0, XAUDIO2_DEFAULT_FREQ_RATIO, &voiceCallback, NULL, NULL) ) )
    {
        printf( "Error %X creating source voice\n", hr );
        return false;
    }

	// Initialize our Xaudio2 buffer structures
	buffer[0].pAudioData = (BYTE*) buf_data[0];
	buffer[0].AudioBytes = 2*BUFFER_SIZE;
	buffer[1].pAudioData = (BYTE*) buf_data[1];
	buffer[1].AudioBytes = 2*BUFFER_SIZE;

/*
    // Submit the wave sample data using an XAUDIO2_BUFFER structure
    XAUDIO2_BUFFER buffer = {0};

    buffer.pAudioData = pbWaveData;
    buffer.Flags = XAUDIO2_END_OF_STREAM;  // tell the source voice not to expect any data after this buffer
    buffer.AudioBytes = cbWaveSize;

    if( FAILED( pSourceVoice->SubmitSourceBuffer( &buffer ) ) )
    {
        printf( "Error submitting source buffer\n" );
        pSourceVoice->DestroyVoice();
        SAFE_DELETE_ARRAY( pbWaveData );
        return false;
    }

*/

#endif
	return true;
}


void Mod_FreeTune()
{
    int i;
    int row;

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

void Mod_End()
{
    Mod_Stop();
#if defined(PSP)
    pspAudioSetChannelCallback(modplayint_channel, 0,0);
#endif
#if defined(WIN32)
	winXAudio2Release();
#endif
    Mod_FreeTune();
}



//////////////////////////////////////////////////////////////////////
// Functions - Local and not public
//////////////////////////////////////////////////////////////////////

//  This is the initialiser and module loader
//  This is a general call, which loads the module from the 
//  given address into the modplayer
//
//  It basically loads into an internal format, so once this function
//  has returned the buffer at 'data' will not be needed again.
bool Mod_Load(char *filename)
{
    int i, numpatterns, row, note;
    int index = 0;
    int numsamples;
    char modname[21];
    FILE* fd;

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
            printf("Error allocing\n");
			fclose(fd);
            return false;
        }
        // Close file
		fclose(fd);
    } else {			//if we couldn't open the file
		printf("no file\n");
        return false;
    }

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

// This function initialises for playing, and starts
bool Mod_Play()
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
	winXAudio2Start();
	// Fill in the first buffer
//	ModPlayCallback(buf_data[0], BUFFER_SIZE/2, NULL);
	// Submit it
//	pSourceVoice->SubmitSourceBuffer( &(buffer[0]) );
	// And play!
//    pSourceVoice->Start();
#endif

	return true;
}

void Mod_Pause()
{
    m_bPlaying = !m_bPlaying;
}

bool Mod_Stop()
{
    // Close it
    m_bPlaying = false;
#if defined(WIN32)
	winXAudio2Stop();
#endif
    return true;
}

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
