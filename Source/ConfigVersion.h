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
#include "FamiTrackerTypes.h"


// TODO: put these into FamiTrackerDoc.h maybe?
enum ft_block_t {
	BLK_PARAMS,
	BLK_INFO,
	BLK_HEADER,
	BLK_INSTRUMENTS,
	BLK_SEQUENCES,
	BLK_FRAMES,
	BLK_PATTERNS,
	BLK_DPCM_SAMPLES,
	BLK_COMMENTS,
	BLK_SEQUENCES_VRC6,
	BLK_SEQUENCES_N163,
	BLK_SEQUENCES_S5B,
	BLK_DETUNETABLES,
	BLK_GROOVES,
	BLK_BOOKMARKS,
	BLK_PARAMS_EXTRA,
	BLK_COUNT
};

struct stVerInfo {
	CString  Name;
	effect_t MaxEffect;
	unsigned FTMver;
	int      MaxArpMode;
	int      Version[BLK_COUNT];
};

#define VERSION_INFO_COUNT 9

// CConfigVersion dialog

class CConfigVersion : public CPropertyPage
{
	DECLARE_DYNAMIC(CConfigVersion)

public:
	CConfigVersion();
	virtual ~CConfigVersion();

// Dialog Data
	enum { IDD = IDD_CONFIG_VERSION };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	void UpdateInfo();

protected:
	static const CString VERSION_TEXT[];
	static const effect_t MAX_EFFECT_INDEX[];
	static const stVerInfo VERSION_INFO[VERSION_INFO_COUNT];

	int m_iModuleErrorLevel;

	CComboBox *m_cComboVersion;
	CSliderCtrl *m_cSliderErrorLevel;

	DECLARE_MESSAGE_MAP()
public:
	virtual BOOL OnInitDialog();
	virtual BOOL OnApply();
	afx_msg void OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg void OnNMCustomdrawSliderVersionErrorlevel(NMHDR *pNMHDR, LRESULT *pResult);
};
