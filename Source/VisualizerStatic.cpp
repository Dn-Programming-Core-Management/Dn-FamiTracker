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
#include <cmath>		// // //
#include "VisualizerWnd.h"
#include "VisualizerStatic.h"
#include "FamiTracker.h"		// // //
#include "Settings.h"		// // //
#include "resource.h"

static const char LOGO_FONT[][7] = {		// // //
	{0x3C, 0x66, 0x66, 0x66, 0x66, 0x66, 0x3C}, // 0
	{0x3C, 0x66, 0x60, 0x60, 0x60, 0x66, 0x3C}, // C
	{0x00, 0x00, 0x00, 0x7E, 0x00, 0x00, 0x00}, // -
	{0x7E, 0x60, 0x60, 0x78, 0x60, 0x60, 0x60}, // F
	{0x3C, 0x66, 0x66, 0x66, 0x7E, 0x66, 0x66}, // A
	{0x63, 0x77, 0x7F, 0x6B, 0x63, 0x63, 0x63}, // M
	{0x7E, 0x18, 0x18, 0x18, 0x18, 0x18, 0x7E}, // I
	{0x7E, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18}, // T
	{0x7C, 0x66, 0x66, 0x7C, 0x66, 0x66, 0x66}, // R
	{0x66, 0x6C, 0x78, 0x70, 0x78, 0x6C, 0x66}, // K
	{0x7E, 0x60, 0x60, 0x78, 0x60, 0x60, 0x7E}, // E
};

CVisualizerStatic::CVisualizerStatic() :
	m_pBlitBuffer(nullptr)
{
}

CVisualizerStatic::~CVisualizerStatic()
{
	SAFE_RELEASE_ARRAY(m_pBlitBuffer);		// // //
	if (m_dcImage.m_hDC != NULL) {
		m_dcImage.SelectObject(m_pOldBmp);
	}
}

void CVisualizerStatic::Create(int Width, int Height)
{
	CVisualizerBase::Create(Width, Height);

	m_pBlitBuffer = new COLORREF[Width * (Height + 1)]();		// // //
}

void CVisualizerStatic::SetSampleRate(int SampleRate)
{
}

void CVisualizerStatic::Draw()
{
	static const char *STR = "0CC-FamiTracker";		// // //
	static const size_t COUNT = strlen(STR);
	static long long t = 0;

	const auto FixRGB = [] (int x) { return RGB(GetBValue(x), GetGValue(x), GetRValue(x)); };

	const COLORREF Back[] = {
		FixRGB(theApp.GetSettings()->Appearance.iColBackground),
		FixRGB(theApp.GetSettings()->Appearance.iColBackgroundHilite),
		FixRGB(theApp.GetSettings()->Appearance.iColBackgroundHilite2)
	};
	const COLORREF Color = FixRGB(theApp.GetSettings()->Appearance.iColPatternText);
	const COLORREF Shadow = RGB((GetRValue(Color) + GetRValue(Back[0]) * 2) / 3,
								(GetGValue(Color) + GetGValue(Back[0]) * 2) / 3,
								(GetBValue(Color) + GetBValue(Back[0]) * 2) / 3);

	for (int y = m_iHeight - 1; y >= 0; --y)
		for (int x = m_iWidth - 1; x >= 0; --x) {
			int Dist = (abs(x - m_iWidth / 2) + abs(y - m_iHeight / 2) - t / 5) % 12;
			if (Dist < 0) Dist += 12;
			m_pBlitBuffer[y * m_iWidth + x] = Back[Dist / 4];
		}
	
	for (size_t i = 0; i < COUNT; ++i) {
		double Phase = .07 * t - .9 * i;
		double x = sin(Phase) * 2. + m_iWidth + 11. * i - .4 * t;
		double y = sin(Phase) * 7.;
		const double MAX = m_iWidth + 120.;
		if (x < 0) {
			x = fmod(x, MAX);
			if (x < -40.) x += MAX;
		}
		DrawChar(STR[i], static_cast<int>(x) + 1, static_cast<int>(m_iHeight / 2. - 3.5 - y) + 1, Shadow);
		DrawChar(STR[i], static_cast<int>(x), static_cast<int>(m_iHeight / 2. - 3.5 - y), Color);
	}

	++t;
}

void CVisualizerStatic::DrawChar(char n, int xPos, int yPos, const COLORREF &Color)		// // //
{
	int Index = 0;
	switch (n) {
	case '0': break;
	case 'C': case 'c': Index = 1; break;
	case '-': Index = 2; break;
	case 'F': case 'f': Index = 3; break;
	case 'A': case 'a': Index = 4; break;
	case 'M': case 'm': Index = 5; break;
	case 'I': case 'i': Index = 6; break;
	case 'T': case 't': Index = 7; break;
	case 'R': case 'r': Index = 8; break;
	case 'K': case 'k': Index = 9; break;
	case 'E': case 'e': Index = 10; break;
	default: return;
	}
	for (int i = 0; i < 7; ++i) {
		if (yPos >= 0 && yPos < m_iHeight) {
			int x = xPos;
			char Row = LOGO_FONT[Index][i];
			for (int j = 0; j < 8; ++j) {
				if (x >= 0 && x < m_iWidth && Row < 0)
					m_pBlitBuffer[yPos * m_iWidth + x] = Color;
				Row <<= 1;
				++x;
			}
		}
		++yPos;
	}
}

void CVisualizerStatic::Display(CDC *pDC, bool bPaintMsg)
{
	StretchDIBits(pDC->m_hDC, 0, 0, m_iWidth, m_iHeight, 0, 0, m_iWidth, m_iHeight, m_pBlitBuffer, &m_bmi, DIB_RGB_COLORS, SRCCOPY);
}
