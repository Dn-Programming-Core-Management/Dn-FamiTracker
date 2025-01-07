/*
** Dn-FamiTracker - NES/Famicom sound tracker
** Copyright (C) 2020-2025 D.P.C.M.
** FamiTracker Copyright (C) 2005-2020 Jonathan Liss
** 0CC-FamiTracker Copyright (C) 2014-2018 HertzDevil
**
** This program is free software: you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation, either version 3 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program. If not, see https://www.gnu.org/licenses/.
*/


#pragma once

#include "stdafx.h"
#include "FFT/FftBuffer.h"		// // //
#include <memory>
#include "VisualizerBase.h"		// // //

// CVisualizerSpectrum, spectrum style visualizer

const int FFT_POINTS = 1024;

class CVisualizerSpectrum : public CVisualizerBase
{
public:
	CVisualizerSpectrum(int Size);		// // //

	void Create(int Width, int Height) override;
	void SetSampleRate(int SampleRate) override;
	bool SetSpectrumData(short const* pSamples, unsigned int iCount) override;
	void Draw() override;

protected:
	void Transform(short const* pSamples, unsigned int Count);

private:
	static const COLORREF BG_COLOR = 0;
	const int m_iBarSize;

	FftBuffer<FFT_POINTS> fft_buffer_;		// // //

	int m_iFillPos;
	std::array<short, FFT_POINTS> m_pSampleBuffer = { };
	std::array<float, FFT_POINTS> m_fFftPoint = { };
};
