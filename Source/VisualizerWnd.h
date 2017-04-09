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


// Visualizer classes

#include <afxmt.h>		// Synchronization objects

class CVisualizerBase
{
public:
	CVisualizerBase() : m_iWidth(0), m_iHeight(0), m_iSampleCount(0), m_pSamples(NULL) {}
	virtual ~CVisualizerBase() {}

	// Create the visualizer
	virtual void Create(int Width, int Height);
	// Set rate of samples
	virtual void SetSampleRate(int SampleRate) = 0;
	// Set new sample data
	virtual void SetSampleData(short *iSamples, unsigned int iCount);
	// Render an image from the sample data
	virtual void Draw() = 0;
	// Display the image
	virtual void Display(CDC *pDC, bool bPaintMsg) = 0;

protected:
	BITMAPINFO m_bmi;
	int m_iWidth;
	int m_iHeight;

	unsigned int m_iSampleCount;
	short *m_pSamples;
};

// CVisualizerWnd

class CVisualizerWnd : public CWnd
{
	DECLARE_DYNAMIC(CVisualizerWnd)

public:
	CVisualizerWnd();
	virtual ~CVisualizerWnd();

	void SetSampleRate(int SampleRate);
	void FlushSamples(short *Samples, int Count);
	void ReportAudioProblem();

private:
	static UINT ThreadProcFunc(LPVOID pParam);

private:
	void NextState();
	UINT ThreadProc();

private:
	static const int STATE_COUNT = 5;		// // //

private:
	CVisualizerBase *m_pStates[STATE_COUNT];
	unsigned int m_iCurrentState;

	int	m_iBufferSize;
	short *m_pBuffer1;
	short *m_pBuffer2;
	short *m_pFillBuffer;

	HANDLE m_hNewSamples;

	bool m_bNoAudio;

	// Thread
	CWinThread *m_pWorkerThread;
	bool m_bThreadRunning;

	CCriticalSection m_csBufferSelect;
	CCriticalSection m_csBuffer;

public:
	virtual BOOL CreateEx(DWORD dwExStyle, LPCTSTR lpszClassName, LPCTSTR lpszWindowName, DWORD dwStyle, const RECT& rect, CWnd* pParentWnd, UINT nID, CCreateContext* pContext = NULL);

protected:
	DECLARE_MESSAGE_MAP()
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnPaint();
	afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
	afx_msg void OnRButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnDestroy();
};
