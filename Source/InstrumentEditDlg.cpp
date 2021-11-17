/*
** FamiTracker - NES/Famicom sound tracker
** Copyright (C) 2005-2015 Jonathan Liss
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
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
** Library General Public License for more details. To obtain a
** copy of the GNU Library General Public License, write to the Free
** Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
**
** Any permitted reproduction of these routines, in whole or in part,
** must bear this legend.
*/

#include <memory>		// // //
#include <string>
#include "stdafx.h"
#include "FamiTracker.h"
#include "FamiTrackerDoc.h"
#include "SeqInstrument.h"		// // //
#include "Instrument2A03.h"		// // //
#include "InstrumentVRC6.h"		// // //
#include "InstrumentN163.h"		// // //
#include "InstrumentS5B.h"		// // //
#include "InstrumentFDS.h"		// // //
#include "InstrumentVRC7.h"		// // //
#include "FamiTrackerView.h"
#include "SequenceEditor.h"
#include "InstrumentEditPanel.h"
#include "InstrumentEditorSeq.h"		// // //
#include "InstrumentEditDlg.h"
#include "InstrumentEditorDPCM.h"
#include "InstrumentEditorVRC7.h"
#include "InstrumentEditorFDS.h"
#include "InstrumentEditorFDSEnvelope.h"
#include "InstrumentEditorN163Wave.h"
#include "InstrumentEditorVRC7Envelope.h"
#include "MainFrm.h"
#include "SoundGen.h"
#include "TrackerChannel.h"
#include "DPI.h"		// // //

// Constants
const int CInstrumentEditDlg::KEYBOARD_WIDTH  = 561;
const int CInstrumentEditDlg::KEYBOARD_HEIGHT = 58;

const TCHAR *CInstrumentEditDlg::CHIP_NAMES[] = {
	_T(""), 
	_T("2A03"), 
	_T("VRC6"), 
	_T("VRC7"), 
	_T("FDS"), 
	_T("Namco"), 
	_T("Sunsoft")
};

// CInstrumentEditDlg dialog

IMPLEMENT_DYNAMIC(CInstrumentEditDlg, CDialog)

CInstrumentEditDlg::CInstrumentEditDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CInstrumentEditDlg::IDD, pParent),
	m_bOpened(false),
	m_fRefreshRate(60.0f),		// // //
	m_iInstrument(-1),
	m_pInstManager(nullptr)		// // //
{
}

CInstrumentEditDlg::~CInstrumentEditDlg()
{
}

void CInstrumentEditDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CInstrumentEditDlg, CDialog)
	ON_NOTIFY(TCN_SELCHANGE, IDC_INST_TAB, OnTcnSelchangeInstTab)
	ON_WM_PAINT()
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
	ON_WM_MOUSEMOVE()
	ON_WM_LBUTTONDBLCLK()
	ON_WM_NCLBUTTONUP()
END_MESSAGE_MAP()


// CInstrumentEditDlg message handlers

BOOL CInstrumentEditDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	m_iSelectedInstType = -1;
	m_iLastKey = -1;
	m_bOpened = true;
	m_iPanels = 0;

	for (int i = 0; i < PANEL_COUNT; ++i)
		m_pPanels[i] = NULL;

	CRect r;		// // //
	GetClientRect(&r);
	int cx = r.Width(), bot = r.bottom;
	GetDlgItem(IDC_INST_TAB)->GetWindowRect(&r);
	GetDesktopWindow()->MapWindowPoints(this, &r);
	auto pKeyboard = GetDlgItem(IDC_KEYBOARD);
	m_KeyboardRect.left   = -1 + (cx - KEYBOARD_WIDTH) / 2;
	m_KeyboardRect.top    = -1 + r.bottom + (bot - r.bottom - KEYBOARD_HEIGHT) / 2;
	m_KeyboardRect.right  =  1 + m_KeyboardRect.left + KEYBOARD_WIDTH;
	m_KeyboardRect.bottom =  1 + m_KeyboardRect.top + KEYBOARD_HEIGHT;
	pKeyboard->MoveWindow(m_KeyboardRect);
	m_KeyboardRect.DeflateRect(1, 1, 1, 1);

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

void CInstrumentEditDlg::InsertPane(CInstrumentEditPanel *pPanel, bool Show)
{
	CRect Rect, ParentRect;
	CTabCtrl *pTabControl = static_cast<CTabCtrl*>(GetDlgItem(IDC_INST_TAB));

	pPanel->SetInstrumentManager(m_pInstManager);		// // //

	pTabControl->GetWindowRect(&ParentRect);
	pTabControl->InsertItem(m_iPanels, pPanel->GetTitle());

	pPanel->Create(pPanel->GetIDD(), this);
	pPanel->GetWindowRect(&Rect);
	Rect.MoveToXY(ParentRect.left - Rect.left + DPI::SX(1), ParentRect.top - Rect.top + DPI::SY(21));
	Rect.bottom -= DPI::SY(2);
	Rect.right += DPI::SX(1);
	pPanel->MoveWindow(Rect);
	pPanel->ShowWindow(Show ? SW_SHOW : SW_HIDE);

	if (Show) {
		pTabControl->SetCurSel(m_iPanels);
		pPanel->SetFocus();
		m_pFocusPanel = pPanel;
	}

	m_pPanels[m_iPanels++] = pPanel;
}

void CInstrumentEditDlg::ClearPanels()
{
	static_cast<CTabCtrl*>(GetDlgItem(IDC_INST_TAB))->DeleteAllItems();

	for (int i = 0; i < PANEL_COUNT; i++) {
		if (m_pPanels[i] != NULL) {
			m_pPanels[i]->DestroyWindow();
			SAFE_RELEASE(m_pPanels[i]);
		}
	}

	m_iPanels = 0;
	m_iInstrument = -1;
}

void CInstrumentEditDlg::SetCurrentInstrument(int Index)
{
	CFamiTrackerDoc *pDoc = CFamiTrackerDoc::GetDoc();
	std::shared_ptr<CInstrument> pInstrument = pDoc->GetInstrument(Index);
	int InstType = pInstrument->GetType();

	// Dialog title
	char Name[256];
	pDoc->GetInstrumentName(Index, Name);	
	CString Suffix;
	Suffix.Format(_T("%02X. %s (%s)"), Index, Name, CHIP_NAMES[InstType]);
	CString Title;
	AfxFormatString1(Title, IDS_INSTRUMENT_EDITOR_TITLE, Suffix);
	SetWindowText(Title);

	if (InstType != m_iSelectedInstType) {
		ShowWindow(SW_HIDE);
		ClearPanels();

		switch (InstType) {
			case INST_2A03: {
					int Channel = CFamiTrackerView::GetView()->GetSelectedChannel();
					int Type = pDoc->GetChannelType(Channel);
					bool bShowDPCM = (Type == CHANID_DPCM) || (std::static_pointer_cast<CInstrument2A03>(pInstrument)->AssignedSamples());
					InsertPane(new CInstrumentEditorSeq(NULL, _T("2A03 settings"), CInstrument2A03::SEQUENCE_NAME, 15, 3, INST_2A03), !bShowDPCM); // // //
					InsertPane(new CInstrumentEditorDPCM(), bShowDPCM);
				}
				break;
			case INST_VRC6:
				InsertPane(new CInstrumentEditorSeq(NULL, _T("Konami VRC6"), CInstrumentVRC6::SEQUENCE_NAME, 15, 7, INST_VRC6), true);
				break;
			case INST_VRC7:
				InsertPane(new CInstrumentEditorVRC7(), true);
				InsertPane(new CInstrumentEditorVRC7Envelope(), false);		// // //
				break;
			case INST_FDS:
				InsertPane(new CInstrumentEditorFDS(), true);
				InsertPane(new CInstrumentEditorFDSEnvelope(), false);
				break;
			case INST_N163:
				InsertPane(new CInstrumentEditorSeq(
					NULL, _T("Envelopes"), CInstrumentN163::SEQUENCE_NAME, 15, CInstrumentN163::MAX_WAVE_COUNT - 1, INST_N163
				), true);
				InsertPane(new CInstrumentEditorN163Wave(), false);
				break;
			case INST_S5B:
				InsertPane(new CInstrumentEditorSeq(NULL, _T("Sunsoft 5B"), CInstrumentS5B::SEQUENCE_NAME, 15, 255, INST_S5B), true);
				break;
		}

		m_iSelectedInstType = InstType;
	}

	for (int i = 0; i < PANEL_COUNT; ++i) {
		if (m_pPanels[i] != NULL) {
			m_pPanels[i]->SelectInstrument(pInstrument);
		}
	}

	ShowWindow(SW_SHOW);
	UpdateWindow();

	m_iSelectedInstType = InstType;
}

float CInstrumentEditDlg::GetRefreshRate() const		// // //
{
	return m_fRefreshRate;
}

void CInstrumentEditDlg::SetRefreshRate(float Rate)		// // //
{
	m_fRefreshRate = Rate;
}

void CInstrumentEditDlg::OnTcnSelchangeInstTab(NMHDR *pNMHDR, LRESULT *pResult)
{
	CTabCtrl *pTabControl = static_cast<CTabCtrl*>(GetDlgItem(IDC_INST_TAB));
	int Selection = pTabControl->GetCurSel();

	for (int i = 0; i < PANEL_COUNT; i++) {
		if (m_pPanels[i] != NULL && i != Selection) {
			m_pPanels[i]->ShowWindow(SW_HIDE);
		}
	}

	m_pPanels[Selection]->ShowWindow(SW_SHOW);

	m_pFocusPanel = m_pPanels[Selection];

	*pResult = 0;
}

/*!
	\brief Helper class to automatically select the previous drawing object when the context goes
	out of scope.
*/
struct CDCObjectContext		// // // TODO: put it somewhere else, maybe Graphics.h
{
	CDCObjectContext(CDC &dc, CGdiObject *obj) : _dc(dc)
	{
		_obj = dc.SelectObject(obj);
	}
	~CDCObjectContext()
	{
		_dc.SelectObject(_obj);
	}
private:
	CDC &_dc;
	CGdiObject *_obj;
};

void CInstrumentEditDlg::OnPaint()
{
	CPaintDC dc(this); // device context for painting
	// Do not call CDialog::OnPaint() for painting messages

	const int WHITE_KEY_W	= 10;
	const int BLACK_KEY_W	= 8;

	CBitmap Bmp, WhiteKeyBmp, BlackKeyBmp, WhiteKeyMarkBmp, BlackKeyMarkBmp;
	Bmp.CreateCompatibleBitmap(&dc, 800, 800);
	WhiteKeyBmp.LoadBitmap(IDB_KEY_WHITE);
	BlackKeyBmp.LoadBitmap(IDB_KEY_BLACK);
	WhiteKeyMarkBmp.LoadBitmap(IDB_KEY_WHITE_MARK);
	BlackKeyMarkBmp.LoadBitmap(IDB_KEY_BLACK_MARK);

	CDC BackDC;
	BackDC.CreateCompatibleDC(&dc);
	CDCObjectContext c {BackDC, &Bmp};		// // //

	CDC WhiteKey;
	WhiteKey.CreateCompatibleDC(&dc);
	CDCObjectContext c2 {WhiteKey, &WhiteKeyBmp};		// // //

	CDC BlackKey;
	BlackKey.CreateCompatibleDC(&dc);
	CDCObjectContext c3 {BlackKey, &BlackKeyBmp};		// // //
	
	const int WHITE[]	= {NOTE_C, NOTE_D, NOTE_E, NOTE_F, NOTE_G, NOTE_A, NOTE_B};
	const int BLACK_1[] = {NOTE_Cs, NOTE_Ds};
	const int BLACK_2[] = {NOTE_Fs, NOTE_Gs, NOTE_As};

	int Note = GET_NOTE(m_iActiveKey);		// // //
	int Octave = GET_OCTAVE(m_iActiveKey);

	for (int j = 0; j < 8; j++) {
		int Pos = (WHITE_KEY_W * 7) * j;

		for (int i = 0; i < 7; i++) {
			bool Selected = (Note == WHITE[i]) && (Octave == j) && m_iActiveKey != -1;
			WhiteKey.SelectObject(Selected ? WhiteKeyMarkBmp : WhiteKeyBmp);
			int Offset = i * WHITE_KEY_W;
			BackDC.BitBlt(Pos + Offset, 0, 100, 100, &WhiteKey, 0, 0, SRCCOPY);
		}

		for (int i = 0; i < 2; i++) {
			bool Selected = (Note == BLACK_1[i]) && (Octave == j) && m_iActiveKey != -1;
			BlackKey.SelectObject(Selected ? BlackKeyMarkBmp : BlackKeyBmp);
			int Offset = i * WHITE_KEY_W + WHITE_KEY_W / 2 + 1;
			BackDC.BitBlt(Pos + Offset, 0, 100, 100, &BlackKey, 0, 0, SRCCOPY);
		}

		for (int i = 0; i < 3; i++) {
			bool Selected = (Note == BLACK_2[i]) && (Octave == j) && m_iActiveKey != -1;
			BlackKey.SelectObject(Selected ? BlackKeyMarkBmp : BlackKeyBmp);
			int Offset = (i + 3) * WHITE_KEY_W + WHITE_KEY_W / 2 + 1;
			BackDC.BitBlt(Pos + Offset, 0, 100, 100, &BlackKey, 0, 0, SRCCOPY);
		}
	}

	dc.BitBlt(m_KeyboardRect.left, m_KeyboardRect.top, KEYBOARD_WIDTH, KEYBOARD_HEIGHT, &BackDC, 0, 0, SRCCOPY);		// // //
}

void CInstrumentEditDlg::ChangeNoteState(int Note)
{
	// A MIDI key number or -1 to disable

	m_iActiveKey = Note;

	if (m_hWnd)
		RedrawWindow(m_KeyboardRect, 0, RDW_INVALIDATE);		// // //
}

void CInstrumentEditDlg::SwitchOnNote(int x, int y)
{
	CFamiTrackerView *pView = CFamiTrackerView::GetView();
	CFamiTrackerDoc *pDoc = static_cast<CFamiTrackerDoc*>(static_cast<CFrameWnd*>(GetParent())->GetActiveDocument());
	CMainFrame *pFrameWnd = static_cast<CMainFrame*>(GetParent());
	int Channel = pView->GetSelectedChannel();		// // //
	int Chip = pDoc->GetExpansionChip();

	stChanNote NoteData { };

	// // // Send to respective channels whenever cursor is outside instrument chip
	if (m_iSelectedInstType == INST_2A03) {
		if (m_pPanels[0]->IsWindowVisible() && Channel > CHANID_NOISE)
			pView->SelectChannel(pDoc->GetChannelIndex(CHANID_SQUARE1));
		if (m_pPanels[1]->IsWindowVisible())
			pView->SelectChannel(pDoc->GetChannelIndex(CHANID_DPCM));
	}
	else {
		chan_id_t First = CHANNELS;
		switch (m_iSelectedInstType) {
		case INST_VRC6: First = CHANID_VRC6_PULSE1; break;
		case INST_N163: First = CHANID_N163_CH1; break;
		case INST_FDS:  First = CHANID_FDS; break;
		case INST_VRC7: First = CHANID_VRC7_CH1; break;
		case INST_S5B:  First = CHANID_S5B_CH1; break;
		}
		int Index = pDoc->GetChannelIndex(First);
		if (Index != -1 && pDoc->GetChipType(Index) != pDoc->GetChipType(Channel))
			pView->SelectChannel(Index);
	}
	Channel = pView->GetSelectedChannel();		// // //

	if (m_KeyboardRect.PtInRect({x, y})) {
		int KeyPos = (x - m_KeyboardRect.left) % 70;		// // //
		int Octave = (x - m_KeyboardRect.left) / 70;
		int Note;

		if (y > m_KeyboardRect.top + 38) {
			// Only white keys
			     if (KeyPos >= 60) Note = NOTE_B;
			else if (KeyPos >= 50) Note = NOTE_A;
			else if (KeyPos >= 40) Note = NOTE_G;
			else if (KeyPos >= 30) Note = NOTE_F;
			else if (KeyPos >= 20) Note = NOTE_E;
			else if (KeyPos >= 10) Note = NOTE_D;
			else if (KeyPos >=  0) Note = NOTE_C;
		}
		else {
			// Black and white keys
			     if (KeyPos >= 62) Note = NOTE_B;
			else if (KeyPos >= 56) Note = NOTE_As;
			else if (KeyPos >= 53) Note = NOTE_A;
			else if (KeyPos >= 46) Note = NOTE_Gs;
			else if (KeyPos >= 43) Note = NOTE_G;
			else if (KeyPos >= 37) Note = NOTE_Fs;
			else if (KeyPos >= 30) Note = NOTE_F;
			else if (KeyPos >= 23) Note = NOTE_E;
			else if (KeyPos >= 16) Note = NOTE_Ds;
			else if (KeyPos >= 13) Note = NOTE_D;
			else if (KeyPos >=  7) Note = NOTE_Cs;
			else if (KeyPos >=  0) Note = NOTE_C;
		}

		int NewNote = MIDI_NOTE(Octave, Note);		// // //
		if (NewNote != m_iLastKey) {
			NoteData.Note			= Note;
			NoteData.Octave			= Octave;
			NoteData.Vol			= MAX_VOLUME - 1;
			NoteData.Instrument		= pFrameWnd->GetSelectedInstrument();
			memset(NoteData.EffNumber, 0, 4);
			memset(NoteData.EffParam, 0, 4);

			theApp.GetSoundGenerator()->QueueNote(Channel, NoteData, NOTE_PRIO_2);
			theApp.GetSoundGenerator()->ForceReloadInstrument(Channel);		// // //
			m_iLastKey = NewNote;
		}
	}
	else {
		NoteData.Note			= pView->DoRelease() ? RELEASE : HALT;//HALT;
		NoteData.Vol			= MAX_VOLUME;
		NoteData.Instrument		= pFrameWnd->GetSelectedInstrument();;
		memset(NoteData.EffNumber, 0, 4);
		memset(NoteData.EffParam, 0, 4);

		theApp.GetSoundGenerator()->QueueNote(Channel, NoteData, NOTE_PRIO_2);

		m_iLastKey = -1;
	}
}

void CInstrumentEditDlg::SwitchOffNote(bool ForceHalt)
{
	stChanNote NoteData { };		// // //

	CFamiTrackerView *pView = CFamiTrackerView::GetView();
	CMainFrame *pFrameWnd = static_cast<CMainFrame*>(GetParent());

	int Channel = pView->GetSelectedChannel();

	NoteData.Note			= (pView->DoRelease() && !ForceHalt) ? RELEASE : HALT;
	NoteData.Instrument		= pFrameWnd->GetSelectedInstrument();

	theApp.GetSoundGenerator()->QueueNote(Channel, NoteData, NOTE_PRIO_2);

	m_iLastKey = -1;
}

void CInstrumentEditDlg::OnLButtonDown(UINT nFlags, CPoint point)
{
	SwitchOnNote(point.x, point.y);
	CDialog::OnLButtonDown(nFlags, point);
}

void CInstrumentEditDlg::OnLButtonUp(UINT nFlags, CPoint point)
{
	SwitchOffNote(false);
	CDialog::OnLButtonUp(nFlags, point);
}

void CInstrumentEditDlg::OnMouseMove(UINT nFlags, CPoint point)
{
	if (nFlags & MK_LBUTTON)
		SwitchOnNote(point.x, point.y);

	CDialog::OnMouseMove(nFlags, point);
}

void CInstrumentEditDlg::OnLButtonDblClk(UINT nFlags, CPoint point)
{
	SwitchOnNote(point.x, point.y);
	CDialog::OnLButtonDblClk(nFlags, point);
}

BOOL CInstrumentEditDlg::DestroyWindow()
{	
	ClearPanels();

	m_iSelectedInstType = -1;
	m_iInstrument = -1;
	m_bOpened = false;
	
	return CDialog::DestroyWindow();
}

void CInstrumentEditDlg::OnOK()
{
//	DestroyWindow();
//	CDialog::OnOK();
}

void CInstrumentEditDlg::OnCancel()
{
	DestroyWindow();
}

void CInstrumentEditDlg::OnNcLButtonUp(UINT nHitTest, CPoint point)
{
	// Mouse was released outside the client area
	SwitchOffNote(true);
	CDialog::OnNcLButtonUp(nHitTest, point);
}

bool CInstrumentEditDlg::IsOpened() const
{
	return m_bOpened;
}

void CInstrumentEditDlg::SetInstrumentManager(CInstrumentManager *pManager)
{
	m_pInstManager = pManager;
}

void CInstrumentEditDlg::PostNcDestroy()
{
	for (int i = 0; i < PANEL_COUNT; i++)		// // //
		SAFE_RELEASE(m_pPanels[i]);
	CDialog::PostNcDestroy();
}
