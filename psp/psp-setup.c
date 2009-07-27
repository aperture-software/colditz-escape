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
 *  psp-setup.c: PSP callbacks initialization
 *  ---------------------------------------------------------------------------
 */
#if defined(PSP)
#include <pspkerneltypes.h>
#include <pspuser.h>
#include "psp-setup.h"
#include "psp-printf.h"

// Define the module info section
PSP_MODULE_INFO("colditz", 0, 1, 1);

// Define the main thread's attribute value
//PSP_MAIN_THREAD_ATTR(THREAD_ATTR_USER | THREAD_ATTR_VFPU);
PSP_MAIN_THREAD_ATTR(0);

// Leave 256 KB for threads
PSP_HEAP_SIZE_KB(-256);


int exit_callback (int arg1, int arg2, void *common)
{
	sceKernelExitGame();
	return 0;
}


int update_thread (SceSize args, void *argp)
{
	int cbid = sceKernelCreateCallback("Exit Callback", exit_callback, NULL);
	sceKernelRegisterExitCallback(cbid);
	sceKernelSleepThreadCB();
	return 0;
}


void setup_callbacks (void)
{
	int id;

	if ((id = sceKernelCreateThread("update_thread", update_thread, 0x11, 0xFA0, 0, 0)) >= 0)
		sceKernelStartThread(id, 0, 0);
}


void back_to_kernel (void)
{
	sceKernelExitGame();
}
#endif