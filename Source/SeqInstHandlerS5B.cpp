/*
** FamiTracker - NES/Famicom sound tracker
** Copyright (C) 2005-2020 Jonathan Liss
**
** 0CC-FamiTracker is (C) 2014-2018 HertzDevil
**
** Dn-FamiTracker is (C) 2020-2022 D.P.C.M.
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

#include "stdafx.h"
#include "Sequence.h"
#include "ChannelHandlerInterface.h"
#include "SeqInstHandlerS5B.h"

bool CSeqInstHandlerS5B::ProcessSequence(int Index, unsigned Setting, int Value)
{
	switch (Index) {
	case SEQ_DUTYCYCLE:
		if (auto pChan = dynamic_cast<CChannelHandlerInterfaceS5B*>(m_pInterface)) {
			m_pInterface->SetDutyPeriod(Value & 0xE0);

			if (Value & S5B_MODE_NOISE) {
				pChan->SetNoiseFreq(Value & 0x1F);
			}
			
			return true;
			
			// In chips other than 5B: case SEQ_DUTYCYCLE:
			// m_pInterface->SetDutyPeriod(Value);
		}
	}
	return CSeqInstHandler::ProcessSequence(Index, Setting, Value);
}