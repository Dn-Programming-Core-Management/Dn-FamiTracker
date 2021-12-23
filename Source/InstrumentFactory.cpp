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
