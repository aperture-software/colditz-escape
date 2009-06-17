/*
PMP Mod - mini lib
Copyright (C) 2006 jonny
Copyright (C) 2007 Raphael <raphael@fx-world.org>

Homepage: http://jonny.leffe.dnsalias.com
          http://wordpress.fx-world.org
E-mail:   jonny@leffe.dnsalias.com

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

/*
this lib should be used to play .pmp files
*/


#ifndef pmp_h
#define pmp_h

#ifdef __cplusplus
extern "C" {
#endif

// Call once on startup to initialize pmp playback (loads the codecs)
char* pmp_init();

// play file s and decode in pixelformat 'format' (see pspmpeg.h for modes, possible are 4444,5650,5551 and 8888)
// if show is set to 1 the display of the video will be handled by the output thread
// else the caller has to display the video itself by querying the frames with pmp_get_frame and somehow getting them
// displayed on screen
char *pmp_play(char *s, int show, int format);

// return pointer to current frame, also set frame format, width, height and buffer width
void* pmp_get_frame(int* format, int* width, int* height, int* vbw );

// Stop the playback and free all resources
void pmp_stop();

// Check if playback is still running
int pmp_isplaying();

#ifdef __cplusplus
}
#endif

#endif
