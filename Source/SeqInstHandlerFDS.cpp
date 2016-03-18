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

#include <memory>
#include "stdafx.h"
#include "Sequence.h"
#include "Instrument.h"
#include "ChannelHandlerInterface.h"
#include "InstHandler.h"
#include "SeqInstHandler.h"
#include "SeqInstHandlerFDS.h"

/*
 * Class CSeqInstHandlerFDS
 */

void CSeqInstHandlerFDS::LoadInstrument(CInstrument *pInst)		// // //
{
	CSeqInstHandler::LoadInstrument(pInst);
	
	const CInstrumentFDS *pFDSInst = dynamic_cast<const CInstrumentFDS*>(m_pInstrument);
	if (pFDSInst == nullptr) return;
	UpdateTables(pFDSInst);
}

void CSeqInstHandlerFDS::TriggerInstrument()
{
	CSeqInstHandler::TriggerInstrument();
	
	CChannelHandlerInterfaceFDS *pInterface = dynamic_cast<CChannelHandlerInterfaceFDS*>(m_pInterface);
	if (pInterface == nullptr) return;
	const CInstrumentFDS *pFDSInst = dynamic_cast<const CInstrumentFDS*>(m_pInstrument);
	if (pFDSInst == nullptr) return;
	pInterface->SetFMSpeed(pFDSInst->GetModulationSpeed());
	pInterface->SetFMDepth(pFDSInst->GetModulationDepth());
	pInterface->SetFMDelay(pFDSInst->GetModulationDelay());
	UpdateTables(pFDSInst);
}

void CSeqInstHandlerFDS::UpdateTables(const CInstrumentFDS *pInst)
{
	CChannelHandlerInterfaceFDS *pInterface = dynamic_cast<CChannelHandlerInterfaceFDS*>(m_pInterface);
	if (pInterface == nullptr) return;
	char Buffer[0x40];		// // //
	for (int i = 0; i < 0x40; i++)
		Buffer[i] = pInst->GetSample(i);
	pInterface->FillWaveRAM(Buffer);
	for (int i = 0; i < 0x20; i++)
		Buffer[i] = pInst->GetModulation(i);
	pInterface->FillModulationTable(Buffer);
}
