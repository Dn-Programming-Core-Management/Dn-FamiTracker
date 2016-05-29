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

#include "stdafx.h"
#include "FamiTracker.h"
#include "VisualizerWnd.h"
#include "VisualizerSpectrum.h"
#include "Graphics.h"
#include "fft\fft.h"

/*
 * Displays a spectrum analyzer
 *
 */

CVisualizerSpectrum::CVisualizerSpectrum(int Size) :		// // //
	m_iBarSize(Size),
	m_pBlitBuffer(NULL),
	m_pFftObject(NULL)
{
}

CVisualizerSpectrum::~CVisualizerSpectrum()
{
	SAFE_RELEASE_ARRAY(m_pBlitBuffer);
	SAFE_RELEASE(m_pFftObject);
}

void CVisualizerSpectrum::Create(int Width, int Height)
{
	CVisualizerBase::Create(Width, Height);

	SAFE_RELEASE_ARRAY(m_pBlitBuffer);

	m_pBlitBuffer = new COLORREF[Width * Height];
	memset(m_pBlitBuffer, BG_COLOR, Width * Height * sizeof(COLORREF));

	// Calculate window function (Hann)
	float fraction = 6.283185f / (FFT_POINTS - 1);
	for (int i = FFT_POINTS; i--;)
		m_fWindow[i] = 0.5f * (1.0f - cosf(float(i * fraction)));
}

void CVisualizerSpectrum::SetSampleRate(int SampleRate)
{
	SAFE_RELEASE(m_pFftObject);

	m_pFftObject = new Fft(FFT_POINTS, SampleRate);

	memset(m_fFftPoint, 0, sizeof(float) * FFT_POINTS);

	m_iSampleCount = 0;
	m_iFillPos = 0;
}

void CVisualizerSpectrum::Transform(short *pSamples, unsigned int Count)
{
	ASSERT(m_pFftObject != NULL);

	for (int i = Count; i--;)
		pSamples[i] = short(pSamples[i] * m_fWindow[i]);
	
	m_pFftObject->CopyIn(FFT_POINTS, pSamples);
	m_pFftObject->Transform();
}

void CVisualizerSpectrum::SetSampleData(short *pSamples, unsigned int iCount)
{
	CVisualizerBase::SetSampleData(pSamples, iCount);

	int size, offset = 0;

	if (m_iFillPos > 0) {
		size = FFT_POINTS - m_iFillPos;
		memcpy(m_pSampleBuffer + m_iFillPos, pSamples, size * sizeof(short));
		Transform(m_pSampleBuffer, FFT_POINTS);
		offset += size;
		iCount -= size;
	}

	while (iCount >= FFT_POINTS) {
		Transform(pSamples + offset, FFT_POINTS);
		offset += FFT_POINTS;
		iCount -= FFT_POINTS;
	}

	// Copy rest
	size = iCount;
	memcpy(m_pSampleBuffer, pSamples + offset, size * sizeof(short));
	m_iFillPos = size;
}

void CVisualizerSpectrum::Draw()
{
	ASSERT(m_pFftObject != NULL);

	static const float SCALING = 250.0f;
	static const int OFFSET = 0;
	static const float DECAY = 3.0f;

	float Step = 0.2f * (float(FFT_POINTS) / float(m_iWidth)) * m_iBarSize;		// // //
	float Pos = 2;	// Add a small offset to remove note on/off actions

	int LastStep = 0;

	for (int i = 0; i < m_iWidth / m_iBarSize; i++) {		// // //
		int iStep = int(Pos + 0.5f);
		
		float level = 0;
		int steps = (iStep - LastStep) + 1;
		for (int j = 0; j < steps; ++j)
			level += float(m_pFftObject->GetIntensity(LastStep + j)) / SCALING;
		level /= steps;
		
		// linear -> db
		level = (20 * logf(level / 4.0f)) * 0.8f;

		if (level < 0.0f)
			level = 0.0f;
		if (level > float(m_iHeight))
			level = float(m_iHeight);

		if (iStep != LastStep) {
			if (level >= m_fFftPoint[iStep])
				m_fFftPoint[iStep] = level;
			else
				m_fFftPoint[iStep] -= DECAY;

			if (m_fFftPoint[iStep] < 1.0f)
				m_fFftPoint[iStep] = 0.0f;
		}

		level = m_fFftPoint[iStep];

		for (int y = 0; y < m_iHeight; ++y) {
			COLORREF Color = BLEND(0x6060FF, 0xFFFFFF, (y * 100) / int(level + 1));
			if (y == 0)
				Color = DIM(Color, 90);
			if (m_iBarSize > 1 && (y & 1))		// // //
				Color = DIM(Color, 40);
			for (int x = 0; x < m_iBarSize; ++x) {		// // //
				if (m_iBarSize > 1 && x == m_iBarSize - 1)
					Color = DIM(Color, 50);
				m_pBlitBuffer[(m_iHeight - y - 1) * m_iWidth + i * m_iBarSize + x + OFFSET] = y < level ? Color : BG_COLOR;
			}
		}	
		
		LastStep = iStep;
		Pos += Step;
	}
}

void CVisualizerSpectrum::Display(CDC *pDC, bool bPaintMsg)
{
	StretchDIBits(pDC->m_hDC, 0, 0, m_iWidth, m_iHeight, 0, 0, m_iWidth, m_iHeight, m_pBlitBuffer, &m_bmi, DIB_RGB_COLORS, SRCCOPY);
}

