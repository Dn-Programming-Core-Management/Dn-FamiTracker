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
