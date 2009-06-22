#ifndef _PSP_PRINTF_H
#define _PSP_PRINTF 1

//#define PSP_ONSCREEN_STDOUT					1

#if defined(PSP_ONSCREEN_STDOUT)
extern int game_suspended;
extern unsigned char last_key_used;
extern void (*work_around_stupid_linkers_glut_idle_suspended)(void);
extern void (*work_around_stupid_linkers_glutIdleFunc)(void (*func)(void));
#define EXIT_STRING "####################################################################" \
					"#                                                                  #" \
					"#           PROGRAM NOTIFICATION - PRESS ANY KEY TO EXIT           #" \
					"#                                                                  #" \
					"####################################################################\n"
#define printf(...)	{									\
	if (game_suspended == 0) {							\
		game_suspended = -1; last_key_used = 0;			\
		work_around_stupid_linkers_glutIdleFunc(work_around_stupid_linkers_glut_idle_suspended);		\
		pspDebugScreenInit();							\
		pspDebugScreenPrintf(EXIT_STRING);}				\
	pspDebugScreenPrintf(__VA_ARGS__);					}
#endif

#endif