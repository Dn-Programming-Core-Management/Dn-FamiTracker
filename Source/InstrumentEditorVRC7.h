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

class CInstrumentEditorVRC7 : public CInstrumentEditPanel
{
	DECLARE_DYNAMIC(CInstrumentEditorVRC7)

public:
	CInstrumentEditorVRC7(CWnd* pParent = NULL);   // standard constructor
	virtual ~CInstrumentEditorVRC7();
	virtual int GetIDD() const { return IDD; };
	virtual TCHAR *GetTitle() const { return _T("Konami VRC7"); };

	virtual void SelectInstrument(std::shared_ptr<CInstrument> pInst);

// Dialog Data
	enum { IDD = IDD_INSTRUMENT_VRC7 };

private:
	uint8_t			OPLLPatchBytes[19 * 8];
	std::string		OPLLPatchNames[19];

	CFamiTrackerDoc *m_pDocument;
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	void LoadPatch(int Num);
	void LoadCustomPatch();
	void SaveCustomPatch();
	void LoadInternalPatch(int Num);
	void PatchBytesToText(int Patch);

	void SetupSlider(int Slider, int Max);
	int GetSliderVal(int Slider);
	void SetSliderVal(int Slider, int Value);
	void EnableControls(bool bEnable);
	void SelectPatch(int Patch);
	void PatchTextToBytes(LPCTSTR pString);
	void CopyAsPlainText();		// // //
	uint8_t FetchPatchByte(int patch, unsigned char patch_byte);

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
