Home
====

General Information
-------------------

__Colditz Escape__ is a rewritten game engine for the classic _"Escape From Colditz"_ Amiga game.
In this game, you control a set of four prisoners of war trying to escape from the infamous Colditz Castle WWII prison.

The original game, created by __Mike Halsall__ and __John Law__ (with intro music by [__Bjørn Lynne__](http://www.lynnemusic.com/)), was published in 1991 by Digital Magic Software.
This new version, which allows you to play the game on the PSP and Windows platforms, has been reverse engineered from the original Amiga game engine and is released under a GPL v3 license.

Please note that the latest version comes with all the data files required for the game - just extract the archive and play ;)

Screenshots
-----------

![Screenshot 1](pics/screenshot1.png) ![Screenshot 2](pics/screenshot2.png)
![Screenshot 3](pics/screenshot3.png) ![Screenshot 4](pics/screenshot4.png)

Installation
------------

 * Extract the 7z content into the relevant directory (`PSP/GAME/` on PSP, any location on other platforms). It should create a `Colditz Escape/` directory there
 * Launch the game (`COLDITZ.EXE` on Windows, `./colditz` on Linux).

Please see the [FAQ](FAQ.md) for additional information.

Controls
--------

The Windows controls use the numeric keypad as virtual joystick, along with <kbd>5</kbd> for joystick fire (similar to UAE controls). Make sure that <kbd>Numlock</kbd> is on.
The controls can be modified by editing the `config.xml` file which is created when the game is first run. You can also use a joystick for directions on Windows

For the PSP, the direction is with the analog pad.

Please see the [FAQ](FAQ.md) for additional information.

Downloads
---------

* [Windows binaries](https://github.com/aperture-software/colditz-escape/releases/download/v1.1/Colditz_Escape_Windows.7z "Colditz Escape (Windows).7z"), v1.1
* [Linux binaries (__x86_64__)](https://github.com/aperture-software/colditz-escape/releases/download/v1.1/Colditz_Escape_Linux.7z "Colditz Escape (Linux).7z"), v1.1
* [PSP binaries](https://github.com/aperture-software/colditz-escape/releases/download/v1.1/Colditz_Escape_PSP.7z "Colditz Escape (PSP).7z"), v1.1
* [Sourcecode](https://github.com/aperture-software/colditz-escape/archive/v1.1.tar.gz), v1.1 (Also accessible through [github](https://github.com/aperture-software/colditz-escape))
* The original game's [Reverse Engineered disassembly](files/Reverse%20Engineering%20Analysis.7z) (if you are actually deranged enough to want to look at this stuff)

History
-------

* v1.1   [2017.06.07]: Add Linux support (with major thanks to __picryott__) + overall codebase cleanup
* v1.0   [2017.05.24]: Add XBox360 controller support, VSYNC and more shaders on Windows + fix various issues
* v0.9.4 [2010.07.06]: Fixed memory leaks + various updates.
* v0.9.3 [2009.11.23]: Minor bugfixes + new 2x GPU shader on Windows. Data files are now included in release.
* v0.9.2 [2009.08.17]: First public release!
