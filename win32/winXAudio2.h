// winXAudio2.h: headers for Window's XAudio2 engine
//
//////////////////////////////////////////////////////////////////////

#ifndef _WINXAUDIO2_H
#define _WINXAUDIO2_H

#ifdef __cplusplus
extern "C" {
#endif

// Windows XAudio2 Helper macros
#ifndef SAFE_DELETE_ARRAY
#define SAFE_DELETE_ARRAY(p) { if(p) { delete[] (p);   (p)=NULL; } }
#endif
#ifndef SAFE_RELEASE
#define SAFE_RELEASE(p)      { if(p) { (p)->Release(); (p)=NULL; } }
#endif

#define BUFFER_SIZE 65536
#define NB_BUFFERS 2

#define PLAYBACK_FREQ 44100
#define NUMCHANNELS 2

// typedef for our callback function. Note that pdata is not handled by our code
typedef void(* winXAudio2Callback_t)(void *buf, unsigned int reqn, void *pdata); 

// Helper prototypes 
bool winXAudio2SetVoiceCallback(int channel, winXAudio2Callback_t callback, void* pdata);
void winXAudio2Release();
void winXAudio2Start();
void winXAudio2Stop();

#ifdef __cplusplus
}
#endif
#endif
