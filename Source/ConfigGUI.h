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

// CConfigGUI dialog

class CConfigGUI : public CPropertyPage
{
	DECLARE_DYNAMIC(CConfigGUI)

public:
	CConfigGUI();   // standard constructor
	virtual ~CConfigGUI();

	// Dialog Data
	enum { IDD = IDD_CONFIG_GUI };

private:

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	int		m_iLowRefreshRate;
	int		m_iMaxChannelView;
	bool	m_bSmoothRegFreq;

	void UpdateSliderTexts();

	DECLARE_MESSAGE_MAP()
public:
	virtual BOOL OnInitDialog();
	virtual BOOL OnApply();
	afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg void OnBnClickedSmoothregfreqCheck();
	afx_msg void OnEnKillfocusMaxchanviewEdit();
	afx_msg void OnEnKillfocusIdleRefreshEdit();
	afx_msg void OnEnChangeMaxchanviewEdit();
	afx_msg void OnEnChangeIdleRefreshEdit();
};
