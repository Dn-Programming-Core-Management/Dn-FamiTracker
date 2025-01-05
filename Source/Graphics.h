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


// Various graphics helpers

// Color macros

#define RED(x)	 (x & 255)
#define GREEN(x) ((x >> 8) & 255)
#define BLUE(x)	 ((x >> 16) & 255)
#define ALPHA(x) ((x >> 24) & 255)

#define COMBINE(r, g, b) (((b) << 16) | ((g) << 8) | r)

#define DIM(c, l) (COMBINE((RED(c) * l) / 100, (GREEN(c) * l) / 100, (BLUE(c) * l) / 100))

#define BLEND_COLOR(c1, c2, level) ((c1 * level) / 100 + (c2 * (100 - level)) / 100)

#define BLEND(c1, c2, level) (COMBINE(BLEND_COLOR(RED(c1), RED(c2), (level)), \
									  BLEND_COLOR(GREEN(c1), GREEN(c2), (level)), \
									  BLEND_COLOR(BLUE(c1), BLUE(c2), (level))))

#define INTENSITY(c) ((((c >> 16) & 0xFF) + ((c >> 8) & 0xFF) + (c & 0xFF)) / 3)

// Functions

void GradientRectTriple(CDC *pDC, int x, int y, int w, int h, COLORREF c1, COLORREF c2, COLORREF c3);
void GradientBar(CDC *pDC, int x, int y, int w, int h, COLORREF col_fg, COLORREF col_bg);
void GradientRect(CDC *pDC, int x, int y, int w, int h, COLORREF top_col, COLORREF bottom_col);

// // // support for rectangles
void GradientRectTriple(CDC *pDC, const CRect &r, COLORREF c1, COLORREF c2, COLORREF c3);
void GradientBar(CDC *pDC, const CRect &r, COLORREF col_fg, COLORREF col_bg);
void GradientRect(CDC *pDC, const CRect &r, COLORREF top_col, COLORREF bottom_col);

void BlurBuffer(COLORREF *pBuffer, int Width, int Height, const int *pColorDecay);
void PutPixel(COLORREF *pBuffer, int Width, int Height, float x, float y, COLORREF col);
