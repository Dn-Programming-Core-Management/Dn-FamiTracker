/*
** FamiTracker - NES/Famicom sound tracker
** Copyright (C) 2005-2014  Jonathan Liss
**
** 0CC-FamiTracker is (C) 2014-2015 HertzDevil
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
#include "version.h"
#include "../resource.h"
#include "AboutDlg.h"

// CAboutDlg dialog used for App About

LPCTSTR LINK_WEB2 = _T("http://hertzdevil.info/programs/");
LPCTSTR LINK_WEB = _T("http://famitracker.com");						// // !!
LPCTSTR LINK_WEB3 = _T("https://github.com/Gumball2415/Dn-FamiTracker");						// // !!
LPCTSTR LINK_BUG  = _T("https://github.com/Gumball2415/Dn-FamiTracker/issues");		// // !!

// CLinkLabel

BEGIN_MESSAGE_MAP(CLinkLabel, CStatic)
	ON_WM_CTLCOLOR_REFLECT()
	ON_WM_LBUTTONUP()
	ON_WM_MOUSELEAVE()
	ON_WM_MOUSEMOVE()
END_MESSAGE_MAP()

CLinkLabel::CLinkLabel(CString address)
{
	m_strAddress = address;
	m_bHover = false;
}

HBRUSH CLinkLabel::CtlColor(CDC* pDC, UINT /*nCtlColor*/)
{
	pDC->SetTextColor(m_bHover ? 0x0000FF : 0xFF0000);
	pDC->SetBkMode(TRANSPARENT);
	return (HBRUSH)GetStockObject(NULL_BRUSH);
}

void CLinkLabel::OnLButtonUp(UINT nFlags, CPoint point)
{
	ShellExecute(NULL, _T("open"), m_strAddress, NULL, NULL, SW_SHOWNORMAL);
	CStatic::OnLButtonUp(nFlags, point);
}

void CLinkLabel::OnMouseLeave()
{
	m_bHover = false;
	CRect rect, parentRect;
	GetWindowRect(&rect);
	GetParent()->GetWindowRect(parentRect);
	rect.OffsetRect(-parentRect.left - GetSystemMetrics(SM_CXDLGFRAME), -parentRect.top - GetSystemMetrics(SM_CYCAPTION) - GetSystemMetrics(SM_CYDLGFRAME));
	GetParent()->RedrawWindow(rect);
	CStatic::OnMouseLeave();
}

void CLinkLabel::OnMouseMove(UINT nFlags, CPoint point)
{
	if (!m_bHover) {
		m_bHover = true;
		CRect rect, parentRect;
		GetWindowRect(&rect);
		GetParent()->GetWindowRect(parentRect);
		rect.OffsetRect(-parentRect.left - GetSystemMetrics(SM_CXDLGFRAME), -parentRect.top - GetSystemMetrics(SM_CYCAPTION) - GetSystemMetrics(SM_CYDLGFRAME));
		GetParent()->RedrawWindow(rect);

		TRACKMOUSEEVENT t;
		t.cbSize = sizeof(TRACKMOUSEEVENT);
		t.dwFlags = TME_LEAVE;
		t.hwndTrack = m_hWnd;
		TrackMouseEvent(&t);
	}

	CStatic::OnMouseMove(nFlags, point);
}

// CHead

BEGIN_MESSAGE_MAP(CHead, CStatic)
END_MESSAGE_MAP()

CHead::CHead()
{
}

void CHead::DrawItem(LPDRAWITEMSTRUCT lpDraw)
{
	CDC *pDC = CDC::FromHandle(lpDraw->hDC);

	CBitmap bmp;
	bmp.LoadBitmap(IDB_ABOUT);

	CDC dcImage;
	dcImage.CreateCompatibleDC(pDC);
	dcImage.SelectObject(bmp);

	pDC->BitBlt(0, 0, 434, 80, &dcImage, 0, 0, SRCCOPY);

}

// CAboutDlg

BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
END_MESSAGE_MAP()

CAboutDlg::CAboutDlg() : 
	CDialog(CAboutDlg::IDD), 
	m_pWeb(nullptr), 
	m_pBug(nullptr),
	m_pLinkFont(nullptr), 
	m_pBoldFont(nullptr),
	m_pTitleFont(nullptr),
	m_pHead(nullptr)
{
}

CAboutDlg::~CAboutDlg()
{
	SAFE_RELEASE(m_pWeb);
	SAFE_RELEASE(m_pHead);
	SAFE_RELEASE(m_pBug);
	SAFE_RELEASE(m_pLinkFont);
	SAFE_RELEASE(m_pBoldFont);
	SAFE_RELEASE(m_pTitleFont);
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}

BOOL CAboutDlg::OnInitDialog()
{
	// draw the icon manually due to scaling issues
	HICON hIco = (HICON)LoadImage(AfxGetInstanceHandle(), MAKEINTRESOURCE(IDR_MAINFRAME), IMAGE_ICON, 48, 48, LR_DEFAULTCOLOR);
	static_cast<CStatic*>(GetDlgItem(IDC_ICON_STATIC))->SetIcon(hIco);

	CString aboutString = _T(APP_NAME " version " VERSION_STR);

#ifdef WIP
	aboutString += " beta";
#endif

	SetDlgItemText(IDC_ABOUT1, aboutString);
	SetDlgItemText(IDC_ABOUT_CONTRIB,
		_T("- Original software by jsr\r\n")
		_T("- 0CC-FamiTracker by HertzDevil\r\n")
		_T("- Additional improvements by nyanpasu64\r\n")
		_T("- Export plugin support by Gradualore\r\n")
		_T("- Sunsoft 5B information in manual by Blue Mario\r\n")		// // !!
		_T("- Toolbar icons are made by ilkke\r\n")
		_T("- Dn-FT icon design by Compass Man\r\n")		// // !!
		_T("- DPCM import resampler by Jarhmander\r\n")
		_T("- DPCM sample bit order reverser mod by Persune\r\n")		// // !!
		_T("- Module text import/export by rainwarrior"));		// // //
	SetDlgItemText(IDC_ABOUT_LIB, // // !!
		_T("- Blip_buffer 0.4.1 is Copyright (C) blargg (LGPL v2.1)\r\n   (modified by nyanpasu64)")
		_T("- FFT code is (C) 2017 Project Nayuki (MIT)\r\n")
		_T("- emu2413 v1.5.2 (C) Mitsutaka Okazaki (MIT)\r\n")
		_T("- 2A03 sound emulator from NSFPlay\r\n")
		_T("- FDS sound emulator from Mesen (C) Sourmesen (GPL v3)\r\n")
		_T("- JSON for Modern C++ is Copyright (C) Niels Lohmann (MIT)"));

	m_pHead = new CHead();
	m_pHead->SubclassDlgItem(IDC_HEAD, this);
	
	EnableToolTips(TRUE);

	m_wndToolTip.Create(this, TTS_ALWAYSTIP);
	m_wndToolTip.Activate(TRUE);

	m_pWeb = new CLinkLabel(LINK_WEB2);
	m_pWeb->SubclassDlgItem(IDC_WEBPAGE, this);

	LOGFONT LogFont;						// // !!
	CFont* pFont;
	pFont = m_pWeb->GetFont();
	pFont->GetLogFont(&LogFont);
	LogFont.lfUnderline = 1;
	m_pLinkFont = new CFont();
	m_pLinkFont->CreateFontIndirect(&LogFont);

	m_pWeb->SetFont(m_pLinkFont);
	m_wndToolTip.AddTool(m_pWeb, IDS_ABOUT_TOOLTIP_WEB);

	m_pWeb = new CLinkLabel(LINK_WEB);		// // !!
	m_pWeb->SubclassDlgItem(IDC_WEBPAGE2, this);
	m_pWeb->SetFont(m_pLinkFont);
	m_wndToolTip.AddTool(m_pWeb, IDS_ABOUT_TOOLTIP_WEB2);

	m_pWeb = new CLinkLabel(LINK_WEB3);		// // !!
	m_pWeb->SubclassDlgItem(IDC_WEBPAGE3, this);
	m_pWeb->SetFont(m_pLinkFont);
	m_wndToolTip.AddTool(m_pWeb, IDS_ABOUT_TOOLTIP_WEB3);

	m_pBug = new CLinkLabel(LINK_BUG);		// // //
	m_pBug->SubclassDlgItem(IDC_BUG, this);
	m_pBug->SetFont(m_pLinkFont);
	m_wndToolTip.AddTool(m_pBug, IDS_ABOUT_TOOLTIP_BUG);
	
	CStatic *pStatic = static_cast<CStatic*>(GetDlgItem(IDC_ABOUT1));
	CFont *pOldFont = pStatic->GetFont();
	LOGFONT NewLogFont;
	pOldFont->GetLogFont(&NewLogFont);
	NewLogFont.lfWeight = FW_BOLD;
	m_pBoldFont = new CFont();
	m_pTitleFont = new CFont();
	m_pBoldFont->CreateFontIndirect(&NewLogFont);
	NewLogFont.lfHeight = 18;
//	NewLogFont.lfUnderline = TRUE;
	m_pTitleFont->CreateFontIndirect(&NewLogFont);
	static_cast<CStatic*>(GetDlgItem(IDC_ABOUT1))->SetFont(m_pTitleFont);
	static_cast<CStatic*>(GetDlgItem(IDC_ABOUT2))->SetFont(m_pBoldFont);
	static_cast<CStatic*>(GetDlgItem(IDC_ABOUT3))->SetFont(m_pBoldFont);
	
	return TRUE;
}

BOOL CAboutDlg::PreTranslateMessage(MSG* pMsg)
{
	m_wndToolTip.RelayEvent(pMsg);
	return CDialog::PreTranslateMessage(pMsg);
}
