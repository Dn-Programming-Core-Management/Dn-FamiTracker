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

#pragma once


// CSwapDlg dialog

class CSwapDlg : public CDialog
{
	DECLARE_DYNAMIC(CSwapDlg)

public:
	CSwapDlg(CWnd* pParent = NULL);   // standard constructor
	void SetTrack(unsigned int Track);
	virtual ~CSwapDlg();

// Dialog Data
	enum { IDD = IDD_SWAP };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	
	void CheckDestination() const;
	static int GetChipFromString(const CString str);
	int GetFinalChannel(unsigned int Channel, unsigned int Chip) const;
	
	unsigned int m_iDestChannel1, m_iDestChannel2;
	unsigned int m_iDestChip1, m_iDestChip2;
	unsigned int m_iTrack;
	
	CEdit *m_cChannelFirst, *m_cChannelSecond;
	CComboBox *m_cChipFirst, *m_cChipSecond;

	DECLARE_MESSAGE_MAP()
public:
	virtual BOOL OnInitDialog();
	afx_msg void OnEnChangeEditSwapChan1();
	afx_msg void OnEnChangeEditSwapChan2();
	afx_msg void OnCbnSelchangeComboSwapChip1();
	afx_msg void OnCbnSelchangeComboSwapChip2();
	afx_msg void OnBnClickedOk();
};
