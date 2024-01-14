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

// Some graphics helpers

#include "stdafx.h"
#include "Graphics.h"

// // // local methods using right/bottom instead of width/height

static void __GradientRectTriple(CDC *pDC, int x, int y, int r, int b, COLORREF c1, COLORREF c2, COLORREF c3)
{
	// c1 -> c2 -> c3
	TRIVERTEX Vertices[4];
	GRADIENT_RECT Rects[2];

	Vertices[0].x = x;
	Vertices[0].y = y;
	Vertices[0].Red = RED(c1) << 8;
	Vertices[0].Green = GREEN(c1) << 8;
	Vertices[0].Blue = BLUE(c1) << 8;
	Vertices[0].Alpha = 0xFF00;

	Vertices[1].x = r;
	Vertices[1].y = (2 * y + b) / 3;
	Vertices[1].Red = RED(c2) << 8;
	Vertices[1].Green = GREEN(c2) << 8;
	Vertices[1].Blue = BLUE(c2) << 8;
	Vertices[1].Alpha = 0xFF00;

	Vertices[2].x = x;
	Vertices[2].y = (2 * y + b) / 3;
	Vertices[2].Red = RED(c2) << 8;
	Vertices[2].Green = GREEN(c2) << 8;
	Vertices[2].Blue = BLUE(c2) << 8;
	Vertices[2].Alpha = 0xFF00;

	Vertices[3].x = r;
	Vertices[3].y = b;
	Vertices[3].Red = RED(c3) << 8;
	Vertices[3].Green = GREEN(c3) << 8;
	Vertices[3].Blue = BLUE(c3) << 8;
	Vertices[3].Alpha = 0xFF00;

	Rects[0].UpperLeft = 0;
	Rects[0].LowerRight = 1;
	Rects[1].UpperLeft = 2;
	Rects[1].LowerRight = 3;

	pDC->GradientFill(Vertices, 4, Rects, 2, GRADIENT_FILL_RECT_V);
}

static void __GradientBar(CDC *pDC, int x, int y, int r, int b, COLORREF col_fg, COLORREF col_bg)
{
	TRIVERTEX Vertices[2];
	GRADIENT_RECT Rect;

	COLORREF col2 = BLEND(col_fg, col_bg, 60);
	COLORREF top_col = BLEND(col_fg, 0xFFFFFF, 95);

	Vertices[0].x = x;
	Vertices[0].y = y + 1;
	Vertices[0].Red = RED(col_fg) << 8;
	Vertices[0].Green = GREEN(col_fg) << 8;
	Vertices[0].Blue = BLUE(col_fg) << 8;
	Vertices[0].Alpha = 0xFF00;

	Vertices[1].x = r;
	Vertices[1].y = b;
	Vertices[1].Red = RED(col2) << 8;
	Vertices[1].Green = GREEN(col2) << 8;
	Vertices[1].Blue = BLUE(col2) << 8;
	Vertices[1].Alpha = 0xFF00;

	Rect.UpperLeft = 0;
	Rect.LowerRight = 1;

	pDC->FillSolidRect(x, y, r - x, 1, top_col);
	pDC->GradientFill(Vertices, 2, &Rect, 1, GRADIENT_FILL_RECT_V);
}

static void __GradientRect(CDC *pDC, int x, int y, int r, int b, COLORREF top_col, COLORREF bottom_col)
{
	TRIVERTEX Vertices[2];
	GRADIENT_RECT Rect;

	Vertices[0].x = x;
	Vertices[0].y = y;
	Vertices[0].Red = RED(top_col) << 8;
	Vertices[0].Green = GREEN(top_col) << 8;
	Vertices[0].Blue = BLUE(top_col) << 8;
	Vertices[0].Alpha = 0xFF00;

	Vertices[1].x = r;
	Vertices[1].y = b;
	Vertices[1].Red = RED(bottom_col) << 8;
	Vertices[1].Green = GREEN(bottom_col) << 8;
	Vertices[1].Blue = BLUE(bottom_col) << 8;
	Vertices[1].Alpha = 0xFF00;

	Rect.UpperLeft = 0;
	Rect.LowerRight = 1;

	pDC->GradientFill(Vertices, 2, &Rect, 1, GRADIENT_FILL_RECT_V);
}

void GradientRectTriple(CDC *pDC, int x, int y, int w, int h, COLORREF c1, COLORREF c2, COLORREF c3)
{
	__GradientRectTriple(pDC, x, y, x + w, y + h, c1, c2, c3);		// // //
}

void GradientBar(CDC *pDC, int x, int y, int w, int h, COLORREF col_fg, COLORREF col_bg)
{
	__GradientBar(pDC, x, y, x + w, y + h, col_fg, col_bg);		// // //
}

void GradientRect(CDC *pDC, int x, int y, int w, int h, COLORREF top_col, COLORREF bottom_col)
{
	__GradientRect(pDC, x, y, x + w, y + h, top_col, bottom_col);		// // //
}

void GradientRectTriple(CDC *pDC, const CRect &r, COLORREF c1, COLORREF c2, COLORREF c3)		// // //
{
	__GradientRectTriple(pDC, r.left, r.top, r.right, r.bottom, c1, c2, c3);
}

void GradientBar(CDC *pDC, const CRect &r, COLORREF col_fg, COLORREF col_bg)		// // //
{
	__GradientBar(pDC, r.left, r.top, r.right, r.bottom, col_fg, col_bg);
}

void GradientRect(CDC *pDC, const CRect &r, COLORREF top_col, COLORREF bottom_col)		// // //
{
	__GradientRect(pDC, r.left, r.top, r.right, r.bottom, top_col, bottom_col);
}

void BlurBuffer(COLORREF *pBuffer, int Width, int Height, const int *pColorDecay)
{
	for (int x = 1; x < Width - 1; ++x) {
		for (int y = 1; y < Height - 1; ++y) {

			COLORREF Col1 = pBuffer[(y + 1) * Width + (x + 0)];
			COLORREF Col2 = pBuffer[(y - 1) * Width + (x + 0)];
			COLORREF Col3 = pBuffer[(y + 0) * Width + (x + 1)];
			COLORREF Col4 = pBuffer[(y + 0) * Width + (x - 1)];
			COLORREF Col5 = pBuffer[(y - 1) * Width + (x + 1)];
			COLORREF Col6 = pBuffer[(y - 1) * Width + (x - 1)];
			COLORREF Col7 = pBuffer[(y + 1) * Width + (x + 1)];
			COLORREF Col8 = pBuffer[(y + 1) * Width + (x - 1)];

			int r = (RED(Col1) + RED(Col2) + RED(Col3) + RED(Col4) + RED(Col5) + RED(Col6) + RED(Col7) + RED(Col8)) >> 3;
			int g = (GREEN(Col1) + GREEN(Col2) + GREEN(Col3) + GREEN(Col4) + GREEN(Col5) + GREEN(Col6) + GREEN(Col7) + GREEN(Col8)) >> 3;
			int b = (BLUE(Col1) + BLUE(Col2) + BLUE(Col3) + BLUE(Col4) + BLUE(Col5) + BLUE(Col6) + BLUE(Col7) + BLUE(Col8)) >> 3;
			
			r -= pColorDecay[0];
			g -= pColorDecay[1];
			b -= pColorDecay[2];
			
			if (r < 0) r = 0;
			if (g < 0) g = 0;
			if (b < 0) b = 0;
			if (r > 255) r = 255;
			if (g > 255) g = 255;
			if (b > 255) b = 255;

			pBuffer[y * Width + x] = COMBINE(r, g, b);
		}
	}

	for (int x = 0; x < Width; ++x) {
		pBuffer[x] = 0;
		pBuffer[x + (Height - 1) * Width] = 0;
	}

	for (int y = 0; y < Height; ++y) {
		pBuffer[y * Width] = 0;
		pBuffer[y * Width + (Width - 1)] = 0;
	}
}

void PutPixel(COLORREF *pBuffer, int Width, int Height, float x, float y, COLORREF col)
{
	if (x < 0.0f)
		x = 0.0f;
	if (x > float(Width - 1))
		x = float(Width - 1);
	if (y < 0.0f)
		y = 0.0f;
	if (y > float(Height - 1))
		y = float(Height - 1);

	int x0 = int(x);
	int x1 = x0 + 1;
	int y0 = int(y);
	int y1 = y0 + 1;

	float w_x1 = x - x0;
	float w_x0 = 1 - w_x1;
	float w_y1 = y - y0;
	float w_y0 = 1 - w_y1;

	if (true) {
		COLORREF c1 = BLEND(col, pBuffer[y0 * Width + x0], int((w_x0 * w_y0) * 100.0f));
		pBuffer[y0 * Width + x0] = c1;
	}
	if (x1 < Width) {
		COLORREF c2 = BLEND(col, pBuffer[y0 * Width + x1], int((w_x1 * w_y0) * 100.0f));
		pBuffer[y0 * Width + x1] = c2;
	}
	if (y1 < Height) {
		COLORREF c3 = BLEND(col, pBuffer[y1 * Width + x0], int((w_x0 * w_y1) * 100.0f));
		pBuffer[y1 * Width + x0] = c3;
	}
	if (x1 < Width && y1 < Height) {
		COLORREF c4 = BLEND(col, pBuffer[y1 * Width + x1], int((w_x1 * w_y1) * 100.0f));
		pBuffer[y1 * Width + x1] = c4;
	}
}
