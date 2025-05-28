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
#include "FamiTrackerDoc.h"
#include "FamiTrackerView.h"
#include "MainFrm.h"
#include "SoundGen.h"
#include "TrackerChannel.h"
#include "WavProgressDlg.h"
#include "CreateWaveDlg.h"
#include "str_conv/str_conv.hpp"

//#include <string>
#include <filesystem>

const int MAX_LOOP_TIMES = 99;
const int MAX_PLAY_TIME	 = (99 * 60) + 0;

// CCreateWaveDlg dialog

IMPLEMENT_DYNAMIC(CCreateWaveDlg, CDialog)

CCreateWaveDlg::CCreateWaveDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CCreateWaveDlg::IDD, pParent)
{
}

CCreateWaveDlg::~CCreateWaveDlg()
{
}

void CCreateWaveDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CCreateWaveDlg, CDialog)
	ON_BN_CLICKED(IDC_BEGIN, &CCreateWaveDlg::OnBnClickedBegin)
	ON_NOTIFY(UDN_DELTAPOS, IDC_SPIN_LOOP, &CCreateWaveDlg::OnDeltaposSpinLoop)
	ON_NOTIFY(UDN_DELTAPOS, IDC_SPIN_TIME, &CCreateWaveDlg::OnDeltaposSpinTime)
	ON_EN_CHANGE(IDC_TIMES, &CCreateWaveDlg::OnEnChangeLoop)
	ON_EN_CHANGE(IDC_SECONDS, &CCreateWaveDlg::OnEnChangeSeconds)
END_MESSAGE_MAP()

int CCreateWaveDlg::GetFrameLoopCount() const
{
	int Frames = GetDlgItemInt(IDC_TIMES);

	if (Frames < 1)
		Frames = 1;
	if (Frames > MAX_LOOP_TIMES)
		Frames = MAX_LOOP_TIMES;

	return Frames;
}

int CCreateWaveDlg::GetTimeLimit() const
{
	int Minutes, Seconds;
	TCHAR str[256];

	GetDlgItemText(IDC_SECONDS, str, 256);
	_stscanf(str, _T("%u:%u"), &Minutes, &Seconds);
	int Time = (Minutes * 60) + (Seconds % 60);

	if (Time < 1)
		Time = 1;
	if (Time > MAX_PLAY_TIME)
		Time = MAX_PLAY_TIME;

	return Time;
}

// CCreateWaveDlg message handlers

void CCreateWaveDlg::OnBnClickedBegin()
{
	namespace fs = std::filesystem;
	using namespace std::string_literals;

	render_end_t EndType = SONG_TIME_LIMIT;
	int EndParam = 0;

	CFamiTrackerDoc *pDoc = CFamiTrackerDoc::GetDoc();
	CFamiTrackerView *pView = CFamiTrackerView::GetView();

	CString FileName = pDoc->GetFileTitle();

	int Track = m_ctlTracks.GetCurSel();

	if (pDoc->GetTrackCount() > 1) {
		FileName.AppendFormat(_T(" - Track %02i (%s)"), Track + 1, pDoc->GetTrackTitle(Track).GetBuffer());
	}

	CString fileFilter = LoadDefaultFilter(IDS_FILTER_WAV, _T(".wav"));	
	CFileDialog SaveDialog(FALSE, _T("wav"), FileName, OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, fileFilter);

	// Close this dialog
	EndDialog(0);

	// Ask for file location
	if (SaveDialog.DoModal() == IDCANCEL)
		return;
	
	// Save
	if (IsDlgButtonChecked(IDC_RADIO_LOOP)) {
		EndType = SONG_LOOP_LIMIT;
		EndParam = GetFrameLoopCount();
	}
	else if (IsDlgButtonChecked(IDC_RADIO_TIME)) {
		EndType = SONG_TIME_LIMIT;
		EndParam = GetTimeLimit();
	}

	pView->UnmuteAllChannels();

	// Mute selected channels

	CString outPathC = SaveDialog.GetPathName();
	fs::path outPath = conv::to_utf8(outPathC);


	auto nchan = m_ctlChannelList.GetCount();
	{
		// Mute selected channels
		pView->UnmuteAllChannels();
		for (int i = 0; i < nchan; ++i) {
			if (m_ctlChannelList.GetCheck(i) == BST_UNCHECKED)
				pView->ToggleChannel(i);
		}

		CWavProgressDlg ProgressDlg;
		// Show the render progress dialog, this will also start rendering
		ProgressDlg.BeginRender(outPathC, EndType, EndParam, Track);
		if (ProgressDlg.CancelRender)
			goto end;
	}

	if (IsDlgButtonChecked(IDC_SEPERATE_CHANNEL_EXPORT)) {
		for (int i = 0; i < nchan; ++i) {
			if (m_ctlChannelList.GetCheck(i) == BST_CHECKED) {
				pView->MuteAllChannels();
				pView->ToggleChannel(i);

				// Write wav file to same name as above, but with a suffix before the extension.
				CString chanNameC; m_ctlChannelList.GetText(i, chanNameC);

				CString textC;
				textC.Format(_T("%02i - "), i + 1);
				textC.Append(chanNameC);

				std::string text = conv::to_utf8(textC);

				fs::path chanOutPath = outPath;
				chanOutPath.replace_filename("");
				chanOutPath += text + ".wav"s;

				CString chanOutPathC = conv::to_t(chanOutPath.string()).c_str();

				CWavProgressDlg ProgressDlg;
				// Show the render progress dialog, this will also start rendering
				ProgressDlg.BeginRender(chanOutPathC, EndType, EndParam, Track);

				// if cancelled early, abort further rendering.
				if (ProgressDlg.CancelRender)
					break;
			}
		}
	}

	end:

	// Unmute all channels
	pView->UnmuteAllChannels();
}

BOOL CCreateWaveDlg::OnInitDialog()
{
	CheckDlgButton(IDC_RADIO_LOOP, BST_CHECKED);
	CheckDlgButton(IDC_RADIO_TIME, BST_UNCHECKED);

	m_isSetup = true;
	SetDlgItemText(IDC_TIMES, _T("1"));
	SetDlgItemText(IDC_SECONDS, _T("01:00"));
	m_isSetup = false;

	m_ctlChannelList.SubclassDlgItem(IDC_CHANNELS, this);

	m_ctlChannelList.ResetContent();
	m_ctlChannelList.SetCheckStyle(BS_AUTOCHECKBOX);

	m_ctlTracks.SubclassDlgItem(IDC_TRACKS, this);

	CFamiTrackerDoc *pDoc = CFamiTrackerDoc::GetDoc();

	int ChannelCount = pDoc->GetAvailableChannels();
	for (int i = 0; i < ChannelCount; ++i) {
		m_ctlChannelList.AddString(pDoc->GetChannel(i)->GetChannelName());
		m_ctlChannelList.SetCheck(i, 1);
	}

	for (unsigned int i = 0; i < pDoc->GetTrackCount(); ++i) {
		CString text;
		text.Format(_T("#%02i - "), i + 1);
		text.Append(pDoc->GetTrackTitle(i));
		m_ctlTracks.AddString(text);
	}

	CMainFrame *pMainFrm = static_cast<CMainFrame*>(theApp.GetMainWnd());
	m_ctlTracks.SetCurSel(pMainFrm->GetSelectedTrack());

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

void CCreateWaveDlg::ShowDialog()
{
	CDialog::DoModal();
}

void CCreateWaveDlg::OnDeltaposSpinLoop(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMUPDOWN pNMUpDown = reinterpret_cast<LPNMUPDOWN>(pNMHDR);
	int Times = GetFrameLoopCount() - pNMUpDown->iDelta;

	if (Times < 1)
		Times = 1;
	if (Times > MAX_LOOP_TIMES)
		Times = MAX_LOOP_TIMES;

	SetDlgItemInt(IDC_TIMES, Times);
	CheckDlgButton(IDC_RADIO_LOOP, BST_CHECKED);
	CheckDlgButton(IDC_RADIO_TIME, BST_UNCHECKED);
	*pResult = 0;
}

void CCreateWaveDlg::OnDeltaposSpinTime(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMUPDOWN pNMUpDown = reinterpret_cast<LPNMUPDOWN>(pNMHDR);
	int Minutes, Seconds;
	int Time = GetTimeLimit() - pNMUpDown->iDelta;
	CString str;

	if (Time < 1)
		Time = 1;
	if (Time > MAX_PLAY_TIME)
		Time = MAX_PLAY_TIME;

	Seconds = Time % 60;
	Minutes = Time / 60;

	str.Format(_T("%02i:%02i"), Minutes, Seconds);
	SetDlgItemText(IDC_SECONDS, str);
	CheckDlgButton(IDC_RADIO_LOOP, BST_UNCHECKED);
	CheckDlgButton(IDC_RADIO_TIME, BST_CHECKED);
	*pResult = 0;
}

void CCreateWaveDlg::OnEnChangeLoop()
{
	if (m_isSetup) return;
	CheckDlgButton(IDC_RADIO_LOOP, BST_CHECKED);
	CheckDlgButton(IDC_RADIO_TIME, BST_UNCHECKED);
}

void CCreateWaveDlg::OnEnChangeSeconds()
{
	if (m_isSetup) return;
	CheckDlgButton(IDC_RADIO_LOOP, BST_UNCHECKED);
	CheckDlgButton(IDC_RADIO_TIME, BST_CHECKED);
}
