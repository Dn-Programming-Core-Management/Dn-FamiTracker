/*
** FamiTracker - NES/Famicom sound tracker
** Copyright (C) 2005-2014  Jonathan Liss
**
** 0CC-FamiTracker is (C) 2014-2017 HertzDevil
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

#include "stdafx.h"		// // //
#include "resource.h"		// // //
#include <memory>		// // //

class CWaveRenderer;		// // //

// CWavProgressDlg dialog

class CWavProgressDlg : public CDialog
{
	DECLARE_DYNAMIC(CWavProgressDlg)

public:
	CWavProgressDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~CWavProgressDlg();

	void BeginRender(CString &File, std::unique_ptr<CWaveRenderer> pRenderer);		// // //

// Dialog Data
	enum { IDD = IDD_WAVE_PROGRESS };

protected:
	DWORD m_dwStartTime;
	std::shared_ptr<CWaveRenderer> m_pWaveRenderer;		// // //
	
	CString m_sFile;

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedCancel();
	virtual BOOL OnInitDialog();
	afx_msg void OnTimer(UINT_PTR nIDEvent);
};
