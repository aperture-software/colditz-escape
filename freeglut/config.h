/*
 * Basic configuration for FreeGLUT
 */

#define HAVE_STDINT_H
#define HAVE_STDBOOL_H
#define HAVE_INTTYPES_H
#define HAVE_SYS_TYPES_H

#if defined(__linux__)
#define HAVE_UNISTD_H
#define HAVE_SYS_TIME_H
#endif
