/*
** FamiTracker - NES/Famicom sound tracker
** Copyright (C) 2005-2014  Jonathan Liss
**
** 0CC-FamiTracker is (C) 2014-2015 HertzDevil
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

class CSeqInstHandler : public CInstHandler
{
public:
	CSeqInstHandler(CChannelInterface *pInterface, int Vol, int Duty);
	virtual void LoadInstrument(CInstrument *pInst);
	virtual void TriggerInstrument();
	virtual void ReleaseInstrument();
	virtual void UpdateInstrument();

	seq_state_t GetSequenceState(int Index) const { return m_iSeqState[Index]; }

protected:
	void SetupSequence(int Index, const CSequence *pSequence);
	void ClearSequence(int Index);

protected:
	const CSequence	*m_pSequence[SEQ_COUNT];
	seq_state_t		m_iSeqState[SEQ_COUNT];
	int				m_iSeqPointer[SEQ_COUNT];
	int				m_iDutyParam;
	const int		m_iDefaultDuty;
};
