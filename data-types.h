#ifndef _DATA_TYPES_H
#define _DATA_TYPES_H

#ifdef	__cplusplus
extern "C" {
#endif


// On the PSP, these are defined in psp-types.h
#if !defined(PSP)
#ifndef u8
#define u8 unsigned char
#endif
#ifndef u16
#define u16 unsigned short
#endif
#ifndef s16
#define s16 short
#endif
#ifndef u32
#define u32 unsigned long
#endif
#ifndef s32
#define s32 long
#endif
#ifndef u64
#define u64 unsigned long long
#endif
#ifndef uint
#define uint unsigned int
#endif
#endif 

#if !defined(bool)
#define bool int
#endif
#if !defined(false)
#define false ((bool)0)
#endif
#if !defined(true)
#define true ((bool)(!false))
#endif

#if !defined(min)
#define min(a,b) (((a)<(b))?(a):(b))
#endif
#if !defined(max)
#define max(a,b) (((a)>(b))?(a):(b))
#endif


#ifdef	__cplusplus
}
#endif

#endif 