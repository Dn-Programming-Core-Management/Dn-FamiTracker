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
#include "FamiTrackerTypes.h"
#include "SoundGen.h"
#include "ConfigEmulation.h"
#include "Settings.h"
#include "MainFrm.h"



// CConfigEmulation dialog

IMPLEMENT_DYNAMIC(CConfigEmulation, CPropertyPage)
CConfigEmulation::CConfigEmulation()
	: CPropertyPage(CConfigEmulation::IDD)
{
}

CConfigEmulation::~CConfigEmulation()
{
}

void CConfigEmulation::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CConfigEmulation, CPropertyPage)
	ON_WM_HSCROLL()
	ON_BN_CLICKED(IDC_N163_MULTIPLEXER, &CConfigEmulation::OnBnClickedN163Multiplexer)
	ON_CBN_SELCHANGE(IDC_COMBO_VRC7_PATCH, &CConfigEmulation::OnCbnSelchangeComboVrc7Patch)
END_MESSAGE_MAP()


// CConfigEmulation message handlers

BOOL CConfigEmulation::OnInitDialog()
{
	CSettings* pSettings = theApp.GetSettings();

	// FDS
	CSliderCtrl* pFDSLowpass = static_cast<CSliderCtrl*>(GetDlgItem(IDC_SLIDER_FDS_LOWPASS));
	pFDSLowpass->SetRange(0, 8000);
	pFDSLowpass->SetPos(pSettings->Emulation.iFDSLowpass);

	// N163
	m_bDisableNamcoMultiplex = pSettings->Emulation.bNamcoMixing;
	CheckDlgButton(IDC_N163_MULTIPLEXER, pSettings->Emulation.bNamcoMixing);

	// VRC7
	CComboBox* pVRC7Patch = static_cast<CComboBox*>(GetDlgItem(IDC_COMBO_VRC7_PATCH));
	pVRC7Patch->AddString("j0CC-FT 0.6.2 by Nuke.YTK (3/20/2019)");
	pVRC7Patch->AddString("FT 0.4.0 by rainwarrior (8/01/2012)");
	pVRC7Patch->AddString("FT 0.3.6 by quietust(1/18/2004)");
	pVRC7Patch->AddString("FT 0.3.5 by Mitsutaka Okazaki (6/24/2001)");
	pVRC7Patch->AddString("VRC7 TONES by okazaki@angel.ne.jp (4/10/2004)");
	pVRC7Patch->AddString("Patch set 2 by kevtris (11/15/1999)");
	pVRC7Patch->AddString("Patch set 1 by kevtris (11/14/1999)");
	pVRC7Patch->AddString("2413 Tone");
	pVRC7Patch->AddString("281B Tone");
	pVRC7Patch->SetCurSel(pSettings->Emulation.iVRC7Patch);

	UpdateSliderTexts();
	CPropertyPage::OnInitDialog();
	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

BOOL CConfigEmulation::OnApply()
{
	CSettings* pSettings = theApp.GetSettings();
	CSoundGen* pSoundGen = theApp.GetSoundGenerator();

	// FDS
	CSliderCtrl* pFDSLowpass = static_cast<CSliderCtrl*>(GetDlgItem(IDC_SLIDER_FDS_LOWPASS));
	pSettings->Emulation.iFDSLowpass = pFDSLowpass->GetPos();

	// N163
	pSettings->Emulation.bNamcoMixing = m_bDisableNamcoMultiplex;
	pSoundGen->SetNamcoMixing(theApp.GetSettings()->Emulation.bNamcoMixing);

	// VRC7
	CComboBox* pVRC7Patch = static_cast<CComboBox*>(GetDlgItem(IDC_COMBO_VRC7_PATCH));
	pSettings->Emulation.iVRC7Patch = pVRC7Patch->GetCurSel();
	
	theApp.LoadSoundConfig();

	return CPropertyPage::OnApply();
}

void CConfigEmulation::OnBnClickedN163Multiplexer()
{
	m_bDisableNamcoMultiplex = IsDlgButtonChecked(IDC_N163_MULTIPLEXER) != 0;
	SetModified();
}

void CConfigEmulation::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
	UpdateSliderTexts();
	SetModified();
	CPropertyPage::OnHScroll(nSBCode, nPos, pScrollBar);
}

void CConfigEmulation::UpdateSliderTexts()
{
	CString str;
	str.Format(_T("%i Hz"), static_cast<CSliderCtrl*>(GetDlgItem(IDC_SLIDER_FDS_LOWPASS))->GetPos());
	SetDlgItemText(IDC_FDS_LOWPASS_FREQ, str);
}


void CConfigEmulation::OnCbnSelchangeComboVrc7Patch()
{
	SetModified();
}
