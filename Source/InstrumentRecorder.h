/*
** FamiTracker - NES/Famicom sound tracker
** Copyright (C) 2005-2015 Jonathan Liss
**
** 0CC-FamiTracker is (C) 2014-2018 HertzDevil
**
** Dn-FamiTracker is (C) 2020-2021 D.P.C.M.
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
** Library General Public License for more details. To obtain a
** copy of the GNU Library General Public License, write to the Free
** Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
**
** Any permitted reproduction of these routines, in whole or in part,
** must bear this legend.
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
