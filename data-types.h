#pragma once

#ifdef	__cplusplus
extern "C" {
#endif

// On the PSP, these are defined in pspkernel.h
#if !defined(PSP)
#ifndef u8
#define u8 unsigned char
#endif
#ifndef s8
#define s8 signed char
#endif
#ifndef u16
#define u16 unsigned short
#endif
#ifndef s16
#define s16 signed short
#endif
#ifndef u32
#define u32 unsigned long
#endif
#ifndef s32
#define s32 signed long
#endif
#ifndef u64
#define u64 unsigned long long
#endif
#ifndef s64
#define s64 signed long long
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

