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

// required by ChannelHandler.h
#include "stdafx.h"
#include "FamiTrackerDoc.h"
#include "SoundGen.h"

#include "ChannelHandler.h"
#include "ChannelsN163.h"
#include "InstHandler.h"
#include "SeqInstHandler.h"
#include "SeqInstHandlerN163.h"

/*
 * Class CSeqInstHandlerN163
 */

void CSeqInstHandlerN163::LoadInstrument(CInstrument *pInst)		// // //
{
	CSeqInstHandler::LoadInstrument(pInst);
	CChannelInterfaceN163 *pInterface = dynamic_cast<CChannelInterfaceN163*>(m_pInterface);
	if (pInterface == nullptr) return;
	CInstrumentN163 *pN163Inst = dynamic_cast<CInstrumentN163*>(pInst);
	if (pN163Inst == nullptr) return;
	pInterface->SetWaveLength(pN163Inst->GetWaveSize());
	pInterface->SetWavePosition(pN163Inst->GetWavePos());
	pInterface->SetWaveCount(pN163Inst->GetWaveCount());
}
