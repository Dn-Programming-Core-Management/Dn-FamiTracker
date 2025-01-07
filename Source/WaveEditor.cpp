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

#include <iterator> 
#include <string>
#include <sstream>
#include "stdafx.h"
#include "FamiTracker.h"
#include "APU/Types.h"		// // //
#include "Instrument.h"
#include "SeqInstrument.h"
#include "InstrumentFDS.h"		// // //
#include "InstrumentN163.h"		// // //
#include "WaveEditor.h"
#include "Graphics.h"
#include "SoundGen.h"
#include "DPI.h"		// // //

/*
 * This is the wave editor for FDS and N163
 *
 */

using namespace std;

bool CWaveEditorFDS::m_bLineMode = true;
bool CWaveEditorN163::m_bLineMode = false;

IMPLEMENT_DYNAMIC(CWaveEditor, CWnd)

BEGIN_MESSAGE_MAP(CWaveEditor, CWnd)
	ON_WM_PAINT()
	ON_WM_MOUSEMOVE()
	ON_WM_LBUTTONDOWN()
	ON_WM_RBUTTONDOWN()
	ON_WM_LBUTTONUP()
	ON_WM_RBUTTONUP()
	ON_WM_MBUTTONUP()
	ON_WM_CONTEXTMENU()
	ON_WM_MBUTTONDOWN()
END_MESSAGE_MAP()

CWaveEditor::CWaveEditor(int sx, int sy, int lx, int ly) 
 : m_iSX(sx), m_iSY(sy), m_iLX(lx), m_iLY(ly)
{
	m_bDrawLine = false;
}

CWaveEditor::~CWaveEditor()
{
}

BOOL CWaveEditor::CreateEx(DWORD dwExStyle, LPCTSTR lpszClassName, LPCTSTR lpszWindowName, DWORD dwStyle, const RECT& rect, CWnd* pParentWnd)
{
	CRect newRect;

	newRect.top = rect.top;
	newRect.left = rect.left;
	newRect.bottom = rect.top + m_iLY * m_iSY + 4;
	newRect.right = rect.left + m_iLX * m_iSX + 4;

	if (CWnd::CreateEx(dwExStyle, lpszClassName, lpszWindowName, dwStyle, newRect, pParentWnd, 0) == -1)
		return -1;

	return 0;
}

void CWaveEditor::PhaseShift(int x)		// // //
{
	const int Length = GetMaxSamples();
	x %= Length;
	if (x < 0) x += Length;
	int *buffer = new int[Length];
	for (int i = 0; i < Length; i++)
		buffer[i] = GetSample(i);
	for (int i = 0; i < Length; i++)
		SetSample(i, buffer[(i + x) % Length]);
	SAFE_RELEASE_ARRAY(buffer);
	Invalidate();
	RedrawWindow();
}

void CWaveEditor::Invert(int x)		// // //
{
	const int Length = GetMaxSamples();
	for (int i = GetMaxSamples() - 1; i >= 0; i--)
		SetSample(i, x - GetSample(i));
	Invalidate();
	RedrawWindow();
}

void CWaveEditor::OnPaint()
{
	const COLORREF GRAY1 = 0xA0A0A0;
	const COLORREF GRAY2 = 0xB0B0B0;
	const COLORREF LINE_COL = 0x404040;

	// Draw the sample
	CPaintDC dc(this);

	// Background
	for (int i = 0; i < m_iLY; ++i) {
		dc.FillSolidRect(0, i * m_iSY, m_iLX * m_iSX, m_iSY, (i & 1) ? GRAY1 : GRAY2);
	}

	if (GetLineMode()) {
		// Lines
		int Steps = m_iLY - 1;
		int Sample = Steps - GetSample(0);

		dc.MoveTo(m_iSX / 2, Sample * m_iSY + m_iSY / 2);

		CPen gray(0, 2, LINE_COL);
		CPen *pOldPen = dc.SelectObject(&gray);

		int icX = m_iSX / 2;
		int icY = m_iSY / 2;

		// Lines
		for (int i = 0; i < m_iLX; ++i) {
			int Sample = Steps - GetSample(i);
			dc.LineTo(i * m_iSX + icX, Sample * m_iSY + icY);
		}

		dc.SelectObject(pOldPen);
	}
	else {
		// Boxes
		for (int i = 0; i < m_iLX; ++i) {
			int Sample = (m_iLY - 1) - GetSample(i);
			DrawRect(&dc, i * m_iSX, Sample * m_iSY, m_iSX, m_iSY);
		}
	}

	CRect rect;
	GetClientRect(rect);
	if ((m_iLX * m_iSX) < rect.Width()) {
		dc.FillSolidRect(m_iLX * m_iSX, 0, rect.Width() - (m_iLX * m_iSX), m_iLY * m_iSY, GRAY1);
	}

	// Draw a line
	if (m_bDrawLine)
		DrawLine(&dc);
}

void CWaveEditor::OnMouseMove(UINT nFlags, CPoint point)
{
	static CPoint last_point; 

	if (nFlags & MK_LBUTTON) {
		// Draw a line
		EditWave(point, last_point);
		last_point = point;
	}
	else if ((nFlags & MK_MBUTTON) && m_bDrawLine) {
		m_ptLineEnd = point;
		EditWave(m_ptLineStart, m_ptLineEnd);
	}
	else
		last_point = point;


	CWnd::OnMouseMove(nFlags, point);
}

void CWaveEditor::OnLButtonDown(UINT nFlags, CPoint point)
{
	SetCapture();
	EditWave(point);

	if (GetLineMode()) {
		Invalidate();
		RedrawWindow();
	}

	CWnd::OnLButtonDown(nFlags, point);
}

void CWaveEditor::OnLButtonUp(UINT nFlags, CPoint point)
{
	ReleaseCapture();
	CWnd::OnLButtonUp(nFlags, point);
}

void CWaveEditor::OnMButtonDown(UINT nFlags, CPoint point)
{
	SetCapture();
	m_ptLineStart = m_ptLineEnd = point;
	m_bDrawLine = true;

	CWnd::OnMButtonDown(nFlags, point);
}

void CWaveEditor::OnMButtonUp(UINT nFlags, CPoint point)
{
	ReleaseCapture();
	m_ptLineStart = m_ptLineEnd = CPoint(0, 0);
	m_bDrawLine = false;
	Invalidate();
	RedrawWindow();

	CWnd::OnMButtonUp(nFlags, point);
}

void CWaveEditor::EditWave(CPoint pt1, CPoint pt2)
{
	int x1 = min(pt2.x, pt1.x);
	int x2 = max(pt2.x, pt1.x);

	float dx = float(pt2.x - pt1.x);
	float dy = float(pt2.y - pt1.y) / dx;

	float y = float( (pt2.x < pt1.x) ? pt2.y : pt1.y);

	if (x1 == x2)
		EditWave(CPoint(x1, int(y)));

	for (int x = x1; x < x2; ++x) {
		EditWave(CPoint(x, int(y)));
		y += dy;
	}

	if (GetLineMode()) {
		Invalidate();
		RedrawWindow();
	}
}

void CWaveEditor::EditWave(CPoint point)
{
	int index = (point.x ) / m_iSX;	
	int sample = (m_iLY - 1) - (point.y / m_iSY);

	if (sample < 0)
		sample = 0;
	if (sample > m_iLY - 1)
		sample = m_iLY - 1;

	if (index < 0)
		index = 0;
	if (index > GetMaxSamples() - 1)
		index = GetMaxSamples() - 1;

	if (!GetLineMode()) {
		CDC *pDC = GetDC();
		if (pDC != NULL) {
			// Erase old sample
			int s = (m_iLY - 1) - GetSample(index);
			pDC->FillSolidRect(index * m_iSX, s * m_iSY, m_iSX, m_iSY, (s & 1) ? 0xA0A0A0 : 0xB0B0B0);
		
			SetSample(index, sample);
		
			// New sample
			s = (m_iLY - 1) - GetSample(index);
			DrawRect(pDC, index * m_iSX, s * m_iSY, m_iSX, m_iSY);

			ReleaseDC(pDC);
		}
	}
	else
		SetSample(index, sample);

	// Indicates wave change
	GetParent()->PostMessage(WM_USER_WAVE_CHANGED);
}

void CWaveEditor::DrawLine(CDC *pDC)
{
	if (m_ptLineStart.x != 0 && m_ptLineStart.y != 0) {
		CPen *pOldPen, Pen;
		Pen.CreatePen(1, 3, 0xFFFFFF);
		pOldPen = pDC->SelectObject(&Pen);
		pDC->MoveTo(m_ptLineStart);
		pDC->LineTo(m_ptLineEnd);
		pDC->SelectObject(pOldPen);
	}
}

void CWaveEditor::WaveChanged()
{
	Invalidate();
	RedrawWindow();
	GetParent()->PostMessage(WM_USER_WAVE_CHANGED);
}

void CWaveEditor::OnContextMenu(CWnd* /*pWnd*/, CPoint point)
{
	CMenu menu;
	
	menu.CreatePopupMenu();
	menu.AppendMenu(MF_STRING, 1, _T("&Steps"));
	menu.AppendMenu(MF_STRING, 2, _T("&Lines"));

	if (GetLineMode())
		menu.CheckMenuItem(1, MF_BYPOSITION | MF_CHECKED);
	else
		menu.CheckMenuItem(0, MF_BYPOSITION | MF_CHECKED);

	switch (menu.TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON | TPM_RETURNCMD, point.x, point.y, this)) {
		case 1: 
			SetLineMode(false);
			break;
		case 2: 
			SetLineMode(true);
			break;
	}

	Invalidate();
	RedrawWindow();
}

// FDS wave

void CWaveEditorFDS::SetInstrument(std::shared_ptr<CInstrumentFDS> pInst)
{
	m_pInstrument = pInst;
	WaveChanged();
}

int CWaveEditorFDS::GetSample(int i) const
{
	ASSERT(m_pInstrument != NULL);
	return m_pInstrument->GetSample(i);
}

void CWaveEditorFDS::SetSample(int i, int s)
{
	ASSERT(m_pInstrument != NULL);
	m_pInstrument->SetSample(i, s);
	theApp.GetSoundGenerator()->WaveChanged();
}

int CWaveEditorFDS::GetMaxSamples() const
{
	return CInstrumentFDS::WAVE_SIZE;
}

void CWaveEditorFDS::DrawRect(CDC *pDC, int x, int y, int sx, int sy) const
{
	pDC->FillSolidRect(x, y, sx, sy, 0x000000);
}

// N163 wave

void CWaveEditorN163::SetInstrument(std::shared_ptr<CInstrumentN163> pInst)
{
	m_pInstrument = pInst;
	WaveChanged();
}

void CWaveEditorN163::SetWave(int i)
{
	m_iWaveIndex = i;
}

int CWaveEditorN163::GetSample(int i) const
{
	ASSERT(m_pInstrument != NULL);
	return m_pInstrument->GetSample(m_iWaveIndex, i);
}

void CWaveEditorN163::SetSample(int i, int s)
{
	ASSERT(m_pInstrument != NULL);
	m_pInstrument->SetSample(m_iWaveIndex, i, s);
	theApp.GetSoundGenerator()->WaveChanged();
}

int CWaveEditorN163::GetMaxSamples() const
{
	return m_pInstrument->GetWaveSize();
}

void CWaveEditorN163::DrawRect(CDC *pDC, int x, int y, int sx, int sy) const
{
	const COLORREF BOX_COLOR = 0x808000;
	const COLORREF BOX_COLOR_HI = 0xB0B030;
	const COLORREF BOX_COLOR_LO = 0x404000;

	pDC->FillSolidRect(x, y, sx, sy, BOX_COLOR);
	pDC->Draw3dRect(x, y, sx, sy, BOX_COLOR_HI, BOX_COLOR_LO);
}

void CWaveEditorN163::SetLength(int Length)
{
	m_iLX = Length;
	m_iSX = 320 / Length;
	WaveChanged();
}
