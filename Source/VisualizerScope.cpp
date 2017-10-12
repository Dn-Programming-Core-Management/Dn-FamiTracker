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

#include "stdafx.h"
#include <cmath>
#include "FamiTracker.h"
#include "VisualizerWnd.h"
#include "VisualizerScope.h"
#include "Graphics.h"

/*
 * Displays a sample scope
 *
 */

CVisualizerScope::CVisualizerScope(bool bBlur) :
#ifdef _DEBUG		// // //
	m_iPeak(0),
#endif
	m_pBlitBuffer(NULL),
	m_pWindowBuf(NULL),
	m_bBlur(bBlur),
	m_iWindowBufPtr(0)
{
}

CVisualizerScope::~CVisualizerScope()
{
	SAFE_RELEASE_ARRAY(m_pBlitBuffer);
	SAFE_RELEASE_ARRAY(m_pWindowBuf);
}

void CVisualizerScope::Create(int Width, int Height)
{
	CVisualizerBase::Create(Width, Height);

	SAFE_RELEASE_ARRAY(m_pBlitBuffer);
	SAFE_RELEASE_ARRAY(m_pWindowBuf);

	m_pBlitBuffer = new COLORREF[Width * (Height + 1)];
	memset(m_pBlitBuffer, 0, Width * Height * sizeof(COLORREF));

	m_pWindowBuf = new short[Width];
	m_iWindowBufPtr = 0;
}

void CVisualizerScope::SetSampleRate(int SampleRate)
{
}

void CVisualizerScope::ClearBackground()
{
	for (int y = 0; y < m_iHeight; ++y) {
		memset(m_pBlitBuffer + y * m_iWidth, int(sinf((float(y) * 3.14f) / float(m_iHeight)) * 40.0f), sizeof(COLORREF) * m_iWidth);
	}
}

void CVisualizerScope::RenderBuffer()
{
	const float SAMPLE_SCALING	= 1200.0f;

	const COLORREF LINE_COL1 = 0xFFFFFF;
	const COLORREF LINE_COL2 = 0x808080;

	const int BLUR_COLORS[] = {3, 12, 12};

	const float HALF_HEIGHT = float(m_iHeight) / 2.0f;

	if (m_bBlur)
		BlurBuffer(m_pBlitBuffer, m_iWidth, m_iHeight, BLUR_COLORS);
	else
		ClearBackground();

	float Sample = -float(m_pWindowBuf[0]) / SAMPLE_SCALING;

	for (float x = 0.0f; x < float(m_iWidth); ++x) {
		float LastSample = Sample;
		Sample = -float(m_pWindowBuf[int(x)]) / SAMPLE_SCALING;

		if (Sample < -HALF_HEIGHT + 1)
			Sample = -HALF_HEIGHT + 1;
		if (Sample > HALF_HEIGHT - 1)
			Sample = HALF_HEIGHT - 1;

		PutPixel(m_pBlitBuffer, m_iWidth, m_iHeight, x, Sample + HALF_HEIGHT - 0.5f, LINE_COL2);
		PutPixel(m_pBlitBuffer, m_iWidth, m_iHeight, x, Sample + HALF_HEIGHT + 0.5f, LINE_COL2);
		PutPixel(m_pBlitBuffer, m_iWidth, m_iHeight, x, Sample + HALF_HEIGHT + 0.0f, LINE_COL1);

		if ((Sample - LastSample) > 1.0f) {
			float frac = LastSample - floor(LastSample);
			for (float y = LastSample; y < Sample; ++y) {
				float Offset = (y - LastSample) / (Sample - LastSample);
				PutPixel(m_pBlitBuffer, m_iWidth, m_iHeight, x + Offset - 1.0f, y + HALF_HEIGHT + frac, LINE_COL1);
			}
		}
		else if ((LastSample - Sample) > 1.0f) {
			float frac = Sample - floor(Sample);
			for (float y = Sample; y < LastSample; ++y) {
				float Offset = (y - Sample) / (LastSample - Sample);
				PutPixel(m_pBlitBuffer, m_iWidth, m_iHeight, x - Offset, y + HALF_HEIGHT + frac, LINE_COL1);
			}
		}
	}
}

void CVisualizerScope::Draw()
{
#ifdef _DEBUG
	int _min = 0;		// // //
	int _max = 0;
#endif

	const int TIME_SCALING = 7;

	static int LastPos = 0;
	static int Accum = 0;

	for (unsigned int i = 0; i < m_iSampleCount; ++i) {
#ifdef _DEBUG
		if (_min > m_pSamples[i])
			_min = m_pSamples[i];
		if (_max < m_pSamples[i])
			_max = m_pSamples[i];
#endif

		int Pos = m_iWindowBufPtr++ / TIME_SCALING;

		Accum += m_pSamples[i];

		if (Pos != LastPos) {
			m_pWindowBuf[LastPos] = Accum / TIME_SCALING;
			Accum = 0;
		}

		LastPos = Pos;

		if (Pos == m_iWidth) {
			m_iWindowBufPtr = 0;
			LastPos = 0;
			RenderBuffer();
		}
	}

#ifdef _DEBUG
	m_iPeak = _max - _min;		// // //
#endif
}

void CVisualizerScope::Display(CDC *pDC, bool bPaintMsg)
{
	StretchDIBits(pDC->m_hDC, 0, 0, m_iWidth, m_iHeight, 0, 0, m_iWidth, m_iHeight, m_pBlitBuffer, &m_bmi, DIB_RGB_COLORS, SRCCOPY);

#ifdef _DEBUG
	CString PeakText;
	PeakText.Format(_T("%i"), m_iPeak);
	pDC->TextOut(0, 0, PeakText);
	PeakText.Format(_T("%gdB"), 20.0 * log(double(m_iPeak) / 65535.0));		// // //
	pDC->TextOut(0, 16, PeakText);
#endif
}
