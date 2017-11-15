/*
** FamiTracker - NES/Famicom sound tracker
** Copyright (C) 2005-2014  Jonathan Liss
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

#include "VisualizerWnd.h"
#include "FamiTracker.h"
#include "Settings.h"
#include "VisualizerScope.h"
#include "VisualizerSpectrum.h"
#include "VisualizerStatic.h"

// Thread entry helper

UINT CVisualizerWnd::ThreadProcFunc(LPVOID pParam)
{
	CVisualizerWnd *pObj = reinterpret_cast<CVisualizerWnd*>(pParam);

	if (pObj == NULL || !pObj->IsKindOf(RUNTIME_CLASS(CVisualizerWnd)))
		return 1;

	return pObj->ThreadProc();
}

// CSampleWindow

IMPLEMENT_DYNAMIC(CVisualizerWnd, CWnd)

CVisualizerWnd::CVisualizerWnd() :
	m_iCurrentState(0),
	m_bThreadRunning(false),
	m_pWorkerThread(NULL),
	m_iBufferSize(0),
	m_pBuffer1(NULL),
	m_pBuffer2(NULL),
	m_pFillBuffer(NULL),
	m_hNewSamples(NULL),
	m_bNoAudio(false)
{
	m_pStates[0] = new CVisualizerScope(false);
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

	SAFE_RELEASE_ARRAY(m_pBuffer1);
	SAFE_RELEASE_ARRAY(m_pBuffer2);
}

BEGIN_MESSAGE_MAP(CVisualizerWnd, CWnd)
	ON_WM_ERASEBKGND()
	ON_WM_LBUTTONDOWN()
	ON_WM_PAINT()
	ON_WM_LBUTTONDBLCLK()
	ON_WM_RBUTTONUP()
	ON_WM_DESTROY()
END_MESSAGE_MAP()

// State methods

void CVisualizerWnd::NextState()
{
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

void CVisualizerWnd::FlushSamples(short *pSamples, int Count)
{
	if (!m_bThreadRunning)
		return;

	if (Count != m_iBufferSize) {
		m_csBuffer.Lock();
		SAFE_RELEASE_ARRAY(m_pBuffer1);
		SAFE_RELEASE_ARRAY(m_pBuffer2);
		m_pBuffer1 = new short[Count];
		m_pBuffer2 = new short[Count];
		m_iBufferSize = Count;
		m_pFillBuffer = m_pBuffer1;
		m_csBuffer.Unlock();
	}

	m_csBufferSelect.Lock();
	memcpy(m_pFillBuffer, pSamples, sizeof(short) * Count);
	m_csBufferSelect.Unlock();

	SetEvent(m_hNewSamples);
}

void CVisualizerWnd::ReportAudioProblem()
{
	m_bNoAudio = true;
	Invalidate();
}

UINT CVisualizerWnd::ThreadProc()
{
	DWORD nThreadID = AfxGetThread()->m_nThreadID;
	m_bThreadRunning = true;

	TRACE("Visualizer: Started thread (0x%04x)\n", nThreadID);

	while (::WaitForSingleObject(m_hNewSamples, INFINITE) == WAIT_OBJECT_0 && m_bThreadRunning) {

		m_bNoAudio = false;

		// Switch buffers
		m_csBufferSelect.Lock();

		short *pDrawBuffer = m_pFillBuffer;

		if (m_pFillBuffer == m_pBuffer1)
			m_pFillBuffer = m_pBuffer2;
		else
			m_pFillBuffer = m_pBuffer1;

		m_csBufferSelect.Unlock();

		// Draw
		m_csBuffer.Lock();

		CDC *pDC = GetDC();
		if (pDC != NULL) {
			m_pStates[m_iCurrentState]->SetSampleData(pDrawBuffer, m_iBufferSize);
			m_pStates[m_iCurrentState]->Draw();
			m_pStates[m_iCurrentState]->Display(pDC, false);
			ReleaseDC(pDC);
		}

		m_csBuffer.Unlock();
	}

	TRACE("Visualizer: Closed thread (0x%04x)\n", nThreadID);

	return 0;
}

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

		// Create a worker thread
		m_pWorkerThread = AfxBeginThread(&ThreadProcFunc, (LPVOID)this, THREAD_PRIORITY_BELOW_NORMAL);
	}

	return Result;
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
			m_iCurrentState = i;
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
		
		m_bThreadRunning = false;
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
