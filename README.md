Colditz Escape
==============

[![Windows/PSP Build status](https://ci.appveyor.com/api/projects/status/sjm9b9rqjlh1pbqa?svg=true)](https://ci.appveyor.com/project/aperture-software/colditz-escape)
[![Linux/OSX Build Status](https://travis-ci.org/aperture-software/colditz-escape.svg?branch=master)](https://travis-ci.org/aperture-software/colditz-escape)
[![Licence](https://img.shields.io/badge/license-GPLv3-blue.svg)](https://www.gnu.org/licenses/gpl-3.0.en.html)

![Colditz Escape Logo](docs/pics/icon-256.png)

_Colditz Escape_ is a rewritten game engine for the classic "Escape From Colditz" Amiga game.
In this game, you control a set of four prisoners of war trying to escape from the infamous Colditz Castle WWII prison.

The original game, created by _Mike Halsall_ and _John Law_ (with intro music by [Bjørn Lynne](http://www.lynnemusic.com/)),
was published in 1991 by Digital Magic Software.
This new version, which allows you to play the game on the PSP and Windows platforms, has been reverse engineered from the
original Amiga game engine and is released under a GPL v3 license.

For more information, see http://tiny.cc/colditz-escape.

Latest Automated Builds
-----------------------

Below are self contained game archives, based on the very latest version of the code. Just extract these files, and you're good to go!

* [__Windows__](https://ci.appveyor.com/api/projects/aperture-software/colditz-escape/artifacts/Colditz_Escape_Windows.7z?job=Environment%3A%20WINDOWS%3DRelease "Colditz Escape (Windows).7z")
* [PSP](https://ci.appveyor.com/api/projects/aperture-software/colditz-escape/artifacts/Colditz_Escape_PSP.7z?job=Environment%3A%20PSP%3DRelease "Colditz Escape (PSP).7z") (Extract to `PSP\GAME\` on Memory Stick)
* Linux and OSX: Unfortunately, Travis CI does not provide the ability to access the build artifacts from their servers... But you can still build from source (`make release`), by cloning this repository using `git`.

Features that might be of interest to you within this source
------------------------------------------------------------

* Code for a cross-platform abstracted sound player that includes Windows (Xaudio2), Linux (ALSA/PulseAudio) and PSP.
* [IFF](http://en.wikipedia.org/wiki/Interchange_File_Format) image loader ([`graphics.c`](graphics.c) &rarr; `load_iff()`)
* RAW texture loader, with or without Alpha ([`graphics.c`](graphics.c) &rarr; `load_raw_rgb()`)
* OpenGL 2D rescale ([`graphics.c`](graphics.c) &rarr; `rescale_buffer()`)
* line/bitplane interleaved interleaved to RGBA texture conversion ([`graphics.c`](graphics.c) &rarr; `line_interleaved_to_wGRAB()` and `bitplane_to_wGRAB()`)
* Bytekiller 1.3 decompression algorithm ([`low-level.c`](low-level.c) &rarr; `uncompress()`)
* PowerPacker decompression ([`low-level.c`](low-level.c)  &rarr; `ppDecrunch()`, courtesy of 'amigadepacker' by _Heikki Orsila_)
* Generic GLSL OpenGL zoom shaders ([HQ2X](Colditz%20Escape/SHADERS/HQ2X.GLSL), [HQ4X](Colditz%20Escape/SHADERS/HQ4X.GLSL),
  [5XBR](Colditz%20Escape/SHADERS/5XBR.GLSL), [SABR](Colditz%20Escape/SHADERS/SABR.GLSL) in [`graphics.c`](graphics.c) &rarr; `compile_shaders()`)
