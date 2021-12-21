/*
** FamiTracker - NES/Famicom sound tracker
** Copyright (C) 2005-2014  Jonathan Liss
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
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU 
** Library General Public License for more details.  To obtain a 
** copy of the GNU Library General Public License, write to the Free 
** Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
**
** Any permitted reproduction of these routines, in whole or in part,
** must bear this legend.
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
