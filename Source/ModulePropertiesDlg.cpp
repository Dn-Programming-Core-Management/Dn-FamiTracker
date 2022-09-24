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
#include <cstdlib>
#include <cmath>

#include "FamiTracker.h"
#include "FamiTrackerDoc.h"
#include "MainFrm.h"
#include "ModulePropertiesDlg.h"
#include "ModuleImportDlg.h"
#include "SoundGen.h"

LPCTSTR TRACK_FORMAT = _T("#%02i %s");

// CModulePropertiesDlg dialog

//
// Contains song list editor and expansion chip selector
//

IMPLEMENT_DYNAMIC(CModulePropertiesDlg, CDialog)
CModulePropertiesDlg::CModulePropertiesDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CModulePropertiesDlg::IDD, pParent), m_iSelectedSong(0), m_pDocument(NULL)
{
}

CModulePropertiesDlg::~CModulePropertiesDlg()
{
}

//void CModulePropertiesDlg::DoDataExchange(CDataExchange* pDX)
//{
//	CDialog::DoDataExchange(pDX);
//}

// cargo cult much?



BEGIN_MESSAGE_MAP(CModulePropertiesDlg, CDialog)
	ON_BN_CLICKED(IDOK, OnBnClickedOk)
	ON_BN_CLICKED(IDC_SONG_ADD, OnBnClickedSongAdd)
	ON_BN_CLICKED(IDC_SONG_INSERT, OnBnClickedSongInsert)		// // //
	ON_BN_CLICKED(IDC_SONG_REMOVE, OnBnClickedSongRemove)
	ON_BN_CLICKED(IDC_SONG_UP, OnBnClickedSongUp)
	ON_BN_CLICKED(IDC_SONG_DOWN, OnBnClickedSongDown)
	ON_EN_CHANGE(IDC_SONGNAME, OnEnChangeSongname)
	ON_BN_CLICKED(IDC_SONG_IMPORT, OnBnClickedSongImport)

	ON_WM_HSCROLL()
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_SONGLIST, OnLvnItemchangedSonglist)
	ON_BN_CLICKED(IDC_EXPANSION_VRC6, OnBnClickedExpansionVRC6)
	ON_BN_CLICKED(IDC_EXPANSION_VRC7, OnBnClickedExpansionVRC7)
	ON_BN_CLICKED(IDC_EXPANSION_FDS, OnBnClickedExpansionFDS)
	ON_BN_CLICKED(IDC_EXPANSION_MMC5, OnBnClickedExpansionMMC5)
	ON_BN_CLICKED(IDC_EXPANSION_S5B, OnBnClickedExpansionS5B)
	ON_BN_CLICKED(IDC_EXPANSION_N163, OnBnClickedExpansionN163)
	ON_WM_VSCROLL()
	ON_EN_CHANGE(IDC_APU1_OFFSET_EDIT, &CModulePropertiesDlg::OnEnChangeApu1OffsetEdit)
	ON_EN_CHANGE(IDC_APU2_OFFSET_EDIT, &CModulePropertiesDlg::OnEnChangeApu2OffsetEdit)
	ON_EN_CHANGE(IDC_VRC6_OFFSET_EDIT, &CModulePropertiesDlg::OnEnChangeVrc6OffsetEdit)
	ON_EN_CHANGE(IDC_VRC7_OFFSET_EDIT, &CModulePropertiesDlg::OnEnChangeVrc7OffsetEdit)
	ON_EN_CHANGE(IDC_FDS_OFFSET_EDIT, &CModulePropertiesDlg::OnEnChangeFdsOffsetEdit)
	ON_EN_CHANGE(IDC_MMC5_OFFSET_EDIT, &CModulePropertiesDlg::OnEnChangeMmc5OffsetEdit)
	ON_EN_CHANGE(IDC_N163_OFFSET_EDIT, &CModulePropertiesDlg::OnEnChangeN163OffsetEdit)
	ON_EN_CHANGE(IDC_S5B_OFFSET_EDIT, &CModulePropertiesDlg::OnEnChangeS5bOffsetEdit)
END_MESSAGE_MAP()


const int LEVEL_RANGE = 12;		// +/- 12 dB range

const int FINE_DELTA = 10;		// you better hope this matches CConfigMixer::LEVEL_SCALE and SoundGen.cpp.
const int COARSE_DELTA = 1;		// slider notches per dB.
const int PAGEUP = COARSE_DELTA * 2;	// 2 dB per pageup.

const int MAX_FINE = LEVEL_RANGE * FINE_DELTA;
	// user input is clamped to +-MAX_FINE.

// CModulePropertiesDlg message handlers
BOOL CModulePropertiesDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	APU1LevelEdit.SubclassDlgItem(IDC_APU1_OFFSET_EDIT, this);
	APU2LevelEdit.SubclassDlgItem(IDC_APU2_OFFSET_EDIT, this);
	VRC6LevelEdit.SubclassDlgItem(IDC_VRC6_OFFSET_EDIT, this);
	VRC7LevelEdit.SubclassDlgItem(IDC_VRC7_OFFSET_EDIT, this);
	FDSLevelEdit.SubclassDlgItem(IDC_FDS_OFFSET_EDIT, this);
	MMC5LevelEdit.SubclassDlgItem(IDC_MMC5_OFFSET_EDIT, this);
	N163LevelEdit.SubclassDlgItem(IDC_N163_OFFSET_EDIT, this);
	S5BLevelEdit.SubclassDlgItem(IDC_S5B_OFFSET_EDIT, this);

	// Get active document
	CFrameWnd *pFrameWnd = static_cast<CFrameWnd*>(GetParent());
	m_pDocument = static_cast<CFamiTrackerDoc*>(pFrameWnd->GetActiveDocument());

	CListCtrl *pSongList = static_cast<CListCtrl*>(GetDlgItem(IDC_SONGLIST));
	pSongList->InsertColumn(0, _T("Songs"), 0, 150);
	pSongList->SetExtendedStyle(LVS_EX_FULLROWSELECT);

	FillSongList();
	SelectSong(0);		// // //

	// Expansion chips
	m_iExpansions = m_pDocument->GetExpansionChip();
	((CButton*)GetDlgItem(IDC_EXPANSION_VRC6))->SetCheck((m_iExpansions & SNDCHIP_VRC6) != 0);
	((CButton*)GetDlgItem(IDC_EXPANSION_VRC7))->SetCheck((m_iExpansions & SNDCHIP_VRC7) != 0);
	((CButton*)GetDlgItem(IDC_EXPANSION_FDS))->SetCheck((m_iExpansions & SNDCHIP_FDS) != 0);
	((CButton*)GetDlgItem(IDC_EXPANSION_MMC5))->SetCheck((m_iExpansions & SNDCHIP_MMC5) != 0);
	((CButton*)GetDlgItem(IDC_EXPANSION_N163))->SetCheck((m_iExpansions & SNDCHIP_N163) != 0);
	((CButton*)GetDlgItem(IDC_EXPANSION_S5B))->SetCheck((m_iExpansions & SNDCHIP_S5B) != 0);

	// Namco channel count
	CSliderCtrl *pChanSlider = static_cast<CSliderCtrl*>(GetDlgItem(IDC_CHANNELS));
	pChanSlider->SetRange(1, 8);
	m_iN163Channels = m_pDocument->GetNamcoChannels();
	updateN163ChannelCount();

	// Vibrato 
	CComboBox *pVibratoBox = static_cast<CComboBox*>(GetDlgItem(IDC_VIBRATO));
	pVibratoBox->SetCurSel((m_pDocument->GetVibratoStyle() == VIBRATO_NEW) ? 0 : 1);
	
	// Pitch
	CComboBox *pPitchBox = static_cast<CComboBox*>(GetDlgItem(IDC_COMBO_LINEARPITCH));		// // //
	pPitchBox->SetCurSel(m_pDocument->GetLinearPitch() ? 1 : 0);
	
	// Level Offsets
	APU1LevelOffset = -m_pDocument->GetLevelOffset(0);
	APU2LevelOffset = -m_pDocument->GetLevelOffset(1);
	VRC6LevelOffset = -m_pDocument->GetLevelOffset(2);
	VRC7LevelOffset = -m_pDocument->GetLevelOffset(3);
	FDSLevelOffset = -m_pDocument->GetLevelOffset(4);
	MMC5LevelOffset = -m_pDocument->GetLevelOffset(5);
	N163LevelOffset = -m_pDocument->GetLevelOffset(6);
	S5BLevelOffset = -m_pDocument->GetLevelOffset(7);

	// Level sliders
	SetupSlider(IDC_APU1_OFFSET_SLIDER);
	SetupSlider(IDC_APU2_OFFSET_SLIDER);
	SetupSlider(IDC_VRC6_OFFSET_SLIDER);
	SetupSlider(IDC_VRC7_OFFSET_SLIDER);
	SetupSlider(IDC_FDS_OFFSET_SLIDER);
	SetupSlider(IDC_MMC5_OFFSET_SLIDER);
	SetupSlider(IDC_N163_OFFSET_SLIDER);
	SetupSlider(IDC_S5B_OFFSET_SLIDER);

	for (int i = 0; i < 8; i++)
		updateLevelGUI(i);
	return TRUE;  // return TRUE unless you set the focus to a control
				  // EXCEPTION: OCX Property Pages should return FALSE
}

void CModulePropertiesDlg::OnBnClickedOk()
{
	CMainFrame *pMainFrame = static_cast<CMainFrame*>(GetParentFrame());

	if (!(m_iExpansions & SNDCHIP_N163)) m_iN163Channels = 0;
	if (m_pDocument->GetNamcoChannels() != m_iN163Channels || m_pDocument->GetExpansionChip() != m_iExpansions)		// // //
	{
		CString str;
		unsigned int Gone = m_pDocument->GetExpansionChip() & ~m_iExpansions;
		for (int i = 0; i < 6; i++) {
			if (Gone & (1 << i)) switch (i) {
			case 0: str += _T("VRC6 "); break;
			case 1: str += _T("VRC7 "); break;
			case 2: str += _T("FDS ");  break;
			case 3: str += _T("MMC5 "); break;
			case 4: str += _T("N163 "); break;
			case 5: str += _T("5B ");   break;
			}
			if (i == 4 && m_pDocument->ExpansionEnabled(SNDCHIP_N163)
				&& (m_iExpansions & SNDCHIP_N163) && m_iN163Channels < m_pDocument->GetNamcoChannels()) {
				str += _T("N163 ");
				Gone |= 0x100;
			}
		}
		str = "You are going to remove channels from the following expansion chips:\n" + str;
		str += "\nDo you want to proceed? There is no undo for this command.";
		if (Gone)
			if (AfxMessageBox(str, MB_YESNO | MB_ICONWARNING | MB_DEFBUTTON2) == IDNO)
				return;
		m_pDocument->SetNamcoChannels(m_iN163Channels, true);
		m_pDocument->SelectExpansionChip(m_iExpansions, true);
		m_pDocument->UpdateAllViews(NULL, UPDATE_PROPERTIES);
	}

	// Vibrato 
	CComboBox *pVibratoBox = static_cast<CComboBox*>(GetDlgItem(IDC_VIBRATO));
	m_pDocument->SetVibratoStyle((pVibratoBox->GetCurSel() == 0) ? VIBRATO_NEW : VIBRATO_OLD);

	// Linear pitch
	CComboBox *pPitchBox = static_cast<CComboBox*>(GetDlgItem(IDC_COMBO_LINEARPITCH));		// // //
	m_pDocument->SetLinearPitch(pPitchBox->GetCurSel() == 1);

	// Volume 
	if (m_pDocument->GetLevelOffset(0) != APU1LevelOffset) m_pDocument->SetLevelOffset(0, -APU1LevelOffset);
	if (m_pDocument->GetLevelOffset(1) != APU2LevelOffset) m_pDocument->SetLevelOffset(1, -APU2LevelOffset);
	if (m_pDocument->GetLevelOffset(2) != VRC6LevelOffset) m_pDocument->SetLevelOffset(2, -VRC6LevelOffset);
	if (m_pDocument->GetLevelOffset(3) != VRC7LevelOffset) m_pDocument->SetLevelOffset(3, -VRC7LevelOffset);
	if (m_pDocument->GetLevelOffset(4) != FDSLevelOffset) m_pDocument->SetLevelOffset(4, -FDSLevelOffset);
	if (m_pDocument->GetLevelOffset(5) != MMC5LevelOffset) m_pDocument->SetLevelOffset(5, -MMC5LevelOffset);
	if (m_pDocument->GetLevelOffset(6) != N163LevelOffset) m_pDocument->SetLevelOffset(6, -N163LevelOffset);
	if (m_pDocument->GetLevelOffset(7) != S5BLevelOffset) m_pDocument->SetLevelOffset(7, -S5BLevelOffset);


	if (pMainFrame->GetSelectedTrack() != m_iSelectedSong)
		pMainFrame->SelectTrack(m_iSelectedSong);

	pMainFrame->UpdateControls();

	theApp.GetSoundGenerator()->DocumentPropertiesChanged(m_pDocument);

	theApp.RefreshFrameEditor();

	OnOK();
}


// **** events ****
void CModulePropertiesDlg::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
	CSliderCtrl* pSlider = reinterpret_cast<CSliderCtrl*>(pScrollBar);
	int pos = pSlider->GetPos();
	if (pSlider->GetDlgCtrlID() == IDC_CHANNELS) {
		setN163NChannels(pos);
	}

	CDialog::OnHScroll(nSBCode, nPos, pScrollBar);
}

void CModulePropertiesDlg::OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
	CSliderCtrl* pSlider = reinterpret_cast<CSliderCtrl*>(pScrollBar);
	int pos = pSlider->GetPos();
	switch (pSlider->GetDlgCtrlID()) {
	case IDC_APU1_OFFSET_SLIDER:
		OffsetSlider(0, pos); break;
	case IDC_APU2_OFFSET_SLIDER:
		OffsetSlider(1, pos); break;
	case IDC_VRC6_OFFSET_SLIDER:
		OffsetSlider(2, pos); break;
	case IDC_VRC7_OFFSET_SLIDER:
		OffsetSlider(3, pos); break;
	case IDC_FDS_OFFSET_SLIDER:
		OffsetSlider(4, pos); break;
	case IDC_MMC5_OFFSET_SLIDER:
		OffsetSlider(5, pos); break;
	case IDC_N163_OFFSET_SLIDER:
		OffsetSlider(6, pos); break;
	case IDC_S5B_OFFSET_SLIDER:
		OffsetSlider(7, pos); break;
	}

	UpdateData(TRUE);
	CDialog::OnVScroll(nSBCode, nPos, pScrollBar);
}


void CModulePropertiesDlg::SetupSlider(int nID) const
{
	CSliderCtrl* pSlider = static_cast<CSliderCtrl*>(GetDlgItem(nID));
	pSlider->SetRange(-LEVEL_RANGE * COARSE_DELTA, LEVEL_RANGE * COARSE_DELTA);
	pSlider->SetTicFreq(COARSE_DELTA * 2);
	pSlider->SetPageSize(PAGEUP);
}

// **** song selection ****

void CModulePropertiesDlg::OnBnClickedSongAdd()
{
	CString TrackTitle;

	// Try to add a track
	int NewTrack = m_pDocument->AddTrack();

	if (NewTrack == -1)
		return;
	
	m_pDocument->UpdateAllViews(NULL, UPDATE_TRACK);

	TrackTitle.Format(TRACK_FORMAT, NewTrack + 1, m_pDocument->GetTrackTitle(NewTrack));
	static_cast<CListCtrl*>(GetDlgItem(IDC_SONGLIST))->InsertItem(NewTrack, TrackTitle);

	SelectSong(NewTrack);
}

void CModulePropertiesDlg::OnBnClickedSongInsert()		// // //
{
	CString TrackTitle;

	// Try to add a track
	unsigned int NewTrack = m_pDocument->AddTrack();

	if (NewTrack == -1)
		return;
	
	while (NewTrack > m_iSelectedSong + 1)
		m_pDocument->MoveTrackUp(NewTrack--);
	m_pDocument->UpdateAllViews(NULL, UPDATE_TRACK);

	TrackTitle.Format(TRACK_FORMAT, NewTrack, m_pDocument->GetTrackTitle(NewTrack));
	auto pSongList = static_cast<CListCtrl*>(GetDlgItem(IDC_SONGLIST));
	pSongList->InsertItem(NewTrack, TrackTitle);

	for (unsigned i = 0; i < m_pDocument->GetTrackCount(); ++i) {
		TrackTitle.Format(_T("#%02i %s"), i + 1, m_pDocument->GetTrackTitle(i));
		pSongList->SetItemText(i, 0, TrackTitle);
	}

	SelectSong(NewTrack);
}

void CModulePropertiesDlg::OnBnClickedSongRemove()
{
	ASSERT(m_iSelectedSong != -1);

	CListCtrl *pSongList = static_cast<CListCtrl*>(GetDlgItem(IDC_SONGLIST));
	CMainFrame *pMainFrame = static_cast<CMainFrame*>(GetParentFrame());
	unsigned Count = m_pDocument->GetTrackCount();
	CString TrackTitle;

	int SelCount = pSongList->GetSelectedCount();		// // //
	if (Count <= static_cast<unsigned>(SelCount))
		return; // Single track

	// Display warning first
	if (AfxMessageBox(IDS_SONG_DELETE, MB_YESNO | MB_ICONWARNING | MB_DEFBUTTON2) == IDNO)
		return;

	for (unsigned i = Count - 1; i < Count; --i)		// // //
		if (pSongList->GetItemState(i, LVIS_SELECTED) == LVIS_SELECTED) {
			if (m_iSelectedSong > i)
				--m_iSelectedSong;
			pSongList->DeleteItem(i);
			m_pDocument->RemoveTrack(i);
		}
	m_pDocument->UpdateAllViews(NULL, UPDATE_TRACK);

	Count = m_pDocument->GetTrackCount();	// Get new track count

	// Redraw track list
	for (unsigned int i = 0; i < Count; ++i) {
		TrackTitle.Format(_T("#%02i %s"), i + 1, m_pDocument->GetTrackTitle(i));
		pSongList->SetItemText(i, 0, TrackTitle);
	}

	if (m_iSelectedSong >= Count)
		m_iSelectedSong = Count - 1;
	SelectSong(m_iSelectedSong);
}

void CModulePropertiesDlg::OnBnClickedSongUp()
{
	CListCtrl *pSongList = static_cast<CListCtrl*>(GetDlgItem(IDC_SONGLIST));
	int Song = m_iSelectedSong;
	CString Text;

	if (Song == 0)
		return;

	m_pDocument->MoveTrackUp(Song);
	m_pDocument->UpdateAllViews(NULL, UPDATE_TRACK);

	Text.Format(TRACK_FORMAT, Song + 1, m_pDocument->GetTrackTitle(Song));
	pSongList->SetItemText(Song, 0, Text);
	Text.Format(TRACK_FORMAT, Song, m_pDocument->GetTrackTitle(Song - 1));
	pSongList->SetItemText(Song - 1, 0, Text);

	SelectSong(Song - 1);
}

void CModulePropertiesDlg::OnBnClickedSongDown()
{
	CListCtrl *pSongList = static_cast<CListCtrl*>(GetDlgItem(IDC_SONGLIST));
	int Song = m_iSelectedSong;
	CString Text;

	if (Song == (m_pDocument->GetTrackCount() - 1))
		return;

	m_pDocument->MoveTrackDown(Song);
	m_pDocument->UpdateAllViews(NULL, UPDATE_TRACK);

	Text.Format(TRACK_FORMAT, Song + 1, m_pDocument->GetTrackTitle(Song));
	pSongList->SetItemText(Song, 0, Text);
	Text.Format(TRACK_FORMAT, Song + 2, m_pDocument->GetTrackTitle(Song + 1));
	pSongList->SetItemText(Song + 1, 0, Text);

	SelectSong(Song + 1);
}

void CModulePropertiesDlg::OnEnChangeSongname()
{
	CListCtrl *pSongList = static_cast<CListCtrl*>(GetDlgItem(IDC_SONGLIST));
	CEdit *pName = static_cast<CEdit*>(GetDlgItem(IDC_SONGNAME));
	CString Text, Title;

	if (m_iSelectedSong == -1 || !m_bSingleSelection)
		return;

	pName->GetWindowText(Text);

	Title.Format(TRACK_FORMAT, m_iSelectedSong + 1, Text);

	pSongList->SetItemText(m_iSelectedSong, 0, Title);
	m_pDocument->SetTrackTitle(m_iSelectedSong, Text);
	m_pDocument->UpdateAllViews(NULL, UPDATE_TRACK);
}

void CModulePropertiesDlg::SelectSong(int Song)
{
	CListCtrl *pSongList = static_cast<CListCtrl*>(GetDlgItem(IDC_SONGLIST));
	ASSERT(Song >= 0);

	for (int i = pSongList->GetItemCount() - 1; i >= 0; --i)		// // //
		pSongList->SetItemState(i, i == Song ? (LVIS_SELECTED | LVIS_FOCUSED) : 0, LVIS_SELECTED | LVIS_FOCUSED);
	m_iSelectedSong = Song;
	m_bSingleSelection = true;
	UpdateSongButtons();
	pSongList->EnsureVisible(Song, FALSE);
	pSongList->SetFocus();
}

void CModulePropertiesDlg::UpdateSongButtons()
{
	unsigned TrackCount = m_pDocument->GetTrackCount();
	bool Empty = static_cast<CListCtrl*>(GetDlgItem(IDC_SONGLIST))->GetSelectedCount() == 0;

	GetDlgItem(IDC_SONG_ADD)->EnableWindow((TrackCount == MAX_TRACKS) ? FALSE : TRUE);
	GetDlgItem(IDC_SONG_INSERT)->EnableWindow((TrackCount == MAX_TRACKS || !m_bSingleSelection || Empty) ? FALSE : TRUE);
	GetDlgItem(IDC_SONG_REMOVE)->EnableWindow((TrackCount == 1 || Empty) ? FALSE : TRUE);
	GetDlgItem(IDC_SONG_UP)->EnableWindow((m_iSelectedSong == 0 || !m_bSingleSelection || Empty) ? FALSE : TRUE);
	GetDlgItem(IDC_SONG_DOWN)->EnableWindow((m_iSelectedSong == TrackCount - 1 || !m_bSingleSelection || Empty) ? FALSE : TRUE);
	GetDlgItem(IDC_SONG_IMPORT)->EnableWindow((TrackCount == MAX_TRACKS) ? FALSE : TRUE);
}

void CModulePropertiesDlg::OnBnClickedSongImport()
{
	CModuleImportDlg importDlg(m_pDocument);

	// TODO use string table
	CFileDialog OpenFileDlg(TRUE, _T("dnm"), 0, OFN_HIDEREADONLY,
							_T(APP_NAME " modules (*.dnm;*.0cc;*.ftm)|*.dnm; *.0cc; *.ftm|All files (*.*)|*.*||"),		// // //
							theApp.GetMainWnd(), 0);

	if (OpenFileDlg.DoModal() == IDCANCEL)
		return;

	if (importDlg.LoadFile(OpenFileDlg.GetPathName(), m_pDocument) == false)
		return;

	importDlg.DoModal();

	FillSongList();
	SelectSong(m_pDocument->GetTrackCount() - 1);		// // //

	m_iExpansions = m_pDocument->GetExpansionChip();		// // //
	m_iN163Channels = m_pDocument->GetNamcoChannels();
	updateN163ChannelCount();
	((CButton*)GetDlgItem(IDC_EXPANSION_VRC6))->SetCheck((m_iExpansions & SNDCHIP_VRC6) != 0);
	((CButton*)GetDlgItem(IDC_EXPANSION_VRC7))->SetCheck((m_iExpansions & SNDCHIP_VRC7) != 0);
	((CButton*)GetDlgItem(IDC_EXPANSION_FDS))->SetCheck((m_iExpansions & SNDCHIP_FDS) != 0);
	((CButton*)GetDlgItem(IDC_EXPANSION_MMC5))->SetCheck((m_iExpansions & SNDCHIP_MMC5) != 0);
	((CButton*)GetDlgItem(IDC_EXPANSION_N163))->SetCheck((m_iExpansions & SNDCHIP_N163) != 0);
	((CButton*)GetDlgItem(IDC_EXPANSION_S5B))->SetCheck((m_iExpansions & SNDCHIP_S5B) != 0);


	for (int i = 0; i < 8; i++)
		updateLevelGUI(i);

	m_pDocument->UpdateAllViews(NULL, UPDATE_PROPERTIES);
}

void CModulePropertiesDlg::OnLvnItemchangedSonglist(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);

	if (pNMLV->uChanged & LVIF_STATE) {
		if (pNMLV->uNewState & LVNI_SELECTED) {		// // //
			m_iSelectedSong = pNMLV->iItem;
			GetDlgItem(IDC_SONGNAME)->SetWindowText(m_pDocument->GetTrackTitle(m_iSelectedSong));
		}
		CListCtrl *pSongList = static_cast<CListCtrl*>(GetDlgItem(IDC_SONGLIST));
		m_bSingleSelection = pSongList->GetSelectedCount() == 1;
		UpdateSongButtons();
	}

	*pResult = 0;
}

void CModulePropertiesDlg::FillSongList()
{
	CListCtrl *pSongList = static_cast<CListCtrl*>(GetDlgItem(IDC_SONGLIST));
	CString Text;

	pSongList->DeleteAllItems();

	// Song editor
	int Songs = m_pDocument->GetTrackCount();

	for (int i = 0; i < Songs; ++i) {
		Text.Format(TRACK_FORMAT, i + 1, m_pDocument->GetTrackTitle(i).GetString());	// start counting songs from 1
		pSongList->InsertItem(i, Text);
	}
}


// **** Expansion selection ****

BOOL CModulePropertiesDlg::PreTranslateMessage(MSG* pMsg)
{
	CListCtrl *pSongList = static_cast<CListCtrl*>(GetDlgItem(IDC_SONGLIST));

	if (GetFocus() == pSongList) {
		if(pMsg->message == WM_KEYDOWN) {
			switch (pMsg->wParam) {
				case VK_DELETE:
					// Delete song
					OnBnClickedSongRemove();		// // //
					break;
				case VK_INSERT:
					// Insert song
					if (m_bSingleSelection)		// // //
						OnBnClickedSongInsert();
					break;
			}
		}
	}

	return CDialog::PreTranslateMessage(pMsg);
}

void CModulePropertiesDlg::OnBnClickedExpansionVRC6()
{
	CButton *pCheckBox = (CButton*)GetDlgItem(IDC_EXPANSION_VRC6);

	if (pCheckBox->GetCheck() == BST_CHECKED)
		m_iExpansions |= SNDCHIP_VRC6;
	else
		m_iExpansions &= ~SNDCHIP_VRC6;

	updateLevelGUI(2);
}

void CModulePropertiesDlg::OnBnClickedExpansionVRC7()
{
	CButton *pCheckBox = (CButton*)GetDlgItem(IDC_EXPANSION_VRC7);

	if (pCheckBox->GetCheck() == BST_CHECKED)
		m_iExpansions |= SNDCHIP_VRC7;
	else
		m_iExpansions &= ~SNDCHIP_VRC7;

	updateLevelGUI(3);
}

void CModulePropertiesDlg::OnBnClickedExpansionFDS()
{
	CButton *pCheckBox = (CButton*)GetDlgItem(IDC_EXPANSION_FDS);

	if (pCheckBox->GetCheck() == BST_CHECKED)
		m_iExpansions |= SNDCHIP_FDS;
	else
		m_iExpansions &= ~SNDCHIP_FDS;

	updateLevelGUI(4);
}

void CModulePropertiesDlg::OnBnClickedExpansionMMC5()
{
	CButton *pCheckBox = (CButton*)GetDlgItem(IDC_EXPANSION_MMC5);

	if (pCheckBox->GetCheck() == BST_CHECKED)
		m_iExpansions |= SNDCHIP_MMC5;
	else
		m_iExpansions &= ~SNDCHIP_MMC5;

	updateLevelGUI(5);
}

void CModulePropertiesDlg::OnBnClickedExpansionS5B()
{
	CButton *pCheckBox = (CButton*)GetDlgItem(IDC_EXPANSION_S5B);

	if (pCheckBox->GetCheck() == BST_CHECKED)
		m_iExpansions |= SNDCHIP_S5B;
	else
		m_iExpansions &= ~SNDCHIP_S5B;		// // //

	updateLevelGUI(7);
}

void CModulePropertiesDlg::OnBnClickedExpansionN163()
{
	CButton *pCheckBox = (CButton*)GetDlgItem(IDC_EXPANSION_N163);
	
	if (pCheckBox->GetCheck() == BST_CHECKED) {
		m_iExpansions |= SNDCHIP_N163;
	} else {
		m_iExpansions &= ~SNDCHIP_N163;
	}
	
	updateN163ChannelCount();
	updateLevelGUI(6);
}


// N163 Channel Count
void CModulePropertiesDlg::setN163NChannels(int nchan) {
	m_iN163Channels = nchan;

	CString text;
	text.LoadString(IDS_PROPERTIES_CHANNELS);
	text.AppendFormat(_T(" %i"), nchan);
	SetDlgItemText(IDC_CHANNELS_NR, text);
}


// N163 GUI
void CModulePropertiesDlg::strFromLevel(CString &target, int Level)
{
	target.Format(_T("%+.1f"), float(-Level) / float(FINE_DELTA));
}

void CModulePropertiesDlg::updateN163ChannelCount(bool renderText)
{
	CStatic *pChannelsLabel = (CStatic*)GetDlgItem(IDC_CHANNELS_NR);
	CString channelsStr;
	channelsStr.LoadString(IDS_PROPERTIES_CHANNELS);

	CSliderCtrl *pChanSlider = (CSliderCtrl*)GetDlgItem(IDC_CHANNELS);
	
	// FIXME https://web.archive.org/web/20080218162024/https://www.microsoft.com/msj/1297/c1297.aspx
	// look up, look down, everything is wrong, this file must be burnt to the ground
	// https://github.com/HertzDevil/0CC-FamiTracker/commit/3400bbb24974ee10ab8ba7afcfa1a3e96f96e7f9#diff-413168e8ded8a41a4d8f9a8b18a34628

	CWnd *N163Enable[]{pChanSlider, pChannelsLabel};

	// Is N163 enabled?
	bool N163Enabled = m_iExpansions & SNDCHIP_N163;
	if (N163Enabled) {
		if (!m_iN163Channels) m_iN163Channels = 1;		// // //
		channelsStr.AppendFormat(_T(" %i"), m_iN163Channels);
	}
	else {
		m_iN163Channels = 0;
		channelsStr.Append(_T(" N/A"));
	}
	
	// Enable/disable UI.
	for (CWnd *widget : N163Enable) widget->EnableWindow(N163Enabled);
	
	// Redraw UI.
	pChanSlider->SetPos(m_iN163Channels);

	SetDlgItemText(IDC_CHANNELS_NR, channelsStr);
}

void CModulePropertiesDlg::updateLevelGUI(int device, bool renderText)
{
	int const deviceLevelSlider[8] = {
		IDC_APU1_OFFSET_SLIDER,
		IDC_APU2_OFFSET_SLIDER,
		IDC_VRC6_OFFSET_SLIDER,
		IDC_VRC7_OFFSET_SLIDER,
		IDC_FDS_OFFSET_SLIDER,
		IDC_MMC5_OFFSET_SLIDER,
		IDC_N163_OFFSET_SLIDER,
		IDC_S5B_OFFSET_SLIDER
	};
	int const deviceLevelEdit[8] = {
		IDC_APU1_OFFSET_EDIT,
		IDC_APU2_OFFSET_EDIT,
		IDC_VRC6_OFFSET_EDIT,
		IDC_VRC7_OFFSET_EDIT,
		IDC_FDS_OFFSET_EDIT,
		IDC_MMC5_OFFSET_EDIT,
		IDC_N163_OFFSET_EDIT,
		IDC_S5B_OFFSET_EDIT
	};

	int const deviceLevelText[8] = {
		IDC_APU1_OFFSET_DB,
		IDC_APU2_OFFSET_DB,
		IDC_VRC6_OFFSET_DB,
		IDC_VRC7_OFFSET_DB,
		IDC_FDS_OFFSET_DB,
		IDC_MMC5_OFFSET_DB,
		IDC_N163_OFFSET_DB,
		IDC_S5B_OFFSET_DB
	};

	int const chipenable[8] = {
		255,
		255,
		SNDCHIP_VRC6,
		SNDCHIP_VRC7,
		SNDCHIP_FDS,
		SNDCHIP_MMC5,
		SNDCHIP_N163,
		SNDCHIP_S5B
	};

	auto levelSlider = static_cast<CSliderCtrl*>(GetDlgItem(deviceLevelSlider[device]));
	auto levelEdit = GetDlgItem(deviceLevelEdit[device]);
	auto levelText = GetDlgItem(deviceLevelText[device]);

	CWnd* ChipEnable[]{ levelSlider, levelEdit, levelText };

	bool ChipEnabled = m_iExpansions & chipenable[device];
	// Always enable 2A03
	if (device <= 1)
		ChipEnabled = true;
	

	if (!ChipEnabled) {
		switch (device) {
		case 0: APU1LevelOffset = 0; break;
		case 1: APU2LevelOffset = 0; break;
		case 2: VRC6LevelOffset = 0; break;
		case 3: VRC7LevelOffset = 0; break;
		case 4: FDSLevelOffset = 0; break;
		case 5: MMC5LevelOffset = 0; break;
		case 6: N163LevelOffset = 0; break;
		case 7: S5BLevelOffset = 0; break;
		}
	}

	// Enable/disable UI.
	for (CWnd* widget : ChipEnable)
		widget->EnableWindow(ChipEnabled);

	// Redraw UI.

	switch (device) {
	case 0:
		levelSlider->SetPos((int)std::round(1.0 * APU1LevelOffset * COARSE_DELTA / FINE_DELTA));
		if (renderText) {
			CString LevelStr;
			strFromLevel(LevelStr, APU1LevelOffset);
			APU1LevelEdit.SetWindowTextNoNotify(LevelStr);
		}
		break;
	case 1:
		levelSlider->SetPos((int)std::round(1.0 * APU2LevelOffset * COARSE_DELTA / FINE_DELTA));
		if (renderText) {
			CString LevelStr;
			strFromLevel(LevelStr, APU2LevelOffset);
			APU2LevelEdit.SetWindowTextNoNotify(LevelStr);
		}
		break;
	case 2:
		levelSlider->SetPos((int)std::round(1.0 * VRC6LevelOffset * COARSE_DELTA / FINE_DELTA));
		if (renderText) {
			CString LevelStr;
			strFromLevel(LevelStr, VRC6LevelOffset);
			VRC6LevelEdit.SetWindowTextNoNotify(LevelStr);
		}
		break;
	case 3:
		levelSlider->SetPos((int)std::round(1.0 * VRC7LevelOffset * COARSE_DELTA / FINE_DELTA));
		if (renderText) {
			CString LevelStr;
			strFromLevel(LevelStr, VRC7LevelOffset);
			VRC7LevelEdit.SetWindowTextNoNotify(LevelStr);
		}
		break;
	case 4:
		levelSlider->SetPos((int)std::round(1.0 * FDSLevelOffset * COARSE_DELTA / FINE_DELTA));
		if (renderText) {
			CString LevelStr;
			strFromLevel(LevelStr, FDSLevelOffset);
			FDSLevelEdit.SetWindowTextNoNotify(LevelStr);
		}
		break;
	case 5:
		levelSlider->SetPos((int)std::round(1.0 * MMC5LevelOffset * COARSE_DELTA / FINE_DELTA));
		if (renderText) {
			CString LevelStr;
			strFromLevel(LevelStr, MMC5LevelOffset);
			MMC5LevelEdit.SetWindowTextNoNotify(LevelStr);
		}
		break;
	case 6:
		levelSlider->SetPos((int)std::round(1.0 * N163LevelOffset * COARSE_DELTA / FINE_DELTA));
		if (renderText) {
			CString LevelStr;
			strFromLevel(LevelStr, N163LevelOffset);
			N163LevelEdit.SetWindowTextNoNotify(LevelStr);
		}
		break;
	case 7:
		levelSlider->SetPos((int)std::round(1.0 * S5BLevelOffset * COARSE_DELTA / FINE_DELTA));
		if (renderText) {
			CString LevelStr;
			strFromLevel(LevelStr, S5BLevelOffset);
			S5BLevelEdit.SetWindowTextNoNotify(LevelStr);
		}
		break;
	}
}

// Level offset callbacks
void CModulePropertiesDlg::OffsetSlider(int device, int pos)
{
	switch (device) {
	case 0:
		APU1LevelOffset = (int)std::round(1.0 * pos * FINE_DELTA / COARSE_DELTA);
		break;
	case 1:
		APU2LevelOffset = (int)std::round(1.0 * pos * FINE_DELTA / COARSE_DELTA);
		break;
	case 2:
		VRC6LevelOffset = (int)std::round(1.0 * pos * FINE_DELTA / COARSE_DELTA);
		break;
	case 3:
		VRC7LevelOffset = (int)std::round(1.0 * pos * FINE_DELTA / COARSE_DELTA);
		break;
	case 4:
		FDSLevelOffset = (int)std::round(1.0 * pos * FINE_DELTA / COARSE_DELTA);
		break;
	case 5:
		MMC5LevelOffset = (int)std::round(1.0 * pos * FINE_DELTA / COARSE_DELTA);
		break;
	case 6:
		N163LevelOffset = (int)std::round(1.0 * pos * FINE_DELTA / COARSE_DELTA);
		break;
	case 7:
		S5BLevelOffset = (int)std::round(1.0 * pos * FINE_DELTA / COARSE_DELTA);
		break;
	}
	updateLevelGUI(device);
}

bool CModulePropertiesDlg::levelFromStr(int &target, CString dBstr) {
	char *endptr;
	double dBval = strtod(dBstr, &endptr);
	if (*endptr == '\0') {									// if no error
		target = static_cast<int>(std::round(dBval * 10));
		target = std::clamp((-target), -MAX_FINE, MAX_FINE);
		return true;
	}
	return false;
}

void CModulePropertiesDlg::OnEnChangeApu1OffsetEdit()
{
	CString str;
	APU1LevelEdit.GetWindowText(str);
	if (levelFromStr(APU1LevelOffset, str)) {
		updateLevelGUI(0, false);
	}
}

void CModulePropertiesDlg::OnEnChangeApu2OffsetEdit()
{
	CString str;
	APU2LevelEdit.GetWindowText(str);
	if (levelFromStr(APU2LevelOffset, str)) {
		updateLevelGUI(1, false);
	}
}


void CModulePropertiesDlg::OnEnChangeVrc6OffsetEdit()
{
	CString str;
	VRC6LevelEdit.GetWindowText(str);
	if (levelFromStr(VRC6LevelOffset, str)) {
		updateLevelGUI(2, false);
	}
}

void CModulePropertiesDlg::OnEnChangeVrc7OffsetEdit()
{
	CString str;
	VRC7LevelEdit.GetWindowText(str);
	if (levelFromStr(VRC7LevelOffset, str)) {
		updateLevelGUI(3, false);
	}
}

void CModulePropertiesDlg::OnEnChangeFdsOffsetEdit()
{
	CString str;
	FDSLevelEdit.GetWindowText(str);
	if (levelFromStr(FDSLevelOffset, str)) {
		updateLevelGUI(4, false);
	}
}


void CModulePropertiesDlg::OnEnChangeMmc5OffsetEdit()
{
	CString str;
	MMC5LevelEdit.GetWindowText(str);
	if (levelFromStr(MMC5LevelOffset, str)) {
		updateLevelGUI(5, false);
	}
}

void CModulePropertiesDlg::OnEnChangeN163OffsetEdit()
{
	CString str;
	N163LevelEdit.GetWindowText(str);
	if (levelFromStr(N163LevelOffset, str)) {
		updateLevelGUI(6, false);
	}
}

void CModulePropertiesDlg::OnEnChangeS5bOffsetEdit()
{
	CString str;
	S5BLevelEdit.GetWindowText(str);
	if (levelFromStr(S5BLevelOffset, str)) {
		updateLevelGUI(7, false);
	}
}
