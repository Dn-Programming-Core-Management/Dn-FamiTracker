/*
** FamiTracker - NES/Famicom sound tracker
** Copyright (C) 2005-2014  Jonathan Liss
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
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU 
** Library General Public License for more details.  To obtain a 
** copy of the GNU Library General Public License, write to the Free 
** Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
**
** Any permitted reproduction of these routines, in whole or in part,
** must bear this legend.
*/

#pragma once

class CInstrumentEditorVRC7 : public CInstrumentEditPanel
{

	enum PatchTone {
		VRC7_NUKE = 0,
		VRC7_RW = 1,
		VRC7_FT36 = 2,
		VRC7_FT35 = 3,
		VRC7_MO = 4,
		VRC7_KT2 = 5,
		VRC7_KT1 = 6,
		TONE_2413 = 7,
		TONE_281B = 8,
	};

	DECLARE_DYNAMIC(CInstrumentEditorVRC7)

public:
	CInstrumentEditorVRC7(CWnd* pParent = NULL);   // standard constructor
	virtual ~CInstrumentEditorVRC7();
	virtual int GetIDD() const { return IDD; };
	virtual TCHAR *GetTitle() const { return _T("Konami VRC7"); };

	virtual void SelectInstrument(std::shared_ptr<CInstrument> pInst);

// Dialog Data
	enum { IDD = IDD_INSTRUMENT_VRC7 };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	void LoadPatch(int Num);
	void LoadCustomPatch();
	void SaveCustomPatch();
	void LoadInternalPatch(int Num);
	void WritePatchText(int Patch);

	void SetupSlider(int Slider, int Max);
	int GetSliderVal(int Slider);
	void SetSliderVal(int Slider, int Value);
	void EnableControls(bool bEnable);
	void SelectPatch(int Patch);
	void PasteSettings(LPCTSTR pString);
	void CopyAsPlainText();		// // //
	uint8_t FetchPatchByte(PatchTone patch_bank_id, int patch, unsigned char patch_byte);

private:
	PatchTone PatchBank;

protected:
	std::shared_ptr<CInstrumentVRC7> m_pInstrument;

	DECLARE_MESSAGE_MAP()
public:
	virtual BOOL OnInitDialog();
	afx_msg void OnCbnSelchangePatch();
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
	afx_msg void OnBnClickedCheckbox();
	afx_msg void OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg void OnContextMenu(CWnd* /*pWnd*/, CPoint /*point*/);
	afx_msg void OnCopy();
	afx_msg void OnPaste();
	virtual BOOL PreTranslateMessage(MSG* pMsg);
};
