/*
** FamiTracker - NES/Famicom sound tracker
** Copyright (C) 2005-2014  Jonathan Liss
**
** 0CC-FamiTracker is (C) 2014-2016 HertzDevil
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


#pragma once

#include "SeqInstHandler.h"

/*!
	\brief Class for VRC6 sawtooth sequence instrument handlers.
	\details The sawtooth channel adds special processing for both 16-step and 64-step volume
	sequences.
*/
class CSeqInstHandlerSawtooth : public CSeqInstHandler
{
public:
	/*!	\brief Constructor of the VRC6 sawtooth sequence instrument handler.
		\param pInterface Pointer to the channel interface.
		\param Vol Default volume for instruments used by this handler.
		\param Duty Default duty cycle for instruments used by this handler. */
	CSeqInstHandlerSawtooth(CChannelHandlerInterface *pInterface, int Vol, int Duty) :
		CSeqInstHandler(pInterface, Vol, Duty) { }

	/*!	\brief Starts a new note for the instrument handler.
		\details This reimplementation checks whether the current instrument uses a 64-step volume
		sequence. */
	void TriggerInstrument() override;

	/*!	\brief Queries whether the duty sequence should be ignored when calculating the volume.
		\return Whether the current instrument uses a 64-step volume sequence. */
	bool IsDutyIgnored() const;

private:
	bool m_bIgnoreDuty = false;
};
