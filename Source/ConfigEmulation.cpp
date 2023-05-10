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
	ON_EN_KILLFOCUS(IDC_EDIT_LOWPASS_FDS, &CConfigEmulation::OnEnKillfocusEditLowpassFDS)
	ON_EN_KILLFOCUS(IDC_EDIT_LOWPASS_N163, &CConfigEmulation::OnEnKillfocusEditLowpassN163)
	ON_EN_CHANGE(IDC_EDIT_LOWPASS_FDS, &CConfigEmulation::OnEnChangeEditLowpassFDS)
	ON_EN_CHANGE(IDC_EDIT_LOWPASS_N163, &CConfigEmulation::OnEnChangeEditLowpassN163)
	ON_NOTIFY(UDN_DELTAPOS, IDC_SPIN_LOWPASS_FDS, &CConfigEmulation::OnDeltaposSpinLowpassFDS)
	ON_NOTIFY(UDN_DELTAPOS, IDC_SPIN_LOWPASS_N163, &CConfigEmulation::OnDeltaposSpinLowpassN163)
END_MESSAGE_MAP()


// CConfigEmulation message handlers

BOOL CConfigEmulation::OnInitDialog()
{
	CString str;
	CSettings* pSettings = theApp.GetSettings();

	// FDS
	str.Format("%i", pSettings->Emulation.iFDSLowpass);
	SetDlgItemText(IDC_EDIT_LOWPASS_FDS, str);
	CSliderCtrl* pFDSLowpass = static_cast<CSliderCtrl*>(GetDlgItem(IDC_SLIDER_LOWPASS_FDS));
	pFDSLowpass->SetRange(0, 8000);
	pFDSLowpass->SetPos(_ttoi(str));

	// N163
	str.Format("%i", pSettings->Emulation.iN163Lowpass);
	SetDlgItemText(IDC_EDIT_LOWPASS_N163, str);
	CSliderCtrl* pN163Lowpass = static_cast<CSliderCtrl*>(GetDlgItem(IDC_SLIDER_LOWPASS_N163));
	pN163Lowpass->SetRange(0, 12000);
	pN163Lowpass->SetPos(_ttoi(str));
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

	CPropertyPage::OnInitDialog();
	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

BOOL CConfigEmulation::OnApply()
{
	CSettings* pSettings = theApp.GetSettings();
	CSoundGen* pSoundGen = theApp.GetSoundGenerator();

	// FDS
	CString str;
	GetDlgItemText(IDC_EDIT_LOWPASS_FDS, str);
	pSettings->Emulation.iFDSLowpass = _ttoi(str);

	// N163
	GetDlgItemText(IDC_EDIT_LOWPASS_N163, str);
	pSettings->Emulation.iN163Lowpass = _ttoi(str);
	pSettings->Emulation.bNamcoMixing = m_bDisableNamcoMultiplex;

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
	str.Format(_T("%i"), static_cast<CSliderCtrl*>(GetDlgItem(IDC_SLIDER_LOWPASS_FDS))->GetPos());
	SetDlgItemText(IDC_EDIT_LOWPASS_FDS, str);
	str.Format(_T("%i"), static_cast<CSliderCtrl*>(GetDlgItem(IDC_SLIDER_LOWPASS_N163))->GetPos());
	SetDlgItemText(IDC_EDIT_LOWPASS_N163, str);
}

void CConfigEmulation::OnCbnSelchangeComboVrc7Patch()
{
	SetModified();
}

void CConfigEmulation::OnEnKillfocusEditLowpassFDS()
{
	SetModified();
}

void CConfigEmulation::OnEnKillfocusEditLowpassN163()
{
	SetModified();
}

void CConfigEmulation::OnEnChangeEditLowpassFDS()
{
	CString str;
	GetDlgItemText(IDC_EDIT_LOWPASS_FDS, str);
	int pos = _ttoi(str);

	if (pos < 0) pos = 0;
	if (pos > 8000) pos = 8000;

	CSliderCtrl* pIdleRefSlider = static_cast<CSliderCtrl*>(GetDlgItem(IDC_SLIDER_LOWPASS_FDS));
	pIdleRefSlider->SetPos(pos);

	SetModified();
}

void CConfigEmulation::OnEnChangeEditLowpassN163()
{
	CString str;
	GetDlgItemText(IDC_EDIT_LOWPASS_N163, str);
	int pos = _ttoi(str);

	if (pos < 0) pos = 0;
	if (pos > 12000) pos = 12000;

	CSliderCtrl* pIdleRefSlider = static_cast<CSliderCtrl*>(GetDlgItem(IDC_SLIDER_LOWPASS_N163));
	pIdleRefSlider->SetPos(pos);

	SetModified();
}

void CConfigEmulation::OnDeltaposSpinLowpassFDS(NMHDR* pNMHDR, LRESULT* pResult)
{
	CString str;
	GetDlgItemText(IDC_EDIT_LOWPASS_FDS, str);

	int freq = _ttoi(str);
	int newfreq = freq - ((NMUPDOWN*)pNMHDR)->iDelta;

	str.Format(_T("%i"), newfreq);
	SetDlgItemText(IDC_EDIT_LOWPASS_FDS, str);

	SetModified();
}

void CConfigEmulation::OnDeltaposSpinLowpassN163(NMHDR* pNMHDR, LRESULT* pResult)
{
	CString str;
	GetDlgItemText(IDC_EDIT_LOWPASS_N163, str);

	int freq = _ttoi(str);
	int newfreq = freq - ((NMUPDOWN*)pNMHDR)->iDelta;

	str.Format(_T("%i"), newfreq);
	SetDlgItemText(IDC_EDIT_LOWPASS_N163, str);

	SetModified();
}
