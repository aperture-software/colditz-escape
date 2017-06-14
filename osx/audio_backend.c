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
 *  audio_backend.c: OSX Core Audio backend
 *  Mostly from http://kaniini.dereferenced.org/2014/08/31/CoreAudio-sucks.html
 *  ---------------------------------------------------------------------------
 */

#include <CoreAudio/CoreAudio.h>
#include <AudioUnit/AudioUnit.h>

#include "low-level.h"
#include "audio_backend.h"

// Max number of voices (aka "channels") we can handle
#define NB_VOICES           4

#if !defined(min)
#define min(a,b) (((a)<(b))?(a):(b))
#endif

typedef struct
{
    int     voice;
    void*   ptr;
    SInt32  rem;
    UInt8   memset_value;
} audio_backend_voice_data_t;

typedef struct {
    audio_backend_voice_callback_t  callback;
    void*                           pdata;
} audio_backend_render_callback_data_t;

static AudioComponent audio_unit;
static AudioComponentInstance voice_unit[NB_VOICES];
static bool voice_in_use[NB_VOICES];
static bool voice_set_up[NB_VOICES];
static audio_backend_render_callback_data_t audio_backend_render_callback_data[NB_VOICES];
static audio_backend_voice_data_t audio_backend_voice_data[NB_VOICES];

static const char* snd_error(OSStatus error)
{
    static char str[] = "'xxxx' Error";
    *(UInt32 *)(str + 1) = CFSwapInt32HostToBig(error);
    if (isprint(str[1]) && isprint(str[2]) && isprint(str[3]) && isprint(str[4]))
    {
        switch(error)
        {
        case kAudioHardwareNoError:
            return "No Error";
        case kAudioHardwareNotRunningError:
            return "Not Running";
        case kAudioHardwareUnspecifiedError:
            return "Unspecified Error";
        case kAudioHardwareUnknownPropertyError:
            return "Unknown Property";
        case kAudioHardwareBadPropertySizeError:
            return "Bad Property Size";
        case kAudioHardwareIllegalOperationError:
            return "Illegal Operation";
        case kAudioHardwareBadDeviceError:
            return "Bad Device";
        case kAudioHardwareBadStreamError:
            return "Bad Stream";
        case kAudioHardwareUnsupportedOperationError:
            return "Unsupported Operation";
        case kAudioDeviceUnsupportedFormatError:
            return "Unsupported Format";
        case kAudioDevicePermissionsError:
            return "Permission Error";
        default:
            return str;
        }
    } else {
        // Else format it as an integer
        snprintf(str, sizeof(str), "Error %d", (int)error);
    }
    return str;
}

bool audio_backend_init(void)
{
    int i;
    OSStatus err;

    // Open the default audio device
    AudioComponentDescription desc;
    desc.componentType = kAudioUnitType_Output;
    desc.componentSubType = kAudioUnitSubType_DefaultOutput;
    desc.componentFlags = 0;
    desc.componentFlagsMask = 0;
    desc.componentManufacturer = kAudioUnitManufacturer_Apple;

    audio_unit = AudioComponentFindNext(NULL, &desc);
    if (!audio_unit)
    {
        perr("audio_backend_init: Could not open default audio device.\n");
        return false;
    }

    for (i=0; i<NB_VOICES; i++)
    {
        err = AudioComponentInstanceNew(audio_unit, &voice_unit[i]);
        if (err)
        {
            perr("audio_backend_init: Could not open instance for voice %d: %s\n", i, snd_error(err));
            return false;
        }
    }

    return true;
}

static OSStatus audio_backend_render_callback(void *pdata, AudioUnitRenderActionFlags *action_flags,
                                              const AudioTimeStamp *time_stamp, UInt32 bus_number,
                                              UInt32 number_of_frames, AudioBufferList *buffer_list)
{
    UInt32 req = buffer_list->mBuffers[0].mDataByteSize;
    audio_backend_render_callback_data_t* data = (audio_backend_render_callback_data_t*)pdata;
    if (data->callback != NULL)
    {
        // Looping callback
        data->callback(buffer_list->mBuffers[0].mData, number_of_frames, data->pdata);
    }
    else
    {
        // One shot sample
        audio_backend_voice_data_t* voice_data = (audio_backend_voice_data_t*)data->pdata;
        if (voice_data->rem <= 0)
        {
            memset(buffer_list->mBuffers[0].mData, voice_data->memset_value, req);
            // Kill our voice if we are asked to silence the buffer for the second time.
            // This is needed because Core Audio is dreadful in terms of performance if
            // multiple callbacks are running (even if these callbacks do nothing)...
            if (voice_data->ptr == NULL)
                AudioOutputUnitStop(voice_unit[voice_data->voice]);
            else
                voice_data->ptr = NULL;
            return noErr;
        }

        memcpy(buffer_list->mBuffers[0].mData, voice_data->ptr, min(req, voice_data->rem));
        // Silence the rest of the buffer if needed
        if (voice_data->rem < req)
            memset(buffer_list->mBuffers[0].mData + voice_data->rem, voice_data->memset_value,
                   req - voice_data->rem);

        voice_data->rem -= req;
        voice_data->ptr += req;
    }
    return noErr;
}

bool audio_backend_set_voice(int voice, void* data, int size, unsigned int frequency,
                             unsigned int bits_per_sample, bool stereo)
{
    OSStatus err;
    AudioFormatFlags format_flags;
    AudioStreamBasicDescription streamFormat;
    int nb_channels = stereo ? 2 : 1;

    if ((voice<0) || (voice>(NB_VOICES - 1)))
    {
        perr("audio_backend_set_voice: Voice ID must be in [0-%d]\n", (NB_VOICES - 1));
        return false;
    }
    if (voice_in_use[voice])
    {
        perr("audio_backend_set_voice: Voice %d already in use\n", voice);
        return false;
    }

    audio_backend_voice_data[voice].voice = voice;
    audio_backend_voice_data[voice].ptr = data;
    audio_backend_voice_data[voice].rem = size;
    audio_backend_render_callback_data[voice].callback = NULL;
    audio_backend_render_callback_data[voice].pdata = &audio_backend_voice_data[voice];

    switch (bits_per_sample)
    {
    case 8:
        format_flags = 0;   // Unsigned
        // Our mono samples have their zero at 0x80, so use that value for the "silencing"
        // memset's. Otherwise, we get a very loud "POP" at the end of playback...
        audio_backend_voice_data[voice].memset_value = 0x80;
        break;
    case 16:
        format_flags = kAudioFormatFlagIsSignedInteger;
        audio_backend_voice_data[voice].memset_value = 0x00;
        break;
    default:
        perr("audio_backend_set_voice_callback: Only 8 and 16 bits samples are supported\n");
        return false;
    }

    err = AudioUnitInitialize(voice_unit[voice]);
    if (err)
    {
        perr("audio_backend_set_voice: Could not initialize voice instance: %s\n", snd_error(err));
        return false;
    }

    streamFormat.mFormatID = kAudioFormatLinearPCM;
    streamFormat.mFormatFlags = format_flags;
    streamFormat.mSampleRate = frequency;
    streamFormat.mBitsPerChannel = bits_per_sample;
    streamFormat.mChannelsPerFrame = nb_channels;
    streamFormat.mFramesPerPacket = 1;
    streamFormat.mBytesPerPacket = ((streamFormat.mBitsPerChannel + 7) / 8) * nb_channels;
    streamFormat.mBytesPerFrame = streamFormat.mFramesPerPacket * streamFormat.mBytesPerPacket;

    err = AudioUnitSetProperty(voice_unit[voice], kAudioUnitProperty_StreamFormat,
                               kAudioUnitScope_Input, 0, &streamFormat, sizeof(streamFormat));
    if (err)
    {
        perr("audio_backend_set_voice: Could not set properties for voice %d: %s\n", voice, snd_error(err));
        return false;
    }

    AURenderCallbackStruct callback_struct = { 0 };
    callback_struct.inputProc = audio_backend_render_callback;
    callback_struct.inputProcRefCon = &audio_backend_render_callback_data[voice];

    err = AudioUnitSetProperty(voice_unit[voice], kAudioUnitProperty_SetRenderCallback,
        kAudioUnitScope_Input, 0, &callback_struct, sizeof(callback_struct));
    if (err)
    {
        perr("audio_backend_set_voice: Could not set callback for voice %d: %s\n", voice, snd_error(err));
        return false;
    }

    voice_set_up[voice] = true;
    return true;
}

bool audio_backend_set_voice_callback(int voice, audio_backend_voice_callback_t callback, void* pdata,
                                      unsigned int frequency, unsigned int bits_per_sample, bool stereo)
{
    OSStatus err;
    AudioFormatFlags format_flags;
    AudioStreamBasicDescription streamFormat;
    int nb_channels = stereo ? 2 : 1;

    if ((voice<0) || (voice>(NB_VOICES - 1)))
    {
        perr("audio_backend_set_voice_callback: Voice ID must be in [0-%d]\n", (NB_VOICES - 1));
        return false;
    }
    if (voice_in_use[voice])
    {
        perr("audio_backend_set_voice_callback: Voice %d already in use\n", voice);
        return false;
    }

    switch (bits_per_sample)
    {
    case 8:
        format_flags = 0;   // Unsigned
        break;
    case 16:
        format_flags = kAudioFormatFlagIsSignedInteger;
        break;
    default:
        perr("audio_backend_set_voice_callback: Only 8 and 16 bits samples are supported\n");
        return false;
    }

    err = AudioUnitInitialize(voice_unit[voice]);
    if (err)
    {
        perr("audio_backend_set_voice_callback: Could not initialize voice instance: %s\n", snd_error(err));
        return false;
    }

    streamFormat.mFormatID = kAudioFormatLinearPCM;
    streamFormat.mFormatFlags = format_flags;
    streamFormat.mSampleRate = frequency;
    streamFormat.mBitsPerChannel = bits_per_sample;
    streamFormat.mChannelsPerFrame = nb_channels;
    streamFormat.mFramesPerPacket = 1;
    streamFormat.mBytesPerPacket = ((streamFormat.mBitsPerChannel + 7)/8) * nb_channels;
    streamFormat.mBytesPerFrame = streamFormat.mFramesPerPacket * streamFormat.mBytesPerPacket;

    err = AudioUnitSetProperty(voice_unit[voice], kAudioUnitProperty_StreamFormat,
                               kAudioUnitScope_Input, 0, &streamFormat, sizeof(streamFormat));
    if (err)
    {
        perr("audio_backend_set_voice_callback: Could not set properties for voice %d: %s\n", voice, snd_error(err));
        return false;
    }

    audio_backend_render_callback_data[voice].callback = callback;
    audio_backend_render_callback_data[voice].pdata = pdata;

    AURenderCallbackStruct callback_struct = { 0 };
    callback_struct.inputProc = audio_backend_render_callback;
    callback_struct.inputProcRefCon = &audio_backend_render_callback_data[voice];

    err = AudioUnitSetProperty(voice_unit[voice], kAudioUnitProperty_SetRenderCallback,
        kAudioUnitScope_Input, 0, &callback_struct, sizeof(callback_struct));
    if (err)
    {
        perr("audio_backend_set_voice_callback: Could not set callback for voice %d: %s\n", voice, snd_error(err));
        return false;
    }

    voice_set_up[voice] = true;
    return true;
}

bool audio_backend_release(void)
{
    int i;

    for (i=0; i<NB_VOICES; i++)
        AudioComponentInstanceDispose(voice_unit[i]);
    return true;
}

bool audio_backend_start_voice(int voice)
{

    if (!voice_set_up[voice])
    {
        perr("audio_backend_start_voice: Voice %d has not been initialized\n", voice);
        return false;
    }

    if (AudioOutputUnitStart(voice_unit[voice]))
    {
        perr("audio_backend_start_voice: Could not start voice instance %d.\n", voice);
        return false;
    }
    voice_in_use[voice] = true;
    return true;
}

bool audio_backend_stop_voice(int voice)
{
    if (!voice_in_use[voice])
        return false;

    AudioOutputUnitStop(voice_unit[voice]);
    voice_in_use[voice] = false;
    AURenderCallbackStruct callback_struct = { 0 };
    AudioUnitSetProperty(voice_unit[voice], kAudioUnitProperty_SetRenderCallback,
                         kAudioUnitScope_Input, 0, &callback_struct, sizeof(callback_struct));
    return true;
}

bool audio_backend_set_voice_volume(int voice, float volume)
{
    if (!voice_set_up[voice])
        return false;
    AudioUnitSetParameter(voice_unit[voice], kHALOutputParam_Volume, kAudioUnitScope_Global, 0, volume, 0);
    return true;
}

bool audio_backend_release_voice(int voice)
{
    if (!voice_set_up[voice])
        return false;
    audio_backend_stop_voice(voice);
    voice_set_up[voice] = false;
    return false;
}
