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

#include <mmsystem.h>
#include <InitGuid.h>
#include <dsound.h>

// Return values from WaitForDirectSoundEvent()
enum buffer_event_t {
	BUFFER_NONE = 0,
	BUFFER_CUSTOM_EVENT = 1,
	BUFFER_TIMEOUT,
	BUFFER_IN_SYNC,
	BUFFER_OUT_OF_SYNC
};

// DirectSound channel
class CSoundStream
{
	friend class CSoundInterface;

public:
	CSoundStream();
	~CSoundStream();

	bool Play() const;
	bool Stop() const;
	bool IsPlaying() const;
	bool ClearBuffer();
	bool WriteBuffer(char const * pBuffer, unsigned int Samples);

	buffer_event_t WaitForSyncEvent(DWORD dwTimeout) const;

	int GetBlockSize() const	{ return m_iBlockSize; };
	int GetBlockSamples() const	{ return m_iBlockSize >> ((m_iSampleSize >> 3) - 1); };
	int GetBlocks()	const		{ return m_iBlocks; };
	int	GetBufferLength() const	{ return m_iBufferLength; };
	int GetSampleSize()	const	{ return m_iSampleSize;	};
	int	GetSampleRate()	const	{ return m_iSampleRate;	};
	int GetChannels() const		{ return m_iChannels; };

private:
	int GetPlayBlock() const;

	/*!
	https://docs.microsoft.com/en-us/previous-versions/windows/desktop/ee418062(v%3Dvs.85)
	The write cursor is the point in the buffer ahead of which it is safe to write data to the buffer.
	Data should not be written to the part of the buffer after the play cursor and before the write cursor.

	Data should not be written to the blocks in (?, write cursor's block].
	Otherwise, you risk writing before the write cursor, which will not be played back properly.
	*/
	int GetWritableBlock() const;

	void AdvanceWritePointer();

private:
	LPDIRECTSOUNDBUFFER	m_lpDirectSoundBuffer;
	LPDIRECTSOUNDNOTIFY	m_lpDirectSoundNotify;

	HANDLE			m_hEventList[2];
	HWND			m_hWndTarget;

	// Configuration
	unsigned int	m_iSampleSize;
	unsigned int	m_iSampleRate;
	unsigned int	m_iChannels;
	unsigned int	m_iBufferLength;
	unsigned int	m_iSoundBufferSize;			// in bytes
	unsigned int	m_iBlocks;
	unsigned int	m_iBlockSize;				// in bytes

	// State
	unsigned int	m_iCurrentWriteBlock;
};

// DirectSound
class CSoundInterface
{
public:
	CSoundInterface(HWND hWnd, HANDLE hNotification);
	~CSoundInterface();

	bool			SetupDevice(int iDevice);
	void			CloseDevice();

	CSoundStream	*OpenChannel(int SampleRate, int SampleSize, int Channels, int BufferLength, int Blocks);
	void			CloseChannel(CSoundStream *pChannel);

	int				CalculateBufferLength(int BufferLen, int Samplerate, int Samplesize, int Channels) const;

	// Enumeration
	void			EnumerateDevices();
	void			ClearEnumeration();
	BOOL			EnumerateCallback(LPGUID lpGuid, LPCTSTR lpcstrDescription, LPCTSTR lpcstrModule, LPVOID lpContext);
	unsigned int	GetDeviceCount() const;
	LPCTSTR			GetDeviceName(unsigned int iDevice) const;
	int				MatchDeviceID(LPCTSTR Name) const;

public:
	static const unsigned int MAX_DEVICES = 256;
	static const unsigned int MAX_BLOCKS = 16;
	static const unsigned int MAX_SAMPLE_RATE = 96000;
	static const unsigned int MAX_BUFFER_LENGTH = 10000;

protected:
	static BOOL CALLBACK DSEnumCallback(LPGUID lpGuid, LPCTSTR lpcstrDescription, LPCTSTR lpcstrModule, LPVOID lpContext);
	static CSoundInterface *pThisObject;

private:
	HWND			m_hWndTarget;
	HANDLE			m_hNotificationHandle;
	LPDIRECTSOUND	m_lpDirectSound;

	// For enumeration
	unsigned int	m_iDevices;
	LPCTSTR			m_pcDevice[MAX_DEVICES];
	GUID			*m_pGUIDs[MAX_DEVICES];
};

#endif /* SOUNDINTERFACE_H */
