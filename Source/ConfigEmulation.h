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


#pragma once

// CConfigEmulation dialog

class CConfigEmulation : public CPropertyPage
{
	DECLARE_DYNAMIC(CConfigEmulation)

public:
	CConfigEmulation();   // standard constructor
	virtual ~CConfigEmulation();

// Dialog Data
	enum { IDD = IDD_CONFIG_EMULATION };

private:

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	// N163
	bool	m_bDisableNamcoMultiplex;

	void UpdateSliderTexts();

	DECLARE_MESSAGE_MAP()
public:
	virtual BOOL OnInitDialog();
	virtual BOOL OnApply();
	afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	//afx_msg void OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	//afx_msg BOOL OnMouseWheel(UINT nFlags, short zDelta, CPoint pt);
	afx_msg void OnBnClickedN163Multiplexer();
	afx_msg void OnCbnSelchangeComboFdsEmulator();
	afx_msg void OnCbnSelchangeComboS5bEmulator();
	afx_msg void OnCbnSelchangeComboVrc7Patch();
};
