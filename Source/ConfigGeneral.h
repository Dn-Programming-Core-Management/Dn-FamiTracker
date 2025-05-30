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

#include "stdafx.h"		// // //
#include "../resource.h"        // // //

#define SETTINGS_BOOL_COUNT 23		// // //

// CConfigGeneral dialog

class CConfigGeneral : public CPropertyPage
{
	DECLARE_DYNAMIC(CConfigGeneral)

public:
	CConfigGeneral();
	virtual ~CConfigGeneral();

// Dialog Data
	enum { IDD = IDD_CONFIG_GENERAL };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	bool	m_bWrapCursor;
	bool	m_bWrapFrames;
	bool	m_bFreeCursorEdit;
	bool	m_bPreviewWAV;
	bool	m_bKeyRepeat;
	bool	m_bRowInHex;
	bool	m_bFramePreview;
	bool	m_bNoDPCMReset;
	bool	m_bNoStepMove;
	bool	m_bPullUpDelete;
	bool	m_bBackups;
	bool	m_bSingleInstance;
	bool	m_bPreviewFullRow;
	bool	m_bDisableDblClick;
	bool	m_bWrapPatternValue;		// // //
	bool	m_bCutVolume;
	bool	m_bFDSOldVolume;
	bool	m_bRetrieveChanState;
	bool	m_bOverflowPaste;
	bool	m_bShowSkippedRows;
	bool	m_bHexKeypad;
	bool	m_bMultiFrameSel;
	bool	m_bCheckVersion;

	int		m_iEditStyle;
	int		m_iPageStepSize;

	int		m_iKeyNoteCut;
	int		m_iKeyNoteRelease;
	int		m_iKeyClear;
	int		m_iKeyRepeat;
	int		m_iKeyEchoBuffer;		// // //

	static const CString CONFIG_STR[SETTINGS_BOOL_COUNT];		// // //
	static const CString CONFIG_DESC[SETTINGS_BOOL_COUNT];		// // //

	CToolTipCtrl m_wndToolTip;

	DECLARE_MESSAGE_MAP()
public:
	virtual BOOL OnSetActive();
	virtual void OnOK();
	virtual BOOL OnApply();
	virtual BOOL OnInitDialog();
	afx_msg void OnCbnEditupdatePagelength();
	afx_msg void OnCbnSelendokPagelength();
	afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	virtual BOOL PreTranslateMessage(MSG* pMsg);

	afx_msg void OnCbnSelchangeComboStyle();		// // //
	afx_msg void OnLvnItemchangedConfigList(NMHDR *pNMHDR, LRESULT *pResult);
};

