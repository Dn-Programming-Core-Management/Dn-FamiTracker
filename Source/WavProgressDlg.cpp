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
#include "FamiTrackerTypes.h"
#include "Settings.h"
#include "APU\Types.h"
#include "SoundGen.h"
#include "WavProgressDlg.h"


// CWavProgressDlg dialog

IMPLEMENT_DYNAMIC(CWavProgressDlg, CDialog)

CWavProgressDlg::CWavProgressDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CWavProgressDlg::IDD, pParent), m_dwStartTime(0), m_iSongEndType(SONG_TIME_LIMIT), m_iSongEndParam(0), m_iTrack(0)
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
	ON_BN_CLICKED(IDC_CANCEL, &CWavProgressDlg::OnCancel)
	ON_WM_TIMER()
END_MESSAGE_MAP()

// CWavProgressDlg message handlers

void CWavProgressDlg::BeginRender(CString &File, render_end_t LengthType, int LengthParam, int Track)
{
	m_iSongEndType = LengthType;
	m_iSongEndParam = LengthParam;
	m_sFile = File;
	m_iTrack = Track;

	if (m_sFile.GetLength() > 0)
		DoModal();
}

BOOL CWavProgressDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	static_cast<CProgressCtrl*>(GetDlgItem(IDC_PROGRESS_BAR))->SetRange(0, 100);
	CView *pView = static_cast<CFrameWnd*>(AfxGetMainWnd())->GetActiveView();		// // //
	CSoundGen *pSoundGen = theApp.GetSoundGenerator();

	m_iTimerPeriod = theApp.GetSettings()->GUI.iLowRefreshRate;

	pView->Invalidate();
	pView->RedrawWindow();

	// Start rendering
	CString FileStr;
	AfxFormatString1(FileStr, IDS_WAVE_PROGRESS_FILE_FORMAT, m_sFile);
	SetDlgItemText(IDC_PROGRESS_FILE, FileStr);

	if (!pSoundGen->RenderToFile(m_sFile.GetBuffer(), m_iSongEndType, m_iSongEndParam, m_iTrack))
		EndDialog(0);

	m_dwStartTime = GetTickCount();
	SetTimer(0, m_iTimerPeriod, NULL);

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

void CWavProgressDlg::OnTimer(UINT_PTR nIDEvent)
{
	// Update progress status
	CString Text;
	DWORD Time = (GetTickCount() - m_dwStartTime) / 1000;

	CProgressCtrl *pProgressBar = static_cast<CProgressCtrl*>(GetDlgItem(IDC_PROGRESS_BAR));
	CSoundGen *pSoundGen = theApp.GetSoundGenerator();

	auto l = pSoundGen->Lock();
	bool Rendering = pSoundGen->IsRendering();

	int Frame, RenderedTime, FramesToRender, RowCount, Row;
	bool Done;
	pSoundGen->GetRenderStat(Frame, RenderedTime, Done, FramesToRender, Row, RowCount);
	l.unlock();

	if (!Rendering)
		Row = RowCount;	// Force 100%

	CString str1, str2;
	int PercentDone;
	switch (m_iSongEndType) {
	case SONG_LOOP_LIMIT:
		if (Frame > FramesToRender)
			Frame = FramesToRender;
		PercentDone = (Row * 100) / RowCount;
		str1.Format(_T("%i / %i"), Row, FramesToRender);		// // //
		str2.Format(_T("%i%%"), PercentDone, Row, RowCount);
		AfxFormatString2(Text, IDS_WAVE_PROGRESS_ROW_FORMAT, str1, str2);		// // //
		break;
	case SONG_TIME_LIMIT:
		int TotalSec = m_iSongEndParam % 60;
		int TotalMin = m_iSongEndParam / 60;
		int CurrSec = RenderedTime % 60;
		int CurrMin = RenderedTime / 60;
		PercentDone = (RenderedTime * 100) / m_iSongEndParam;
		str1.Format(_T("%02i:%02i / %02i:%02i"), CurrMin, CurrSec, TotalMin, TotalSec);
		str2.Format(_T("%i%%"), PercentDone);
		AfxFormatString2(Text, IDS_WAVE_PROGRESS_TIME_FORMAT, str1, str2);
		break;
	}

	SetDlgItemText(IDC_PROGRESS_LBL, Text);

	CString str;
	str.Format(_T("%02i:%02i"), (Time / 60), (Time % 60));
	AfxFormatString1(Text, IDS_WAVE_PROGRESS_ELAPSED_FORMAT, str);
	SetDlgItemText(IDC_TIME, Text);

	pProgressBar->SetPos(PercentDone);

	if (!Rendering) {
		// TODO don't close single-wav export dialog or last dialog in multi-wav export
		EndDialog(0);

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


void CWavProgressDlg::OnCancel()
{
	CSoundGen* pSoundGen = theApp.GetSoundGenerator();

	if (pSoundGen->IsRendering()) {
		//pSoundGen->StopRendering();
		pSoundGen->PostGuiMessage(WM_USER_STOP_RENDER, 0, 0);
	}
	CancelRender = true;

	CDialog::OnCancel();
}
