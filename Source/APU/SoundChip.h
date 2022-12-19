/*
** FamiTracker - NES/Famicom sound tracker
** Copyright (C) 2005-2020 Jonathan Liss
**
** 0CC-FamiTracker is (C) 2014-2018 HertzDevil
**
** Dn-FamiTracker is (C) 2020-2022 D.P.C.M.
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

#include <cstdint>		// // //

class CMixer;
class CRegisterLogger;		// // //

class CSoundChip {
public:
	CSoundChip(CMixer *pMixer = nullptr);		// // //
	virtual ~CSoundChip();

	virtual void	Reset() = 0;
	virtual void	Process(uint32_t Time) = 0;
	virtual void	EndFrame() = 0;

	virtual void	Write(uint16_t Address, uint8_t Value) = 0;
	virtual uint8_t	Read(uint16_t Address, bool &Mapped) = 0;

	// TODO: unify with definitions in DetuneTable.cpp?
	virtual double	GetFreq(int Channel) const;		// // //

	virtual void	Log(uint16_t Address, uint8_t Value);		// // //
	CRegisterLogger *GetRegisterLogger() const;		// // //

protected:
	CMixer *m_pMixer;
	CRegisterLogger *m_pRegisterLogger;		// // //
};
