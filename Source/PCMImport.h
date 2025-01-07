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

namespace jarh {
	class sinc;
}

class CPCMImport : public CDialog
{
	DECLARE_DYNAMIC(CPCMImport)

public:
	CPCMImport(CWnd* pParent = NULL);   // standard constructor
	virtual ~CPCMImport();

// Dialog Data
	enum { IDD = IDD_PCMIMPORT };

	CDSample *ShowDialog();

protected:
	CDSample *m_pImported;
	CDSample *m_pCachedSample;

	CString		m_strPath, m_strFileName;
	CFile		m_fSampleFile;
	ULONGLONG	m_ullSampleStart;

	int m_iQuality;
	int m_iVolume;
	int m_iSampleSize;
	int m_iChannels;
	int m_iBlockAlign;
	int m_iAvgBytesPerSec;
	int m_iSamplesPerSec;
	int m_iCachedQuality;
	int m_iCachedVolume;
	unsigned int m_iWaveSize;

	jarh::sinc *m_psinc;

protected:
	static const int QUALITY_RANGE;
	static const int VOLUME_RANGE;

protected:
	CDSample *GetSample();
	CDSample *ConvertFile();

	bool OpenWaveFile();
	void UpdateFileInfo();
	void UpdateText();

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	virtual BOOL OnInitDialog();
	afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg void OnBnClickedCancel();
	afx_msg void OnBnClickedOk();
	afx_msg void OnBnClickedPreview();
};
