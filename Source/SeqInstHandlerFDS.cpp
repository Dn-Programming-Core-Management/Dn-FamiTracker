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
#include "ChannelsFDS.h"
#include "InstHandler.h"
#include "SeqInstHandler.h"
#include "SeqInstHandlerFDS.h"

/*
 * Class CSeqInstHandlerFDS
 */

void CSeqInstHandlerFDS::LoadInstrument(CInstrument *pInst)		// // //
{
	m_pInstrument = pInst;
	CInstrumentFDS *pSeqInst = dynamic_cast<CInstrumentFDS*>(pInst);
	ASSERT(pInst == nullptr || pSeqInst != nullptr);
	CSequence *pSeq[] = {pSeqInst->GetVolumeSeq(), pSeqInst->GetArpSeq(), pSeqInst->GetPitchSeq()};
	for (size_t i = 0; i < sizeof(pSeq) / sizeof(CSequence*); i++)
		pSeq[i]->GetItemCount() > 0 ? SetupSequence(i, pSeq[i]) : ClearSequence(i);
	
	CChannelInterfaceFDS *pInterface = dynamic_cast<CChannelInterfaceFDS*>(m_pInterface);
	if (pInterface == nullptr) return;
	const CInstrumentFDS *pFDSInst = dynamic_cast<const CInstrumentFDS*>(m_pInstrument);
	if (pFDSInst == nullptr) return;
	pInterface->FillWaveRAM(pFDSInst);
	pInterface->FillModulationTable(pFDSInst);
}

void CSeqInstHandlerFDS::TriggerInstrument()
{
	CSeqInstHandler::TriggerInstrument();
	
	CChannelInterfaceFDS *pInterface = dynamic_cast<CChannelInterfaceFDS*>(m_pInterface);
	if (pInterface == nullptr) return;
	const CInstrumentFDS *pFDSInst = dynamic_cast<const CInstrumentFDS*>(m_pInstrument);
	if (pFDSInst == nullptr) return;
	pInterface->SetFMSpeed(pFDSInst->GetModulationSpeed());
	pInterface->SetFMDepth(pFDSInst->GetModulationDepth());
	pInterface->SetFMDelay(pFDSInst->GetModulationDelay());
	pInterface->FillWaveRAM(pFDSInst);
	pInterface->FillModulationTable(pFDSInst);
}
