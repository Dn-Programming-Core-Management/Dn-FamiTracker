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
#include "APU/Types.h"
#include "SoundGen.h"
#include "InstrumentRecorder.h"
#include "RecordSettingsDlg.h"

#define MIN_INTERVAL 24

// CRecordSettingsDlg dialog

IMPLEMENT_DYNAMIC(CRecordSettingsDlg, CDialog)

CRecordSettingsDlg::CRecordSettingsDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CRecordSettingsDlg::IDD, pParent)
{

}

CRecordSettingsDlg::~CRecordSettingsDlg()
{
}

void CRecordSettingsDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CRecordSettingsDlg, CDialog)
	ON_EN_CHANGE(IDC_EDIT_RECORDER_INTERVAL, OnEnChangeEditRecorderInterval)
	ON_EN_CHANGE(IDC_EDIT_RECORDER_COUNT, OnEnChangeEditRecorderCount)
	ON_BN_CLICKED(IDC_CHECK_RECORDER_RESET, OnBnClickedCheckRecorderReset)
	ON_EN_KILLFOCUS(IDC_EDIT_RECORDER_INTERVAL, &CRecordSettingsDlg::OnEnKillfocusEditRecorderInterval)
	ON_EN_KILLFOCUS(IDC_EDIT_RECORDER_COUNT, &CRecordSettingsDlg::OnEnKillfocusEditRecorderCount)
END_MESSAGE_MAP()


// CRecordSettingsDlg message handlers

BOOL CRecordSettingsDlg::OnInitDialog()
{
	stRecordSetting *Setting = theApp.GetSoundGenerator()->GetRecordSetting();

	CSpinButtonCtrl *pSpin = static_cast<CSpinButtonCtrl*>(GetDlgItem(IDC_SPIN_RECORDER_INTERVAL));
	pSpin->SetRange(MIN_INTERVAL, MAX_SEQUENCE_ITEMS);
	pSpin = static_cast<CSpinButtonCtrl*>(GetDlgItem(IDC_SPIN_RECORDER_COUNT));
	pSpin->SetRange(1, MAX_INSTRUMENTS);

	CheckDlgButton(IDC_CHECK_RECORDER_RESET, Setting->Reset ? BST_CHECKED : BST_UNCHECKED);

	CEdit *pEdit = static_cast<CEdit*>(GetDlgItem(IDC_EDIT_RECORDER_COUNT));
	pEdit->SetLimitText(2);
	SetDlgItemInt(IDC_EDIT_RECORDER_COUNT, Setting->InstCount);
	pEdit = static_cast<CEdit*>(GetDlgItem(IDC_EDIT_RECORDER_INTERVAL));
	pEdit->SetLimitText(3);
	SetDlgItemInt(IDC_EDIT_RECORDER_INTERVAL, Setting->Interval);
	pEdit->SetFocus();

	delete Setting;

	return CDialog::OnInitDialog();
}

stRecordSetting *CRecordSettingsDlg::GetRecordSetting()
{
	return new stRecordSetting {m_iInterval, m_iCount, m_bReset};
}

void CRecordSettingsDlg::OnEnChangeEditRecorderInterval()
{
	m_iInterval = GetDlgItemInt(IDC_EDIT_RECORDER_INTERVAL);
}

void CRecordSettingsDlg::OnEnChangeEditRecorderCount()
{
	m_iCount = GetDlgItemInt(IDC_EDIT_RECORDER_COUNT);
}

void CRecordSettingsDlg::OnBnClickedCheckRecorderReset()
{
	m_bReset = IsDlgButtonChecked(IDC_CHECK_RECORDER_RESET) == BST_CHECKED;
}


void CRecordSettingsDlg::OnEnKillfocusEditRecorderInterval()
{
	if (m_iInterval < MIN_INTERVAL)
		SetDlgItemInt(IDC_EDIT_RECORDER_INTERVAL, MIN_INTERVAL);
	if (m_iInterval > MAX_SEQUENCE_ITEMS)
		SetDlgItemInt(IDC_EDIT_RECORDER_INTERVAL, MAX_SEQUENCE_ITEMS);
}


void CRecordSettingsDlg::OnEnKillfocusEditRecorderCount()
{
	if (m_iCount < 1)
		SetDlgItemInt(IDC_EDIT_RECORDER_COUNT, 1);
	if (m_iCount > MAX_INSTRUMENTS)
		SetDlgItemInt(IDC_EDIT_RECORDER_COUNT, MAX_INSTRUMENTS);
}
