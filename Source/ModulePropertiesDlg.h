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

#include "NoNotifyEdit.h"


// CModulePropertiesDlg dialog

class CModulePropertiesDlg : public CDialog
{
	DECLARE_DYNAMIC(CModulePropertiesDlg)

private:
	void SelectSong(int Song);
	void UpdateSongButtons();
	void SetupSlider(int nID) const;
	
	bool m_bSingleSelection;		// // //
	unsigned int m_iSelectedSong;
	unsigned char m_iExpansions;		// // //
	int16_t m_iN163Channels;
	int16_t APU1LevelOffset;
	int16_t APU2LevelOffset;
	int16_t VRC6LevelOffset;
	int16_t VRC7LevelOffset;
	int16_t FDSLevelOffset;
	int16_t MMC5LevelOffset;
	int16_t N163LevelOffset;
	int16_t S5BLevelOffset;

	CFamiTrackerDoc *m_pDocument;

public:
	CModulePropertiesDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~CModulePropertiesDlg();

// Dialog Data
	enum { IDD = IDD_PROPERTIES };

protected:
	// virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	void strFromLevel(CString & target, int16_t Level);
	bool levelFromStr(int16_t & target, CString dBstr);
	
	void FillSongList();

	NoNotifyEdit APU1LevelEdit;
	NoNotifyEdit APU2LevelEdit;
	NoNotifyEdit VRC6LevelEdit;
	NoNotifyEdit VRC7LevelEdit;
	NoNotifyEdit FDSLevelEdit;
	NoNotifyEdit MMC5LevelEdit;
	NoNotifyEdit N163LevelEdit;
	NoNotifyEdit S5BLevelEdit;

	DECLARE_MESSAGE_MAP()
public:
	virtual BOOL OnInitDialog();
	afx_msg void OnBnClickedOk();
	afx_msg void OnBnClickedSongAdd();
	afx_msg void OnBnClickedSongInsert();		// // //
	afx_msg void OnBnClickedSongRemove();
	afx_msg void OnBnClickedSongUp();
	afx_msg void OnBnClickedSongDown();
	afx_msg void OnEnChangeSongname();
	afx_msg void OnBnClickedSongImport();
	
	afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg void OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg void OnLvnItemchangedSonglist(NMHDR *pNMHDR, LRESULT *pResult);
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	afx_msg void OnBnClickedExpansionVRC6();		// // //
	afx_msg void OnBnClickedExpansionVRC7();
	afx_msg void OnBnClickedExpansionFDS();
	afx_msg void OnBnClickedExpansionMMC5();
	afx_msg void OnBnClickedExpansionS5B();
	afx_msg void OnBnClickedExpansionN163();
	void setN163NChannels(int nchan);
	void updateN163ChannelCount(bool renderText=true);
	void updateLevelGUI(int device, bool renderText=true);
	void OffsetSlider(int device, int pos);
	afx_msg void OnEnChangeApu1OffsetEdit();
	afx_msg void OnEnChangeApu2OffsetEdit();
	afx_msg void OnEnChangeVrc6OffsetEdit();
	afx_msg void OnEnChangeVrc7OffsetEdit();
	afx_msg void OnEnChangeFdsOffsetEdit();
	afx_msg void OnEnChangeMmc5OffsetEdit();
	afx_msg void OnEnChangeN163OffsetEdit();
	afx_msg void OnEnChangeS5bOffsetEdit();
};
