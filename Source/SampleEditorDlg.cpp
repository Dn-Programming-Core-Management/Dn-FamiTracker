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
END_MESSAGE_MAP()


// CSampleEditorDlg message handlers

BOOL CSampleEditorDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	m_pSampleEditorView = new CSampleEditorView();
	m_pSampleEditorView->SubclassDlgItem(IDC_SAMPLE, this);

	CSliderCtrl *pitch = static_cast<CSliderCtrl*>(GetDlgItem(IDC_PITCH));
	pitch->SetRange(0, 15);
	pitch->SetPos(15);

	MoveControls();

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
	CDialog::OnSize(nType, cx, cy);
	MoveControls();
}

void CSampleEditorDlg::MoveControls()
{
	CRect rect;
	GetClientRect(&rect);

	if (m_pSampleEditorView) {
		rect.top++;
		rect.left++;
		rect.bottom -= 60;
		rect.right -= 2;
		m_pSampleEditorView->MoveWindow(rect);
		m_pSampleEditorView->Invalidate();
	}

	CWnd *control;
	CRect controlRect;

	GetClientRect(&rect);

	if (control = GetDlgItem(IDC_PLAY)) {
		control->GetClientRect(&controlRect);
		controlRect.MoveToXY(10, rect.bottom - 30);
		control->MoveWindow(controlRect);
	}

	if (control = GetDlgItem(IDC_POS)) {
		control->GetClientRect(&controlRect);
		controlRect.MoveToXY(5, rect.bottom - 53);
		controlRect.InflateRect(0, 0, 2, 2);
		control->MoveWindow(controlRect);
	}

	if (control = GetDlgItem(IDC_INFO)) {
		control->GetClientRect(&controlRect);
		controlRect.MoveToXY(170, rect.bottom - 53);
		controlRect.InflateRect(0, 0, 2, 2);
		controlRect.right = rect.right - 190;
		//controlRect.right = rect.right - 90;
		control->MoveWindow(controlRect);
	}

	if (control = GetDlgItem(IDC_ZOOM)) {
		control->GetClientRect(&controlRect);
		controlRect.MoveToXY(rect.right - 180, rect.bottom - 53);
		control->MoveWindow(controlRect);
	}

	if (control = GetDlgItem(IDC_STATIC_PITCH)) {
		control->GetClientRect(&controlRect);
		controlRect.MoveToXY(93, rect.bottom - 27);
		control->MoveWindow(controlRect);
	}

	if (control = GetDlgItem(IDC_PITCH)) {
		control->GetClientRect(&controlRect);
		controlRect.MoveToXY(145, rect.bottom - 30);
		control->MoveWindow(controlRect);
	}

	if (control = GetDlgItem(IDC_DELETE)) {
		control->GetClientRect(&controlRect);
		controlRect.MoveToXY(270, rect.bottom - 30);
		control->MoveWindow(controlRect);
	}

	if (control = GetDlgItem(IDC_TILT)) {
		control->GetClientRect(&controlRect);
		controlRect.MoveToXY(350, rect.bottom - 30);
		control->MoveWindow(controlRect);
	}

	if (control = GetDlgItem(IDC_DELTASTART)) {
		control->GetClientRect(&controlRect);
		controlRect.MoveToXY(440, rect.bottom - 25);
		control->MoveWindow(controlRect);
	}

	if (control = GetDlgItem(IDCANCEL)) {
		control->GetClientRect(&controlRect);
		controlRect.MoveToXY(rect.right - controlRect.right - 10, rect.bottom - 30);
		control->MoveWindow(controlRect);
	}

	if (control = GetDlgItem(IDOK)) {
		control->GetClientRect(&controlRect);
		controlRect.MoveToXY(rect.right - controlRect.right - 10, rect.bottom - 55);
		control->MoveWindow(controlRect);
	}

	Invalidate();
	RedrawWindow();
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
	pZoom->SetRange(0, 10);
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
		case 0x50:
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

	float Zoom = float(static_cast<CSliderCtrl*>(GetDlgItem(IDC_ZOOM))->GetPos()) * 0.1f;
	m_pSampleEditorView->SetZoom(1.0f - Zoom);
	m_pSampleEditorView->Invalidate();

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

void CSampleEditorDlg::OnGetMinMaxInfo(MINMAXINFO* lpMMI)
{
	lpMMI->ptMinTrackSize.x = 630;
	lpMMI->ptMinTrackSize.y = 400;
	CDialog::OnGetMinMaxInfo(lpMMI);
}
