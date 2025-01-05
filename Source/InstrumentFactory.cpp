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
#include "InstrumentFactory.h"

#include "Sequence.h"
#include "DSample.h"

#include "SeqInstrument.h"
#include "Instrument2A03.h"
#include "InstrumentVRC6.h"
#include "InstrumentFDS.h"
#include "InstrumentVRC7.h"
#include "InstrumentN163.h"
#include "InstrumentS5B.h"

CInstrumentFactory::CInstrumentFactory() : CFactory()
{
	AddProduct<CInstrument2A03>(INST_2A03);
	AddProduct<CSeqInstrument, inst_type_t>(INST_VRC6, INST_VRC6);
	AddProduct<CInstrumentVRC7>(INST_VRC7);
	AddProduct<CInstrumentN163>(INST_N163);
	AddProduct<CInstrumentFDS>(INST_FDS);
	AddProduct<CSeqInstrument, inst_type_t>(INST_S5B, INST_S5B);
}

CInstrument *CInstrumentFactory::CreateNew(inst_type_t Type)		// // // TODO: make this non-static
{
	static CInstrumentFactory a {};
	return a.Produce(Type);
}
