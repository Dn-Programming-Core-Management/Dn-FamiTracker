/*
** FamiTracker - NES/Famicom sound tracker
** Copyright (C) 2005-2014  Jonathan Liss
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


// CConfigMixer dialog

class CConfigMixer : public CPropertyPage
{
	DECLARE_DYNAMIC(CConfigMixer)

public:
	CConfigMixer();
	virtual ~CConfigMixer();

// Dialog Data
	enum { IDD = IDD_CONFIG_MIXER };

	static const int LEVEL_RANGE;
	static const int LEVEL_SCALE;

private:
	int m_iLevelAPU1;
	int m_iLevelAPU2;
	int m_iLevelVRC6;
	int m_iLevelVRC7;
	int m_iLevelMMC5;
	int m_iLevelFDS;
	int m_iLevelN163;
	int m_iLevelS5B;

protected:
	void SetupSlider(int nID) const;
	void UpdateLevels();
	void UpdateLevel(int nID, int Level);

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	virtual BOOL OnInitDialog();
	virtual BOOL OnApply();
	afx_msg void OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
};
