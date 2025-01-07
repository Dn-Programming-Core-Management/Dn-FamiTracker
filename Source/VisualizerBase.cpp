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

bool CVisualizerBase::SetScopeData(short const* iSamples, unsigned int iCount) {
	return false;
}

bool CVisualizerBase::SetSpectrumData(short const* iSamples, unsigned int iCount) {
	return false;
}

void CVisualizerBase::Display(CDC *pDC, bool bPaintMsg) {		// // //
	StretchDIBits(pDC->m_hDC,
		0, 0, m_iWidth, m_iHeight,
		0, 0, m_iWidth, m_iHeight,
		m_pBlitBuffer.get(), &m_bmi, DIB_RGB_COLORS, SRCCOPY);
}
