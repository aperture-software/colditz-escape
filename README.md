_Colditz Escape!_ is a rewritten game engine for the classic "Escape From Colditz" Amiga game.
In this game, you control a set of four prisoners of war trying to escape from the infamous Colditz Castle WWII prison.

The original game, created by Mike Halsall and John Law (with intro music by Bjørn Lynne), was published in 1991
by Digital Magic Software. This new version, which allows you to play the game on the PSP and Windows platforms,
has been reverse engineered from the original Amiga game engine and is released under a GPL v3 license.

For more information, see http://sites.google.com/site/colditzescape.

####FEATURES THAT MIGHT BE OF INTEREST TO YOU WITHIN THIS SOURCE:####

* Code for a simple directShow movie player for Windows (see win32/wmp)
* Code for an XAudio2 sound player, with double buffered streaming capabilities, for Windows (see win32/winXAudio)
* Multiplatform wrapper for XML configuration files (see conf.c/conf.h and Eschew)
* Code for onscreen message printouts on PSP (see psp/psp-printf.h)
* IFF image loader (graphics.c -> load_iff)
* RAW texture loader, with or without Alpha (graphics.c -> load_raw_rgb)
* OpenGL 2D rescale (graphics.c -> rescale_buffer)
* line interleaved / bitplane interleaves to RGBA texture conversion (line_interleaved_to_wGRAB and bitplane_to_wGRAB in graphics.c)
* Bytekiller 1.3 decompression algorithm (uncompress in low-level.c)
* PowerPacker decompression (ppDecrunch in low-level.c, courtesy of 'amigadepacker' by Heikki Orsila)
* Generic HQ2X GLSL OpenGL Zoom shader 