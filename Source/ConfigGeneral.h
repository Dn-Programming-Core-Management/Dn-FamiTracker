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

