Colditz Escape
==============

[![Windows Build Status](https://img.shields.io/github/actions/workflow/status/aperture-software/colditz-escape/Windows.yml?branch=master&style=flat-square&label=Windows%20Build)](https://github.com/aperture-software/colditz-escape/actions/workflows/Windows.yml)
[![Linux Build Status](https://img.shields.io/github/actions/workflow/status/aperture-software/colditz-escape/Linux.yml?branch=master&style=flat-square&label=Linux%20Build)](https://github.com/aperture-software/colditz-escape/actions/workflows/Linux.yml)
[![MacOS Build Status](https://img.shields.io/github/actions/workflow/status/aperture-software/colditz-escape/MacOS.yml?branch=master&style=flat-square&label=MacOS%20Build)](https://github.com/aperture-software/colditz-escape/actions/workflows/MacOS.yml)
[![Licence](https://img.shields.io/badge/license-GPLv3-blue.svg)](https://www.gnu.org/licenses/gpl-3.0.en.html)

![Colditz Escape Logo](docs/pics/icon-256.png)

_Colditz Escape_ is a rewritten game engine for the classic "Escape From Colditz" Amiga game.
In this game, you control a set of four prisoners of war trying to escape from the infamous Colditz Castle WWII prison.

The original game, created by _Mike Halsall_ and _John Law_ (with intro music by [Bjørn Lynne](https://lynnemusic.com/)),
was published in 1991 by Digital Magic Software.
This new version, which allows you to play the game on Windows, Linux, macOS and PSP platforms, has been reverse engineered from the
original Amiga game engine and is released under a GPL v3 license.

For more information, see [the documentation](docs/index.md).

Latest Builds
-------------

Below are self contained game archives, based on the latest version of the code. Just extract these files, and you're good to go!

* [__Windows__](https://github.com/aperture-software/colditz-escape/releases/download/v1.2/Colditz_Escape_Windows.7z)
* [__Linux__](https://github.com/aperture-software/colditz-escape/releases/download/v1.2/Colditz_Escape_Linux.7z)
* [__MacOS__](https://github.com/aperture-software/colditz-escape/releases/download/v1.2/Colditz_Escape_OSX.7z)
* [__PSP__](https://github.com/aperture-software/colditz-escape/releases/download/v1.2/Colditz_Escape_PSP.7z) (Extract to `PSP\GAME\` on Memory Stick)

Features that might be of interest to you within this source
------------------------------------------------------------

* Automated builds using [Travis CI](.travis.yml) and [AppVeyor](appveyor.yml), for Windows, Linux, OSX and PSP, including release artifact deployment to github.
* Code for a cross-platform abstracted sound player that supports Windows (XAudio2), Linux (ALSA/PulseAudio), OSX (Core Audio) and PSP.
* [IFF](https://en.wikipedia.org/wiki/Interchange_File_Format) image loader ([`graphics.c`](graphics.c) &rarr; `load_iff()`)
* RAW texture loader, with or without Alpha ([`graphics.c`](graphics.c) &rarr; `load_raw_rgb()`)
* OpenGL 2D rescale ([`graphics.c`](graphics.c) &rarr; `rescale_buffer()`)
* line/bitplane interleaved interleaved to RGBA texture conversion ([`graphics.c`](graphics.c) &rarr; `line_interleaved_to_wGRAB()` and `bitplane_to_wGRAB()`)
* Bytekiller 1.3 decompression algorithm ([`low-level.c`](low-level.c) &rarr; `uncompress()`)
* PowerPacker decompression ([`low-level.c`](low-level.c)  &rarr; `ppDecrunch()`, courtesy of 'amigadepacker' by _Heikki Orsila_)
* Generic GLSL OpenGL zoom shaders ([HQ2X](Colditz%20Escape/SHADERS/HQ2X.GLSL), [HQ4X](Colditz%20Escape/SHADERS/HQ4X.GLSL),
  [5XBR](Colditz%20Escape/SHADERS/5XBR.GLSL), [SABR](Colditz%20Escape/SHADERS/SABR.GLSL) in [`graphics.c`](graphics.c) &rarr; `compile_shaders()`)
