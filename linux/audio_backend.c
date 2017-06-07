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
#include <pthread.h>
#include <signal.h>
#include <sys/eventfd.h>
#define ALSA_PCM_NEW_HW_PARAMS_API
#define ALSA_PCM_NEW_SW_PARAMS_API
#include <alsa/asoundlib.h>
#include "low-level.h"
#include "audio_backend.h"

#define ALSA_NAME "default"

// Max number of voices (aka "channels") we can handle
#define NB_VOICES           4
// We'll use double buffering to fill our data for callbacks
#define NB_BUFFERS          2
// We'll set our buffer size as a multiple of period_size
#define NB_PERIODS          4
#define BUFFER_SIZE(voice)  (NB_PERIODS * period_size[voice])

#if !defined(min)
#define min(a,b) (((a)<(b))?(a):(b))
#endif
#if !defined(max)
#define max(a,b) (((a)>(b))?(a):(b))
#endif

static snd_pcm_t *handle[NB_VOICES] = { 0 };        /* Try to open one PCM device for each voice */
static unsigned int buffer_time[NB_VOICES];         /* Ring buffer length, in us */
static unsigned int period_time[NB_VOICES];         /* Period time, in us */
static snd_pcm_sframes_t buffer_size[NB_VOICES];
static snd_pcm_sframes_t period_size[NB_VOICES];
static bool voice_in_use[NB_VOICES];
static bool voice_set_up[NB_VOICES];
static bool voice_is_callback[NB_VOICES];
static uint8_t *snd_pcm_buffer[NB_VOICES][NB_BUFFERS];
static pthread_t snd_pcm_poll_thread_id[NB_VOICES];

struct audio_backend_thread_private_data {
    int                             voice;
    audio_backend_voice_callback_t  callback;
    void*                           pdata;
    snd_pcm_uframes_t               sample_size;    /* Buffer size, in frames */
    uint16_t                        frame_size;
    int                             fd;
} snd_pcm_poll_thread_data[NB_VOICES];

static int snd_pcm_set_hwparams(int voice, unsigned int frequency, unsigned int nb_channels,
                                snd_pcm_format_t format)
{
    int err, dir;
    unsigned int rrate = frequency;
    buffer_time[voice] = 1000000;
    period_time[voice] = 250000;
    snd_pcm_uframes_t size;
    snd_pcm_hw_params_t *params;

    /* Choose all parameters */
    snd_pcm_hw_params_alloca(&params);
    err = snd_pcm_hw_params_any(handle[voice], params);
    if (err < 0)
    {
        perr("audio_backend: Broken configuration for playback - No configurations available: %s\n",
            snd_strerror(err));
        return err;
    }
    /* Set hardware resampling */
    err = snd_pcm_hw_params_set_rate_resample(handle[voice], params, 1);
    if (err < 0)
    {
        perr("audio_backend: Resampling setup failed for playback: %s\n", snd_strerror(err));
        return err;
    }
    /* Set the interleaved read/write format (even if our samples are mono) */
    err = snd_pcm_hw_params_set_access(handle[voice], params, SND_PCM_ACCESS_RW_INTERLEAVED);
    if (err < 0)
    {
        perr("audio_backend: Access type not available for playback: %s\n", snd_strerror(err));
        return err;
    }
    /* Set the sample format */
    err = snd_pcm_hw_params_set_format(handle[voice], params, format);
    if (err < 0)
    {
        perr("audio_backend: Sample format not available for playback: %s\n", snd_strerror(err));
        return err;
    }
    /* Set the count of channels */
    err = snd_pcm_hw_params_set_channels(handle[voice], params, nb_channels);
    if (err < 0)
    {
        perr("audio_backend: Playout of %d channel(s) not available: %s\n", nb_channels,
            snd_strerror(err));
        return err;
    }
    /* Set the stream rate */
    err = snd_pcm_hw_params_set_rate_near(handle[voice], params, &rrate, 0);
    if (err < 0)
    {
        perr("audio_backend: Rate %dHz not available for playback: %s\n", frequency,
            snd_strerror(err));
        return err;
    }
    if (rrate != frequency)
    {
        perr("audio_backend: Unable to match rate (requested %dHz, got %dHz)\n",
            frequency, rrate);
        return -EINVAL;
    }
    /* Set the buffer time */
    err = snd_pcm_hw_params_set_buffer_time_near(handle[voice], params, &buffer_time[voice], &dir);
    if (err < 0)
    {
        perr("audio_backend: Unable to set buffer time %u for voice %d: %s\n",
            buffer_time[voice], voice, snd_strerror(err));
        return err;
    }
    err = snd_pcm_hw_params_get_buffer_size(params, &size);
    if (err < 0)
    {
        perr("audio_backend: Unable to get buffer size for playback: %s\n", snd_strerror(err));
        return err;
    }
    buffer_size[voice] = size;
    /* Set the period time */
    err = snd_pcm_hw_params_set_period_time_near(handle[voice], params, &period_time[voice], &dir);
    if (err < 0)
    {
        perr("audio_backend: Unable to set period time %u for voice %d: %s\n",
            period_time[voice], voice, snd_strerror(err));
        return err;
    }
    err = snd_pcm_hw_params_get_period_size(params, &size, &dir);
    if (err < 0)
    {
        perr("audio_backend: Unable to get period size for playback: %s\n", snd_strerror(err));
        return err;
    }
    period_size[voice] = size;
    /* Write the parameters to device */
    err = snd_pcm_hw_params(handle[voice], params);
    if (err < 0)
    {
        perr("audio_backend: Unable to set hw params for voice %d: %s\n", voice, snd_strerror(err));
        return err;
    }
    return 0;
}

static int snd_pcm_set_swparams(int voice)
{
    int err;
    snd_pcm_sw_params_t *params;

    /* Get the current swparams */
    snd_pcm_sw_params_alloca(&params);
    err = snd_pcm_sw_params_current(handle[voice], params);
    if (err < 0) {
        perr("audio_backend: Unable to determine current swparams for playback: %s\n",
            snd_strerror(err));
        return err;
    }
    /* Start the transfer when the buffer is almost full */
    err = snd_pcm_sw_params_set_start_threshold(handle[voice], params,
        (buffer_size[voice] / period_size[voice]) * period_size[voice]);
    if (err < 0) {
        perr("audio_backend: Unable to set start threshold mode for voice %d: %s\n",
            voice, snd_strerror(err));
        return err;
    }
    /* Allow the transfer when at least period_size samples can be processed */
    err = snd_pcm_sw_params_set_avail_min(handle[voice], params, period_size[voice]);
    if (err < 0) {
        perr("audio_backend: Unable to set avail min for voice %d: %s\n",
            voice, snd_strerror(err));
        return err;
    }
    /* write the parameters to the playback device */
    err = snd_pcm_sw_params(handle[voice], params);
    if (err < 0) {
        perr("audio_backend: Unable to set sw params for voice %d: %s\n",
            voice, snd_strerror(err));
        return err;
    }
    return 0;
}

bool audio_backend_init(void)
{
    int voice, err;
    snd_ctl_t *ctl_pcm = NULL;
    snd_ctl_card_info_t *info;
    const char *name, *alsa_name = ALSA_NAME;

    memset(snd_pcm_buffer, 0, sizeof(snd_pcm_buffer));

    /* Enable override of default output device with env variable "ALSA_NAME" */
    name = getenv("ALSA_NAME");
    if (name != NULL)
        alsa_name = name;
    err = snd_ctl_open(&ctl_pcm, alsa_name, 0);
    if (err < 0)
    {
        perr("audio_backend_init: Could not open sound control: %s\n", snd_strerror(err));
        return false;
    }
    snd_ctl_card_info_alloca(&info);
    err = snd_ctl_card_info(ctl_pcm, info);
    if (err < 0)
    {
        perr("audio_backend_init: Could not obtain sound control info: %s\n", snd_strerror(err));
        snd_ctl_card_info_clear(info);
        return false;
    }
    if (ctl_pcm != NULL)
        snd_ctl_close(ctl_pcm);

    name = snd_ctl_card_info_get_name(info);
    printv("Using %s's \"%s\" for audio output\n", name, alsa_name);
    snd_ctl_card_info_clear(info);

    for (voice=0; voice<NB_VOICES; voice++)
    {
        err = snd_pcm_open(&handle[voice], alsa_name, SND_PCM_STREAM_PLAYBACK, SND_PCM_NONBLOCK);
        if (err < 0)
        {
            perr("audio_backend_init: Could not open PCM channel %d: %s\n", voice, snd_strerror(err));
            return false;
        }
        err = snd_pcm_nonblock(handle[voice], 0);
        if (err <0)
        {
            perr("audio_backend_init: Could not set non-blocking mode: %s\n", snd_strerror(err));
            return false;
        }
    }
    return true;
}

bool audio_backend_release(void)
{
    int voice;
    for (voice=0; voice<NB_VOICES; voice++)
    {
        if (handle[voice] != NULL)
        {
            snd_pcm_drop(handle[voice]);
            snd_pcm_close(handle[voice]);
            handle[voice] = NULL;
        }
    }
    return true;
}

bool audio_backend_set_voice(int voice, void* data, int size, unsigned int frequency,
                             unsigned int bits_per_sample, bool stereo)
{
    int err;
    snd_pcm_uframes_t i;
    snd_pcm_format_t format;

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

    switch (bits_per_sample)
    {
    case 8:
        format = SND_PCM_FORMAT_U8;
        break;
    case 16:
        format = SND_PCM_FORMAT_S16;
        break;
    default:
        perr("audio_backend_set_voice: Only 8 and 16 bits samples are supported\n");
        return false;
    }

    if ((snd_pcm_set_hwparams(voice, frequency, stereo?2:1, format) < 0))
        return false;
    if (snd_pcm_set_swparams(voice) < 0)
        return false;

    /* Write the sample data */
    for (i = 0; i<(snd_pcm_uframes_t)size; i += period_size[voice])
    {
        if ((err = snd_pcm_writei(handle[voice], (const void*)((uintptr_t)data + i),
            min(period_size[voice], ((snd_pcm_uframes_t)size) - i))) == (unsigned int)-EPIPE)
            snd_pcm_prepare(handle[voice]);
        else if (err < 0) {
            perr("audio_backend_set_voice: Could not write to PCM device. %s\n", snd_strerror(err));
            return false;
        }
    }

    voice_is_callback[voice] = false;
    voice_set_up[voice] = true;
    return true;
}

/*
 * Thanks to Lennart Poettering's utter inability to grasps the core concepts of
 * *PRACTICAL* Software Development (such the ability to balance developers needs
 * for "good enough", when it saves COUNTLESS PEOPLE A LOT OF *TIME* vs. his OWN
 * personal flawed vision of what computing paradygms should be RESTRICTED to)
 * we can't use the perfectly adequate ALSA async API but have to waste *OUR TIME*
 * shoving a poll() implementation into a thread...
 */
static void* audio_backend_poll_thread(void *arg)
{
    int err, count;
    uint16_t revents, period, buffer_nr = 0;
    snd_pcm_sframes_t avail;
    struct audio_backend_thread_private_data* data = (struct audio_backend_thread_private_data*)arg;
    struct pollfd* ufds = NULL;

    // We'll use an event fd to poll for requests to terminate the thread
    data->fd = eventfd(0, 0);
    if (data->fd < 0)
    {
        perr("audio_backend_poll_thread: Unable to create event fd: %s\n", strerror(errno));
        goto out;
    }

    count = snd_pcm_poll_descriptors_count(handle[data->voice]);
    if (count <= 0) {
        perr("audio_backend_poll_thread: Invalid poll descriptors count\n");
        goto out;
    }
    ufds = malloc(sizeof(struct pollfd) * count + 1);
    if (ufds == NULL) {
        perr("audio_backend_poll_thread: Could not allocate ufds!\n");
        goto out;
    }
    err = snd_pcm_poll_descriptors(handle[data->voice], ufds, count);
    if (err < 0) {
        perr("audio_backend_poll_thread: Unable to obtain poll descriptors for playback: %s\n",
            snd_strerror(err));
        goto out;
    }
    // Add our event fd to the list of pollable fds
    ufds[count].fd = data->fd;
    ufds[count].events = POLLIN;
    ufds[count].revents = 0;

    // Fill the initial buffer
    avail = BUFFER_SIZE(data->voice);
    data->callback(snd_pcm_buffer[data->voice][buffer_nr], avail, data->pdata);
    period = 0;
    while (1)
    {
        while (avail > 0)
        {
            // First task is to write the data we have available in our "write" buffer
            err = snd_pcm_writei(handle[data->voice],
                &snd_pcm_buffer[data->voice][buffer_nr][period*period_size[data->voice]*data->frame_size], period_size[data->voice]);
            if (err < 0)
            {
                perr("audio_backend_poll_thread: Write error: %s\n", snd_strerror(err));
                goto out;
            } else if (err != period_size[data->voice])
            {
                perr("audio_backend_poll_thread: Write error: written %d expected %d\n",
                    err, (int)period_size[data->voice]);
                goto out;
            }
            // With the sound engine now longer starving, we can take care of filling the *OTHER*
            // buffer with the same amount of data as the one we depleted from the "write" buffer
            data->callback(&snd_pcm_buffer[data->voice][(buffer_nr+1)%2]
                [period*period_size[data->voice]*data->frame_size],
                (unsigned long) period_size[data->voice], data->pdata);
            avail -= period_size[data->voice];
            // See if we need to switch buffers
            period++;
            if (period % NB_PERIODS == 0)
            {
                buffer_nr = (buffer_nr+1) % 2;
                period = 0;
            }
        }
        // No more data to write or fill => we can wait for poll
        poll(ufds, count+1, -1);
        snd_pcm_poll_descriptors_revents(handle[data->voice], ufds, count, &revents);
        if (revents & POLLERR)
        {
            perr("audio_backend_poll_thread: Poll error\n");
            goto out;
        }
        if (revents & POLLOUT)
        {
            // Make sure that we always have at least period_size available
            avail = snd_pcm_avail_update(handle[data->voice]);
            if (avail < period_size[data->voice]) {
                perr("audio_backend_poll_thread: Error, avail is smaller than period_size (%d vs %d)\n",
                    (int)avail, (int)period_size[data->voice]);
                goto out;
            }
        }
        // Thread stop requested
        if (ufds[count].revents & POLLIN)
            goto out;
    }
out:
    free(ufds);
    return NULL;
}

bool audio_backend_set_voice_callback(int voice, audio_backend_voice_callback_t callback,
                                      void* pdata, unsigned int frequency,
                                      unsigned int bits_per_sample, bool stereo)
{
    uint8_t* buffer = NULL;
    uint16_t frame_size = ((bits_per_sample+7)/8)*(stereo?2:1);
    snd_pcm_format_t format;

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
        format = SND_PCM_FORMAT_U8;
        break;
    case 16:
        format = SND_PCM_FORMAT_S16;
        break;
    default:
        perr("audio_backend_set_voice_callback: Only 8 and 16 bits samples are supported\n");
        return false;
    }

    if ((snd_pcm_set_hwparams(voice, frequency, stereo?2:1, format) < 0))
        return false;
    if (snd_pcm_set_swparams(voice) < 0)
        return false;

    buffer = (uint8_t*)malloc(NB_BUFFERS*BUFFER_SIZE(voice)*frame_size);
    if (buffer == NULL)
    {
        perr("audio_backend_set_voice_callback: Could not allocate buffers\n");
        return false;
    }
    snd_pcm_buffer[voice][0] = buffer;
    snd_pcm_buffer[voice][1] = buffer + BUFFER_SIZE(voice)*frame_size;

    snd_pcm_poll_thread_data[voice].voice = voice;
    snd_pcm_poll_thread_data[voice].callback = callback;
    snd_pcm_poll_thread_data[voice].pdata = pdata;
    snd_pcm_poll_thread_data[voice].frame_size = frame_size;
    snd_pcm_poll_thread_data[voice].sample_size = BUFFER_SIZE(voice);
    snd_pcm_poll_thread_data[voice].fd = -1;

    voice_is_callback[voice] = true;
    voice_set_up[voice] = true;
    return true;
}

bool audio_backend_start_voice(int voice)
{
    int err;

    if (!voice_set_up[voice])
    {
        perr("audio_backend_start_voice: Voice %d has not been initialized\n", voice);
        return false;
    }
    if (snd_pcm_state(handle[voice]) != SND_PCM_STATE_PREPARED)
    {
        perr("audio_backend_start_voice: Voice %d is not in prepared state\n", voice);
        return false;
    }
    if (voice_is_callback[voice])
    {
        /* Create the polling thread */
        err = pthread_create(&snd_pcm_poll_thread_id[voice], NULL, &audio_backend_poll_thread,
            (void*)&snd_pcm_poll_thread_data[voice]);
        if (err != 0)
        {
            perr("audio_backend_start_voice: Cannot create thread for voice %d: %s\n",
                voice, strerror(err));
            return false;
        }
    }

    voice_in_use[voice] = true;
    return (snd_pcm_start(handle[voice]) >= 0);
}

bool audio_backend_stop_voice(int voice)
{
    uint64_t val = 1;

    if (!voice_in_use[voice])
        return false;
    if (voice_is_callback[voice])
    {
        if (snd_pcm_poll_thread_data[voice].fd < 0)
        {
            perr("audio_backend_stop_voice: Can not kill thread for voice %d\n", voice);
        }
        else
        {
            IGNORE_RETVAL(write(snd_pcm_poll_thread_data[voice].fd, &val, sizeof(val)));
            pthread_join(snd_pcm_poll_thread_id[voice], NULL);
        }
    }
    free(snd_pcm_buffer[voice][0]);
    snd_pcm_buffer[voice][0] = NULL;
    voice_in_use[voice] = false;
    return (snd_pcm_drop(handle[voice]) >= 0);
}

bool audio_backend_set_voice_volume(int voice, float volume)
{
    // Not implemented on Linux
    return true;
}

bool audio_backend_release_voice(int voice)
{
    if (!voice_set_up[voice])
        return false;
    audio_backend_stop_voice(voice);
    return (snd_pcm_hw_free(handle[voice]) >= 0);
}
