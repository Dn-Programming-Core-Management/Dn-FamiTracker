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

// CInstrumentEditorFDSEnvelope dialog

class CInstrumentEditorFDSEnvelope : public CSequenceInstrumentEditPanel
{
	DECLARE_DYNAMIC(CInstrumentEditorFDSEnvelope)

public:
	CInstrumentEditorFDSEnvelope(CWnd* pParent = NULL);   // standard constructor
	virtual ~CInstrumentEditorFDSEnvelope();
	virtual int GetIDD() const { return IDD; };
	virtual TCHAR *GetTitle() const { return _T("Envelopes"); };

	// Public
	virtual void SelectInstrument(std::shared_ptr<CInstrument> pInst);
	void UpdateSequenceString(bool Changed) override;		// // //
	void SetupParser() const override;		// // //

// Dialog Data
	enum { IDD = IDD_INSTRUMENT_FDS_ENVELOPE };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	virtual void OnKeyReturn();
	void LoadSequence();

protected:
	static const int MAX_VOLUME = 32;

protected:
	std::shared_ptr<CInstrumentFDS> m_pInstrument;

	DECLARE_MESSAGE_MAP()
public:
	virtual BOOL OnInitDialog();
	afx_msg void OnCbnSelchangeType();
};
