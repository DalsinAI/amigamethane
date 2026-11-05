Super Methane Brothers for AmigaOS 3.x
======================================

A port of Super Methane Brothers (Mark Page's 1993 Amiga platformer, in
the GPL conversion he released as Methane 1.4.8), from capehill's SDL 2
version for AmigaOS 4 (https://github.com/capehill/smb, commit
9f1bea67044bab88028307a81dc2614119d8ce12, itself from Spot's SDL 1 port),
for AmigaOS 3.2 on a 68040 or 68060 with an FPU and an RTG screen, on
GFOpenGPU's SDL 2.

Why this source: Mark Page's own repository (https://github.com/rombust/Methane)
is now a C++20 program on its own Vulkan, X11 and ALSA layer ("TinyClan"),
with no SDL; capehill's tree is the same game (the same source/ and the
same data) on SDL 2, and already big-endian clean.

Status: see "Testing" at the end.


What you need
-------------

- A 68040 or 68060 with an FPU (not an LC040 or EC040), AmigaOS 3.2, an RTG
  card (or an AmiCradle instance).
- GFOpenGPU: gfopengpu.library and LIBS:GFOpenGPU/SDL2.module and
  SDL2_mixer.module (the tunes are ProTracker modules, played by its
  libxmp), installed by OpenUp.
- About 3 MB of disk. The graphics, maps, tunes and effects are inside the
  program: there are no data files to find.


How to play
-----------

Run Install-Methane from its icon (it asks where the Methane drawer goes,
checks the CPU, the FPU and GFOpenGPU's modules, and keeps your high scores
on an update), or copy the Methane drawer anywhere. Double-click Methane,
or from a Shell:

    cd Methane
    Methane [-scale 1..4] [-fullscreen] [-filter] [-nomusic] [-nosound]

Puff and Blow carry methane gas guns. Tap fire to shoot gas at a bad guy,
hold fire to suck the trapped bad guy into the gun, let go to throw him
at a wall.

- Title: fire starts (player 1's fire for one player, player 2's for two).
- Then type a name of up to four letters and press Return (cursor left and
  right move along it).
- Player 1: cursor keys, fire is Ctrl, Space or Right Alt (or joystick 1).
- Player 2: S (up), X (down), Z (left), C (right), fire is Left Shift (or
  joystick 2).
- Tab swaps Puff's and Blow's pictures; P pauses.
- Esc or Right Amiga+Q quits, from anywhere, and saves the high scores.

The window is 640x512 (the game's 320x256 drawn twice the size); -scale 1
is the game's own size. -fullscreen opens a screen of that size rather
than a window on the Workbench. -filter smooths the scaling. -nomusic
keeps the effects without the tunes; -nosound runs with no audio at all.

High scores are kept in the drawer, in User/Methane.HiScores (made when
the game first quits).


Building
--------

GFOpenAmigaGCC 16.2.0-open6 (the os32-gcc16-open6 stove, which carries the
NDK and GFOpenGPU's SDL 2 kit):

    make -f amiga/Makefile.amigaos -j2 STOVE=/path/to/os32-gcc16-open6

The default STOVE is ~/AmiCradle/stoves/os32-gcc16-open6. Output:
build/amigaos/Methane/ (Methane, ReadMe, ReadMe-methane, COPYING.txt,
Licences.txt, User/, with GlowIcons), build/amigaos/Methane.info and
Install-Methane with its icon beside them. The icons come from
amiga/icons/icons.json and the Installer script from
amiga/installer/installer.json (amicradle's scripts/port_icons.py and
port_installer.py).


What was changed
----------------

- amiga/amiga_main.cpp (from capehill's source/amigaos4/doc.cpp): the SDL 2
  front end. The game's 8-bit 320x256 picture is turned into a 32-bit XRGB
  surface each frame and handed to the window's renderer as one streaming
  texture, which scales it. capehill drew flat out between game steps
  (vsync paced it on AmigaOS 4) and stepped the game 30 times a second;
  here the loop makes one game step and one picture every 1/25 s, the rate
  Mark Page's own current version keeps (its GameTime(25, 25)), and sleeps
  (SDL_Delay) in between. A key tapped between two steps still counts.
  Esc, Right Amiga+Q and the window's close gadget quit; P pauses; the
  name on the high score screen is typed (capehill always sent Return, so
  every name stayed "????"); Ctrl and Space are player 1's fire too (the
  original's key was Ctrl). High scores in
  PROGDIR:User/Methane.HiScores (amiga/amiga_dos.c makes the drawer).
  Unknown arguments are reported, not fatal.
- amiga/amiga_sound.cpp: the tunes and effects on SDL2_mixer in place of
  MikMod (not in the kit). The six tunes are the game's own ProTracker
  modules, loaded from memory with Mix_LoadMUSType_RW(MUS_MOD); the effects
  are its signed 8-bit samples made into 16-bit stereo chunks at 22050 Hz
  from each sample's own rate (the xylophone's varying rates are cached as
  they are asked for), panned and weighted as the MikMod driver did.
- amiga/doc.h: capehill's doc.h with this driver.
- source/data/snddata.cpp: METHANE_SDLMIXER builds the samples raw (as for
  RISC OS), without MikMod's WAV headers.
- amiga/amiga_screens.c (from the Seven Kingdoms and Abbaye ports):
  SDL2.module leaves empty "SDL" screens after a full-screen window; after
  SDL_Quit() they are closed, or sent behind the Workbench.
- amiga/stack.c: 256 KB of stack (the icon asks for it too) and a $VER
  string.
- amiga/Makefile.amigaos: capehill's file list; -fsigned-char as his
  makefile; METHANE_OLD_CPP_NEW (a plain new: upstream's "informative" new
  wants a ::size_t this libstdc++'s <new> doesn't give); -Wno-narrowing for
  upstream's 2007 tables; -lpthread for libstdc++'s exception support.

Big-endian: the maps, sprite positions and collision boxes are Amiga data
already, read byte by byte (CMap::Amiga2PC builds each word from its two
bytes, so it is right on either order); the graphics are bytes; the
samples bytes; the tunes ProTracker files. The high score file is text.
No change was needed.


Known limits
------------

- All drawing is the 68k's: the 320x256 conversion each frame, then the
  renderer's scaling. On a slow 68040 the game runs slower than 25 steps a
  second rather than skipping frames.
- Two players share one keyboard; the Amiga's keyboard may not report
  every three-key combination.
- The game's "next level" cheat key of the PC versions isn't mapped.
