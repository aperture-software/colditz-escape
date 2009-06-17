// videoplayer.c: wrapper for PSP (PMP) & Windows (DirectShow) video playout
//
////////////////////////////////////////////////////////////////////////////

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "data-types.h"
#include "videoplayer.h"

#if defined(WIN32)
#include "colditz.h"
#include "win32/wmp.h"

bool video_init() 
{
	return wmp_init(APPNAME);
}

bool video_play(char* filename)
{
	return wmp_play(filename);
}

void video_stop()
{
	wmp_stop();
}

bool video_isplaying()
{
	return wmp_isplaying();
}
#endif


#if defined(PSP)
#include "psp/pmp.h"

bool video_init() 
{
	return (pmp_init() == NULL);
}

bool video_play(char* filename)
{
	return (pmp_play(filename, 1, -1) == NULL);
}

void video_stop()
{
	pmp_stop();
}

// Check if playback is still running
bool video_isplaying()
{
	return pmp_isplaying();
}
#endif