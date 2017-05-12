/*
 *  Colditz Escape - Rewritten Engine for "Escape From Colditz"
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
 *  psp-printf.h: Onscreen STDOUT printf for PSP
 *  ---------------------------------------------------------------------------
 */
#pragma once

#define PSP_ONSCREEN_STDOUT

#if defined(PSP_ONSCREEN_STDOUT)
extern int game_suspended;
extern unsigned char last_key_used;
extern void (*work_around_stupid_linkers_glut_idle_suspended)(void);
extern void (*work_around_stupid_linkers_glutIdleFunc)(void (*func)(void));

#define EXIT_STRING "####################################################################" \
					"#                                                                  #" \
					"#      PROGRAM NOTIFICATION - PRESS ANY KEY TO RETURN TO GAME      #" \
					"#                                                                  #" \
					"####################################################################\n"
#define printf(...) {												\
	if (game_suspended == 0) {										\
		game_suspended = -1; last_key_used = 0;						\
		work_around_stupid_linkers_glutIdleFunc(work_around_stupid_linkers_glut_idle_suspended);\
		pspDebugScreenInit();										\
		pspDebugScreenPrintf(EXIT_STRING);}							\
	pspDebugScreenPrintf(__VA_ARGS__);								\
}
// NB: the default behaviour is to continue adding non-fatal messages until 
// the idle_suspended loop kicks in
#endif
