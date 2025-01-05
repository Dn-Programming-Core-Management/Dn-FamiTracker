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

#include "InstHandler.h"

class CSequence;
class CSeqInstrument;

/*!
	\brief Class for sequence instrument handlers.
	\details Sequence instruments are the major type of instruments provided by FamiTracker;
	they are used by all sound channels except for the Konami VRC7.
*/
class CSeqInstHandler : public CInstHandler
{
public:
	/*!	\brief Constants representing the state of each sequence. */
	enum seq_state_t {		// // //
		SEQ_STATE_DISABLED, /*!< Current sequence is not enabled. */
		SEQ_STATE_RUNNING,	/*!< Current sequence is running. */
		SEQ_STATE_END,		/*!< Current sequence has just finished running the last tick. */
		SEQ_STATE_HALT		/*!< Current sequence has finished running until the next note. */
	};

	/*!	\brief Constructor of the sequence instrument handler.
		\details A default duty value must be provided in the parameters.
		\param pInterface Pointer to the channel interface.
		\param Vol Default volume for instruments used by this handler.
		\param Duty Default duty cycle for instruments used by this handler. */
	CSeqInstHandler(CChannelHandlerInterface *pInterface, int Vol, int Duty);

	void LoadInstrument(std::shared_ptr<CInstrument> pInst) override;
	void TriggerInstrument() override;
	void ReleaseInstrument() override;
	void UpdateInstrument() override;

	/*!	\brief Obtains the current sequence state of a given sequence type.
		\param Index The sequence type, which should be a member of sequence_t.
		\return The sequence state of the given sequence type. */
	seq_state_t GetSequenceState(int Index) const { return m_iSeqState[Index]; }

protected:
	/*!	\brief Processes the value retrieved from a sequence.
		\return True if the sequence has finished processing.
		\param Index The sequence type.
		\param Setting The sequence setting.
		\param Value The sequence value to be processed. */
	virtual bool ProcessSequence(int Index, unsigned Setting, int Value);

	/*!	\brief Prepares a sequence type for use by CSeqInstHandler::UpdateInstrument.
		\param Index The sequence type.
		\param pSequence Pointer to the sequence. */
	virtual void SetupSequence(int Index, const CSequence *pSequence);

	/*!	\brief Clears a sequence type from use.
		\param Index The sequence type. */
	virtual void ClearSequence(int Index);

protected:
	/*!	\brief An array holding pointers to the sequences used by the current instrument. */
	const CSequence	*m_pSequence[SEQ_COUNT];
	/*!	\brief An array holding the states of each sequence type used in sequence instruments. */
	seq_state_t		m_iSeqState[SEQ_COUNT];
	/*!	\brief An array holding the tick index of each sequence type used in sequence instruments. */
	int				m_iSeqPointer[SEQ_COUNT];
	/*!	\brief The current duty cycle of the instrument.
		\details The exact interpretation of this member may not be identical across sound channels.
		\warning Currently unused. */
	int				m_iDutyParam;
	/*!	\brief The default duty cycle of the instrument.
		\details On triggering a new note, the duty cycle value is reset to this value.
		\warning Currently unused. */
	const int		m_iDefaultDuty;
};
