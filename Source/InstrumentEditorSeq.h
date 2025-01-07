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
#include "CustomControls.h"

/*!
	\brief Specialization of the instrument sequence editor panel class for CSeqInstrument.
*/
class CInstrumentEditorSeq : public CSequenceInstrumentEditPanel
{
	DECLARE_DYNAMIC(CInstrumentEditorSeq)

public:
	CInstrumentEditorSeq(CWnd* pParent, TCHAR *Title, LPCTSTR *SeqName, int Vol, int Duty, inst_type_t Type);
	virtual int GetIDD() const { return IDD; };
	virtual TCHAR *GetTitle() const { return m_pTitle; };

	// Public
	virtual void SelectInstrument(std::shared_ptr<CInstrument> pInst);
	void UpdateSequenceString(bool Changed) override;		// // //

// Dialog Data
	enum { IDD = IDD_INSTRUMENT_INTERNAL };

protected:
	virtual void OnKeyReturn();

	void SelectSequence(int Sequence, int Type);
	void SetupParser() const override;		// // //

protected:
	LPCTSTR *m_pSequenceName;
	TCHAR *m_pTitle;
	const int m_iMaxVolume;
	const int m_iMaxDuty;
	const inst_type_t m_iInstType;

protected:
	DECLARE_MESSAGE_MAP()
public:
	virtual BOOL OnInitDialog();
	afx_msg void OnLvnItemchangedInstsettings(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnEnChangeSeqIndex();
	afx_msg void OnBnClickedFreeSeq();
	virtual BOOL DestroyWindow();
	afx_msg void OnCloneSequence();
};
