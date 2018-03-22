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

#include "VisualizerBase.h"

void CVisualizerBase::Create(int Width, int Height)
{
	memset(&m_bmi, 0, sizeof(BITMAPINFO));
	m_bmi.bmiHeader.biSize	   = sizeof(BITMAPINFOHEADER);
	m_bmi.bmiHeader.biBitCount = 32;
	m_bmi.bmiHeader.biHeight   = -Height;
	m_bmi.bmiHeader.biWidth	   = Width;
	m_bmi.bmiHeader.biPlanes   = 1;

	m_iWidth = Width;
	m_iHeight = Height;
	m_pBlitBuffer = std::make_unique<COLORREF[]>(Width * Height);		// // //
}

void CVisualizerBase::SetSampleData(short *pSamples, unsigned int iCount)
{
	m_pSamples = pSamples;
	m_iSampleCount = iCount;
}

void CVisualizerBase::Display(CDC *pDC, bool bPaintMsg) {		// // //
	StretchDIBits(pDC->m_hDC,
		0, 0, m_iWidth, m_iHeight,
		0, 0, m_iWidth, m_iHeight,
		m_pBlitBuffer.get(), &m_bmi, DIB_RGB_COLORS, SRCCOPY);
}
