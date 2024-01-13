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

#include "VisualizerScope.h"
#include <algorithm>  // std::fill
#include <cmath>
#include <stdexcept>
#include "FamiTracker.h"
#include "Graphics.h"

/*
 * Displays a sample scope
 *
 */

CVisualizerScope::CVisualizerScope(bool bBlur) :
	m_pWindowBuf(NULL),
	m_bBlur(bBlur)
{
}

CVisualizerScope::~CVisualizerScope()
{
	SAFE_RELEASE_ARRAY(m_pWindowBuf);
}

void CVisualizerScope::Create(int Width, int Height)
{
	CVisualizerBase::Create(Width, Height);

	SAFE_RELEASE_ARRAY(m_pWindowBuf);
	m_pWindowBuf = new short[Width];
	std::fill(m_pWindowBuf, m_pWindowBuf + Width, 0);
}

void CVisualizerScope::SetSampleRate(int SampleRate)
{
}

static constexpr int TIME_SCALING = 7;

bool CVisualizerScope::SetScopeData(short const* pSamples, unsigned int iCount)
{
	m_pSamples = pSamples;
	m_iSampleCount = iCount;
	ASSERT(m_iSampleCount == (unsigned int)(m_iWidth * TIME_SCALING));
	return true;
}

void CVisualizerScope::ClearBackground()
{
	for (int y = 0; y < m_iHeight; ++y) {
		int intensity = static_cast<int>(sinf((float(y) * 3.14f) / float(m_iHeight)) * 40.0f);		// // //
		memset(&m_pBlitBuffer[y * m_iWidth], intensity, sizeof(COLORREF) * m_iWidth);
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
		BlurBuffer(m_pBlitBuffer.get(), m_iWidth, m_iHeight, BLUR_COLORS);
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

		PutPixel(m_pBlitBuffer.get(), m_iWidth, m_iHeight, x, Sample + HALF_HEIGHT - 0.5f, LINE_COL2);
		PutPixel(m_pBlitBuffer.get(), m_iWidth, m_iHeight, x, Sample + HALF_HEIGHT + 0.5f, LINE_COL2);
		PutPixel(m_pBlitBuffer.get(), m_iWidth, m_iHeight, x, Sample + HALF_HEIGHT + 0.0f, LINE_COL1);

		if ((Sample - LastSample) > 1.0f) {
			float frac = LastSample - floor(LastSample);
			for (float y = LastSample; y < Sample; ++y) {
				float Offset = (y - LastSample) / (Sample - LastSample);
				PutPixel(m_pBlitBuffer.get(), m_iWidth, m_iHeight, x + Offset - 1.0f, y + HALF_HEIGHT + frac, LINE_COL1);
			}
		}
		else if ((LastSample - Sample) > 1.0f) {
			float frac = Sample - floor(Sample);
			for (float y = Sample; y < LastSample; ++y) {
				float Offset = (y - Sample) / (LastSample - Sample);
				PutPixel(m_pBlitBuffer.get(), m_iWidth, m_iHeight, x - Offset, y + HALF_HEIGHT + frac, LINE_COL1);
			}
		}
	}
}

void CVisualizerScope::Draw()
{
#ifdef _DEBUG
	static int _peak = 0;
	static int _min = 0;
	static int _max = 0;
#endif

	if (!(m_iSampleCount >= (unsigned int)(m_iWidth * TIME_SCALING))) {
		throw std::runtime_error("Not enough data supplied to CVisualizerScope::Draw()");
	}

	for (unsigned int Pos = 0; Pos < (unsigned int)m_iWidth; Pos++) {
		const unsigned int startSmp = TIME_SCALING * Pos;
		int Accum = 0;

		for (unsigned int rel = 0; rel < TIME_SCALING; rel++) {
			const auto Amplitude = m_pSamples[startSmp + rel];
			Accum += Amplitude;
#ifdef _DEBUG
			if (_min > Amplitude)
				_min = Amplitude;
			if (_max < Amplitude)
				_max = Amplitude;
			if (abs(Amplitude) > _peak)
				_peak = abs(Amplitude);
#endif
		}

		m_pWindowBuf[Pos] = Accum / TIME_SCALING;
	}

	RenderBuffer();

#ifdef _DEBUG
	_peak = _max - _min;
	m_iPeak = _peak;
	_peak = 0;
	_min = 0;
	_max = 0;
#endif
}

void CVisualizerScope::Display(CDC *pDC, bool bPaintMsg)
{
	CVisualizerBase::Display(pDC, bPaintMsg);		// // //

#ifdef _DEBUG
	CString PeakText;
	PeakText.Format(_T("%i"), m_iPeak);
	pDC->TextOut(0, 0, PeakText);
	PeakText.Format(_T("-%gdB"), 20.0 * log(double(m_iPeak) / 65535.0));
	pDC->TextOut(0, 16, PeakText);
#endif
}

size_t CVisualizerScope::NeededSamples() const
{
	return (size_t)(m_iWidth * TIME_SCALING);
}
