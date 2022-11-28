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
	bool m_bExternalOPLL;		// !! !!
	unsigned int m_iSelectedSong;
	unsigned char m_iExpansions;		// // //
	uint8_t m_iN163Channels;
	int16_t m_iAPU1LevelOffset;
	int16_t m_iAPU2LevelOffset;
	int16_t m_iVRC6LevelOffset;
	int16_t m_iVRC7LevelOffset;
	int16_t m_iFDSLevelOffset;
	int16_t m_iMMC5LevelOffset;
	int16_t m_iN163LevelOffset;
	int16_t m_iS5BLevelOffset;

	uint8_t m_iOPLLPatchBytes[19 * 8];
	std::string m_strOPLLPatchNames[19];

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

	NoNotifyEdit m_cAPU1LevelEdit;
	NoNotifyEdit m_cAPU2LevelEdit;
	NoNotifyEdit m_cVRC6LevelEdit;
	NoNotifyEdit m_cVRC7LevelEdit;
	NoNotifyEdit m_cFDSLevelEdit;
	NoNotifyEdit m_cMMC5LevelEdit;
	NoNotifyEdit m_cN163LevelEdit;
	NoNotifyEdit m_cS5BLevelEdit;

	CStatic m_cAPU1LevelLabel;
	CStatic m_cAPU2LevelLabel;
	CStatic m_cVRC6LevelLabel;
	CStatic m_cVRC7LevelLabel;
	CStatic m_cFDSLevelLabel;
	CStatic m_cMMC5LevelLabel;
	CStatic m_cN163LevelLabel;
	CStatic m_cS5BLevelLabel;

	CStatic m_cAPU1dBLabel;
	CStatic m_cAPU2dBLabel;
	CStatic m_cVRC6dBLabel;
	CStatic m_cVRC7dBLabel;
	CStatic m_cFDSdBLabel;
	CStatic m_cMMC5dBLabel;
	CStatic m_cN163dBLabel;
	CStatic m_cS5BdBLabel;

	CSliderCtrl m_cAPU1LevelSlider;
	CSliderCtrl m_cAPU2LevelSlider;
	CSliderCtrl m_cVRC6LevelSlider;
	CSliderCtrl m_cVRC7LevelSlider;
	CSliderCtrl m_cFDSLevelSlider;
	CSliderCtrl m_cMMC5LevelSlider;
	CSliderCtrl m_cN163LevelSlider;
	CSliderCtrl m_cS5BLevelSlider;

	CStatic m_cOPLLPatchLabel0;
	CStatic m_cOPLLPatchLabel1;
	CStatic m_cOPLLPatchLabel2;
	CStatic m_cOPLLPatchLabel3;
	CStatic m_cOPLLPatchLabel4;
	CStatic m_cOPLLPatchLabel5;
	CStatic m_cOPLLPatchLabel6;
	CStatic m_cOPLLPatchLabel7;
	CStatic m_cOPLLPatchLabel8;
	CStatic m_cOPLLPatchLabel9;
	CStatic m_cOPLLPatchLabel10;
	CStatic m_cOPLLPatchLabel11;
	CStatic m_cOPLLPatchLabel12;
	CStatic m_cOPLLPatchLabel13;
	CStatic m_cOPLLPatchLabel14;
	CStatic m_cOPLLPatchLabel15;
	CStatic m_cOPLLPatchLabel16;
	CStatic m_cOPLLPatchLabel17;
	CStatic m_cOPLLPatchLabel18;
	
	NoNotifyEdit m_cOPLLPatchBytesEdit0;
	NoNotifyEdit m_cOPLLPatchBytesEdit1;
	NoNotifyEdit m_cOPLLPatchBytesEdit2;
	NoNotifyEdit m_cOPLLPatchBytesEdit3;
	NoNotifyEdit m_cOPLLPatchBytesEdit4;
	NoNotifyEdit m_cOPLLPatchBytesEdit5;
	NoNotifyEdit m_cOPLLPatchBytesEdit6;
	NoNotifyEdit m_cOPLLPatchBytesEdit7;
	NoNotifyEdit m_cOPLLPatchBytesEdit8;
	NoNotifyEdit m_cOPLLPatchBytesEdit9;
	NoNotifyEdit m_cOPLLPatchBytesEdit10;
	NoNotifyEdit m_cOPLLPatchBytesEdit11;
	NoNotifyEdit m_cOPLLPatchBytesEdit12;
	NoNotifyEdit m_cOPLLPatchBytesEdit13;
	NoNotifyEdit m_cOPLLPatchBytesEdit14;
	NoNotifyEdit m_cOPLLPatchBytesEdit15;
	NoNotifyEdit m_cOPLLPatchBytesEdit16;
	NoNotifyEdit m_cOPLLPatchBytesEdit17;
	NoNotifyEdit m_cOPLLPatchBytesEdit18;

	NoNotifyEdit m_cOPLLPatchNameEdit0;
	NoNotifyEdit m_cOPLLPatchNameEdit1;
	NoNotifyEdit m_cOPLLPatchNameEdit2;
	NoNotifyEdit m_cOPLLPatchNameEdit3;
	NoNotifyEdit m_cOPLLPatchNameEdit4;
	NoNotifyEdit m_cOPLLPatchNameEdit5;
	NoNotifyEdit m_cOPLLPatchNameEdit6;
	NoNotifyEdit m_cOPLLPatchNameEdit7;
	NoNotifyEdit m_cOPLLPatchNameEdit8;
	NoNotifyEdit m_cOPLLPatchNameEdit9;
	NoNotifyEdit m_cOPLLPatchNameEdit10;
	NoNotifyEdit m_cOPLLPatchNameEdit11;
	NoNotifyEdit m_cOPLLPatchNameEdit12;
	NoNotifyEdit m_cOPLLPatchNameEdit13;
	NoNotifyEdit m_cOPLLPatchNameEdit14;
	NoNotifyEdit m_cOPLLPatchNameEdit15;
	NoNotifyEdit m_cOPLLPatchNameEdit16;
	NoNotifyEdit m_cOPLLPatchNameEdit17;
	NoNotifyEdit m_cOPLLPatchNameEdit18;

	CStatic m_cChannelsLabel;
	CSliderCtrl m_cChanSlider;

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
	void updateN163ChannelCountUI();
	void updateDeviceMixOffsetUI(int device, bool renderText=true);
	void updateExternallOPLLUI(int patchnum, bool renderText=true);
	CString PatchBytesToText(uint8_t *patchbytes);
	uint8_t PatchTextToBytes(LPCTSTR pString, int index);
	void OffsetSlider(int device, int pos);
	afx_msg void OnEnChangeApu1OffsetEdit();
	afx_msg void OnEnChangeApu2OffsetEdit();
	afx_msg void OnEnChangeVrc6OffsetEdit();
	afx_msg void OnEnChangeVrc7OffsetEdit();
	afx_msg void OnEnChangeFdsOffsetEdit();
	afx_msg void OnEnChangeMmc5OffsetEdit();
	afx_msg void OnEnChangeN163OffsetEdit();
	afx_msg void OnEnChangeS5bOffsetEdit();
	afx_msg void OnBnClickedExternalOpll();
	afx_msg void OnEnKillfocusOpllPatchbyte1();
	afx_msg void OnEnChangeOpllPatchname1();
	afx_msg void OnEnKillfocusOpllPatchbyte2();
	afx_msg void OnEnChangeOpllPatchname2();
	afx_msg void OnEnKillfocusOpllPatchbyte3();
	afx_msg void OnEnChangeOpllPatchname3();
	afx_msg void OnEnKillfocusOpllPatchbyte4();
	afx_msg void OnEnChangeOpllPatchname4();
	afx_msg void OnEnKillfocusOpllPatchbyte5();
	afx_msg void OnEnChangeOpllPatchname5();
	afx_msg void OnEnKillfocusOpllPatchbyte6();
	afx_msg void OnEnChangeOpllPatchname6();
	afx_msg void OnEnKillfocusOpllPatchbyte7();
	afx_msg void OnEnChangeOpllPatchname7();
	afx_msg void OnEnKillfocusOpllPatchbyte8();
	afx_msg void OnEnChangeOpllPatchname8();
	afx_msg void OnEnKillfocusOpllPatchbyte9();
	afx_msg void OnEnChangeOpllPatchname9();
	afx_msg void OnEnKillfocusOpllPatchbyte10();
	afx_msg void OnEnChangeOpllPatchname10();
	afx_msg void OnEnKillfocusOpllPatchbyte11();
	afx_msg void OnEnChangeOpllPatchname11();
	afx_msg void OnEnKillfocusOpllPatchbyte12();
	afx_msg void OnEnChangeOpllPatchname12();
	afx_msg void OnEnKillfocusOpllPatchbyte13();
	afx_msg void OnEnChangeOpllPatchname13();
	afx_msg void OnEnKillfocusOpllPatchbyte14();
	afx_msg void OnEnChangeOpllPatchname14();
	afx_msg void OnEnKillfocusOpllPatchbyte15();
	afx_msg void OnEnChangeOpllPatchname15();
	afx_msg void OnEnKillfocusOpllPatchbyte16();
	afx_msg void OnEnChangeOpllPatchname16();
	afx_msg void OnEnKillfocusOpllPatchbyte17();
	afx_msg void OnEnChangeOpllPatchname17();
	afx_msg void OnEnKillfocusOpllPatchbyte18();
	afx_msg void OnEnChangeOpllPatchname18();
};
