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

#include "Blip_Buffer/Blip_Buffer.h"

#include "gsl/span"
#include <cstdint>		// // //
#include <memory>

class CRegisterLogger;		// // //
class Blip_Buffer;

class CSoundChip2 {
public:
	CSoundChip2();		// // //
	virtual ~CSoundChip2() = default;

	virtual void	Reset() = 0;
	virtual void UpdateFilter(blip_eq_t eq) = 0;

	/// The empty default implementation is sufficient
	/// unless your CSoundChip2 subclass owns its own Blip_Buffer (not just Blip_Synth).
	///
	/// tbh the proliferation of mutable state with setters is evil,
	/// I'd much rather set clock rate as a constructor parameter
	/// and tear down all sound chips when it changes.
	virtual void SetClockRate(uint32_t Rate) {}

	/// Advance the sound chip emulator.
	///
	/// - Time is the number of clock cycles to advance.
	/// - Output is where audio will be written to.
	virtual void	Process(uint32_t Time, Blip_Buffer& Output) = 0;

	/// End an audio frame/tick.
	/// Each subclass of CSoundChip2 can choose to write audio to Output
	/// on every call to Process(), or on the final call to EndFrame().
	///
	/// - Output is where audio will be written to.
	/// - TempBuffer can be overwritten freely, and the contents will be discarded
	///   after the function returns.
	virtual void	EndFrame(Blip_Buffer& Output, gsl::span<int16_t> TempBuffer) = 0;

	virtual void	Write(uint16_t Address, uint8_t Value) = 0;
	virtual uint8_t	Read(uint16_t Address, bool &Mapped) = 0;

	virtual double	GetFreq(int Channel) const;		// // //

	virtual void	Log(uint16_t Address, uint8_t Value);		// // //
	CRegisterLogger *GetRegisterLogger() const;		// // //

protected:
	std::unique_ptr<CRegisterLogger> m_pRegisterLogger;		// // //
};
