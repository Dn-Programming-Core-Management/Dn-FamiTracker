/*
** FamiTracker - NES/Famicom sound tracker
** Copyright (C) 2005-2014  Jonathan Liss
**
** 0CC-FamiTracker is (C) 2014-2017 HertzDevil
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

#include "stdafx.h"		// // //
#include "resource.h"		// // //
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
