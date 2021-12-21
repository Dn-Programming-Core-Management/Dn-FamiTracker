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

#include "stdafx.h"
#include "FamiTracker.h"
#include "DSample.h"
#include "FamiTrackerTypes.h"
#include "APU/Types.h"
#include "SoundGen.h"
#include "SampleEditorView.h"
#include "SampleEditorDlg.h"

//
// The DPCM sample editor
//

static UINT indicators[] = {		// // //
	ID_INDICATOR_DPCM_SEGMENT,
	ID_INDICATOR_DPCM_SIZE,
	ID_INDICATOR_DPCM_ENDPOS,
};

enum {
	TMR_PLAY_CURSOR, 
	TMR_START_CURSOR
};

// CSampleEditorDlg dialog

IMPLEMENT_DYNAMIC(CSampleEditorDlg, CDialog)

CSampleEditorDlg::CSampleEditorDlg(CWnd* pParent /*=NULL*/, CDSample *pSample)
	: CDialog(CSampleEditorDlg::IDD, pParent), m_pSampleEditorView(NULL),
	m_pSample(pSample)		// // //
{
	m_pSoundGen = theApp.GetSoundGenerator();
}

CSampleEditorDlg::~CSampleEditorDlg()
{
	SAFE_RELEASE(m_pSampleEditorView);
	SAFE_RELEASE(m_pSample);
}

CDSample *CSampleEditorDlg::GetDSample() const		// // //
{
	return m_pSample;
}

void CSampleEditorDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CSampleEditorDlg, CDialog)
	ON_WM_SIZE()
	ON_BN_CLICKED(IDC_PLAY, &CSampleEditorDlg::OnBnClickedPlay)
	ON_WM_TIMER()
	ON_BN_CLICKED(IDC_DELETE, &CSampleEditorDlg::OnBnClickedDelete)
	ON_BN_CLICKED(IDC_DELTASTART, &CSampleEditorDlg::OnBnClickedDeltastart)
	ON_WM_KEYDOWN()
	ON_BN_CLICKED(IDC_TILT, &CSampleEditorDlg::OnBnClickedTilt)
	ON_WM_HSCROLL()
	ON_WM_GETMINMAXINFO()
	ON_BN_CLICKED(IDC_BIT_REVERSE, &CSampleEditorDlg::OnBnClickedBitReverse)
END_MESSAGE_MAP()


// CSampleEditorDlg message handlers

BOOL CSampleEditorDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	if (!m_wndInfoStatusBar.Create(this) ||
		!m_wndInfoStatusBar.SetIndicators(indicators, sizeof(indicators) / sizeof(*indicators)))		// // // 050B
		return FALSE;
	m_wndInfoStatusBar.SetPaneInfo(0, ID_INDICATOR_DPCM_SEGMENT, SBPS_STRETCH, 0);
	m_wndInfoStatusBar.SetPaneInfo(1, ID_INDICATOR_DPCM_SIZE, SBPS_NORMAL, 150);
	m_wndInfoStatusBar.SetPaneInfo(2, ID_INDICATOR_DPCM_ENDPOS, SBPS_NORMAL, 150);
	RepositionBars(AFX_IDW_CONTROLBAR_FIRST, AFX_IDW_CONTROLBAR_LAST, ID_INDICATOR_DPCM_SEGMENT);

	m_pSampleEditorView = new CSampleEditorView();
	m_pSampleEditorView->SubclassDlgItem(IDC_SAMPLE, this);

	CSliderCtrl *pitch = static_cast<CSliderCtrl*>(GetDlgItem(IDC_PITCH));
	pitch->SetRange(0, 15);
	pitch->SetPos(15);

	// A timer for the flashing start cursor
	SetTimer(TMR_START_CURSOR, 500, NULL);

	CString title;
	GetWindowText(title);
	title.AppendFormat(_T(" [%s]"), m_pSample->GetName());
	SetWindowText(title);

	UpdateSampleView();
	SelectionChanged();

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

void CSampleEditorDlg::OnSize(UINT nType, int cx, int cy)
{
	RepositionBars(AFX_IDW_CONTROLBAR_FIRST, AFX_IDW_CONTROLBAR_LAST, ID_INDICATOR_DPCM_SEGMENT);
	CDialog::OnSize(nType, cx, cy);
}

void CSampleEditorDlg::OnBnClickedPlay()
{
	if (!m_pSample) return;

	int Pitch = static_cast<CSliderCtrl*>(GetDlgItem(IDC_PITCH))->GetPos();
	m_pSoundGen->WriteAPU(0x4011, IsDlgButtonChecked(IDC_DELTASTART) ? 64 : 0);
	m_pSoundGen->PreviewSample(m_pSample, m_pSampleEditorView->GetStartOffset(), Pitch);
	// Wait for sample to play (at most 400ms)
	DWORD time = GetTickCount() + 400;
	while (m_pSoundGen->PreviewDone() == true && GetTickCount() < time);
	// Start play cursor timer
	SetTimer(TMR_PLAY_CURSOR, 10, NULL);
}

void CSampleEditorDlg::OnTimer(UINT_PTR nIDEvent)
{
	// Update play cursor

	switch (nIDEvent) {
		case TMR_PLAY_CURSOR: {
			// Play cursor
			stDPCMState state = m_pSoundGen->GetDPCMState();

			// Pos is in bytes
			int Pos = state.SamplePos /*<< 6*/;

			if (m_pSoundGen->PreviewDone()) {
				KillTimer(0);
				Pos = -1;
			}

			m_pSampleEditorView->DrawPlayCursor(Pos);
		}
		case TMR_START_CURSOR: {
			// Start cursor
			if (m_pSoundGen->PreviewDone()) {
				static bool bDraw = false;
				if (!bDraw)
					m_pSampleEditorView->DrawStartCursor();
				else
					m_pSampleEditorView->DrawPlayCursor(-1);
				bDraw = !bDraw;
			}
		}
	}

	CDialog::OnTimer(nIDEvent);
}

void CSampleEditorDlg::OnBnClickedDelete()
{
	// Make sure no sample is currently playing
	m_pSoundGen->CancelPreviewSample();

	if (!m_pSampleEditorView->HasSelection())
		return;

	unsigned int StartSample = m_pSampleEditorView->GetSelStart() * 16;
	unsigned int EndSample = m_pSampleEditorView->GetSelEnd() * 16;

	ASSERT(StartSample <= 4081);
	ASSERT(EndSample <= 4081);

	if (EndSample >= m_pSample->GetSize())
		EndSample = m_pSample->GetSize() - 1;

	TRACE(_T("Removing selected part from sample, start: %i, end %i (diff: %i)\n"), StartSample, EndSample, EndSample - StartSample);

	// Remove the selected part
	memcpy(m_pSample->GetData() + StartSample, m_pSample->GetData() + EndSample, m_pSample->GetSize() - EndSample);
	int NewSize = m_pSample->GetSize() - (EndSample - StartSample);

	// Reallocate
	char *pData = new char[NewSize];
	memcpy(pData, m_pSample->GetData(), NewSize);
	m_pSample->SetData(NewSize, pData);

	UpdateSampleView();
	SelectionChanged();
}

void CSampleEditorDlg::OnBnClickedTilt()
{
	if (!m_pSampleEditorView->HasSelection())
		return;

	int StartSample = m_pSampleEditorView->GetSelStart() * 16;
	int EndSample = m_pSampleEditorView->GetSelEnd() * 16;

	int Diff = EndSample - StartSample;

	int Nr = 10;
	int Step = (Diff * 8) / Nr;
	int Cntr = rand() % Step;
	char *pData = m_pSample->GetData();

	for (int i = StartSample; i < EndSample; ++i) {
		for (int j = 0; j < 8; ++j) {
			if (++Cntr == Step) {
				pData[i] &= (0xFF ^ (1 << j));
				Cntr = 0;
			}
		}
	}

	UpdateSampleView();
	SelectionChanged();
}

void CSampleEditorDlg::OnBnClickedBitReverse()
{
	m_pSoundGen->CancelPreviewSample();
	
	// using a lookup table for bit reversal
	static const unsigned char BitTable[] =
	{
	  0x00, 0x80, 0x40, 0xC0, 0x20, 0xA0, 0x60, 0xE0, 0x10, 0x90, 0x50, 0xD0, 0x30, 0xB0, 0x70, 0xF0,
	  0x08, 0x88, 0x48, 0xC8, 0x28, 0xA8, 0x68, 0xE8, 0x18, 0x98, 0x58, 0xD8, 0x38, 0xB8, 0x78, 0xF8,
	  0x04, 0x84, 0x44, 0xC4, 0x24, 0xA4, 0x64, 0xE4, 0x14, 0x94, 0x54, 0xD4, 0x34, 0xB4, 0x74, 0xF4,
	  0x0C, 0x8C, 0x4C, 0xCC, 0x2C, 0xAC, 0x6C, 0xEC, 0x1C, 0x9C, 0x5C, 0xDC, 0x3C, 0xBC, 0x7C, 0xFC,
	  0x02, 0x82, 0x42, 0xC2, 0x22, 0xA2, 0x62, 0xE2, 0x12, 0x92, 0x52, 0xD2, 0x32, 0xB2, 0x72, 0xF2,
	  0x0A, 0x8A, 0x4A, 0xCA, 0x2A, 0xAA, 0x6A, 0xEA, 0x1A, 0x9A, 0x5A, 0xDA, 0x3A, 0xBA, 0x7A, 0xFA,
	  0x06, 0x86, 0x46, 0xC6, 0x26, 0xA6, 0x66, 0xE6, 0x16, 0x96, 0x56, 0xD6, 0x36, 0xB6, 0x76, 0xF6,
	  0x0E, 0x8E, 0x4E, 0xCE, 0x2E, 0xAE, 0x6E, 0xEE, 0x1E, 0x9E, 0x5E, 0xDE, 0x3E, 0xBE, 0x7E, 0xFE,
	  0x01, 0x81, 0x41, 0xC1, 0x21, 0xA1, 0x61, 0xE1, 0x11, 0x91, 0x51, 0xD1, 0x31, 0xB1, 0x71, 0xF1,
	  0x09, 0x89, 0x49, 0xC9, 0x29, 0xA9, 0x69, 0xE9, 0x19, 0x99, 0x59, 0xD9, 0x39, 0xB9, 0x79, 0xF9,
	  0x05, 0x85, 0x45, 0xC5, 0x25, 0xA5, 0x65, 0xE5, 0x15, 0x95, 0x55, 0xD5, 0x35, 0xB5, 0x75, 0xF5,
	  0x0D, 0x8D, 0x4D, 0xCD, 0x2D, 0xAD, 0x6D, 0xED, 0x1D, 0x9D, 0x5D, 0xDD, 0x3D, 0xBD, 0x7D, 0xFD,
	  0x03, 0x83, 0x43, 0xC3, 0x23, 0xA3, 0x63, 0xE3, 0x13, 0x93, 0x53, 0xD3, 0x33, 0xB3, 0x73, 0xF3,
	  0x0B, 0x8B, 0x4B, 0xCB, 0x2B, 0xAB, 0x6B, 0xEB, 0x1B, 0x9B, 0x5B, 0xDB, 0x3B, 0xBB, 0x7B, 0xFB,
	  0x07, 0x87, 0x47, 0xC7, 0x27, 0xA7, 0x67, 0xE7, 0x17, 0x97, 0x57, 0xD7, 0x37, 0xB7, 0x77, 0xF7,
	  0x0F, 0x8F, 0x4F, 0xCF, 0x2F, 0xAF, 0x6F, 0xEF, 0x1F, 0x9F, 0x5F, 0xDF, 0x3F, 0xBF, 0x7F, 0xFF
	};
	char* InputData = new char[m_pSample->GetSize()];
	memcpy(InputData, m_pSample->GetData(), m_pSample->GetSize());
	char* DataCache = new char[m_pSample->GetSize()];
	// bit reverse each byte of InputData and save it to DataCache
	for (int i=0; i != m_pSample->GetSize(); i++) {
		char InputByte = InputData[i];
		char ByteCache = InputByte & 1;
		ByteCache = (BitTable[InputByte & 0xff]);
		DataCache[i] = ByteCache;
	}
	m_pSample->SetData(m_pSample->GetSize(), DataCache);

	UpdateSampleView();
	SelectionChanged();
}

void CSampleEditorDlg::OnBnClickedDeltastart()
{
	UpdateSampleView();
}

void CSampleEditorDlg::UpdateSampleView()
{
	m_pSampleEditorView->ExpandSample(m_pSample, IsDlgButtonChecked(IDC_DELTASTART) ? 64 : 0);
	m_pSampleEditorView->UpdateInfo();
	m_pSampleEditorView->Invalidate();
	m_pSampleEditorView->RedrawWindow();

	CSliderCtrl *pZoom = static_cast<CSliderCtrl*>(GetDlgItem(IDC_ZOOM));
	pZoom->SetRange(0, 20);		// // //
}

void CSampleEditorDlg::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	switch (nChar) {
		case VK_DELETE:
			OnBnClickedDelete();
			break;
		case VK_HOME:
			m_pSampleEditorView->OnHome();
			break;
		case VK_END:
			m_pSampleEditorView->OnEnd();
			break;
		case VK_RIGHT:
			m_pSampleEditorView->OnRight();
			break;
		case VK_LEFT:
			m_pSampleEditorView->OnLeft();
			break;
		case 'P':
			OnBnClickedPlay();
			break;
	}

	CDialog::OnKeyDown(nChar, nRepCnt, nFlags);
}

void CSampleEditorDlg::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
	int Pitch = static_cast<CSliderCtrl*>(GetDlgItem(IDC_PITCH))->GetPos();

	CString text;
	text.Format(_T("Pitch (%i)"), Pitch);
	SetDlgItemText(IDC_STATIC_PITCH, text);

	auto pZoom = static_cast<CSliderCtrl*>(GetDlgItem(IDC_ZOOM));		// // //
	float Zoom = static_cast<float>(pZoom->GetPos()) / pZoom->GetRangeMax();
	m_pSampleEditorView->SetZoom(1.0f - Zoom);
	m_pSampleEditorView->Invalidate();
	text.Format(_T("Zoom (%.2fx)"), 1. / m_pSampleEditorView->GetZoom());		// // //
	SetDlgItemText(IDC_STATIC_DPCM_ZOOM, text);

	CDialog::OnHScroll(nSBCode, nPos, pScrollBar);
}

BOOL CSampleEditorDlg::PreTranslateMessage(MSG* pMsg)
{
	if (pMsg->message == WM_KEYDOWN) {
		OnKeyDown(pMsg->wParam, 0, 0);
		return TRUE;
	}

	return CDialog::PreTranslateMessage(pMsg);
}

void CSampleEditorDlg::SelectionChanged()
{
	if (m_pSampleEditorView->HasSelection()) {
		GetDlgItem(IDC_DELETE)->EnableWindow(TRUE);
		GetDlgItem(IDC_TILT)->EnableWindow(TRUE);
	}
	else {
		GetDlgItem(IDC_DELETE)->EnableWindow(FALSE);
		GetDlgItem(IDC_TILT)->EnableWindow(FALSE);
	}
}

void CSampleEditorDlg::UpdateStatus(int Index, LPCTSTR Text)		// // //
{
	m_wndInfoStatusBar.SetPaneText(Index, Text);
}

void CSampleEditorDlg::OnGetMinMaxInfo(MINMAXINFO* lpMMI)
{
	lpMMI->ptMinTrackSize.x = 738;
	lpMMI->ptMinTrackSize.y = 456;
	CDialog::OnGetMinMaxInfo(lpMMI);
}
