/*
 * The Methane Brothers document for AmigaOS 3.x: capehill's
 * source/amigaos4/doc.h with the sound driver of amiga/amiga_sound.h.
 * GPL 2 or later, like Super Methane Brothers. Changes: the AmiCradle Team.
 */

#ifndef _doc_h
#define _doc_h 1

#include "game.h"
#include "target.h"

class CAmigaSound;

class CMethDoc
{
public:
	CMethDoc();
	~CMethDoc();

	void InitGame(void);
	void StartGame(void);
	void MainLoop(bool update_logic);
	void RedrawMainView( int pal_change_flag );
	void PlayModule(int id);
	void StopModule(void);
	void PlaySample(int id, int pos, int rate);
	void RemoveSoundDriver(void);
	void InitSoundDriver(void);
	void UpdateModule(int id);

	void SaveScores(void);
	void LoadScores(void);

	void DrawScreen();

	CAmigaSound *m_pSound;
	CGameTarget m_GameTarget;
};

#endif
