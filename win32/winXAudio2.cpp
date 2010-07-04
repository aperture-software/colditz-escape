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
 *  winXAudio2.cpp: XAudio2 engine wrapper, with double buffered
 *  streaming capabilities through Voice Callbacks
 *  ---------------------------------------------------------------------------
 */

// Remove some annoying warnings
#define _WIN32_DCOM
#define _CRT_SECURE_NO_WARNINGS 1
#pragma warning(disable:4995)

#include <windows.h>
// If you don't include that initguid.h before xaudio2.h, you'll get
// error LNK2001: unresolved external symbol _CLSID_XAudio2
#include <initguid.h>
#include <xaudio2.h>
#include <strsafe.h>
#include <shellapi.h>
#include <mmsystem.h>
#include <conio.h>
#include "winXAudio2.h"

// Max number of voices (aka "channels") we can handle
#define NB_VOICES_MAX		4
// We'll use double buffering to fill our data for callbacks
#define NB_BUFFERS			2
// Buffer size, in bytes
#define BUFFER_SIZE			65536

// XAudio2 global variables
static IXAudio2*				pXAudio2 = NULL;
static IXAudio2MasteringVoice*	pMasteringVoice = NULL;
static IXAudio2SourceVoice*		pSourceVoice[NB_VOICES_MAX] =
                                {NULL, NULL, NULL, NULL};

static bool XAudio2Init	= false;
static bool voice_in_use[NB_VOICES_MAX];
static bool voice_set_up[NB_VOICES_MAX];

// Wanna have fun and waste one day debugging unexplainable crashes in Xaudio2 DLLs? Then just make
// x2buffer[NB_BUFFERS] a *PUBLIC* member of the VoiceCallBack class below and see what happens...
// Oh, and don't ask me why setting a buffer pointer as a public member of said class is no
// problemo, while an XAUDIO2_BUFFER (which is just a glorified buffer pointer struct anyway) isn't.
static XAUDIO2_BUFFER x2buffer[NB_VOICES_MAX][NB_BUFFERS];

// To setup buffer callbacks, we first create an override
// of the virtual IXAudio2VoiceCallback class
class VoiceCallback : public IXAudio2VoiceCallback
{
private:
    int						_voice;
    winXAudio2Callback_t	_callback;
    void*					_pdata;
    bool					_even_buffer;
    // buffer size, in number of samples
    unsigned long			_audio_sample_size;
public:
//	XAUDIO2_BUFFER			x2buffer[NB_BUFFERS];
    BYTE*					buffer;
    VoiceCallback(int voice, winXAudio2Callback_t callback, void* pdata, unsigned long audio_sample_size)
    {
        _voice = voice;
        _callback = callback;
        _pdata = pdata;
        _audio_sample_size = audio_sample_size;
        _even_buffer = true;
        buffer = (BYTE*) malloc(NB_BUFFERS*BUFFER_SIZE);
        x2buffer[_voice][0].pAudioData = buffer;
        x2buffer[_voice][0].AudioBytes = BUFFER_SIZE;
        x2buffer[_voice][1].pAudioData = buffer + BUFFER_SIZE;
        x2buffer[_voice][1].AudioBytes = BUFFER_SIZE;
    }
    ~VoiceCallback()
    {
        SAFE_FREE(buffer);
    }
    // Initial callback call and buffer submit
    void initCallback()
    {
        _even_buffer = true;	// Always start with even buffer
        _callback(buffer, _audio_sample_size, _pdata);
        pSourceVoice[_voice]->SubmitSourceBuffer( &(x2buffer[_voice][0]) );
    }
    // Called when the voice starts playing a new buffer => fill and enqueue the other one
    STDMETHOD_(void, OnBufferStart) (THIS_ void* pBufferContext)
    {
        if (_callback == NULL)
            // Don't bother if we don't have a callback
            return;
        // update the next buffer we'll submit
        int buffer_to_fill = _even_buffer?1:0;
        // Call the user defined callback function
        _callback( (void*)x2buffer[_voice][buffer_to_fill].pAudioData, _audio_sample_size, NULL);
        // Enqueue the new buffer
        pSourceVoice[_voice]->SubmitSourceBuffer( &(x2buffer[_voice][buffer_to_fill]) );
        _even_buffer = !_even_buffer;
    }

    // The ProcessingPassStart callback is a good way of finding if our fillout calls are fast enough
    STDMETHOD_(void, OnVoiceProcessingPassStart)(UINT32 BytesRequired) {
        if (BytesRequired != 0)
            fprintf(stderr, "winXAudio2: Voice[%d] is starving for %d bytes!\n", _voice, BytesRequired);
    }

    // Unused methods are stubs
    // Note: If you don't use STDMETHOD_(), you'll get:
    // "error C2695: overriding virtual function differs only by calling convention"
    STDMETHOD_(void, OnStreamEnd)() { };
    STDMETHOD_(void, OnVoiceProcessingPassEnd)() { }
    STDMETHOD_(void, OnBufferEnd)(void * pBufferContext)    { }
    STDMETHOD_(void, OnLoopEnd)(void * pBufferContext) {    }
    STDMETHOD_(void, OnVoiceError)(void * pBufferContext, HRESULT Error) { }
};


// This is the handle for our instanciated callbacks
static VoiceCallback*	pVoiceCallback[NB_VOICES_MAX] =
                        {NULL, NULL, NULL, NULL};


// Initialize the XAudio2 engine for playback
bool winXAudio2Init()
{
int i;

    // Already init'd
    if (XAudio2Init)
        return true;

    // Get a handle to the XAudio engine
    CoInitializeEx( NULL, COINIT_MULTITHREADED );
    if( FAILED( XAudio2Create(&pXAudio2, 0) ) )
    {
        fprintf(stderr, "winXAudio2Init: Failed to init XAudio2 engine\n");
        CoUninitialize();
        return false;
    }

    // Create the master voice
    pMasteringVoice = NULL;
    if( FAILED( pXAudio2->CreateMasteringVoice( &pMasteringVoice ) ) )
    {
        fprintf(stderr, "winXAudio2Init: Failed to create mastering voice\n");
        SAFE_RELEASE( pXAudio2 );
        CoUninitialize();
        return false;
    }

    for (i=0; i<NB_VOICES_MAX; i++)
    {
        voice_in_use[i] = false;
        voice_set_up[i] = false;
    }

    XAudio2Init = true;
    return true;
}


// Kill the XAudio2 Engine
bool winXAudio2Release()
{
    if (XAudio2Init)
    {
        SAFE_RELEASE( pXAudio2 );
        CoUninitialize();
    }
    XAudio2Init = false;
    return true;
}


// Setup a voice (channel) for a static audio buffer (eg. static sample of fixed size)
bool winXAudio2SetVoice(int voice, BYTE* audioData, int audioSize, unsigned int frequency,
                        unsigned int bits_per_sample, bool stereo)
{

    if ((voice<0) || (voice>(NB_VOICES_MAX-1)))
    {
        fprintf(stderr, "winXAudio2SetVoice: Voice ID must be in [0-%d]\n", (NB_VOICES_MAX-1));
        return false;
    }

    if (voice_in_use[voice])
    {
        fprintf(stderr, "winXAudio2SetVoice: Voice %d already in use\n", voice);
        return false;
    }

    // Set the format of the source voice
    WAVEFORMATEX* pwfx = ( WAVEFORMATEX* )new CHAR[ sizeof( WAVEFORMATEX ) ];
    pwfx->wFormatTag = WAVE_FORMAT_PCM;
    pwfx->wBitsPerSample = bits_per_sample;
    pwfx->cbSize = 0;
    pwfx->nChannels = (stereo?2:1);
    pwfx->nSamplesPerSec = frequency;
    pwfx->nBlockAlign = ((pwfx->wBitsPerSample+7)/8) * pwfx->nChannels;
    pwfx->nAvgBytesPerSec = pwfx->nSamplesPerSec * pwfx->nBlockAlign;

    // Create the source voice
    SAFE_DELETE(pSourceVoice[voice]);
    if( FAILED( pXAudio2->CreateSourceVoice( &pSourceVoice[voice], pwfx) ) )
    {
        fprintf(stderr, "winXAudio2SetVoice: Error creating source voice\n");
        return false;
    }

    // Make sure the VoiceCallback is set to NULL
    SAFE_DELETE(pVoiceCallback[voice]);

    // Submit the wave sample data using an XAUDIO2_BUFFER structure
    XAUDIO2_BUFFER x2buffer = {0};
    x2buffer.AudioBytes = audioSize;
    x2buffer.pAudioData = audioData;
    x2buffer.Flags = XAUDIO2_END_OF_STREAM;

    if( FAILED( pSourceVoice[voice]->SubmitSourceBuffer( &x2buffer ) ) )
    {
        fprintf(stderr, "winXAudio2SetVoice: Error submitting source buffer\n");
        pSourceVoice[voice]->DestroyVoice();
        return false;
    }

    voice_set_up[voice] = true;

    return true;
}


bool winXAudio2SetVoiceVolume(int voice, float volume)
{
     if( FAILED( pSourceVoice[voice]->SetVolume(volume) ) )
         return false;
     return true;
}


// Setup a voice (channel) for a dynamic audio buffer (eg. streaming)
// Requires a pointer to the callback function that fills the streaming buffer
bool winXAudio2SetVoiceCallback(int voice, winXAudio2Callback_t callback, void * pdata,
                                unsigned int frequency, unsigned int bits_per_sample, bool stereo)
{
unsigned long	audio_sample_size;
unsigned char	nb_channels;

    if ((voice<0) || (voice>(NB_VOICES_MAX-1)))
    {
        fprintf(stderr, "winXAudio2SetVoiceCallback: Voice ID must be in [0-%d]\n", (NB_VOICES_MAX-1));
        return false;
    }

    if (voice_in_use[voice])
    {
        fprintf(stderr, "winXAudio2SetVoiceCallback: Voice %d already in use\n", voice);
        return false;
    }

    // Set the format of the source voice according to our parameters
    nb_channels = stereo?2:1;
    audio_sample_size = (BUFFER_SIZE/((bits_per_sample+7)/8*nb_channels));

    WAVEFORMATEX* pwfx = ( WAVEFORMATEX* )new CHAR[ sizeof( WAVEFORMATEX ) ];
    pwfx->wFormatTag = WAVE_FORMAT_PCM;
    pwfx->wBitsPerSample = bits_per_sample;
    pwfx->cbSize = 0;
    pwfx->nChannels = nb_channels;
    pwfx->nSamplesPerSec = frequency;
    pwfx->nBlockAlign = ((pwfx->wBitsPerSample+7)/8) * pwfx->nChannels;
    pwfx->nAvgBytesPerSec = pwfx->nSamplesPerSec * pwfx->nBlockAlign;

    // Instantiate the relevant voice callback
    SAFE_DELETE(pVoiceCallback[voice]);
    pVoiceCallback[voice] = new VoiceCallback(voice, callback, pdata, audio_sample_size);

    // Create the source voice and set it to have buffer callback
    SAFE_DELETE(pSourceVoice[voice]);
    if( FAILED( pXAudio2->CreateSourceVoice( &pSourceVoice[voice], pwfx,
        0, XAUDIO2_DEFAULT_FREQ_RATIO, pVoiceCallback[voice], NULL, NULL) ) )
    {
        fprintf(stderr, "winXAudio2SetVoiceCallback: Error creating source voice\n" );
        return false;
    }

    voice_set_up[voice] = true;

    return true;
}


// Start playout of a previously initialized voice
bool winXAudio2StartVoice(int voice)
{
    if (!voice_set_up[voice])
    {
        fprintf(stderr, "winXAudio2StartVoice: voice %d has not been initialized\n", voice);
        return false;
    }

    // If the voice has a callback, then we need to initiate the fillout too
    if (pVoiceCallback[voice] != NULL)
        pVoiceCallback[voice]->initCallback();

    pSourceVoice[voice]->Start();
    voice_in_use[voice] = true;
    return true;
}


// Stop playout
bool winXAudio2StopVoice(int voice)
{
    if (!voice_in_use[voice])
        return false;
    pSourceVoice[voice]->Stop();
    pSourceVoice[voice]->FlushSourceBuffers();
    voice_in_use[voice] = false;
    return true;
}


// Release the voice
bool winXAudio2ReleaseVoice(int voice)
{
    if (!voice_set_up[voice])
        return false;
    winXAudio2StopVoice(voice);
    pSourceVoice[voice]->DestroyVoice();
    SAFE_FREE(pSourceVoice[voice]);
    voice_set_up[voice] = false;
    return true;
}
