/*
** FamiTracker - NES/Famicom sound tracker
** Copyright (C) 2005-2020 Jonathan Liss
**
** 0CC-FamiTracker is (C) 2014-2018 HertzDevil
**
** Dn-FamiTracker is (C) 2020-2024 D.P.C.M.
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

//
// Sound Interface
//

#include "stdafx.h"
#include <cstdio>
#include <utility>  // std::move
#include "Common.h"
#include "SoundInterface.h"
#include "../resource.h"

// WASAPI headers
#define WIN32_LEAN_AND_MEAN
#include <synchapi.h>  // CreateEventW
#include <initguid.h>  // Possibly needed for GUIDs?
#include <avrt.h>  // AvSetMmThreadCharacteristicsW
#include <mmdeviceapi.h>  // IMMDeviceEnumerator
#include <audioclient.h>  // IAudioClient
#include <Functiondiscoverykeys_devpkey.h>  // PKEY_Device_FriendlyName

// Instance members

CSoundInterface::CSoundInterface(HANDLE hInterrupt) :
	m_hInterrupt(hInterrupt)
{
	// Based off:
	// https://github.com/wareya/AudioLibraryRosettaStone/blob/master/wasapi.cpp#L52
	// https://github.com/thestk/rtaudio/blob/e9b1d0262a5e75e09c510eb9c5825daf86884d29/RtAudio.cpp#L4320

	// Instantiate device enumerator
	if (FAILED(CoCreateInstance(__uuidof(MMDeviceEnumerator), NULL,
		CLSCTX_ALL, __uuidof(IMMDeviceEnumerator),
		(void**)m_maybeDeviceEnumerator.GetAddressOf()))
	) {
		return;
	}
}

CSoundInterface::~CSoundInterface()
{
}

void CSoundInterface::EnumerateDevices()
{
	// It is probably safe to construct m_maybeDeviceEnumerator: *IMMDeviceEnumerator
	// in the constructor on the main thread, Send it to the audio thread, and access
	// it from methods called there: https://github.com/RustAudio/cpal/pull/597

	// Populate device list.
	if (!m_maybeDeviceEnumerator) {
		return;
	}

	// https://github.com/thestk/rtaudio/blob/e9b1d0262a5e75e09c510eb9c5825daf86884d29/RtAudio.cpp#L4379-L4390
	HRESULT hr = m_maybeDeviceEnumerator->EnumAudioEndpoints(
		eRender, DEVICE_STATE_ACTIVE, m_maybeRawDeviceList.ReleaseAndGetAddressOf()
	);
	if (FAILED(hr)) return;
}

void CSoundInterface::ClearEnumeration()
{
	// Clear device list.
	m_maybeRawDeviceList.Reset();
}

unsigned int CSoundInterface::GetDeviceCount() const
{
	if (!m_maybeRawDeviceList) {
		return 0;
	}
	UINT RawDeviceCount;
	HRESULT hr = m_maybeRawDeviceList->GetCount(&RawDeviceCount);
	if (FAILED(hr)) {
		return 0;
	}

	// External device 0 is "Default Device".
	return RawDeviceCount + 1;
}

// https://docs.microsoft.com/en-us/windows/uwp/cpp-and-winrt-apis/author-coclasses#add-helper-types-and-functions
struct PropVariant : PROPVARIANT
{
	PropVariant() noexcept : PROPVARIANT{}
	{
	}

	~PropVariant() noexcept
	{
		clear();
	}

	void clear() noexcept
	{
		// Don't check the return value. It should be fine, I hope...
		PropVariantClear(this);
	}
};

static conv::tstring GetDevicePtrName(IMMDevice * pDevice) {
	// https://github.com/thestk/rtaudio/blob/e9b1d0262a5e75e09c510eb9c5825daf86884d29/RtAudio.cpp#L4522-L4530
	ComPtr<IPropertyStore> pDevicePropStore;
	auto hr = pDevice->OpenPropertyStore(STGM_READ, pDevicePropStore.GetAddressOf());
	if (FAILED(hr)) return _T("Error: opening device properties");

	PropVariant deviceNameProp;
	hr = pDevicePropStore->GetValue(PKEY_Device_FriendlyName, &deviceNameProp);
	if (FAILED(hr)) return _T("Error: getting device name");

	return conv::to_t(deviceNameProp.pwszVal);
}

conv::tstring CSoundInterface::GetDeviceName(unsigned int iDevice) const
{
	ASSERT(iDevice < GetDeviceCount());

	ComPtr<IMMDevice> pDevice;

	if (iDevice == 0) {
		if (!m_maybeDeviceEnumerator) {
			return _T("Error: device enumerator missing");
		}

		// https://github.com/thestk/rtaudio/blob/e9b1d0262a5e75e09c510eb9c5825daf86884d29/RtAudio.cpp#L4493-L4513
		auto hr = m_maybeDeviceEnumerator->GetDefaultAudioEndpoint(
			eRender, eConsole, pDevice.GetAddressOf()
		);
		if (FAILED(hr)) return _T("Error: getting default device");

		auto name = GetDevicePtrName(pDevice.Get());
		return _T("Default Device (") + name + _T(")");
	} else {
		UINT rawDevice = iDevice - 1;

		if (!m_maybeRawDeviceList) {
			return _T("Error: device list missing");
		}
		auto hr = m_maybeRawDeviceList->Item(rawDevice, pDevice.GetAddressOf());
		if (FAILED(hr)) return _T("Error: getting device");

		return GetDevicePtrName(pDevice.Get());
	}
}

bool CSoundInterface::SetupDevice(int iDevice)
{
	// Replace out-of-bounds devices with default device.
	if (iDevice >= 1 && (unsigned)(iDevice - 1) >= GetDeviceCount()) {
		iDevice = 0;
	}

	m_maybeDevice.Reset();

	if (!m_maybeDeviceEnumerator) {
		return false;
	}
	if (iDevice == 0) {
		auto hr = m_maybeDeviceEnumerator->GetDefaultAudioEndpoint(
			eRender, eConsole, m_maybeDevice.GetAddressOf()
		);
		if (FAILED(hr)) return false;
	}
	else {
		UINT rawDevice = iDevice - 1;
		if (!m_maybeRawDeviceList) {
			return false;
		}
		auto hr = m_maybeRawDeviceList->Item(rawDevice, m_maybeDevice.GetAddressOf());
		if (FAILED(hr)) return false;
	}

	return true;
}

void CSoundInterface::CloseDevice()
{
	m_maybeDevice.Reset();
	m_maybeRawDeviceList.Reset();
}

// static method, helper function
int CSoundInterface::CalculateBufferLength(int BufferLen, int Samplerate, int Samplesize, int Channels)
{
	// Calculate size of the buffer, in bytes
	return ((Samplerate * BufferLen) / 1000) * (Samplesize / 8) * Channels;
}

CSoundStream *CSoundInterface::OpenFloatChannel(const int InputChannels, int BufferLength) {
	// Based off https://docs.microsoft.com/en-us/windows/win32/coreaudio/exclusive-mode-streams
	if (!m_maybeDevice) {
		return nullptr;
	}
	ComPtr<IAudioClient> pAudioClient;
	auto hr = m_maybeDevice->Activate(
		__uuidof(IAudioClient), CLSCTX_ALL, NULL, (void**)pAudioClient.GetAddressOf()
	);
	if (FAILED(hr)) return nullptr;

	// Set sample rate to mix format. This is necessary on Windows since WASAPI rejects
	// non-system sampling rates, and a good idea on Linux since Wine delegates resampling
	// to PulseAudio/PipeWire, and PulseAudio uses a low-quality resampler by default
	// (https://gitlab.freedesktop.org/pulseaudio/pulseaudio/-/issues/310).
	DWORD SampleRate{};
	int OutputChannels{};
	DWORD outChannelMask{};
	
	{
		WAVEFORMATEX* pMixFormat{};
		hr = pAudioClient->GetMixFormat(&pMixFormat);
		if (FAILED(hr)) {
			CoTaskMemFree(pMixFormat);
			return nullptr;
		}
		SampleRate = pMixFormat->nSamplesPerSec;
		OutputChannels = (int) pMixFormat->nChannels;

		// https://learn.microsoft.com/en-us/windows/win32/api/audioclient/nf-audioclient-iaudioclient-getmixformat
		// "the GetMixFormat method retrieves a format descriptor that is in the
		// form of a WAVEFORMATEXTENSIBLE structure instead of a standalone
		// WAVEFORMATEX structure. Through the ppDeviceFormat parameter, the method
		// outputs a pointer to the WAVEFORMATEX structure that is embedded at the
		// start of this WAVEFORMATEXTENSIBLE structure."
		if (pMixFormat->wFormatTag != WAVE_FORMAT_EXTENSIBLE) {
			TRACE("GetMixFormat() wFormatTag=%x not WAVE_FORMAT_EXTENSIBLE!", pMixFormat->wFormatTag);
			CoTaskMemFree(pMixFormat);
			return nullptr;
		}
		ASSERT(pMixFormat->cbSize >= 22);
		auto mixFormatExt = (WAVEFORMATEXTENSIBLE*)pMixFormat;
		outChannelMask = mixFormatExt->dwChannelMask;

		CoTaskMemFree(pMixFormat);
	}

	// Can't use designated initializers since we're not in C++20 mode.
	WAVEFORMATEXTENSIBLE format{}; // Format.cbSize = 22
	
	constexpr WORD FLOAT32_BITS = 32;
	{
		// Bytes per frame.
		auto blockAlign = (WORD)(OutputChannels * FLOAT32_BITS / 8);

		// https://docs.microsoft.com/en-us/windows/win32/api/mmeapi/ns-mmeapi-waveformatex#members
		format.Format.wFormatTag = WAVE_FORMAT_EXTENSIBLE;
		format.Format.nChannels = OutputChannels;
		format.Format.nSamplesPerSec = SampleRate;
		format.Format.nAvgBytesPerSec = (DWORD)(SampleRate * blockAlign);
		format.Format.nBlockAlign = blockAlign;
		format.Format.wBitsPerSample = FLOAT32_BITS;
		
		// Both WAVEFORMATEX and WAVEFORMATEXTENSIBLE (and the entirety of mmreg.h)
		// are packed (alignof=1).
		format.Format.cbSize = 22;

		// IEEE_FLOAT is uncompressed, so set wValidBitsPerSample.
		// https://www.ambisonic.net/mulchaud.html#_Toc446153097
		format.Samples.wValidBitsPerSample = FLOAT32_BITS;
		format.dwChannelMask = outChannelMask;

		format.SubFormat = KSDATAFORMAT_SUBTYPE_IEEE_FLOAT;
	};

	auto pFormat = (WAVEFORMATEX*)&format;

	// Ensure that chosen audio format is supported.
	WAVEFORMATEX* pClosestMatch{};
	hr = pAudioClient->IsFormatSupported(AUDCLNT_SHAREMODE_SHARED, pFormat, &pClosestMatch);

	// So far we don't need to handle audio format fallback, since Windows 7/10 and
	// Wine all support float32 audio.
	// If the audio format is still unsupported, give up and show an error to the user.
	if (hr != S_OK) {
		CoTaskMemFree(pClosestMatch);
		return nullptr;
	}

	// Convert BufferLength (ms) to REFERENCE_TIME (100ns).
	REFERENCE_TIME bufferLengthTime = (LONGLONG)BufferLength * 10'000;

	// Shared-mode WASAPI (excluding IAudioClient3::InitializeSharedAudioStream())
	// creates one buffer of length bufferLengthTime, and lends out random portions
	// to be written by the program. So don't divide buffer length by 2.

	// Open stream.
	// The actual buffer size is bufferLengthTime or (on my machine) 22 ms, whichever is
	// greater. It is legal to call IAudioClient::Initialize(hnsBufferDuration=0),
	// causing WASAPI to pick a buffer size automatically
	// (https://docs.microsoft.com/en-us/windows/win32/api/audioclient/nf-audioclient-iaudioclient-initialize#:~:text=shared-mode%20stream%20that%20uses%20event-driven%20buffering).
	// Passing in too-short sizes like 1ms automatically clamps up to 22ms on Windows 10
	// and Wine, but hangs on Windows 7.
	hr = pAudioClient->Initialize(
		AUDCLNT_SHAREMODE_SHARED,
		AUDCLNT_STREAMFLAGS_EVENTCALLBACK,
		bufferLengthTime,
		0,  // periodicity must be 0 for shared streams
		pFormat,
		nullptr);  // We ignore GUID.
	if (FAILED(hr)) return nullptr;

	// Setup buffer event.
	auto bufferEvent = HandlePtr(CreateEventW(nullptr, FALSE, FALSE, nullptr));
	hr = pAudioClient->SetEventHandle(bufferEvent.get());
	if (FAILED(hr)) return nullptr;

	// Get the actual size of buffer.
	UINT32 bufferFrameCount;
	hr = pAudioClient->GetBufferSize(&bufferFrameCount);
	if (FAILED(hr)) return nullptr;

	// Open stream's buffer writing interface.
	ComPtr<IAudioRenderClient> pAudioRenderClient;
	hr = pAudioClient->GetService(
		__uuidof(IAudioRenderClient), (void**)pAudioRenderClient.GetAddressOf()
	);
	if (FAILED(hr)) return nullptr;

	// We want to generate one buffer of audio from CSoundGen before starting IAudioClient.
	// This is easiest if CSoundStream calls CSoundGen to generate audio, but instead
	// CSoundGen pushes/waits on CSoundStream. So instead we return a stopped stream,
	// return immediately from the first call to CSoundStream::WaitForReady(),
	// and start the stream on the *second* CSoundStream::WaitForReady() call.
	//
	// ~~what if CSoundGen and CSoundStream were coroutines~~

	return new CSoundStream(
		std::move(pAudioClient),
		std::move(pAudioRenderClient),
		m_hInterrupt,
		std::move(bufferEvent),
		SampleRate,
		bufferFrameCount,
		4,  // bytesPerSample = 4 for float
		InputChannels,
		OutputChannels);
}

void CSoundInterface::CloseChannel(CSoundStream *pSoundStream)
{
	if (pSoundStream == NULL)
		return;

	delete pSoundStream;
}

// CSoundStream

CSoundStream::CSoundStream(
	ComPtr<IAudioClient> pAudioClient,
	ComPtr<IAudioRenderClient> pAudioRenderClient,
	HANDLE hInterrupt,
	HandlePtr bufferEvent,
	unsigned int iSampleRate,
	unsigned int bufferFrameCount,
	unsigned int bytesPerSample,
	unsigned int inputChannels,
	unsigned int outputChannels)
:
	m_bufferEvent(std::move(bufferEvent)),
	m_pAudioClient(std::move(pAudioClient)),
	m_pAudioRenderClient(std::move(pAudioRenderClient)),
	m_state(StreamState::Stopped),
	m_hInterrupt(hInterrupt),
	m_hTask(nullptr),
	m_iSampleRate(iSampleRate),
	m_bufferFrameCount(bufferFrameCount),
	m_bytesPerSample(bytesPerSample),
	m_inputChannels(inputChannels),
	m_outputChannels(outputChannels)
{
	ASSERT(m_inputChannels == 1);
}

CSoundStream::~CSoundStream()
{
	if (m_hTask) {
		// AvRevertMmThreadCharacteristics must be called on the same thread as
		// AvSetMmMaxThreadCharacteristics. This is currently the case, but still
		// makes me uneasy.
		AvRevertMmThreadCharacteristics(m_hTask);
	}
}

bool CSoundStream::Play()
{
	// "To avoid start-up glitches with rendering streams, clients should not call Start
	// until the audio engine has been initially loaded with data by calling the
	// IAudioRenderClient::GetBuffer and IAudioRenderClient::ReleaseBuffer methods
	// on the rendering interface."

	auto hr = m_pAudioClient->Start();
	if (FAILED(hr)) return false;

	m_state = StreamState::Started;

	// Called on the audio thread.
	// Ask MMCSS to temporarily boost the thread priority to reduce glitches
	// while the low-latency stream plays.
	DWORD taskIndex = 0;
	if (m_hTask == nullptr) {
		// AvSetMmThreadCharacteristicsW is unimplemented on Wine (prints fixme), but
		// returns a handle anyway.
		m_hTask = AvSetMmThreadCharacteristicsW(L"Pro Audio", &taskIndex);
		TRACE("AvSetMmThreadCharacteristicsW success: %d", m_hTask != 0);
	}

	return true;
}

bool CSoundStream::Stop()
{
	// Only called by CSoundGen::CloseAudioDevice(), before deleting CSoundStream.
	// Exact behavior is unimportant.
	auto hr = m_pAudioClient->Stop();
	if (FAILED(hr)) return false;

	m_state = StreamState::Stopped;

	// Called on the audio thread.
	if (m_hTask) {
		// I could check the return value (failed = FALSE) and return it...
		// but I don't want CSoundStream::ClearBuffer() -> CSoundStream::Stop() to fail
		// if m_pAudioClient->Stop() succeeds but AvRevertMmThreadCharacteristics fails.
		AvRevertMmThreadCharacteristics(m_hTask);
		m_hTask = nullptr;
	}

	return true;
}

bool CSoundStream::ClearBuffer()
{
	// This function is only called when starting or stopping WAV export:
	// CSoundGen::OnStartRender()/StopRendering() -> CSoundGen::ResetBuffer() ->
	// CSoundStream::ClearBuffer().
	//
	// I'm not sure if it's even necessary to stop the output stream during
	// WAV export, though it avoids underruns (if we actually reported underruns
	// properly).

	if (m_state == StreamState::Started)
		if (!Stop())
			return false;

	auto hr = m_pAudioClient->Reset();
	if (FAILED(hr)) return false;

	return true;
}

// Buffering

uint32_t CSoundStream::PubBytesToFrames(uint32_t Bytes) const
{
	return Bytes / m_bytesPerSample / m_inputChannels;
}

uint32_t CSoundStream::FramesToPubBytes(uint32_t Frames) const
{
	return Frames * m_bytesPerSample * m_inputChannels;
}

uint32_t CSoundStream::TotalBufferSizeFrames() const {
	return m_bufferFrameCount;
}

uint32_t CSoundStream::TotalBufferSizeBytes() const {
	return FramesToPubBytes(m_bufferFrameCount);
}

// Steady-state

WaitResult CSoundStream::WaitForReady(DWORD dwTimeout, bool SkipIfWritable)
{
	auto onInterrupted = []() {
		return WaitResult::Interrupted;
	};

	auto onWritable = []() {
		// I don't know of any way to detect playback buffer underruns in WASAPI,
		// to return WaitResult::OutOfSync.
		// https://stackoverflow.com/q/32112562 has no answer, and
		// https://github.com/mackron/miniaudio/issues/81 couldn't figure it out either.
		return WaitResult::Ready;
	};

	// Check for cancellation upfront.
	if (WaitForSingleObject(m_hInterrupt, 0) == WAIT_OBJECT_0) {
		return onInterrupted();
	}

	// Check for special cases.
	if (m_state == StreamState::Stopped) {
		// The first few times CSoundGen waits to write audio, don't start stream playback.
		// Instead return and let CSoundGen write audio. (At this point,
		// CSoundStream::BufferFramesWritable() returns the full buffer size.)
		//
		// When WriteBuffer() is called, if m_state == StreamState::Stopped and the buffer is
		// full, it calls Play() which sets m_state = StreamState::Started.
		return WaitResult::Ready;
	}
	// TODO we can get marginally less latency by having CSoundGen call
	// CSoundStream::WaitForReady before generating audio, rather than before
	// converting/buffering it.

	// Check if we can write audio without waiting at all (which is the case if the
	// previous WriteBuffer() didn't fill the whole buffer).
	//
	// SkipIfWritable=false (after a full write) causes WaitForReady() to block if WASAPI
	// hasn't set m_bufferEvent, but BufferFramesWritable() > 0. I've never seen this
	// happen after a full write, on either Windows or Wine. So not checking
	// BufferFramesWritable() if SkipIfWritable=false isn't needed to avoid an endless
	// loop of writing 1 sample at a time, but saves an IAudioClient::GetCurrentPadding()
	// call.
	if (SkipIfWritable && BufferFramesWritable()) {
		return onWritable();
	}

	// Wait for events
	HANDLE waitEvents[2] = { m_hInterrupt, m_bufferEvent.get() };

	switch (WaitForMultipleObjects(2, waitEvents, FALSE, dwTimeout)) {
	case WAIT_OBJECT_0:  // hInterrupt: interrupted by GUI
		return onInterrupted();

	case WAIT_OBJECT_0 + 1:  // m_bufferEvent: WASAPI buffer ready to write
		return onWritable();

	case WAIT_TIMEOUT:  // Timeout
		return WaitResult::Timeout;

	default:  // Error
		return WaitResult::InternalError;
	}
}

unsigned int CSoundStream::BufferFramesWritable() const {
	UINT32 numFramesPadding;
	auto hr = m_pAudioClient->GetCurrentPadding(&numFramesPadding);
	if (FAILED(hr)) return 0;

	UINT32 numFramesAvailable = m_bufferFrameCount - numFramesPadding;
	// TRACE("%d frames available of %d\n", numFramesAvailable, m_bufferFrameCount);
	return numFramesAvailable;
}

uint32_t CSoundStream::BufferBytesWritable() const {
	return FramesToPubBytes(BufferFramesWritable());
}

bool CSoundStream::WriteBuffer(float const* pSrcBuffer, unsigned int Bytes)
{
	// Bytes comes from CSoundStream::BufferBytesWritable().
	const unsigned int nFrames = PubBytesToFrames(Bytes);

	BYTE* pOutData = nullptr;
	auto hr = m_pAudioRenderClient->GetBuffer(nFrames, &pOutData);
	if (FAILED(hr)) return false;

	// https://johnnysswlab.com/decreasing-the-number-of-memory-accesses-the-compilers-secret-life-2-2/ idk
	const auto nOutputChannels = m_outputChannels;
	if (nOutputChannels >= 2) {
		ASSERT(m_inputChannels == 1);
		ASSERT(m_bytesPerSample == sizeof(float));

		// Upmix mono input to stereo output.
		auto  *__restrict inputSpan = (float const*)pSrcBuffer;  // idx < nFrames * (m_inputChannels=1)
		auto *__restrict outputSpan = (float *)pOutData;  // idx < nFrames * nOutputChannels

		// If output has over 2 channels, fill trailing channels with silence.
		if (nOutputChannels > 2) {
			std::fill(outputSpan, outputSpan + nFrames * nOutputChannels, 0.f);
		}

		// Fill first 2 output channels with single input channel.
		for (size_t frame = 0; frame < nFrames; frame++) {
			outputSpan[(frame) * nOutputChannels]
				= outputSpan[(frame) * nOutputChannels + 1]
				= inputSpan[frame];
		}
	} else {
		memcpy(pOutData, pSrcBuffer, Bytes);
	}
	hr = m_pAudioRenderClient->ReleaseBuffer(nFrames, 0);
	if (FAILED(hr)) return false;

	if (m_state == StreamState::Stopped && BufferFramesWritable() == 0) {
		if (!Play()) {
			return false;
		}
		// m_state = StreamState::Started (in Play())
	}

	return true;
}
