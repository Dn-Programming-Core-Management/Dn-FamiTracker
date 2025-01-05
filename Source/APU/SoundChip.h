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
