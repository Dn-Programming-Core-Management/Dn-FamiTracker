/*
** FamiTracker - NES/Famicom sound tracker
** Copyright (C) 2005-2014  Jonathan Liss
**
** 0CC-FamiTracker is (C) 2014-2015 HertzDevil
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


class CFamiTrackerDoc;

// CTransposeDlg dialog

class CTransposeDlg : public CDialog
{
	DECLARE_DYNAMIC(CTransposeDlg)

public:
	CTransposeDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~CTransposeDlg();

	void SetTrack(unsigned int Track);

// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_TRANSPOSE };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

private:
	void Transpose(int Trsp, unsigned int Track, bool Remap);
	void RemapDPCM(int Trsp);
	
	CFamiTrackerDoc *m_pDocument;
	int m_iTrack;

	static bool s_bDisableInst[MAX_INSTRUMENTS + 1];
	static const UINT BUTTON_ID;

	CButton **m_cInstButton;
	CFont *m_pFont;
	
protected:
	DECLARE_MESSAGE_MAP()
public:
	virtual BOOL OnInitDialog();
	afx_msg void OnBnClickedInst(UINT nID);
	afx_msg void OnBnClickedOk();
	afx_msg void OnBnClickedCheckTrspAll();
	afx_msg void OnBnClickedButtonTrspReverse();
	afx_msg void OnBnClickedButtonTrspClear();
};
