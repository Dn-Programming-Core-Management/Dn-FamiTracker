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
