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
