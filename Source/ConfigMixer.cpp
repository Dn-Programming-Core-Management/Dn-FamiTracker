/*
** FamiTracker - NES/Famicom sound tracker
** Copyright (C) 2005-2020 Jonathan Liss
**
** 0CC-FamiTracker is (C) 2014-2018 HertzDevil
**
** Dn-FamiTracker is (C) 2020-2022 D.P.C.M.
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
#include "ConfigMixer.h"
#include "Settings.h"


// CConfigLevels dialog

IMPLEMENT_DYNAMIC(CConfigMixer, CPropertyPage)
CConfigMixer::CConfigMixer()
	: CPropertyPage(CConfigMixer::IDD)
{
}

CConfigMixer::~CConfigMixer()
{
}

void CConfigMixer::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);

	DDX_Slider(pDX, IDC_SLIDER_APU1, m_iLevelAPU1);
	DDX_Slider(pDX, IDC_SLIDER_APU2, m_iLevelAPU2);
	DDX_Slider(pDX, IDC_SLIDER_VRC6, m_iLevelVRC6);
	DDX_Slider(pDX, IDC_SLIDER_VRC7, m_iLevelVRC7);
	DDX_Slider(pDX, IDC_SLIDER_MMC5, m_iLevelMMC5);
	DDX_Slider(pDX, IDC_SLIDER_FDS, m_iLevelFDS);
	DDX_Slider(pDX, IDC_SLIDER_N163, m_iLevelN163);
	DDX_Slider(pDX, IDC_SLIDER_S5B, m_iLevelS5B);

	UpdateLevels();
}

BEGIN_MESSAGE_MAP(CConfigMixer, CPropertyPage)
	ON_WM_VSCROLL()
	ON_BN_CLICKED(IDC_BUTTON_MIXER_RESET, &CConfigMixer::OnBnClickedButtonMixerReset)
END_MESSAGE_MAP()


// CConfigMixer message handlers

const int CConfigMixer::LEVEL_RANGE = 12;		// +/- 12 dB range
const int CConfigMixer::LEVEL_SCALE = 10;		// 0.1 dB resolution

BOOL CConfigMixer::OnInitDialog()
{
	const CSettings *pSettings = theApp.GetSettings();

	m_iLevelAPU1 = -pSettings->ChipLevels.iLevelAPU1;
	m_iLevelAPU2 = -pSettings->ChipLevels.iLevelAPU2;
	m_iLevelVRC6 = -pSettings->ChipLevels.iLevelVRC6;
	m_iLevelVRC7 = -pSettings->ChipLevels.iLevelVRC7;
	m_iLevelMMC5 = -pSettings->ChipLevels.iLevelMMC5;
	m_iLevelFDS = -pSettings->ChipLevels.iLevelFDS;
	m_iLevelN163 = -pSettings->ChipLevels.iLevelN163;
	m_iLevelS5B = -pSettings->ChipLevels.iLevelS5B;

	SetupSlider(IDC_SLIDER_APU1);
	SetupSlider(IDC_SLIDER_APU2);
	SetupSlider(IDC_SLIDER_VRC6);
	SetupSlider(IDC_SLIDER_VRC7);
	SetupSlider(IDC_SLIDER_MMC5);
	SetupSlider(IDC_SLIDER_FDS);
	SetupSlider(IDC_SLIDER_N163);
	SetupSlider(IDC_SLIDER_S5B);

	CPropertyPage::OnInitDialog();

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

BOOL CConfigMixer::OnApply()
{
	CSettings *pSettings = theApp.GetSettings();

	pSettings->ChipLevels.iLevelAPU1 = -m_iLevelAPU1;
	pSettings->ChipLevels.iLevelAPU2 = -m_iLevelAPU2;
	pSettings->ChipLevels.iLevelVRC6 = -m_iLevelVRC6;
	pSettings->ChipLevels.iLevelVRC7 = -m_iLevelVRC7;
	pSettings->ChipLevels.iLevelMMC5 = -m_iLevelMMC5;
	pSettings->ChipLevels.iLevelFDS = -m_iLevelFDS;
	pSettings->ChipLevels.iLevelN163 = -m_iLevelN163;
	pSettings->ChipLevels.iLevelS5B = -m_iLevelS5B;

	theApp.LoadSoundConfig();

	return CPropertyPage::OnApply();
}

void CConfigMixer::OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
	UpdateData(TRUE);
	SetModified();
	CPropertyPage::OnVScroll(nSBCode, nPos, pScrollBar);
}

void CConfigMixer::SetupSlider(int nID) const
{
	CSliderCtrl *pSlider = static_cast<CSliderCtrl*>(GetDlgItem(nID));
	pSlider->SetRange(-LEVEL_RANGE * LEVEL_SCALE, LEVEL_RANGE * LEVEL_SCALE);
	pSlider->SetTicFreq(LEVEL_SCALE * 2);
	pSlider->SetPageSize(5);
}

void CConfigMixer::UpdateLevels()
{
	UpdateLevel(IDC_LEVEL_APU1, m_iLevelAPU1);
	UpdateLevel(IDC_LEVEL_APU2, m_iLevelAPU2);
	UpdateLevel(IDC_LEVEL_VRC6, m_iLevelVRC6);
	UpdateLevel(IDC_LEVEL_VRC7, m_iLevelVRC7);
	UpdateLevel(IDC_LEVEL_MMC5, m_iLevelMMC5);
	UpdateLevel(IDC_LEVEL_FDS, m_iLevelFDS);
	UpdateLevel(IDC_LEVEL_N163, m_iLevelN163);
	UpdateLevel(IDC_LEVEL_S5B, m_iLevelS5B);
}

void CConfigMixer::UpdateLevel(int nID, int Level)
{
	CString str, str2;
	str.Format(_T("%+.1f dB"), float(-Level) / float(LEVEL_SCALE));
	GetDlgItemText(nID, str2);
	if (str.Compare(str2) != 0)
		SetDlgItemText(nID, str);
}

void CConfigMixer::OnBnClickedButtonMixerReset()		// // // 050B
{
	m_iLevelAPU1 = 0;
	m_iLevelAPU2 = 0;
	m_iLevelVRC6 = 0;
	m_iLevelVRC7 = 0;
	m_iLevelMMC5 = 0;
	m_iLevelFDS = 0;
	m_iLevelN163 = 0;
	m_iLevelS5B = 0;

	UpdateData(FALSE);
	SetModified();
}
