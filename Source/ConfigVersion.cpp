/*
** FamiTracker - NES/Famicom sound tracker
** Copyright (C) 2005-2014  Jonathan Liss
**
** 0CC-FamiTracker is (C) 2014-2015 HertzDevil
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

#include "stdafx.h"
#include "FamiTracker.h"
#include "ConfigVersion.h"
#include "FamiTrackerTypes.h"


const CString CConfigVersion::VERSION_TEXT[] = {
	_T("FamiTracker 0.2.2"),
	_T("FamiTracker 0.2.4"),
	_T("FamiTracker 0.2.5"),
	_T("FamiTracker 0.2.6"),
	_T("FamiTracker 0.2.7"),
	_T("FamiTracker 0.2.9 - 0.3.0"),
	_T("FamiTracker 0.3.0 re-release"),
	_T("FamiTracker 0.3.5 beta 0"),
	_T("FamiTracker 0.3.5"),
	_T("FamiTracker 0.3.6 beta 1 - 0.3.6 beta 3"),
	_T("FamiTracker 0.3.6 beta 4 - 0.3.6"),
	_T("FamiTracker 0.3.7 beta 0"),
	_T("FamiTracker 0.3.7"),
	_T("FamiTracker 0.3.8 beta 0"),
	_T("FamiTracker 0.3.8 beta 1 - 0.3.8 beta 5"),
	_T("FamiTracker 0.4.0 - 0.4.1"),
	_T("FamiTracker 0.4.2 beta 1 - 0.4.6"),
	_T("0CC-FamiTracker 0.1.0 - 0.1.1"),
	_T("0CC-FamiTracker 0.2.0 - 0.3.1"),
	_T("0CC-FamiTracker 0.3.2 - 0.3.3"),		// DETUNETABLES
	_T("0CC-FamiTracker 0.3.4 - 0.3.7"),		// GROOVES
	_T("0CC-FamiTracker 0.3.8"),
	_T("0CC-FamiTracker 0.3.9"),
	_T("0CC-FamiTracker 0.3.10"),
	_T("0CC-FamiTracker 0.3.11"),				// BOOKMARKS
	_T("Current version"),
};

const effect_t CConfigVersion::MAX_EFFECT_INDEX[] = {
	EF_SWEEPDOWN,			// 0.2.2
	EF_TREMOLO,				// 0.2.4
	EF_PITCH,				// 0.2.5
	EF_DAC,					// 0.2.6
	EF_SAMPLE_OFFSET,		// 0.2.7
	EF_VOLUME_SLIDE,		// 0.2.9
	EF_VOLUME_SLIDE,
	EF_RETRIGGER,			// 0.3.5b0
	EF_RETRIGGER,
	EF_RETRIGGER,
	EF_RETRIGGER,
	EF_FDS_MOD_SPEED_LO,	// 0.3.7b0
	EF_DPCM_PITCH,			// 0.3.7
	EF_DPCM_PITCH,
	EF_DPCM_PITCH,
	EF_DPCM_PITCH,
	EF_SUNSOFT_ENV_TYPE,	// 0.4.2 (actually 0.4.1 already had 5B effects)
	EF_SUNSOFT_ENV_TYPE,
	EF_NOTE_RELEASE,		// 0CC 0.2.0
	EF_NOTE_RELEASE,
	EF_GROOVE,				// 0CC 0.3.4
	EF_TRANSPOSE,			// 0CC 0.3.8
	EF_N163_WAVE_BUFFER,	// 0CC 0.3.9
	EF_FDS_VOLUME,			// 0CC 0.3.10
	EF_FDS_VOLUME,
	EF_FDS_MOD_BIAS,		// 0CC 0.3.12
};

const stVerInfo CConfigVersion::VERSION_INFO[] = {
	{_T("FamiTracker 0.2.2"),                 EF_SWEEPDOWN,     0x0200, 0, {1, 1, 0, 1, 1, 1, 1, 1}},
	{_T("FamiTracker 0.2.4"),                 EF_TREMOLO,       0x0201, 0, {1, 1, 1, 1, 1, 1, 1, 1}},
	{_T("FamiTracker 0.2.5"),                 EF_PITCH,         0x0203, 0, {1, 1, 1, 1, 2, 1, 1, 1}},
	{_T("FamiTracker 0.2.6"),                 EF_DAC,           0x0203, 0, {2, 1, 2, 2, 2, 2, 2, 1}},
	{_T("FamiTracker 0.2.7"),                 EF_SAMPLE_OFFSET, 0x0300, 0, {2, 1, 2, 2, 3, 3, 3, 1}},
	{_T("FamiTracker 0.2.9 - 0.3.0"),         EF_VOLUME_SLIDE,  0x0300, 0, {2, 1, 2, 2, 3, 3, 3, 1, 1}},
	{_T("FamiTracker 0.3.0 re-release"),      EF_VOLUME_SLIDE,  0x0410, 1, {2, 1, 2, 2, 4, 3, 3, 1, 1}},
	{_T("FamiTracker 0.3.5 beta 0"),          EF_RETRIGGER,     0x0420, 0, {3, 1, 2, 2, 4, 3, 3, 1, 1}},
	{_T("FamiTracker 0.3.5"),                 EF_RETRIGGER,     0x0420, 0, {3, 1, 2, 2, 5, 3, 3, 1, 5}},
};

// CConfigVersion dialog

IMPLEMENT_DYNAMIC(CConfigVersion, CPropertyPage)
CConfigVersion::CConfigVersion()
	: CPropertyPage(CConfigVersion::IDD)
{
}

CConfigVersion::~CConfigVersion()
{
}

void CConfigVersion::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CConfigVersion, CPropertyPage)
END_MESSAGE_MAP()


// CConfigVersion message handlers

BOOL CConfigVersion::OnInitDialog()
{
	CPropertyPage::OnInitDialog();

	m_cComboVersion = new CComboBox();
	m_cComboVersion->SubclassDlgItem(IDC_COMBO_VERSION_SELECT, this);

	for (size_t i = 0; i < sizeof(VERSION_TEXT) / sizeof(CString); i++)
		m_cComboVersion->AddString(VERSION_TEXT[i]);
	m_cComboVersion->SetCurSel(m_cComboVersion->GetCount() - 1); // TODO: add to registry

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}
