/*
** FamiTracker - NES/Famicom sound tracker
** Copyright (C) 2005-2015 Jonathan Liss
**
** 0CC-FamiTracker is (C) 2014-2018 HertzDevil
**
** Dn-FamiTracker is (C) 2020-2021 D.P.C.M.
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

#ifndef SOUNDINTERFACE_H
#define SOUNDINTERFACE_H

#include <cstdint>
#include <memory>
#include <wrl/client.h>
#include "str_conv/str_conv.hpp"
#include "utils/handle_ptr.h"

using Microsoft::WRL::ComPtr;

// Return values from CSoundStream::WaitForReady()
enum class WaitResult {
	InternalError = 0,

	/// Stop playback, etc.
	Interrupted = 1,

	/// Audio stream stuck, can't play audio
	Timeout,

	/// Ready to write data
	Ready,

	/// TODO how to identify underruns?
	OutOfSync,
};

struct IAudioClient;
struct IAudioRenderClient;

enum class StreamState {
	Stopped,
	ReadyToStart,
	Started,
};

// Audio output stream to a device
class CSoundStream
{
	friend class CSoundInterface;

public:
	CSoundStream(
		ComPtr<IAudioClient> pAudioClient,
		ComPtr<IAudioRenderClient> pAudioRenderClient,
		HANDLE hInterrupt,
		HandlePtr bufferEvent,
		unsigned int iSampleRate,
		unsigned int bufferFrameCount,
		unsigned int bytesPerSample,
		unsigned int inputChannels,
		unsigned int outputChannels);
	~CSoundStream();

	// State changes
	bool Play();
	bool Stop();
	bool ClearBuffer();

	// Buffer calculations

	uint32_t PubBytesToFrames(uint32_t Bytes) const;
	uint32_t FramesToPubBytes(uint32_t Frames) const;

	uint32_t TotalBufferSizeFrames() const;

	/// Get public/input buffer size. If upmixing mono to stereo, this is mono.
	uint32_t TotalBufferSizeBytes() const;

	uint32_t GetSampleRate() const {
		return m_iSampleRate;
	};

	// Steady-state

	/// Automatically starts the stream when enough audio is buffered.
	///
	/// If SkipIfWritable is true (after a partial write), return immediately if there is
	/// already room to write audio.
	WaitResult WaitForReady(DWORD dwTimeout, bool SkipIfWritable);

	uint32_t BufferFramesWritable() const;
	uint32_t BufferBytesWritable() const;

	bool WriteBuffer(float const * pBuffer, unsigned int Bytes);

private:
	// m_bufferEvent should outlive IAudioClient probably, so list it first.
	HandlePtr m_bufferEvent;

	ComPtr<IAudioClient> m_pAudioClient;
	ComPtr<IAudioRenderClient> m_pAudioRenderClient;
	StreamState m_state;

	/// Owned by CSoundGen, borrowed by CSoundStream.
	HANDLE m_hInterrupt;

	/// Returned from AvSetMmThreadCharacteristicsW(), passed into
	/// AvRevertMmThreadCharacteristics()
	HANDLE  m_hTask;

	// Configuration
	unsigned int m_iSampleRate;
	unsigned int m_bufferFrameCount;
	unsigned int m_bytesPerSample;

	// Public, picked by user, 1 for mono sound.
	unsigned int m_inputChannels;

	// Private, generally 2 or greater since WASAPI shared mode doesn't support mono.
	unsigned int m_outputChannels;
};

struct IMMDeviceEnumerator;
struct IMMDeviceCollection;
struct IMMDevice;

// WASAPI interface
class CSoundInterface
{
public:
	CSoundInterface(HANDLE hNotification);
	~CSoundInterface();

	// Listing devices
	void			EnumerateDevices();
	void			ClearEnumeration();
	unsigned int	GetDeviceCount() const;
	conv::tstring	GetDeviceName(unsigned int iDevice) const;

	// Picking active device
	bool			SetupDevice(int iDevice);
	void			CloseDevice();

	// Opening streams for active device

	/// May return a CSoundStream with a different sampling rate than specified. Call
	/// CSoundStream::GetSampleRate() to get the actual rate.
	///
	/// Always returns a CSoundStream with the same channel count as provided. If Channels
	/// = 1 and not supported by WASAPI (on Windows, possibly Wine), we accept 1ch audio
	/// and upmix to 2ch before sending to WASAPI.
	CSoundStream	*OpenFloatChannel(
		int TargetSampleRate, int Channels, int BufferLength, int Blocks
	);
	void			CloseChannel(CSoundStream *pChannel);

	// Utility
	static int		CalculateBufferLength(int BufferLen, int Samplerate, int Samplesize, int Channels);

public:
	static const unsigned int MAX_DEVICES = 256;
	static const unsigned int MAX_BLOCKS = 16;
	static const unsigned int MAX_SAMPLE_RATE = 96000;
	static const unsigned int MAX_BUFFER_LENGTH = 10000;

private:
	/// Owned by CSoundGen, borrowed by CSoundInterface.
	HANDLE m_hInterrupt;

	// For enumeration
	ComPtr<IMMDeviceEnumerator> m_maybeDeviceEnumerator;

	// In addition to the physical devices present, we expose a virtual device 0 named
	// "Default Device". So all externally exposed device IDs must be converted to a raw ID
	// by adding 1.
	ComPtr<IMMDeviceCollection> m_maybeRawDeviceList;

	ComPtr<IMMDevice> m_maybeDevice;
};

#endif /* SOUNDINTERFACE_H */
