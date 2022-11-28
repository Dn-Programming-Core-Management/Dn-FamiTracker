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

	// N163 channel count UI
	m_cChannelsLabel.SubclassDlgItem(IDC_CHANNELS_NR, this);
	m_cChanSlider.SubclassDlgItem(IDC_CHANNELS, this);

	// Device mix offset UI
	m_cAPU1LevelEdit.SubclassDlgItem(IDC_APU1_OFFSET_EDIT, this);
	m_cAPU2LevelEdit.SubclassDlgItem(IDC_APU2_OFFSET_EDIT, this);
	m_cVRC6LevelEdit.SubclassDlgItem(IDC_VRC6_OFFSET_EDIT, this);
	m_cVRC7LevelEdit.SubclassDlgItem(IDC_VRC7_OFFSET_EDIT, this);
	m_cFDSLevelEdit.SubclassDlgItem(IDC_FDS_OFFSET_EDIT, this);
	m_cMMC5LevelEdit.SubclassDlgItem(IDC_MMC5_OFFSET_EDIT, this);
	m_cN163LevelEdit.SubclassDlgItem(IDC_N163_OFFSET_EDIT, this);
	m_cS5BLevelEdit.SubclassDlgItem(IDC_S5B_OFFSET_EDIT, this);

	m_cAPU1LevelSlider.SubclassDlgItem(IDC_APU1_OFFSET_SLIDER, this);
	m_cAPU2LevelSlider.SubclassDlgItem(IDC_APU2_OFFSET_SLIDER, this);
	m_cVRC6LevelSlider.SubclassDlgItem(IDC_VRC6_OFFSET_SLIDER, this);
	m_cVRC7LevelSlider.SubclassDlgItem(IDC_VRC7_OFFSET_SLIDER, this);
	m_cFDSLevelSlider.SubclassDlgItem(IDC_FDS_OFFSET_SLIDER, this);
	m_cMMC5LevelSlider.SubclassDlgItem(IDC_MMC5_OFFSET_SLIDER, this);
	m_cN163LevelSlider.SubclassDlgItem(IDC_N163_OFFSET_SLIDER, this);
	m_cS5BLevelSlider.SubclassDlgItem(IDC_S5B_OFFSET_SLIDER, this);

	m_cAPU1dBLabel.SubclassDlgItem(IDC_APU1_OFFSET_DB, this);
	m_cAPU2dBLabel.SubclassDlgItem(IDC_APU2_OFFSET_DB, this);
	m_cVRC6dBLabel.SubclassDlgItem(IDC_VRC6_OFFSET_DB, this);
	m_cVRC7dBLabel.SubclassDlgItem(IDC_VRC7_OFFSET_DB, this);
	m_cFDSdBLabel.SubclassDlgItem(IDC_FDS_OFFSET_DB, this);
	m_cMMC5dBLabel.SubclassDlgItem(IDC_MMC5_OFFSET_DB, this);
	m_cN163dBLabel.SubclassDlgItem(IDC_N163_OFFSET_DB, this);
	m_cS5BdBLabel.SubclassDlgItem(IDC_S5B_OFFSET_DB, this);

	m_cAPU1LevelLabel.SubclassDlgItem(IDC_STATIC_APU1, this);
	m_cAPU2LevelLabel.SubclassDlgItem(IDC_STATIC_APU2, this);
	m_cVRC6LevelLabel.SubclassDlgItem(IDC_STATIC_VRC6, this);
	m_cVRC7LevelLabel.SubclassDlgItem(IDC_STATIC_VRC7, this);
	m_cFDSLevelLabel.SubclassDlgItem(IDC_STATIC_FDS, this);
	m_cMMC5LevelLabel.SubclassDlgItem(IDC_STATIC_MMC5, this);
	m_cN163LevelLabel.SubclassDlgItem(IDC_STATIC_N163, this);
	m_cS5BLevelLabel.SubclassDlgItem(IDC_STATIC_S5B, this);

	// External OPLL UI
	m_cOPLLPatchLabel0.SubclassDlgItem(IDC_STATIC_PATCH0, this);
	m_cOPLLPatchLabel1.SubclassDlgItem(IDC_STATIC_PATCH1, this);
	m_cOPLLPatchLabel2.SubclassDlgItem(IDC_STATIC_PATCH2, this);
	m_cOPLLPatchLabel3.SubclassDlgItem(IDC_STATIC_PATCH3, this);
	m_cOPLLPatchLabel4.SubclassDlgItem(IDC_STATIC_PATCH4, this);
	m_cOPLLPatchLabel5.SubclassDlgItem(IDC_STATIC_PATCH5, this);
	m_cOPLLPatchLabel6.SubclassDlgItem(IDC_STATIC_PATCH6, this);
	m_cOPLLPatchLabel7.SubclassDlgItem(IDC_STATIC_PATCH7, this);
	m_cOPLLPatchLabel8.SubclassDlgItem(IDC_STATIC_PATCH8, this);
	m_cOPLLPatchLabel9.SubclassDlgItem(IDC_STATIC_PATCH9, this);
	m_cOPLLPatchLabel10.SubclassDlgItem(IDC_STATIC_PATCH10, this);
	m_cOPLLPatchLabel11.SubclassDlgItem(IDC_STATIC_PATCH11, this);
	m_cOPLLPatchLabel12.SubclassDlgItem(IDC_STATIC_PATCH12, this);
	m_cOPLLPatchLabel13.SubclassDlgItem(IDC_STATIC_PATCH13, this);
	m_cOPLLPatchLabel14.SubclassDlgItem(IDC_STATIC_PATCH14, this);
	m_cOPLLPatchLabel15.SubclassDlgItem(IDC_STATIC_PATCH15, this);
	m_cOPLLPatchLabel16.SubclassDlgItem(IDC_STATIC_PATCH16, this);
	m_cOPLLPatchLabel17.SubclassDlgItem(IDC_STATIC_PATCH17, this);
	m_cOPLLPatchLabel18.SubclassDlgItem(IDC_STATIC_PATCH18, this);

	m_cOPLLPatchBytesEdit0.SubclassDlgItem(IDC_OPLL_PATCHBYTE0, this);
	m_cOPLLPatchBytesEdit1.SubclassDlgItem(IDC_OPLL_PATCHBYTE1, this);
	m_cOPLLPatchBytesEdit2.SubclassDlgItem(IDC_OPLL_PATCHBYTE2, this);
	m_cOPLLPatchBytesEdit3.SubclassDlgItem(IDC_OPLL_PATCHBYTE3, this);
	m_cOPLLPatchBytesEdit4.SubclassDlgItem(IDC_OPLL_PATCHBYTE4, this);
	m_cOPLLPatchBytesEdit5.SubclassDlgItem(IDC_OPLL_PATCHBYTE5, this);
	m_cOPLLPatchBytesEdit6.SubclassDlgItem(IDC_OPLL_PATCHBYTE6, this);
	m_cOPLLPatchBytesEdit7.SubclassDlgItem(IDC_OPLL_PATCHBYTE7, this);
	m_cOPLLPatchBytesEdit8.SubclassDlgItem(IDC_OPLL_PATCHBYTE8, this);
	m_cOPLLPatchBytesEdit9.SubclassDlgItem(IDC_OPLL_PATCHBYTE9, this);
	m_cOPLLPatchBytesEdit10.SubclassDlgItem(IDC_OPLL_PATCHBYTE10, this);
	m_cOPLLPatchBytesEdit11.SubclassDlgItem(IDC_OPLL_PATCHBYTE11, this);
	m_cOPLLPatchBytesEdit12.SubclassDlgItem(IDC_OPLL_PATCHBYTE12, this);
	m_cOPLLPatchBytesEdit13.SubclassDlgItem(IDC_OPLL_PATCHBYTE13, this);
	m_cOPLLPatchBytesEdit14.SubclassDlgItem(IDC_OPLL_PATCHBYTE14, this);
	m_cOPLLPatchBytesEdit15.SubclassDlgItem(IDC_OPLL_PATCHBYTE15, this);
	m_cOPLLPatchBytesEdit16.SubclassDlgItem(IDC_OPLL_PATCHBYTE16, this);
	m_cOPLLPatchBytesEdit17.SubclassDlgItem(IDC_OPLL_PATCHBYTE17, this);
	m_cOPLLPatchBytesEdit18.SubclassDlgItem(IDC_OPLL_PATCHBYTE18, this);

	m_cOPLLPatchNameEdit0.SubclassDlgItem(IDC_OPLL_PATCHNAME0, this);
	m_cOPLLPatchNameEdit1.SubclassDlgItem(IDC_OPLL_PATCHNAME1, this);
	m_cOPLLPatchNameEdit2.SubclassDlgItem(IDC_OPLL_PATCHNAME2, this);
	m_cOPLLPatchNameEdit3.SubclassDlgItem(IDC_OPLL_PATCHNAME3, this);
	m_cOPLLPatchNameEdit4.SubclassDlgItem(IDC_OPLL_PATCHNAME4, this);
	m_cOPLLPatchNameEdit5.SubclassDlgItem(IDC_OPLL_PATCHNAME5, this);
	m_cOPLLPatchNameEdit6.SubclassDlgItem(IDC_OPLL_PATCHNAME6, this);
	m_cOPLLPatchNameEdit7.SubclassDlgItem(IDC_OPLL_PATCHNAME7, this);
	m_cOPLLPatchNameEdit8.SubclassDlgItem(IDC_OPLL_PATCHNAME8, this);
	m_cOPLLPatchNameEdit9.SubclassDlgItem(IDC_OPLL_PATCHNAME9, this);
	m_cOPLLPatchNameEdit10.SubclassDlgItem(IDC_OPLL_PATCHNAME10, this);
	m_cOPLLPatchNameEdit11.SubclassDlgItem(IDC_OPLL_PATCHNAME11, this);
	m_cOPLLPatchNameEdit12.SubclassDlgItem(IDC_OPLL_PATCHNAME12, this);
	m_cOPLLPatchNameEdit13.SubclassDlgItem(IDC_OPLL_PATCHNAME13, this);
	m_cOPLLPatchNameEdit14.SubclassDlgItem(IDC_OPLL_PATCHNAME14, this);
	m_cOPLLPatchNameEdit15.SubclassDlgItem(IDC_OPLL_PATCHNAME15, this);
	m_cOPLLPatchNameEdit16.SubclassDlgItem(IDC_OPLL_PATCHNAME16, this);
	m_cOPLLPatchNameEdit17.SubclassDlgItem(IDC_OPLL_PATCHNAME17, this);
	m_cOPLLPatchNameEdit18.SubclassDlgItem(IDC_OPLL_PATCHNAME18, this);

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
	updateN163ChannelCountUI();

	// Vibrato 
	CComboBox *pVibratoBox = static_cast<CComboBox*>(GetDlgItem(IDC_VIBRATO));
	pVibratoBox->SetCurSel((m_pDocument->GetVibratoStyle() == VIBRATO_NEW) ? 0 : 1);
	
	// Pitch
	CComboBox *pPitchBox = static_cast<CComboBox*>(GetDlgItem(IDC_COMBO_LINEARPITCH));		// // //
	pPitchBox->SetCurSel(m_pDocument->GetLinearPitch() ? 1 : 0);
	
	// Level Offsets
	m_iAPU1LevelOffset = -m_pDocument->GetLevelOffset(0);
	m_iAPU2LevelOffset = -m_pDocument->GetLevelOffset(1);
	m_iVRC6LevelOffset = -m_pDocument->GetLevelOffset(2);
	m_iVRC7LevelOffset = -m_pDocument->GetLevelOffset(3);
	m_iFDSLevelOffset = -m_pDocument->GetLevelOffset(4);
	m_iMMC5LevelOffset = -m_pDocument->GetLevelOffset(5);
	m_iN163LevelOffset = -m_pDocument->GetLevelOffset(6);
	m_iS5BLevelOffset = -m_pDocument->GetLevelOffset(7);

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
		updateDeviceMixOffsetUI(i);

	// OPLL patch bytes and patch names
	m_bExternalOPLL = m_pDocument->GetExternalOPLLChipCheck();
	((CButton*)GetDlgItem(IDC_EXTERNAL_OPLL))->SetCheck(m_bExternalOPLL);

	for (int i = 0; i < 19; i++) {
		for (int j = 0; j < 8; j++)
			m_iOPLLPatchBytes[(8 * i) + j] = m_pDocument->GetOPLLPatchByte((8 * i) + j);
		m_strOPLLPatchNames[i] = m_pDocument->GetOPLLPatchName(i);
	}

	for (int i = 0; i < 19; i++)
		updateExternallOPLLUI(i);

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
	if (m_pDocument->GetLevelOffset(0) != m_iAPU1LevelOffset) m_pDocument->SetLevelOffset(0, -m_iAPU1LevelOffset);
	if (m_pDocument->GetLevelOffset(1) != m_iAPU2LevelOffset) m_pDocument->SetLevelOffset(1, -m_iAPU2LevelOffset);
	if (m_pDocument->GetLevelOffset(2) != m_iVRC6LevelOffset) m_pDocument->SetLevelOffset(2, -m_iVRC6LevelOffset);
	if (m_pDocument->GetLevelOffset(3) != m_iVRC7LevelOffset) m_pDocument->SetLevelOffset(3, -m_iVRC7LevelOffset);
	if (m_pDocument->GetLevelOffset(4) != m_iFDSLevelOffset) m_pDocument->SetLevelOffset(4, -m_iFDSLevelOffset);
	if (m_pDocument->GetLevelOffset(5) != m_iMMC5LevelOffset) m_pDocument->SetLevelOffset(5, -m_iMMC5LevelOffset);
	if (m_pDocument->GetLevelOffset(6) != m_iN163LevelOffset) m_pDocument->SetLevelOffset(6, -m_iN163LevelOffset);
	if (m_pDocument->GetLevelOffset(7) != m_iS5BLevelOffset) m_pDocument->SetLevelOffset(7, -m_iS5BLevelOffset);

	// Externall OPLL
	for (int i = 0; i < 19; i++) {
		for (int j = 0; j < 8; j++)
			m_pDocument->SetOPLLPatchByte((8 * i) + j, m_iOPLLPatchBytes[(8 * i) + j]);
		m_pDocument->SetOPLLPatchName(i, m_strOPLLPatchNames[i]);
	}
	m_pDocument->SetExternalOPLLChipCheck(m_bExternalOPLL);

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

	std::array<CWnd*, 2> N163ChannelcountUI = {&m_cChannelsLabel, &m_cChanSlider};

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
	for (auto *widget : N163ChannelcountUI)
		widget->EnableWindow(N163Enabled);
	
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

	std::array<CWnd*, 8> DeviceLevelEdit = {
		&m_cAPU1LevelEdit,
		&m_cAPU2LevelEdit,
		&m_cVRC6LevelEdit,
		&m_cVRC7LevelEdit,
		&m_cFDSLevelEdit,
		&m_cMMC5LevelEdit,
		&m_cN163LevelEdit,
		&m_cS5BLevelEdit
	};

	std::array<CWnd*, 8> DeviceLevelSlider = {
		&m_cAPU1LevelSlider,
		&m_cAPU2LevelSlider,
		&m_cVRC6LevelSlider,
		&m_cVRC7LevelSlider,
		&m_cFDSLevelSlider,
		&m_cMMC5LevelSlider,
		&m_cN163LevelSlider,
		&m_cS5BLevelSlider,
	};

	std::array<CWnd*, 8> DevicedBStatic = {
		&m_cAPU1dBLabel,
		&m_cAPU2dBLabel,
		&m_cVRC6dBLabel,
		&m_cVRC7dBLabel,
		&m_cFDSdBLabel,
		&m_cMMC5dBLabel,
		&m_cN163dBLabel,
		&m_cS5BdBLabel
	};

	std::array<CWnd*, 8> DeviceLevelStatic = {
		&m_cAPU1LevelLabel,
		&m_cAPU2LevelLabel,
		&m_cVRC6LevelLabel,
		&m_cVRC7LevelLabel,
		&m_cFDSLevelLabel,
		&m_cMMC5LevelLabel,
		&m_cN163LevelLabel,
		&m_cS5BLevelLabel
	};

	std::array<CWnd*, 4> DeviceMixOffsetUI = {
		DeviceLevelSlider[device],
		DeviceLevelEdit[device],
		DevicedBStatic[device],
		DeviceLevelStatic[device]
	};

	bool ChipEnabled = m_iExpansions & chipenable[device];

	// Always enable 2A03
	if (device <= 1)
		ChipEnabled = true;
	

	if (!ChipEnabled) {
		switch (device) {
		case 0: m_iAPU1LevelOffset = 0; break;
		case 1: m_iAPU2LevelOffset = 0; break;
		case 2: m_iVRC6LevelOffset = 0; break;
		case 3: m_iVRC7LevelOffset = 0; break;
		case 4: m_iFDSLevelOffset = 0; break;
		case 5: m_iMMC5LevelOffset = 0; break;
		case 6: m_iN163LevelOffset = 0; break;
		case 7: m_iS5BLevelOffset = 0; break;
		}
	}

	// Enable/disable UI.
	for (auto *widget : DeviceMixOffsetUI)
		widget->EnableWindow(ChipEnabled);

	if (device == 3)
		((CButton*)GetDlgItem(IDC_EXTERNAL_OPLL))->EnableWindow(ChipEnabled);

	// Redraw UI.
	switch (device) {
	case 0:
		m_cAPU1LevelSlider.SetPos((int)std::round(1.0 * m_iAPU1LevelOffset * COARSE_DELTA / FINE_DELTA));
		if (renderText) {
			CString LevelStr;
			strFromLevel(LevelStr, m_iAPU1LevelOffset);
			m_cAPU1LevelEdit.SetWindowTextNoNotify(LevelStr);
		}
		break;
	case 1:
		m_cAPU2LevelSlider.SetPos((int)std::round(1.0 * m_iAPU2LevelOffset * COARSE_DELTA / FINE_DELTA));
		if (renderText) {
			CString LevelStr;
			strFromLevel(LevelStr, m_iAPU2LevelOffset);
			m_cAPU2LevelEdit.SetWindowTextNoNotify(LevelStr);
		}
		break;
	case 2:
		m_cVRC6LevelSlider.SetPos((int)std::round(1.0 * m_iVRC6LevelOffset * COARSE_DELTA / FINE_DELTA));
		if (renderText) {
			CString LevelStr;
			strFromLevel(LevelStr, m_iVRC6LevelOffset);
			m_cVRC6LevelEdit.SetWindowTextNoNotify(LevelStr);
		}
		break;
	case 3:
		m_cVRC7LevelSlider.SetPos((int)std::round(1.0 * m_iVRC7LevelOffset * COARSE_DELTA / FINE_DELTA));
		if (renderText) {
			CString LevelStr;
			strFromLevel(LevelStr, m_iVRC7LevelOffset);
			m_cVRC7LevelEdit.SetWindowTextNoNotify(LevelStr);
		}
		break;
	case 4:
		m_cFDSLevelSlider.SetPos((int)std::round(1.0 * m_iFDSLevelOffset * COARSE_DELTA / FINE_DELTA));
		if (renderText) {
			CString LevelStr;
			strFromLevel(LevelStr, m_iFDSLevelOffset);
			m_cFDSLevelEdit.SetWindowTextNoNotify(LevelStr);
		}
		break;
	case 5:
		m_cMMC5LevelSlider.SetPos((int)std::round(1.0 * m_iMMC5LevelOffset * COARSE_DELTA / FINE_DELTA));
		if (renderText) {
			CString LevelStr;
			strFromLevel(LevelStr, m_iMMC5LevelOffset);
			m_cMMC5LevelEdit.SetWindowTextNoNotify(LevelStr);
		}
		break;
	case 6:
		m_cN163LevelSlider.SetPos((int)std::round(1.0 * m_iN163LevelOffset * COARSE_DELTA / FINE_DELTA));
		if (renderText) {
			CString LevelStr;
			strFromLevel(LevelStr, m_iN163LevelOffset);
			m_cN163LevelEdit.SetWindowTextNoNotify(LevelStr);
		}
		break;
	case 7:
		m_cS5BLevelSlider.SetPos((int)std::round(1.0 * m_iS5BLevelOffset * COARSE_DELTA / FINE_DELTA));
		if (renderText) {
			CString LevelStr;
			strFromLevel(LevelStr, m_iS5BLevelOffset);
			m_cS5BLevelEdit.SetWindowTextNoNotify(LevelStr);
		}
		break;
	}
}

// Level offset callbacks
void CModulePropertiesDlg::OffsetSlider(int device, int pos)
{
	switch (device) {
	case 0:
		m_iAPU1LevelOffset = (int16_t)std::round(1.0 * pos * FINE_DELTA / COARSE_DELTA);
		break;
	case 1:
		m_iAPU2LevelOffset = (int16_t)std::round(1.0 * pos * FINE_DELTA / COARSE_DELTA);
		break;
	case 2:
		m_iVRC6LevelOffset = (int16_t)std::round(1.0 * pos * FINE_DELTA / COARSE_DELTA);
		break;
	case 3:
		m_iVRC7LevelOffset = (int16_t)std::round(1.0 * pos * FINE_DELTA / COARSE_DELTA);
		break;
	case 4:
		m_iFDSLevelOffset = (int16_t)std::round(1.0 * pos * FINE_DELTA / COARSE_DELTA);
		break;
	case 5:
		m_iMMC5LevelOffset = (int16_t)std::round(1.0 * pos * FINE_DELTA / COARSE_DELTA);
		break;
	case 6:
		m_iN163LevelOffset = (int16_t)std::round(1.0 * pos * FINE_DELTA / COARSE_DELTA);
		break;
	case 7:
		m_iS5BLevelOffset = (int16_t)std::round(1.0 * pos * FINE_DELTA / COARSE_DELTA);
		break;
	}
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

void CModulePropertiesDlg::OnEnChangeApu1OffsetEdit()
{
	CString str;
	m_cAPU1LevelEdit.GetWindowText(str);
	if (levelFromStr(m_iAPU1LevelOffset, str)) {
		updateDeviceMixOffsetUI(0, false);
	}
}

void CModulePropertiesDlg::OnEnChangeApu2OffsetEdit()
{
	CString str;
	m_cAPU2LevelEdit.GetWindowText(str);
	if (levelFromStr(m_iAPU2LevelOffset, str)) {
		updateDeviceMixOffsetUI(1, false);
	}
}

void CModulePropertiesDlg::OnEnChangeVrc6OffsetEdit()
{
	CString str;
	m_cVRC6LevelEdit.GetWindowText(str);
	if (levelFromStr(m_iVRC6LevelOffset, str)) {
		updateDeviceMixOffsetUI(2, false);
	}
}

void CModulePropertiesDlg::OnEnChangeVrc7OffsetEdit()
{
	CString str;
	m_cVRC7LevelEdit.GetWindowText(str);
	if (levelFromStr(m_iVRC7LevelOffset, str)) {
		updateDeviceMixOffsetUI(3, false);
	}
}

void CModulePropertiesDlg::OnEnChangeFdsOffsetEdit()
{
	CString str;
	m_cFDSLevelEdit.GetWindowText(str);
	if (levelFromStr(m_iFDSLevelOffset, str)) {
		updateDeviceMixOffsetUI(4, false);
	}
}

void CModulePropertiesDlg::OnEnChangeMmc5OffsetEdit()
{
	CString str;
	m_cMMC5LevelEdit.GetWindowText(str);
	if (levelFromStr(m_iMMC5LevelOffset, str)) {
		updateDeviceMixOffsetUI(5, false);
	}
}

void CModulePropertiesDlg::OnEnChangeN163OffsetEdit()
{
	CString str;
	m_cN163LevelEdit.GetWindowText(str);
	if (levelFromStr(m_iN163LevelOffset, str)) {
		updateDeviceMixOffsetUI(6, false);
	}
}

void CModulePropertiesDlg::OnEnChangeS5bOffsetEdit()
{
	CString str;
	m_cS5BLevelEdit.GetWindowText(str);
	if (levelFromStr(m_iS5BLevelOffset, str)) {
		updateDeviceMixOffsetUI(7, false);
	}
}

// Externall OPLL UI
void CModulePropertiesDlg::updateExternallOPLLUI(int patchnum, bool renderText)
{
	std::array<NoNotifyEdit*, 19> OPLLPatchByteEdit = {
		&m_cOPLLPatchBytesEdit0,
		&m_cOPLLPatchBytesEdit1,
		&m_cOPLLPatchBytesEdit2,
		&m_cOPLLPatchBytesEdit3,
		&m_cOPLLPatchBytesEdit4,
		&m_cOPLLPatchBytesEdit5,
		&m_cOPLLPatchBytesEdit6,
		&m_cOPLLPatchBytesEdit7,
		&m_cOPLLPatchBytesEdit8,
		&m_cOPLLPatchBytesEdit9,
		&m_cOPLLPatchBytesEdit10,
		&m_cOPLLPatchBytesEdit11,
		&m_cOPLLPatchBytesEdit12,
		&m_cOPLLPatchBytesEdit13,
		&m_cOPLLPatchBytesEdit14,
		&m_cOPLLPatchBytesEdit15,
		&m_cOPLLPatchBytesEdit16,
		&m_cOPLLPatchBytesEdit17,
		&m_cOPLLPatchBytesEdit18
	};

	std::array<NoNotifyEdit*, 19> OPLLPatchNameEdit = {
		&m_cOPLLPatchNameEdit0,
		&m_cOPLLPatchNameEdit1,
		&m_cOPLLPatchNameEdit2,
		&m_cOPLLPatchNameEdit3,
		&m_cOPLLPatchNameEdit4,
		&m_cOPLLPatchNameEdit5,
		&m_cOPLLPatchNameEdit6,
		&m_cOPLLPatchNameEdit7,
		&m_cOPLLPatchNameEdit8,
		&m_cOPLLPatchNameEdit9,
		&m_cOPLLPatchNameEdit10,
		&m_cOPLLPatchNameEdit11,
		&m_cOPLLPatchNameEdit12,
		&m_cOPLLPatchNameEdit13,
		&m_cOPLLPatchNameEdit14,
		&m_cOPLLPatchNameEdit15,
		&m_cOPLLPatchNameEdit16,
		&m_cOPLLPatchNameEdit17,
		&m_cOPLLPatchNameEdit18
	};

	std::array<CWnd*, 19> OPLLPatchLabel = {
		&m_cOPLLPatchLabel0,
		&m_cOPLLPatchLabel1,
		&m_cOPLLPatchLabel2,
		&m_cOPLLPatchLabel3,
		&m_cOPLLPatchLabel4,
		&m_cOPLLPatchLabel5,
		&m_cOPLLPatchLabel6,
		&m_cOPLLPatchLabel7,
		&m_cOPLLPatchLabel8,
		&m_cOPLLPatchLabel9,
		&m_cOPLLPatchLabel10,
		&m_cOPLLPatchLabel11,
		&m_cOPLLPatchLabel12,
		&m_cOPLLPatchLabel13,
		&m_cOPLLPatchLabel14,
		&m_cOPLLPatchLabel15,
		&m_cOPLLPatchLabel16,
		&m_cOPLLPatchLabel17,
		&m_cOPLLPatchLabel18
	};

	// Enable/disable UI.
	for (auto* widget : OPLLPatchByteEdit)
		widget->EnableWindow(m_bExternalOPLL);
	for (auto* widget : OPLLPatchNameEdit)
		widget->EnableWindow(m_bExternalOPLL);
	for (auto* widget : OPLLPatchLabel)
		widget->EnableWindow(m_bExternalOPLL);

	// Redraw UI.
	// update patch names
	if (renderText) {
		uint8_t patchbytes[8]{};
		for (int i = 0; i < 8; i++)
			patchbytes[i] = m_iOPLLPatchBytes[(8 * patchnum) + i];
		OPLLPatchByteEdit[patchnum]->SetWindowTextNoNotify(PatchBytesToText(patchbytes));
		OPLLPatchNameEdit[patchnum]->SetWindowTextNoNotify(m_strOPLLPatchNames[patchnum].c_str());
	}
}

CString CModulePropertiesDlg::PatchBytesToText(uint8_t* patchbytes)
{
	CString patchtxt;

	for (int i = 0; i < 8; ++i)
		patchtxt.AppendFormat(_T("$%02X "), patchbytes[i]);

	return patchtxt;
}

uint8_t CModulePropertiesDlg::PatchTextToBytes(LPCTSTR pString, int index)
{
	std::string str(pString);
	uint8_t patchbytes[8]{};

	// Convert to register values
	std::istringstream values(str);
	std::istream_iterator<std::string> begin(values);
	std::istream_iterator<std::string> end;

	for (int i = 0; (i < 8) && (begin != end); ++i) {
		int value = CSequenceInstrumentEditPanel::ReadStringValue(*begin++, false);		// // //
		if (value < 0) value = 0;
		if (value > 0xFF) value = 0xFF;
		patchbytes[i] = value;
	}
	
	return patchbytes[index];
}

void CModulePropertiesDlg::OnBnClickedExternalOpll()
{
	m_bExternalOPLL = (((CButton*)GetDlgItem(IDC_EXTERNAL_OPLL))->GetCheck() == BST_CHECKED);

	for (int i = 0; i < 19; i++)
		updateExternallOPLLUI(i);
}

void CModulePropertiesDlg::OnEnKillfocusOpllPatchbyte1()
{
	CString str;
	m_cOPLLPatchBytesEdit1.GetWindowText(str);

	for (int i = 0; i < 8; i++)
		m_iOPLLPatchBytes[(8 * 1) + i] = PatchTextToBytes(str, i);

	updateExternallOPLLUI(1, false);
}

void CModulePropertiesDlg::OnEnChangeOpllPatchname1()
{
	CString str;
	m_cOPLLPatchNameEdit1.GetWindowText(str);
	m_strOPLLPatchNames[1] = str;
	updateExternallOPLLUI(1, false);
}

void CModulePropertiesDlg::OnEnKillfocusOpllPatchbyte2()
{
	CString str;
	m_cOPLLPatchBytesEdit2.GetWindowText(str);

	for (int i = 0; i < 8; i++)
		m_iOPLLPatchBytes[(8 * 2) + i] = PatchTextToBytes(str, i);

	updateExternallOPLLUI(2, false);
}

void CModulePropertiesDlg::OnEnChangeOpllPatchname2()
{
	CString str;
	m_cOPLLPatchNameEdit2.GetWindowText(str);
	m_strOPLLPatchNames[2] = str;
	updateExternallOPLLUI(2, false);
}

void CModulePropertiesDlg::OnEnKillfocusOpllPatchbyte3()
{
	CString str;
	m_cOPLLPatchBytesEdit3.GetWindowText(str);

	for (int i = 0; i < 8; i++)
		m_iOPLLPatchBytes[(8 * 3) + i] = PatchTextToBytes(str, i);

	updateExternallOPLLUI(3, false);
}

void CModulePropertiesDlg::OnEnChangeOpllPatchname3()
{
	CString str;
	m_cOPLLPatchNameEdit3.GetWindowText(str);
	m_strOPLLPatchNames[3] = str;
	updateExternallOPLLUI(3, false);
}

void CModulePropertiesDlg::OnEnKillfocusOpllPatchbyte4()
{
	CString str;
	m_cOPLLPatchBytesEdit4.GetWindowText(str);

	for (int i = 0; i < 8; i++)
		m_iOPLLPatchBytes[(8 * 4) + i] = PatchTextToBytes(str, i);

	updateExternallOPLLUI(4, false);
}

void CModulePropertiesDlg::OnEnChangeOpllPatchname4()
{
	CString str;
	m_cOPLLPatchNameEdit4.GetWindowText(str);
	m_strOPLLPatchNames[4] = str;
	updateExternallOPLLUI(4, false);
}

void CModulePropertiesDlg::OnEnKillfocusOpllPatchbyte5()
{
	CString str;
	m_cOPLLPatchBytesEdit5.GetWindowText(str);

	for (int i = 0; i < 8; i++)
		m_iOPLLPatchBytes[(8 * 5) + i] = PatchTextToBytes(str, i);

	updateExternallOPLLUI(5, false);
}

void CModulePropertiesDlg::OnEnChangeOpllPatchname5()
{
	CString str;
	m_cOPLLPatchNameEdit5.GetWindowText(str);
	m_strOPLLPatchNames[5] = str;
	updateExternallOPLLUI(5, false);
}

void CModulePropertiesDlg::OnEnKillfocusOpllPatchbyte6()
{
	CString str;
	m_cOPLLPatchBytesEdit6.GetWindowText(str);

	for (int i = 0; i < 8; i++)
		m_iOPLLPatchBytes[(8 * 6) + i] = PatchTextToBytes(str, i);

	updateExternallOPLLUI(6, false);
}

void CModulePropertiesDlg::OnEnChangeOpllPatchname6()
{
	CString str;
	m_cOPLLPatchNameEdit6.GetWindowText(str);
	m_strOPLLPatchNames[6] = str;
	updateExternallOPLLUI(6, false);
}

void CModulePropertiesDlg::OnEnKillfocusOpllPatchbyte7()
{
	CString str;
	m_cOPLLPatchBytesEdit7.GetWindowText(str);

	for (int i = 0; i < 8; i++)
		m_iOPLLPatchBytes[(8 * 7) + i] = PatchTextToBytes(str, i);

	updateExternallOPLLUI(7, false);
}

void CModulePropertiesDlg::OnEnChangeOpllPatchname7()
{
	CString str;
	m_cOPLLPatchNameEdit7.GetWindowText(str);
	m_strOPLLPatchNames[7] = str;
	updateExternallOPLLUI(7, false);
}

void CModulePropertiesDlg::OnEnKillfocusOpllPatchbyte8()
{
	CString str;
	m_cOPLLPatchBytesEdit8.GetWindowText(str);

	for (int i = 0; i < 8; i++)
		m_iOPLLPatchBytes[(8 * 8) + i] = PatchTextToBytes(str, i);

	updateExternallOPLLUI(8, false);
}

void CModulePropertiesDlg::OnEnChangeOpllPatchname8()
{
	CString str;
	m_cOPLLPatchNameEdit8.GetWindowText(str);
	m_strOPLLPatchNames[8] = str;
	updateExternallOPLLUI(8, false);
}

void CModulePropertiesDlg::OnEnKillfocusOpllPatchbyte9()
{
	CString str;
	m_cOPLLPatchBytesEdit9.GetWindowText(str);

	for (int i = 0; i < 8; i++)
		m_iOPLLPatchBytes[(8 * 9) + i] = PatchTextToBytes(str, i);

	updateExternallOPLLUI(9, false);
}

void CModulePropertiesDlg::OnEnChangeOpllPatchname9()
{
	CString str;
	m_cOPLLPatchNameEdit9.GetWindowText(str);
	m_strOPLLPatchNames[9] = str;
	updateExternallOPLLUI(9, false);
}

void CModulePropertiesDlg::OnEnKillfocusOpllPatchbyte10()
{
	CString str;
	m_cOPLLPatchBytesEdit10.GetWindowText(str);

	for (int i = 0; i < 8; i++)
		m_iOPLLPatchBytes[(8 * 10) + i] = PatchTextToBytes(str, i);

	updateExternallOPLLUI(10, false);
}

void CModulePropertiesDlg::OnEnChangeOpllPatchname10()
{
	CString str;
	m_cOPLLPatchNameEdit10.GetWindowText(str);
	m_strOPLLPatchNames[10] = str;
	updateExternallOPLLUI(10, false);
}

void CModulePropertiesDlg::OnEnKillfocusOpllPatchbyte11()
{
	CString str;
	m_cOPLLPatchBytesEdit11.GetWindowText(str);

	for (int i = 0; i < 8; i++)
		m_iOPLLPatchBytes[(8 * 11) + i] = PatchTextToBytes(str, i);

	updateExternallOPLLUI(11, false);
}

void CModulePropertiesDlg::OnEnChangeOpllPatchname11()
{
	CString str;
	m_cOPLLPatchNameEdit11.GetWindowText(str);
	m_strOPLLPatchNames[11] = str;
	updateExternallOPLLUI(11, false);
}

void CModulePropertiesDlg::OnEnKillfocusOpllPatchbyte12()
{
	CString str;
	m_cOPLLPatchBytesEdit12.GetWindowText(str);

	for (int i = 0; i < 8; i++)
		m_iOPLLPatchBytes[(8 * 12) + i] = PatchTextToBytes(str, i);

	updateExternallOPLLUI(12, false);
}

void CModulePropertiesDlg::OnEnChangeOpllPatchname12()
{
	CString str;
	m_cOPLLPatchNameEdit12.GetWindowText(str);
	m_strOPLLPatchNames[12] = str;
	updateExternallOPLLUI(12, false);
}

void CModulePropertiesDlg::OnEnKillfocusOpllPatchbyte13()
{
	CString str;
	m_cOPLLPatchBytesEdit13.GetWindowText(str);

	for (int i = 0; i < 8; i++)
		m_iOPLLPatchBytes[(8 * 13) + i] = PatchTextToBytes(str, i);

	updateExternallOPLLUI(13, false);
}

void CModulePropertiesDlg::OnEnChangeOpllPatchname13()
{
	CString str;
	m_cOPLLPatchNameEdit13.GetWindowText(str);
	m_strOPLLPatchNames[13] = str;
	updateExternallOPLLUI(13, false);
}

void CModulePropertiesDlg::OnEnKillfocusOpllPatchbyte14()
{
	CString str;
	m_cOPLLPatchBytesEdit14.GetWindowText(str);

	for (int i = 0; i < 8; i++)
		m_iOPLLPatchBytes[(8 * 14) + i] = PatchTextToBytes(str, i);

	updateExternallOPLLUI(14, false);
}

void CModulePropertiesDlg::OnEnChangeOpllPatchname14()
{
	CString str;
	m_cOPLLPatchNameEdit14.GetWindowText(str);
	m_strOPLLPatchNames[14] = str;
	updateExternallOPLLUI(14, false);
}

void CModulePropertiesDlg::OnEnKillfocusOpllPatchbyte15()
{
	CString str;
	m_cOPLLPatchBytesEdit15.GetWindowText(str);

	for (int i = 0; i < 8; i++)
		m_iOPLLPatchBytes[(8 * 15) + i] = PatchTextToBytes(str, i);

	updateExternallOPLLUI(15, false);
}

void CModulePropertiesDlg::OnEnChangeOpllPatchname15()
{
	CString str;
	m_cOPLLPatchNameEdit15.GetWindowText(str);
	m_strOPLLPatchNames[15] = str;
	updateExternallOPLLUI(15, false);
}

void CModulePropertiesDlg::OnEnKillfocusOpllPatchbyte16()
{
	CString str;
	m_cOPLLPatchBytesEdit16.GetWindowText(str);

	for (int i = 0; i < 8; i++)
		m_iOPLLPatchBytes[(8 * 16) + i] = PatchTextToBytes(str, i);

	updateExternallOPLLUI(16, false);
}

void CModulePropertiesDlg::OnEnChangeOpllPatchname16()
{
	CString str;
	m_cOPLLPatchNameEdit16.GetWindowText(str);
	m_strOPLLPatchNames[16] = str;
	updateExternallOPLLUI(16, false);
}

void CModulePropertiesDlg::OnEnKillfocusOpllPatchbyte17()
{
	CString str;
	m_cOPLLPatchBytesEdit17.GetWindowText(str);

	for (int i = 0; i < 8; i++)
		m_iOPLLPatchBytes[(8 * 17) + i] = PatchTextToBytes(str, i);

	updateExternallOPLLUI(17, false);
}

void CModulePropertiesDlg::OnEnChangeOpllPatchname17()
{
	CString str;
	m_cOPLLPatchNameEdit17.GetWindowText(str);
	m_strOPLLPatchNames[17] = str;
	updateExternallOPLLUI(17, false);
}

void CModulePropertiesDlg::OnEnKillfocusOpllPatchbyte18()
{
	CString str;
	m_cOPLLPatchBytesEdit18.GetWindowText(str);

	for (int i = 0; i < 8; i++)
		m_iOPLLPatchBytes[(8 * 18) + i] = PatchTextToBytes(str, i);

	updateExternallOPLLUI(18, false);
}

void CModulePropertiesDlg::OnEnChangeOpllPatchname18()
{
	CString str;
	m_cOPLLPatchNameEdit18.GetWindowText(str);
	m_strOPLLPatchNames[18] = str;
	updateExternallOPLLUI(18, false);
}
