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


// CVisualizerSpectrum, spectrum style visualizer

class Fft;

const int FFT_POINTS = 256;

class CVisualizerSpectrum : public CVisualizerBase
{
public:
	CVisualizerSpectrum(int Size);		// // //
	virtual ~CVisualizerSpectrum();

	void Create(int Width, int Height);
	void SetSampleRate(int SampleRate);
	void SetSampleData(short *iSamples, unsigned int iCount);
	void Draw();
	void Display(CDC *pDC, bool bPaintMsg);

protected:
	void Transform(short *pSamples, unsigned int Count);

private:
	static const COLORREF BG_COLOR = 0;
	const int m_iBarSize;

	COLORREF *m_pBlitBuffer;
	Fft	*m_pFftObject;

	int m_iFillPos;
	short m_pSampleBuffer[FFT_POINTS];
	float m_fWindow[FFT_POINTS];
	float m_fFftPoint[FFT_POINTS];
};
