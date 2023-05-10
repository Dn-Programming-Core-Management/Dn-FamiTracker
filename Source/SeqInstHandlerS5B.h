/*
** FamiTracker - NES/Famicom sound tracker
** Copyright (C) 2005-2020 Jonathan Liss
**
** 0CC-FamiTracker is (C) 2014-2018 HertzDevil
**
** Dn-FamiTracker is (C) 2020-2023 D.P.C.M.
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
