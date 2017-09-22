/*
** FamiTracker - NES/Famicom sound tracker
** Copyright (C) 2005-2014  Jonathan Liss
**
** 0CC-FamiTracker is (C) 2014-2017 HertzDevil
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful, 
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU 
** Library General Public License for more details.  To obtain a 
** copy of the GNU Library General Public License, write to the Free 
** Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
**
** Any permitted reproduction of these routines, in whole or in part,
** must bear this legend.
*/


#pragma once

#include <memory>
#include <vector>
#include <string>

const int VIBRATO_LENGTH = 256;
const int TREMOLO_LENGTH = 256;

class CFamiTrackerDoc;
class CTempoCounter;
class CPlayerCursor;
class CChannelHandler;
class CTrackerChannel;
class CAPU;
class CSongState;
class stChanNote;
class CSoundGenBase;
class CSoundGen; // TODO: remove
enum note_prio_t;

class CSoundDriver {
public:
	explicit CSoundDriver(CSoundGenBase *parent = nullptr);
	~CSoundDriver();

	void SetupTracks();
	void LoadDocument(const CFamiTrackerDoc &doc, CAPU &apu, CSoundGen &sound);
	void ConfigureDocument();
	void RegisterTracks(CFamiTrackerDoc &doc);

	void StartPlayer(std::unique_ptr<CPlayerCursor> cur);
	void StopPlayer();
	void ResetTracks();

	void LoadSoundState(const CSongState &state);
	void SetTempoCounter(const std::shared_ptr<CTempoCounter> &tempo);

	void Tick();
	void StepRow();
	void UpdateChannels();
	void UpdateAPU(int cycles);

	void QueueNote(int chan, stChanNote &note, note_prio_t priority);
	void SetPlayerPos(int Frame, int Row);
	void EnqueueFrame(int Frame);
	void ForceReloadInstrument(int chan);

	bool IsPlaying() const;
	bool ShouldHalt() const;

	int GetCurrentSong() const;
	std::pair<unsigned, unsigned> GetPlayerPos() const;
	unsigned GetPlayerTicks() const;
	unsigned GetQueuedFrame() const;

	int GetChannelVolume(int chan) const;
	std::string GetChannelStateString(int chan) const;

	int ReadPeriodTable(int Index, int Table) const;
	int ReadVibratoTable(int index) const;

private:
	void AssignTrack(std::unique_ptr<CTrackerChannel> track);
	CChannelHandler *GetChannelHandler(int Index) const;

	void SetupVibrato();
	void SetupPeriodTables();

	void HandleGlobalEffects(stChanNote &note, int fxCols);

private:
	std::vector<std::pair<
		std::unique_ptr<CChannelHandler>, std::unique_ptr<CTrackerChannel>
	>> tracks_;
	const CFamiTrackerDoc *doc_ = nullptr;		// // //
	CAPU *apu_ = nullptr;		// // //
	CSoundGenBase *parent_ = nullptr;		// // //

	bool				m_bPlaying = false;
	bool				m_bHaltRequest = false;

	// Play control
	int					m_iJumpToPattern = -1;
	int					m_iSkipToRow = -1;
	bool				m_bDoHalt = false;					// // // Cxx effect

	unsigned int		m_iNoteLookupTableNTSC[96];			// For 2A03
	unsigned int		m_iNoteLookupTablePAL[96];			// For 2A07
	unsigned int		m_iNoteLookupTableSaw[96];			// For VRC6 sawtooth
	unsigned int		m_iNoteLookupTableVRC7[12];			// // // For VRC7
	unsigned int		m_iNoteLookupTableFDS[96];			// For FDS
	unsigned int		m_iNoteLookupTableN163[96];			// For N163
	unsigned int		m_iNoteLookupTableS5B[96];			// // // For 5B, internal use only
	int					m_iVibratoTable[VIBRATO_LENGTH];

	std::shared_ptr<CTempoCounter> m_pTempoCounter;			// // // tempo calculation
	// Player state
	std::unique_ptr<CPlayerCursor> m_pPlayerCursor;			// // //
};
