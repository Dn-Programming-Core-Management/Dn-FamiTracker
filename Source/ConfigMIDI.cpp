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
#include "ConfigMIDI.h"
#include "MIDI.h"
#include "Settings.h"


// CConfigMIDI dialog

IMPLEMENT_DYNAMIC(CConfigMIDI, CPropertyPage)
CConfigMIDI::CConfigMIDI()
	: CPropertyPage(CConfigMIDI::IDD)
{
}

CConfigMIDI::~CConfigMIDI()
{
}

void CConfigMIDI::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CConfigMIDI, CPropertyPage)
	ON_CBN_SELCHANGE(IDC_INDEVICES, OnCbnSelchangeDevices)
	ON_BN_CLICKED(IDC_MASTER_SYNC, OnBnClickedMasterSync)
	ON_BN_CLICKED(IDC_KEY_RELEASE, OnBnClickedKeyRelease)
	ON_BN_CLICKED(IDC_CHANMAP, OnBnClickedChanmap)
	ON_BN_CLICKED(IDC_VELOCITY, OnBnClickedVelocity)
	ON_BN_CLICKED(IDC_ARPEGGIATE, OnBnClickedArpeggiate)
	ON_CBN_SELCHANGE(IDC_OUTDEVICES, OnCbnSelchangeOutdevices)
END_MESSAGE_MAP()


// CConfigMIDI message handlers

BOOL CConfigMIDI::OnInitDialog()
{
	CPropertyPage::OnInitDialog();

	CMIDI	*pMIDI = theApp.GetMIDI();
	int		NumDev, i;
	CString Text;

	CComboBox *pInDevices = static_cast<CComboBox*>(GetDlgItem(IDC_INDEVICES));
	CComboBox *pOutDevices = static_cast<CComboBox*>(GetDlgItem(IDC_OUTDEVICES));

	pInDevices->AddString(_T("<none>"));
	pOutDevices->AddString(_T("<none>"));

	// Input
	NumDev = pMIDI->GetNumInputDevices();

	for (i = 0; i < NumDev; ++i) {
		pMIDI->GetInputDeviceString(i, Text);
		pInDevices->AddString(Text);
	}

	// Output
	NumDev = pMIDI->GetNumOutputDevices();

	for (i = 0; i < NumDev; ++i) {
		pMIDI->GetOutputDeviceString(i, Text);
		pOutDevices->AddString(Text);
	}

	pInDevices->SetCurSel(pMIDI->GetInputDevice());
	pOutDevices->SetCurSel(pMIDI->GetOutputDevice());

	CheckDlgButton(IDC_MASTER_SYNC, theApp.GetSettings()->Midi.bMidiMasterSync	? 1 : 0);
	CheckDlgButton(IDC_KEY_RELEASE, theApp.GetSettings()->Midi.bMidiKeyRelease	? 1 : 0);
	CheckDlgButton(IDC_CHANMAP,		theApp.GetSettings()->Midi.bMidiChannelMap	? 1 : 0);
	CheckDlgButton(IDC_VELOCITY,	theApp.GetSettings()->Midi.bMidiVelocity	? 1 : 0);
	CheckDlgButton(IDC_ARPEGGIATE,	theApp.GetSettings()->Midi.bMidiArpeggio	? 1 : 0);

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

BOOL CConfigMIDI::OnApply()
{
	CComboBox	*pInDevices	 = static_cast<CComboBox*>(GetDlgItem(IDC_INDEVICES));
	CComboBox	*pOutDevices = static_cast<CComboBox*>(GetDlgItem(IDC_OUTDEVICES));
	CMIDI		*pMIDI		 = theApp.GetMIDI();
	
	pMIDI->SetInputDevice(pInDevices->GetCurSel(), IsDlgButtonChecked(IDC_MASTER_SYNC) != 0);
	pMIDI->SetOutputDevice(pOutDevices->GetCurSel());
	
	theApp.GetSettings()->Midi.bMidiMasterSync	= IsDlgButtonChecked(IDC_MASTER_SYNC)	== 1;
	theApp.GetSettings()->Midi.bMidiKeyRelease	= IsDlgButtonChecked(IDC_KEY_RELEASE)	== 1;
	theApp.GetSettings()->Midi.bMidiChannelMap	= IsDlgButtonChecked(IDC_CHANMAP)		== 1;
	theApp.GetSettings()->Midi.bMidiVelocity	= IsDlgButtonChecked(IDC_VELOCITY)		== 1;
	theApp.GetSettings()->Midi.bMidiArpeggio	= IsDlgButtonChecked(IDC_ARPEGGIATE)	== 1;

	return CPropertyPage::OnApply();
}

void CConfigMIDI::OnCbnSelchangeDevices()
{
	SetModified();
}

void CConfigMIDI::OnBnClickedMasterSync()
{
	SetModified();
}

void CConfigMIDI::OnBnClickedKeyRelease()
{
	SetModified();
}

void CConfigMIDI::OnBnClickedChanmap()
{
	SetModified();
}

void CConfigMIDI::OnBnClickedVelocity()
{
	SetModified();
}

void CConfigMIDI::OnBnClickedArpeggiate()
{
	SetModified();
}

void CConfigMIDI::OnCbnSelchangeOutdevices()
{
	SetModified();
}
