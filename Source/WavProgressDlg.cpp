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

#include "WavProgressDlg.h"
#include "FamiTracker.h"
#include "FamiTrackerTypes.h"
#include "APU\Types.h"
#include "SoundGen.h"
#include "WaveRenderer.h"		// // //

// CWavProgressDlg dialog

IMPLEMENT_DYNAMIC(CWavProgressDlg, CDialog)

CWavProgressDlg::CWavProgressDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CWavProgressDlg::IDD, pParent), m_dwStartTime(0)		// // //
{
}

CWavProgressDlg::~CWavProgressDlg()
{
}

void CWavProgressDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CWavProgressDlg, CDialog)
	ON_BN_CLICKED(IDC_CANCEL, &CWavProgressDlg::OnBnClickedCancel)
	ON_WM_TIMER()
END_MESSAGE_MAP()

// CWavProgressDlg message handlers

void CWavProgressDlg::OnBnClickedCancel()
{
	CSoundGen *pSoundGen = theApp.GetSoundGenerator();

	if (pSoundGen->IsRendering()) {
		//pSoundGen->StopRendering();
		pSoundGen->PostThreadMessage(WM_USER_STOP_RENDER, 0, 0);
	}

	EndDialog(0);
}

void CWavProgressDlg::BeginRender(CString &File, std::unique_ptr<CWaveRenderer> pRender)		// // //
{
	m_sFile = File;
	m_pWaveRenderer = std::move(pRender);		// // //

	if (m_sFile.GetLength() > 0)
		DoModal();
}

BOOL CWavProgressDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	static_cast<CProgressCtrl*>(GetDlgItem(IDC_PROGRESS_BAR))->SetRange(0, 100);
	CView *pView = static_cast<CFrameWnd*>(AfxGetMainWnd())->GetActiveView();		// // //
	CSoundGen *pSoundGen = theApp.GetSoundGenerator();

	pView->Invalidate();
	pView->RedrawWindow();

	// Start rendering
	CString FileStr;
	AfxFormatString1(FileStr, IDS_WAVE_PROGRESS_FILE_FORMAT, m_sFile);
	SetDlgItemText(IDC_PROGRESS_FILE, FileStr);

	if (!pSoundGen->RenderToFile(m_sFile.GetBuffer(), m_pWaveRenderer))		// // //
		EndDialog(0);

	m_dwStartTime = GetTickCount();
	SetTimer(0, 200, NULL);

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

void CWavProgressDlg::OnTimer(UINT_PTR nIDEvent)
{
	// Update progress status
	CProgressCtrl *pProgressBar = static_cast<CProgressCtrl*>(GetDlgItem(IDC_PROGRESS_BAR));
	CSoundGen *pSoundGen = theApp.GetSoundGenerator();

	SetDlgItemText(IDC_PROGRESS_LBL, m_pWaveRenderer->GetProgressString().c_str());
	pProgressBar->SetPos(m_pWaveRenderer->GetProgressPercent());		// // //

	CString str, Text;
	const DWORD Time = (GetTickCount() - m_dwStartTime) / 1000;		// // //
	str.Format(_T("%02i:%02i"), (Time / 60), (Time % 60));
	AfxFormatString1(Text, IDS_WAVE_PROGRESS_ELAPSED_FORMAT, str);
	SetDlgItemText(IDC_TIME, Text);

	if (!pSoundGen->IsRendering()) {
		m_pWaveRenderer->CloseOutputFile();		// // //
		SetDlgItemText(IDC_CANCEL, CString(MAKEINTRESOURCE(IDS_WAVE_EXPORT_DONE)));
		CString title;
		GetWindowText(title);
		title.Append(_T(" "));
		title.Append(CString(MAKEINTRESOURCE(IDS_WAVE_EXPORT_FINISHED)));
		SetWindowText(title);
		pProgressBar->SetPos(100);
		KillTimer(0);
	}

	CDialog::OnTimer(nIDEvent);
}
