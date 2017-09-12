/*
** FamiTracker - NES/Famicom sound tracker
** Copyright (C) 2005-2014  Jonathan Liss
**
** 0CC-FamiTracker is (C) 2014-2017 HertzDevil
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

#include <cstdint>
#include <memory>
#include <utility>
#include "Common.h"

class CDSoundChannel;

class CAudioDriver : public IAudioCallback {
public:
	CAudioDriver(const CAudioDriver &) = delete;
	virtual ~CAudioDriver();

	CAudioDriver(IAudioCallback &Parent, std::unique_ptr<CDSoundChannel> pDevice, unsigned SampleSize);

	void Reset();
	void FlushBuffer(int16_t *Buffer, uint32_t Size) override;
	bool PlayBuffer() override;
	bool DoPlayBuffer();
	std::pair<char *, uint32_t> ReleaseSoundBuffer();
	std::pair<int16_t *, uint32_t> ReleaseGraphBuffer();

	void CloseAudioDevice();
	bool IsAudioDeviceOpen() const;

	bool GetSoundTimeout() const;
	bool DidBufferUnderrun();
	bool WasAudioClipping();
	unsigned GetUnderruns() const;

private:
	template <class T, int SHIFT>
	void FillBuffer(int16_t *Buffer, uint32_t Size);

private:
	std::unique_ptr<CDSoundChannel> m_pDSoundChannel;		// // // directsound channel
	IAudioCallback		&m_Parent;							// // //

	unsigned int		m_iSampleSize;						// Size of samples, in bits
	unsigned int		m_iBufSizeBytes = 0;				// Buffer size in bytes
	unsigned int		m_iBufSizeSamples = 0;				// Buffer size in samples
	unsigned int		m_iBufferPtr = 0;					// This will point in samples
	std::unique_ptr<char[]> m_pAccumBuffer;					// // //
	std::unique_ptr<int16_t[]> m_iGraphBuffer;
	unsigned int		m_iAudioUnderruns = 0;				// Keep track of underruns to inform user
	bool				m_bBufferTimeout = false;
	bool				m_bBufferUnderrun = false;
	bool				m_bAudioClipping = false;
	unsigned int		m_iClipCounter = 0;
};
