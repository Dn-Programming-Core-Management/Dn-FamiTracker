/*
** FamiTracker - NES/Famicom sound tracker
** Copyright (C) 2005-2014  Jonathan Liss
**
** 0CC-FamiTracker is (C) 2014-2017 HertzDevil
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

#include "InstrumentFactory.h"
#include "Instrument2A03.h"
#include "InstrumentVRC6.h"
#include "InstrumentFDS.h"
#include "InstrumentVRC7.h"
#include "InstrumentN163.h"
#include "InstrumentS5B.h"

std::unique_ptr<CInstrument> FTExt::InstrumentFactory::Make(std::size_t index) {
	switch (index) {
	case INST_2A03: return std::make_unique<CInstrument2A03>();
	case INST_VRC6: return std::make_unique<CInstrumentVRC6>();
	case INST_VRC7: return std::make_unique<CInstrumentVRC7>();
	case INST_FDS:  return std::make_unique<CInstrumentFDS>();
	case INST_N163: return std::make_unique<CInstrumentN163>();
	case INST_S5B: return std::make_unique<CInstrumentS5B>();
	}
	return nullptr;
}
