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
#include "DPI.h"

const int DPI::DEFAULT_DPI = 96;
int DPI::_dpiX = DPI::DEFAULT_DPI;
int DPI::_dpiY = DPI::DEFAULT_DPI;

int DPI::SX(int pt)
{
	return ::MulDiv(pt, _dpiX, DPI::DEFAULT_DPI);
}

int DPI::SY(int pt)
{
	return ::MulDiv(pt, _dpiY, DPI::DEFAULT_DPI);
}

void DPI::ScaleMouse(CPoint &pt)
{
	pt.x = SX(pt.x);
	pt.y = SY(pt.y);
}

void DPI::ScaleRect(CRect &r)		// // //
{
	r.left = SX(r.left);
	r.right = SX(r.right);
	r.top = SY(r.top);
	r.bottom = SY(r.bottom);
}

CRect DPI::Rect(int x, int y, int w, int h)
{
	return CRect {SX(x), SY(y), SX(x + w), SY(y + h)};
}

void DPI::SetScale(int X, int Y)		// // //
{
	_dpiX = X;
	_dpiY = Y;
}
