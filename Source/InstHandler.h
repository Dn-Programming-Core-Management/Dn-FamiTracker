/*
** FamiTracker - NES/Famicom sound tracker
** Copyright (C) 2005-2014  Jonathan Liss
**
** 0CC-FamiTracker is (C) 2014-2015 HertzDevil
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

class CChannelInterface;

/*!
	\brief Base class for instrument handlers.
	\details An instrument handler generates sound parameters on every tick, and controls the
	associated channel via the assigned channel interface. Its exact type is determined by the
	channel handler when creating a new handler for the currently played instrument; exactly one
	subclass must exist for each instrument type. In absence of an instrument handler, a channel
	receives no changes to its sound state from the current instrument.
*/
class CInstHandler {
protected:
	/*! \brief Constructor of the instrument handler. */
	CInstHandler(CChannelInterface *pInterface, int Vol);

public:
	/*! \brief Destructor of the instrument handler. */
	virtual ~CInstHandler();
	/*! \brief Loads a new instrument into the instrument handler.
		\details All relevant instrument parameters should be initialized in this method. This method
		might be called more than once from the same object, when a new instrument is issued by the
		channel handler, but the instrument type remains identical. This method is never called when
		the same instrument is used on successive notes.
		\param pInst Pointer to the instrument to be loaded.
		\sa CChannelHandler::CreateInstHandler
	*/
	virtual void LoadInstrument(CInstrument *pInst) = 0;
	/*! \brief Runs the instrument by one tick and updates the channel state.
		\details The channel handler calls this method on every tick to allow continuous control of
		the channel state from the instrument handler.
	*/
	virtual void UpdateInstrument() = 0;
	/*! \brief Starts a new note for the instrument handler.
		\details The next call to CInstHandler::UpdateInstrument should generate the first tick of
		the instrument or a suitable default sound state.
	*/
	virtual void TriggerInstrument() = 0;
	/*! \brief Releases the current note for the instrument handler.
		\details A note can only be released once until another new note is triggered.
	*/
	virtual void ReleaseInstrument() = 0;

protected:
	/*! \brief An interface to the underlying channel handler.
		\details The instrument handler may control the channel only through methods provided by
		this interface or any subclass of CChannelInterface.
	*/
	CChannelInterface *m_pInterface;
	/*! \brief A const pointer to the current instrument used by this instrument handler.
		\warning This pointer does not participate in reference counting provided by CRefCounter.
	*/
	const CInstrument *m_pInstrument;
	/*! \brief The current volume of the instrument.
		\warning Currently unused.
	*/
	int m_iVolume;
	/*! \brief The current note value of the instrument.
		\warning Currently unused.
	*/
	int m_iNoteOffset;
	/*! \brief The current pitch deviation of the instrument.
		\details The pitch offset is handled in the same way as the fine pitch offset of the
		underlying channel handler.
		\warning Currently unused.
	*/
	int m_iPitchOffset;
	/*! \brief The default volume of the instrument.
		\details On triggering a new note, the instrument volume is reset to this value.
		\warning Currently unused.
	*/
	const int m_iDefaultVolume;
};
