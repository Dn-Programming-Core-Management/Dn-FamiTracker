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

#include "VisualizerStatic.h"
#include <cmath>		// // //
#include "FamiTracker.h"		// // //
#include "Settings.h"		// // //

// derived from Generic bitmap font
// https://forums.nesdev.org/viewtopic.php?t=10284&start=6
static const uint8_t LOGO_FONT[][7] = {		// // !!
	// D
	{	0b01111110,
		0b01100011,
		0b01100011,
		0b01100011,
		0b01100011,
		0b01100011,
		0b01111110},
	// n
	{	0b00000000,
		0b00000000,
		0b01111110,
		0b01100011,
		0b01100011,
		0b01100011,
		0b01100011},
	// -
	{	0b00000000,
		0b00000000,
		0b00000000,
		0b00111110,
		0b00000000,
		0b00000000,
		0b00000000},
	// F
	{	0b00111111,
		0b01100000,
		0b01100000,
		0b01111100,
		0b01100000,
		0b01100000,
		0b01100000},
	// A
	{	0b00111110,
		0b01100011,
		0b01100011,
		0b01111111,
		0b01100011,
		0b01100011,
		0b01100011},
	// M
	{	0b01100011,
		0b01110111,
		0b01111111,
		0b01101011,
		0b01100011,
		0b01100011,
		0b01100011},
	// I
	{	0b00001100,
		0b00000000,
		0b00001100,
		0b00001100,
		0b00001100,
		0b00001100,
		0b00001100},
	// T
	{	0b00111111,
		0b00001100,
		0b00001100,
		0b00001100,
		0b00001100,
		0b00001100,
		0b00001100},
	// R
	{	0b01111110,
		0b01100011,
		0b01100011,
		0b01111110,
		0b01100011,
		0b01100011,
		0b01100011},
	// C
	{	0b00111110,
		0b01100000,
		0b01100000,
		0b01100000,
		0b01100000,
		0b01100000,
		0b00111110},
	// K
	{	0b01100011,
		0b01100011,
		0b01100110,
		0b01111100,
		0b01100110,
		0b01100011,
		0b01100011},
	// E
	{	0b00111111,
		0b01100000,
		0b01100000,
		0b01111100,
		0b01100000,
		0b01100000,
		0b00111111},
	// cool string
	{'D','n','-','F','T','!','~'}
};

CVisualizerStatic::~CVisualizerStatic()
{
	if (m_dcImage.m_hDC)
		m_dcImage.SelectObject(m_pOldBmp);
}

void CVisualizerStatic::SetSampleRate(int SampleRate)
{
}

void CVisualizerStatic::Draw()
{
	std::string STR = std::string(APP_NAME) + "~";		// // //
	static const size_t COUNT = std::size(STR);
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

void CVisualizerStatic::DrawChar(char letter, int xPos, int yPos, const COLORREF &Color)		// // //
{
	const auto drawFunc = [&] (const uint8_t (&glyph)[7]) {
		for (int i = 0; i < 7; ++i) {
			if (yPos >= 0 && yPos < m_iHeight) {
				int x = xPos;
				char Row = glyph[i];
				for (int j = 0; j < 8; ++j) {
					if (x >= 0 && x < m_iWidth && Row < 0)
						m_pBlitBuffer[yPos * m_iWidth + x] = Color;
					Row <<= 1;
					++x;
				}
			}
			++yPos;
		}
	};


	switch (letter) {
	case 'D': case 'd': return drawFunc(LOGO_FONT[0]);
	case 'N': case 'n': return drawFunc(LOGO_FONT[1]);
	case '-':			return drawFunc(LOGO_FONT[2]);
	case 'F': case 'f': return drawFunc(LOGO_FONT[3]);
	case 'A': case 'a': return drawFunc(LOGO_FONT[4]);
	case 'M': case 'm': return drawFunc(LOGO_FONT[5]);
	case 'I': case 'i': return drawFunc(LOGO_FONT[6]);
	case 'T': case 't': return drawFunc(LOGO_FONT[7]);
	case 'R': case 'r': return drawFunc(LOGO_FONT[8]);
	case 'C': case 'c': return drawFunc(LOGO_FONT[9]);
	case 'K': case 'k': return drawFunc(LOGO_FONT[10]);
	case 'E': case 'e': return drawFunc(LOGO_FONT[11]);
	}
}
