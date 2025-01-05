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
