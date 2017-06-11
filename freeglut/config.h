/*
 * Basic configuration for FreeGLUT
 */

#define HAVE_STDINT_H
#define HAVE_STDBOOL_H
#define HAVE_INTTYPES_H
#define HAVE_SYS_TYPES_H
#define HAVE_USBHID_H

#define FREEGLUT_PRINT_ERRORS
#if defined(_DEBUG)
#define FREEGLUT_PRINT_WARNINGS
#endif

#if defined(__linux__)
#define HAVE_UNISTD_H
#define HAVE_SYS_TIME_H
#endif

#if defined(__APPLE__) && defined(__MACH__)
#define MAC_OSX_JOYSTICK_SUPPORT
#endif
