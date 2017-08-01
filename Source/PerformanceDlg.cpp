/*
** FamiTracker - NES/Famicom sound tracker
** Copyright (C) 2005-2014  Jonathan Liss
**
** 0CC-FamiTracker is (C) 2014-2017 HertzDevil
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

#include "PerformanceDlg.h"
#include "FamiTracker.h"
#include "FamiTrackerTypes.h"
#include "APU/Types.h"
#include "SoundGen.h"

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
END_MESSAGE_MAP()


// CPerformanceDlg message handlers

BOOL CPerformanceDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	theApp.GetCPUUsage();
	theApp.GetSoundGenerator()->GetFrameRate();

	SetTimer(1, 1000, NULL);

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

void CPerformanceDlg::OnTimer(UINT nIDEvent)
{
	CProgressCtrl *pBar = static_cast<CProgressCtrl*>(GetDlgItem(IDC_CPU_BAR));
	unsigned int Usage = theApp.GetCPUUsage();
	unsigned int Rate = theApp.GetSoundGenerator()->GetFrameRate();
	unsigned int Underruns = theApp.GetSoundGenerator()->GetUnderruns();
	CString Text;

	Text.Format(_T("%i%%"), Usage / 100);
	SetDlgItemText(IDC_CPU, Text);

	AfxFormatString1(Text, IDS_PERFORMANCE_FRAMERATE_FORMAT, MakeIntString(Rate));
	SetDlgItemText(IDC_FRAMERATE, Text);

	AfxFormatString1(Text, IDS_PERFORMANCE_UNDERRUN_FORMAT, MakeIntString(Underruns));
	SetDlgItemText(IDC_UNDERRUN, Text);

	pBar->SetRange(0, 100);
	pBar->SetPos(Usage / 100);

	CDialog::OnTimer(nIDEvent);
}

void CPerformanceDlg::OnBnClickedOk()
{
	DestroyWindow();
}

BOOL CPerformanceDlg::DestroyWindow()
{
	KillTimer(1);
	return CDialog::DestroyWindow();
}
