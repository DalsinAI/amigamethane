/* 256 KB of stack for SDL 2 and the game (a Shell gives 4 KB), and the
 * version string. The AmiCradle Team. GPL 2 or later, like the game. */
unsigned long __stack = 256 * 1024;
static const char version[] __attribute__((used)) = "$VER: Methane 1.4.8 Amiga (05.11.2026)";
