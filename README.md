# smb
Super Methane Bros for AmigaOS 4 / SDL2.

This SDL2 conversion is based on Spot's SDL1 source codes (see OS4Depot.net).

Supports 640x480 (scaled from the original 320x256 size) resolution, 2 players and joysticks.

To build, run "gmake" in amigaos4 directory. Supported command-line parameters:

"window": start in window mode instead of fullscreen.

"nosync": disable VSYNC. If game is too slow, try this.

"filter": smoothen the graphics

Example:

methane window filter
