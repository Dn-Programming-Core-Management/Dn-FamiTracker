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
#include "Instrument.h"
#include "InstrumentVRC7.h"
#include "ChannelHandlerInterface.h"
#include "InstHandlerVRC7.h"

void CInstHandlerVRC7::LoadInstrument(std::shared_ptr<CInstrument> pInst)
{
	m_pInstrument = pInst;
	//m_pInstrument = std::dynamic_pointer_cast<CInstrumentVRC7>(pInst);
	UpdateRegs();
}

void CInstHandlerVRC7::TriggerInstrument()
{
	UpdateRegs();
}

void CInstHandlerVRC7::ReleaseInstrument()
{
}

void CInstHandlerVRC7::UpdateInstrument()
{
	if (!m_bUpdate) return;
	CChannelHandlerInterfaceVRC7 *pInterface = dynamic_cast<CChannelHandlerInterfaceVRC7*>(m_pInterface);
	if (pInterface == nullptr) return;
	auto pVRC7Inst = std::dynamic_pointer_cast<const CInstrumentVRC7>(m_pInstrument);
	if (pVRC7Inst == nullptr) return;
	pInterface->SetPatch(pVRC7Inst->GetPatch());
	if (!pVRC7Inst->GetPatch())
		for (std::size_t i = 0; i < 8; i++)
			pInterface->SetCustomReg(i, pVRC7Inst->GetCustomReg(static_cast<int>(i)));
	m_bUpdate = false;
}

void CInstHandlerVRC7::UpdateRegs()
{
	m_bUpdate = true;
}
