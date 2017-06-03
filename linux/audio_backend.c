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
 *  audio_backend.c: Linux ALSA audio backend
 *  Mostly from http://www.alsa-project.org/alsa-doc/alsa-lib/_2test_2pcm_8c-example.html
 *  ---------------------------------------------------------------------------
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#define ALSA_PCM_NEW_HW_PARAMS_API
#define ALSA_PCM_NEW_SW_PARAMS_API
#include <alsa/asoundlib.h>
#include "audio_backend.h"

#define NB_VOICES 4

#if !defined(min)
#define min(a,b) (((a)<(b))?(a):(b))
#endif
#if !defined(max)
#define max(a,b) (((a)>(b))?(a):(b))
#endif

static snd_pcm_t *handle[NB_VOICES] = { 0 };          /* Try to open one PCM device for each voice */
static unsigned int buffer_time[NB_VOICES];           /* Ring buffer length, in us */
static unsigned int period_time[NB_VOICES];           /* Period time, in us */
static snd_pcm_sframes_t buffer_size[NB_VOICES];
static snd_pcm_sframes_t period_size[NB_VOICES];
//static snd_output_t *output = NULL;

static int snd_pcm_set_hwparams(int channel, unsigned int frequency, unsigned int nb_channels, snd_pcm_format_t format)
{
    int err, dir;
    unsigned int rrate = frequency;
    buffer_time[channel] = 500000;
    period_time[channel] = 100000;
    snd_pcm_uframes_t size;
    snd_pcm_hw_params_t *params;

    /* Choose all parameters */
    snd_pcm_hw_params_alloca(&params);
    err = snd_pcm_hw_params_any(handle[channel], params);
    if (err < 0)
    {
        fprintf(stderr, "Broken configuration for playback: no configurations available: %s\n", snd_strerror(err));
        return err;
    }
    /* Set hardware resampling */
    err = snd_pcm_hw_params_set_rate_resample(handle[channel], params, 1);
    if (err < 0)
    {
        fprintf(stderr, "Resampling setup failed for playback: %s\n", snd_strerror(err));
        return err;
    }
    /* Set the interleaved read/write format (even if our samples are mono) */
    err = snd_pcm_hw_params_set_access(handle[channel], params, SND_PCM_ACCESS_RW_INTERLEAVED);
    if (err < 0)
    {
        fprintf(stderr, "Access type not available for playback: %s\n", snd_strerror(err));
        return err;
    }
    /* Set the sample format */
    err = snd_pcm_hw_params_set_format(handle[channel], params, format);
    if (err < 0)
    {
        fprintf(stderr, "Sample format not available for playback: %s\n", snd_strerror(err));
        return err;
    }
    /* Set the count of channels */
    err = snd_pcm_hw_params_set_channels(handle[channel], params, nb_channels);
    if (err < 0)
    {
        fprintf(stderr, "Mono playback is not available: %s\n", snd_strerror(err));
        return err;
    }
    /* Set the stream rate */
    err = snd_pcm_hw_params_set_rate_near(handle[channel], params, &rrate, 0);
    if (err < 0)
    {
        fprintf(stderr, "Rate %iHz not available for playback: %s\n", frequency, snd_strerror(err));
        return err;
    }
    if (rrate != frequency)
    {
        fprintf(stderr, "Rate doesn't match (requested %iHz, got %iHz)\n", frequency, rrate);
        return -EINVAL;
    }
    /* Set the buffer time */
    err = snd_pcm_hw_params_set_buffer_time_near(handle[channel], params, &buffer_time[channel], &dir);
    if (err < 0)
    {
        fprintf(stderr, "Unable to set buffer time %u for playback on channel %i: %s\n", buffer_time[channel], channel, snd_strerror(err));
        return err;
    }
    err = snd_pcm_hw_params_get_buffer_size(params, &size);
    if (err < 0)
    {
        fprintf(stderr, "Unable to get buffer size for playback: %s\n", snd_strerror(err));
        return err;
    }
    buffer_size[channel] = size;
    /* Set the period time */
    err = snd_pcm_hw_params_set_period_time_near(handle[channel], params, &period_time[channel], &dir);
    if (err < 0)
    {
        fprintf(stderr, "Unable to set period time %u for playback on channel %i: %s\n", period_time[channel], channel, snd_strerror(err));
        return err;
    }
    err = snd_pcm_hw_params_get_period_size(params, &size, &dir);
    if (err < 0)
    {
        fprintf(stderr, "Unable to get period size for playback: %s\n", snd_strerror(err));
        return err;
    }
    period_size[channel] = size;
//printf("period_size[%d] = %d, buffer_size[%d] = %d\n", channel, size, channel, buffer_size[channel]);
    /* Write the parameters to device */
    err = snd_pcm_hw_params(handle[channel], params);
    if (err < 0)
    {
        fprintf(stderr, "Unable to set hw params for playback on channel %i: %s\n", channel, snd_strerror(err));
        return err;
    }
    return 0;
}

#if 0
static int snd_pcm_set_swparams(int channel)
{
    int err;
    snd_pcm_sw_params_t *params;

    /* Get the current swparams */
    snd_pcm_sw_params_alloca(&params);
    err = snd_pcm_sw_params_current(handle[channel], params);
    if (err < 0) {
        fprintf(stderr, "Unable to determine current swparams for playback: %s\n", snd_strerror(err));
        return err;
    }
    /* Start the transfer when the buffer is almost full: (buffer_size / avail_min) * avail_min */
    err = snd_pcm_sw_params_set_start_threshold(handle[channel], params,
        (buffer_size[channel] / period_size[channel]) * period_size[channel]);
    if (err < 0) {
        fprintf(stderr, "Unable to set start threshold mode for playback: %s\n", snd_strerror(err));
        return err;
    }
    /* Allow the transfer when at least period_size samples can be processed */
    err = snd_pcm_sw_params_set_avail_min(handle[channel], params, period_size[channel]);
    if (err < 0) {
        fprintf(stderr, "Unable to set avail min for playback: %s\n", snd_strerror(err));
        return err;
    }
    /* write the parameters to the playback device */
    err = snd_pcm_sw_params(handle[channel], params);
    if (err < 0) {
        fprintf(stderr, "Unable to set sw params for playback: %s\n", snd_strerror(err));
        return err;
    }
    return 0;
}
#endif

bool audio_backend_init(void)
{
    snd_ctl_t *ctl_pcm = NULL;
    snd_ctl_card_info_t *info;
    const char *name, *alsa_name = "default";
    unsigned int i, err;

    name = getenv("ALSA_NAME");
    if (name != NULL)
        alsa_name = name;

    snd_ctl_card_info_alloca(&info);
    if ((err = snd_ctl_open(&ctl_pcm, alsa_name, 0)) < 0)
        fprintf(stderr, "Could not open sound control: %s\n", snd_strerror(err));
    else if ((err = snd_ctl_card_info(ctl_pcm, info)) < 0)
    {
        printf("Could not obtain sound control info: %s\n", snd_strerror(err));
        snd_ctl_card_info_clear(info);
    }
    if (ctl_pcm != NULL)
        snd_ctl_close(ctl_pcm);

    name = snd_ctl_card_info_get_name(info);
    printf("Using %s's \"%s\" for audio output\n", name, alsa_name);

    for (i=0; i<NB_VOICES; i++)
    {
        if ((err = snd_pcm_open(&handle[i], alsa_name, SND_PCM_STREAM_PLAYBACK, SND_PCM_NONBLOCK)) < 0)
        {
            fprintf(stderr, "Could not open PCM channel %d: %s\n", i, snd_strerror(err));
            return false;
        }
        if ((err = snd_pcm_nonblock(handle[i], 0)) <0)
        {
            fprintf(stderr, "Could not set non-blocking mode: %s\n", snd_strerror(err));
            return false;
        }
    }
    return true;
}

bool audio_backend_release(void)
{
    unsigned i;
    for (i=0; i<NB_VOICES; i++)
    {
        if (handle[i] != NULL)
        {
            snd_pcm_drop(handle[i]);
            snd_pcm_close(handle[i]);
            handle[i] = NULL;
        }
    }
    return true;
}

bool audio_backend_set_voice(int voice, void* data, int size, unsigned int frequency,
                             unsigned int bits_per_sample, bool stereo)
{
    unsigned int err;
    snd_pcm_uframes_t i;
    snd_pcm_format_t format;

    switch (bits_per_sample)
    {
    case 8:
        format = SND_PCM_FORMAT_U8;
        break;
    case 16:
        format = SND_PCM_FORMAT_S16;
        break;
    default:
        fprintf(stderr, "Only 8 and 16 bits samples are supported\n");
        return false;
    }

    if ((snd_pcm_set_hwparams(voice, frequency, stereo?2:1, format) < 0))
        return false;
//    if (snd_pcm_set_swparams(play_channel) < 0)
//        return false;

    /* Write the sample data */
    for (i = 0; i<(snd_pcm_uframes_t)size; i += period_size[voice])
    {
        if ((err = snd_pcm_writei(handle[voice], (const void*)((uintptr_t)data + i),
            min(period_size[voice], ((snd_pcm_uframes_t)size) - i))) == (unsigned int)-EPIPE)
            snd_pcm_prepare(handle[voice]);
        else if (err < 0) {
            fprintf(stderr, "Could not write to PCM device. %s\n", snd_strerror(err));
            return false;
        }
    }
    //    snd_pcm_drain(pcm[play_channel]);
    return true;
}

bool audio_backend_set_voice_callback(int voice, AudioVoiceCallback_t callback, void* pdata,
                                      unsigned int frequency, unsigned int bits_per_sample, bool stereo)
{
    return false;
}

bool audio_backend_start_voice(int voice)
{
    return true;
}

bool audio_backend_stop_voice(int voice)
{
    return true;
}

bool audio_backend_set_voice_volume(int voice, float volume)
{
    return true;
}

bool audio_backend_release_voice(int voice)
{
    return true;
}
