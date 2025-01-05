/*
** Dn-FamiTracker - NES/Famicom sound tracker
** Copyright (C) 2020-2025 D.P.C.M.
** FamiTracker Copyright (C) 2005-2020 Jonathan Liss
** 0CC-FamiTracker Copyright (C) 2014-2018 HertzDevil
**
** This program is free software: you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation, either version 3 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program. If not, see https://www.gnu.org/licenses/.
*/


#pragma once

#include "FamiTrackerTypes.h"

class CSequence;
class CInstrument;
class CFamiTrackerDoc;
class CFamiTrackerView;
class CSoundGen;

struct stRecordSetting {
	int Interval;
	int InstCount;
	bool Reset;
};

class CInstrumentRecorder
{
public:
	CInstrumentRecorder(CSoundGen *pSG);
	~CInstrumentRecorder();

public:
	void			StartRecording();
	void			StopRecording(CFamiTrackerView *pView);
	void			RecordInstrument(const unsigned Tick, CFamiTrackerView *pView);

	CInstrument		*GetRecordInstrument(unsigned Tick) const;
	int				GetRecordChannel() const;
	void			SetRecordChannel(int Channel);;
	stRecordSetting *GetRecordSetting() const;;
	void			SetRecordSetting(stRecordSetting *Setting);;
	void			SetDumpCount(int Count) { m_iDumpCount = Count; };

	void			ResetDumpInstrument();
	void			ResetRecordCache();
	void			ReleaseCurrent();

private:
	void			InitRecordInstrument();
	void			FinalizeRecordInstrument();

public:
	CFamiTrackerDoc *m_pDocument;

private:
	CSoundGen		*m_pSoundGen;
	int				m_iRecordChannel;
	int				m_iDumpCount;
	CInstrument		**m_pDumpInstrument;
	CInstrument		*m_pDumpCache[MAX_INSTRUMENTS];
	CSequence		*m_pSequenceCache[SEQ_COUNT];
	stRecordSetting	m_stRecordSetting;
	char			*m_iRecordWaveCache;
	int				m_iRecordWaveSize;
	int				m_iRecordWaveCount;
};
