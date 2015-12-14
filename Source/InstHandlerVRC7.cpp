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

#include "stdafx.h"
#include "Instrument.h"
#include "ChannelHandlerInterface.h"
#include "InstHandler.h"
#include "InstHandlerVRC7.h"

void CInstHandlerVRC7::LoadInstrument(CInstrument *pInst)
{
	m_pInstrument = pInst;
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
	const CInstrumentVRC7 *pVRC7Inst = dynamic_cast<const CInstrumentVRC7*>(m_pInstrument);
	if (pVRC7Inst == nullptr) return;
	pInterface->SetPatch(pVRC7Inst->GetPatch());
	if (!pVRC7Inst->GetPatch())
		for (size_t i = 0; i < 8; i++)
			pInterface->SetCustomReg(i, pVRC7Inst->GetCustomReg(i));
	m_bUpdate = false;
}

void CInstHandlerVRC7::UpdateRegs()
{
	m_bUpdate = true;
}
