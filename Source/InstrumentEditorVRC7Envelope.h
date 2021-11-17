/*
** FamiTracker - NES/Famicom sound tracker
** Copyright (C) 2005-2014  Jonathan Liss
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

// CInstrumentEditorVRC7Envelope dialog

class CInstrumentEditorVRC7Envelope : public CSequenceInstrumentEditPanel
{
	DECLARE_DYNAMIC(CInstrumentEditorVRC7Envelope)

public:
	CInstrumentEditorVRC7Envelope(CWnd* pParent = NULL);   // standard constructor
	virtual ~CInstrumentEditorVRC7Envelope();
	virtual int GetIDD() const { return IDD; };
	virtual TCHAR *GetTitle() const { return _T("Envelopes"); };

	// Public
	virtual void SelectInstrument(std::shared_ptr<CInstrument> pInst);
	void UpdateSequenceString(bool Changed) override;		// // //
	void SetupParser() const override;		// // //

// Dialog Data
	enum { IDD = IDD_INSTRUMENT_VRC7_ENVELOPE };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	virtual void OnKeyReturn();
	void LoadSequence();

protected:
	static const int MAX_VOLUME = 15;
	static const int MAX_DUTY   = 15;

protected:
	std::shared_ptr<CInstrumentVRC7> m_pInstrument;

	DECLARE_MESSAGE_MAP()
public:
	virtual BOOL OnInitDialog();
	afx_msg void OnCbnSelchangeType();
};
