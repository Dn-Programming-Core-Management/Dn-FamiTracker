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

#include "stdafx.h"
#include "Sequence.h"
#include "Instrument.h"
#include "SeqInstrument.h"
#include "InstrumentVRC7.h"
#include "ChannelHandlerInterface.h"
#include "SeqInstHandlerVRC7.h"

/*
 * Class CSeqInstHandlerVRC7
 */

void CSeqInstHandlerVRC7::LoadInstrument(std::shared_ptr<CInstrument> pInst)		// // //
{
	CSeqInstHandler::LoadInstrument(pInst);
	m_pInstrument = pInst;
	UpdateRegs();
}

void CSeqInstHandlerVRC7::TriggerInstrument()
{
	CSeqInstHandler::TriggerInstrument();
	UpdateRegs();
}

void CSeqInstHandlerVRC7::ReleaseInstrument()
{
	CSeqInstHandler::ReleaseInstrument();
}

void CSeqInstHandlerVRC7::UpdateInstrument()
{
	CSeqInstHandler::UpdateInstrument();

	if (!m_bUpdate) return;
	CChannelHandlerInterfaceVRC7* pInterface = dynamic_cast<CChannelHandlerInterfaceVRC7*>(m_pInterface);
	if (pInterface == nullptr) return;
	auto pVRC7Inst = std::dynamic_pointer_cast<const CInstrumentVRC7>(m_pInstrument);
	if (pVRC7Inst == nullptr) return;
	pInterface->SetPatch(pVRC7Inst->GetPatch());
	if (!pVRC7Inst->GetPatch())
		for (std::size_t i = 0; i < 8; i++)
			pInterface->SetCustomReg(i, pVRC7Inst->GetCustomReg(i));
	m_bUpdate = false;
}

void CSeqInstHandlerVRC7::UpdateRegs()
{
	m_bUpdate = true;
}
