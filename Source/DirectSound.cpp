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

//
// DirectSound Interface
//

#include "stdafx.h"
#include <cstdio>
#include "common.h"
#include "DirectSound.h"
#include "resource.h"

// The single CDSound object
CDSound *CDSound::pThisObject = NULL;

// Class members

BOOL CALLBACK CDSound::DSEnumCallback(LPGUID lpGuid, LPCTSTR lpcstrDescription, LPCTSTR lpcstrModule, LPVOID lpContext)
{
	return pThisObject->EnumerateCallback(lpGuid, lpcstrDescription, lpcstrModule, lpContext);
}

// Instance members

CDSound::CDSound(HWND hWnd, HANDLE hNotification) :
	m_iDevices(0),
	m_lpDirectSound(NULL),
	m_hWndTarget(hWnd),
	m_hNotificationHandle(hNotification)
{
	ASSERT(pThisObject == NULL);
	pThisObject = this;
}

CDSound::~CDSound()
{
	for (int i = 0; i < (int)m_iDevices; ++i) {
		delete [] m_pcDevice[i];
		delete [] m_pGUIDs[i];
	}
}

bool CDSound::SetupDevice(int iDevice)
{	
	if (iDevice > (int)m_iDevices)
		iDevice = 0;
	
	if (m_lpDirectSound) {
		m_lpDirectSound->Release();
		m_lpDirectSound = NULL;
	}

	if (FAILED(DirectSoundCreate((LPCGUID)m_pGUIDs[iDevice], &m_lpDirectSound, NULL))) {
		m_lpDirectSound = NULL;
		return false;
	}

	if (FAILED(m_lpDirectSound->SetCooperativeLevel(m_hWndTarget, DSSCL_PRIORITY))) {
		m_lpDirectSound = NULL;
		return false;
	}

	return true;
}

void CDSound::CloseDevice()
{
	if (!m_lpDirectSound)
		return;

	m_lpDirectSound->Release();
	m_lpDirectSound = NULL;

	if (m_iDevices != 0)
		ClearEnumeration();
}

void CDSound::ClearEnumeration()
{
	for (unsigned i = 0; i < m_iDevices; ++i) {
		delete [] m_pcDevice[i];
		if (m_pGUIDs[i] != NULL)
			delete m_pGUIDs[i];
	}

	m_iDevices = 0;
}

BOOL CDSound::EnumerateCallback(LPGUID lpGuid, LPCTSTR lpcstrDescription, LPCTSTR lpcstrModule, LPVOID lpContext)
{
	m_pcDevice[m_iDevices] = new TCHAR[_tcslen(lpcstrDescription) + 1];
	_tcscpy((TCHAR*)m_pcDevice[m_iDevices], lpcstrDescription);

	if (lpGuid != NULL) {
		m_pGUIDs[m_iDevices] = new GUID;
		memcpy(m_pGUIDs[m_iDevices], lpGuid, sizeof(GUID));
	}
	else
		m_pGUIDs[m_iDevices] = NULL;

	++m_iDevices;

	return TRUE;
}

void CDSound::EnumerateDevices()
{
	if (m_iDevices != 0)
		ClearEnumeration();

	DirectSoundEnumerate(DSEnumCallback, NULL);
	
#ifdef _DEBUG
	// Add an invalid device for debugging reasons
	GUID g;
	g.Data1 = 1;
	g.Data2 = 2;
	g.Data3 = 3;
	for (int i = 0; i < 8; ++i)
		g.Data4[i] = i;
	EnumerateCallback(&g, _T("Invalid device"), NULL, NULL);
#endif
}

unsigned int CDSound::GetDeviceCount() const
{
	return m_iDevices;
}

LPCTSTR CDSound::GetDeviceName(unsigned int iDevice) const
{
	ASSERT(iDevice < m_iDevices);
	return m_pcDevice[iDevice];
}

int CDSound::MatchDeviceID(LPCTSTR Name) const
{
	for (unsigned int i = 0; i < m_iDevices; ++i) {
		if (!_tcscmp(Name, m_pcDevice[i]))
			return i;
	}

	return 0;
}

int CDSound::CalculateBufferLength(int BufferLen, int Samplerate, int Samplesize, int Channels) const
{
	// Calculate size of the buffer, in bytes
	return ((Samplerate * BufferLen) / 1000) * (Samplesize / 8) * Channels;
}

CDSoundChannel *CDSound::OpenChannel(int SampleRate, int SampleSize, int Channels, int BufferLength, int Blocks)
{
	// Open a new secondary buffer
	//

	DSBPOSITIONNOTIFY	dspn[MAX_BLOCKS];
	WAVEFORMATEX		wfx;
	DSBUFFERDESC		dsbd;

	ASSERT(Blocks > 1);

	if (!m_lpDirectSound)
		return NULL;

	// Adjust buffer length in case a buffer would end up in half samples
	while ((SampleRate * BufferLength / (Blocks * 1000) != (double)SampleRate * BufferLength / (Blocks * 1000)))
		++BufferLength;
 
	CDSoundChannel *pChannel = new CDSoundChannel();

	HANDLE hBufferEvent = CreateEvent(NULL, FALSE, FALSE, NULL);

	int SoundBufferSize = CalculateBufferLength(BufferLength, SampleRate, SampleSize, Channels);
	int BlockSize = SoundBufferSize / Blocks;
	
	pChannel->m_iBufferLength		= BufferLength;			// in ms
	pChannel->m_iSoundBufferSize	= SoundBufferSize;		// in bytes
	pChannel->m_iBlockSize			= BlockSize;			// in bytes
	pChannel->m_iBlocks				= Blocks;
	pChannel->m_iSampleSize			= SampleSize;
	pChannel->m_iSampleRate			= SampleRate;
	pChannel->m_iChannels			= Channels;

	pChannel->m_iCurrentWriteBlock	= 0;
	pChannel->m_hWndTarget			= m_hWndTarget;
	pChannel->m_hEventList[0]		= m_hNotificationHandle;
	pChannel->m_hEventList[1]		= hBufferEvent;

	memset(&wfx, 0x00, sizeof(WAVEFORMATEX));
	wfx.cbSize				= sizeof(WAVEFORMATEX);
	wfx.nChannels			= Channels;
	wfx.nSamplesPerSec		= SampleRate;
	wfx.wBitsPerSample		= SampleSize;
	wfx.nBlockAlign			= wfx.nChannels * (wfx.wBitsPerSample / 8);
	wfx.nAvgBytesPerSec		= wfx.nSamplesPerSec * wfx.nBlockAlign;
	wfx.wFormatTag			= WAVE_FORMAT_PCM;

	memset(&dsbd, 0x00, sizeof(DSBUFFERDESC));
	dsbd.dwSize			= sizeof(DSBUFFERDESC);
	dsbd.dwBufferBytes	= SoundBufferSize;
	dsbd.dwFlags		= DSBCAPS_LOCSOFTWARE | DSBCAPS_GLOBALFOCUS | DSBCAPS_CTRLPOSITIONNOTIFY | DSBCAPS_GETCURRENTPOSITION2;
	dsbd.lpwfxFormat	= &wfx;

	if (FAILED(m_lpDirectSound->CreateSoundBuffer(&dsbd, &pChannel->m_lpDirectSoundBuffer, NULL))) {
		delete pChannel;
		return NULL;
	}

	// Setup notifications
	for (int i = 0; i < Blocks; ++i) {
		dspn[i].dwOffset = i * BlockSize;
		dspn[i].hEventNotify = hBufferEvent;
	}

	if (FAILED(pChannel->m_lpDirectSoundBuffer->QueryInterface(IID_IDirectSoundNotify, (void**)&pChannel->m_lpDirectSoundNotify))) {
		delete pChannel;
		return NULL;
	}

	if (FAILED(pChannel->m_lpDirectSoundNotify->SetNotificationPositions(Blocks, dspn))) {
		delete pChannel;
		return NULL;
	}

	pChannel->ClearBuffer();
	
	return pChannel;
}

void CDSound::CloseChannel(CDSoundChannel *pChannel)
{
	if (pChannel == NULL)
		return;

	pChannel->m_lpDirectSoundBuffer->Release();
	pChannel->m_lpDirectSoundNotify->Release();

	delete pChannel;
}

// CDSoundChannel

CDSoundChannel::CDSoundChannel()
{
	m_iCurrentWriteBlock = 0;

	m_hEventList[0] = NULL;
	m_hEventList[1] = NULL;
	m_hWndTarget = NULL;
}

CDSoundChannel::~CDSoundChannel()
{
	// Kill buffer event
	if (m_hEventList[1])
		CloseHandle(m_hEventList[1]);
}

bool CDSoundChannel::Play() const
{
	// Begin playback of buffer
	return FAILED(m_lpDirectSoundBuffer->Play(NULL, NULL, DSBPLAY_LOOPING)) ? false : true;
}

bool CDSoundChannel::Stop() const
{
	// Stop playback
	return FAILED(m_lpDirectSoundBuffer->Stop()) ? false : true;
}

bool CDSoundChannel::IsPlaying() const
{
	DWORD Status;
	m_lpDirectSoundBuffer->GetStatus(&Status);
	return ((Status & DSBSTATUS_PLAYING) == DSBSTATUS_PLAYING);
}

bool CDSoundChannel::ClearBuffer()
{
	LPVOID pAudioPtr1, pAudioPtr2;
	DWORD AudioBytes1, AudioBytes2;

	if (IsPlaying())
		if (!Stop())
			return false;

	if (FAILED(m_lpDirectSoundBuffer->Lock(0, m_iSoundBufferSize, (void**)&pAudioPtr1, &AudioBytes1, (void**)&pAudioPtr2, &AudioBytes2, 0)))
		return false;
	
	if (m_iSampleSize == 8) {
		memset(pAudioPtr1, 0x80, AudioBytes1);
		if (pAudioPtr2)
			memset(pAudioPtr2, 0x80, AudioBytes2);
	}
	else {
		memset(pAudioPtr1, 0x00, AudioBytes1);
		if (pAudioPtr2)
			memset(pAudioPtr2, 0x00, AudioBytes2);
	}

	if (FAILED(m_lpDirectSoundBuffer->Unlock((void*)pAudioPtr1, AudioBytes1, (void*)pAudioPtr2, AudioBytes2)))
		return false;

	m_lpDirectSoundBuffer->SetCurrentPosition(0);
	m_iCurrentWriteBlock = 0;

	return true;
}

bool CDSoundChannel::WriteBuffer(char *pBuffer, unsigned int Samples)
{
	// Fill sound buffer
	//
	// Buffer	- Pointer to a buffer with samples
	// Samples	- Number of samples, in bytes
	//

	LPVOID pAudioPtr1, pAudioPtr2;
	DWORD AudioBytes1, AudioBytes2;
	int	  Block = m_iCurrentWriteBlock;

	ASSERT(Samples == m_iBlockSize);
	
	if (FAILED(m_lpDirectSoundBuffer->Lock(Block * m_iBlockSize, m_iBlockSize, (void**)&pAudioPtr1, &AudioBytes1, (void**)&pAudioPtr2, &AudioBytes2, 0)))
		return false;

	memcpy(pAudioPtr1, pBuffer, AudioBytes1);

	if (pAudioPtr2)
		memcpy(pAudioPtr2, pBuffer + AudioBytes1, AudioBytes2);

	if (FAILED(m_lpDirectSoundBuffer->Unlock((void*)pAudioPtr1, AudioBytes1, (void*)pAudioPtr2, AudioBytes2)))
		return false;

	AdvanceWritePointer();

	return true;
}

buffer_event_t CDSoundChannel::WaitForSyncEvent(DWORD dwTimeout) const
{
	// Wait for a DirectSound event
	if (!IsPlaying()) {
		if (!Play())
			return BUFFER_NONE;
	}

	// Wait for events
	switch (::WaitForMultipleObjects(2, m_hEventList, FALSE, dwTimeout)) {
		case WAIT_OBJECT_0:			// External event
			return BUFFER_CUSTOM_EVENT;
		case WAIT_OBJECT_0 + 1:		// DirectSound buffer
			return (GetWriteBlock() == m_iCurrentWriteBlock) ? BUFFER_OUT_OF_SYNC : BUFFER_IN_SYNC;
		case WAIT_TIMEOUT:			// Timeout
			return BUFFER_TIMEOUT;
	}

	// Error
	return BUFFER_NONE;
}

int CDSoundChannel::GetPlayBlock() const
{
	// Return the block where the play pos is
	DWORD PlayPos, WritePos;
	m_lpDirectSoundBuffer->GetCurrentPosition(&PlayPos, &WritePos);
	return (PlayPos / m_iBlockSize);
}

int CDSoundChannel::GetWriteBlock() const
{
	// Return the block where the write pos is
	DWORD PlayPos, WritePos;
	m_lpDirectSoundBuffer->GetCurrentPosition(&PlayPos, &WritePos);
	return (WritePos / m_iBlockSize);
}

void CDSoundChannel::AdvanceWritePointer()
{
	m_iCurrentWriteBlock = (m_iCurrentWriteBlock + 1) % m_iBlocks;
}
