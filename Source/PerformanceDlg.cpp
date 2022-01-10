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

#include "stdafx.h"
#include "FamiTracker.h"
#include "PerformanceDlg.h"
#include "FamiTrackerTypes.h"
#include "Settings.h"
#include "APU/Types.h"
#include "SoundGen.h"

// Timer IDs
enum {
	TMR_BAR,
	TMR_INFO
};

// CPerformanceDlg dialog

IMPLEMENT_DYNAMIC(CPerformanceDlg, CDialog)
CPerformanceDlg::CPerformanceDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CPerformanceDlg::IDD, pParent)
{
}

CPerformanceDlg::~CPerformanceDlg()
{
}

void CPerformanceDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CPerformanceDlg, CDialog)
	ON_WM_TIMER()
	ON_BN_CLICKED(IDOK, OnBnClickedOk)
	ON_WM_CLOSE()
END_MESSAGE_MAP()


// CPerformanceDlg message handlers

BOOL CPerformanceDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	theApp.GetCPUUsage();
	theApp.GetSoundGenerator()->GetFrameRate();

	PerRefreshRate = theApp.GetSettings()->GUI.iLowRefreshRate;
	SetTimer(TMR_BAR, PerRefreshRate, NULL);
	SetTimer(TMR_INFO, 1000, NULL);

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

void CPerformanceDlg::OnTimer(UINT_PTR nIDEvent)
{
	switch (nIDEvent) {
		case TMR_INFO:
			UpdateInfo();
			break;

		case TMR_BAR:
			UpdateBar();
			break;
	}

	CDialog::OnTimer(nIDEvent);
}

void CPerformanceDlg::UpdateBar()
{
	CProgressCtrl* pBar = static_cast<CProgressCtrl*>(GetDlgItem(IDC_CPU_BAR));
	unsigned int Usage = theApp.GetCPUUsage();
	CString Text;

	Text.Format(_T("%i%%"), ((Usage / 100) * (1000 / PerRefreshRate)));
	SetDlgItemText(IDC_CPU, Text);
	pBar->SetRange(0, 100);
	pBar->SetPos((Usage / 100) * (1000 / PerRefreshRate));
}

void CPerformanceDlg::UpdateInfo()
{
	unsigned int Rate = theApp.GetSoundGenerator()->GetFrameRate();
	unsigned int Underruns = theApp.GetSoundGenerator()->GetUnderruns();
	CString Text;

	AfxFormatString1(Text, IDS_PERFORMANCE_FRAMERATE_FORMAT, MakeIntString(Rate));
	SetDlgItemText(IDC_FRAMERATE, Text);

	AfxFormatString1(Text, IDS_PERFORMANCE_UNDERRUN_FORMAT, MakeIntString(Underruns));
	SetDlgItemText(IDC_UNDERRUN, Text);
}

void CPerformanceDlg::OnBnClickedOk()
{
	DestroyWindow();
}

void CPerformanceDlg::OnClose()
{
	DestroyWindow();
}

BOOL CPerformanceDlg::DestroyWindow()
{
	KillTimer(TMR_BAR);
	KillTimer(TMR_INFO);
	return CDialog::DestroyWindow();
}
