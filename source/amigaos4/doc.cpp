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

TODO:
- wrap SDL stuff into a class since this is C++
- use SDL logging features instead of fprintf
- configurable keys
- configurable speed
- configurable player names

*/

#include <stdlib.h>

#include "SDL.h"
#include "doc.h"

#ifdef METHANE_MIKMOD
#include "../mikmod/audiodrv.h"
#endif

#define WINDOW_WIDTH 640
#define WINDOW_HEIGHT 480

static void main_code(void);

static CMethDoc Game;

static SDL_Surface * SdlScreen = 0;
static SDL_Window * SdlWindow;
static SDL_Renderer * SdlRenderer;
static SDL_Texture * SdlTexture;

static char TheScreen[SCR_WIDTH * SCR_HEIGHT];

static SDL_bool fullscreen = SDL_TRUE;
static SDL_bool vsync = SDL_TRUE;
static SDL_bool filter = SDL_FALSE;

static const char* renderer_name = "compositing";

static char HighScoreFileName[] = "Methane.HiScores";
#define HighScoreLoadBufferSize (MAX_HISCORES * 64)

static SDL_Joystick * joystick1;
static SDL_Joystick * joystick2;

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

#define NUM_OF_ACTIONS 6

static KeyBinding keys[2 * NUM_OF_ACTIONS];

static void read_args(int argc, char** argv)
{
    int i = 1;
    while (i < argc) {
        //printf("arg %d - %s\n", i, argv[i]);

        if (!strcmp(argv[i], "nosync")) {
            vsync = SDL_FALSE;
        } else if (!strcmp(argv[i], "window")) {
            fullscreen = SDL_FALSE;
        } else if (!strcmp(argv[i], "filter")) {
            filter = SDL_TRUE;
        } else {
            fprintf(stderr, "Unknown argument '%s'\n", argv[i]);
        }

        i++;
    }
}

static SDL_Joystick* open_joy(int index)
{
    SDL_Joystick* joy = SDL_JoystickOpen(index);

    if (joy) {
        printf("Opened joystick %d (%s) (%d axes, %d buttons)\n",
            index,
            SDL_JoystickNameForIndex(index),
            SDL_JoystickNumAxes(joy),
            SDL_JoystickNumButtons(joy));
    } else {
        fprintf(stderr, "Failed to open joystick %d (%s)\n", index, SDL_GetError());
    }

    return joy;
}

static void open_joysticks()
{
    int num = SDL_NumJoysticks();
    printf("Found %d joysticks\n", num);

    if (num > 0) {
        joystick1 = open_joy(0);
    }

    if (num > 1) {
        joystick2 = open_joy(1);
    }
}

static void close_joy(SDL_Joystick * joy)
{
    if (SDL_JoystickGetAttached(joy)) {
        SDL_JoystickClose(joy);
    }
}

static void close_joysticks()
{
    close_joy(joystick1);
    close_joy(joystick2);
}

int main(int argc, char** argv)
{
    if (SDL_Init(SDL_INIT_JOYSTICK | SDL_INIT_VIDEO) < 0) {
    	fprintf(stderr, "Can't init SDL : %s", SDL_GetError());
    	return 1;
    }

    read_args(argc, argv);

    open_joysticks();

    main_code();

    close_joysticks();

    SDL_Quit();

    return (0);
}

static void print_info()
{
    puts("The GNU General Public License V2 applies to this game.\n");
    puts("See: http://methane.sourceforge.net");
    puts("Instructions:\n");
    puts("Player 1 controls: cursor keys + right alt (or joystick)");
    puts("Player 2 controls: SZXC keys + left shift (or joystick)");
    puts("ESC = Quit (and save high scores)");
    puts("TAB = Change player graphic during game");
    //puts("F1 = Next Level (DISABLED)\n");
}

static SDL_bool alloc_resources()
{
    SDL_bool result = SDL_FALSE;

    printf("Fullscreen %d, vsync %d, filter %d\n",
        fullscreen, vsync, filter);

    // ARGB(8888) surface
    SdlScreen = SDL_CreateRGBSurface(0, SCR_WIDTH, SCR_HEIGHT, 32,
        0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);

    if (!SdlScreen)
    {
    	fprintf(stderr, "Couldn't create surface: %s\n", SDL_GetError());
    	goto out;
    }

    SdlWindow = SDL_CreateWindow("Super Methane Bros SDL2",
       SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
       WINDOW_WIDTH, WINDOW_HEIGHT,
       fullscreen ? SDL_WINDOW_FULLSCREEN : 0);

    if (!SdlWindow)
    {
        fprintf(stderr, "Couldn't create window: %s\n", SDL_GetError());
        goto out;
    }

    SDL_SetHint(SDL_HINT_RENDER_DRIVER, renderer_name);
    SDL_SetHint(SDL_HINT_RENDER_VSYNC, vsync ? "1" : "0");
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, filter ? "best" : "nearest");

    SdlRenderer = SDL_CreateRenderer(SdlWindow, -1, SDL_RENDERER_ACCELERATED);

    if (!SdlRenderer)
    {
        fprintf(stderr, "Couldn't create renderer: %s\n", SDL_GetError());
        goto out;
    }

    SDL_RendererInfo ri;

    if (SDL_GetRendererInfo(SdlRenderer, &ri) == 0) {
         printf("Renderer '%s' created\n", ri.name);
    } else {
        fprintf(stderr, "Failed to get renderer info: %s\n", SDL_GetError());
    }

    SdlTexture = SDL_CreateTexture(SdlRenderer, SDL_PIXELFORMAT_ARGB8888,
        SDL_TEXTUREACCESS_STREAMING, SCR_WIDTH, SCR_HEIGHT);

    if (!SdlTexture)
    {
        fprintf(stderr, "Couldn't create texture: %s\n", SDL_GetError());
        goto out;
    }

    result = SDL_TRUE;

out:
    return result;
}

static void free_resources()
{
    if (SdlTexture) SDL_DestroyTexture(SdlTexture);
    if (SdlRenderer) SDL_DestroyRenderer(SdlRenderer);
    if (SdlWindow) SDL_DestroyWindow(SdlWindow);
    if (SdlScreen) SDL_FreeSurface(SdlScreen);
}

static void handle_joystick(int index)
{
    SDL_Joystick * joy;
    KeyBinding * keyOffset;

    if (index == 1) {
        joy = joystick1;
        keyOffset = &keys[0];
    } else {
        joy = joystick2;
        keyOffset= &keys[NUM_OF_ACTIONS];
    }

    if (SDL_JoystickGetAttached(joy)) {
        if (SDL_JoystickGetButton(joy, 0)) {
            *keyOffset[EKeyFire].data = 1;
        }

        const Sint16 AXIS_THRESHOLD = 30000;

        const Sint16 xAxis = SDL_JoystickGetAxis(joy, 0);
        const Sint16 yAxis = SDL_JoystickGetAxis(joy, 1);

        if (xAxis < -AXIS_THRESHOLD) {
            *keyOffset[EKeyLeft].data = 1;
        } else if (xAxis > AXIS_THRESHOLD) {
            *keyOffset[EKeyRight].data = 1;
        }

        if (yAxis < -AXIS_THRESHOLD) {
            *keyOffset[EKeyUp].data = 1;
        } else if (yAxis > AXIS_THRESHOLD) {
            *keyOffset[EKeyDown].data = 1;
        }

        // D-pad
        const Uint8 hat = SDL_JoystickGetHat(joy, 0);

        if (hat & SDL_HAT_UP) {
            *keyOffset[EKeyUp].data = 1;
        }

        if (hat & SDL_HAT_DOWN) {
            *keyOffset[EKeyDown].data = 1;
        }

        if (hat & SDL_HAT_LEFT) {
            *keyOffset[EKeyLeft].data = 1;
        }

        if (hat & SDL_HAT_RIGHT) {
            *keyOffset[EKeyRight].data = 1;
        }
    }
}

static SDL_bool handle_input(void)
{
    SDL_bool running = SDL_TRUE;

    SDL_PumpEvents();

    const Uint8 *state = SDL_GetKeyboardState(NULL);

    if (state[SDL_SCANCODE_ESCAPE]) {
        running = SDL_FALSE;
    }

    if (state[SDL_SCANCODE_TAB]) {
    	Game.m_GameTarget.m_Game.TogglePuffBlow();
    }

    for (size_t i = 0; i < sizeof(keys) / sizeof(keys[0]); i++) {
        *keys[i].data = state[keys[i].sc];
    }

    *keys[5].data = *keys[11].data = 13; // TODO: allow proper text input

    handle_joystick(1);
    handle_joystick(2);

    SDL_Event e;
    while (SDL_PollEvent(&e)) {
        if (e.type == SDL_QUIT) running = SDL_FALSE;
    }

    return running;
}

static void bind_keys()
{
    JOYSTICK *jptr1 = &Game.m_GameTarget.m_Joy1;
    JOYSTICK *jptr2 = &Game.m_GameTarget.m_Joy2;

    // Player 1
    keys[0] = (KeyBinding){ SDL_SCANCODE_UP, &jptr1->up };
    keys[1] = (KeyBinding){ SDL_SCANCODE_DOWN, &jptr1->down };
    keys[2] = (KeyBinding){ SDL_SCANCODE_LEFT, &jptr1->left };
    keys[3] = (KeyBinding){ SDL_SCANCODE_RIGHT, &jptr1->right };
    keys[4] = (KeyBinding){ SDL_SCANCODE_RALT, &jptr1->fire };
    keys[5] = (KeyBinding){ SDL_SCANCODE_UNKNOWN, &jptr1->key };

    // Player 2
    keys[6] = (KeyBinding){ SDL_SCANCODE_S, &jptr2->up };
    keys[7] = (KeyBinding){ SDL_SCANCODE_X, &jptr2->down };
    keys[8] = (KeyBinding){ SDL_SCANCODE_Z, &jptr2->left };
    keys[9] = (KeyBinding){ SDL_SCANCODE_C, &jptr2->right };
    keys[10] = (KeyBinding){ SDL_SCANCODE_LSHIFT , &jptr2->fire };
    keys[11] = (KeyBinding){ SDL_SCANCODE_UNKNOWN, &jptr2->key };
}

//------------------------------------------------------------------------------
//! \brief The program main code
//------------------------------------------------------------------------------
static void main_code(void)
{
    if (!alloc_resources()) {
        free_resources();
        return;
    }

    SDL_ShowCursor(SDL_DISABLE);

    print_info();

    Game.InitSoundDriver();
    Game.InitGame ();
    Game.LoadScores();
    Game.StartGame();

    bind_keys();

    SDL_bool running = SDL_TRUE;

    Uint32 lastLogicUpdate = SDL_GetTicks();

    const Uint32 logicUpdatesPerSecond = 30;
    const Uint32 ticksToUpdate = 1000 / logicUpdatesPerSecond;

    while (running)
    {
        const Uint32 now = SDL_GetTicks();
        const bool updateLogic = ((now - lastLogicUpdate) >= ticksToUpdate);

        if (updateLogic) {
            lastLogicUpdate = now;
        }

        running = handle_input();

    	Game.MainLoop(updateLogic);
    }

    Game.SaveScores();
    free_resources();
}

//------------------------------------------------------------------------------
//! \brief Initialise Document
//------------------------------------------------------------------------------
CMethDoc::CMethDoc()
{
#ifdef METHANE_MIKMOD
	SMB_NEW(m_pMikModDrv,CMikModDrv);
#endif
	m_GameTarget.Init(this);
}

//------------------------------------------------------------------------------
//! \brief Destroy Document
//------------------------------------------------------------------------------
CMethDoc::~CMethDoc()
{
#ifdef METHANE_MIKMOD
	if (m_pMikModDrv)
	{
		delete(m_pMikModDrv);
		m_pMikModDrv = 0;
	}
#endif
}

//------------------------------------------------------------------------------
//! \brief Initialise the game
//------------------------------------------------------------------------------
void CMethDoc::InitGame(void)
{
	m_GameTarget.InitGame (TheScreen);
	m_GameTarget.PrepareSoundDriver ();
}

//------------------------------------------------------------------------------
//! \brief Initialise the sound driver
//------------------------------------------------------------------------------
void CMethDoc::InitSoundDriver(void)
{
#ifdef METHANE_MIKMOD
	m_pMikModDrv->InitDriver();
#endif
}

//------------------------------------------------------------------------------
//! \brief Remove the sound driver
//------------------------------------------------------------------------------
void CMethDoc::RemoveSoundDriver(void)
{
#ifdef METHANE_MIKMOD
	m_pMikModDrv->RemoveDriver();
#endif
}

//------------------------------------------------------------------------------
//! \brief Start the game
//------------------------------------------------------------------------------
void CMethDoc::StartGame(void)
{
	m_GameTarget.StartGame();
}

//------------------------------------------------------------------------------
//! \brief Redraw the game main view
//!
//! 	\param pal_change_flag : 0 = Palette not changed
//------------------------------------------------------------------------------
void CMethDoc::RedrawMainView( int pal_change_flag )
{
	// Function not used
}

//------------------------------------------------------------------------------
//! \brief Draw the screen
//!
//! 	\param screen_ptr = UNUSED
//------------------------------------------------------------------------------
void CMethDoc::DrawScreen()
{
    //SDL_Color colors[PALETTE_SIZE];
    Uint32 colors[PALETTE_SIZE];

    // Set the game palette
    METHANE_RGB *pptr = m_GameTarget.m_rgbPalette;

    for (int cnt=0; cnt < PALETTE_SIZE; cnt++, pptr++)
    {
        /*
    	colors[cnt].r = pptr->red;
    	colors[cnt].g = pptr->green;
    	colors[cnt].b = pptr->blue;
        colors[cnt].a = 255;
        */
        colors[cnt] = 255 << 24 | pptr->red << 16 | pptr->green << 8 | pptr->blue; // ARGB
    }

    if (SDL_MUSTLOCK (SdlScreen))
    {
    	SDL_LockSurface (SdlScreen);
    }

    // Update surface
    Uint32 * dptr = (Uint32 *) SdlScreen->pixels;
    char * sptr = TheScreen;

    for (int y = 0; y < SCR_HEIGHT; y++) {
    	for (int x = 0; x < SCR_WIDTH; x++) {
    		dptr[x] = colors[(Uint8) *sptr++];
    	}
    	dptr += (SdlScreen->pitch / 4);
    }

    if (SDL_MUSTLOCK (SdlScreen))
    {
    	SDL_UnlockSurface (SdlScreen);
    }

    SDL_UpdateTexture(SdlTexture, NULL, SdlScreen->pixels, SdlScreen->pitch);
    SDL_RenderCopy(SdlRenderer, SdlTexture, NULL, NULL);
    SDL_RenderPresent(SdlRenderer);
}

//------------------------------------------------------------------------------
//! \brief The Game Main Loop
//!
//------------------------------------------------------------------------------
void CMethDoc::MainLoop( bool update_logic )
{
	m_GameTarget.MainLoop(update_logic);
	DrawScreen();
#ifdef METHANE_MIKMOD
	m_pMikModDrv->Update();
#endif
}

//------------------------------------------------------------------------------
//! \brief Play a sample (called from the game)
//!
//! 	\param id = SND_xxx id
//!	\param pos = Sample Position to use 0 to 255
//!	\param rate = The rate
//------------------------------------------------------------------------------
void CMethDoc::PlaySample(int id, int pos, int rate)
{
#ifdef METHANE_MIKMOD
	m_pMikModDrv->PlaySample(id, pos, rate);
#endif
}

//------------------------------------------------------------------------------
//! \brief Stop the module (called from the game)
//------------------------------------------------------------------------------
void CMethDoc::StopModule(void)
{
#ifdef METHANE_MIKMOD
	m_pMikModDrv->StopModule();
#endif
}

//------------------------------------------------------------------------------
//! \brief Play a module (called from the game)
//!
//! 	\param id = SMOD_xxx id
//------------------------------------------------------------------------------
void CMethDoc::PlayModule(int id)
{
#ifdef METHANE_MIKMOD
	m_pMikModDrv->PlayModule(id);
#endif
}

//------------------------------------------------------------------------------
//! \brief Update the current module (ie restart the module if it has stopped) (called from the game)
//!
//! 	\param id = SMOD_xxx id (The module that should be playing)
//------------------------------------------------------------------------------
void CMethDoc::UpdateModule(int id)
{
#ifdef METHANE_MIKMOD
	m_pMikModDrv->UpdateModule(id);
#endif
}

//------------------------------------------------------------------------------
//! \brief Load the high scores
//------------------------------------------------------------------------------
void CMethDoc::LoadScores(void)
{
    FILE *fptr = fopen(HighScoreFileName, "r");
    if (!fptr) return;	// No scores available

    // Allocate file memory, which is cleared to zero
    char *mptr = (char *) calloc(1, HighScoreLoadBufferSize);
    if (!mptr)		// No memory
    {
    	fclose(fptr);
    	return;
    }
        	
    fread(mptr, 1, HighScoreLoadBufferSize - 2, fptr);	 // Get the file

    // (Note: mptr is zero terminated)
    char *tptr = mptr;
        	
    for (int cnt = 0; cnt < MAX_HISCORES; cnt++)	// For each highscore
    {
    	if (!tptr[0]) break;

    	m_GameTarget.m_Game.InsertHiScore(atoi(&tptr[4]), tptr);

    	char let;

    	do	// Find next name
    	{
    		let = *(tptr++);
    	} while (!( (let == '$') || (!let) ));

    	if (!let) break;	// Unexpected EOF
    }

    free(mptr);

    fclose(fptr);
}

//------------------------------------------------------------------------------
//! \brief Save the high scores
//------------------------------------------------------------------------------
void CMethDoc::SaveScores(void)
{
    FILE *fptr = fopen(HighScoreFileName, "w");
        	
    if (!fptr) return;	// Cannot write scores

    int cnt;
    HISCORES *hs;
        	
    for (cnt = 0, hs = m_GameTarget.m_Game.m_HiScores; cnt < MAX_HISCORES; cnt++, hs++)
    {
    	fprintf(fptr, "%c%c%c%c%d$", hs->name[0], hs->name[1], hs->name[2], hs->name[3], hs->score);
    }

    fclose(fptr);
}

