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
 *  winXAudio2.h: headers for Window's XAudio2 sound engine
 *  ---------------------------------------------------------------------------
 */
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

