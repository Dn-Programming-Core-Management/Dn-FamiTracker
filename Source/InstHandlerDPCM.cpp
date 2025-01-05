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
#include "SeqInstrument.h"
#include "Instrument2A03.h"
#include "ChannelHandlerInterface.h"
#include "InstHandlerDPCM.h"

CInstHandlerDPCM::CInstHandlerDPCM(CChannelHandlerInterface *pInterface) :
	CInstHandler(pInterface, 0)
{
}

void CInstHandlerDPCM::LoadInstrument(std::shared_ptr<CInstrument> pInst)
{
	m_pInstrument = pInst;
}

void CInstHandlerDPCM::TriggerInstrument()
{
	CChannelHandlerInterfaceDPCM *pInterface = dynamic_cast<CChannelHandlerInterfaceDPCM*>(m_pInterface);
	if (pInterface == nullptr) return;
	if (auto pDPCMInst = std::dynamic_pointer_cast<const CInstrument2A03>(m_pInstrument)) {
		const int Val = m_pInterface->GetNote();
		const int Octave = GET_OCTAVE(Val);
		const int Note = GET_NOTE(Val) - 1;
		if (const CDSample *pSamp = pDPCMInst->GetDSample(Octave, Note)) {
			pInterface->WriteDCOffset(pDPCMInst->GetSampleDeltaValue(Octave, Note));
			pInterface->SetLoopOffset(pDPCMInst->GetSampleLoopOffset(Octave, Note));
			pInterface->PlaySample(pSamp, pDPCMInst->GetSamplePitch(Octave, Note));
		}
	}
}

void CInstHandlerDPCM::ReleaseInstrument()
{
}

void CInstHandlerDPCM::UpdateInstrument()
{
}
