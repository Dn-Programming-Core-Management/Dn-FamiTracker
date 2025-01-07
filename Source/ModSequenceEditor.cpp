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

// This is the modulation editor for FDS

#include <memory>		// // //
#include "stdafx.h"
#include "Instrument.h"
#include "SeqInstrument.h"		// // //
#include "InstrumentFDS.h"		// // //
#include "ModSequenceEditor.h"
#include "DPI.h"		// // //

int SIZE_X = 12;

IMPLEMENT_DYNAMIC(CModSequenceEditor, CWnd)

BEGIN_MESSAGE_MAP(CModSequenceEditor, CWnd)
	ON_WM_PAINT()
	ON_WM_MOUSEMOVE()
	ON_WM_LBUTTONDOWN()
END_MESSAGE_MAP()


CModSequenceEditor::CModSequenceEditor()
{
}

CModSequenceEditor::~CModSequenceEditor()
{
}

BOOL CModSequenceEditor::CreateEx(DWORD dwExStyle, LPCTSTR lpszClassName, LPCTSTR lpszWindowName, DWORD dwStyle, const RECT& rect, CWnd* pParentWnd)
{
	CRect newRect;

	// SIZE_X = DPI::SX(12);		// // //

	newRect.top = rect.top;
	newRect.left = rect.left;
	newRect.bottom = rect.top + 61 + 4;
	newRect.right = rect.left + 32 * (SIZE_X + 2) + 4 + 4;

	if (CWnd::CreateEx(dwExStyle, lpszClassName, lpszWindowName, dwStyle, newRect, pParentWnd, 0) == -1)
		return -1;

	return 0;
}

void FillItem(CDC *pDC, int Index, int Value, bool Remove)
{
	const int POS[] = {0, 7, 14, 21, 28, 35, 42, 52};
	const int VAL[] = {3, 2, 1, 0, 7, 6, 5, 4};
	const int COL[] = {0x00FF00, 0x00FF00, 0x00FF00, 0x00FFFF, 0x00FF00, 0x00FF00, 0x00FF00, 0xFF0000};

	int x = 2 + Index * (SIZE_X + 2);
	pDC->FillSolidRect(x, POS[VAL[Value]] + 2, SIZE_X, 5, Remove == false ? COL[VAL[Value]] : 0xD0D0D0);
}

void CModSequenceEditor::OnPaint()
{
	CPaintDC dc(this);

	// Draw the sample
	dc.FillSolidRect(0, 0, 32 * (SIZE_X + 2) + 4 /*388*/, 61, 0xA0A0A0);

	for (int i = 0; i < 32; i++) {
		int x = 2 + i * (SIZE_X + 2);

		int mod = m_pInstrument->GetModulation(i);

		dc.FillSolidRect(x, 2 + 0, SIZE_X, 5, mod == 3 ? 0x00FF00 : 0xD0D0D0);		// +4
 		dc.FillSolidRect(x, 2 + 7, SIZE_X, 5, mod == 2 ? 0x00FF00 : 0xD0D0D0);		// +2
		dc.FillSolidRect(x, 2 + 14, SIZE_X, 5, mod == 1 ? 0x00FF00 : 0xD0D0D0);		// +1
		dc.FillSolidRect(x, 2 + 21, SIZE_X, 5, mod == 0 ? 0x00FFFF : 0xD0D0D0);		// 0
		dc.FillSolidRect(x, 2 + 28, SIZE_X, 5, mod == 7 ? 0x00FF00 : 0xD0D0D0);		// -1
		dc.FillSolidRect(x, 2 + 35, SIZE_X, 5, mod == 6 ? 0x00FF00 : 0xD0D0D0);		// -2
		dc.FillSolidRect(x, 2 + 42, SIZE_X, 5, mod == 5 ? 0x00FF00 : 0xD0D0D0);		// -4
		dc.FillSolidRect(x, 2 + 52, SIZE_X, 5, mod == 4 ? 0xFF0000 : 0xD0D0D0);		// reset
	}

}

void CModSequenceEditor::SetInstrument(std::shared_ptr<CInstrumentFDS> pInst)
{
	m_pInstrument = pInst;
	Invalidate();
	RedrawWindow();
}

void CModSequenceEditor::OnMouseMove(UINT nFlags, CPoint point)
{
	if (nFlags & MK_LBUTTON)
		EditSequence(point);

	CWnd::OnMouseMove(nFlags, point);
}

void CModSequenceEditor::OnLButtonDown(UINT nFlags, CPoint point)
{
	EditSequence(point);

	CWnd::OnLButtonDown(nFlags, point);
}

void CModSequenceEditor::EditSequence(CPoint point)
{	
	const int VALUES[] = {3, 2, 1, 0, 7, 6, 5, 4};

	int index = (point.x - 2) / (SIZE_X + 2);
	int value = ((point.y - 3) / 7);

	if (index < 0)
		index = 0;
	if (index > 31)
		index = 31;

	if (value < 0)
		value = 0;
	if (value > 7)
		value = 7;

	value = VALUES[value];

	CDC *pDC = GetDC();

	if (pDC != NULL) {
		int s = m_pInstrument->GetModulation(index);
		FillItem(pDC, index, s, true);
		m_pInstrument->SetModulation(index, value);
		FillItem(pDC, index, value, false);
		ReleaseDC(pDC);
	}

	// Notify parent that the sequence has changed
	GetParent()->PostMessage(WM_USER + 1);
}
