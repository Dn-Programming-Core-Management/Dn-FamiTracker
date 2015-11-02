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
#include "FamiTrackerTypes.h"
#include "APU/Types.h"
#include "SoundGen.h"
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
	ON_BN_CLICKED(IDCANCEL, OnBnClickedCancel)
END_MESSAGE_MAP()


// CRecordSettingsDlg message handlers

BOOL CRecordSettingsDlg::OnInitDialog()
{
	m_stSetting = theApp.GetSoundGenerator()->GetRecordSetting();

	CSpinButtonCtrl *pSpin = static_cast<CSpinButtonCtrl*>(GetDlgItem(IDC_SPIN_RECORDER_INTERVAL));
	pSpin->SetRange(MIN_INTERVAL, MAX_SEQUENCE_ITEMS);
	pSpin = static_cast<CSpinButtonCtrl*>(GetDlgItem(IDC_SPIN_RECORDER_COUNT));
	pSpin->SetRange(1, MAX_INSTRUMENTS);

	CheckDlgButton(IDC_CHECK_RECORDER_RESET, m_stSetting.Reset ? BST_CHECKED : BST_UNCHECKED);

	CEdit *pEdit = static_cast<CEdit*>(GetDlgItem(IDC_EDIT_RECORDER_COUNT));
	pEdit->SetLimitText(2);
	SetDlgItemInt(IDC_EDIT_RECORDER_COUNT, m_stSetting.InstCount);
	pEdit = static_cast<CEdit*>(GetDlgItem(IDC_EDIT_RECORDER_INTERVAL));
	pEdit->SetLimitText(3);
	SetDlgItemInt(IDC_EDIT_RECORDER_INTERVAL, m_stSetting.Interval);
	pEdit->SetFocus();

	return CDialog::OnInitDialog();
}

stRecordSetting CRecordSettingsDlg::GetRecordSetting()
{
	CDialog::DoModal();
	return m_stSetting;
}

void CRecordSettingsDlg::OnEnChangeEditRecorderInterval()
{
	m_stSetting.Interval = GetDlgItemInt(IDC_EDIT_RECORDER_INTERVAL);
	if (m_stSetting.Interval < MIN_INTERVAL)
		m_stSetting.Interval = MIN_INTERVAL;
	if (m_stSetting.Interval > MAX_SEQUENCE_ITEMS)
		m_stSetting.Interval = MAX_SEQUENCE_ITEMS;
}

void CRecordSettingsDlg::OnEnChangeEditRecorderCount()
{
	m_stSetting.InstCount = GetDlgItemInt(IDC_EDIT_RECORDER_COUNT);
	if (m_stSetting.InstCount < 1)
		m_stSetting.InstCount = 1;
	if (m_stSetting.InstCount > MAX_INSTRUMENTS)
		m_stSetting.InstCount = MAX_INSTRUMENTS;
}

void CRecordSettingsDlg::OnBnClickedCheckRecorderReset()
{
	m_stSetting.Reset = IsDlgButtonChecked(IDC_CHECK_RECORDER_RESET) == BST_CHECKED;
}

void CRecordSettingsDlg::OnBnClickedCancel()
{
	m_stSetting.Interval = m_stSetting.InstCount = 0;
	CDialog::OnCancel();
}
