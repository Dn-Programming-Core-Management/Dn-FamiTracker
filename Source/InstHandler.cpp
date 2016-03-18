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

#include <memory> // TODO: remove
#include "stdafx.h"
#include "Instrument.h"
#include "InstHandler.h"
#include "ChannelHandlerInterface.h"

/*
 * Class CInstHandler
 */

CInstHandler::CInstHandler(CChannelHandlerInterface *pInterface, int Vol) :		// // //
	m_pInterface(pInterface),
	m_iNoteOffset(0),
	m_iVolume(Vol),
	m_iPitchOffset(0),
	m_iDefaultVolume(Vol),
	m_pInstrument(nullptr)
{
}

CInstHandler::~CInstHandler()
{
}

int CInstHandler::GetType(inst_type_t Type)
{
	switch (Type) {
	case INST_2A03: case INST_VRC6: case INST_S5B:
		return CInstHandler::TYPE_SEQ;
	case INST_N163:
		return CInstHandler::TYPE_N163;
	case INST_FDS:
		return CInstHandler::TYPE_FDS;
	case INST_VRC7:
		return CInstHandler::TYPE_VRC7;
	}
	return CInstHandler::TYPE_NONE;
}
