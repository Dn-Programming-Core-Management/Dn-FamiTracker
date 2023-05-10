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

#include "VisualizerWnd.h"
#include "FamiTracker.h"
#include "Settings.h"
#include "VisualizerScope.h"
#include "VisualizerSpectrum.h"
#include "VisualizerStatic.h"
#include <algorithm>  // std::copy, std::fill
#include <tuple>

void TripleBuffer::Initialize(size_t Size)
{
	shared.store(INIT_SHARED, std::memory_order_relaxed);

	for (size_t i = 0; i < 3; i++) {
		this->pBuffers[i] = std::make_unique<short[]>(Size);
		auto pBuffer = this->pBuffers[i].get();
		std::fill(pBuffer, pBuffer + Size, 0);
	}
}

Reader::Reader(TripleBuffer* pBuffer) :
	m_pBuffer(pBuffer),
	m_ReadIndex(TripleBuffer::INIT_READ)
{
}

Reader::~Reader() = default;

short const* Reader::Curr() const
{
	return m_pBuffer->pBuffers[m_ReadIndex].get();
}

bool Reader::Fetch()
{
	if (!(m_pBuffer->shared.load(std::memory_order_relaxed) & TripleBuffer::SHARED_WRITTEN)) {
		return false;
	}

	// Release currently owned buffer and acquire new one to read from.
	auto readTmp = m_pBuffer->shared.exchange(m_ReadIndex, std::memory_order_acq_rel);
	ASSERT(readTmp & TripleBuffer::SHARED_WRITTEN);
	m_ReadIndex = readTmp & TripleBuffer::SHARED_INDEX;
	ASSERT(m_ReadIndex < 3);
	return true;
}

Writer::Writer(TripleBuffer* pBuffer) :
	m_pBuffer(pBuffer),
	m_WriteIndex(TripleBuffer::INIT_WRITE)
{
}

Writer::~Writer() = default;

short* Writer::Curr()
{
	return m_pBuffer->pBuffers[m_WriteIndex].get();
}

void Writer::Publish()
{
	// Release currently owned buffer and acquire new one to write to.
	uint8_t writeTmp = m_pBuffer->shared.exchange(
		m_WriteIndex | TripleBuffer::SHARED_WRITTEN, std::memory_order_acq_rel
	);
	m_WriteIndex = writeTmp & TripleBuffer::SHARED_INDEX;
	ASSERT(m_WriteIndex < 3);
}

// CSampleWindow

IMPLEMENT_DYNAMIC(CVisualizerWnd, CWnd)

CVisualizerWnd::CVisualizerWnd() :
	m_pStates(),
	m_iCurrentState(0),
	m_pScope(nullptr),
	m_ScopeBufferSize(0),
	m_pScopeData(std::make_unique<TripleBuffer>()),
	m_pSpectrumData(std::make_unique<TripleBuffer>()),
	m_ScopeWriter(m_pScopeData.get()),
	m_ScopeWriteProgress(0),
	m_SpectrumWriter(m_pSpectrumData.get()),
	m_pSpectrumHistory(std::make_unique<short[]>(FFT_POINTS)),
	m_hNewSamples(NULL),
	m_bNoAudio(false),
	m_pWorkerThread(NULL),
	m_bThreadRunning(false),
	m_csBuffer()
{
	m_pStates[0] = m_pScope = new CVisualizerScope(false);
	m_pStates[1] = new CVisualizerScope(true);
	m_pStates[2] = new CVisualizerSpectrum(4);		// // //
	m_pStates[3] = new CVisualizerSpectrum(1);
	m_pStates[4] = new CVisualizerStatic();
}

CVisualizerWnd::~CVisualizerWnd()
{
	for (int i = 0; i < STATE_COUNT; ++i) {		// // //
		SAFE_RELEASE(m_pStates[i]);
	}
}

HANDLE CVisualizerWnd::GetThreadHandle() const {		// // //
	return m_pWorkerThread->m_hThread;
}

BEGIN_MESSAGE_MAP(CVisualizerWnd, CWnd)
	ON_WM_ERASEBKGND()
	ON_WM_LBUTTONDOWN()
	ON_WM_PAINT()
	ON_WM_LBUTTONDBLCLK()
	ON_WM_RBUTTONUP()
	ON_WM_DESTROY()
END_MESSAGE_MAP()

BOOL CVisualizerWnd::CreateEx(DWORD dwExStyle, LPCTSTR lpszClassName, LPCTSTR lpszWindowName, DWORD dwStyle, const RECT& rect, CWnd* pParentWnd, UINT nID, CCreateContext* pContext)
{
	// This is saved
	m_iCurrentState = theApp.GetSettings()->SampleWinState;

	// Create an event used to signal that new samples are available
	m_hNewSamples = CreateEvent(NULL, FALSE, FALSE, NULL);

	BOOL Result = CWnd::CreateEx(dwExStyle, lpszClassName, lpszWindowName, dwStyle, rect, pParentWnd, nID, pContext);

	if (Result) {
		// Get client rect and create visualizers
		CRect crect;
		GetClientRect(&crect);
		for (int i = 0; i < STATE_COUNT; ++i) {
			m_pStates[i]->Create(crect.Width(), crect.Height());
		}

		m_ScopeBufferSize = m_pScope->NeededSamples();

		// Initialize triple buffer.
		m_pScopeData->Initialize(m_ScopeBufferSize);
		m_pSpectrumData->Initialize(FFT_POINTS);

		// Create a worker thread
		m_pWorkerThread = AfxBeginThread(&ThreadProcFunc, (LPVOID)this, THREAD_PRIORITY_BELOW_NORMAL);
	}

	return Result;
}

// Thread entry helper

UINT CVisualizerWnd::ThreadProcFunc(LPVOID pParam)
{
	CVisualizerWnd* pObj = reinterpret_cast<CVisualizerWnd*>(pParam);

	if (pObj == NULL || !pObj->IsKindOf(RUNTIME_CLASS(CVisualizerWnd)))
		return 1;

	return pObj->ThreadProc();
}

UINT CVisualizerWnd::ThreadProc()
{
	DWORD nThreadID = AfxGetThread()->m_nThreadID;
	m_bThreadRunning.store(true, std::memory_order_release);

	TRACE("Visualizer: Started thread (0x%04x)\n", nThreadID);
	auto scopeReader = Reader(m_pScopeData.get());
	auto spectrumReader = Reader(m_pSpectrumData.get());

	while (
		::WaitForSingleObject(m_hNewSamples, INFINITE) == WAIT_OBJECT_0
		&& m_bThreadRunning.load(std::memory_order_relaxed)
	) {

		m_bNoAudio = false;

		bool scopeChanged = scopeReader.Fetch();
		bool spectrumChanged = spectrumReader.Fetch();

		// CVisualizerWnd::FlushSamples always publishes buffers before signalling
		// m_hNewSamples, and we just waited for m_hNewSamples to be signalled.
		// You'd think the buffers are guaranteed to be changed.
		//
		// But it's possible CVisualizerWnd::FlushSamples is called again and publishes
		// again, before we see m_hNewSamples and fetch the new buffer contents.
		// Then on the next iteration WaitForSingleObject passes but the buffer
		// isn't changed.
		//
		// If this happens, retry the loop instead of drawing unchanged data.
		//
		// (This can also happen on program shutdown, when CVisualizerWnd::OnDestroy()
		// signals m_hNewSamples.)
		if (!(scopeChanged || spectrumChanged)) {
			continue;
		}

		m_csBuffer.Lock();

		// Draw
		CDC* pDC = GetDC();
		if (pDC != NULL) {
			auto* state = m_pStates[m_iCurrentState];

			bool updated = false;
			if (scopeChanged) {
				updated |= state->SetScopeData(
					scopeReader.Curr(), (unsigned int) m_ScopeBufferSize
				);
			}
			if (spectrumChanged) {
				updated |= state->SetSpectrumData(
					spectrumReader.Curr(), FFT_POINTS
				);
			}

			// Always update static visualizer
			updated |= (m_iCurrentState == 4);

			if (updated) {
				state->Draw();
				state->Display(pDC, false);
			}
			ReleaseDC(pDC);
		}

		m_csBuffer.Unlock();
	}

	TRACE("Visualizer: Closed thread (0x%04x)\n", nThreadID);

	return 0;
}

// State methods

void CVisualizerWnd::NextState()
{
	// Acquiring a lock here is probably unnecessary?
	m_csBuffer.Lock();
	m_iCurrentState = (m_iCurrentState + 1) % STATE_COUNT;
	m_csBuffer.Unlock();

	Invalidate();

	theApp.GetSettings()->SampleWinState = m_iCurrentState;
}

// CSampleWindow message handlers

void CVisualizerWnd::SetSampleRate(int SampleRate)
{
	for (auto &state : m_pStates)		// // //
		if (state)
			state->SetSampleRate(SampleRate);
}

void CVisualizerWnd::FlushSamples(gsl::span<const short> Samples)
{
	if (!m_bThreadRunning.load(std::memory_order_acquire))
		return;

	// On GUI thread, buffers are initialized before visualizer thread is started.
	// After visualizer thread is started, m_bThreadRunning.store(true, Release).
	// On audio thread, after m_bThreadRunning.load(Acquire) == true, buffers are
	// initialized.

	// Fill m_pSpectrumHistory. (We can't write incrementally to m_SpectrumWriter.Curr()
	// because we keep some old data from one publish-swap to the next.)
	{
		auto tail = Samples;
		if (tail.size() > (size_t)FFT_POINTS) {
			tail = tail.subspan(tail.size() - (size_t)FFT_POINTS);
		}
		ASSERT(tail.size() <= (size_t)FFT_POINTS);

		auto history = m_pSpectrumHistory.get();
		// Push tail to end of history.
		std::copy(
			history + tail.size(),
			history + FFT_POINTS,
			history);
		std::copy(
			tail.begin(),
			tail.end(),
			history + FFT_POINTS - tail.size());

		short* pSpectrumBuffer = m_SpectrumWriter.Curr();
		std::copy(history, history + FFT_POINTS, pSpectrumBuffer);
		m_SpectrumWriter.Publish();
	}

	// Fill pScopeBuffer. (We can write incrementally to m_ScopeWriter.Curr() because
	// we discard all old contents between publishing.)
	short* pScopeBuffer = m_ScopeWriter.Curr();
	while (!Samples.empty()) {
		size_t push = std::min(Samples.size(), m_ScopeBufferSize - m_ScopeWriteProgress);
		for (size_t i = 0; i < push; i++) {
			pScopeBuffer[m_ScopeWriteProgress] = Samples[i];
			m_ScopeWriteProgress++;
		}

		ASSERT(m_ScopeWriteProgress <= m_ScopeBufferSize);
		if (m_ScopeWriteProgress == m_ScopeBufferSize) {
			// Release currently owned buffer and acquire new one to write to.
			m_ScopeWriter.Publish();
			pScopeBuffer = m_ScopeWriter.Curr();
			m_ScopeWriteProgress = 0;
		}

		// Drop initial samples already processed.
		Samples = Samples.subspan(push);
	}

	SetEvent(m_hNewSamples);
}

void CVisualizerWnd::ReportAudioProblem()
{
	m_bNoAudio = true;
	Invalidate();
}

void CVisualizerWnd::OnLButtonDown(UINT nFlags, CPoint point)
{
	NextState();
	CWnd::OnLButtonDown(nFlags, point);
}

void CVisualizerWnd::OnLButtonDblClk(UINT nFlags, CPoint point)
{
	NextState();
	CWnd::OnLButtonDblClk(nFlags, point);
}

void CVisualizerWnd::OnPaint()
{
	m_csBuffer.Lock();

	CPaintDC dc(this); // device context for painting

	if (m_bNoAudio) {
		CRect rect;
		GetClientRect(rect);
		dc.DrawText("No audio", rect, DT_CENTER | DT_VCENTER);
	}
	else
		m_pStates[m_iCurrentState]->Display(&dc, true);

	m_csBuffer.Unlock();
}

void CVisualizerWnd::OnRButtonUp(UINT nFlags, CPoint point)
{
	CMenu PopupMenuBar;
	PopupMenuBar.LoadMenu(IDR_SAMPLE_WND_POPUP);

	CMenu *pPopupMenu = PopupMenuBar.GetSubMenu(0);

	CPoint menuPoint;
	CRect rect;

	GetWindowRect(rect);

	menuPoint.x = rect.left + point.x;
	menuPoint.y = rect.top + point.y;

	static const UINT menuIds[] = {
		ID_POPUP_SAMPLESCOPE1,
		ID_POPUP_SAMPLESCOPE2,
		ID_POPUP_SPECTRUMANALYZER,
		ID_POPUP_SPECTRUMANALYZER2,		// // //
		ID_POPUP_NOTHING
	};

	pPopupMenu->CheckMenuItem(menuIds[m_iCurrentState], MF_BYCOMMAND | MF_CHECKED);

	UINT Result = pPopupMenu->TrackPopupMenu(TPM_RETURNCMD, menuPoint.x, menuPoint.y, this);

	m_csBuffer.Lock();

	for (size_t i = 0; i < sizeof(menuIds) / sizeof(UINT); i++)		// // //
		if (Result == menuIds[i]) {
			m_iCurrentState = static_cast<int>(i);
			break;
		}

	m_csBuffer.Unlock();

	Invalidate();
	theApp.GetSettings()->SampleWinState = m_iCurrentState;

	CWnd::OnRButtonUp(nFlags, point);
}

void CVisualizerWnd::OnDestroy()
{
	// Shut down worker thread
	if (m_pWorkerThread != NULL) {
		HANDLE hThread = m_pWorkerThread->m_hThread;

		m_bThreadRunning.store(false, std::memory_order_relaxed);
		::SetEvent(m_hNewSamples);

		TRACE("Visualizer: Joining thread...\n");
		if (::WaitForSingleObject(hThread, 5000) == WAIT_OBJECT_0) {
			::CloseHandle(m_hNewSamples);
			m_hNewSamples = NULL;
			m_pWorkerThread = NULL;
			TRACE("Visualizer: Thread has finished.\n");
		}
		else {
			TRACE("Visualizer: Could not shutdown worker thread\n");
		}
	}

	CWnd::OnDestroy();
}
