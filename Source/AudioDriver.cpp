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

#include "AudioDriver.h"
#include "DirectSound.h"

// 1kHz test tone
//#define AUDIO_TEST

// Enable audio dithering
//#define DITHERING

#ifdef DITHERING
int dither(long size);
#endif

CAudioDriver::CAudioDriver(IAudioCallback &Parent, std::unique_ptr<CDSoundChannel> pDevice, unsigned SampleSize) :
	m_Parent(Parent),
	m_pDSoundChannel(std::move(pDevice)),
	m_iSampleSize(SampleSize),
	m_iBufSizeBytes(m_pDSoundChannel ? m_pDSoundChannel->GetBlockSize() : 0),
	m_iBufSizeSamples(m_iBufSizeBytes / (SampleSize / 8)),
	m_pAccumBuffer(std::make_unique<char[]>(m_iBufSizeBytes)),		// // //
	m_iGraphBuffer(std::make_unique<int16_t[]>(m_iBufSizeSamples))
{
}

CAudioDriver::~CAudioDriver() {
	CloseAudioDevice();
}

void CAudioDriver::Reset() {
	m_iBufferPtr = 0;
	if (m_pDSoundChannel)
		m_pDSoundChannel->ClearBuffer();
}

void CAudioDriver::FlushBuffer(int16_t *pBuffer, uint32_t Size) {
	if (!m_pDSoundChannel)
		return;

	if (m_iSampleSize == 8)
		FillBuffer<uint8_t, 8>(pBuffer, Size);
	else
		FillBuffer<int16_t, 0>(pBuffer, Size);

	if (m_iClipCounter > 50) {
		// Ignore some clipping to allow the HP-filter adjust itself
		m_iClipCounter = 0;
		m_bAudioClipping = true;
	}
	else if (m_iClipCounter > 0)
		--m_iClipCounter;
}

bool CAudioDriver::PlayBuffer() {
	return m_Parent.PlayBuffer();
}

bool CAudioDriver::DoPlayBuffer() {
	static const int AUDIO_TIMEOUT = 2000;		// // // 2s buffer timeout

	// Output to direct sound
	DWORD dwEvent;

	// Wait for a buffer event
	while ((dwEvent = m_pDSoundChannel->WaitForSyncEvent(AUDIO_TIMEOUT)) != BUFFER_IN_SYNC) {
		switch (dwEvent) {
			case BUFFER_TIMEOUT:
				// Buffer timeout
				m_bBufferTimeout = true;
			case BUFFER_CUSTOM_EVENT:
				// Custom event, quit
				m_iBufferPtr = 0;
				return false;
			case BUFFER_OUT_OF_SYNC:
				// Buffer underrun detected
				m_iAudioUnderruns++;
				m_bBufferUnderrun = true;
				break;
		}
	}

	// Write audio to buffer
	auto [pBuf, size] = ReleaseSoundBuffer();		// // //
	m_pDSoundChannel->WriteBuffer(pBuf, size);

	// Reset buffer position
	m_bBufferTimeout = false;

	return true;
}

std::pair<char *, uint32_t> CAudioDriver::ReleaseSoundBuffer() {
	m_iBufferPtr = 0;
	return {m_pAccumBuffer.get(), m_iBufSizeBytes};
}

std::pair<int16_t *, uint32_t> CAudioDriver::ReleaseGraphBuffer() {
	return {m_iGraphBuffer.get(), m_iBufSizeSamples};
}

void CAudioDriver::CloseAudioDevice() {
	if (m_pDSoundChannel) {
		m_pDSoundChannel->Stop();
		m_pDSoundChannel.reset();		// // //
	}
}

bool CAudioDriver::IsAudioDeviceOpen() const {
	return static_cast<bool>(m_pDSoundChannel);
}

bool CAudioDriver::GetSoundTimeout() const {
	return m_bBufferTimeout;
}

bool CAudioDriver::DidBufferUnderrun() {
	return std::exchange(m_bBufferUnderrun, false);
}

bool CAudioDriver::WasAudioClipping() {
	return std::exchange(m_bAudioClipping, false);
}

unsigned CAudioDriver::GetUnderruns() const {
	return m_iAudioUnderruns;
}

template <class T, int SHIFT>
void CAudioDriver::FillBuffer(int16_t *pBuffer, uint32_t Size)
{
	// Called when the APU audio buffer is full and
	// ready for playing

	auto pConversionBuffer = reinterpret_cast<T *>(m_pAccumBuffer.get());		// // //

	for (uint32_t i = 0; i < Size; ++i) {
		int16_t Sample = pBuffer[i];

		// 1000 Hz test tone
#ifdef AUDIO_TEST
		static double sine_phase = 0;
		Sample = int32_t(sin(sine_phase) * 10000.0);

		static double freq = 1000;
		// Sweep
		//freq+=0.1;
		if (freq > 20000)
			freq = 20;

		sine_phase += freq / (double(m_pDSoundChannel->GetSampleRate()) / 6.283184);
		if (sine_phase > 6.283184)
			sine_phase -= 6.283184;
#endif /* AUDIO_TEST */

		// Clip detection
		if (Sample == std::numeric_limits<int16_t>::max() || Sample == std::numeric_limits<int16_t>::min())
			++m_iClipCounter;

		ASSERT(m_iBufferPtr < m_iBufSizeSamples);

		// Visualizer
		m_iGraphBuffer[m_iBufferPtr] = (short)Sample;

		// Convert sample and store in temp buffer
#ifdef DITHERING
		if (SHIFT > 0)
			Sample = (Sample + dither(1 << SHIFT)) >> SHIFT;
#else
		Sample >>= SHIFT;
#endif

		if (SHIFT == 8)
			Sample ^= 0x80;

		pConversionBuffer[m_iBufferPtr++] = (T)Sample;

		// If buffer is filled, throw it to direct sound
		if (m_iBufferPtr >= m_iBufSizeSamples) {
			if (!PlayBuffer())
				return;
		}
	}
}
