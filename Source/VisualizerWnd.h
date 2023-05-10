/*
** FamiTracker - NES/Famicom sound tracker
** Copyright (C) 2005-2020 Jonathan Liss
**
** 0CC-FamiTracker is (C) 2014-2018 HertzDevil
**
** Dn-FamiTracker is (C) 2020-2023 D.P.C.M.
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

// Visualizer classes

#include "stdafx.h"		// // //
#include <afxmt.h>		// Synchronization objects
#include <atomic>
#include <cstdint>
#include <memory>
#include <vector>
#include <gsl/span>

class CVisualizerBase;		// // //
class CVisualizerScope;

// CVisualizerWnd

/// https://github.com/nyanpasu64/spectro2/blob/master/flip-cell/src/lib.rs
class alignas(64) TripleBuffer {
	// Constants
	static constexpr uint8_t INIT_WRITE = 0;
	static constexpr uint8_t INIT_SHARED = 1;
	static constexpr uint8_t INIT_READ = 2;
	static constexpr uint8_t SHARED_INDEX = 0x7F;
	static constexpr uint8_t SHARED_WRITTEN = 0x80;

	// Fields
	// [3] box[...] short
	std::unique_ptr<short[]> pBuffers[3];

	// atomic u8
	alignas(64) std::atomic<uint8_t> shared;

	friend class Reader;
	friend class Writer;

public:
	void Initialize(size_t Size);
};

class Reader {
	TripleBuffer * m_pBuffer;
	uint8_t m_ReadIndex;

	friend class TripleBuffer;

public:
	Reader(TripleBuffer* pBuffer);
	~Reader();
	short const* Curr() const;
	bool Fetch();
};

class Writer {
	TripleBuffer* m_pBuffer;
	uint8_t m_WriteIndex;

	friend class TripleBuffer;

public:
	Writer(TripleBuffer* pBuffer);
	~Writer();
	short* Curr();
	void Publish();
};

class CVisualizerWnd : public CWnd
{
	DECLARE_DYNAMIC(CVisualizerWnd)

public:
	CVisualizerWnd();
	virtual ~CVisualizerWnd();

	HANDLE GetThreadHandle() const;		// // //

	// Spawns new thread calling ThreadProc().
	BOOL CreateEx(DWORD dwExStyle, LPCTSTR lpszClassName, LPCTSTR lpszWindowName, DWORD dwStyle, const RECT& rect, CWnd* pParentWnd, UINT nID, CCreateContext* pContext = NULL);
private:
	static UINT ThreadProcFunc(LPVOID pParam);

private:
	UINT ThreadProc();
	void NextState();

public:
	void SetSampleRate(int SampleRate);
	void FlushSamples(gsl::span<const short> Samples);
	void ReportAudioProblem();

private:
	static const int STATE_COUNT = 5;		// // //

private:
	CVisualizerBase *m_pStates[STATE_COUNT];
	int m_iCurrentState;
	CVisualizerScope* m_pScope;  // Non-owning, points within m_pStates.

	// Samples/frames.
	std::size_t m_ScopeBufferSize;

	// Triple-buffer between audio and visualizer threads:
	std::unique_ptr<TripleBuffer> m_pScopeData;
	std::unique_ptr<TripleBuffer> m_pSpectrumData;

	// box u8 (written by audio thread)
	Writer m_ScopeWriter;
	size_t m_ScopeWriteProgress;

	Writer m_SpectrumWriter;
	std::unique_ptr<short[]> m_pSpectrumHistory;

	HANDLE m_hNewSamples;

	bool m_bNoAudio;

	// Thread
	CWinThread *m_pWorkerThread;
	std::atomic<bool> m_bThreadRunning;

	// Held while (visualizer) rendering the image or (gui) painting on-screen or
	// switching visualizers when clicked.
	CCriticalSection m_csBuffer;

protected:
	DECLARE_MESSAGE_MAP()
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnPaint();
	afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
	afx_msg void OnRButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnDestroy();
};
