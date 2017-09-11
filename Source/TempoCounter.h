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

#include <cstdint>

class CFamiTrackerDoc;
class CGroove;
class stFullState;

class CTempoCounter {
public:
	explicit CTempoCounter(const CFamiTrackerDoc &pDoc);
	void AssignDocument(const CFamiTrackerDoc &pDoc);

	void LoadTempo(unsigned Track);
	double GetTempo() const;

	void Tick();
	void StepRow();
	bool CanStepRow() const;

	void DoFxx(uint8_t Param);
	void DoOxx(uint8_t Param);
	void LoadSoundState(const stFullState &state);

private:
	void SetupSpeed();
	void LoadGroove(const CGroove &Groove);
	void UpdateGrooveSpeed();
	void StepGroove();

private:
	const CFamiTrackerDoc *m_pDocument = nullptr;
	const CGroove *m_pCurrentGroove = nullptr;

	unsigned int m_iTempo;
	unsigned int m_iSpeed;
	unsigned int m_iGroovePosition = 0;		// // // Groove position

	int m_iTempoAccum = 0;		// Used for speed calculation
	int m_iTempoDecrement = 0;
	int m_iTempoRemainder = 0;
};
