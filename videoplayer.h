// videoplayer.h: wrapper for PSP (PMP) & Windows (DirectShow) video playout
//
////////////////////////////////////////////////////////////////////////////

#ifndef _VIDEOPLAYER_H
#define _VIDEOPLAYER_H

#ifdef __cplusplus
extern "C" {
#endif

// Call once on startup to initialize video playback
bool video_init();

// Playback file 
bool video_play(char* filename);

// Stop the playback and free all resources
void video_stop();

// Check if playback is still running
bool video_isplaying();

#ifdef __cplusplus
}
#endif

#endif
