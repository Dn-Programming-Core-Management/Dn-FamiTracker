/*
** FamiTracker - NES/Famicom sound tracker
** Copyright (C) 2005-2020 Jonathan Liss
**
** 0CC-FamiTracker is (C) 2014-2018 HertzDevil
**
** Dn-FamiTracker is (C) 2020-2024 D.P.C.M.
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
	bool m_bSurveyMixing;		// !! !!
	unsigned int m_iSelectedSong;
	unsigned char m_iExpansions;		// // //
	uint8_t m_iN163Channels;
	int16_t m_iDeviceLevelOffset[8];

	uint8_t m_iOPLLPatchBytes[19 * 8];
	std::string m_strOPLLPatchNames[19];

	CFamiTrackerDoc* m_pDocument;

public:
	CModulePropertiesDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~CModulePropertiesDlg();

	// Dialog Data
	enum { IDD = IDD_PROPERTIES };

protected:
	// virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	void strFromLevel(CString& target, int16_t Level);
	bool levelFromStr(int16_t& target, CString dBstr);

	void FillSongList();

	NoNotifyEdit m_cDeviceLevelEdit[8];
	CSliderCtrl m_cDeviceLevelSlider[8];
	CStatic m_cDeviceLevelLabel[8];
	CStatic m_cDevicedBLabel[8];

	CStatic m_cOPLLPatchLabel[19];
	NoNotifyEdit m_cOPLLPatchBytesEdit[19];
	NoNotifyEdit m_cOPLLPatchNameEdit[19];

	CStatic m_cChannelsLabel;
	CSliderCtrl m_cChanSlider;

	DECLARE_MESSAGE_MAP()
public:
	void setN163NChannels(int nchan);
	void updateN163ChannelCountUI();
	void updateDeviceMixOffsetUI(int device, bool renderText = true);
	void updateExternallOPLLUI(int patchnum, bool renderText = true);
	CString PatchBytesToText(uint8_t* patchbytes);
	void PatchTextToBytes(LPCTSTR pString, int index);
	void OffsetSlider(int device, int pos);
	void DeviceOffsetEdit(int device);
	void OpllPatchByteEdit(int patchnum);
	void OpllPatchNameEdit(int patchnum);

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
	afx_msg void OnBnClickedSurveyMixing();
};
