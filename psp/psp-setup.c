#if defined(PSP)
#include <pspkerneltypes.h>
#include <pspuser.h>
#include "psp-setup.h"

// Define the module info section
PSP_MODULE_INFO("colditz", 0, 1, 1);

// Define the main thread's attribute value
PSP_MAIN_THREAD_ATTR(THREAD_ATTR_USER | THREAD_ATTR_VFPU);

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