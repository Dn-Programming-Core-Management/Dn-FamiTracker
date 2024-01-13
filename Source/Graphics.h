/*
** FamiTracker - NES/Famicom sound tracker
** Copyright (C) 2005-2020 Jonathan Liss
**
** 0CC-FamiTracker is (C) 2014-2018 HertzDevil
**
** Dn-FamiTracker is (C) 2020-2024 D.P.C.M.
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
