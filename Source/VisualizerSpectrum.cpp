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

#include "VisualizerSpectrum.h"
#include "FamiTracker.h"
#include "Graphics.h"

/*
 * Displays a spectrum analyzer
 *
 */

CVisualizerSpectrum::CVisualizerSpectrum(int Size) :		// // //
	m_iBarSize(Size)
{
}

void CVisualizerSpectrum::Create(int Width, int Height)
{
	CVisualizerBase::Create(Width, Height);

	std::fill(m_pBlitBuffer.get(), m_pBlitBuffer.get() + Width * Height, BG_COLOR);		// // //
}

void CVisualizerSpectrum::SetSampleRate(int SampleRate)
{
	fft_buffer_ = FftBuffer<FFT_POINTS> { };		// // //

	m_fFftPoint.fill(0.f);		// // //

	m_iSampleCount = 0;
	m_iFillPos = 0;
}

void CVisualizerSpectrum::Transform(short *pSamples, unsigned int Count)
{
	fft_buffer_.CopyIn(pSamples, Count);
	fft_buffer_.Transform();
}

void CVisualizerSpectrum::SetSampleData(short *pSamples, unsigned int iCount)
{
	CVisualizerBase::SetSampleData(pSamples, iCount);

	Transform(pSamples, iCount);
	/*
	int offset = 0;

	if (m_iFillPos > 0) {
		const int size = std::min((int)iCount, FFT_POINTS - m_iFillPos);
		std::copy_n(pSamples, size, m_pSampleBuffer.begin() + m_iFillPos);
		Transform(m_pSampleBuffer.data(), FFT_POINTS);
		offset += size;
		iCount -= size;
	}

	while (iCount >= FFT_POINTS) {
		Transform(pSamples + offset, FFT_POINTS);
		offset += FFT_POINTS;
		iCount -= FFT_POINTS;
	}

	// Copy rest
	std::copy_n(pSamples + offset, iCount, m_pSampleBuffer.begin());
	m_iFillPos = iCount;
	*/
}

void CVisualizerSpectrum::Draw()
{
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
			level += float(fft_buffer_.GetIntensity(LastStep + j)) / SCALING;
		level /= steps;
		
		// linear -> db
		level = (20 * std::log10(level));// *0.8f;

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
				m_pBlitBuffer[(m_iHeight - 1 - y) * m_iWidth + i * m_iBarSize + x + OFFSET] = y < level ? Color : BG_COLOR;
			}
		}	
		
		LastStep = iStep;
		Pos += Step;
	}
}
