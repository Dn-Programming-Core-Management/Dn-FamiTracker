/*
** FamiTracker - NES/Famicom sound tracker
** Copyright (C) 2005-2020 Jonathan Liss
**
** 0CC-FamiTracker is (C) 2014-2018 HertzDevil
**
** Dn-FamiTracker is (C) 2020-2022 D.P.C.M.
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

#include "stdafx.h"
#include "../resource.h"
#include "FamiTracker.h"
#include "DialogReBar.h"
#include "CustomControls.h"		// // //

// COctaveDlgBar dialog

IMPLEMENT_DYNAMIC(CDialogReBar, CDialogBar)
CDialogReBar::CDialogReBar(CWnd* pParent /*=NULL*/)
	: CDialogBar(/*COctaveDlgBar::IDD, pParent*/)
{
}

CDialogReBar::~CDialogReBar()
{
}

BEGIN_MESSAGE_MAP(CDialogReBar, CDialogBar)
	ON_WM_ERASEBKGND()
	ON_WM_MOVE()
	ON_WM_CTLCOLOR()
END_MESSAGE_MAP()


// COctaveDlgBar message handlers

BOOL CDialogReBar::OnEraseBkgnd(CDC* pDC)
{
	if (!theApp.IsThemeActive()) {
		CDialogBar::OnEraseBkgnd(pDC);
		return TRUE;
	}

	CWnd* pParent = GetParent();
	ASSERT_VALID(pParent);
	CPoint pt(0, 0);
	MapWindowPoints(pParent, &pt, 1);
	pt = pDC->OffsetWindowOrg(pt.x, pt.y);
	LRESULT lResult = pParent->SendMessage(WM_ERASEBKGND, (WPARAM)pDC->m_hDC, 0L);
	pDC->SetWindowOrg(pt.x, pt.y);
	return (BOOL)lResult;
}

void CDialogReBar::OnMove(int x, int y)
{
	Invalidate();
}

HBRUSH CDialogReBar::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor)
{
	HBRUSH hbr = CDialogBar::OnCtlColor(pDC, pWnd, nCtlColor);

	if (nCtlColor == CTLCOLOR_STATIC && theApp.IsThemeActive() && !dynamic_cast<CLockedEdit*>(pWnd)) {
		pDC->SetBkMode(TRANSPARENT);
		return (HBRUSH)GetStockObject(NULL_BRUSH);
	}

	return hbr;
}
