/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 * Program WebSite: http://methane.sourceforge.net/index.html              *
 * Project Email: rombust@postmaster.co.uk                                 *
 *                                                                         *
 ***************************************************************************/

/*
 * Super Methane Brothers for AmigaOS 3.x on OpenGPU's SDL 2: capehill's
 * source/amigaos4/doc.cpp (the SDL 2 front end), changed for a 68040:
 * - a window (or -fullscreen screen) of 2x the game's 320x256, the game's
 *   8-bit picture turned into one streaming XRGB texture each frame;
 * - one game step and one picture each 1/30 s (upstream drew flat out
 *   between steps), waiting with SDL_Delay in between;
 * - music and effects on SDL2_mixer (amiga/amiga_sound.cpp), not MikMod;
 * - Esc or Right Amiga+Q quits, P pauses; the high score name is typed
 *   (upstream always sent Return);
 * - high scores in PROGDIR:User/Methane.HiScores;
 * - the empty screens SDL2.module leaves are closed (amiga_screens.c).
 * Changes: the AmiCradle Team, GPL 2 or later like the game.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "SDL.h"
#include "doc.h"
#include "amiga_sound.h"

extern "C" {
void amiga_screens_snapshot(void);
void amiga_screens_close_leftovers(void);
int amiga_make_dir(const char *name);
}

#define LOGIC_HZ 30

static CMethDoc Game;

static SDL_Surface *SdlScreen = 0;
static SDL_Window *SdlWindow = 0;
static SDL_Renderer *SdlRenderer = 0;
static SDL_Texture *SdlTexture = 0;

static char TheScreen[SCR_WIDTH * SCR_HEIGHT];

static bool fullscreen = false;
static bool filter = false;
static bool music = true;
static bool effects = true;
static int scale = 2;
static bool paused = false;

static const char UserDir[] = "PROGDIR:User";
static const char HighScoreFileName[] = "PROGDIR:User/Methane.HiScores";
#define HighScoreLoadBufferSize (MAX_HISCORES * 64)

static SDL_Joystick *joystick1;
static SDL_Joystick *joystick2;

struct KeyBinding {
	SDL_Scancode sc;
	char *data;
};

enum EKey {
	EKeyUp = 0,
	EKeyDown,
	EKeyLeft,
	EKeyRight,
	EKeyFire
};

#define NUM_OF_ACTIONS 5

static KeyBinding keys[2 * NUM_OF_ACTIONS];

static void usage(void)
{
	puts("Methane [-scale 1..4] [-fullscreen] [-filter] [-nomusic] [-nosound]");
}

static void read_args(int argc, char **argv)
{
	int i;
	for (i = 1; i < argc; i++)
	{
		const char *a = argv[i];
		if (*a == '-') a++;
		if (!strcmp(a, "fullscreen")) fullscreen = true;
		else if (!strcmp(a, "window")) fullscreen = false;
		else if (!strcmp(a, "filter")) filter = true;
		else if (!strcmp(a, "nomusic")) music = false;
		else if (!strcmp(a, "nosound")) music = effects = false;
		else if (!strcmp(a, "scale") && i + 1 < argc)
		{
			scale = atoi(argv[++i]);
			if (scale < 1) scale = 1;
			if (scale > 4) scale = 4;
		}
		else
		{
			fprintf(stderr, "Unknown argument '%s'\n", argv[i]);
			usage();
		}
	}
}

static SDL_Joystick *open_joy(int index)
{
	SDL_Joystick *joy = SDL_JoystickOpen(index);
	if (joy)
		printf("Joystick %d: %s\n", index, SDL_JoystickNameForIndex(index));
	return joy;
}

static void open_joysticks(void)
{
	int num = SDL_NumJoysticks();
	if (num > 0) joystick1 = open_joy(0);
	if (num > 1) joystick2 = open_joy(1);
}

static void close_joysticks(void)
{
	if (joystick1) SDL_JoystickClose(joystick1);
	if (joystick2) SDL_JoystickClose(joystick2);
	joystick1 = joystick2 = 0;
}

static void print_info(void)
{
	puts("Super Methane Brothers. The GNU General Public License V2 applies to this game.");
	puts("Player 1: cursor keys + Ctrl, Space or Right Alt (or joystick 1)");
	puts("Player 2: S Z X C + Left Shift (or joystick 2)");
	puts("Tab: Puff/Blow graphic. P: pause. Esc or Right Amiga+Q: quit (saves high scores)");
}

static bool alloc_resources(void)
{
	SdlScreen = SDL_CreateRGBSurfaceWithFormat(0, SCR_WIDTH, SCR_HEIGHT, 32,
						   SDL_PIXELFORMAT_RGB888);
	if (!SdlScreen)
	{
		fprintf(stderr, "Couldn't create surface: %s\n", SDL_GetError());
		return false;
	}

	SdlWindow = SDL_CreateWindow("Super Methane Brothers",
		SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
		SCR_WIDTH * scale, SCR_HEIGHT * scale,
		fullscreen ? SDL_WINDOW_FULLSCREEN : 0);
	if (!SdlWindow)
	{
		fprintf(stderr, "Couldn't create window: %s\n", SDL_GetError());
		return false;
	}

	SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, filter ? "linear" : "nearest");

	SdlRenderer = SDL_CreateRenderer(SdlWindow, -1, 0);
	if (!SdlRenderer)
	{
		fprintf(stderr, "Couldn't create renderer: %s\n", SDL_GetError());
		return false;
	}
	SDL_RenderSetLogicalSize(SdlRenderer, SCR_WIDTH, SCR_HEIGHT);

	SdlTexture = SDL_CreateTexture(SdlRenderer, SDL_PIXELFORMAT_RGB888,
		SDL_TEXTUREACCESS_STREAMING, SCR_WIDTH, SCR_HEIGHT);
	if (!SdlTexture)
	{
		fprintf(stderr, "Couldn't create texture: %s\n", SDL_GetError());
		return false;
	}
	return true;
}

static void free_resources(void)
{
	if (SdlTexture) SDL_DestroyTexture(SdlTexture);
	if (SdlRenderer) SDL_DestroyRenderer(SdlRenderer);
	if (SdlWindow) SDL_DestroyWindow(SdlWindow);
	if (SdlScreen) SDL_FreeSurface(SdlScreen);
	SdlTexture = 0;
	SdlRenderer = 0;
	SdlWindow = 0;
	SdlScreen = 0;
}

static void handle_joystick(SDL_Joystick *joy, JOYSTICK *jptr)
{
	const Sint16 AXIS_THRESHOLD = 16000;

	if (!joy)
		return;
	if (SDL_JoystickGetButton(joy, 0)) jptr->fire = 1;

	Sint16 xAxis = SDL_JoystickGetAxis(joy, 0);
	Sint16 yAxis = SDL_JoystickGetAxis(joy, 1);
	if (xAxis < -AXIS_THRESHOLD) jptr->left = 1;
	else if (xAxis > AXIS_THRESHOLD) jptr->right = 1;
	if (yAxis < -AXIS_THRESHOLD) jptr->up = 1;
	else if (yAxis > AXIS_THRESHOLD) jptr->down = 1;

	if (SDL_JoystickNumHats(joy) > 0)
	{
		Uint8 hat = SDL_JoystickGetHat(joy, 0);
		if (hat & SDL_HAT_UP) jptr->up = 1;
		if (hat & SDL_HAT_DOWN) jptr->down = 1;
		if (hat & SDL_HAT_LEFT) jptr->left = 1;
		if (hat & SDL_HAT_RIGHT) jptr->right = 1;
	}
}

/* A key for the high score name: a letter, digit or space, Return, or
 * cursor left and right (sent as a key with the stick held, as upstream's
 * EditName() reads them). */
static char name_key(const SDL_Keysym &k)
{
	SDL_Keycode c = k.sym;
	if (c >= 'a' && c <= 'z') return (char)(c - 'a' + 'A');
	if (c >= '0' && c <= '9') return (char)c;
	if (c == ' ') return ' ';
	if (c == SDLK_RETURN || c == SDLK_KP_ENTER) return 13;
	if (c == SDLK_LEFT || c == SDLK_RIGHT) return 1;
	return 0;
}

static bool handle_input(void)
{
	bool running = true;
	SDL_Event e;
	JOYSTICK *jptr1 = &Game.m_GameTarget.m_Joy1;
	char key = 0;

	while (SDL_PollEvent(&e))
	{
		if (e.type == SDL_QUIT)
			running = false;
		else if (e.type == SDL_KEYDOWN && !e.key.repeat)
		{
			SDL_Keycode sym = e.key.keysym.sym;
			if (sym == SDLK_ESCAPE)
				running = false;
			else if (sym == SDLK_q && (e.key.keysym.mod & KMOD_RGUI))
				running = false;
			else if (sym == SDLK_TAB)
				Game.m_GameTarget.m_Game.TogglePuffBlow();
			else if (sym == SDLK_p)
				paused = !paused;
			if (!key)
				key = name_key(e.key.keysym);
		}
	}

	const Uint8 *state = SDL_GetKeyboardState(NULL);
	for (size_t i = 0; i < sizeof(keys) / sizeof(keys[0]); i++)
		*keys[i].data = state[keys[i].sc];
	if (state[SDL_SCANCODE_SPACE] || state[SDL_SCANCODE_LCTRL] || state[SDL_SCANCODE_RCTRL])
		jptr1->fire = 1;

	handle_joystick(joystick1, jptr1);
	handle_joystick(joystick2, &Game.m_GameTarget.m_Joy2);

	/* The game clears the key when it has used it. */
	if (key)
		jptr1->key = key;

	return running;
}

static void bind_keys(void)
{
	JOYSTICK *jptr1 = &Game.m_GameTarget.m_Joy1;
	JOYSTICK *jptr2 = &Game.m_GameTarget.m_Joy2;

	keys[0] = (KeyBinding){ SDL_SCANCODE_UP, &jptr1->up };
	keys[1] = (KeyBinding){ SDL_SCANCODE_DOWN, &jptr1->down };
	keys[2] = (KeyBinding){ SDL_SCANCODE_LEFT, &jptr1->left };
	keys[3] = (KeyBinding){ SDL_SCANCODE_RIGHT, &jptr1->right };
	keys[4] = (KeyBinding){ SDL_SCANCODE_RALT, &jptr1->fire };

	keys[5] = (KeyBinding){ SDL_SCANCODE_S, &jptr2->up };
	keys[6] = (KeyBinding){ SDL_SCANCODE_X, &jptr2->down };
	keys[7] = (KeyBinding){ SDL_SCANCODE_Z, &jptr2->left };
	keys[8] = (KeyBinding){ SDL_SCANCODE_C, &jptr2->right };
	keys[9] = (KeyBinding){ SDL_SCANCODE_LSHIFT, &jptr2->fire };
}

static void main_code(void)
{
	if (!alloc_resources())
	{
		free_resources();
		return;
	}

	SDL_ShowCursor(SDL_DISABLE);
	print_info();

	Game.InitSoundDriver();
	Game.InitGame();
	Game.LoadScores();
	Game.StartGame();
	bind_keys();

	const Uint32 step = 1000 / LOGIC_HZ;
	Uint32 next = SDL_GetTicks();
	bool running = true;

	while (running)
	{
		running = handle_input();
		if (!running)
			break;
		if (!paused)
			Game.MainLoop(true);
		else
			Game.DrawScreen();

		next += step;
		Uint32 now = SDL_GetTicks();
		if ((Sint32)(next - now) > 0)
			SDL_Delay(next - now);
		else if ((Sint32)(now - next) > 200)
			next = now;	/* far behind (a slow machine): don't race */
	}

	Game.SaveScores();
	Game.RemoveSoundDriver();
	free_resources();
}

static void quit_sdl(void)
{
	close_joysticks();
	SDL_Quit();
	amiga_screens_close_leftovers();
}

int main(int argc, char **argv)
{
	read_args(argc, argv);

	amiga_screens_snapshot();
	if (SDL_Init(SDL_INIT_JOYSTICK | SDL_INIT_VIDEO) < 0)
	{
		fprintf(stderr, "Can't init SDL: %s\n", SDL_GetError());
		return 20;
	}
	atexit(quit_sdl);

	open_joysticks();
	main_code();
	return 0;
}

CMethDoc::CMethDoc()
{
	m_pSound = new CAmigaSound;
	m_GameTarget.Init(this);
}

CMethDoc::~CMethDoc()
{
	delete m_pSound;
	m_pSound = 0;
}

void CMethDoc::InitGame(void)
{
	m_GameTarget.InitGame(TheScreen);
	m_GameTarget.PrepareSoundDriver();
}

void CMethDoc::InitSoundDriver(void)
{
	m_pSound->InitDriver(music, effects);
}

void CMethDoc::RemoveSoundDriver(void)
{
	m_pSound->RemoveDriver();
}

void CMethDoc::StartGame(void)
{
	m_GameTarget.StartGame();
}

void CMethDoc::RedrawMainView(int pal_change_flag)
{
	// Not used: the picture is drawn once a step by MainLoop()
}

void CMethDoc::DrawScreen()
{
	Uint32 colors[PALETTE_SIZE];
	METHANE_RGB *pptr = m_GameTarget.m_rgbPalette;
	int cnt, x, y;

	for (cnt = 0; cnt < PALETTE_SIZE; cnt++, pptr++)
		colors[cnt] = ((Uint32)pptr->red << 16) | ((Uint32)pptr->green << 8) | pptr->blue;

	if (SDL_MUSTLOCK(SdlScreen))
		SDL_LockSurface(SdlScreen);

	Uint32 *dptr = (Uint32 *)SdlScreen->pixels;
	const unsigned char *sptr = (const unsigned char *)TheScreen;
	for (y = 0; y < SCR_HEIGHT; y++)
	{
		for (x = 0; x < SCR_WIDTH; x++)
			dptr[x] = colors[sptr[x] & (PALETTE_SIZE - 1)];
		sptr += SCR_WIDTH;
		dptr += SdlScreen->pitch / 4;
	}

	if (SDL_MUSTLOCK(SdlScreen))
		SDL_UnlockSurface(SdlScreen);

	SDL_UpdateTexture(SdlTexture, NULL, SdlScreen->pixels, SdlScreen->pitch);
	SDL_RenderClear(SdlRenderer);
	SDL_RenderCopy(SdlRenderer, SdlTexture, NULL, NULL);
	SDL_RenderPresent(SdlRenderer);
}

void CMethDoc::MainLoop(bool update_logic)
{
	m_GameTarget.MainLoop(update_logic);
	DrawScreen();
}

void CMethDoc::PlaySample(int id, int pos, int rate)
{
	m_pSound->PlaySample(id, pos, rate);
}

void CMethDoc::StopModule(void)
{
	m_pSound->StopModule();
}

void CMethDoc::PlayModule(int id)
{
	m_pSound->PlayModule(id);
}

void CMethDoc::UpdateModule(int id)
{
	m_pSound->UpdateModule(id);
}

void CMethDoc::LoadScores(void)
{
	FILE *fptr = fopen(HighScoreFileName, "r");
	if (!fptr) return;	// No scores yet

	char *mptr = (char *)calloc(1, HighScoreLoadBufferSize);
	if (!mptr)
	{
		fclose(fptr);
		return;
	}

	fread(mptr, 1, HighScoreLoadBufferSize - 2, fptr);

	char *tptr = mptr;
	for (int cnt = 0; cnt < MAX_HISCORES; cnt++)
	{
		if (!tptr[0]) break;
		m_GameTarget.m_Game.InsertHiScore(atoi(&tptr[4]), tptr);

		char let;
		do
		{
			let = *(tptr++);
		} while (!((let == '$') || (!let)));
		if (!let) break;
	}

	free(mptr);
	fclose(fptr);
}

void CMethDoc::SaveScores(void)
{
	amiga_make_dir(UserDir);
	FILE *fptr = fopen(HighScoreFileName, "w");
	if (!fptr) return;

	int cnt;
	HISCORES *hs;
	for (cnt = 0, hs = m_GameTarget.m_Game.m_HiScores; cnt < MAX_HISCORES; cnt++, hs++)
		fprintf(fptr, "%c%c%c%c%d$", hs->name[0], hs->name[1], hs->name[2], hs->name[3], hs->score);

	fclose(fptr);
}
