// winXAudio2.cpp: A simple XAudio2 engine initialization for double
// buffered 1 channel playback
//
////////////////////////////////////////////////////////////////////////////

// Remove some annoying warnings
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
#include "winXAudio2.h"

// We'll use double simple double buffering to fill our data
// Setup the marker for the buffer to use
static bool even_buffer = true;

// We just use a single buffer that we split in 2
short			_buf_data[NB_BUFFERS*BUFFER_SIZE*2];
short*			buf_data[NB_BUFFERS] = {_buf_data, _buf_data+2*BUFFER_SIZE};
XAUDIO2_BUFFER	buffer[NB_BUFFERS];

// XAudio2 global variables
IXAudio2*				pXAudio2 = NULL;
IXAudio2MasteringVoice* pMasteringVoice = NULL;
IXAudio2SourceVoice*	pSourceVoice = NULL;


// To setup buffer callbacks, we need to override the virtual callback class
class VoiceCallback : public IXAudio2VoiceCallback
{
private:
	// Where we will store our callback address
	winXAudio2Callback_t winXAudio2Callback;
public:
	// Called when the voice starts playing a new buffer
	// Don't ask me why we need to use STDMETHOD() - ask Microsoft!
	STDMETHOD_(void, OnBufferStart) (THIS_ void* pBufferContext) 
	{ 
//		printf("OnBufferStart, to_fill %d\n", buffer_to_fill);
		if (this->winXAudio2Callback == NULL)
			// Don't bother if we don't have a callback
			return;
		// update the next buffer we'll submit
		int buffer_to_fill = even_buffer?1:0;
		// Call the user defined callback function
		this->winXAudio2Callback(buf_data[buffer_to_fill], BUFFER_SIZE/2, NULL);
		// Enqueue the new buffer
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
	void setCallback(winXAudio2Callback_t callback) { winXAudio2Callback = callback; }
	void forceCallback(void *buf, unsigned int reqn, void *pdata) {
		winXAudio2Callback(buf, reqn, pdata) ; }
};


// Now that we have the class
static	VoiceCallback voiceCallback;

//static void ModPlayCallback(void *_buf2, unsigned int length, void *pdata)
//static void (*fillOutputBuffer) (void*, unsigned int, void*);

bool winXAudio2SetVoiceCallback(int nb_voices, winXAudio2Callback_t callback, void * pdata)
{
//	HRESULT hr = S_OK;

	// This is just meant to fit our immediate need for this game
	if (nb_voices != 1)
	{
		printf( "Cannot handle more than one Voice\n" );
		return false;
	}

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
    if( FAILED( pXAudio2->CreateSourceVoice( &pSourceVoice, pwfx,
		0, XAUDIO2_DEFAULT_FREQ_RATIO, &voiceCallback, NULL, NULL) ) )
    {
        printf( "Error creating source voice\n" );
        return false;
    }

	// Initialize our Xaudio2 buffer structures
	buffer[0].pAudioData = (BYTE*) buf_data[0];
	buffer[0].AudioBytes = 2*BUFFER_SIZE;
	buffer[1].pAudioData = (BYTE*) buf_data[1];
	buffer[1].AudioBytes = 2*BUFFER_SIZE;

	// Don't forget to set the callback
	voiceCallback.setCallback(callback);

	return true;
}


void winXAudio2Release()
{
    SAFE_RELEASE( pXAudio2 );
    CoUninitialize();
}

void winXAudio2Start()
{
	// Indicate that we start with the first buffer
	even_buffer = true;
	// Fill in the first buffer
	voiceCallback.forceCallback(buf_data[0], BUFFER_SIZE/2, NULL);
	// Submit it
	pSourceVoice->SubmitSourceBuffer( &(buffer[0]) );
	// And play!
    pSourceVoice->Start();
}


void winXAudio2Stop()
{
	pSourceVoice->Stop();
	pSourceVoice->FlushSourceBuffers();
}