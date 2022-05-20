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

#include "stdafx.h"

#include "FamiTracker.h"
#include "FamiTrackerDoc.h"
#include "FamiTrackerView.h"
#include "MainFrm.h"
#include "MIDI.h"
#include "InstrumentEditDlg.h"
#include "SoundGen.h"
#include "PatternAction.h"
#include "PatternEditor.h"
#include "FrameEditor.h"
#include "Settings.h"
#include "Accelerator.h"
#include "TrackerChannel.h"
#include "Clipboard.h"
#include "APU/APU.h"
// // //
#include "Bookmark.h"
#include "BookmarkCollection.h"
#include "BookmarkManager.h"
#include "DetuneDlg.h"
#include "StretchDlg.h"
#include "RecordSettingsDlg.h"
#include "SplitKeyboardDlg.h"
#include "NoteQueue.h"

#include <cmath>
#include <assert.h>
#include <synchapi.h>  // CreateEvent

using std::get_if;

// Clipboard ID
const TCHAR CFamiTrackerView::CLIPBOARD_ID[] = _T("FamiTracker Pattern");

// Effect texts
// 0CC: add verbose description as in modplug
const CString EFFECT_TEXTS[] = {		// // //
	_T(""),
	_T("Fxx - Set speed to XX, cancels groove. If xx>=10, tempo must be fixed."),
	_T("Fxx - Set tempo to XX (if tempo not fixed)"),
	_T("Bxx - Jump to beginning of frame XX"),
	_T("Dxx - Skip to row XX of next frame"),
	_T("Cxx - Halt song"),
	_T("Exx - Set length counter index to XX"),
	_T("EEx - Set length counter mode, bit 0 = length counter, bit 1 = disable loop"),
	_T("3xx - Automatic portamento, XX = speed"),
	_T("(not used)"),
	_T("Hxy - Hardware sweep up, X = speed, Y = shift"),
	_T("Ixy - Hardware sweep down, X = speed, Y = shift"),
	_T("0xy - Arpeggio, X = second note, Y = third note (if zero, produces 2-frame period)"),
	_T("4xy - Vibrato, X = speed, Y = depth"),
	_T("7xy - Tremolo, X = speed, Y = depth"),
	_T("Pxx - Fine pitch, XX - 80 = offset"),
	_T("Gxx - Row delay, XX = number of frames"),
	_T("Zxx - DPCM delta counter setting, XX<=7F = DC bias"),
	_T("1xx - Slide up, XX = speed (pitch/frame)"),
	_T("2xx - Slide down, XX = speed (pitch/frame)"),
	_T("Vxx - Set Square duty / Noise mode to XX"),
	_T("Vxx - Set N163 wave index to XX"),
	_T("Vxx - Set VRC7 patch index to XX"),
	_T("Yxx - Set DPCM sample offset to XX<=3F"),
	_T("Qxy - Portamento up, X = speed, Y = notes"),
	_T("Rxy - Portamento down, X = speed, Y = notes"),
	_T("Axy - Volume slide, X = up, Y = down"),
	_T("Sxx - Note cut, XX = frames to wait"),
	_T("Sxx - Triangle channel linear counter, XX - 80 = duration"),
	_T("Xxx - DPCM retrigger, XX = frames to wait"),
	_T("Mxy - Delayed channel volume, X = frames to wait, Y = channel volume"),
	_T("Hxx - FDS modulation depth, XX = depth, 3F = highest"),
	_T("Hxx - Auto FDS modulation ratio, XX - 80 = multiplier"),
	_T("I0x - FDS modulation rate, high byte; disable auto modulation"),
	_T("Ixy - Auto FDS modulation, X = multiplier, Y + 1 = divider"),
	_T("Jxx - FDS modulation rate, low byte"),
	_T("W0x - DPCM pitch, F = highest"),
	_T("H0y - 5B envelope shape, bit 3 = Continue, bit 2 = Attack, bit 1 = Alternate, bit 0 = Hold"),
	_T("Hxy - Auto 5B envelope, X - 8 = shift amount, Y = shape"),
	_T("Ixx - 5B envelope rate, high byte"),
	_T("Jxx - 5B envelope rate, low byte"),
	_T("Wxx - 5B noise pitch, 1F = highest"),
	_T("Hxx - VRC7 custom patch port, XX = register address"),
	_T("Ixx - VRC7 custom patch write, XX = register value"),
	_T("Lxx - Note release, XX = frames to wait"),
	_T("Oxx - Set groove to XX"),
	_T("Txy - Delayed transpose (upward), X = frames to wait, Y = semitone offset"),
	_T("Txy - Delayed transpose (downward), X - 8 = frames to wait, Y = semitone offset"),
	_T("Zxx - N163 wave buffer access, XX = position in bytes"),
	_T("Exx - FDS volume envelope (attack), XX = rate"),
	_T("Exx - FDS volume envelope (decay), XX - 40 = rate"),
	_T("Zxx - Auto FDS modulation rate bias, XX - 80 = offset"),
	_T("=00 - Reset channel phase"),
	_T("Kxx - Multiply frequency by XX, does not affect FDS Ixy modulator"),
};

// OLE copy and mix
#define	DROPEFFECT_COPY_MIX	( 8 )

const int NOTE_HALT = -1;
const int NOTE_RELEASE = -2;
const int NOTE_ECHO = -16;		// // //

const int SINGLE_STEP = 1;				// Size of single step moves (default: 1)

// Timer IDs
enum {
	TMR_UPDATE,
	TMR_SCROLL
};

// CFamiTrackerView

IMPLEMENT_DYNCREATE(CFamiTrackerView, CView)

BEGIN_MESSAGE_MAP(CFamiTrackerView, CView)
	ON_WM_CREATE()
	ON_WM_ERASEBKGND()
	ON_WM_KILLFOCUS()
	ON_WM_SETFOCUS()
	ON_WM_TIMER()
	ON_WM_VSCROLL()
	ON_WM_HSCROLL()

	ON_WM_KEYDOWN()
	ON_WM_CHAR()
	ON_WM_KEYUP()

	ON_WM_MOUSEMOVE()
	ON_WM_MOUSEWHEEL()
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
	ON_WM_LBUTTONDBLCLK()
	ON_WM_RBUTTONUP()
	ON_WM_XBUTTONDOWN()		// // //
	ON_WM_MENUCHAR()
	ON_WM_SYSKEYDOWN()
	ON_COMMAND(ID_EDIT_PASTEMIX, OnEditPasteMix)
	ON_COMMAND(ID_EDIT_INSTRUMENTMASK, OnEditInstrumentMask)
	ON_COMMAND(ID_EDIT_VOLUMEMASK, OnEditVolumeMask)
	ON_COMMAND(ID_EDIT_INTERPOLATE, OnEditInterpolate)
	ON_COMMAND(ID_EDIT_REVERSE, OnEditReverse)
	ON_COMMAND(ID_EDIT_REPLACEINSTRUMENT, OnEditReplaceInstrument)
	ON_COMMAND(ID_TRANSPOSE_DECREASENOTE, OnTransposeDecreasenote)
	ON_COMMAND(ID_TRANSPOSE_DECREASEOCTAVE, OnTransposeDecreaseoctave)
	ON_COMMAND(ID_TRANSPOSE_INCREASENOTE, OnTransposeIncreasenote)
	ON_COMMAND(ID_TRANSPOSE_INCREASEOCTAVE, OnTransposeIncreaseoctave)
	ON_COMMAND(ID_DECREASEVALUES, OnDecreaseValues)
	ON_COMMAND(ID_INCREASEVALUES, OnIncreaseValues)
	ON_COMMAND(ID_TRACKER_PLAYROW, OnTrackerPlayrow)
	ON_COMMAND(ID_TRACKER_EDIT, OnTrackerEdit)
	ON_COMMAND(ID_TRACKER_TOGGLECHANNEL, OnTrackerToggleChannel)
	ON_COMMAND(ID_TRACKER_SOLOCHANNEL, OnTrackerSoloChannel)
	ON_COMMAND(ID_CMD_INCREASESTEPSIZE, OnIncreaseStepSize)
	ON_COMMAND(ID_CMD_DECREASESTEPSIZE, OnDecreaseStepSize)
	ON_COMMAND(ID_CMD_STEP_UP, OnOneStepUp)
	ON_COMMAND(ID_CMD_STEP_DOWN, OnOneStepDown)
	ON_COMMAND(ID_POPUP_TOGGLECHANNEL, OnTrackerToggleChannel)
	ON_COMMAND(ID_POPUP_SOLOCHANNEL, OnTrackerSoloChannel)
	ON_COMMAND(ID_POPUP_UNMUTEALLCHANNELS, OnTrackerUnmuteAllChannels)
//	ON_UPDATE_COMMAND_UI(ID_EDIT_CUT, OnUpdateEditCut)
//	ON_UPDATE_COMMAND_UI(ID_EDIT_DELETE, OnUpdateEditDelete)
	ON_UPDATE_COMMAND_UI(ID_EDIT_INSTRUMENTMASK, OnUpdateEditInstrumentMask)
	ON_UPDATE_COMMAND_UI(ID_EDIT_VOLUMEMASK, OnUpdateEditVolumeMask)
	ON_UPDATE_COMMAND_UI(ID_TRACKER_EDIT, OnUpdateTrackerEdit)
	ON_UPDATE_COMMAND_UI(ID_EDIT_PASTEMIX, OnUpdateEditPaste)
	ON_WM_NCMOUSEMOVE()
	ON_COMMAND(ID_BLOCK_START, OnBlockStart)
	ON_COMMAND(ID_BLOCK_END, OnBlockEnd)
	ON_COMMAND(ID_POPUP_PICKUPROW, OnPickupRow)
	ON_MESSAGE(WM_USER_MIDI_EVENT, OnUserMidiEvent)
	ON_MESSAGE(AM_PLAYER, OnUserPlayerEvent)
	ON_MESSAGE(AM_NOTE_EVENT, OnUserNoteEvent)
	ON_MESSAGE(AM_ERROR, &CFamiTrackerView::OnAudioThreadError)
	ON_WM_CLOSE()
	ON_WM_DESTROY()
	// // //
	ON_MESSAGE(AM_DUMP_INST, OnUserDumpInst)
	ON_COMMAND(ID_MODULE_DETUNE, OnTrackerDetune)
	ON_UPDATE_COMMAND_UI(ID_FIND_NEXT, OnUpdateFind)
	ON_UPDATE_COMMAND_UI(ID_FIND_PREVIOUS, OnUpdateFind)
	ON_COMMAND(ID_DECREASEVALUESCOARSE, OnCoarseDecreaseValues)
	ON_COMMAND(ID_INCREASEVALUESCOARSE, OnCoarseIncreaseValues)
//	ON_COMMAND(ID_EDIT_PASTEOVERWRITE, OnEditPasteOverwrite)
	ON_COMMAND(ID_EDIT_PASTEINSERT, OnEditPasteInsert)
//	ON_UPDATE_COMMAND_UI(ID_EDIT_PASTEOVERWRITE, OnUpdateEditPaste)
	ON_UPDATE_COMMAND_UI(ID_EDIT_PASTEINSERT, OnUpdateEditPaste)
	ON_COMMAND(ID_PASTESPECIAL_CURSOR, OnEditPasteSpecialCursor)
	ON_COMMAND(ID_PASTESPECIAL_SELECTION, OnEditPasteSpecialSelection)
	ON_COMMAND(ID_PASTESPECIAL_FILL, OnEditPasteSpecialFill)
	ON_UPDATE_COMMAND_UI(ID_PASTESPECIAL_CURSOR, OnUpdatePasteSpecial)
	ON_UPDATE_COMMAND_UI(ID_PASTESPECIAL_SELECTION, OnUpdatePasteSpecial)
	ON_UPDATE_COMMAND_UI(ID_PASTESPECIAL_FILL, OnUpdatePasteSpecial)
	ON_COMMAND(ID_BOOKMARKS_TOGGLE, OnBookmarksToggle)
	ON_COMMAND(ID_BOOKMARKS_NEXT, OnBookmarksNext)
	ON_COMMAND(ID_BOOKMARKS_PREVIOUS, OnBookmarksPrevious)
	ON_COMMAND(ID_EDIT_SPLITKEYBOARD, OnEditSplitKeyboard)
	ON_COMMAND(ID_TRACKER_TOGGLECHIP, OnTrackerToggleChip)
	ON_COMMAND(ID_TRACKER_SOLOCHIP, OnTrackerSoloChip)
	ON_COMMAND(ID_TRACKER_RECORDTOINST, OnTrackerRecordToInst)
	ON_COMMAND(ID_TRACKER_RECORDERSETTINGS, OnTrackerRecorderSettings)
	ON_UPDATE_COMMAND_UI(ID_TRACKER_RECORDTOINST, OnUpdateDisableWhilePlaying)
	ON_UPDATE_COMMAND_UI(ID_TRACKER_RECORDERSETTINGS, OnUpdateDisableWhilePlaying)
	ON_COMMAND(ID_RECALL_CHANNEL_STATE, OnRecallChannelState)
	ON_COMMAND(ID_DECAY_FAST, CMainFrame::OnDecayFast)		// // //
	ON_COMMAND(ID_DECAY_SLOW, CMainFrame::OnDecaySlow)		// // //
END_MESSAGE_MAP()

bool CFamiTrackerView::PostAudioMessage(AudioMessageId message, WPARAM wParam, LPARAM lParam)
{
	ASSERT(std::this_thread::get_id() == theApp.GetSoundGenerator()->m_audioThreadID);

	if (m_MessageQueue.try_push(AudioMessage{
		message,
		wParam,
		lParam,
	})) {
		SetEvent(m_hQueueEvent.get());
	}
	return false;
}

// Convert keys 0-F to numbers, -1 = invalid key
static int ConvertKeyToHex(Keycode Key)
{
	switch (Key) {
	case '0': case VK_NUMPAD0: return 0x00;
	case '1': case VK_NUMPAD1: return 0x01;
	case '2': case VK_NUMPAD2: return 0x02;
	case '3': case VK_NUMPAD3: return 0x03;
	case '4': case VK_NUMPAD4: return 0x04;
	case '5': case VK_NUMPAD5: return 0x05;
	case '6': case VK_NUMPAD6: return 0x06;
	case '7': case VK_NUMPAD7: return 0x07;
	case '8': case VK_NUMPAD8: return 0x08;
	case '9': case VK_NUMPAD9: return 0x09;
	case 'A': return 0x0A;
	case 'B': return 0x0B;
	case 'C': return 0x0C;
	case 'D': return 0x0D;
	case 'E': return 0x0E;
	case 'F': return 0x0F;

	// case KEY_DOT:
	// case KEY_DASH:
	//	return 0x80;
	}

	return -1;
}

static int ConvertKeyExtra(Keycode Key)		// // //
{
	switch (Key) {
		case VK_DIVIDE:   return 0x0A;
		case VK_MULTIPLY: return 0x0B;
		case VK_SUBTRACT: return 0x0C;
		case VK_ADD:      return 0x0D;
		case VK_RETURN:   return 0x0E;
		case VK_DECIMAL:  return 0x0F;
	}

	return -1;
}

// CFamiTrackerView construction/destruction

CFamiTrackerView::CFamiTrackerView() :
	m_MessageQueue(8192),
	mClipboardFormat(0),
	m_iMenuChannel(-1),
	m_iInsertKeyStepping(1),
	m_bEditEnable(false),
	m_bSwitchToInstrument(false),
	m_bFollowMode(true),
	m_bCompactMode(false),		// // //
	m_bMaskInstrument(false),		// // //
	m_bMaskVolume(true),
	m_iPastePos(PASTE_CURSOR),
	m_iSwitchToInstrument(-1),		// // //
	m_iMarkerFrame(-1),		// // //
	m_iMarkerRow(-1),
	m_iAutoArpNotes(),
	m_iAutoArpPtr(0),		// // //
	m_iLastAutoArpPtr(0),		// // // 050B
	m_iAutoArpKeyCount(0),		// // // 050B
	m_iKeyboardNote(-1),		// // //
	m_iLastNote(NONE),
	m_iLastInstrument(0),
	m_iLastVolume(MAX_VOLUME),
	m_iLastEffect(EF_NONE),		// // //
	m_iLastEffectParam(0),		// // //
	m_iSplitNote(-1),		// // //
	m_iSplitChannel(-1),		// // //
	m_iSplitInstrument(MAX_INSTRUMENTS),		// // //
	m_iSplitTranspose(0),		// // //
	m_iNoteCorrection(),
	m_pNoteQueue(new CNoteQueue { }),
	m_pPatternEditor(new CPatternEditor()),
	m_nDropEffect(DROPEFFECT_NONE),
	m_bDragSource(false)
{
	m_hQuitEvent = HandlePtr(::CreateEvent(NULL, FALSE, FALSE, NULL));
	ASSERT(m_hQuitEvent);

	m_hQueueEvent = HandlePtr(::CreateEvent(NULL, FALSE, FALSE, NULL));
	ASSERT(m_hQueueEvent);

	memset(m_bMuteChannels, 0, sizeof(bool) * MAX_CHANNELS);
	memset(m_iActiveNotes, 0, sizeof(int) * MAX_CHANNELS);
	memset(m_cKeyList, 0, sizeof(char) * 256);
	memset(m_iArpeggiate, 0, sizeof(int) * MAX_CHANNELS);

	// Register this object in the sound generator
	CSoundGen *pSoundGen = theApp.GetSoundGenerator();
	ASSERT(pSoundGen);

	m_ReceiveThread = std::thread([this]() {
		HANDLE events[2] = {
			m_hQueueEvent.get(),
			m_hQuitEvent.get(),
		};
		while (true) {
			while (auto msg = m_MessageQueue.front()) {
				m_MessageQueue.pop();
				PostMessage(msg->message, msg->wParam, msg->lParam);
			}

			if (WaitForMultipleObjects(2, events, FALSE, INFINITE) != WAIT_OBJECT_0) {
				break;
			}
		}
	});

	pSoundGen->AssignView(this);
}

CFamiTrackerView::~CFamiTrackerView()
{
	SetEvent(m_hQuitEvent.get());
	m_ReceiveThread.join();

	// Release allocated objects
	SAFE_RELEASE(m_pPatternEditor);
	SAFE_RELEASE(m_pNoteQueue);		// // //
}



// CFamiTrackerView diagnostics

#ifdef _DEBUG
void CFamiTrackerView::AssertValid() const
{
	CView::AssertValid();
}

void CFamiTrackerView::Dump(CDumpContext& dc) const
{
	CView::Dump(dc);
}

CFamiTrackerDoc* CFamiTrackerView::GetDocument() const // non-debug version is inline
{
	ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CFamiTrackerDoc)));
	return (CFamiTrackerDoc*)m_pDocument;
}
#endif //_DEBUG


//
// Static functions
//

CFamiTrackerView *CFamiTrackerView::GetView()
{
	CFrameWnd *pFrame = static_cast<CFrameWnd*>(AfxGetApp()->m_pMainWnd);
	ASSERT_VALID(pFrame);

	CView *pView = pFrame->GetActiveView();

	if (!pView)
		return NULL;

	// Fail if view is of wrong kind
	// (this could occur with splitter windows, or additional
	// views on a single document
	if (!pView->IsKindOf(RUNTIME_CLASS(CFamiTrackerView)))
		return NULL;

	return static_cast<CFamiTrackerView*>(pView);
}

// Creation / destroy

int CFamiTrackerView::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CView::OnCreate(lpCreateStruct) == -1)
		return -1;

	TimerDelayer = 100;
	// Install a timer for screen updates, 20ms
	SetTimer(TMR_UPDATE, TimerDelayer, NULL);

	m_DropTarget.Register(this);

	// Setup pattern editor
	m_pPatternEditor->ApplyColorScheme();

	// Create clipboard format
	mClipboardFormat = ::RegisterClipboardFormat(CLIPBOARD_ID);

	if (mClipboardFormat == 0)
		AfxMessageBox(IDS_CLIPBOARD_ERROR, MB_ICONERROR);

	return 0;
}

void CFamiTrackerView::OnDestroy()
{
	// Kill timers
	KillTimer(TMR_UPDATE);
	KillTimer(TMR_SCROLL);

	CView::OnDestroy();
}


// CFamiTrackerView message handlers

////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Tracker drawing routines
////////////////////////////////////////////////////////////////////////////////////////////////////////////

void CFamiTrackerView::OnDraw(CDC* pDC)
{
	// How should we protect the DC in this method?

	CFamiTrackerDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	// Check document
	if (!pDoc->IsFileLoaded()) {
		LPCTSTR str = _T("No module loaded.");
		pDC->FillSolidRect(0, 0, m_iWindowWidth, m_iWindowHeight, 0x000000);
		pDC->SetTextColor(0xFFFFFF);
		CRect textRect(0, 0, m_iWindowWidth, m_iWindowHeight);
		pDC->DrawText(str, _tcslen(str), &textRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
		return;
	}

	// Don't draw when rendering to wave file
	CSoundGen *pSoundGen = theApp.GetSoundGenerator();
	if (pSoundGen == NULL || pSoundGen->IsBackgroundTask())
		return;

	m_pPatternEditor->DrawScreen(pDC, this);
}

BOOL CFamiTrackerView::OnEraseBkgnd(CDC* pDC)
{
	CFamiTrackerDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	// Check document
	if (!pDoc->IsFileLoaded())
		return FALSE;

	// Called when the background should be erased
	m_pPatternEditor->CreateBackground(pDC);

	return FALSE;
}

void CFamiTrackerView::SetupColors()
{
	// Color scheme has changed
	CMainFrame *pMainFrame = static_cast<CMainFrame*>(GetParentFrame());
	m_pPatternEditor->ApplyColorScheme();

	m_pPatternEditor->InvalidatePatternData();
	m_pPatternEditor->InvalidateBackground();
	RedrawPatternEditor();

	// Frame editor
	CFrameEditor *pFrameEditor = GetFrameEditor();
	pFrameEditor->SetupColors();
	pFrameEditor->RedrawFrameEditor();

	pMainFrame->RedrawWindow(NULL, NULL, RDW_INVALIDATE | RDW_ERASE);
}

void CFamiTrackerView::UpdateMeters()
{
	// TODO: Change this to use the ordinary drawing routines
	{
		// "what is the use of a mutex that only one thread locks?"
		// "what is the sound of one hand clapping?"
		std::unique_lock<std::mutex> lock(m_csDrawLock);

		CDC* pDC = GetDC();
		if (pDC && pDC->m_hDC) {
			m_pPatternEditor->DrawMeters(pDC);
			ReleaseDC(pDC);
		}
	}
}

void CFamiTrackerView::InvalidateCursor()
{
	// Cursor has moved, redraw screen
	m_pPatternEditor->InvalidateCursor();
	RedrawPatternEditor();
	RedrawFrameEditor();

	static CCursorPos LastPosition { };		// // //
	CCursorPos p = m_pPatternEditor->GetCursor();
	if (memcmp(&p, &LastPosition, sizeof(CCursorPos)))
	{
		LastPosition = p;
		static_cast<CMainFrame*>(GetParentFrame())->ResetFind();		// // //
	}
}

void CFamiTrackerView::InvalidateHeader()
{
	// Header area has changed (channel muted etc...)
	m_pPatternEditor->InvalidateHeader();
	RedrawWindow(m_pPatternEditor->GetHeaderRect(), NULL, RDW_INVALIDATE);
}

void CFamiTrackerView::InvalidatePatternEditor()
{
	// Pattern data has changed, redraw screen
	RedrawPatternEditor();
	// ??? TODO do we need this??
	RedrawFrameEditor();
}

void CFamiTrackerView::InvalidateFrameEditor()
{
	// Frame data has changed, redraw frame editor

	CFrameEditor *pFrameEditor = GetFrameEditor();
	pFrameEditor->InvalidateFrameData();

	RedrawFrameEditor();
	// Update pattern editor according to selected frame
	RedrawPatternEditor();
}

void CFamiTrackerView::RedrawPatternEditor()
{
	// Redraw the pattern editor, partial or full if needed
	bool NeedErase = m_pPatternEditor->CursorUpdated();

	if (NeedErase)
		RedrawWindow(NULL, NULL, RDW_INVALIDATE | RDW_ERASE);
	else
		RedrawWindow(m_pPatternEditor->GetInvalidatedRect(), NULL, RDW_INVALIDATE);
}

void CFamiTrackerView::RedrawFrameEditor()
{
	// Redraw the frame editor
	CFrameEditor *pFrameEditor = GetFrameEditor();
	pFrameEditor->RedrawFrameEditor();
}

CFrameEditor *CFamiTrackerView::GetFrameEditor() const
{
	CMainFrame *pMainFrame = static_cast<CMainFrame*>(GetParentFrame());
	ASSERT_VALID(pMainFrame);
	CFrameEditor *pFrameEditor = pMainFrame->GetFrameEditor();
	ASSERT_VALID(pFrameEditor);
	return pFrameEditor;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////
// General
////////////////////////////////////////////////////////////////////////////////////////////////////////////

LRESULT CFamiTrackerView::OnUserMidiEvent(WPARAM wParam, LPARAM lParam)
{
	TranslateMidiMessage();
	return 0;
}

LRESULT CFamiTrackerView::OnUserPlayerEvent(WPARAM wParam, LPARAM lParam)
{
	// Player is playing
	// TODO clean up
	int Frame = (int)wParam;
	int Row = (int)lParam;

	m_pPatternEditor->InvalidateCursor();
	RedrawPatternEditor();

	//m_pPatternEditor->AdjustCursor();	// Fix frame editor cursor
	RedrawFrameEditor();
	return 0;
}

LRESULT CFamiTrackerView::OnUserNoteEvent(WPARAM wParam, LPARAM lParam)
{
	int Channel = wParam;
	int Note = lParam;

	RegisterKeyState(Channel, Note);

	return 0;
}

LRESULT CFamiTrackerView::OnAudioThreadError(WPARAM wParam, LPARAM lParam)
{
	AfxMessageBox((UINT)wParam, (UINT)lParam);
	return 0;
}

void CFamiTrackerView::CalcWindowRect(LPRECT lpClientRect, UINT nAdjustType)
{
	// Window size has changed
	m_iWindowWidth	= lpClientRect->right - lpClientRect->left;
	m_iWindowHeight	= lpClientRect->bottom - lpClientRect->top;

	m_iWindowWidth  -= ::GetSystemMetrics(SM_CXEDGE) * 2;
	m_iWindowHeight -= ::GetSystemMetrics(SM_CYEDGE) * 2;

	m_pPatternEditor->SetWindowSize(m_iWindowWidth, m_iWindowHeight);
	m_pPatternEditor->InvalidateBackground();
	// Update cursor since first visible channel might change
	m_pPatternEditor->CursorUpdated();

	CView::CalcWindowRect(lpClientRect, nAdjustType);
}

// Scroll

void CFamiTrackerView::OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
	CFamiTrackerDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	m_pPatternEditor->OnVScroll(nSBCode, nPos);
	InvalidateCursor();

	CView::OnVScroll(nSBCode, nPos, pScrollBar);
}

void CFamiTrackerView::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
	CFamiTrackerDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	m_pPatternEditor->OnHScroll(nSBCode, nPos);
	InvalidateCursor();

	CView::OnHScroll(nSBCode, nPos, pScrollBar);
}

// Mouse

void CFamiTrackerView::OnRButtonUp(UINT nFlags, CPoint point)
{
	// Popup menu
	CRect WinRect;
	CMenu *pPopupMenu, PopupMenuBar;

	if (m_pPatternEditor->CancelDragging()) {
		InvalidateCursor();
		CView::OnRButtonUp(nFlags, point);
		return;
	}

	m_pPatternEditor->OnMouseRDown(point);

	GetWindowRect(WinRect);

	if (m_pPatternEditor->IsOverHeader(point)) {
		// Pattern header
		m_iMenuChannel = m_pPatternEditor->GetChannelAtPoint(point.x);
		PopupMenuBar.LoadMenu(IDR_PATTERN_HEADER_POPUP);
		static_cast<CMainFrame*>(theApp.GetMainWnd())->UpdateMenu(&PopupMenuBar);
		pPopupMenu = PopupMenuBar.GetSubMenu(0);
		pPopupMenu->EnableMenuItem(ID_TRACKER_RECORDTOINST, theApp.IsPlaying() ? MF_DISABLED : MF_ENABLED);		// // //
		pPopupMenu->EnableMenuItem(ID_TRACKER_RECORDERSETTINGS, theApp.IsPlaying() ? MF_DISABLED : MF_ENABLED);		// // //
		CMenu *pMeterMenu = pPopupMenu->GetSubMenu(6);		// // // 050B
		int Rate = theApp.GetSoundGenerator()->GetMeterDecayRate();
		pMeterMenu->CheckMenuItem(Rate == DECAY_FAST ? ID_DECAY_FAST : ID_DECAY_SLOW, MF_CHECKED | MF_BYCOMMAND);
		pPopupMenu->TrackPopupMenu(TPM_RIGHTBUTTON, point.x + WinRect.left, point.y + WinRect.top, this);
	}
	else if (m_pPatternEditor->IsOverPattern(point)) {		// // // 050B todo
		// Pattern area
		m_iMenuChannel = -1;
		PopupMenuBar.LoadMenu(IDR_PATTERN_POPUP);
		static_cast<CMainFrame*>(theApp.GetMainWnd())->UpdateMenu(&PopupMenuBar);
		pPopupMenu = PopupMenuBar.GetSubMenu(0);
		// Send messages to parent in order to get the menu options working
		pPopupMenu->TrackPopupMenu(TPM_RIGHTBUTTON, point.x + WinRect.left, point.y + WinRect.top, GetParentFrame());
	}

	CView::OnRButtonUp(nFlags, point);
}

void CFamiTrackerView::OnLButtonDown(UINT nFlags, CPoint point)
{
	SetTimer(TMR_SCROLL, 10, NULL);

	m_pPatternEditor->OnMouseDown(point);
	SetCapture();	// Capture mouse
	InvalidateCursor();

	if (m_pPatternEditor->IsOverHeader(point))
		InvalidateHeader();

	CView::OnLButtonDown(nFlags, point);
}

void CFamiTrackerView::OnLButtonUp(UINT nFlags, CPoint point)
{
	KillTimer(TMR_SCROLL);

	m_pPatternEditor->OnMouseUp(point);
	ReleaseCapture();	// Release mouse

	InvalidateCursor();
	InvalidateHeader();

	/*
	if (m_bControlPressed && !m_pPatternEditor->IsSelecting()) {
		m_pPatternEditor->JumpToRow(m_pPatternEditor->GetRow());
		theApp.GetSoundGenerator()->StartPlayer(MODE_PLAY_CURSOR);
	}
	*/

	CView::OnLButtonUp(nFlags, point);
}

void CFamiTrackerView::OnLButtonDblClk(UINT nFlags, CPoint point)
{
	if (theApp.GetSettings()->General.bDblClickSelect && !m_pPatternEditor->IsOverHeader(point))
		return;

	m_pPatternEditor->OnMouseDblClk(point);
	InvalidateCursor();
	CView::OnLButtonDblClk(nFlags, point);
}

void CFamiTrackerView::OnXButtonDown(UINT nFlags, UINT nButton, CPoint point)		// // //
{
	/*
	switch (nFlags & (MK_CONTROL | MK_SHIFT)) {
	case MK_CONTROL:
		if (nButton == XBUTTON2)
			OnEditCopy();
		else if (nButton == XBUTTON1)
			OnEditPaste();
		break;
	case MK_SHIFT:
		m_iMenuChannel = m_pPatternEditor->GetChannelAtPoint(point.x);
		if (nButton == XBUTTON2)
			OnTrackerToggleChannel();
		else if (nButton == XBUTTON1)
			OnTrackerSoloChannel();
		break;
	default:
		if (nButton == XBUTTON2)
			static_cast<CMainFrame*>(GetParentFrame())->OnEditRedo();
		else if (nButton == XBUTTON1)
			static_cast<CMainFrame*>(GetParentFrame())->OnEditUndo();
	}
	*/
	CView::OnXButtonDown(nFlags, nButton, point);
}

void CFamiTrackerView::OnMouseMove(UINT nFlags, CPoint point)
{
	CFamiTrackerDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	if (nFlags & MK_LBUTTON) {
		// Left button down
		m_pPatternEditor->OnMouseMove(nFlags, point);
		InvalidateCursor();
	}
	else {
		// Left button up
		if (m_pPatternEditor->OnMouseHover(nFlags, point))
			InvalidateHeader();
	}

	CView::OnMouseMove(nFlags, point);
}

BOOL CFamiTrackerView::OnMouseWheel(UINT nFlags, short zDelta, CPoint pt)
{
	CPatternAction *pAction = NULL;

	bool bShiftPressed = IsShiftPressed();
	bool bControlPressed = IsControlPressed();

	if (bControlPressed && bShiftPressed) {
		if (zDelta < 0)
			m_pPatternEditor->NextFrame();
		else
			m_pPatternEditor->PreviousFrame();
		InvalidateFrameEditor();		// // //
	}
	else if (bControlPressed) {
		if (m_bEditEnable && !(theApp.IsPlaying() && !IsSelecting() && m_bFollowMode))		// // //
			pAction = new CPActionTranspose {zDelta > 0 ? TRANSPOSE_INC_NOTES : TRANSPOSE_DEC_NOTES};		// // //
	}
	else if (bShiftPressed) {
		if (m_bEditEnable && !(theApp.IsPlaying() && !IsSelecting() && m_bFollowMode))
			pAction = new CPActionScrollValues {zDelta > 0 ? 1 : -1};		// // //
	}
	else
		m_pPatternEditor->OnMouseScroll(zDelta);

	if (pAction != NULL) {
		AddAction(pAction);
	}
	else
		InvalidateCursor();

	return CView::OnMouseWheel(nFlags, zDelta, pt);
}

// End of mouse

void CFamiTrackerView::OnKillFocus(CWnd* pNewWnd)
{
	CView::OnKillFocus(pNewWnd);
	m_bHasFocus = false;
	m_pPatternEditor->SetFocus(false);
	InvalidateCursor();
}

void CFamiTrackerView::OnSetFocus(CWnd* pOldWnd)
{
	CView::OnSetFocus(pOldWnd);
	m_bHasFocus = true;
	m_pPatternEditor->SetFocus(true);
	CMainFrame *pMainFrm = static_cast<CMainFrame*>(GetParentFrame());		// // //
	pMainFrm->GetFrameEditor()->CancelSelection();
	InvalidateCursor();
}

void CFamiTrackerView::OnTimer(UINT_PTR nIDEvent)
{
	// Timer callback function
	if (theApp.IsPlaying()) {
		KillTimer(TMR_UPDATE);
		TimerDelayer = 16;
		SetTimer(TMR_UPDATE, TimerDelayer, NULL);
	}
	else {
		KillTimer(TMR_UPDATE);
		TimerDelayer = theApp.GetSettings()->GUI.iLowRefreshRate;
		SetTimer(TMR_UPDATE, TimerDelayer, NULL);
	}
	switch (nIDEvent) {
		// Drawing updates when playing
		case TMR_UPDATE:
			PeriodicUpdate();
			break;

		// Auto-scroll timer
		case TMR_SCROLL:
			if (m_pPatternEditor->ScrollTimerCallback()) {
				// Redraw entire since pattern layout might change
				RedrawPatternEditor();
			}
			break;
	}

	CView::OnTimer(nIDEvent);
}

void CFamiTrackerView::PeriodicUpdate()
{
	// Called periodically by an background timer

	CFamiTrackerDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	CMainFrame *pMainFrm = static_cast<CMainFrame*>(GetParentFrame());
	ASSERT_VALID(pMainFrm);

	CSoundGen *pSoundGen = theApp.GetSoundGenerator();

	if (pSoundGen != NULL) {
		// Skip updates when doing background tasks (WAV render for example)
		if (!pSoundGen->IsBackgroundTask()) {

			int PlayTicks = pSoundGen->GetPlayerTicks();
			int PlayTime = (PlayTicks * 10) / pDoc->GetFrameRate();

			// Play time
			int Min = PlayTime / 600;
			int Sec = (PlayTime / 10) % 60;
			int mSec = PlayTime % 10;

			pMainFrm->SetIndicatorTime(Min, Sec, mSec);

			int Frame = pSoundGen->GetPlayerFrame();
			int Row = pSoundGen->GetPlayerRow();

			pMainFrm->SetIndicatorPos(Frame, Row);

			// DPCM info
			stDPCMState DPCMState = pSoundGen->GetDPCMState();
			m_pPatternEditor->SetDPCMState(DPCMState);

			if (pDoc->IsFileLoaded()) {
				UpdateMeters();
				// // //
			}
		}
	}

	// TODO get rid of static variables
	static int LastNoteState;

	if (LastNoteState != m_iKeyboardNote)
		pMainFrm->ChangeNoteState(m_iKeyboardNote);

	LastNoteState = m_iKeyboardNote;

	// Switch instrument
	if (m_iSwitchToInstrument != -1) {
		SetInstrument(m_iSwitchToInstrument);
		m_iSwitchToInstrument = -1;
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Menu commands
////////////////////////////////////////////////////////////////////////////////////////////////////////////

void CFamiTrackerView::OnEditCopy()
{
	if (m_pPatternEditor->GetSelectionCondition() == SEL_NONTERMINAL_SKIP) {		// // //
		CMainFrame *pMainFrm = static_cast<CMainFrame*>(GetParentFrame());
		ASSERT_VALID(pMainFrm);
		MessageBeep(MB_ICONWARNING);
		pMainFrm->SetMessageText(IDS_SEL_NONTERMINAL_SKIP);
		return;
	}

	std::shared_ptr<CPatternClipData> pClipData(m_pPatternEditor->Copy());		// // //

	CClipboard Clipboard(this, mClipboardFormat);

	if (!Clipboard.IsOpened()) {
		AfxMessageBox(IDS_CLIPBOARD_OPEN_ERROR);
		return;
	}

	SIZE_T Size = pClipData->GetAllocSize();
	HGLOBAL hMem = Clipboard.AllocMem(Size);

	if (hMem != NULL) {
		pClipData->ToMem(hMem);
		// Set clipboard for internal data, hMem may not be used after this point
		Clipboard.SetData(hMem);
	}
}

void CFamiTrackerView::OnEditCut()
{
	if (!m_bEditEnable) return;		// // //
	OnEditCopy();
	OnEditDelete();
}

void CFamiTrackerView::OnEditPaste()
{
	if (!m_bEditEnable) return;		// // //
	CClipboard Clipboard(this, mClipboardFormat);
	HGLOBAL hMem;		// // //
	if (!Clipboard.GetData(hMem))
		return;

	CPatternClipData *pClipData = new CPatternClipData();
	pClipData->FromMem(hMem);
	// Create an undo point
	CPatternAction *pAction = new CPatternAction(CPatternAction::ACT_EDIT_PASTE);
	pAction->SetPaste(pClipData);
	pAction->SetPasteMode(PASTE_DEFAULT);		// // //
	pAction->SetPastePos(m_iPastePos);
	AddAction(pAction);
}

void CFamiTrackerView::OnEditPasteMix()		// // //
{
	if (!m_bEditEnable) return;		// // //
	CClipboard Clipboard(this, mClipboardFormat);
	HGLOBAL hMem;		// // //
	if (!Clipboard.GetData(hMem))
		return;

	CPatternClipData *pClipData = new CPatternClipData();
	pClipData->FromMem(hMem);

	// Add an undo point
	CPatternAction *pAction = new CPatternAction(CPatternAction::ACT_EDIT_PASTE);		// // //
	pAction->SetPaste(pClipData);
	pAction->SetPasteMode(PASTE_MIX);		// // //
	pAction->SetPastePos(m_iPastePos);
	AddAction(pAction);
}

void CFamiTrackerView::OnEditPasteOverwrite()		// // //
{
	if (!m_bEditEnable) return;
	CClipboard Clipboard(this, mClipboardFormat);
	HGLOBAL hMem;		// // //
	if (!Clipboard.GetData(hMem))
		return;

	CPatternClipData *pClipData = new CPatternClipData();
	pClipData->FromMem(hMem);

	// Add an undo point
	CPatternAction *pAction = new CPatternAction(CPatternAction::ACT_EDIT_PASTE);
	pAction->SetPaste(pClipData);
	pAction->SetPasteMode(PASTE_OVERWRITE);
	pAction->SetPastePos(m_iPastePos);
	AddAction(pAction);
}

void CFamiTrackerView::OnEditPasteInsert()		// // //
{
	if (!m_bEditEnable) return;
	CClipboard Clipboard(this, mClipboardFormat);
	HGLOBAL hMem;		// // //
	if (!Clipboard.GetData(hMem))
		return;

	CPatternClipData *pClipData = new CPatternClipData();
	pClipData->FromMem(hMem);

	// Add an undo point
	CPatternAction *pAction = new CPatternAction(CPatternAction::ACT_EDIT_PASTE);
	pAction->SetPaste(pClipData);
	pAction->SetPasteMode(PASTE_INSERT);
	pAction->SetPastePos(m_iPastePos);
	AddAction(pAction);
}

void CFamiTrackerView::OnEditDelete()
{
	if (!m_bEditEnable) return;		// // //
	AddAction(new CPActionClearSel { });		// // //
}

void CFamiTrackerView::OnTrackerEdit()
{
	m_bEditEnable = !m_bEditEnable;

	if (m_bEditEnable)
		GetParentFrame()->SetMessageText(IDS_EDIT_MODE);
	else
		GetParentFrame()->SetMessageText(IDS_NORMAL_MODE);

	m_pPatternEditor->InvalidateBackground();
	m_pPatternEditor->InvalidateHeader();
	m_pPatternEditor->InvalidateCursor();
	RedrawPatternEditor();
	UpdateNoteQueues();		// // //
}

LRESULT CFamiTrackerView::OnUserDumpInst(WPARAM wParam, LPARAM lParam)		// // //
{
	CFamiTrackerDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);
	CInstrument *Inst = theApp.GetSoundGenerator()->GetRecordInstrument();
	int Slot = pDoc->AddInstrument(Inst);
	// use unique_ptr here
	// Inst->Retain();
	CMainFrame *pMainFrm = static_cast<CMainFrame*>(GetParentFrame());
	ASSERT_VALID(pMainFrm);
	pMainFrm->UpdateInstrumentList();
	theApp.GetSoundGenerator()->ResetDumpInstrument();
	InvalidateHeader();
	if (Slot != INVALID_INSTRUMENT)
		pMainFrm->SelectInstrument(Slot);

	return 0;
}

void CFamiTrackerView::OnTrackerDetune()			// // //
{
	CFamiTrackerDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);
	CDetuneDlg DetuneDlg;
	UINT nResult = DetuneDlg.DoModal();
	if (nResult != IDOK) return;
	const int *Table = DetuneDlg.GetDetuneTable();
	for (int i = 0; i < 6; i++) for (int j = 0; j < NOTE_COUNT; j++)
		pDoc->SetDetuneOffset(i, j, *(Table + j + i * NOTE_COUNT));
	pDoc->SetTuning(DetuneDlg.GetDetuneSemitone(), DetuneDlg.GetDetuneCent());		// // // 050B
	theApp.GetSoundGenerator()->DocumentPropertiesChanged(pDoc);
}

void CFamiTrackerView::OnTransposeDecreasenote()
{
	if (!m_bEditEnable) return;		// // //
	AddAction(new CPActionTranspose {TRANSPOSE_DEC_NOTES});
}

void CFamiTrackerView::OnTransposeDecreaseoctave()
{
	if (!m_bEditEnable) return;		// // //
	AddAction(new CPActionTranspose {TRANSPOSE_DEC_OCTAVES});
}

void CFamiTrackerView::OnTransposeIncreasenote()
{
	if (!m_bEditEnable) return;		// // //
	AddAction(new CPActionTranspose {TRANSPOSE_INC_NOTES});
}

void CFamiTrackerView::OnTransposeIncreaseoctave()
{
	if (!m_bEditEnable) return;		// // //
	AddAction(new CPActionTranspose {TRANSPOSE_INC_OCTAVES});
}

void CFamiTrackerView::OnDecreaseValues()
{
	if (!m_bEditEnable) return;		// // //
	AddAction(new CPActionScrollValues {-1});
}

void CFamiTrackerView::OnIncreaseValues()
{
	if (!m_bEditEnable) return;		// // //
	AddAction(new CPActionScrollValues {1});
}

void CFamiTrackerView::OnCoarseDecreaseValues()		// // //
{
	if (!m_bEditEnable) return;
	AddAction(new CPActionScrollValues {-16});
}

void CFamiTrackerView::OnCoarseIncreaseValues()		// // //
{
	if (!m_bEditEnable) return;
	AddAction(new CPActionScrollValues {16});
}

void CFamiTrackerView::OnEditInstrumentMask()
{
	m_bMaskInstrument = !m_bMaskInstrument;
}

void CFamiTrackerView::OnEditVolumeMask()
{
	m_bMaskVolume = !m_bMaskVolume;
}

void CFamiTrackerView::OnEditSelectall()
{
	m_pPatternEditor->SelectAll();
	InvalidateCursor();
}

void CFamiTrackerView::OnEditSelectnone()		// // //
{
	m_pPatternEditor->CancelSelection();
	InvalidateCursor();
}

void CFamiTrackerView::OnEditSelectrow()		// // //
{
	m_pPatternEditor->SetSelection(SEL_SCOPE_VROW | SEL_SCOPE_HFRAME);
	InvalidateCursor();
}

void CFamiTrackerView::OnEditSelectcolumn()		// // //
{
	m_pPatternEditor->SetSelection(SEL_SCOPE_VFRAME | SEL_SCOPE_HCOL);
	InvalidateCursor();
}

void CFamiTrackerView::OnEditSelectpattern()		// // //
{
	m_pPatternEditor->SetSelection(SEL_SCOPE_VFRAME | SEL_SCOPE_HCHAN);
	InvalidateCursor();
}

void CFamiTrackerView::OnEditSelectframe()		// // //
{
	m_pPatternEditor->SetSelection(SEL_SCOPE_VFRAME | SEL_SCOPE_HFRAME);
	InvalidateCursor();
}

void CFamiTrackerView::OnEditSelectchannel()		// // //
{
	m_pPatternEditor->SetSelection(SEL_SCOPE_VTRACK | SEL_SCOPE_HCHAN);
	InvalidateCursor();
}

void CFamiTrackerView::OnEditSelecttrack()		// // //
{
	m_pPatternEditor->SetSelection(SEL_SCOPE_VTRACK | SEL_SCOPE_HFRAME);
	InvalidateCursor();
}

void CFamiTrackerView::OnTrackerPlayrow()
{
	CFamiTrackerDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	const int Track = static_cast<CMainFrame*>(GetParentFrame())->GetSelectedTrack();
	const int Frame = m_pPatternEditor->GetFrame();
	const int Row = m_pPatternEditor->GetRow();
	const int Channels = pDoc->GetAvailableChannels();

	for (int i = 0; i < Channels; ++i) {
		stChanNote Note;
		pDoc->GetNoteData(Track, Frame, i, Row, &Note);
		if (!m_bMuteChannels[i])
			theApp.GetSoundGenerator()->QueueNote(i, Note, NOTE_PRIO_1);
	}

	m_pPatternEditor->MoveDown(1);
	InvalidateCursor();
}

void CFamiTrackerView::OnEditCopyAsVolumeSequence()		// // //
{
	CString str;
	m_pPatternEditor->GetVolumeColumn(str);

	CClipboard Clipboard(this, CF_TEXT);

	if (!Clipboard.IsOpened()) {
		AfxMessageBox(IDS_CLIPBOARD_OPEN_ERROR);
		return;
	}

	if (!Clipboard.SetDataPointer(str.GetBuffer(), str.GetLength() + 1)) {
		AfxMessageBox(IDS_CLIPBOARD_COPY_ERROR);
	}
}

void CFamiTrackerView::OnEditCopyAsText()		// // //
{
	CString str;
	m_pPatternEditor->GetSelectionAsText(str);

	CClipboard Clipboard(this, CF_TEXT);

	if (!Clipboard.IsOpened()) {
		AfxMessageBox(IDS_CLIPBOARD_OPEN_ERROR);
		return;
	}

	if (!Clipboard.SetDataPointer(str.GetBuffer(), str.GetLength() + 1)) {
		AfxMessageBox(IDS_CLIPBOARD_COPY_ERROR);
	}
}

void CFamiTrackerView::OnEditCopyAsPPMCK()		// // //
{
	CString str;
	m_pPatternEditor->GetSelectionAsPPMCK(str);

	CClipboard Clipboard(this, CF_TEXT);

	if (!Clipboard.IsOpened()) {
		AfxMessageBox(IDS_CLIPBOARD_OPEN_ERROR);
		return;
	}

	if (!Clipboard.SetDataPointer(str.GetBuffer(), str.GetLength() + 1)) {
		AfxMessageBox(IDS_CLIPBOARD_COPY_ERROR);
	}
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////
// UI updates
////////////////////////////////////////////////////////////////////////////////////////////////////////////

void CFamiTrackerView::OnInitialUpdate()
{
	//
	// Called when the view is first attached to a document,
	// when a file is loaded or new document is created.
	//

	CFamiTrackerDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	CMainFrame *pMainFrame = static_cast<CMainFrame*>(GetParentFrame());
	ASSERT_VALID(pMainFrame);

	CFrameEditor *pFrameEditor = GetFrameEditor();

	TRACE("View: OnInitialUpdate (%s)\n", pDoc->GetTitle());

	// Setup order window
	pFrameEditor->AssignDocument(pDoc, this);
	m_pPatternEditor->SetDocument(pDoc, this);

	// Always start with first track
	pMainFrame->SelectTrack(0);

	// Notify the pattern view about new document & view
	m_pPatternEditor->ResetCursor();
	pFrameEditor->ResetCursor();		// // //

	// // !! limit channel view count to the max allowed
	if (static_cast<unsigned int>(pFrameEditor->m_iMaxChannelView) < pDoc->GetAvailableChannels())
		pFrameEditor->m_iChannelView = pFrameEditor->m_iMaxChannelView;
	else
		pFrameEditor->m_iChannelView = pDoc->GetAvailableChannels();

	// Update mainframe with new document settings
	pMainFrame->UpdateInstrumentList();
	pMainFrame->SetSongInfo(pDoc->GetSongName(), pDoc->GetSongArtist(), pDoc->GetSongCopyright());
	pMainFrame->UpdateTrackBox();
	pMainFrame->DisplayOctave();
	pMainFrame->UpdateControls();
	pMainFrame->ResetUndo();
	pMainFrame->ResizeFrameWindow();

	// Fetch highlight
	m_pPatternEditor->SetHighlight(pDoc->GetHighlight());		// // //
	pMainFrame->SetHighlightRows(pDoc->GetHighlight());

	// Follow mode
	SetFollowMode(theApp.GetSettings()->FollowMode);

	// Setup speed/tempo (TODO remove?)
	theApp.GetSoundGenerator()->ResetState();
	theApp.GetSoundGenerator()->ResetTempo();
	theApp.GetSoundGenerator()->SetMeterDecayRate(theApp.GetSettings()->MeterDecayRate);		// // // 050B
	theApp.GetSoundGenerator()->DocumentPropertiesChanged(pDoc);		// // //

	// Default
	SetInstrument(0);

	// Unmute all channels
	for (int i = 0; i < MAX_CHANNELS; ++i) {
		SetChannelMute(i, false);
	}

	UpdateNoteQueues();		// // //

	// Draw screen
	m_pPatternEditor->InvalidateBackground();
	m_pPatternEditor->InvalidatePatternData();
	RedrawPatternEditor();

	pFrameEditor->InvalidateFrameData();
	RedrawFrameEditor();

	if (theApp.m_pMainWnd->IsWindowVisible())
		SetFocus();

	// Display comment box
	if (pDoc->ShowCommentOnOpen())
		pMainFrame->PostMessage(WM_COMMAND, ID_MODULE_COMMENTS);

	// Call OnUpdate
	//CView::OnInitialUpdate();
}

void CFamiTrackerView::RefreshFrameEditor()
{
	// Updates the state of the frame editor max channel count,
	// refreshes and redraws the frame editor

	CFamiTrackerDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	CMainFrame* pMainFrame = static_cast<CMainFrame*>(GetParentFrame());
	ASSERT_VALID(pMainFrame);

	CFrameEditor* pFrameEditor = GetFrameEditor();
	pFrameEditor->AssignDocument(pDoc, this);

	if (static_cast<unsigned int>(pFrameEditor->m_iMaxChannelView) < pDoc->GetAvailableChannels())
		pFrameEditor->m_iChannelView = pFrameEditor->m_iMaxChannelView;
	else
		pFrameEditor->m_iChannelView = pDoc->GetAvailableChannels();

	pMainFrame->ResizeFrameWindow();

	pFrameEditor->InvalidateFrameData();
	RedrawFrameEditor();
}

void CFamiTrackerView::OnUpdate(CView* /*pSender*/, LPARAM lHint, CObject* /*pHint*/)
{
	// Called when the document has changed
	CMainFrame *pMainFrm = static_cast<CMainFrame*>(GetParentFrame());
	ASSERT_VALID(pMainFrm);

	// Handle new flags
	switch (lHint) {
	// Track has been added, removed or changed
	case UPDATE_TRACK:
		if (theApp.IsPlaying())
			theApp.StopPlayer();
		pMainFrm->UpdateTrackBox();
		m_pPatternEditor->InvalidateBackground();
		m_pPatternEditor->InvalidatePatternData();
		m_pPatternEditor->InvalidateHeader();
		RedrawPatternEditor();
		break;
	// Pattern data has been edited
	case UPDATE_PATTERN:
		m_pPatternEditor->InvalidatePatternData();
		RedrawPatternEditor();
		break;
	// Frame data has been edited
	case UPDATE_FRAME:
		InvalidateFrameEditor();

		m_pPatternEditor->InvalidatePatternData();
		RedrawPatternEditor();
		pMainFrm->UpdateBookmarkList();		// // //
		break;
	// Instrument has been added / removed
	case UPDATE_INSTRUMENT:
		pMainFrm->UpdateInstrumentList();
		m_pPatternEditor->InvalidatePatternData();
		RedrawPatternEditor();
		break;
	// Module properties has changed (including channel count)
	case UPDATE_PROPERTIES:
		// Old
		m_pPatternEditor->ResetCursor();
		//m_pPatternEditor->Modified();
		pMainFrm->ResetUndo();
		pMainFrm->ResizeFrameWindow();

		m_pPatternEditor->InvalidateBackground();	// Channel count might have changed, recalculated pattern layout
		m_pPatternEditor->InvalidatePatternData();
		m_pPatternEditor->InvalidateHeader();
		RedrawPatternEditor();
		UpdateNoteQueues();		// // //
		break;
	// Row highlight option has changed
	case UPDATE_HIGHLIGHT:
		m_pPatternEditor->SetHighlight(GetDocument()->GetHighlight());		// // //
		m_pPatternEditor->InvalidatePatternData();
		RedrawPatternEditor();
		break;
	// Effect columns has changed
	case UPDATE_COLUMNS:
		m_pPatternEditor->InvalidateBackground();	// Recalculate pattern layout
		m_pPatternEditor->InvalidateHeader();
		m_pPatternEditor->InvalidatePatternData();
		RedrawPatternEditor();
		break;
	// Document is closing
	case UPDATE_CLOSE:
		// Old
		pMainFrm->CloseGrooveSettings();		// // //
		pMainFrm->CloseBookmarkSettings();		// // //
		pMainFrm->UpdateBookmarkList();		// // //
		pMainFrm->CloseInstrumentEditor();
		break;
	}
}

void CFamiTrackerView::TrackChanged(unsigned int Track)
{
	// Called when the selected track has changed
	CMainFrame *pMainFrm = static_cast<CMainFrame*>(GetParentFrame());
	ASSERT_VALID(pMainFrm);

	m_pPatternEditor->ResetCursor();
	m_pPatternEditor->InvalidatePatternData();
	m_pPatternEditor->InvalidateBackground();
	m_pPatternEditor->InvalidateHeader();
	RedrawPatternEditor();

	pMainFrm->UpdateTrackBox();

	InvalidateFrameEditor();
	RedrawFrameEditor();
}

// GUI elements updates

void CFamiTrackerView::OnUpdateEditInstrumentMask(CCmdUI *pCmdUI)
{
	pCmdUI->SetCheck(m_bMaskInstrument ? 1 : 0);
}

void CFamiTrackerView::OnUpdateEditVolumeMask(CCmdUI *pCmdUI)
{
	pCmdUI->SetCheck(m_bMaskVolume ? 1 : 0);
}

void CFamiTrackerView::OnUpdateEditCopy(CCmdUI *pCmdUI)
{
	pCmdUI->Enable(!m_pPatternEditor->IsSelecting() ? 0 : 1);
}

void CFamiTrackerView::OnUpdateEditCut(CCmdUI *pCmdUI)
{
	pCmdUI->Enable(!m_pPatternEditor->IsSelecting() ? 0 : 1);
}

void CFamiTrackerView::OnUpdateEditPaste(CCmdUI *pCmdUI)
{
	pCmdUI->Enable(IsClipboardAvailable() ? 1 : 0);
}

void CFamiTrackerView::OnUpdateEditDelete(CCmdUI *pCmdUI)
{
	pCmdUI->Enable(!m_pPatternEditor->IsSelecting() ? 0 : 1);
}

void CFamiTrackerView::OnUpdateTrackerEdit(CCmdUI *pCmdUI)
{
	pCmdUI->SetCheck(m_bEditEnable ? 1 : 0);
}

void CFamiTrackerView::OnEditPasteSpecialCursor()		// // //
{
	m_iPastePos = PASTE_CURSOR;
}

void CFamiTrackerView::OnEditPasteSpecialSelection()
{
	m_iPastePos = PASTE_SELECTION;
}

void CFamiTrackerView::OnEditPasteSpecialFill()
{
	m_iPastePos = PASTE_FILL;
}

void CFamiTrackerView::OnUpdatePasteSpecial(CCmdUI *pCmdUI)
{
	if (pCmdUI->m_pMenu != NULL)
		pCmdUI->m_pMenu->CheckMenuRadioItem(ID_PASTESPECIAL_CURSOR, ID_PASTESPECIAL_FILL, ID_PASTESPECIAL_CURSOR + m_iPastePos, MF_BYCOMMAND);
}

void CFamiTrackerView::OnUpdateDisableWhilePlaying(CCmdUI *pCmdUI)
{
	pCmdUI->Enable(!theApp.IsPlaying());
}

void CFamiTrackerView::SetMarker(int Frame, int Row)		// // // 050B
{
	m_iMarkerFrame = Frame;
	m_iMarkerRow = Row;
	m_pPatternEditor->InvalidatePatternData();
	RedrawPatternEditor();
	GetFrameEditor()->InvalidateFrameData();
	RedrawFrameEditor();
}

bool CFamiTrackerView::IsMarkerValid() const		// // //
{
	if (m_iMarkerFrame < 0 || m_iMarkerRow < 0)
		return false;

	CFamiTrackerDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	const int Track = static_cast<CMainFrame*>(GetParentFrame())->GetSelectedTrack();
	if (m_iMarkerFrame >= static_cast<int>(pDoc->GetFrameCount(Track)))
		return false;
	if (m_iMarkerRow >= m_pPatternEditor->GetCurrentPatternLength(m_iMarkerFrame))
		return false;
	return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Tracker playing routines
////////////////////////////////////////////////////////////////////////////////////////////////////////////

void CFamiTrackerView::PlayerTick()
{
	// Callback from sound thread (TODO move this to sound thread?)

	if (m_iAutoArpKeyCount == 1 || !theApp.GetSettings()->Midi.bMidiArpeggio)
		return;

	// auto arpeggio
	int OldPtr = m_iAutoArpPtr;
	do {
		m_iAutoArpPtr = (m_iAutoArpPtr + 1) & 127;
		if (m_iAutoArpNotes[m_iAutoArpPtr] == 1) {
			m_iLastAutoArpPtr = m_iAutoArpPtr;
			m_iArpeggiate[m_pPatternEditor->GetChannel()] = m_iAutoArpPtr;
			break;
		}
		else if (m_iAutoArpNotes[m_iAutoArpPtr] == 2) {
			m_iAutoArpNotes[m_iAutoArpPtr] = 0;
		}
	}
	while (m_iAutoArpPtr != OldPtr);
}

bool CFamiTrackerView::PlayerGetNote(int Track, int Frame, int Channel, int Row, stChanNote &NoteData)
{
	CFamiTrackerDoc *pDoc = GetDocument();
	bool ValidCommand = false;

	pDoc->GetNoteData(Track, Frame, Channel, Row, &NoteData);

	if (!IsChannelMuted(Channel)) {
		// Let view know what is about to play
		PlayerPlayNote(Channel, &NoteData);
		ValidCommand = true;
	}
	else {
		// These effects will pass even if the channel is muted
		const int PASS_EFFECTS[] = {EF_HALT, EF_JUMP, EF_SPEED, EF_SKIP, EF_GROOVE};		// // //
		int Columns = pDoc->GetEffColumns(Track, Channel) + 1;

		NoteData.Note		= HALT;
		NoteData.Octave		= 0;
		NoteData.Instrument = 0;

		for (int j = 0; j < Columns; ++j) {
			bool Clear = true;
			for (int k = 0; k < sizeof(PASS_EFFECTS) / sizeof(int); ++k) {		// // //
				if (NoteData.EffNumber[j] == PASS_EFFECTS[k]) {
					ValidCommand = true;
					Clear = false;
				}
			}
			if (Clear)
				NoteData.EffNumber[j] = EF_NONE;
		}
	}

	return ValidCommand;
}

void CFamiTrackerView::PlayerPlayNote(int Channel, stChanNote *pNote)
{
	// Callback from sound thread

	if (pNote->Instrument < MAX_INSTRUMENTS && pNote->Note > 0 && Channel == m_pPatternEditor->GetChannel() && m_bSwitchToInstrument) {
		m_iSwitchToInstrument = pNote->Instrument;
	}
}

unsigned int CFamiTrackerView::GetSelectedFrame() const
{
	return m_pPatternEditor->GetFrame();
}

unsigned int CFamiTrackerView::GetSelectedChannel() const
{
	return m_pPatternEditor->GetChannel();
}

unsigned int CFamiTrackerView::GetSelectedRow() const
{
	return m_pPatternEditor->GetRow();
}

void CFamiTrackerView::SetFollowMode(bool Mode)
{
	m_bFollowMode = Mode;
	m_pPatternEditor->SetFollowMove(Mode);
}

bool CFamiTrackerView::GetFollowMode() const
{
	return m_bFollowMode;
}

void CFamiTrackerView::SetCompactMode(bool Mode)		// // //
{
	m_bCompactMode = Mode;
	m_pPatternEditor->SetCompactMode(Mode);
	InvalidateCursor();
}

int	CFamiTrackerView::GetAutoArpeggio(unsigned int Channel)
{
	// Return and reset next arpeggio note
	int ret = m_iArpeggiate[Channel];
	m_iArpeggiate[Channel] = 0;
	return ret;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////
// General
////////////////////////////////////////////////////////////////////////////////////////////////////////////

void CFamiTrackerView::RegisterKeyState(int Channel, int Note)
{
	CFamiTrackerDoc *pDoc = GetDocument();
	CTrackerChannel *pChannel = pDoc->GetChannel(m_pPatternEditor->GetChannel());

	if (pChannel->GetID() == Channel)
		m_iKeyboardNote = Note;

	/*
	if (Channel == m_pPatternEditor->GetChannel())
		m_iKeyboardNote = Note;
		*/
}

void CFamiTrackerView::SelectNextFrame()
{
	m_pPatternEditor->NextFrame();
	InvalidateFrameEditor();
}

void CFamiTrackerView::SelectPrevFrame()
{
	m_pPatternEditor->PreviousFrame();
	InvalidateFrameEditor();
}

void CFamiTrackerView::SelectFirstFrame()
{
	m_pPatternEditor->MoveToFrame(0);
	InvalidateFrameEditor();
}

void CFamiTrackerView::SelectLastFrame()
{
	CFamiTrackerDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	int Track = static_cast<CMainFrame*>(GetParentFrame())->GetSelectedTrack();
	m_pPatternEditor->MoveToFrame(pDoc->GetFrameCount(Track) - 1);
	InvalidateFrameEditor();
}

void CFamiTrackerView::MoveCursorNextChannel()
{
	m_pPatternEditor->NextChannel();
	InvalidateCursor();
}

void CFamiTrackerView::MoveCursorPrevChannel()
{
	m_pPatternEditor->PreviousChannel();
	InvalidateCursor();
}

void CFamiTrackerView::SelectFrame(unsigned int Frame)
{
	ASSERT(Frame < MAX_FRAMES);
	m_pPatternEditor->MoveToFrame(Frame);
	InvalidateCursor();
}

void CFamiTrackerView::SelectRow(unsigned int Row)
{
	ASSERT(Row < MAX_PATTERN_LENGTH);
	m_pPatternEditor->MoveToRow(Row);
	InvalidateCursor();
}

void CFamiTrackerView::SelectChannel(unsigned int Channel)
{
	ASSERT(Channel < MAX_CHANNELS);
	m_pPatternEditor->MoveToChannel(Channel);
	InvalidateCursor();
}

// // // TODO: move these to CMainFrame?

void CFamiTrackerView::OnBookmarksToggle()
{
	if ((theApp.IsPlaying() && m_bFollowMode) || !m_bEditEnable)
		return;

	CFamiTrackerDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);
	int Track = static_cast<CMainFrame*>(GetParentFrame())->GetSelectedTrack();
	const int Frame = GetSelectedFrame();
	const int Row = GetSelectedRow();

	CBookmarkCollection *pCol = pDoc->GetBookmarkManager()->GetCollection(Track);
	ASSERT(pCol);
	if (CBookmark *pMark = pCol->FindAt(Frame, Row))
		pCol->RemoveAt(Frame, Row);
	else {
		pMark = new CBookmark(Frame, Row);
		pMark->m_Highlight.First = pMark->m_Highlight.Second = -1;
		pMark->m_bPersist = false;
		char buf[32] = {};
		sprintf_s(buf, sizeof(buf), _T("Bookmark %i"), pCol->GetCount() + 1);
		pMark->m_sName = buf;
		pCol->AddBookmark(pMark);
	}

	static_cast<CMainFrame*>(GetParentFrame())->UpdateBookmarkList();
	SetFocus();
	pDoc->SetModifiedFlag();
	pDoc->SetExceededFlag();
	m_pPatternEditor->InvalidatePatternData();
	RedrawPatternEditor();
	GetFrameEditor()->InvalidateFrameData();
	RedrawFrameEditor();
}

void CFamiTrackerView::OnBookmarksNext()
{
	if (theApp.IsPlaying() && m_bFollowMode)
		return;

	CFamiTrackerDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);
	CMainFrame *pMainFrame = static_cast<CMainFrame*>(GetParentFrame());
	CBookmarkCollection *pCol = pDoc->GetBookmarkManager()->GetCollection(pMainFrame->GetSelectedTrack());
	ASSERT(pCol);

	if (CBookmark *pMark = pCol->FindNext(GetSelectedFrame(), GetSelectedRow())) {
		SelectFrame(pMark->m_iFrame);
		SelectRow(pMark->m_iRow);
		CString str1 = _T("None");
		if (pMark->m_Highlight.First != -1) str1.Format(_T("%i"), pMark->m_Highlight.First);
		CString str2 = _T("None");
		if (pMark->m_Highlight.Second != -1) str2.Format(_T("%i"), pMark->m_Highlight.Second);
		CString Text;
		AfxFormatString3(Text, IDS_BOOKMARK_FORMAT, pMark->m_sName.c_str(), str1, str2);
		pMainFrame->SetMessageText(Text);
		pMainFrame->UpdateBookmarkList(pCol->GetBookmarkIndex(pMark));
		SetFocus();
	}
	else {
		MessageBeep(MB_ICONINFORMATION);
		pMainFrame->SetMessageText(IDS_BOOKMARK_EMPTY);
	}
}

void CFamiTrackerView::OnBookmarksPrevious()
{
	if (theApp.IsPlaying() && m_bFollowMode)
		return;

	CFamiTrackerDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);
	CMainFrame *pMainFrame = static_cast<CMainFrame*>(GetParentFrame());
	CBookmarkCollection *pCol = pDoc->GetBookmarkManager()->GetCollection(pMainFrame->GetSelectedTrack());
	ASSERT(pCol);

	if (CBookmark *pMark = pCol->FindPrevious(GetSelectedFrame(), GetSelectedRow())) {
		SelectFrame(pMark->m_iFrame);
		SelectRow(pMark->m_iRow);
		CString str1 = _T("None");
		if (pMark->m_Highlight.First != -1) str1.Format(_T("%i"), pMark->m_Highlight.First);
		CString str2 = _T("None");
		if (pMark->m_Highlight.Second != -1) str2.Format(_T("%i"), pMark->m_Highlight.Second);
		CString Text;
		AfxFormatString3(Text, IDS_BOOKMARK_FORMAT, pMark->m_sName.c_str(), str1, str2);
		pMainFrame->SetMessageText(Text);
		pMainFrame->UpdateBookmarkList(pCol->GetBookmarkIndex(pMark));
		SetFocus();
	}
	else {
		MessageBeep(MB_ICONINFORMATION);
		pMainFrame->SetMessageText(IDS_BOOKMARK_EMPTY);
	}
}

void CFamiTrackerView::OnEditSplitKeyboard()		// // //
{
	CSplitKeyboardDlg dlg;
	dlg.m_bSplitEnable = m_iSplitNote != -1;
	dlg.m_iSplitNote = m_iSplitNote;
	dlg.m_iSplitChannel = m_iSplitChannel;
	dlg.m_iSplitInstrument = m_iSplitInstrument;
	dlg.m_iSplitTranspose = m_iSplitTranspose;

	if (dlg.DoModal() == IDOK) {
		if (dlg.m_bSplitEnable) {
			m_iSplitNote = dlg.m_iSplitNote;
			m_iSplitChannel = dlg.m_iSplitChannel;
			m_iSplitInstrument = dlg.m_iSplitInstrument;
			m_iSplitTranspose = dlg.m_iSplitTranspose;
		}
		else {
			m_iSplitNote = -1;
			m_iSplitChannel = -1;
			m_iSplitInstrument = MAX_INSTRUMENTS;
			m_iSplitTranspose = 0;
		}
	}
}

void CFamiTrackerView::ToggleChannel(unsigned int Channel)
{
	CFamiTrackerDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	if (Channel >= pDoc->GetAvailableChannels())
		return;

	SetChannelMute(Channel, !IsChannelMuted(Channel));
	InvalidateHeader();
}

void CFamiTrackerView::SoloChannel(unsigned int Channel)
{
	CFamiTrackerDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	int channels = pDoc->GetAvailableChannels();

	if (Channel >= unsigned(channels))
		return;

	if (IsChannelSolo(Channel))
		for (int i = 0; i < channels; ++i) // Revert channels
			SetChannelMute(i, false);
	else
		for (int i = 0; i < channels; ++i) // Solo selected channel
			SetChannelMute(i, i != Channel);

	InvalidateHeader();
}

void CFamiTrackerView::ToggleChip(unsigned int Channel)		// // //
{
	CFamiTrackerDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	int channels = pDoc->GetAvailableChannels();

	if (Channel >= unsigned(channels))
		return;

	int Chip = pDoc->GetChipType(Channel);
	for (int i = 0; i < channels; ++i)
		if (pDoc->GetChipType(i) == Chip && !IsChannelMuted(i)) {
			for (int j = 0; j < channels; ++j)
				if (pDoc->GetChipType(j) == Chip)
					SetChannelMute(j, true);
			InvalidateHeader();
			return;
		}

	for (int j = 0; j < channels; ++j) if (pDoc->GetChipType(j) == Chip)
		SetChannelMute(j, false);

	InvalidateHeader();
}

void CFamiTrackerView::SoloChip(unsigned int Channel)		// // //
{
	CFamiTrackerDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	int channels = pDoc->GetAvailableChannels();

	if (Channel >= unsigned(channels))
		return;

	int Chip = pDoc->GetChipType(Channel);
	if (IsChipSolo(Chip))
		for (int i = 0; i < channels; ++i)
			SetChannelMute(i, false);
	else
		for (int i = 0; i < channels; ++i)
			SetChannelMute(i, pDoc->GetChipType(i) != Chip);

	InvalidateHeader();
}

void CFamiTrackerView::UnmuteAllChannels()
{
	CFamiTrackerDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	int channels = pDoc->GetAvailableChannels();

	for (int i = 0; i < channels; ++i)
		SetChannelMute(i, false);

	InvalidateHeader();
}

void CFamiTrackerView::MuteAllChannels() {
	CFamiTrackerDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	int channels = pDoc->GetAvailableChannels();

	for (int i = 0; i < channels; ++i)
		SetChannelMute(i, true);

	InvalidateHeader();
}

bool CFamiTrackerView::IsChannelSolo(unsigned int Channel) const
{
	// Returns true if Channel is the only active channel
	CFamiTrackerDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	int channels = pDoc->GetAvailableChannels();

	for (int i = 0; i < channels; ++i)
		if (!IsChannelMuted(i) && i != Channel)		// // //
			return false;
	return true;
}

bool CFamiTrackerView::IsChipSolo(unsigned int Chip) const		// // //
{
	CFamiTrackerDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	int channels = pDoc->GetAvailableChannels();

	for (int i = 0; i < channels; ++i)
		if (!IsChannelMuted(i) && pDoc->GetChipType(i) != Chip)
			return false;
	return true;
}

void CFamiTrackerView::SetChannelMute(int Channel, bool bMute)
{
	CFamiTrackerDoc* pDoc = GetDocument();

	if (m_bMuteChannels[Channel] != bMute) {		// // //
		HaltNoteSingle(Channel);
//		if (bMute)
//			m_pNoteQueue->MuteChannel(pDoc->GetChannelType(Channel));
//		else
//			m_pNoteQueue->UnmuteChannel(pDoc->GetChannelType(Channel));
	}
	m_bMuteChannels[Channel] = bMute;

	if (bMute && pDoc->GetChannelType(Channel) == theApp.GetSoundGenerator()->GetRecordChannel())
		theApp.GetSoundGenerator()->SetRecordChannel(-1);
}

bool CFamiTrackerView::IsChannelMuted(unsigned int Channel) const
{
	ASSERT(Channel < MAX_CHANNELS);
	return m_bMuteChannels[Channel];
}

void CFamiTrackerView::SetInstrument(int Instrument)
{
	CMainFrame *pMainFrm = static_cast<CMainFrame*>(GetParentFrame());
	ASSERT_VALID(pMainFrm);

	if (Instrument >= MAX_INSTRUMENTS)		// // // 050B
		return; // may be called by emptying inst field or using &&

	pMainFrm->SelectInstrument(Instrument);
	m_iLastInstrument = GetInstrument(); // Gets actual selected instrument //  Instrument;
}

unsigned int CFamiTrackerView::GetInstrument() const
{
	CMainFrame *pMainFrm = static_cast<CMainFrame*>(GetParentFrame());
	ASSERT_VALID(pMainFrm);
	return pMainFrm->GetSelectedInstrument();
}

unsigned int CFamiTrackerView::GetSplitInstrument() const		// // //
{
	return m_iSplitInstrument;
}

void CFamiTrackerView::StepDown()
{
	// Update pattern length in case it has changed
	m_pPatternEditor->UpdatePatternLength();

	if (m_iInsertKeyStepping)
		m_pPatternEditor->MoveDown(m_iInsertKeyStepping);

	InvalidateCursor();
}

void CFamiTrackerView::InsertNote(int Note, int Octave, int Channel, int Velocity)
{
	// Inserts a note
	const int Track = static_cast<CMainFrame*>(GetParentFrame())->GetSelectedTrack();
	const int Frame = m_pPatternEditor->GetFrame();
	const int Row = m_pPatternEditor->GetRow();

	stChanNote Cell;
	GetDocument()->GetNoteData(Track, Frame, Channel, Row, &Cell);

	Cell.Note = Note;

	if (Note != HALT && Note != RELEASE) {
		Cell.Octave	= Octave;

		if (!m_bMaskInstrument && Cell.Instrument != HOLD_INSTRUMENT)		// // // 050B
			Cell.Instrument = GetInstrument();

		if (!m_bMaskVolume) {
			Cell.Vol = m_iLastVolume;
			if (Velocity < 128)
				Cell.Vol = (Velocity / 8);
		}
		if (Note != NONE && Note != ECHO) {		// // //
			if (GetDocument()->GetChannelType(Channel) == CHANID_NOISE) {		// // //
				unsigned int MidiNote = (MIDI_NOTE(Octave, Note) % 16) + 16;
				Cell.Octave = Octave = GET_OCTAVE(MidiNote);
				Cell.Note = Note = GET_NOTE(MidiNote);
			}
			else if (IsSplitEnabled(MIDI_NOTE(Octave, Note), Channel))
				SplitKeyboardAdjust(Cell);

		}
	}

	// Quantization
	if (theApp.GetSettings()->Midi.bMidiMasterSync) {
		int Delay = theApp.GetMIDI()->GetQuantization();
		if (Delay > 0) {
			Cell.EffNumber[0] = EF_DELAY;
			Cell.EffParam[0] = Delay;
		}
	}

	if (m_bEditEnable) {
		if (Note == HALT)
			m_iLastNote = NOTE_HALT;
		else if (Note == RELEASE)
			m_iLastNote = NOTE_RELEASE;
		else if (Note == ECHO) {		// // //
			if (Cell.Octave > ECHO_BUFFER_LENGTH) Cell.Octave = ECHO_BUFFER_LENGTH;
			if (Cell.Octave < 1) Cell.Octave = 1;
			m_iLastNote = NOTE_ECHO + Cell.Octave;
		}
		else {
			m_iLastNote = (Note - 1) + Octave * 12;
		}

		CPatternAction *pAction = new CPActionEditNote(Cell);		// // //
		if (AddAction(pAction)) {
			const CSettings *pSettings = theApp.GetSettings();
			if (m_pPatternEditor->GetColumn() == C_NOTE && !theApp.IsPlaying() && m_iInsertKeyStepping > 0 && !pSettings->Midi.bMidiMasterSync) {
				StepDown();
				pAction->SaveRedoState(static_cast<CMainFrame*>(GetParentFrame()));		// // //
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Note playing routines
//////////////////////////////////////////////////////////////////////////////////////////////////////////

void CFamiTrackerView::PlayNote(unsigned int Channel, unsigned int Note, unsigned int Octave, unsigned int Velocity) const
{
	// Play a note in a channel
	stChanNote NoteData { };		// // //

	NoteData.Note		= Note;
	NoteData.Octave		= Octave;
	NoteData.Instrument	= GetInstrument();
	if (theApp.GetSettings()->Midi.bMidiVelocity)
		NoteData.Vol = Velocity / 8;
/*
	if (theApp.GetSettings()->General.iEditStyle == EDIT_STYLE_IT)
		NoteData.Instrument	= m_iLastInstrument;
	else
		NoteData.Instrument	= GetInstrument();
*/
	int MidiNote = MIDI_NOTE(Octave, Note);		// // //
	CFamiTrackerDoc *pDoc = GetDocument();

	SplitAdjustChannel(Channel, NoteData);
	if (Channel < static_cast<unsigned>(pDoc->GetChannelCount())) {
		int ret = pDoc->GetChannelIndex(m_pNoteQueue->Trigger(MidiNote, pDoc->GetChannelType(Channel)));
		if (ret != -1) {
			if (IsSplitEnabled(MidiNote, ret)) 	// // //
				SplitKeyboardAdjust(NoteData);
			pDoc->GetChannel(ret)->SetNote(NoteData, NOTE_PRIO_2);
			theApp.GetSoundGenerator()->ForceReloadInstrument(ret);		// // //
		}
	}

	if (theApp.GetSettings()->General.bPreviewFullRow) {
		stChanNote ChanNote;
		int Track = static_cast<CMainFrame*>(GetParentFrame())->GetSelectedTrack();
		int Frame = m_pPatternEditor->GetFrame();
		int Row = m_pPatternEditor->GetRow();
		int Channels = pDoc->GetAvailableChannels();

		for (int i = 0; i < Channels; ++i) {
			pDoc->GetNoteData(Track, Frame, i, Row, &ChanNote);
			if (!m_bMuteChannels[i] && i != Channel)
				theApp.GetSoundGenerator()->QueueNote(i, ChanNote, (i == Channel) ? NOTE_PRIO_2 : NOTE_PRIO_1);
		}
	}
}

void CFamiTrackerView::ReleaseNote(unsigned int Channel, unsigned int Note, unsigned int Octave) const
{
	// Releases a channel
	stChanNote NoteData { };		// // //

	NoteData.Note = RELEASE;
	NoteData.Instrument = GetInstrument();

	SplitAdjustChannel(Channel, NoteData);		// // //
	CFamiTrackerDoc *pDoc = GetDocument();		// // //
	if (Channel < static_cast<unsigned>(pDoc->GetChannelCount())) {
		int ch = pDoc->GetChannelIndex(m_pNoteQueue->Cut(MIDI_NOTE(Octave, Note), pDoc->GetChannelType(Channel)));
	//	int ch = pDoc->GetChannelIndex(m_pNoteQueue->Release(MIDI_NOTE(Octave, Note), pDoc->GetChannelType(Channel)));
		if (ch != -1)
			theApp.GetSoundGenerator()->QueueNote(ch, NoteData, NOTE_PRIO_2);

		if (theApp.GetSettings()->General.bPreviewFullRow) {
			NoteData.Note = HALT;
			NoteData.Instrument = MAX_INSTRUMENTS;

			int Channels = pDoc->GetChannelCount();
			for (int i = 0; i < Channels; ++i) {
				if (i != ch)
					theApp.GetSoundGenerator()->QueueNote(i, NoteData, NOTE_PRIO_1);
			}
		}
	}
}

void CFamiTrackerView::HaltNote(unsigned int Channel, unsigned int Note, unsigned int Octave) const
{
	// Halts a channel
	stChanNote NoteData { };		// // //

	NoteData.Note = HALT;
	NoteData.Instrument = GetInstrument();

	SplitAdjustChannel(Channel, NoteData);		// // //
	CFamiTrackerDoc *pDoc = GetDocument();		// // //
	if (Channel < static_cast<unsigned>(pDoc->GetChannelCount())) {
		int ch = pDoc->GetChannelIndex(m_pNoteQueue->Cut(MIDI_NOTE(Octave, Note), pDoc->GetChannelType(Channel)));
		if (ch != -1)
			theApp.GetSoundGenerator()->QueueNote(ch, NoteData, NOTE_PRIO_2);

		if (theApp.GetSettings()->General.bPreviewFullRow) {
			NoteData.Instrument = MAX_INSTRUMENTS;

			int Channels = pDoc->GetChannelCount();
			for (int i = 0; i < Channels; ++i) {
				if (i != ch)
					theApp.GetSoundGenerator()->QueueNote(i, NoteData, NOTE_PRIO_1);
			}
		}
	}
}

void CFamiTrackerView::HaltNoteSingle(unsigned int Channel) const
{
	// Halts one single channel only
	stChanNote NoteData { };		// // //

	NoteData.Note = HALT;
	NoteData.Instrument = GetInstrument();

	SplitAdjustChannel(Channel, NoteData);		// // // ?
	CFamiTrackerDoc *pDoc = GetDocument();		// // //
	if (Channel < static_cast<unsigned>(pDoc->GetChannelCount())) {
		for (const auto &i : m_pNoteQueue->StopChannel(pDoc->GetChannelType(Channel))) {
			int ch = pDoc->GetChannelIndex(i);
			if (ch != -1)
				theApp.GetSoundGenerator()->QueueNote(ch, NoteData, NOTE_PRIO_2);
		}
	}

	if (theApp.GetSoundGenerator()->IsPlaying())
		theApp.GetSoundGenerator()->QueueNote(Channel, NoteData, NOTE_PRIO_2);
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////
/// MIDI note handling functions
//////////////////////////////////////////////////////////////////////////////////////////////////////////

/*
static void FixNoise(unsigned int &MidiNote, unsigned int &Octave, unsigned int &Note)
{
	// NES noise channel
	MidiNote = (MidiNote % 16) + 16;
	Octave = GET_OCTAVE(MidiNote);
	Note = GET_NOTE(MidiNote);
}
*/

// Play a note
void CFamiTrackerView::TriggerMIDINote(unsigned int Channel, unsigned int MidiNote, unsigned int Velocity, bool Insert)
{
	CFamiTrackerDoc *pDoc = GetDocument();

	if (MidiNote >= NOTE_COUNT) MidiNote = NOTE_COUNT - 1;		// // //

	// Play a MIDI note
	unsigned int Octave = GET_OCTAVE(MidiNote);
	unsigned int Note = GET_NOTE(MidiNote);
//	if (pDoc->GetChannel(Channel)->GetID() == CHANID_NOISE)
//		FixNoise(MidiNote, Octave, Note);

	m_iActiveNotes[Channel] = MidiNote;

	if (!theApp.GetSettings()->Midi.bMidiVelocity) {
		if (theApp.GetSettings()->General.iEditStyle != EDIT_STYLE_IT)
			Velocity = 127;
		else {
			Velocity = m_iLastVolume * 8;
		}
	}

	if (!(theApp.IsPlaying() && m_bEditEnable && !m_bFollowMode))		// // //
		PlayNote(Channel, Note, Octave, Velocity);

	if (Insert)
		InsertNote(Note, Octave, Channel, Velocity + 1);

	m_iAutoArpNotes[MidiNote] = 1;
	m_iAutoArpPtr = MidiNote;
	m_iLastAutoArpPtr = m_iAutoArpPtr;

	UpdateArpDisplay();

	m_iLastMIDINote = MidiNote;

	m_iAutoArpKeyCount = 0;

	for (int i = 0; i < 128; ++i) {
		if (m_iAutoArpNotes[i] == 1)
			++m_iAutoArpKeyCount;
	}

	TRACE("%i: Trigger note %i on channel %i\n", GetTickCount(), MidiNote, Channel);
}

// Cut the currently playing note
void CFamiTrackerView::CutMIDINote(unsigned int Channel, unsigned int MidiNote, bool InsertCut)
{
	CFamiTrackerDoc *pDoc = GetDocument();

	if (MidiNote >= NOTE_COUNT) MidiNote = NOTE_COUNT - 1;		// // //

	// Cut a MIDI note
	unsigned int Octave = GET_OCTAVE(MidiNote);
	unsigned int Note = GET_NOTE(MidiNote);
//	if (pDoc->GetChannel(Channel)->GetID() == CHANID_NOISE)
//		FixNoise(MidiNote, Octave, Note);

	m_iActiveNotes[Channel] = 0;
	m_iAutoArpNotes[MidiNote] = 2;

	UpdateArpDisplay();

	// Cut note
	if (!(theApp.IsPlaying() && m_bEditEnable && !m_bFollowMode))		// // //
		if (m_bEditEnable) {
			if (m_iLastMIDINote == MidiNote)
				HaltNote(Channel, Note, Octave);
		}
		else
			HaltNote(Channel, Note, Octave);

	if (InsertCut)
		InsertNote(HALT, 0, Channel, 0);

	// IT-mode, cut note on cuts
	if (theApp.GetSettings()->General.iEditStyle == EDIT_STYLE_IT)
		HaltNote(Channel, Note, Octave);		// // //

	TRACE("%i: Cut note %i on channel %i\n", GetTickCount(), MidiNote, Channel);
}

// Release the currently playing note
void CFamiTrackerView::ReleaseMIDINote(unsigned int Channel, unsigned int MidiNote, bool InsertCut)
{
	CFamiTrackerDoc *pDoc = GetDocument();

	if (MidiNote >= NOTE_COUNT) MidiNote = NOTE_COUNT - 1;		// // //

	// Release a MIDI note
	unsigned int Octave = GET_OCTAVE(MidiNote);
	unsigned int Note = GET_NOTE(MidiNote);
//	if (pDoc->GetChannel(Channel)->GetID() == CHANID_NOISE)
//		FixNoise(MidiNote, Octave, Note);

	m_iActiveNotes[Channel] = 0;
	m_iAutoArpNotes[MidiNote] = 2;

	UpdateArpDisplay();

	// Cut note
	if (!(theApp.IsPlaying() && m_bEditEnable && !m_bFollowMode))		// // //
		if (m_bEditEnable) {
			if (m_iLastMIDINote == MidiNote)
				ReleaseNote(Channel, Note, Octave);
		}
		else
			ReleaseNote(Channel, Note, Octave);

	if (InsertCut)
		InsertNote(RELEASE, 0, Channel, 0);

	// IT-mode, release note
	if (theApp.GetSettings()->General.iEditStyle == EDIT_STYLE_IT)
		ReleaseNote(Channel, Note, Octave);		// // //

	TRACE("%i: Release note %i on channel %i\n", GetTickCount(), MidiNote, Channel);
}

void CFamiTrackerView::UpdateArpDisplay()
{
	if (theApp.GetSettings()->Midi.bMidiArpeggio == true) {
		int Base = -1;
		CString str = _T("Auto-arpeggio: ");

		for (int i = 0; i < 128; ++i) {
			if (m_iAutoArpNotes[i] == 1) {
				if (Base == -1)
					Base = i;
				str.AppendFormat(_T("%i "), i - Base);
			}
		}

		if (Base != -1)
			GetParentFrame()->SetMessageText(str);
	}
}

void CFamiTrackerView::UpdateNoteQueues()		// // //
{
	const CFamiTrackerDoc *pDoc = GetDocument();
	ASSERT_VALID(pDoc);
	const int Channels = pDoc->GetChannelCount();

	m_pNoteQueue->ClearMaps();

	if (m_bEditEnable)
		for (int i = 0; i < Channels; ++i) {
			unsigned ID = pDoc->GetChannelType(i);
			if (ID != -1)
				m_pNoteQueue->AddMap({ID});
		}
	else {
		m_pNoteQueue->AddMap({CHANID_TRIANGLE});
		m_pNoteQueue->AddMap({CHANID_NOISE});
		m_pNoteQueue->AddMap({CHANID_DPCM});

		if (pDoc->ExpansionEnabled(SNDCHIP_VRC6)) {
			m_pNoteQueue->AddMap({CHANID_VRC6_PULSE1, CHANID_VRC6_PULSE2});
			m_pNoteQueue->AddMap({CHANID_VRC6_SAWTOOTH});
		}
		if (pDoc->ExpansionEnabled(SNDCHIP_VRC7))
			m_pNoteQueue->AddMap({CHANID_VRC7_CH1, CHANID_VRC7_CH2, CHANID_VRC7_CH3,
								  CHANID_VRC7_CH4, CHANID_VRC7_CH5, CHANID_VRC7_CH6});
		if (pDoc->ExpansionEnabled(SNDCHIP_FDS))
			m_pNoteQueue->AddMap({CHANID_FDS});
		if (pDoc->ExpansionEnabled(SNDCHIP_MMC5))
			m_pNoteQueue->AddMap({CHANID_SQUARE1, CHANID_SQUARE2, CHANID_MMC5_SQUARE1, CHANID_MMC5_SQUARE2});
		else
			m_pNoteQueue->AddMap({CHANID_SQUARE1, CHANID_SQUARE2});
		if (pDoc->ExpansionEnabled(SNDCHIP_N163)) {
			std::vector<unsigned> n;
			int Channels = pDoc->GetNamcoChannels();
			for (int i = 0; i < Channels; ++i)
				n.push_back(CHANID_N163_CH1 + i);
			m_pNoteQueue->AddMap(n);
		}
		if (pDoc->ExpansionEnabled(SNDCHIP_S5B))
			m_pNoteQueue->AddMap({CHANID_S5B_CH1, CHANID_S5B_CH2, CHANID_S5B_CH3});
	}

//	for (int i = 0; i < Channels; ++i)
//		if (IsChannelMuted(i))
//			m_pNoteQueue->MuteChannel(pDoc->GetChannelType(i));
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Tracker input routines
//////////////////////////////////////////////////////////////////////////////////////////////////////////

//
// API keyboard handling routines
//

bool CFamiTrackerView::IsShiftPressed() const
{
	return (::GetKeyState(VK_SHIFT) & 0x80) == 0x80;
}

bool CFamiTrackerView::IsControlPressed() const
{
	return (::GetKeyState(VK_CONTROL) & 0x80) == 0x80;
}

// OnKeyDown accepts virtual keycodes, and handles most input.
void CFamiTrackerView::OnKeyDown(UINT key, UINT nRepCnt, UINT nFlags)
{
	// Called when a key is pressed
	if (GetFocus() != this)
		return;

	auto pFrame = static_cast<CMainFrame*>(GetParentFrame());		// // //
	if (pFrame && pFrame->TypeInstrumentNumber(ConvertKeyToHex(key)))
		return;

	if (key >= VK_NUMPAD0 && key <= VK_NUMPAD9) {
		// Switch instrument
		if (m_pPatternEditor->GetColumn() == C_NOTE) {
			SetInstrument(key - VK_NUMPAD0);
			return;
		}
	}

	if ((key == VK_ADD || key == VK_SUBTRACT) && theApp.GetSettings()->General.bHexKeypad)		// // //
		HandleKeyboardInput(Keycode(key));
	else if (!theApp.GetSettings()->General.bHexKeypad || !(key == VK_RETURN && !(nFlags & KF_EXTENDED))) switch (key) {
		case VK_UP:
			OnKeyDirUp();
			break;
		case VK_DOWN:
			OnKeyDirDown();
			break;
		case VK_LEFT:
			OnKeyDirLeft();
			break;
		case VK_RIGHT:
			OnKeyDirRight();
			break;
		case VK_HOME:
			OnKeyHome();
			break;
		case VK_END:
			OnKeyEnd();
			break;
		case VK_PRIOR:
			OnKeyPageUp();
			break;
		case VK_NEXT:
			OnKeyPageDown();
			break;
		case VK_TAB:
			OnKeyTab();
			break;
		case VK_ADD:
			KeyIncreaseAction();
			break;
		case VK_SUBTRACT:
			KeyDecreaseAction();
			break;
		case VK_DELETE:
			OnKeyDelete();
			break;
		case VK_INSERT:
			OnKeyInsert();
			break;
		case VK_BACK:
			OnKeyBackspace();
			break;
		// // //

		// Octaves, unless overridden
		case VK_F2: pFrame->SelectOctave(0); break;
		case VK_F3: pFrame->SelectOctave(1); break;
		case VK_F4: pFrame->SelectOctave(2); break;
		case VK_F5: pFrame->SelectOctave(3); break;
		case VK_F6: pFrame->SelectOctave(4); break;
		case VK_F7: pFrame->SelectOctave(5); break;
		case VK_F8: pFrame->SelectOctave(6); break;
		case VK_F9: pFrame->SelectOctave(7); break;

		default:
			HandleKeyboardInput(Keycode(key));
	}

	CView::OnKeyDown(key, nRepCnt, nFlags);
}


inline bool withinInclusive(int x, int first, int last) {
	return first <= x && x <= last;
}

template <typename T>
bool isAlphanumeric(T x) {
	auto num = static_cast<int>(x);
	return withinInclusive(num, 'A', 'Z')
		|| withinInclusive(num, 'a', 'z')
		|| withinInclusive(num, '0', '9');
}

// OnChar operates on translated characters, and inserts non-alphanumeric effects.
void CFamiTrackerView::OnChar(UINT chr, UINT _nEvents, UINT flags) {
	if (GetFocus() != this)
		return;

	// don't handle standard effects
	if (isAlphanumeric(chr)) {
		return;
	}

	// Don't repeat effect, if key repeat disabled.
	bool isRepeat = flags & 0x00004000;	// I have no clue why this works.
	if (isRepeat && !(theApp.GetSettings()->General.bKeyRepeat)) {
		return;
	}

	HandleKeyboardInput(Character(chr));
}

void CFamiTrackerView::OnSysKeyDown(UINT key, UINT nRepCnt, UINT nFlags)
{
	// This is called when a key + ALT is pressed
	if (key >= VK_NUMPAD0 && key <= VK_NUMPAD9) {
		SetStepping(key - VK_NUMPAD0);
		return;
	}

	switch (key) {
		case VK_LEFT:
			m_pPatternEditor->MoveChannelLeft();
			InvalidateCursor();
			break;
		case VK_RIGHT:
			m_pPatternEditor->MoveChannelRight();
			InvalidateCursor();
			break;
	}

	CView::OnSysKeyDown(key, nRepCnt, nFlags);
}

void CFamiTrackerView::OnKeyUp(UINT key, UINT nRepCnt, UINT nFlags)
{
	// Called when a key is released
	HandleKeyboardNote(key, false);
	m_cKeyList[key] = 0;

	CView::OnKeyUp(key, nRepCnt, nFlags);
}

//
// Custom key handling routines
//

void CFamiTrackerView::OnKeyDirUp()
{
	// Move up
	m_pPatternEditor->MoveUp(MoveKeyStepping());
	InvalidateCursor();
}

void CFamiTrackerView::OnKeyDirDown()
{
	// Move down
	m_pPatternEditor->MoveDown(MoveKeyStepping());
	InvalidateCursor();
}

void CFamiTrackerView::OnKeyDirLeft()
{
	// Move left
	m_pPatternEditor->MoveLeft();
	InvalidateCursor();
}

void CFamiTrackerView::OnKeyDirRight()
{
	// Move right
	m_pPatternEditor->MoveRight();
	InvalidateCursor();
}

void CFamiTrackerView::OnKeyTab()
{
	// Move between channels
	bool bShiftPressed = IsShiftPressed();

	if (bShiftPressed)
		m_pPatternEditor->PreviousChannel();
	else
		m_pPatternEditor->NextChannel();

	InvalidateCursor();
}

void CFamiTrackerView::OnKeyPageUp()
{
	// Move page up
	int PageSize = theApp.GetSettings()->General.iPageStepSize;
	m_pPatternEditor->MoveUp(PageSize);
	InvalidateCursor();
}

void CFamiTrackerView::OnKeyPageDown()
{
	// Move page down
	int PageSize = theApp.GetSettings()->General.iPageStepSize;
	m_pPatternEditor->MoveDown(PageSize);
	InvalidateCursor();
}

void CFamiTrackerView::OnKeyHome()
{
	// Move home
	m_pPatternEditor->OnHomeKey();
	InvalidateCursor();
}

void CFamiTrackerView::OnKeyEnd()
{
	// Move to end
	m_pPatternEditor->OnEndKey();
	InvalidateCursor();
}

void CFamiTrackerView::OnKeyInsert()
{
	// Insert row
	CFamiTrackerDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	if (PreventRepeat(VK_INSERT, true) || !m_bEditEnable)		// // //
		return;

	if (m_pPatternEditor->IsSelecting())
		AddAction(new CPActionInsertAtSel { });		// // //
	else
		AddAction(new CPActionInsertRow { });
}

void CFamiTrackerView::OnKeyBackspace()
{
	// Pull up row
	CFamiTrackerDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	if (!m_bEditEnable)		// // //
		return;

	if (m_pPatternEditor->IsSelecting()) {
		AddAction(new CPActionDeleteAtSel { });
	}
	else {
		if (PreventRepeat(VK_BACK, true))
			return;
		CPatternAction *pAction = new CPActionDeleteRow(true, true);		// // //
		if (AddAction(pAction)) {
			m_pPatternEditor->MoveUp(1);
			InvalidateCursor();
			pAction->SaveRedoState(static_cast<CMainFrame*>(GetParentFrame()));		// // //
		}
	}
}

void CFamiTrackerView::OnKeyDelete()
{
	// Delete row data
	CFamiTrackerDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	bool bShiftPressed = IsShiftPressed();

	if (PreventRepeat(VK_DELETE, true) || !m_bEditEnable)		// // //
		return;

	if (m_pPatternEditor->IsSelecting()) {
		OnEditDelete();
	}
	else {
		bool bPullUp = theApp.GetSettings()->General.bPullUpDelete || bShiftPressed;
		CPatternAction *pAction = new CPActionDeleteRow(bPullUp, false);		// // //
		AddAction(pAction);
		if (!bPullUp) {
			StepDown();
			pAction->SaveRedoState(static_cast<CMainFrame*>(GetParentFrame()));		// // //
		}
	}
}

void CFamiTrackerView::KeyIncreaseAction()
{
	if (!m_bEditEnable)		// // //
		return;

	AddAction(new CPActionScrollField {1});		// // //
}

void CFamiTrackerView::KeyDecreaseAction()
{
	if (!m_bEditEnable)		// // //
		return;

	AddAction(new CPActionScrollField {-1});		// // //
}

bool CFamiTrackerView::EditInstrumentColumn(stChanNote &Note, Keycode Key, bool &StepDown, bool &MoveRight, bool &MoveLeft)
{
	int EditStyle = theApp.GetSettings()->General.iEditStyle;
	const cursor_column_t Column = m_pPatternEditor->GetColumn();		// // //

	if (!m_bEditEnable)
		return false;

	if (CheckClearKey(Key)) {
		SetInstrument(Note.Instrument = MAX_INSTRUMENTS);	// Indicate no instrument selected
		if (EditStyle != EDIT_STYLE_MPT)
			StepDown = true;
		return true;
	}
	else if (CheckRepeatKey(Key)) {
		SetInstrument(Note.Instrument = m_iLastInstrument);
		if (EditStyle != EDIT_STYLE_MPT)
			StepDown = true;
		return true;
	}
	else if (Key == 'H') {		// // // 050B
		SetInstrument(Note.Instrument = HOLD_INSTRUMENT);
		if (EditStyle != EDIT_STYLE_MPT)
			StepDown = true;
		return true;
	}

	int Value = ConvertKeyToHex(Key);
	unsigned char Mask, Shift;

	if (Value == -1 && theApp.GetSettings()->General.bHexKeypad)		// // //
		Value = ConvertKeyExtra(Key);
	if (Value == -1)
		return false;

	if (Column == C_INSTRUMENT1) {
		Mask = 0x0F;
		Shift = 4;
	}
	else {
		Mask = 0xF0;
		Shift = 0;
	}

	if (Note.Instrument == MAX_INSTRUMENTS || Note.Instrument == HOLD_INSTRUMENT)		// // // 050B
		Note.Instrument = 0;

	switch (EditStyle) {
		case EDIT_STYLE_FT2: // FT2
			Note.Instrument = (Note.Instrument & Mask) | (Value << Shift);
			StepDown = true;
			break;
		case EDIT_STYLE_MPT: // MPT
			Note.Instrument = ((Note.Instrument & 0x0F) << 4) | Value & 0x0F;
			if (Note.Instrument >= MAX_INSTRUMENTS)		// // //
				Note.Instrument &= 0x0F;
			break;
		case EDIT_STYLE_IT: // IT
			Note.Instrument = (Note.Instrument & Mask) | (Value << Shift);
			if (Column == C_INSTRUMENT1)
				MoveRight = true;
			else if (Column == C_INSTRUMENT2) {
				MoveLeft = true;
				StepDown = true;
			}
			break;
		case EDIT_STYLE_FT2_JP: // FT2-JP106
			Note.Instrument = (Note.Instrument & Mask) | (Value << Shift);
			StepDown = true;
			break;
	}

	if (Note.Instrument > (MAX_INSTRUMENTS - 1))
		Note.Instrument = (MAX_INSTRUMENTS - 1);
	// // //
	SetInstrument(Note.Instrument);

	return true;
}

bool CFamiTrackerView::EditVolumeColumn(stChanNote &Note, Keycode Key, bool &bStepDown)
{
	int EditStyle = theApp.GetSettings()->General.iEditStyle;

	if (!m_bEditEnable)
		return false;

	if (CheckClearKey(Key)) {
		Note.Vol = MAX_VOLUME;
		if (EditStyle != EDIT_STYLE_MPT)
			bStepDown = true;
		m_iLastVolume = MAX_VOLUME;
		return true;
	}
	else if (CheckRepeatKey(Key)) {
		Note.Vol = m_iLastVolume;
		if (EditStyle != EDIT_STYLE_MPT)
			bStepDown = true;
		return true;
	}

	int Value = ConvertKeyToHex(Key);

	if (Value == -1 && theApp.GetSettings()->General.bHexKeypad)		// // //
		Value = ConvertKeyExtra(Key);
	if (Value == -1)
		return false;

	m_iLastVolume = Note.Vol = Value;		// // //

	if (EditStyle != EDIT_STYLE_MPT)
		bStepDown = true;

	return true;
}

bool CFamiTrackerView::EditEffNumberColumn(stChanNote &Note, Input input, int EffectIndex, bool &bStepDown)
{
	int EditStyle = theApp.GetSettings()->General.iEditStyle;

	if (!m_bEditEnable)
		return false;

	if (auto p = get_if<Keycode>(&input)) {
		auto key = *p;
		// class InputKey -> isKey, isChar
		// if input.isKey():
		// KeyType key = input.getKey();

		// union input;
		// if type == KEYCODE: KeyType key = input;

		if (CheckRepeatKey(key)) {
			Note.EffNumber[EffectIndex] = m_iLastEffect;
			Note.EffParam[EffectIndex] = m_iLastEffectParam;
			if (EditStyle != EDIT_STYLE_MPT)		// // //
				bStepDown = true;
			if (m_bEditEnable && Note.EffNumber[EffectIndex] != EF_NONE)		// // //
				GetParentFrame()->SetMessageText(GetEffectHint(Note, EffectIndex));
			return true;
		}

		if (CheckClearKey(key)) {
			Note.EffNumber[EffectIndex] = EF_NONE;
			if (EditStyle != EDIT_STYLE_MPT)
				bStepDown = true;
			return true;
		}

		if (key >= VK_NUMPAD0 && key <= VK_NUMPAD9)
			key = '0' + (key - VK_NUMPAD0);
	}

	CFamiTrackerDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	int Chip = pDoc->GetChannel(m_pPatternEditor->GetChannel())->GetChip();


	// **** Handle effect names. ****

	char effect;	// TODO promote to int. So Unicode codepoints won't clash with ASCII effects.

	if (auto p = get_if<Keycode>(&input)) {
		auto key = *p;
		if (!isAlphanumeric(key)) {
			return false;
		}
		effect = (char)key;

	} else if (auto p = get_if<Character>(&input)) {
		int chr = (int)*p;
		assert(!isAlphanumeric(chr));
		if ((unsigned)chr >= 0x100) {
			throw std::runtime_error("non-8bit character or something");
		}
		effect = (char)chr;

	} else {
		throw std::runtime_error("EditEffNumberColumn called with invalid Input (neither Keycode nor Character)");
	}

	bool bValidEffect = false;
	effect_t Effect = GetEffectFromChar(effect, Chip, &bValidEffect);		// // //
	effect_t prev = Note.EffNumber[EffectIndex];

	if (bValidEffect) {
		Note.EffNumber[EffectIndex] = Effect;
		if (m_bEditEnable && Note.EffNumber[EffectIndex] != EF_NONE)		// // //
			GetParentFrame()->SetMessageText(GetEffectHint(Note, EffectIndex));

		if (prev == EF_NONE || effects[Effect].uiDefault != 0) {
			Note.EffParam[EffectIndex] = effects[Effect].uiDefault;
		}

		switch (EditStyle) {
			case EDIT_STYLE_MPT:	// Modplug
				if (Effect == m_iLastEffect)
					Note.EffParam[EffectIndex] = m_iLastEffectParam;
				break;
			default:
				bStepDown = true;
		}
		m_iLastEffect = Effect;
		m_iLastEffectParam = Note.EffParam[EffectIndex];		// // //
		return true;
	}

	return false;
}

bool CFamiTrackerView::EditEffParamColumn(stChanNote &Note, Keycode Key, int EffectIndex, bool &bStepDown, bool &bMoveRight, bool &bMoveLeft)
{

	int EditStyle = theApp.GetSettings()->General.iEditStyle;
	const cursor_column_t Column = m_pPatternEditor->GetColumn();		// // //
	int Value = ConvertKeyToHex(Key);

	if (!m_bEditEnable)
		return false;

	if (CheckRepeatKey(Key)) {
		Note.EffNumber[EffectIndex] = m_iLastEffect;
		Note.EffParam[EffectIndex] = m_iLastEffectParam;
		if (EditStyle != EDIT_STYLE_MPT)		// // //
			bStepDown = true;
		if (m_bEditEnable && Note.EffNumber[EffectIndex] != EF_NONE)		// // //
			GetParentFrame()->SetMessageText(GetEffectHint(Note, EffectIndex));
		return true;
	}

	if (CheckClearKey(Key)) {
		Note.EffParam[EffectIndex] = 0;
		if (EditStyle != EDIT_STYLE_MPT)
			bStepDown = true;
		return true;
	}

	if (Value == -1 && theApp.GetSettings()->General.bHexKeypad)		// // //
		Value = ConvertKeyExtra(Key);
	if (Value == -1)
		return false;

	unsigned char Mask, Shift;
	if (Column == C_EFF1_PARAM1 || Column == C_EFF2_PARAM1 || Column == C_EFF3_PARAM1 || Column == C_EFF4_PARAM1) {
		Mask = 0x0F;
		Shift = 4;
	}
	else {
		Mask = 0xF0;
		Shift = 0;
	}

	switch (EditStyle) {
		case EDIT_STYLE_FT2:	// FT2
			Note.EffParam[EffectIndex] = (Note.EffParam[EffectIndex] & Mask) | Value << Shift;
			bStepDown = true;
			break;
		case EDIT_STYLE_MPT:	// Modplug
			Note.EffParam[EffectIndex] = ((Note.EffParam[EffectIndex] & 0x0F) << 4) | Value & 0x0F;
			break;
		case EDIT_STYLE_IT:	// IT
			Note.EffParam[EffectIndex] = (Note.EffParam[EffectIndex] & Mask) | Value << Shift;
			if (Mask == 0x0F)
				bMoveRight = true;
			else {
				bMoveLeft = true;
				bStepDown = true;
			}
			break;
		case EDIT_STYLE_FT2_JP:	// FT2-JP106
			Note.EffParam[EffectIndex] = (Note.EffParam[EffectIndex] & Mask) | Value << Shift;
			bStepDown = true;
			break;
	}

	m_iLastEffect = Note.EffNumber[EffectIndex];		// // //
	m_iLastEffectParam = Note.EffParam[EffectIndex];

	if (m_bEditEnable && Note.EffNumber[EffectIndex] != EF_NONE)		// // //
		GetParentFrame()->SetMessageText(GetEffectHint(Note, EffectIndex));

	return true;
}

/*
https://github.com/nyanpasu64/0CC-FamiTracker/wiki/Keyboard-Input-Handling:-Row-Editor-(FamiTrackerView.cpp)
*/
void CFamiTrackerView::HandleKeyboardInput(Input input)
{
	// https://github.com/solodon4/Mach7 ???
	if (auto p = get_if<Keycode>(&input)) {
		auto key = *p;
		if (theApp.GetAccelerator()->IsKeyUsed(static_cast<int>(key)))
			return;

		// Watch for repeating keys
		if (PreventRepeat(key, m_bEditEnable))
			return;
	}

	CFamiTrackerDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	stChanNote Note;

	int EditStyle = theApp.GetSettings()->General.iEditStyle;
	int Index = 0;

	int Track = static_cast<CMainFrame*>(GetParentFrame())->GetSelectedTrack();
	int Frame = m_pPatternEditor->GetFrame();
	int Row = m_pPatternEditor->GetRow();
	int Channel = m_pPatternEditor->GetChannel();
	cursor_column_t Column = m_pPatternEditor->GetColumn();

	bool bStepDown = false;
	bool bMoveRight = false;
	bool bMoveLeft = false;


	// Get the note data
	pDoc->GetNoteData(Track, Frame, Channel, Row, &Note);

	// Make all effect columns look the same, save an index instead
	switch (Column) {
		case C_EFF1_NUM:	Column = C_EFF1_NUM;	Index = 0; break;
		case C_EFF2_NUM:	Column = C_EFF1_NUM;	Index = 1; break;
		case C_EFF3_NUM:	Column = C_EFF1_NUM;	Index = 2; break;
		case C_EFF4_NUM:	Column = C_EFF1_NUM;	Index = 3; break;
		case C_EFF1_PARAM1:	Column = C_EFF1_PARAM1; Index = 0; break;
		case C_EFF2_PARAM1:	Column = C_EFF1_PARAM1; Index = 1; break;
		case C_EFF3_PARAM1:	Column = C_EFF1_PARAM1; Index = 2; break;
		case C_EFF4_PARAM1:	Column = C_EFF1_PARAM1; Index = 3; break;
		case C_EFF1_PARAM2:	Column = C_EFF1_PARAM2; Index = 0; break;
		case C_EFF2_PARAM2:	Column = C_EFF1_PARAM2; Index = 1; break;
		case C_EFF3_PARAM2:	Column = C_EFF1_PARAM2; Index = 2; break;
		case C_EFF4_PARAM2:	Column = C_EFF1_PARAM2; Index = 3; break;
	}

	if (auto p = get_if<Keycode>(&input)) {
		auto key = *p;

		if (Column != C_NOTE && !m_bEditEnable)		// // //
			HandleKeyboardNote(key, true);

		switch (Column) {
		// Note & octave column
		case C_NOTE:
			if (CheckRepeatKey(key)) {
				if (m_iLastNote == 0) {
					Note.Note = 0;
				} else if (m_iLastNote == NOTE_HALT) {
					Note.Note = HALT;
				} else if (m_iLastNote == NOTE_RELEASE) {
					Note.Note = RELEASE;
				} else if (m_iLastNote >= NOTE_ECHO && m_iLastNote <= NOTE_ECHO + ECHO_BUFFER_LENGTH) {		// // //
					Note.Note = ECHO;
					Note.Octave = m_iLastNote - NOTE_ECHO;
				} else {
					Note.Note = GET_NOTE(m_iLastNote);
					Note.Octave = GET_OCTAVE(m_iLastNote);
				}
			} else if (CheckEchoKey(key)) {		// // //
				Note.Note = ECHO;
				Note.Octave = static_cast<CMainFrame*>(GetParentFrame())->GetSelectedOctave();		// // //
				if (Note.Octave > ECHO_BUFFER_LENGTH) Note.Octave = ECHO_BUFFER_LENGTH;
				if (!m_bMaskInstrument)
					Note.Instrument = GetInstrument();
				m_iLastNote = NOTE_ECHO + Note.Octave;
			} else if (CheckClearKey(key)) {
				// Remove note
				Note.Note = 0;
				Note.Octave = 0;
				m_iLastNote = 0;
			} else {
				// This is special
				HandleKeyboardNote(key, true);
				return;
			}
			if (EditStyle != EDIT_STYLE_MPT)		// // //
				bStepDown = true;
			break;

		// TODO isAlphanumeric

		// Instrument column
		case C_INSTRUMENT1:
		case C_INSTRUMENT2:
			if (!EditInstrumentColumn(Note, key, bStepDown, bMoveRight, bMoveLeft))		// invert test?
				return;
			break;
		// Volume column
		case C_VOLUME:
			if (!EditVolumeColumn(Note, key, bStepDown))
				return;
			break;
		// Effect type (character)
		case C_EFF1_NUM:
			if (!EditEffNumberColumn(Note, input, Index, bStepDown))
				return;
			break;
		// Effect parameter
		case C_EFF1_PARAM1:
		case C_EFF1_PARAM2:
			if (!EditEffParamColumn(Note, key, Index, bStepDown, bMoveRight, bMoveLeft))
				return;
			break;
		}

		if (CheckClearKey(key) && IsControlPressed())		// // //
			Note = stChanNote{};
	}
	else if (auto p = get_if<Character>(&input)) {
		if (Column == C_EFF1_NUM) {
			if (EditEffNumberColumn(Note, input, Index, bStepDown)) {
				goto altered_char;
			}
		}
		return;

	altered_char:
		;
	}

	// Something changed, store pattern data in document and update screen
	if (m_bEditEnable) {
		CPatternAction *pAction = new CPActionEditNote(Note);		// // //
		if (AddAction(pAction)) {
			if (bMoveLeft)
				m_pPatternEditor->MoveLeft();
			if (bMoveRight)
				m_pPatternEditor->MoveRight();
			if (bStepDown)
				StepDown();
			InvalidateCursor();
			pAction->SaveRedoState(static_cast<CMainFrame*>(GetParentFrame()));		// // //
		}
	}
}

bool CFamiTrackerView::DoRelease() const
{
	// Return true if there are a valid release sequence for selected instrument
	auto pInstrument = GetDocument()->GetInstrument(GetInstrument());
	if (!pInstrument) return false;
	return pInstrument->CanRelease();
}

void CFamiTrackerView::HandleKeyboardNote(Keycode key, bool Pressed)
{
	if (theApp.GetAccelerator()->IsKeyUsed(static_cast<int>(key))) return;		// // //

	// Play a note from the keyboard
	int Note = TranslateKey(key);
	int Channel = m_pPatternEditor->GetChannel();

	if (Pressed) {
		static int LastNote;

		if (CheckHaltKey(key)) {
			if (m_bEditEnable)
				CutMIDINote(Channel, LastNote, true);
			else {
				for (const auto &x : m_iNoteCorrection)		// // //
					CutMIDINote(Channel, TranslateKey(x.first), true);
				m_iNoteCorrection.clear();
			}
		}
		else if (CheckReleaseKey(key)) {
			if (m_bEditEnable)
				ReleaseMIDINote(Channel, LastNote, true);
			else {
				for (const auto &x : m_iNoteCorrection)		// // //
					ReleaseMIDINote(Channel, TranslateKey(x.first), true);
				m_iNoteCorrection.clear();
			}
		}
		else {
			// Invalid key
			if (Note == -1)
				return;
			TriggerMIDINote(Channel, Note, 0x7F, m_bEditEnable);
			LastNote = Note;
			m_iNoteCorrection[key] = 0;		// // //
		}
	}
	else {
		if (Note == -1)
			return;
		// IT doesn't cut the note when key is released
		if (theApp.GetSettings()->General.iEditStyle != EDIT_STYLE_IT) {
			// Find if note release should be used
			// TODO: make this an option instead?
			if (DoRelease())
				ReleaseMIDINote(Channel, Note, false);
			else
				CutMIDINote(Channel, Note, false);
			auto it = m_iNoteCorrection.find(key);		// // //
			if (it != m_iNoteCorrection.end())
				m_iNoteCorrection.erase(it);
		}
		else {
			m_iActiveNotes[Channel] = 0;
			m_iAutoArpNotes[Note] = 2;
		}
	}
}

bool CFamiTrackerView::IsSplitEnabled(int MidiNote, int Channel) const
{
	if (m_iSplitNote == -1)
		return false;
	if (const auto Chan = GetDocument()->GetChannel(Channel)) {
		if (Chan->GetID() == CHANID_NOISE)
			return false;
		return MidiNote <= m_iSplitNote;
	}
	return false;
}

void CFamiTrackerView::SplitKeyboardAdjust(stChanNote &Note) const		// // //
{
	ASSERT(Note.Note >= NOTE_C && Note.Note <= NOTE_B);
	int MidiNote = MIDI_NOTE(Note.Octave, Note.Note) + m_iSplitTranspose;
	if (MidiNote < 0) MidiNote = 0;
	if (MidiNote >= NOTE_COUNT) MidiNote = NOTE_COUNT - 1;
	Note.Octave = GET_OCTAVE(MidiNote);
	Note.Note = GET_NOTE(MidiNote);

	if (m_iSplitInstrument != MAX_INSTRUMENTS)
		Note.Instrument = m_iSplitInstrument;
}

void CFamiTrackerView::SplitAdjustChannel(unsigned int &Channel, const stChanNote &Note) const		// // //
{
	if (m_bEditEnable || m_iSplitChannel == -1) return;
	if (m_iSplitNote != -1 && MIDI_NOTE(Note.Octave, Note.Note) <= m_iSplitNote) {
		int Index = GetDocument()->GetChannelIndex(m_iSplitChannel);
		if (Index != -1) Channel = Index;
	}
}

bool CFamiTrackerView::CheckClearKey(Keycode Key) const
{
	return (Key == theApp.GetSettings()->Keys.iKeyClear);
}

bool CFamiTrackerView::CheckReleaseKey(Keycode Key) const
{
	return (Key == theApp.GetSettings()->Keys.iKeyNoteRelease);
}

bool CFamiTrackerView::CheckHaltKey(Keycode Key) const
{
	return (Key == theApp.GetSettings()->Keys.iKeyNoteCut);
}

bool CFamiTrackerView::CheckEchoKey(Keycode Key) const		// // //
{
	return (Key == theApp.GetSettings()->Keys.iKeyEchoBuffer);
}

bool CFamiTrackerView::CheckRepeatKey(Keycode Key) const
{
	return (Key == theApp.GetSettings()->Keys.iKeyRepeat);
}

int CFamiTrackerView::TranslateKeyModplug(Keycode Key) const
{
	// Modplug conversion
	CFamiTrackerDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	int	KeyNote = 0, KeyOctave = 0;
	int Track = static_cast<CMainFrame*>(GetParentFrame())->GetSelectedTrack();
	int Octave = static_cast<CMainFrame*>(GetParentFrame())->GetSelectedOctave();		// // // 050B

	stChanNote NoteData;
	pDoc->GetNoteData(Track, m_pPatternEditor->GetFrame(), m_pPatternEditor->GetChannel(), m_pPatternEditor->GetRow(), &NoteData);

	if (m_bEditEnable && Key >= '0' && Key <= '9') {		// // //
		KeyOctave = static_cast<int>(Key - '1');
		if (KeyOctave < 0) KeyOctave += 10;
		if (KeyOctave >= OCTAVE_RANGE) KeyOctave = OCTAVE_RANGE - 1;
		KeyNote = NoteData.Note;
	}

	// Convert key to a note, Modplug style
	switch (Key) {
		case 81:	KeyNote = NOTE_C;	KeyOctave = Octave;	break;	// Q		// // //
		case 87:	KeyNote = NOTE_Cs;	KeyOctave = Octave;	break;	// W
		case 69:	KeyNote = NOTE_D;	KeyOctave = Octave;	break;	// E
		case 82:	KeyNote = NOTE_Ds;	KeyOctave = Octave;	break;	// R
		case 84:	KeyNote = NOTE_E;	KeyOctave = Octave;	break;	// T
		case 89:	KeyNote = NOTE_F;	KeyOctave = Octave;	break;	// Y
		case 85:	KeyNote = NOTE_Fs;	KeyOctave = Octave;	break;	// U
		case 73:	KeyNote = NOTE_G;	KeyOctave = Octave;	break;	// I
		case 79:	KeyNote = NOTE_Gs;	KeyOctave = Octave;	break;	// O
		case 80:	KeyNote = NOTE_A;	KeyOctave = Octave;	break;	// P
		case 219:	KeyNote = NOTE_As;	KeyOctave = Octave;	break;	// [{		// // //
		case 221:	KeyNote = NOTE_B;	KeyOctave = Octave;	break;	// ]}		// // //

		case 65:	KeyNote = NOTE_C;	KeyOctave = Octave + 1;	break;	// A
		case 83:	KeyNote = NOTE_Cs;	KeyOctave = Octave + 1;	break;	// S
		case 68:	KeyNote = NOTE_D;	KeyOctave = Octave + 1;	break;	// D
		case 70:	KeyNote = NOTE_Ds;	KeyOctave = Octave + 1;	break;	// F
		case 71:	KeyNote = NOTE_E;	KeyOctave = Octave + 1;	break;	// G
		case 72:	KeyNote = NOTE_F;	KeyOctave = Octave + 1;	break;	// H
		case 74:	KeyNote = NOTE_Fs;	KeyOctave = Octave + 1;	break;	// J
		case 75:	KeyNote = NOTE_G;	KeyOctave = Octave + 1;	break;	// K
		case 76:	KeyNote = NOTE_Gs;	KeyOctave = Octave + 1;	break;	// L
		case 186:	KeyNote = NOTE_A;	KeyOctave = Octave + 1;	break;	// ;:		// // //
		case 222:	KeyNote = NOTE_As;	KeyOctave = Octave + 1;	break;	// '"
		//case 191:	KeyNote = NOTE_B;	KeyOctave = Octave + 1;	break;	// // //

		case 90:	KeyNote = NOTE_C;	KeyOctave = Octave + 2;	break;	// Z
		case 88:	KeyNote = NOTE_Cs;	KeyOctave = Octave + 2;	break;	// X
		case 67:	KeyNote = NOTE_D;	KeyOctave = Octave + 2;	break;	// C
		case 86:	KeyNote = NOTE_Ds;	KeyOctave = Octave + 2;	break;	// V
		case 66:	KeyNote = NOTE_E;	KeyOctave = Octave + 2;	break;	// B
		case 78:	KeyNote = NOTE_F;	KeyOctave = Octave + 2;	break;	// N
		case 77:	KeyNote = NOTE_Fs;	KeyOctave = Octave + 2;	break;	// M
		case 188:	KeyNote = NOTE_G;	KeyOctave = Octave + 2;	break;	// ,<
		case 190:	KeyNote = NOTE_Gs;	KeyOctave = Octave + 2;	break;	// .>
		case 191:	KeyNote = NOTE_A;	KeyOctave = Octave + 2;	break;	// /?		// // //
	}

	// Invalid
	if (KeyNote == 0)
		return -1;

	auto it = m_iNoteCorrection.find(Key);		// // //
	if (it != m_iNoteCorrection.end())
		KeyOctave += it->second;

	// Return a MIDI note
	return MIDI_NOTE(KeyOctave, KeyNote);
}

int CFamiTrackerView::TranslateKeyDefault(Keycode Key) const
{
	// Default conversion
	int	KeyNote = 0;
	int KeyOctave = static_cast<CMainFrame*>(GetParentFrame())->GetSelectedOctave();		// // // 050B

	// Convert key to a note
	switch (Key) {
		case 50:	KeyNote = NOTE_Cs;	KeyOctave += 1;	break;	// 2		// // //
		case 51:	KeyNote = NOTE_Ds;	KeyOctave += 1;	break;	// 3
		case 53:	KeyNote = NOTE_Fs;	KeyOctave += 1;	break;	// 5
		case 54:	KeyNote = NOTE_Gs;	KeyOctave += 1;	break;	// 6
		case 55:	KeyNote = NOTE_As;	KeyOctave += 1;	break;	// 7
		case 57:	KeyNote = NOTE_Cs;	KeyOctave += 2;	break;	// 9
		case 48:	KeyNote = NOTE_Ds;	KeyOctave += 2;	break;	// 0
		case 187:	KeyNote = NOTE_Fs;	KeyOctave += 2;	break;	// =+

		case 81:	KeyNote = NOTE_C;	KeyOctave += 1;	break;	// Q
		case 87:	KeyNote = NOTE_D;	KeyOctave += 1;	break;	// W
		case 69:	KeyNote = NOTE_E;	KeyOctave += 1;	break;	// E
		case 82:	KeyNote = NOTE_F;	KeyOctave += 1;	break;	// R
		case 84:	KeyNote = NOTE_G;	KeyOctave += 1;	break;	// T
		case 89:	KeyNote = NOTE_A;	KeyOctave += 1;	break;	// Y
		case 85:	KeyNote = NOTE_B;	KeyOctave += 1;	break;	// U
		case 73:	KeyNote = NOTE_C;	KeyOctave += 2;	break;	// I
		case 79:	KeyNote = NOTE_D;	KeyOctave += 2;	break;	// O
		case 80:	KeyNote = NOTE_E;	KeyOctave += 2;	break;	// P
		case 219:	KeyNote = NOTE_F;	KeyOctave += 2;	break;	// [{		// // //
		case 221:	KeyNote = NOTE_G;	KeyOctave += 2;	break;	// ]}		// // //

		case 83:	KeyNote = NOTE_Cs;					break;	// S
		case 68:	KeyNote = NOTE_Ds;					break;	// D
		case 71:	KeyNote = NOTE_Fs;					break;	// G
		case 72:	KeyNote = NOTE_Gs;					break;	// H
		case 74:	KeyNote = NOTE_As;					break;	// J
		case 76:	KeyNote = NOTE_Cs;	KeyOctave += 1;	break;	// L
		case 186:	KeyNote = NOTE_Ds;	KeyOctave += 1;	break;	// ;:		// // //

		case 90:	KeyNote = NOTE_C;					break;	// Z
		case 88:	KeyNote = NOTE_D;					break;	// X
		case 67:	KeyNote = NOTE_E;					break;	// C
		case 86:	KeyNote = NOTE_F;					break;	// V
		case 66:	KeyNote = NOTE_G;					break;	// B
		case 78:	KeyNote = NOTE_A;					break;	// N
		case 77:	KeyNote = NOTE_B;					break;	// M
		case 188:	KeyNote = NOTE_C;	KeyOctave += 1;	break;	// ,<
		case 190:	KeyNote = NOTE_D;	KeyOctave += 1;	break;	// .>
		case 191:	KeyNote = NOTE_E;	KeyOctave += 1;	break;	// /?		// // //
	}

	// Invalid
	if (KeyNote == 0)
		return -1;

	auto it = m_iNoteCorrection.find(Key);		// // //
	if (it != m_iNoteCorrection.end())
		KeyOctave += it->second;

	// Return a MIDI note
	return MIDI_NOTE(KeyOctave, KeyNote);
}

int CFamiTrackerView::TranslateKeyFT2JP(Keycode Key) const
{
	// FastTracker 2 style with JP106 keyboard conversion
	int	KeyNote = 0;
	int KeyOctave = static_cast<CMainFrame*>(GetParentFrame())->GetSelectedOctave();		// // // 050B

	// Convert key to a note for FastTracker 2 style with JP106 keyboard
	switch (Key) {
		case 50:	KeyNote = NOTE_Cs;	KeyOctave += 1;	break;	// 2		// // //
		case 51:	KeyNote = NOTE_Ds;	KeyOctave += 1;	break;	// 3
		case 53:	KeyNote = NOTE_Fs;	KeyOctave += 1;	break;	// 5
		case 54:	KeyNote = NOTE_Gs;	KeyOctave += 1;	break;	// 6
		case 55:	KeyNote = NOTE_As;	KeyOctave += 1;	break;	// 7
		case 57:	KeyNote = NOTE_Cs;	KeyOctave += 2;	break;	// 9
		case 48:	KeyNote = NOTE_Ds;	KeyOctave += 2;	break;	// 0
		case 222:	KeyNote = NOTE_Fs;	KeyOctave += 2;	break;	// Japanese ^~

		case 81:	KeyNote = NOTE_C;	KeyOctave += 1;	break;	// Q
		case 87:	KeyNote = NOTE_D;	KeyOctave += 1;	break;	// W
		case 69:	KeyNote = NOTE_E;	KeyOctave += 1;	break;	// E
		case 82:	KeyNote = NOTE_F;	KeyOctave += 1;	break;	// R
		case 84:	KeyNote = NOTE_G;	KeyOctave += 1;	break;	// T
		case 89:	KeyNote = NOTE_A;	KeyOctave += 1;	break;	// Y
		case 85:	KeyNote = NOTE_B;	KeyOctave += 1;	break;	// U
		case 73:	KeyNote = NOTE_C;	KeyOctave += 2;	break;	// I
		case 79:	KeyNote = NOTE_D;	KeyOctave += 2;	break;	// O
		case 80:	KeyNote = NOTE_E;	KeyOctave += 2;	break;	// P
		case 192:	KeyNote = NOTE_F;	KeyOctave += 2;	break;	// Japanese @`
		case 219:	KeyNote = NOTE_G;	KeyOctave += 2;	break;	// Japanese [{		// // //

		case 83:	KeyNote = NOTE_Cs;					break;	// S
		case 68:	KeyNote = NOTE_Ds;					break;	// D
		case 71:	KeyNote = NOTE_Fs;					break;	// G
		case 72:	KeyNote = NOTE_Gs;					break;	// H
		case 74:	KeyNote = NOTE_As;					break;	// J
		case 76:	KeyNote = NOTE_Cs;	KeyOctave += 1;	break;	// L
		case 187:	KeyNote = NOTE_Ds;	KeyOctave += 1;	break;	// Japanese :*		// // //

		case 90:	KeyNote = NOTE_C;					break;	// Z
		case 88:	KeyNote = NOTE_D;					break;	// X
		case 67:	KeyNote = NOTE_E;					break;	// C
		case 86:	KeyNote = NOTE_F;					break;	// V
		case 66:	KeyNote = NOTE_G;					break;	// B
		case 78:	KeyNote = NOTE_A;					break;	// N
		case 77:	KeyNote = NOTE_B;					break;	// M
		case 188:	KeyNote = NOTE_C;	KeyOctave += 1;	break;	// ,<
		case 190:	KeyNote = NOTE_D;	KeyOctave += 1;	break;	// .>
		case 191:	KeyNote = NOTE_E;	KeyOctave += 1;	break;	// /?		// // //
	}

	// Invalid
	if (KeyNote == 0)
		return -1;

	auto it = m_iNoteCorrection.find(Key);		// // //
	if (it != m_iNoteCorrection.end())
		KeyOctave += it->second;

	// Return a MIDI note
	return MIDI_NOTE(KeyOctave, KeyNote);
}

int CFamiTrackerView::TranslateKey(Keycode Key) const
{
	// Translates a keyboard character into a MIDI note

	// For modplug users
	if (theApp.GetSettings()->General.iEditStyle == EDIT_STYLE_MPT)
		return TranslateKeyModplug(Key);

	// For FastTracker 2 JP106 users
	if (theApp.GetSettings()->General.iEditStyle == EDIT_STYLE_FT2_JP)
		return TranslateKeyFT2JP(Key);

	// Default
	return TranslateKeyDefault(Key);
}

bool CFamiTrackerView::PreventRepeat(Keycode Key, bool Insert)
{
	if (m_cKeyList[Key] == 0)
		m_cKeyList[Key] = 1;
	else {
		if ((!theApp.GetSettings()->General.bKeyRepeat || !Insert))
			return true;
	}

	return false;
}

void CFamiTrackerView::RepeatRelease(Keycode Key)
{
	memset(m_cKeyList, 0, 256);
}

//
// Note preview
//

bool CFamiTrackerView::PreviewNote(Keycode Key)
{
	if (PreventRepeat(Key, false))
		return false;

	int Note = TranslateKey(Key);

	TRACE("View: Note preview\n");

	if (Note > 0) {
		TriggerMIDINote(m_pPatternEditor->GetChannel(), Note, 0x7F, false);
		return true;
	}

	return false;
}

void CFamiTrackerView::PreviewRelease(Keycode Key)
{
	memset(m_cKeyList, 0, 256);

	int Note = TranslateKey(Key);

	if (Note > 0) {
		if (DoRelease())
			ReleaseMIDINote(m_pPatternEditor->GetChannel(), Note, false);
		else
			CutMIDINote(m_pPatternEditor->GetChannel(), Note, false);
	}
}

//
// MIDI in routines
//

void CFamiTrackerView::TranslateMidiMessage()
{
	// Check and handle MIDI messages

	CMIDI *pMIDI = theApp.GetMIDI();
	CFamiTrackerDoc* pDoc = GetDocument();
	CString Status;

	if (!pMIDI || !pDoc)
		return;

	unsigned char Message, Channel, Data1, Data2;
	while (pMIDI->ReadMessage(Message, Channel, Data1, Data2)) {

		if (Message != 0x0F) {
			if (!theApp.GetSettings()->Midi.bMidiChannelMap)
				Channel = m_pPatternEditor->GetChannel();
			if (Channel > pDoc->GetAvailableChannels() - 1)
				Channel = pDoc->GetAvailableChannels() - 1;
		}

		// Translate key releases to note off messages
		if (Message == MIDI_MSG_NOTE_ON && Data2 == 0) {
			Message = MIDI_MSG_NOTE_OFF;
		}

		if (Message == MIDI_MSG_NOTE_ON || Message == MIDI_MSG_NOTE_OFF) {
			// Remove two octaves from MIDI notes
			Data1 -= 24;
			if (Data1 > 127)
				return;
		}

		switch (Message) {
			case MIDI_MSG_NOTE_ON:
				TriggerMIDINote(Channel, Data1, Data2, true);
				AfxFormatString3(Status, IDS_MIDI_MESSAGE_ON_FORMAT,
					MakeIntString(Data1 % 12),
					MakeIntString(Data1 / 12),
					MakeIntString(Data2));
				break;

			case MIDI_MSG_NOTE_OFF:
				// MIDI key is released, don't input note break into pattern
				if (DoRelease())
					ReleaseMIDINote(Channel, Data1, false);
				else
					CutMIDINote(Channel, Data1, false);
				Status.Format(IDS_MIDI_MESSAGE_OFF);
				break;

			case MIDI_MSG_PITCH_WHEEL:
				{
					CTrackerChannel *pChannel = pDoc->GetChannel(Channel);
					int PitchValue = 0x2000 - ((Data1 & 0x7F) | ((Data2 & 0x7F) << 7));
					pChannel->SetPitch(-PitchValue / 0x10);
				}
				break;

			case 0x0F:
				if (Channel == 0x08) {
					m_pPatternEditor->MoveDown(m_iInsertKeyStepping);
					InvalidateCursor();
				}
				break;
		}
	}

	if (Status.GetLength() > 0)
		GetParentFrame()->SetMessageText(Status);
}

//
// Effects menu
//

void CFamiTrackerView::OnTrackerToggleChannel()
{
	if (m_iMenuChannel == -1)
		m_iMenuChannel = m_pPatternEditor->GetChannel();

	ToggleChannel(m_iMenuChannel);

	m_iMenuChannel = -1;
}

void CFamiTrackerView::OnTrackerSoloChannel()
{
	if (m_iMenuChannel == -1)
		m_iMenuChannel = m_pPatternEditor->GetChannel();

	SoloChannel(m_iMenuChannel);

	m_iMenuChannel = -1;
}

void CFamiTrackerView::OnTrackerToggleChip()		// // //
{
	if (m_iMenuChannel == -1)
		m_iMenuChannel = m_pPatternEditor->GetChannel();

	ToggleChip(m_iMenuChannel);

	m_iMenuChannel = -1;
}

void CFamiTrackerView::OnTrackerSoloChip()		// // //
{
	if (m_iMenuChannel == -1)
		m_iMenuChannel = m_pPatternEditor->GetChannel();

	SoloChip(m_iMenuChannel);

	m_iMenuChannel = -1;
}

void CFamiTrackerView::OnTrackerUnmuteAllChannels()
{
	UnmuteAllChannels();
}

void CFamiTrackerView::OnTrackerRecordToInst()		// // //
{
	if (m_iMenuChannel == -1)
		m_iMenuChannel = m_pPatternEditor->GetChannel();

	CFamiTrackerDoc	*pDoc = GetDocument();
	int Channel = pDoc->GetChannelType(m_iMenuChannel);
	int Chip = pDoc->GetChipType(m_iMenuChannel);
	m_iMenuChannel = -1;

	if (Channel == CHANID_DPCM || Chip == SNDCHIP_VRC7) {
		AfxMessageBox(IDS_DUMP_NOT_SUPPORTED, MB_ICONERROR); return;
	}
	if (pDoc->GetInstrumentCount() >= MAX_INSTRUMENTS) {
		AfxMessageBox(IDS_INST_LIMIT, MB_ICONERROR); return;
	}
	if (Chip != SNDCHIP_FDS) {
		inst_type_t Type = INST_NONE;
		switch (Chip) {
		case SNDCHIP_NONE: case SNDCHIP_MMC5: Type = INST_2A03; break;
		case SNDCHIP_VRC6: Type = INST_VRC6; break;
		case SNDCHIP_N163: Type = INST_N163; break;
		case SNDCHIP_S5B:  Type = INST_S5B; break;
		}
		if (Type != INST_NONE) for (int i = 0; i < SEQ_COUNT; i++)
			if (pDoc->GetFreeSequence(Type, i) == -1) {
				AfxMessageBox(IDS_SEQUENCE_LIMIT, MB_ICONERROR); return;
			}
	}

	if (IsChannelMuted(m_pPatternEditor->GetChannel()))
		ToggleChannel(m_pPatternEditor->GetChannel());
	theApp.GetSoundGenerator()->SetRecordChannel(Channel == theApp.GetSoundGenerator()->GetRecordChannel() ? -1 : Channel);
	InvalidateHeader();
}

void CFamiTrackerView::OnTrackerRecorderSettings()
{
	CRecordSettingsDlg dlg;
	if (dlg.DoModal() == IDOK)
		theApp.GetSoundGenerator()->SetRecordSetting(dlg.GetRecordSetting());
}

void CFamiTrackerView::AdjustOctave(int Delta)		// // //
{
	for (auto &x : m_iNoteCorrection)
		x.second -= Delta;
}

int CFamiTrackerView::GetSelectedChipType() const
{
	CFamiTrackerDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	return pDoc->GetChipType(m_pPatternEditor->GetChannel());
}

void CFamiTrackerView::OnIncreaseStepSize()
{
	if (m_iInsertKeyStepping < MAX_PATTERN_LENGTH)		// // //
		SetStepping(m_iInsertKeyStepping + 1);
}

void CFamiTrackerView::OnDecreaseStepSize()
{
	if (m_iInsertKeyStepping > 0)
		SetStepping(m_iInsertKeyStepping - 1);
}

void CFamiTrackerView::SetStepping(int Step)
{
	m_iInsertKeyStepping = Step;
	static_cast<CMainFrame*>(GetParentFrame())->UpdateControls();
}

unsigned CFamiTrackerView::MoveKeyStepping() const {
	auto step = m_iInsertKeyStepping;
	if (step > 0 && !theApp.GetSettings()->General.bNoStepMove) {
		return step;
	} else {
		return 1;
	}
}

void CFamiTrackerView::OnEditInterpolate()
{
	if (!m_bEditEnable) return;		// // //
	AddAction(new CPActionInterpolate { });
}

void CFamiTrackerView::OnEditReverse()
{
	if (!m_bEditEnable) return;		// // //
	AddAction(new CPActionReverse { });
}

void CFamiTrackerView::OnEditReplaceInstrument()
{
	if (!m_bEditEnable) return;		// // //
	AddAction(new CPActionReplaceInst {GetInstrument()});
}

void CFamiTrackerView::OnEditExpandPatterns()		// // //
{
	if (!m_bEditEnable) return;
	AddAction(new CPActionStretch {std::vector<int> {1, 0}});
}

void CFamiTrackerView::OnEditShrinkPatterns()		// // //
{
	if (!m_bEditEnable) return;
	AddAction(new CPActionStretch {std::vector<int> {2}});
}

void CFamiTrackerView::OnEditStretchPatterns()		// // //
{
	if (!m_bEditEnable) return;

	CStretchDlg StretchDlg;
	AddAction(new CPActionStretch {StretchDlg.GetStretchMap()});
}

void CFamiTrackerView::OnNcMouseMove(UINT nHitTest, CPoint point)
{
	if (m_pPatternEditor->OnMouseNcMove())
		InvalidateHeader();

	CView::OnNcMouseMove(nHitTest, point);
}

void CFamiTrackerView::OnOneStepUp()
{
	m_pPatternEditor->MoveUp(SINGLE_STEP);
	InvalidateCursor();
}

void CFamiTrackerView::OnOneStepDown()
{
	m_pPatternEditor->MoveDown(SINGLE_STEP);
	InvalidateCursor();
}

void CFamiTrackerView::MakeSilent()
{
	m_iAutoArpPtr		= 0;
	m_iLastAutoArpPtr	= 0;
	m_iAutoArpKeyCount	= 0;

	memset(m_iActiveNotes, 0, sizeof(int) * MAX_CHANNELS);
	memset(m_cKeyList, 0, sizeof(char) * 256);
	memset(m_iArpeggiate, 0, sizeof(int) * MAX_CHANNELS);
	memset(m_iAutoArpNotes, 0, sizeof(char) * 128);
}

bool CFamiTrackerView::IsSelecting() const
{
	return m_pPatternEditor->IsSelecting();
}

bool CFamiTrackerView::IsClipboardAvailable() const
{
	return ::IsClipboardFormatAvailable(mClipboardFormat) == TRUE;
}

void CFamiTrackerView::OnBlockStart()
{
	m_pPatternEditor->SetBlockStart();
	InvalidateCursor();
}

void CFamiTrackerView::OnBlockEnd()
{
	m_pPatternEditor->SetBlockEnd();
	InvalidateCursor();
}

void CFamiTrackerView::OnPickupRow()
{
	// Get row info
	CFamiTrackerDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	int Track = static_cast<CMainFrame*>(GetParentFrame())->GetSelectedTrack();
	int Frame = m_pPatternEditor->GetFrame();
	int Row = m_pPatternEditor->GetRow();
	int Channel = m_pPatternEditor->GetChannel();

	stChanNote Note;

	pDoc->GetNoteData(Track, Frame, Channel, Row, &Note);

	m_iLastVolume = Note.Vol;
	m_iLastInstrument = Note.Instrument;		// // //
	if (Note.Instrument != MAX_INSTRUMENTS)
		SetInstrument(Note.Instrument);

	switch (Note.Note) {
	case NONE:    m_iLastNote = 0; break;
	case HALT:    m_iLastNote = NOTE_HALT; break;
	case RELEASE: m_iLastNote = NOTE_RELEASE; break;
	case ECHO:    m_iLastNote = NOTE_ECHO + Note.Octave; break;
	default:      m_iLastNote = (Note.Note - 1) + Note.Octave * 12;
	}

	column_t Col = GetSelectColumn(m_pPatternEditor->GetColumn());		// // //
	if (Col >= COLUMN_EFF1) {
		m_iLastEffect = Note.EffNumber[Col - COLUMN_EFF1];
		m_iLastEffectParam = Note.EffParam[Col - COLUMN_EFF1];
	}
}

bool CFamiTrackerView::AddAction(Action *pAction) const
{
	// Performs an action and adds it to the undo queue
	CMainFrame *pMainFrm = static_cast<CMainFrame*>(GetParentFrame());
	return (pMainFrm != NULL) ? pMainFrm->AddAction(pAction) : false;
}

// OLE support

DROPEFFECT CFamiTrackerView::OnDragEnter(COleDataObject* pDataObject, DWORD dwKeyState, CPoint point)
{
	TRACE("OLE: OnDragEnter\n");

	sel_condition_t Cond = m_pPatternEditor->GetSelectionCondition();
	if (m_pPatternEditor->GetSelectionCondition() == SEL_NONTERMINAL_SKIP) {		// // //
		MessageBeep(MB_ICONWARNING);
		static_cast<CMainFrame*>(GetParentFrame())->SetMessageText(IDS_SEL_NONTERMINAL_SKIP);
		m_nDropEffect = DROPEFFECT_NONE;
	}
	else if (m_pPatternEditor->GetSelectionCondition() == SEL_REPEATED_ROW) {		// // //
		MessageBeep(MB_ICONWARNING);
		static_cast<CMainFrame*>(GetParentFrame())->SetMessageText(IDS_SEL_REPEATED_ROW);
		m_nDropEffect = DROPEFFECT_NONE;
	}
	else if (pDataObject->IsDataAvailable(mClipboardFormat)) {
		if (dwKeyState & (MK_CONTROL | MK_SHIFT)) {
			m_nDropEffect = DROPEFFECT_COPY;
			if (dwKeyState & MK_SHIFT)
				m_bDropMix = true;
			else
				m_bDropMix = false;
		}
		else {
			m_nDropEffect = DROPEFFECT_MOVE;
		}

		// Get drag rectangle
		std::shared_ptr<CPatternClipData> pDragData(new CPatternClipData());		// // //

		HGLOBAL hMem = pDataObject->GetGlobalData(mClipboardFormat);
		pDragData->FromMem(hMem);

		// Begin drag operation
		m_pPatternEditor->BeginDrag(pDragData.get());
		m_pPatternEditor->UpdateDrag(point);

		InvalidateCursor();
	}

	return m_nDropEffect;
}

void CFamiTrackerView::OnDragLeave()
{
	TRACE("OLE: OnDragLeave\n");

	if (m_nDropEffect != DROPEFFECT_NONE) {
		m_pPatternEditor->EndDrag();
		InvalidateCursor();
	}

	m_nDropEffect = DROPEFFECT_NONE;

	CView::OnDragLeave();
}

DROPEFFECT CFamiTrackerView::OnDragOver(COleDataObject* pDataObject, DWORD dwKeyState, CPoint point)
{
	TRACE("OLE: OnDragOver\n");

	// Update drag'n'drop cursor
	if (m_nDropEffect != DROPEFFECT_NONE) {
		m_pPatternEditor->UpdateDrag(point);
		InvalidateCursor();
	}

	return m_nDropEffect;
}

BOOL CFamiTrackerView::OnDrop(COleDataObject* pDataObject, DROPEFFECT dropEffect, CPoint point)
{
	TRACE("OLE: OnDrop\n");

	BOOL Result = FALSE;

	// Perform drop
	if (m_nDropEffect != DROPEFFECT_NONE) {

		bool bCopy = (dropEffect == DROPEFFECT_COPY) || (m_bDragSource == false);

		m_pPatternEditor->UpdateDrag(point);

		// Get clipboard data
		CPatternClipData *pClipData = new CPatternClipData();
		HGLOBAL hMem = pDataObject->GetGlobalData(mClipboardFormat);
		pClipData->FromMem(hMem);

		// Paste into pattern
		if (!m_pPatternEditor->PerformDrop(pClipData, bCopy, m_bDropMix)) {
			SAFE_RELEASE(pClipData);
		}

		InvalidateCursor();
		m_bDropped = true;

		Result = TRUE;
	}

	m_nDropEffect = DROPEFFECT_NONE;

	return Result;
}

void CFamiTrackerView::BeginDragData(int ChanOffset, int RowOffset)
{
	TRACE("OLE: BeginDragData\n");

	std::shared_ptr<COleDataSource> pSrc(new COleDataSource());		// // //
	std::shared_ptr<CPatternClipData> pClipData(m_pPatternEditor->Copy());
	SIZE_T Size = pClipData->GetAllocSize();

	pClipData->ClipInfo.OleInfo.ChanOffset = ChanOffset;
	pClipData->ClipInfo.OleInfo.RowOffset = RowOffset;

	HGLOBAL hMem = ::GlobalAlloc(GMEM_MOVEABLE, Size);

	if (hMem != NULL) {
		// Copy data
		pClipData->ToMem(hMem);

		m_bDragSource = true;
		m_bDropped = false;

		pSrc->CacheGlobalData(mClipboardFormat, hMem);
		DROPEFFECT res = pSrc->DoDragDrop(DROPEFFECT_COPY | DROPEFFECT_MOVE);

		if (m_bDropped == false) {
			// Target was another window
			switch (res) {
				case DROPEFFECT_MOVE:
					// Delete data
					AddAction(new CPActionClearSel { });		// // //
					break;
			}

			m_pPatternEditor->CancelSelection();
		}
	}

	m_bDragSource = false;

	::GlobalFree(hMem);
}

bool CFamiTrackerView::IsDragging() const
{
	return m_bDragSource;
}

void CFamiTrackerView::EditReplace(stChanNote &Note)		// // //
{
	AddAction(new CPActionEditNote(Note));
	InvalidateCursor();
	// pAction->SaveRedoState(static_cast<CMainFrame*>(GetParentFrame()));		// // //
}

void CFamiTrackerView::OnUpdateFind(CCmdUI *pCmdUI)		// // //
{
	InvalidateCursor();
}

void CFamiTrackerView::OnRecallChannelState()		// // //
{
	int Channel = static_cast<CFamiTrackerDoc*>(m_pDocument)->GetChannelType(m_pPatternEditor->GetChannel());
	GetParentFrame()->SetMessageText(theApp.GetSoundGenerator()->RecallChannelState(Channel));
}

CString	CFamiTrackerView::GetEffectHint(const stChanNote &Note, int Column) const		// // //
{
	int Index = Note.EffNumber[Column];
	int Param = Note.EffParam[Column];
	if (Index >= EF_COUNT) return _T("Undefined effect");

	int Channel = m_pPatternEditor->GetChannel();
	int Chip = GetDocument()->GetChannel(Channel)->GetChip();
	if (Index > EF_FDS_VOLUME || (Index == EF_FDS_VOLUME && Param >= 0x40)) ++Index;
	if (Index > EF_TRANSPOSE || (Index == EF_TRANSPOSE && Param >= 0x80)) ++Index;
	if (Index > EF_SUNSOFT_ENV_TYPE || (Index == EF_SUNSOFT_ENV_TYPE && Param >= 0x10)) ++Index;
	if (Index > EF_FDS_MOD_SPEED_HI || (Index == EF_FDS_MOD_SPEED_HI && Param >= 0x10)) ++Index;
	if (Index > EF_FDS_MOD_DEPTH || (Index == EF_FDS_MOD_DEPTH && Param >= 0x80)) ++Index;
	if (Index > EF_NOTE_CUT || (Index == EF_NOTE_CUT && Param >= 0x80 && Channel == CHANID_TRIANGLE)) ++Index;
	if (Index > EF_DUTY_CYCLE || (Index == EF_DUTY_CYCLE && (Chip == SNDCHIP_VRC7 || Chip == SNDCHIP_N163))) ++Index;
	if (Index > EF_DUTY_CYCLE || (Index == EF_DUTY_CYCLE && Chip == SNDCHIP_N163)) ++Index;
	if (Index > EF_VOLUME || (Index == EF_VOLUME && Param >= 0xE0)) ++Index;
	if (Index > EF_SPEED || (Index == EF_SPEED && Param >= GetDocument()->GetSpeedSplitPoint())) ++Index;

	return EFFECT_TEXTS[Index];
}
