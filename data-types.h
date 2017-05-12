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
 *  data-types.h: data types shortcuts
 *  ---------------------------------------------------------------------------
 */
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
