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

class CExportDialog;

typedef void (CExportDialog::*exportFunc_t)();

// CExportDialog dialog

class CExportDialog : public CDialog
{
	DECLARE_DYNAMIC(CExportDialog)

public:
	CExportDialog(CWnd* pParent = NULL);   // standard constructor
	virtual ~CExportDialog();

// Dialog Data
	enum { IDD = IDD_EXPORT };

protected:
	static int m_iExportOption;

protected:
	static const exportFunc_t DEFAULT_EXPORT_FUNCS[];
	static const LPTSTR		  DEFAULT_EXPORT_NAMES[];
	static const int		  DEFAULT_EXPORTERS;

	static LPCTSTR NSF_FILTER[2];
	static LPCTSTR NSFE_FILTER[2];		// // //
	static LPCTSTR NSF2_FILTER[2];		// !! !!
	static LPCTSTR NES_FILTER[2];
	static LPCTSTR RAW_FILTER[2];
	static LPCTSTR DPCMS_FILTER[2];
	static LPCTSTR PRG_FILTER[2];
	static LPCTSTR ASM_FILTER[2];

#ifdef _DEBUG
	CString m_strFile;
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	void CreateNSF();
	void CreateNSFe();		// // //
	void CreateNSF2();		// !! !!
	void CreateNES();
	void CreateBIN();
	void CreatePRG();
	void CreateASM();
	void CreateCustom( CString name );

	DECLARE_MESSAGE_MAP()
public:
	virtual BOOL OnInitDialog();
	afx_msg void OnBnClickedClose();
	afx_msg void OnBnClickedExport();
	afx_msg void OnBnClickedPlay();
	afx_msg void OnCbnSelchangeType();
};
