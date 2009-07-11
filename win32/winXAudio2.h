// winXAudio2.h: headers for Window's XAudio2 sound engine
//
//////////////////////////////////////////////////////////////////////

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

// Windows XAudio2 Helper macros
#ifndef SAFE_DELETE
#define SAFE_DELETE(p) { if(p) { delete (p);   (p)=NULL; } }
#endif
#ifndef SAFE_DELETE_ARRAY
#define SAFE_DELETE_ARRAY(p) { if(p) { delete[] (p);   (p)=NULL; } }
#endif
#ifndef SAFE_RELEASE
#define SAFE_RELEASE(p)      { if(p) { (p)->Release(); (p)=NULL; } }
#endif
#ifndef SAFE_FREE
#define SAFE_FREE(p)      { if(p) { free(p); (p)=NULL; } }
#endif


// typedef for our callback function. This callback is setup to be compatible
// with the PSP Audio callback
typedef void(* winXAudio2Callback_t)(void *buf, unsigned int reqn, void *pdata); 

// Public functions
bool winXAudio2Init();
bool winXAudio2SetVoice(int voice, BYTE* audioData, int audioSize, unsigned int frequency, 
						unsigned int bits_per_sample, bool stereo);
bool winXAudio2SetVoiceCallback(int channel, winXAudio2Callback_t callback, void* pdata, 
						unsigned int frequency, unsigned int bits_per_sample, bool stereo);
bool winXAudio2Release();
bool winXAudio2StartVoice(int voice);
bool winXAudio2StopVoice(int voice);
bool winXAudio2SetVoiceVolume(int voice, float volume);
bool winXAudio2ReleaseVoice(int voice);

#ifdef __cplusplus
}
#endif

