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

class CChannelHandlerInterface;
class CInstrument;
class CSequence;
class CSeqInstrument;
class CInstrumentN163;

/*!
	\brief Class for N163 sequence instrument handlers.
	\details N163 instruments contain waveforms apart from sequence data.
*/
class CSeqInstHandlerN163 : public CSeqInstHandler
{
public:
	/*! \brief Constructor of the N163 sequence instrument handler.
		\warning As of now, the actual loading of instrument waveforms takes place in the channel handler
		rather than through its interface.
		\param pInterface Pointer to the channel interface.
		\param Vol Default volume for instruments used by this handler.
		\param Duty Default duty cycle for instruments used by this handler.
	*/
	CSeqInstHandlerN163(CChannelHandlerInterface *pInterface, int Vol, int Duty) :
		CSeqInstHandler(pInterface, Vol, Duty) {}
	/*! \brief Loads a new instrument into the instrument handler.
		\details This reimplementation sets up the wave size, wave position, and wave count of the channel
		handler.
		\param pInst Pointer to the instrument to be loaded.
	*/
	void LoadInstrument(CInstrument *pInst);
};
