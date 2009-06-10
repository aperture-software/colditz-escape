#ifndef _PSP_PRINTF_H
#define _PSP_PRINTF 1

extern int psp_printf_mode;
#define PSP_ONSCREEN_STDOUT					1
#define EXIT_STRING "           -----     PRESS X TO EXIT THIS SCREEN     -----\n\n"
#define printf(...)	{						\
	if (psp_printf_mode == 0) {				\
		psp_printf_mode = -1;				\
		pspDebugScreenInit();				\
		pspDebugScreenPrintf(EXIT_STRING);}	\
	pspDebugScreenPrintf(__VA_ARGS__);		}

#endif