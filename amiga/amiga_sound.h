/*
 * Super Methane Brothers' music and effects on SDL2_mixer (in place of
 * MikMod): the tunes are the game's own ProTracker modules, played by
 * SDL2_mixer.module's libxmp; the effects are its signed 8-bit samples,
 * turned into 16-bit chunks at the mixer's rate.
 * The AmiCradle Team. GPL 2 or later, like Super Methane Brothers.
 */

#ifndef AMIGA_SOUND_H
#define AMIGA_SOUND_H 1

class CAmigaSound
{
public:
	CAmigaSound();
	~CAmigaSound();
	void InitDriver(bool music, bool effects);
	void RemoveDriver(void);
	void PlaySample(int id, int pos, int rate);
	void StopModule(void);
	void PlayModule(int id);
	void UpdateModule(int id);

	bool m_AudioValid;
	bool m_Music;
	bool m_Effects;
};

#endif
