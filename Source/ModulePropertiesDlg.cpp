/*
** FamiTracker - NES/Famicom sound tracker
** Copyright (C) 2005-2020 Jonathan Liss
**
** 0CC-FamiTracker is (C) 2014-2018 HertzDevil
**
** Dn-FamiTracker is (C) 2020-2023 D.P.C.M.
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
#include <algorithm>
#include <sstream>

#include "FamiTracker.h"
#include "FamiTrackerDoc.h"
#include "MainFrm.h"
#include "ModulePropertiesDlg.h"
#include "ModuleImportDlg.h"
#include "SoundGen.h"
#include "InstrumentEditPanel.h"

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
	ON_BN_CLICKED(IDC_EXTERNAL_OPLL, &CModulePropertiesDlg::OnBnClickedExternalOpll)
	ON_EN_KILLFOCUS(IDC_OPLL_PATCHBYTE1, &CModulePropertiesDlg::OnEnKillfocusOpllPatchbyte1)
	ON_EN_CHANGE(IDC_OPLL_PATCHNAME1, &CModulePropertiesDlg::OnEnChangeOpllPatchname1)
	ON_EN_KILLFOCUS(IDC_OPLL_PATCHBYTE2, &CModulePropertiesDlg::OnEnKillfocusOpllPatchbyte2)
	ON_EN_CHANGE(IDC_OPLL_PATCHNAME2, &CModulePropertiesDlg::OnEnChangeOpllPatchname2)
	ON_EN_KILLFOCUS(IDC_OPLL_PATCHBYTE3, &CModulePropertiesDlg::OnEnKillfocusOpllPatchbyte3)
	ON_EN_CHANGE(IDC_OPLL_PATCHNAME3, &CModulePropertiesDlg::OnEnChangeOpllPatchname3)
	ON_EN_KILLFOCUS(IDC_OPLL_PATCHBYTE4, &CModulePropertiesDlg::OnEnKillfocusOpllPatchbyte4)
	ON_EN_CHANGE(IDC_OPLL_PATCHNAME4, &CModulePropertiesDlg::OnEnChangeOpllPatchname4)
	ON_EN_KILLFOCUS(IDC_OPLL_PATCHBYTE5, &CModulePropertiesDlg::OnEnKillfocusOpllPatchbyte5)
	ON_EN_CHANGE(IDC_OPLL_PATCHNAME5, &CModulePropertiesDlg::OnEnChangeOpllPatchname5)
	ON_EN_KILLFOCUS(IDC_OPLL_PATCHBYTE6, &CModulePropertiesDlg::OnEnKillfocusOpllPatchbyte6)
	ON_EN_CHANGE(IDC_OPLL_PATCHNAME6, &CModulePropertiesDlg::OnEnChangeOpllPatchname6)
	ON_EN_KILLFOCUS(IDC_OPLL_PATCHBYTE7, &CModulePropertiesDlg::OnEnKillfocusOpllPatchbyte7)
	ON_EN_CHANGE(IDC_OPLL_PATCHNAME7, &CModulePropertiesDlg::OnEnChangeOpllPatchname7)
	ON_EN_KILLFOCUS(IDC_OPLL_PATCHBYTE8, &CModulePropertiesDlg::OnEnKillfocusOpllPatchbyte8)
	ON_EN_CHANGE(IDC_OPLL_PATCHNAME8, &CModulePropertiesDlg::OnEnChangeOpllPatchname8)
	ON_EN_KILLFOCUS(IDC_OPLL_PATCHBYTE9, &CModulePropertiesDlg::OnEnKillfocusOpllPatchbyte9)
	ON_EN_CHANGE(IDC_OPLL_PATCHNAME9, &CModulePropertiesDlg::OnEnChangeOpllPatchname9)
	ON_EN_KILLFOCUS(IDC_OPLL_PATCHBYTE10, &CModulePropertiesDlg::OnEnKillfocusOpllPatchbyte10)
	ON_EN_CHANGE(IDC_OPLL_PATCHNAME10, &CModulePropertiesDlg::OnEnChangeOpllPatchname10)
	ON_EN_KILLFOCUS(IDC_OPLL_PATCHBYTE11, &CModulePropertiesDlg::OnEnKillfocusOpllPatchbyte11)
	ON_EN_CHANGE(IDC_OPLL_PATCHNAME11, &CModulePropertiesDlg::OnEnChangeOpllPatchname11)
	ON_EN_KILLFOCUS(IDC_OPLL_PATCHBYTE12, &CModulePropertiesDlg::OnEnKillfocusOpllPatchbyte12)
	ON_EN_CHANGE(IDC_OPLL_PATCHNAME12, &CModulePropertiesDlg::OnEnChangeOpllPatchname12)
	ON_EN_KILLFOCUS(IDC_OPLL_PATCHBYTE13, &CModulePropertiesDlg::OnEnKillfocusOpllPatchbyte13)
	ON_EN_CHANGE(IDC_OPLL_PATCHNAME13, &CModulePropertiesDlg::OnEnChangeOpllPatchname13)
	ON_EN_KILLFOCUS(IDC_OPLL_PATCHBYTE14, &CModulePropertiesDlg::OnEnKillfocusOpllPatchbyte14)
	ON_EN_CHANGE(IDC_OPLL_PATCHNAME14, &CModulePropertiesDlg::OnEnChangeOpllPatchname14)
	ON_EN_KILLFOCUS(IDC_OPLL_PATCHBYTE15, &CModulePropertiesDlg::OnEnKillfocusOpllPatchbyte15)
	ON_EN_CHANGE(IDC_OPLL_PATCHNAME15, &CModulePropertiesDlg::OnEnChangeOpllPatchname15)
	ON_EN_KILLFOCUS(IDC_OPLL_PATCHBYTE16, &CModulePropertiesDlg::OnEnKillfocusOpllPatchbyte16)
	ON_EN_CHANGE(IDC_OPLL_PATCHNAME16, &CModulePropertiesDlg::OnEnChangeOpllPatchname16)
	ON_EN_KILLFOCUS(IDC_OPLL_PATCHBYTE17, &CModulePropertiesDlg::OnEnKillfocusOpllPatchbyte17)
	ON_EN_CHANGE(IDC_OPLL_PATCHNAME17, &CModulePropertiesDlg::OnEnChangeOpllPatchname17)
	ON_EN_KILLFOCUS(IDC_OPLL_PATCHBYTE18, &CModulePropertiesDlg::OnEnKillfocusOpllPatchbyte18)
	ON_EN_CHANGE(IDC_OPLL_PATCHNAME18, &CModulePropertiesDlg::OnEnChangeOpllPatchname18)
	ON_BN_CLICKED(IDC_SURVEY_MIXING, &CModulePropertiesDlg::OnBnClickedSurveyMixing)
END_MESSAGE_MAP()


const int LEVEL_RANGE = 12;		// +/- 12 dB range

const int FINE_DELTA = 10;		// you better hope this matches CConfigMixer::LEVEL_SCALE and SoundGen.cpp.
const int COARSE_DELTA = 1;		// slider notches per dB.
const int PAGEUP = COARSE_DELTA * 2;	// 2 dB per pageup.

const int MAX_FINE = LEVEL_RANGE * FINE_DELTA;
	// user input is clamped to +-MAX_FINE.

constexpr std::array<unsigned int, 19> IDC_STATIC_PATCH = {
	IDC_STATIC_PATCH0,
	IDC_STATIC_PATCH1,
	IDC_STATIC_PATCH2,
	IDC_STATIC_PATCH3,
	IDC_STATIC_PATCH4,
	IDC_STATIC_PATCH5,
	IDC_STATIC_PATCH6,
	IDC_STATIC_PATCH7,
	IDC_STATIC_PATCH8,
	IDC_STATIC_PATCH9,
	IDC_STATIC_PATCH10,
	IDC_STATIC_PATCH11,
	IDC_STATIC_PATCH12,
	IDC_STATIC_PATCH13,
	IDC_STATIC_PATCH14,
	IDC_STATIC_PATCH15,
	IDC_STATIC_PATCH16,
	IDC_STATIC_PATCH17,
	IDC_STATIC_PATCH18
};

constexpr std::array<unsigned int, 19> IDC_OPLL_PATCHBYTE = {
	IDC_OPLL_PATCHBYTE0,
	IDC_OPLL_PATCHBYTE1,
	IDC_OPLL_PATCHBYTE2,
	IDC_OPLL_PATCHBYTE3,
	IDC_OPLL_PATCHBYTE4,
	IDC_OPLL_PATCHBYTE5,
	IDC_OPLL_PATCHBYTE6,
	IDC_OPLL_PATCHBYTE7,
	IDC_OPLL_PATCHBYTE8,
	IDC_OPLL_PATCHBYTE9,
	IDC_OPLL_PATCHBYTE10,
	IDC_OPLL_PATCHBYTE11,
	IDC_OPLL_PATCHBYTE12,
	IDC_OPLL_PATCHBYTE13,
	IDC_OPLL_PATCHBYTE14,
	IDC_OPLL_PATCHBYTE15,
	IDC_OPLL_PATCHBYTE16,
	IDC_OPLL_PATCHBYTE17,
	IDC_OPLL_PATCHBYTE18
};

constexpr std::array<unsigned int, 19> IDC_OPLL_PATCHNAME = {
	IDC_OPLL_PATCHNAME0,
	IDC_OPLL_PATCHNAME1,
	IDC_OPLL_PATCHNAME2,
	IDC_OPLL_PATCHNAME3,
	IDC_OPLL_PATCHNAME4,
	IDC_OPLL_PATCHNAME5,
	IDC_OPLL_PATCHNAME6,
	IDC_OPLL_PATCHNAME7,
	IDC_OPLL_PATCHNAME8,
	IDC_OPLL_PATCHNAME9,
	IDC_OPLL_PATCHNAME10,
	IDC_OPLL_PATCHNAME11,
	IDC_OPLL_PATCHNAME12,
	IDC_OPLL_PATCHNAME13,
	IDC_OPLL_PATCHNAME14,
	IDC_OPLL_PATCHNAME15,
	IDC_OPLL_PATCHNAME16,
	IDC_OPLL_PATCHNAME17,
	IDC_OPLL_PATCHNAME18
};

constexpr std::array<unsigned int, 8> IDC_DEVICE_OFFSET_EDIT = {
	IDC_APU1_OFFSET_EDIT,
	IDC_APU2_OFFSET_EDIT,
	IDC_VRC6_OFFSET_EDIT,
	IDC_VRC7_OFFSET_EDIT,
	IDC_FDS_OFFSET_EDIT,
	IDC_MMC5_OFFSET_EDIT,
	IDC_N163_OFFSET_EDIT,
	IDC_S5B_OFFSET_EDIT
};

constexpr std::array<unsigned int, 8> IDC_DEVICE_OFFSET_SLIDER = {
	IDC_APU1_OFFSET_SLIDER,
	IDC_APU2_OFFSET_SLIDER,
	IDC_VRC6_OFFSET_SLIDER,
	IDC_VRC7_OFFSET_SLIDER,
	IDC_FDS_OFFSET_SLIDER,
	IDC_MMC5_OFFSET_SLIDER,
	IDC_N163_OFFSET_SLIDER,
	IDC_S5B_OFFSET_SLIDER
};

constexpr std::array<unsigned int, 8> IDC_DEVICE_OFFSET_DB = {
	IDC_APU1_OFFSET_DB,
	IDC_APU2_OFFSET_DB,
	IDC_VRC6_OFFSET_DB,
	IDC_VRC7_OFFSET_DB,
	IDC_FDS_OFFSET_DB,
	IDC_MMC5_OFFSET_DB,
	IDC_N163_OFFSET_DB,
	IDC_S5B_OFFSET_DB
};

constexpr std::array<unsigned int, 8> IDC_STATIC_DEVICE = {
	IDC_STATIC_APU1,
	IDC_STATIC_APU2,
	IDC_STATIC_VRC6,
	IDC_STATIC_VRC7,
	IDC_STATIC_FDS,
	IDC_STATIC_MMC5,
	IDC_STATIC_N163,
	IDC_STATIC_S5B
};

// CModulePropertiesDlg message handlers
BOOL CModulePropertiesDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

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

	// N163 channel count UI
	m_cChannelsLabel.SubclassDlgItem(IDC_CHANNELS_NR, this);
	m_cChanSlider.SubclassDlgItem(IDC_CHANNELS, this);

	// Device mix offset UI
	for (int i = 0; i < 8; i++) {
		m_cDeviceLevelEdit[i].SubclassDlgItem(IDC_DEVICE_OFFSET_EDIT[i], this);
		m_cDeviceLevelSlider[i].SubclassDlgItem(IDC_DEVICE_OFFSET_SLIDER[i], this);
		m_cDevicedBLabel[i].SubclassDlgItem(IDC_DEVICE_OFFSET_DB[i], this);
		m_cDeviceLevelLabel[i].SubclassDlgItem(IDC_STATIC_DEVICE[i], this);

		SetupSlider(IDC_DEVICE_OFFSET_SLIDER[i]);
		m_iDeviceLevelOffset[i] = -m_pDocument->GetLevelOffset(i);
		updateDeviceMixOffsetUI(i);
	}


	// Hardware-based mixing
	m_bSurveyMixing = m_pDocument->GetSurveyMixCheck();
	((CButton*)GetDlgItem(IDC_SURVEY_MIXING))->SetCheck(m_bSurveyMixing);

	// OPLL patch bytes and patch names
	m_bExternalOPLL = m_pDocument->GetExternalOPLLChipCheck();

	((CButton*)GetDlgItem(IDC_EXTERNAL_OPLL))->SetCheck(m_bExternalOPLL);
	for (int i = 0; i < 19; ++i) {
		m_cOPLLPatchLabel[i].SubclassDlgItem(IDC_STATIC_PATCH[i], this);
		m_cOPLLPatchBytesEdit[i].SubclassDlgItem(IDC_OPLL_PATCHBYTE[i], this);
		m_cOPLLPatchNameEdit[i].SubclassDlgItem(IDC_OPLL_PATCHNAME[i], this);
	}

	if (m_bExternalOPLL)
		for (int i = 0; i < 19; ++i) {
			for (int j = 0; j < 8; j++)
				m_iOPLLPatchBytes[(8 * i) + j] = m_pDocument->GetOPLLPatchByte((8 * i) + j);
			m_strOPLLPatchNames[i] = m_pDocument->GetOPLLPatchName(i);
		}
	else {
		// initialize default patchset if it hasn't been already
		m_pDocument->SetOPLLPatchSet(theApp.GetSettings()->Emulation.iVRC7Patch);
		for (int i = 0; i < 19; ++i) {
			for (int j = 0; j < 8; j++)
				m_iOPLLPatchBytes[(8 * i) + j] = m_pDocument->GetOPLLPatchByte((8 * i) + j);
			m_strOPLLPatchNames[i] = m_pDocument->GetOPLLPatchName(i);
		}
	}

	// Update UI after, since this updates all components at once,
	// and all window objects need to be valid at this point
	for (int i = 0; i < 19; i++)
		updateExternallOPLLUI(i);

	// Namco channel count
	CSliderCtrl *pChanSlider = static_cast<CSliderCtrl*>(GetDlgItem(IDC_CHANNELS));
	pChanSlider->SetRange(1, 8);
	m_iN163Channels = m_pDocument->GetNamcoChannels();
	updateN163ChannelCountUI();

	// Vibrato 
	CComboBox *pVibratoBox = static_cast<CComboBox*>(GetDlgItem(IDC_VIBRATO));
	pVibratoBox->SetCurSel((m_pDocument->GetVibratoStyle() == VIBRATO_NEW) ? 0 : 1);
	
	// Pitch
	CComboBox *pPitchBox = static_cast<CComboBox*>(GetDlgItem(IDC_COMBO_LINEARPITCH));		// // //
	pPitchBox->SetCurSel(m_pDocument->GetLinearPitch() ? 1 : 0);
	


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

	// Device mix offset
	for (int i = 0; i < 8; i++)
		m_pDocument->SetLevelOffset(i, -m_iDeviceLevelOffset[i]);

	// Hardware-based mixing
	m_pDocument->SetSurveyMixCheck(m_bSurveyMixing);

	// Externall OPLL
	m_pDocument->SetExternalOPLLChipCheck(m_bExternalOPLL);

	if (m_bExternalOPLL)
		for (int i = 0; i < 19; i++) {
			for (int j = 0; j < 8; j++)
				m_pDocument->SetOPLLPatchByte((8 * i) + j, m_iOPLLPatchBytes[(8 * i) + j]);
			m_pDocument->SetOPLLPatchName(i, m_strOPLLPatchNames[i]);
		}
	else
		m_pDocument->SetOPLLPatchSet(theApp.GetSettings()->Emulation.iVRC7Patch);

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
	updateN163ChannelCountUI();
	((CButton*)GetDlgItem(IDC_EXPANSION_VRC6))->SetCheck((m_iExpansions & SNDCHIP_VRC6) != 0);
	((CButton*)GetDlgItem(IDC_EXPANSION_VRC7))->SetCheck((m_iExpansions & SNDCHIP_VRC7) != 0);
	((CButton*)GetDlgItem(IDC_EXPANSION_FDS))->SetCheck((m_iExpansions & SNDCHIP_FDS) != 0);
	((CButton*)GetDlgItem(IDC_EXPANSION_MMC5))->SetCheck((m_iExpansions & SNDCHIP_MMC5) != 0);
	((CButton*)GetDlgItem(IDC_EXPANSION_N163))->SetCheck((m_iExpansions & SNDCHIP_N163) != 0);
	((CButton*)GetDlgItem(IDC_EXPANSION_S5B))->SetCheck((m_iExpansions & SNDCHIP_S5B) != 0);


	for (int i = 0; i < 8; i++)
		updateDeviceMixOffsetUI(i);

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

	updateDeviceMixOffsetUI(2);
}

void CModulePropertiesDlg::OnBnClickedExpansionVRC7()
{
	CButton *pCheckBox = (CButton*)GetDlgItem(IDC_EXPANSION_VRC7);

	if (pCheckBox->GetCheck() == BST_CHECKED)
		m_iExpansions |= SNDCHIP_VRC7;
	else
		m_iExpansions &= ~SNDCHIP_VRC7;

	updateDeviceMixOffsetUI(3);
}

void CModulePropertiesDlg::OnBnClickedExpansionFDS()
{
	CButton *pCheckBox = (CButton*)GetDlgItem(IDC_EXPANSION_FDS);

	if (pCheckBox->GetCheck() == BST_CHECKED)
		m_iExpansions |= SNDCHIP_FDS;
	else
		m_iExpansions &= ~SNDCHIP_FDS;

	updateDeviceMixOffsetUI(4);
}

void CModulePropertiesDlg::OnBnClickedExpansionMMC5()
{
	CButton *pCheckBox = (CButton*)GetDlgItem(IDC_EXPANSION_MMC5);

	if (pCheckBox->GetCheck() == BST_CHECKED)
		m_iExpansions |= SNDCHIP_MMC5;
	else
		m_iExpansions &= ~SNDCHIP_MMC5;

	updateDeviceMixOffsetUI(5);
}

void CModulePropertiesDlg::OnBnClickedExpansionS5B()
{
	CButton *pCheckBox = (CButton*)GetDlgItem(IDC_EXPANSION_S5B);

	if (pCheckBox->GetCheck() == BST_CHECKED)
		m_iExpansions |= SNDCHIP_S5B;
	else
		m_iExpansions &= ~SNDCHIP_S5B;		// // //

	updateDeviceMixOffsetUI(7);
}

void CModulePropertiesDlg::OnBnClickedExpansionN163()
{
	CButton *pCheckBox = (CButton*)GetDlgItem(IDC_EXPANSION_N163);
	
	if (pCheckBox->GetCheck() == BST_CHECKED) {
		m_iExpansions |= SNDCHIP_N163;
	} else {
		m_iExpansions &= ~SNDCHIP_N163;
	}
	
	updateN163ChannelCountUI();
	updateDeviceMixOffsetUI(6);
}

// N163 Channel Count
void CModulePropertiesDlg::setN163NChannels(int nchan) {
	m_iN163Channels = nchan;

	CString text;
	text.LoadString(IDS_PROPERTIES_CHANNELS);
	text.AppendFormat(_T(" %i"), nchan);
	SetDlgItemText(IDC_CHANNELS_NR, text);
}

// Device mix offset GUI
void CModulePropertiesDlg::strFromLevel(CString &target, int16_t Level)
{
	target.Format(_T("%+.1f"), float(-Level) / float(FINE_DELTA));
}

void CModulePropertiesDlg::updateN163ChannelCountUI()
{
	CString channelsStr;
	channelsStr.LoadString(IDS_PROPERTIES_CHANNELS);

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
	m_cChannelsLabel.EnableWindow(N163Enabled);
	m_cChanSlider.EnableWindow(N163Enabled);
	
	// Redraw UI.
	m_cChanSlider.SetPos(m_iN163Channels);

	SetDlgItemText(IDC_CHANNELS_NR, channelsStr);
}

void CModulePropertiesDlg::updateDeviceMixOffsetUI(int device, bool renderText)
{
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

	std::array<CWnd*, 4> DeviceMixOffsetUI = {
		&m_cDeviceLevelSlider[device],
		&m_cDeviceLevelEdit[device],
		&m_cDevicedBLabel[device],
		&m_cDeviceLevelLabel[device]
	};

	bool ChipEnabled = m_iExpansions & chipenable[device];

	// Always enable 2A03
	if (device <= 1)
		ChipEnabled = true;

	if (!ChipEnabled)
		m_iDeviceLevelOffset[device] = 0;

	// Enable/disable UI.
	for (auto *widget : DeviceMixOffsetUI)
		widget->EnableWindow(ChipEnabled);

	if (device == 3)
		((CButton*)GetDlgItem(IDC_EXTERNAL_OPLL))->EnableWindow(ChipEnabled);

	// Redraw UI.
	m_cDeviceLevelSlider[device].SetPos((int)std::round(1.0 * m_iDeviceLevelOffset[device] * COARSE_DELTA / FINE_DELTA));
	if (renderText) {
		CString LevelStr;
		strFromLevel(LevelStr, m_iDeviceLevelOffset[device]);
		m_cDeviceLevelEdit[device].SetWindowTextNoNotify(LevelStr);
	}
}

// Level offset callbacks
void CModulePropertiesDlg::OffsetSlider(int device, int pos)
{
	m_iDeviceLevelOffset[device] = (int16_t)std::round(1.0 * pos * FINE_DELTA / COARSE_DELTA);
	updateDeviceMixOffsetUI(device);
}

bool CModulePropertiesDlg::levelFromStr(int16_t &target, CString dBstr) {
	char *endptr;
	double dBval = strtod(dBstr, &endptr);
	if (*endptr == '\0') {									// if no error
		target = static_cast<int16_t>(std::round(dBval * 10));
		target = std::clamp((-target), -MAX_FINE, MAX_FINE);
		return true;
	}
	return false;
}

void CModulePropertiesDlg::DeviceOffsetEdit(int device)
{
	CString str;
	m_cDeviceLevelEdit[device].GetWindowText(str);
	if (levelFromStr(m_iDeviceLevelOffset[device], str)) {
		updateDeviceMixOffsetUI(device, false);
	}
}

void CModulePropertiesDlg::OnEnChangeApu1OffsetEdit()
{
	DeviceOffsetEdit(0);
}

void CModulePropertiesDlg::OnEnChangeApu2OffsetEdit()
{
	DeviceOffsetEdit(1);
}

void CModulePropertiesDlg::OnEnChangeVrc6OffsetEdit()
{
	DeviceOffsetEdit(2);
}

void CModulePropertiesDlg::OnEnChangeVrc7OffsetEdit()
{
	DeviceOffsetEdit(3);
}

void CModulePropertiesDlg::OnEnChangeFdsOffsetEdit()
{
	DeviceOffsetEdit(4);
}

void CModulePropertiesDlg::OnEnChangeMmc5OffsetEdit()
{
	DeviceOffsetEdit(5);
}

void CModulePropertiesDlg::OnEnChangeN163OffsetEdit()
{
	DeviceOffsetEdit(6);
}

void CModulePropertiesDlg::OnEnChangeS5bOffsetEdit()
{
	DeviceOffsetEdit(7);
}

// Externall OPLL UI
void CModulePropertiesDlg::updateExternallOPLLUI(int patchnum, bool renderText)
{
	bool ValidOPLLState = m_bExternalOPLL && (m_iExpansions & SNDCHIP_VRC7);

	// Enable/disable entire UI.
	for (int i = 0; i < 19; i++) {
		m_cOPLLPatchLabel[i].EnableWindow(ValidOPLLState);
		m_cOPLLPatchBytesEdit[i].EnableWindow(ValidOPLLState);
		m_cOPLLPatchNameEdit[i].EnableWindow(ValidOPLLState);
	}

	// Redraw UI.
	// update patch names
	if (renderText) {
		uint8_t patchbytes[8]{};
		for (int i = 0; i < 8; i++)
			patchbytes[i] = m_iOPLLPatchBytes[(8 * patchnum) + i];
		m_cOPLLPatchBytesEdit[patchnum].SetWindowTextNoNotify(PatchBytesToText(patchbytes));
		m_cOPLLPatchNameEdit[patchnum].SetWindowTextNoNotify(m_strOPLLPatchNames[patchnum].c_str());
	}
}

CString CModulePropertiesDlg::PatchBytesToText(uint8_t* patchbytes)
{
	CString patchtxt;

	for (int i = 0; i < 8; ++i)
		patchtxt.AppendFormat(_T("$%02X "), patchbytes[i]);

	return patchtxt;
}

void CModulePropertiesDlg::PatchTextToBytes(LPCTSTR pString, int index)
{
	std::string str(pString);
	std::vector<uint8_t> patchbytes(8, 0);

	// Convert to register values
	std::istringstream values(str);
	std::istream_iterator<std::string> begin(values);
	std::istream_iterator<std::string> end;

	for (int i = 0; (i < 8) && (begin != end); ++i) {
		int value = CSequenceInstrumentEditPanel::ReadStringValue(*begin++, false);		// // //
		if (value < 0) value = 0;
		if (value > 0xFF) value = 0xFF;
		m_iOPLLPatchBytes[(8 * index) + i] = value;
	}
}

void CModulePropertiesDlg::OnBnClickedExternalOpll()
{
	m_bExternalOPLL = ((CButton*)GetDlgItem(IDC_EXTERNAL_OPLL))->GetCheck() == BST_CHECKED;

	for (int i = 0; i < 19; i++)
		updateExternallOPLLUI(i);
}

void CModulePropertiesDlg::OpllPatchByteEdit(int patchnum)
{
	CString str;
	m_cOPLLPatchBytesEdit[patchnum].GetWindowText(str);
	PatchTextToBytes(str, patchnum);

	updateExternallOPLLUI(patchnum, false);
}

void CModulePropertiesDlg::OpllPatchNameEdit(int patchnum)
{
	CString str;
	m_cOPLLPatchNameEdit[patchnum].GetWindowText(str);
	m_strOPLLPatchNames[patchnum] = str;
	updateExternallOPLLUI(patchnum, false);
}

void CModulePropertiesDlg::OnEnKillfocusOpllPatchbyte1()
{
	OpllPatchByteEdit(1);
}

void CModulePropertiesDlg::OnEnChangeOpllPatchname1()
{
	OpllPatchNameEdit(1);
}

void CModulePropertiesDlg::OnEnKillfocusOpllPatchbyte2()
{
	OpllPatchByteEdit(2);
}

void CModulePropertiesDlg::OnEnChangeOpllPatchname2()
{
	OpllPatchNameEdit(2);
}

void CModulePropertiesDlg::OnEnKillfocusOpllPatchbyte3()
{
	OpllPatchByteEdit(3);
}

void CModulePropertiesDlg::OnEnChangeOpllPatchname3()
{
	OpllPatchNameEdit(3);
}

void CModulePropertiesDlg::OnEnKillfocusOpllPatchbyte4()
{
	OpllPatchByteEdit(4);
}

void CModulePropertiesDlg::OnEnChangeOpllPatchname4()
{
	OpllPatchNameEdit(4);
}

void CModulePropertiesDlg::OnEnKillfocusOpllPatchbyte5()
{
	OpllPatchByteEdit(5);
}

void CModulePropertiesDlg::OnEnChangeOpllPatchname5()
{
	OpllPatchNameEdit(5);
}

void CModulePropertiesDlg::OnEnKillfocusOpllPatchbyte6()
{
	OpllPatchByteEdit(6);
}

void CModulePropertiesDlg::OnEnChangeOpllPatchname6()
{
	OpllPatchNameEdit(6);
}

void CModulePropertiesDlg::OnEnKillfocusOpllPatchbyte7()
{
	OpllPatchByteEdit(7);
}

void CModulePropertiesDlg::OnEnChangeOpllPatchname7()
{
	OpllPatchNameEdit(7);
}

void CModulePropertiesDlg::OnEnKillfocusOpllPatchbyte8()
{
	OpllPatchByteEdit(8);
}

void CModulePropertiesDlg::OnEnChangeOpllPatchname8()
{
	OpllPatchNameEdit(8);
}

void CModulePropertiesDlg::OnEnKillfocusOpllPatchbyte9()
{
	OpllPatchByteEdit(9);
}

void CModulePropertiesDlg::OnEnChangeOpllPatchname9()
{
	OpllPatchNameEdit(9);
}

void CModulePropertiesDlg::OnEnKillfocusOpllPatchbyte10()
{
	OpllPatchByteEdit(10);
}

void CModulePropertiesDlg::OnEnChangeOpllPatchname10()
{
	OpllPatchNameEdit(10);
}

void CModulePropertiesDlg::OnEnKillfocusOpllPatchbyte11()
{
	OpllPatchByteEdit(11);
}

void CModulePropertiesDlg::OnEnChangeOpllPatchname11()
{
	OpllPatchNameEdit(11);
}

void CModulePropertiesDlg::OnEnKillfocusOpllPatchbyte12()
{
	OpllPatchByteEdit(12);
}

void CModulePropertiesDlg::OnEnChangeOpllPatchname12()
{
	OpllPatchNameEdit(12);
}

void CModulePropertiesDlg::OnEnKillfocusOpllPatchbyte13()
{
	OpllPatchByteEdit(13);
}

void CModulePropertiesDlg::OnEnChangeOpllPatchname13()
{
	OpllPatchNameEdit(13);
}

void CModulePropertiesDlg::OnEnKillfocusOpllPatchbyte14()
{
	OpllPatchByteEdit(14);
}

void CModulePropertiesDlg::OnEnChangeOpllPatchname14()
{
	OpllPatchNameEdit(14);
}

void CModulePropertiesDlg::OnEnKillfocusOpllPatchbyte15()
{
	OpllPatchByteEdit(15);
}

void CModulePropertiesDlg::OnEnChangeOpllPatchname15()
{
	OpllPatchNameEdit(15);
}

void CModulePropertiesDlg::OnEnKillfocusOpllPatchbyte16()
{
	OpllPatchByteEdit(16);
}

void CModulePropertiesDlg::OnEnChangeOpllPatchname16()
{
	OpllPatchNameEdit(16);
}

void CModulePropertiesDlg::OnEnKillfocusOpllPatchbyte17()
{
	OpllPatchByteEdit(17);
}

void CModulePropertiesDlg::OnEnChangeOpllPatchname17()
{
	OpllPatchNameEdit(17);
}

void CModulePropertiesDlg::OnEnKillfocusOpllPatchbyte18()
{
	OpllPatchByteEdit(18);
}

void CModulePropertiesDlg::OnEnChangeOpllPatchname18()
{
	OpllPatchNameEdit(18);
}

void CModulePropertiesDlg::OnBnClickedSurveyMixing()
{
	m_bSurveyMixing = (((CButton*)GetDlgItem(IDC_SURVEY_MIXING))->GetCheck() == BST_CHECKED);
}
