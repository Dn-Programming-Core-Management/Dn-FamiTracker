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

#include "stdafx.h"
#include "Sequence.h"
#include "Instrument.h"
#include "SeqInstrument.h"
#include "InstrumentFDS.h"
#include "ChannelHandlerInterface.h"
#include "SeqInstHandlerFDS.h"

/*
 * Class CSeqInstHandlerFDS
 */

void CSeqInstHandlerFDS::LoadInstrument(std::shared_ptr<CInstrument> pInst)		// // //
{
	CSeqInstHandler::LoadInstrument(pInst);

	if (auto pFDSInst = std::dynamic_pointer_cast<const CInstrumentFDS>(m_pInstrument))
		UpdateTables(pFDSInst.get());
}

void CSeqInstHandlerFDS::TriggerInstrument()
{
	CSeqInstHandler::TriggerInstrument();

	auto *pInterface = dynamic_cast<CChannelHandlerInterfaceFDS*>(m_pInterface);
	if (pInterface == nullptr) return;
	auto pFDSInst = std::dynamic_pointer_cast<const CInstrumentFDS>(m_pInstrument);
	if (pFDSInst == nullptr) return;
	pInterface->SetFMSpeed(pFDSInst->GetModulationSpeed());
	pInterface->SetFMDepth(pFDSInst->GetModulationDepth());
	pInterface->SetFMDelay(pFDSInst->GetModulationDelay());
	UpdateTables(pFDSInst.get());
}

void CSeqInstHandlerFDS::UpdateInstrument()
{
	CSeqInstHandler::UpdateInstrument();
	
	if (auto pFDSInst = std::dynamic_pointer_cast<const CInstrumentFDS>(m_pInstrument))
		UpdateTables(pFDSInst.get());
}

void CSeqInstHandlerFDS::UpdateTables(const CInstrumentFDS *pInst)
{
	auto *pInterface = dynamic_cast<CChannelHandlerInterfaceFDS*>(m_pInterface);
	if (pInterface == nullptr) return;
	char Buffer[0x40];		// // //
	for (int i = 0; i < 0x40; i++)
		Buffer[i] = pInst->GetSample(i);
	pInterface->FillWaveRAM(Buffer);
	for (int i = 0; i < 0x20; i++)
		Buffer[i] = pInst->GetModulation(i);
	pInterface->FillModulationTable(Buffer);
}
