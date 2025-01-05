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


#pragma once

#include "Instrument.h"
#include "Factory.h"

/*!
	\brief The instrument factory namespace.
	\details Including this class will automatically include all other instrument header files.
*/
class CInstrumentFactory : CFactory<inst_type_t, CInstrument>
{
private:
	CInstrumentFactory();
public:
	/*!	\brief Creates a new instrument.
		\param Type The instrument type, which should be a member of inst_type_t.
		\return Pointer to the created instrument, or nullptr if the type is invalid.
	*/
	static CInstrument *CreateNew(inst_type_t Type);
};
