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

#include "SeqInstHandler.h"

/*!
	\brief Class for Sunsoft 5B sequence instrument handlers.
	\details The 5B instruments process noise frequency in the duty sequence.
*/
class CSeqInstHandlerS5B : public CSeqInstHandler
{
public:
	using CSeqInstHandler::CSeqInstHandler;

private:
	/*!	\brief Processes the value retrieved from a sequence.
		\return True if the sequence has finished processing.
		\param Index The sequence type.
		\param Setting The sequence setting.
		\param Value The sequence value to be processed. */
	virtual bool ProcessSequence(int Index, unsigned Setting, int Value);
};
