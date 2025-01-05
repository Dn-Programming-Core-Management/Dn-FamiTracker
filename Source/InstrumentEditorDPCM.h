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

class CFamiTrackerView;
class CInstrument2A03;		// // //
class CDSample;		// // //

// CInstrumentDPCM dialog

class CInstrumentEditorDPCM : public CInstrumentEditPanel
{
	DECLARE_DYNAMIC(CInstrumentEditorDPCM)

public:
	CInstrumentEditorDPCM(CWnd* pParent = NULL);   // standard constructor
	virtual ~CInstrumentEditorDPCM();
	virtual int GetIDD() const { return IDD; };
	virtual TCHAR *GetTitle() const { return _T("DPCM samples"); };

// Dialog Data
	enum { IDD = IDD_INSTRUMENT_DPCM };

	virtual void SelectInstrument(std::shared_ptr<CInstrument> pInst);

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	void BuildKeyList();
	void BuildSampleList();
	void UpdateCurrentKey();		// // //
	void UpdateKey(int Index);
	bool LoadSample(const CString &FilePath, const CString &FileName);
	bool InsertSample(CDSample *pNewSample);

	const CDSample *GetSelectedSample();		// // //
	void SetSelectedSample(CDSample *pSamp) const;		// // //

protected:
	std::shared_ptr<CInstrument2A03> m_pInstrument;

	int	m_iSelectedSample;
	int	m_iOctave;
	int m_iSelectedKey;

	DECLARE_MESSAGE_MAP()
public:
	virtual BOOL OnInitDialog();
	afx_msg void OnBnClickedLoad();
	afx_msg void OnBnClickedUnload();
	afx_msg void OnNMClickSampleList(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnBnClickedImport();
	afx_msg void OnCbnSelchangePitch();
	afx_msg void OnLvnItemchangedTable(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnNMClickTable(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnCbnSelchangeSamples();
	afx_msg void OnBnClickedSave();
	afx_msg void OnBnClickedLoop();
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	afx_msg void OnBnClickedSawhack();
	afx_msg void OnBnClickedAdd();
	afx_msg void OnBnClickedRemove();
	afx_msg void OnEnChangeLoopPoint();
	afx_msg void OnBnClickedEdit();
	afx_msg void OnNMDblclkSampleList(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnBnClickedPreview();
	afx_msg void OnNMRClickSampleList(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnNMRClickTable(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnNMDblclkTable(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnEnChangeDeltaCounter();
	afx_msg void OnDeltaposDeltaSpin(NMHDR *pNMHDR, LRESULT *pResult);
};
