/*
 * Super Methane Brothers' music and effects on SDL2_mixer, in place of
 * upstream's MikMod driver (source/mikmod/audiodrv.cpp, which it follows:
 * the same tunes, voices, panning and rates).
 * The AmiCradle Team. GPL 2 or later, like Super Methane Brothers.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "SDL.h"
#include "SDL_mixer.h"

#include "global.h"
#include "snddef.h"
#include "amiga_sound.h"

#define MIX_RATE	22050
#define NUM_VOICES	12

extern MODULE_RESOURCE_DATA MethaneModuleData[MODULE_COUNT];
extern SAMPLE_RESOURCE_DATA MethaneSampleData[SND_COUNT];

/* One chunk per sample and rate; the game asks for few rates (the
 * xylophone's notes are the only ones that change). */
#define MAX_CHUNKS 96
static struct { int id, rate; Mix_Chunk *chunk; } chunks[MAX_CHUNKS];
static int chunk_count;

static Mix_Music *tunes[MODULE_COUNT];
static int playing_tune = -1;

CAmigaSound::CAmigaSound()
{
	m_AudioValid = false;
	m_Music = true;
	m_Effects = true;
}

CAmigaSound::~CAmigaSound()
{
	RemoveDriver();
}

void CAmigaSound::InitDriver(bool music, bool effects)
{
	m_Music = music;
	m_Effects = effects;
	if (!music && !effects)
		return;
	if (SDL_InitSubSystem(SDL_INIT_AUDIO) < 0)
	{
		fprintf(stderr, "No sound: %s\n", SDL_GetError());
		return;
	}
	if (Mix_OpenAudio(MIX_RATE, AUDIO_S16SYS, 2, 2048) < 0)
	{
		fprintf(stderr, "No sound: %s\n", Mix_GetError());
		SDL_QuitSubSystem(SDL_INIT_AUDIO);
		return;
	}
	Mix_Init(MIX_INIT_MOD);
	Mix_AllocateChannels(NUM_VOICES);
	m_AudioValid = true;
}

void CAmigaSound::RemoveDriver(void)
{
	int i;

	if (!m_AudioValid)
		return;
	m_AudioValid = false;
	Mix_HaltMusic();
	Mix_HaltChannel(-1);
	for (i = 0; i < MODULE_COUNT; i++)
		if (tunes[i])
		{
			Mix_FreeMusic(tunes[i]);
			tunes[i] = 0;
		}
	for (i = 0; i < chunk_count; i++)
		Mix_FreeChunk(chunks[i].chunk);
	chunk_count = 0;
	Mix_CloseAudio();
	Mix_Quit();
	SDL_QuitSubSystem(SDL_INIT_AUDIO);
}

/* The sample (signed 8-bit at its own rate) as a 16-bit stereo chunk at
 * the mixer's rate. The buffer is ours: Mix_QuickLoad_RAW keeps it. */
static Mix_Chunk *make_chunk(int idx, int rate)
{
	const signed char *src = (const signed char *)MethaneSampleData[idx].ptr;
	int len = MethaneSampleData[idx].length;
	int out_len, i;
	Sint16 *buf;
	Mix_Chunk *chunk;

	if (len <= 0 || rate <= 0)
		return 0;
	out_len = (int)(((long long)len * MIX_RATE) / rate);
	if (out_len < 1)
		return 0;
	buf = (Sint16 *)SDL_malloc(out_len * 2 * sizeof(Sint16));
	if (!buf)
		return 0;
	for (i = 0; i < out_len; i++)
	{
		int s = (int)(((long long)i * rate) / MIX_RATE);
		Sint16 v;
		if (s >= len) s = len - 1;
		v = (Sint16)(src[s] * 256);
		buf[i * 2] = v;
		buf[i * 2 + 1] = v;
	}
	chunk = Mix_QuickLoad_RAW((Uint8 *)buf, out_len * 2 * sizeof(Sint16));
	if (!chunk)
	{
		SDL_free(buf);
		return 0;
	}
	chunk->allocated = 1;	/* Mix_FreeChunk frees buf */
	return chunk;
}

static Mix_Chunk *get_chunk(int idx, int rate)
{
	int i;
	Mix_Chunk *c;

	for (i = 0; i < chunk_count; i++)
		if (chunks[i].id == idx && chunks[i].rate == rate)
			return chunks[i].chunk;
	if (chunk_count >= MAX_CHUNKS)
		return 0;
	c = make_chunk(idx, rate);
	if (!c)
		return 0;
	chunks[chunk_count].id = idx;
	chunks[chunk_count].rate = rate;
	chunks[chunk_count].chunk = c;
	chunk_count++;
	return c;
}

void CAmigaSound::PlaySample(int id, int pos, int rate)
{
	int idx, channel, left, right, volume;
	Mix_Chunk *c;

	if (!m_AudioValid || !m_Effects)
		return;
	idx = id - SND_START_NUMBER;
	if (idx < 0 || idx >= SND_COUNT)
		return;
	c = get_chunk(idx, rate);
	if (!c)
		return;
	channel = Mix_PlayChannel(-1, c, 0);
	if (channel < 0)
		return;
	/* As the MikMod driver: louder at the centre, panned by pos 0..255. */
	volume = 256 - 8 - (pos > 128 ? pos - 128 : 128 - pos);
	Mix_Volume(channel, volume / 2);
	left = (255 - pos) * 2;
	right = pos * 2;
	Mix_SetPanning(channel, left > 255 ? 255 : left, right > 255 ? 255 : right);
}

void CAmigaSound::StopModule(void)
{
	if (!m_AudioValid)
		return;
	Mix_HaltMusic();
	playing_tune = -1;
}

void CAmigaSound::PlayModule(int id)
{
	int idx;

	if (!m_AudioValid)
		return;
	StopModule();
	if (!m_Music)
		return;
	idx = id - MODULE_START_NUMBER;
	if (idx < 0 || idx >= MODULE_COUNT || id == SMOD_EMPTY)
		return;
	if (!tunes[idx])
	{
		SDL_RWops *rw = SDL_RWFromConstMem(MethaneModuleData[idx].ptr,
						   MethaneModuleData[idx].length);
		if (!rw)
			return;
		tunes[idx] = Mix_LoadMUSType_RW(rw, MUS_MOD, 1);
		if (!tunes[idx])
		{
			fprintf(stderr, "Tune %d: %s\n", idx, Mix_GetError());
			m_Music = false;	/* say it once */
			return;
		}
	}
	Mix_VolumeMusic(MIX_MAX_VOLUME * 3 / 4);
	if (Mix_PlayMusic(tunes[idx], -1) == 0)
		playing_tune = idx;
}

/* Upstream restarts the tune that should be playing when it has stopped. */
void CAmigaSound::UpdateModule(int id)
{
	if (!m_AudioValid || !m_Music || id == SMOD_EMPTY)
		return;
	if (id - MODULE_START_NUMBER != playing_tune || !Mix_PlayingMusic())
		PlayModule(id);
}
