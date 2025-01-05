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

class CInstrumentFDS;

/*!
	\brief Class for FDS sequence instrument handlers.
	\details FDS instruments use a slightly different internal format than other seuqnece instruments
	in that sequences are bound to instruments rather than the underlying FamiTracker module.
*/
class CSeqInstHandlerFDS : public CSeqInstHandler
{
public:
	/*!	\brief Constructor of the FDS sequence instrument handler.
		\warning Because FDS instruments have no duty cycles or wave indices, the duty cycle
		parameter is unused.
		\param pInterface Pointer to the channel interface.
		\param Vol Default volume for instruments used by this handler.
		\param Duty Default duty cycle for instruments used by this handler. */
	CSeqInstHandlerFDS(CChannelHandlerInterface *pInterface, int Vol, int Duty) :
		CSeqInstHandler(pInterface, Vol, Duty) { }
	/*!	\brief Loads a new instrument into the instrument handler.
		\details This reimplementation calls the channel interface to write the contents of the
		instrument waveform to the FDS sound channel.
		\param pInst Pointer to the instrument to be loaded. */
	void LoadInstrument(std::shared_ptr<CInstrument> pInst) override;
	/*!	\brief Starts a new note for the instrument handler.
		\details This reimplementation calls the channel interface to write the contents of the
		instrument waveform to the FDS sound channel. */
	void TriggerInstrument() override;
	/*!	\brief Runs the instrument by one tick and updates the channel state.
		\details This reimplementation calls the channel interface to write the contents of the
		instrument waveform to the FDS sound channel. */
	void UpdateInstrument() override;

private:
	void UpdateTables(const CInstrumentFDS *pInst);
};
