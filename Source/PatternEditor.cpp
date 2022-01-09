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
#include "DPI.h"

#include <algorithm>
#include <functional>		// // //
#include <vector>		// // //
#include <cmath>
#include "FamiTracker.h"
#include "FamiTrackerDoc.h"
#include "FamiTrackerView.h"
#include "PatternEditor.h"
#include "SoundGen.h"
#include "TrackerChannel.h"
#include "Settings.h"
#include "MainFrm.h"
#include "PatternAction.h"
#include "ColorScheme.h"
#include "Graphics.h"
#include "TextExporter.h"		// // //
#include "APU/APU.h"
#include "RegisterState.h"		// // //

/*
 * CPatternEditor
 * This is the pattern editor. This class is not derived from any MFC class.
 *
 */

// Define pattern layout here

const int CPatternEditor::HEADER_HEIGHT_NODPI = 36;
static constexpr int HEADER_CHAN_START = 0;
#define HEADER_CHAN_HEIGHT  HEADER_HEIGHT
static constexpr int ROW_HEIGHT = 12;

// Pattern header font
static constexpr LPCTSTR DEFAULT_HEADER_FONT = _T("Tahoma");

static constexpr int DEFAULT_HEADER_FONT_SIZE = 11;

// // //

void CopyNoteSection(stChanNote *Target, const stChanNote *Source, paste_mode_t Mode, column_t Begin, column_t End)		// // //
{
	if (Begin == COLUMN_NOTE && End == COLUMN_EFF4) {
		*Target = *Source;
		return;
	}
	static const char Offset[] = {
		offsetof(stChanNote, Note),
		offsetof(stChanNote, Instrument),
		offsetof(stChanNote, Vol),
		offsetof(stChanNote, EffNumber),
		offsetof(stChanNote, EffNumber) + 1,
		offsetof(stChanNote, EffNumber) + 2,
		offsetof(stChanNote, EffNumber) + 3,
	};
	bool Protected[sizeof(Offset)] = {};
	for (size_t i = 0; i < sizeof(Offset); ++i) {
		const unsigned char TByte = *(reinterpret_cast<unsigned char*>(Target) + Offset[i]); // skip octave byte
		const unsigned char SByte = *(reinterpret_cast<const unsigned char*>(Source) + Offset[i]);
		switch (Mode) {
		case PASTE_MIX:
			switch (i) {
			case COLUMN_NOTE:
				if (TByte != NONE) Protected[i] = true;
				break;
			case COLUMN_INSTRUMENT:
				if (TByte != MAX_INSTRUMENTS) Protected[i] = true;
				break;
			case COLUMN_VOLUME:
				if (TByte != MAX_VOLUME) Protected[i] = true;
				break;
			default:
				if (TByte != EF_NONE) Protected[i] = true;
			}
			// continue
		case PASTE_OVERWRITE:
			switch (i) {
			case COLUMN_NOTE:
				if (SByte == NONE) Protected[i] = true;
				break;
			case COLUMN_INSTRUMENT:
				if (SByte == MAX_INSTRUMENTS) Protected[i] = true;
				break;
			case COLUMN_VOLUME:
				if (SByte == MAX_VOLUME) Protected[i] = true;
				break;
			default:
				if (SByte == EF_NONE) Protected[i] = true;
			}
		}
	}

	if (theApp.GetSettings()->General.iEditStyle == EDIT_STYLE_IT) {
		switch (Mode) {
		case PASTE_MIX:
			if (Target->Note != NONE || Target->Instrument != MAX_INSTRUMENTS || Target->Vol != MAX_VOLUME)
				Protected[COLUMN_NOTE] = Protected[COLUMN_INSTRUMENT] = Protected[COLUMN_VOLUME] = true;
			// continue
		case PASTE_OVERWRITE:
			if (Source->Note == NONE && Source->Instrument == MAX_INSTRUMENTS && Source->Vol == MAX_VOLUME)
				Protected[COLUMN_NOTE] = Protected[COLUMN_INSTRUMENT] = Protected[COLUMN_VOLUME] = true;
		}
	}

	if (Begin > End) {
		column_t Temp = End; End = Begin; Begin = Temp;
	}

	for (unsigned i = Begin; i <= End; i++) if (!Protected[i]) switch (i) {
	case COLUMN_NOTE:
		Target->Note = Source->Note;
		Target->Octave = Source->Octave;
		break;
	case COLUMN_INSTRUMENT:
		Target->Instrument = Source->Instrument;
		break;
	case COLUMN_VOLUME:
		Target->Vol = Source->Vol;
		break;
	default:
		Target->EffNumber[i - 3] = Source->EffNumber[i - 3];
		Target->EffParam[i - 3] = Source->EffParam[i - 3];
	}
}

// CPatternEditor

CPatternEditor::CPatternEditor() :
	HEADER_HEIGHT(DPI::SY(HEADER_HEIGHT_NODPI)),
	// Pointers
	m_pDocument(NULL),
	m_pView(NULL),
	m_pPatternDC(NULL),
	m_pHeaderDC(NULL),
	m_pRegisterDC(NULL),		// // //
	m_pPatternBmp(NULL),
	m_pHeaderBmp(NULL),
	m_pRegisterBmp(NULL),		// // //
	// Drawing
	m_iWinWidth(0),
	m_iWinHeight(0),
	m_bPatternInvalidated(false),
	m_bCursorInvalidated(false),
	m_bBackgroundInvalidated(false),
	m_bHeaderInvalidated(false),
	m_bSelectionInvalidated(false),
	m_iCenterRow(0),
	m_iPatternLength(0),		// // //
	m_iLastCenterRow(0),
	m_iLastFrame(0),
	m_iLastFirstChannel(0),
	m_iLastPlayRow(0),
	m_iPlayRow(0),
	m_iPlayFrame(0),
	m_iPatternWidth(0),
	m_iPatternHeight(0),
	m_iLinesVisible(0),
	m_iLinesFullVisible(0),
	m_iChannelsVisible(0),
	m_iChannelsFullVisible(0),
	m_iFirstChannel(0),
	m_iRowHeight(ROW_HEIGHT),
	m_iCharWidth(10),		// // //
	m_iColumnSpacing(4),		// // //
	m_iRowColumnWidth(32),		// // //
	m_iPatternFontSize(ROW_HEIGHT),
	m_iDrawCursorRow(0),
	m_iDrawFrame(0),
	m_bFollowMode(true),
	m_bHasFocus(false),
	m_vHighlight(CPatternData::DEFAULT_HIGHLIGHT),		// // //
	m_iMouseHoverChan(-1),
	m_iMouseHoverEffArrow(0),
	m_bSelecting(false),
	m_bCurrentlySelecting(false),
	m_bDragStart(false),
	m_bDragging(false),
	m_bFullRowSelect(false),
	m_bMouseActive(false),
	m_iChannelPushed(-1),
	m_bChannelPushed(false),
	m_iDragChannels(0),
	m_iDragRows(0),
	m_iDragStartCol(C_NOTE),
	m_iDragEndCol(C_NOTE),
	m_iDragOffsetChannel(0),
	m_iDragOffsetRow(0),
	m_nScrollFlags(0),
	m_iScrolling(SCROLL_NONE),
	m_iCurrentHScrollPos(0),
	m_bCompactMode(false),		// // //
	m_iWarpCount(0),		// // //
	m_iDragBeginWarp(0),		// // //
	m_iSelectionCondition(SEL_CLEAN),		// // //
	// Benchmarking
	m_iRedraws(0),
	m_iFullRedraws(0),
	m_iQuickRedraws(0),
	m_iHeaderRedraws(0),
	m_iPaints(0),
	m_iErases(0),
	m_iBuffers(0),
	m_iCharsDrawn(0)
{
	// Get drag info from OS
	m_iDragThresholdX = ::GetSystemMetrics(SM_CXDRAG);
	m_iDragThresholdY = ::GetSystemMetrics(SM_CYDRAG);

	memset(m_iChannelWidths, 0, sizeof(int) * MAX_CHANNELS);
	memset(m_iColumns, 0, sizeof(int) * MAX_CHANNELS);
}

CPatternEditor::~CPatternEditor()
{
	SAFE_RELEASE(m_pPatternDC);
	SAFE_RELEASE(m_pPatternBmp);
	SAFE_RELEASE(m_pRegisterDC);		// // //
	SAFE_RELEASE(m_pRegisterBmp);		// // //
	SAFE_RELEASE(m_pHeaderDC);
	SAFE_RELEASE(m_pHeaderBmp);
}

// Drawing

void CPatternEditor::ApplyColorScheme()
{
	// The color scheme has changed
	//
	const CSettings *settings = theApp.GetSettings();

	LOGFONT LogFont;
	LPCTSTR FontName = settings->Appearance.strFont;		// // //
	LPCTSTR HeaderFace = DEFAULT_HEADER_FONT;

	COLORREF ColBackground = settings->Appearance.iColBackground;

	// Grid size
	// FIXME
	const int rowHeight = DPI::SY(settings->Appearance.rowHeight);
	m_iRowHeight = rowHeight;
	m_iCharWidth = rowHeight - 1;
	m_iColumnSpacing = (rowHeight + 1) / 3;
	m_iRowColumnWidth = m_iCharWidth * 3 + 2;

	// Font size (rounded to nearest pixel)
	const int fontSize = calculateFontSize(rowHeight, settings->Appearance.fontPercent);

	m_iPatternFontSize = fontSize;

	CalcLayout();

	// Create pattern font
	memset(&LogFont, 0, sizeof(LOGFONT));
	memcpy(LogFont.lfFaceName, FontName, _tcslen(FontName));

	LogFont.lfHeight = -m_iPatternFontSize;
	LogFont.lfQuality = DRAFT_QUALITY;
	LogFont.lfPitchAndFamily = DEFAULT_PITCH | FF_DONTCARE;

	// Remove old font
	if (m_fontPattern.m_hObject != NULL)
		m_fontPattern.DeleteObject();

	m_fontPattern.CreateFontIndirect(&LogFont);

	// Create header font
	memset(&LogFont, 0, sizeof(LOGFONT));
	memcpy(LogFont.lfFaceName, HeaderFace, _tcslen(HeaderFace));

	LogFont.lfHeight = -DPI::SY(DEFAULT_HEADER_FONT_SIZE);
	//LogFont.lfWeight = 550;
	LogFont.lfPitchAndFamily = VARIABLE_PITCH | FF_SWISS;

	// Remove old font
	if (m_fontHeader.m_hObject != NULL)
		m_fontHeader.DeleteObject();

	m_fontHeader.CreateFontIndirect(&LogFont);

	if (m_fontCourierNew.m_hObject == NULL)		// // // smaller
		m_fontCourierNew.CreateFont(
			DPI::SY(14), 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, 0, 0, 0, DRAFT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, _T("Courier New"));

	// Cache some colors
	m_colSeparator	= BLEND(ColBackground, (ColBackground ^ 0xFFFFFF), SHADE_LEVEL.SEPARATOR);
	m_colEmptyBg	= DIM(theApp.GetSettings()->Appearance.iColBackground, SHADE_LEVEL.EMPTY_BG);

	m_colHead1 = GetSysColor(COLOR_3DFACE);
	m_colHead2 = GetSysColor(COLOR_BTNHIGHLIGHT);
	m_colHead3 = GetSysColor(COLOR_APPWORKSPACE);
	m_colHead4 = BLEND(m_colHead3, 0x4040F0, 80);
	m_colHead5 = BLEND(m_colHead3, 0x40F040, 60);		// // //

	InvalidateBackground();
	InvalidatePatternData();
	InvalidateHeader();
}

void CPatternEditor::SetDocument(CFamiTrackerDoc *pDoc, CFamiTrackerView *pView)
{
	// Set a new document and view, reset everything
	m_pDocument = pDoc;
	m_pView = pView;

	// Reset
	ResetCursor();
}

void CPatternEditor::SetWindowSize(int width, int height)
{
	// Sets the window size of parent view
	m_iWinWidth = width;
	m_iWinHeight = height - ::GetSystemMetrics(SM_CYHSCROLL);

	CalcLayout();
}

void CPatternEditor::ResetCursor()
{
	// Clear cursor state to first row and first frame
	// Call this when for example changing track

	m_cpCursorPos	= CCursorPos();
	m_iCenterRow	= 0;
	m_iFirstChannel = 0;
	m_iPlayFrame	= 0;
	m_iPlayRow		= 0;

	m_bCurrentlySelecting = false;
	m_bSelecting = false;
	m_bDragStart = false;
	m_bDragging = false;

	m_iScrolling = SCROLL_NONE;

	CancelSelection();

	InvalidatePatternData();
}

// Flags

void CPatternEditor::InvalidatePatternData()
{
	// Pattern data has changed
	m_bPatternInvalidated = true;
}

void CPatternEditor::InvalidateCursor()
{
	// Cursor has moved
	m_bCursorInvalidated = true;
}

void CPatternEditor::InvalidateBackground()
{
	// Window size has changed, pattern layout has changed
	m_bBackgroundInvalidated = true;
}

void CPatternEditor::InvalidateHeader()
{
	// Channel header has changed
	m_bHeaderInvalidated = true;
}

void CPatternEditor::UpdatePatternLength()
{
	m_iPatternLength = GetCurrentPatternLength(m_cpCursorPos.m_iFrame);
	// // //
}

// Drawing

void CPatternEditor::DrawScreen(CDC *pDC, CFamiTrackerView *pView)
{
	//
	// Call this from the parent view's paint routine only
	//
	// It will both update the pattern editor picture (if necessary)
	// and paint it on the screen.
	//

//#define BENCHMARK

	ASSERT(m_pPatternDC != NULL);
	ASSERT(m_pHeaderDC != NULL);
	ASSERT(m_pRegisterDC != NULL);		// // //

	m_iCharsDrawn = 0;

	// Performance checking
#ifdef BENCHMARK
	LARGE_INTEGER StartTime, EndTime;
	LARGE_INTEGER Freq;
	QueryPerformanceCounter(&StartTime);
#endif

	//
	// Draw the pattern area, if necessary
	//

	bool bDrawPattern = m_bCursorInvalidated || m_bPatternInvalidated || m_bBackgroundInvalidated;
	bool bQuickRedraw = !m_bPatternInvalidated && !m_bBackgroundInvalidated;

	if (m_bSelectionInvalidated) {
		// Selection has changed, do full redraw
		bDrawPattern = true;
		bQuickRedraw = false;
	}

	// Drag & drop
	if (m_bDragging) {
		bDrawPattern = true;
		bQuickRedraw = false;
	}

	// New frames
	if (m_iLastFrame != m_cpCursorPos.m_iFrame) {		// // //
		UpdatePatternLength();
		bDrawPattern = true;
		bQuickRedraw = false;
	}

	if (m_iLastPlayRow != m_iPlayRow) {
		if (theApp.IsPlaying() && !m_bFollowMode) {
			bDrawPattern = true;		// // //
			bQuickRedraw = false;
		}
	}

	// First channel changed
	if (m_iLastFirstChannel != m_iFirstChannel) {
		bDrawPattern = true;
		bQuickRedraw = false;
	}

	if (bDrawPattern) {

		// Wrap arounds
		if (abs(m_iCenterRow - m_iLastCenterRow) >= (m_iLinesVisible / 2))
			bQuickRedraw = false;
		
		// Todo: fix this
		if (theApp.GetSettings()->General.bFreeCursorEdit)
			bQuickRedraw = false;

		// Todo: remove
		m_iDrawCursorRow = m_cpCursorPos.m_iRow;

		if (bQuickRedraw) {
			// Quick redraw is possible
			PerformQuickRedraw(m_pPatternDC);
		}
		else {
			// Perform a full redraw
			PerformFullRedraw(m_pPatternDC);
		}

		++m_iRedraws;
	}

	++m_iPaints;

	// Save state
	m_iLastCenterRow = m_iCenterRow;
	m_iLastFrame = m_cpCursorPos.m_iFrame;		// // //
	m_iLastFirstChannel = m_iFirstChannel;
	m_iLastPlayRow = m_iPlayRow;

	//
	// Draw pattern header, when needed
	//

	if (m_bHeaderInvalidated) {
		// Pattern header
		DrawHeader(m_pHeaderDC);
		++m_iHeaderRedraws;
	}
	DrawMeters(m_pHeaderDC);		// // //

	// Clear flags
	m_bPatternInvalidated = false;
	m_bCursorInvalidated = false;
	m_bBackgroundInvalidated = false;
	m_bHeaderInvalidated = false;
	m_bSelectionInvalidated = false;

	//
	// Blit to visible surface
	//
	
	const int iBlitHeight = m_iWinHeight - HEADER_HEIGHT;
	const int iBlitWidth = m_iPatternWidth + m_iRowColumnWidth;

//	if (iBlitWidth > m_iWinWidth)
//		iBlitWidth = m_iWinWidth;

	// Pattern area
	if (pDC->RectVisible(GetPatternRect()))
		pDC->BitBlt(0, HEADER_HEIGHT, iBlitWidth, iBlitHeight, m_pPatternDC, 0, 0, SRCCOPY);

	// Header area
	if (pDC->RectVisible(GetHeaderRect()))
		pDC->BitBlt(0, 0, m_iWinWidth, HEADER_HEIGHT, m_pHeaderDC, 0, 0, SRCCOPY);		// // //

	// Background
	//if (pDC->RectVisible(GetUnbufferedRect()))
	//	DrawUnbufferedArea(pDC);

	int Line = 1;

#ifdef _DEBUG
	pDC->SetBkColor(DEFAULT_COLOR_SCHEME.CURSOR);
	pDC->SetTextColor(DEFAULT_COLOR_SCHEME.TEXT_HILITE);
	pDC->TextOut(m_iWinWidth - 70, 42, _T("DEBUG"));
	pDC->TextOut(m_iWinWidth - 70, 62, _T("DEBUG"));
	pDC->TextOut(m_iWinWidth - 70, 82, _T("DEBUG"));
#endif
#ifdef BENCHMARK

	QueryPerformanceCounter(&EndTime);
	QueryPerformanceFrequency(&Freq);

	CRect clipBox;
	pDC->GetClipBox(&clipBox);
	CString txt;
	pDC->SetTextColor(0xFFFF);
	txt.Format(_T("Clip box: %ix%i %ix%i"), clipBox.top, clipBox.left, clipBox.bottom, clipBox.right);
	pDC->TextOut(10, 10, txt);
	txt.Format(_T("Pattern area: %i x %i"), m_iPatternWidth, m_iPatternHeight);
	pDC->TextOut(10, 30, txt);

	CString Text;
	int PosY = 100;
	const int LINE_BREAK = 18;
	pDC->SetTextColor(0xFFFF);
	pDC->SetBkColor(0);

#define PUT_TEXT(x) pDC->TextOut(m_iWinWidth - x, PosY, Text); PosY += LINE_BREAK

	Text.Format(_T("%i ms"), (__int64(EndTime.QuadPart) - __int64(StartTime.QuadPart)) / (__int64(Freq.QuadPart) / 1000)); PUT_TEXT(160);
	Text.Format(_T("%i redraws"), m_iRedraws); PUT_TEXT(160);
	Text.Format(_T("%i paints"), m_iPaints); PUT_TEXT(160);
	Text.Format(_T("%i quick redraws"), m_iQuickRedraws); PUT_TEXT(160);
	Text.Format(_T("%i full redraws"), m_iFullRedraws); PUT_TEXT(160);
	Text.Format(_T("%i header redraws"), m_iHeaderRedraws); PUT_TEXT(160);
	Text.Format(_T("%i erases"), m_iErases); PUT_TEXT(160);
	Text.Format(_T("%i new buffers"), m_iBuffers); PUT_TEXT(160);
	Text.Format(_T("%i chars drawn"), m_iCharsDrawn); PUT_TEXT(160);
	Text.Format(_T("%i rows visible"), m_iLinesVisible); PUT_TEXT(160);
	Text.Format(_T("%i full rows visible"), m_iLinesFullVisible); PUT_TEXT(160);
	Text.Format(_T("%i (%i) end sel"), m_selection.m_cpEnd.m_iChannel, GetChannelCount()); PUT_TEXT(160);
	Text.Format(_T("Channels: %i, %i"), m_iFirstChannel, m_iChannelsVisible); PUT_TEXT(160);

	PosY += 20;

	Text.Format(_T("Selection channel: %i - %i"), m_selection.m_cpStart.m_iChannel, m_selection.m_cpEnd.m_iChannel); PUT_TEXT(220);
	Text.Format(_T("Selection column: %i - %i"), m_selection.m_cpStart.m_iColumn, m_selection.m_cpEnd.m_iColumn); PUT_TEXT(220);
	Text.Format(_T("Selection row: %i - %i"), m_selection.m_cpStart.m_iRow, m_selection.m_cpEnd.m_iRow); PUT_TEXT(220);
	Text.Format(_T("Window width: %i - %i"), m_iWinWidth, m_iPatternWidth); PUT_TEXT(220);
	Text.Format(_T("Play: %i - %i"), m_iPlayFrame, m_iPlayRow); PUT_TEXT(220);
	Text.Format(_T("Middle row: %i"), m_iCenterRow); PUT_TEXT(220);

#else

#ifdef WIP
	// Display the BETA text
	CString Text;
	int line = 0;
	int offset = m_iWinWidth - 200;		// // //
	Text.Format(_T("Revision %i beta (%s)"), VERSION_REV, __DATE__);
	pDC->SetTextColor(0x00FFFF);
	pDC->SetBkMode(TRANSPARENT);
	pDC->TextOut(offset, m_iWinHeight - 24 - 18 * line++, Text);

#endif

#endif

	// Update scrollbars
	UpdateVerticalScroll();
	UpdateHorizontalScroll();
}

// Rect calculation

CRect CPatternEditor::GetActiveRect() const
{
	// Return the rect with pattern and header only
	return CRect(0, 0, m_iWinWidth, m_iWinHeight);		// // //
}

CRect CPatternEditor::GetHeaderRect() const
{
	return CRect(0, 0, m_iWinWidth, HEADER_HEIGHT);			// // //
}

CRect CPatternEditor::GetPatternRect() const
{
	// Return the rect with pattern and header only
	return CRect(0, HEADER_HEIGHT, m_iPatternWidth + m_iRowColumnWidth, m_iWinHeight);
}

CRect CPatternEditor::GetUnbufferedRect() const
{
	return CRect(m_iPatternWidth + m_iRowColumnWidth, 0, m_iWinWidth, m_iWinHeight);
}

CRect CPatternEditor::GetInvalidatedRect() const
{
	if (m_bHeaderInvalidated)
		return GetActiveRect();
	else if (theApp.GetSettings()->Display.bRegisterState)		// // //
		return GetActiveRect();

	return GetPatternRect();
}

void CPatternEditor::CalcLayout()
{
	int Height = m_iWinHeight - HEADER_HEIGHT;

	m_iLinesVisible		= (Height + m_iRowHeight - 1) / m_iRowHeight;
	m_iLinesFullVisible = Height / m_iRowHeight;
}

bool CPatternEditor::CalculatePatternLayout()
{
	// Calculate and cache pattern layout
	// must be called when layout or window size has changed
	const int Track = GetSelectedTrack();
	const int ChannelCount = GetChannelCount();
	const int PatternLength = m_pDocument->GetPatternLength(Track);
	const int LastPatternWidth = m_iPatternWidth;
	const int LastPatternHeight = m_iPatternHeight;

	// Get actual window width
	int WinWidth = m_iWinWidth;
	if (PatternLength > 1)
		WinWidth -= ::GetSystemMetrics(SM_CXVSCROLL);

	// Calculate channel widths
	int Offset = 0;
	for (int i = 0; i < ChannelCount; ++i) {
		int Width;		// // //
		if (m_bCompactMode) Width = (3 * m_iCharWidth + m_iColumnSpacing);
		else Width = m_iCharWidth * 9 + m_iColumnSpacing * 4 + m_pDocument->GetEffColumns(Track, i) * (3 * m_iCharWidth + m_iColumnSpacing);
		m_iChannelWidths[i] = Width + 1;
		m_iColumns[i] = GetChannelColumns(i);		// // //
		m_iChannelOffsets[i] = Offset;
		Offset += m_iChannelWidths[i];
	}

	// Calculate pattern width and height
	bool HiddenChannels = false;
	int LastChannel = ChannelCount;

	m_iPatternWidth = 0;
	for (int i = m_iFirstChannel; i < ChannelCount; ++i) {
		m_iPatternWidth += m_iChannelWidths[i];
		if ((m_iPatternWidth + m_iRowColumnWidth) >= WinWidth) {
			// We passed end of window width, there are hidden channels
			HiddenChannels = true;
			LastChannel = i + 1;
			break;
		}
	}

	if (HiddenChannels) {
		m_iChannelsVisible = LastChannel - m_iFirstChannel;
		m_iChannelsFullVisible = LastChannel - m_iFirstChannel - 1;
	}
	else {
		m_iChannelsVisible = ChannelCount - m_iFirstChannel;
		m_iChannelsFullVisible = ChannelCount - m_iFirstChannel;
	}

	// Pattern height
	m_iPatternHeight = m_iLinesVisible * m_iRowHeight;

	// Need full redraw after this
	InvalidateBackground();

	// Return a flag telling if buffers must be created
	return (m_iPatternWidth != LastPatternWidth) || (m_iPatternHeight != LastPatternHeight);
}

bool CPatternEditor::CursorUpdated()
{
	// This method must be called after the cursor has changed
	// Returns true if a new pattern layout is needed

	// No channels visible, create the pattern layout
	if (m_iChannelsVisible == 0)
		return true;

	const int Frames = GetFrameCount();		// // //
	const int ChannelCount = GetChannelCount();

	bool bUpdateNeeded = false;

	// Update pattern lengths
	UpdatePatternLength();

	// Channel cursor moved left of first visible channel
	if (m_iFirstChannel > m_cpCursorPos.m_iChannel) {
		m_iFirstChannel = m_cpCursorPos.m_iChannel;
		bUpdateNeeded = true;
	}

	// Channel cursor moved to the right of all visible channels
	while ((m_cpCursorPos.m_iChannel - m_iFirstChannel) > (m_iChannelsFullVisible - 1)) {
		++m_iFirstChannel;
		bUpdateNeeded = true;
	}

	if (m_iFirstChannel + m_iChannelsVisible > ChannelCount)
		m_iChannelsVisible = ChannelCount - m_iFirstChannel;

	if (m_iChannelsFullVisible > m_iChannelsVisible)
		m_iChannelsFullVisible = m_iChannelsVisible;

	for (int i = 0; i < ChannelCount; ++i) {
		if (i == m_cpCursorPos.m_iChannel) {
			if (m_cpCursorPos.m_iColumn > m_iColumns[i])
				m_cpCursorPos.m_iColumn = m_iColumns[i];
		}
	}

	if (m_cpCursorPos.m_iRow > m_iPatternLength - 1)
		m_cpCursorPos.m_iRow = m_iPatternLength - 1;

	// Frame
	if (m_cpCursorPos.m_iFrame >= Frames)		// // //
		m_cpCursorPos.m_iFrame = Frames - 1;

	// Ignore user cursor moves if the player is playing
	if (theApp.IsPlaying()) {

		const CSoundGen *pSoundGen = theApp.GetSoundGenerator();
		// Store a synchronized copy of frame & row position from player
		m_iPlayFrame = pSoundGen->GetPlayerFrame();
		m_iPlayRow = pSoundGen->GetPlayerRow();
		
		if (m_bFollowMode) {
			m_cpCursorPos.m_iRow = m_iPlayRow;
			m_cpCursorPos.m_iFrame = m_iPlayFrame;		// // //
		}
	}
	else {
		m_iPlayRow = -1;
	}

	// Decide center row
	if (theApp.GetSettings()->General.bFreeCursorEdit) {

		// Adjust if cursor is out of screen
		if (m_iCenterRow < m_iLinesVisible / 2)
			m_iCenterRow = m_iLinesVisible / 2;

		int CursorDifference = m_cpCursorPos.m_iRow - m_iCenterRow;

		// Bottom
		while (CursorDifference >= (m_iLinesFullVisible / 2) && CursorDifference > 0) {
			// Change these if you want one whole page to scroll instead of single lines
			m_iCenterRow += 1;
			CursorDifference = (m_cpCursorPos.m_iRow - m_iCenterRow);
		}

		// Top
		while (-CursorDifference > (m_iLinesVisible / 2) && CursorDifference < 0) {
			m_iCenterRow -= 1;
			CursorDifference = (m_cpCursorPos.m_iRow - m_iCenterRow);
		}
	}
	else {
		m_iCenterRow = m_cpCursorPos.m_iRow;
	}

	// Erase if background needs to be redrawn
	if (m_bBackgroundInvalidated)
		bUpdateNeeded = true;

	return bUpdateNeeded;
}

void CPatternEditor::CreateBackground(CDC *pDC)
{
	// Called when the background is erased, create new pattern layout
	const bool bCreateBuffers = CalculatePatternLayout();

	// Make sure cursor is aligned
	if (CursorUpdated()) {
		InvalidateBackground();
	}

	//if (m_iLastFirstChannel != m_iFirstChannel)
		InvalidateHeader();

	// Allocate backbuffer area, only if window size or pattern width has changed
	if (bCreateBuffers || true) {		// // // very hacky

		// Allocate backbuffer
		SAFE_RELEASE(m_pPatternBmp);
		SAFE_RELEASE(m_pHeaderBmp);
		SAFE_RELEASE(m_pRegisterBmp);		// // //
		SAFE_RELEASE(m_pPatternDC);
		SAFE_RELEASE(m_pHeaderDC);
		SAFE_RELEASE(m_pRegisterDC);		// // //

		m_pPatternBmp = new CBitmap;
		m_pHeaderBmp = new CBitmap;
		m_pRegisterBmp = new CBitmap;		// // //
		m_pPatternDC = new CDC;
		m_pHeaderDC = new CDC;
		m_pRegisterDC = new CDC;		// // //

		int Width  = m_iRowColumnWidth + m_iPatternWidth;
		int Height = m_iPatternHeight;

		// Setup pattern dc
		m_pPatternBmp->CreateCompatibleBitmap(pDC, Width, Height);
		m_pPatternDC->CreateCompatibleDC(pDC);
		m_pPatternDC->SelectObject(m_pPatternBmp);

		// Setup header dc
		m_pHeaderBmp->CreateCompatibleBitmap(pDC, m_iWinWidth, HEADER_HEIGHT);		// // //
		m_pHeaderDC->CreateCompatibleDC(pDC);
		m_pHeaderDC->SelectObject(m_pHeaderBmp);

		// // // Setup registers dc
		m_pRegisterBmp->CreateCompatibleBitmap(pDC, std::max(0, m_iWinWidth - Width), Height);
		m_pRegisterDC->CreateCompatibleDC(pDC);
		m_pRegisterDC->SelectObject(m_pRegisterBmp);

		++m_iBuffers;
	}

	++m_iErases;
}

void CPatternEditor::DrawUnbufferedArea(CDC *pDC)
{
	// This part of the surface doesn't contain anything useful

	if (m_iPatternWidth < m_iWinWidth) {
		int Width = m_iWinWidth - m_iPatternWidth - m_iRowColumnWidth;
		if (m_iPatternLength > 1)
			Width -= ::GetSystemMetrics(SM_CXVSCROLL);

		// Channel header background
		// GradientRectTriple(pDC, m_iPatternWidth + m_iRowColumnWidth, HEADER_CHAN_START, Width, HEADER_HEIGHT, m_colHead1, m_colHead2, m_pView->GetEditMode() ? m_colHead4 : m_colHead3);
		// pDC->Draw3dRect(m_iPatternWidth + m_iRowColumnWidth, HEADER_CHAN_START, Width, HEADER_HEIGHT, STATIC_COLOR_SCHEME.FRAME_LIGHT, STATIC_COLOR_SCHEME.FRAME_DARK);

		// The big empty area
		pDC->FillSolidRect(m_iPatternWidth + m_iRowColumnWidth, HEADER_HEIGHT, Width, m_iWinHeight - HEADER_HEIGHT, m_colEmptyBg);	
	}
}

void CPatternEditor::PerformFullRedraw(CDC *pDC)
{
	// Draw entire pattern area
	
	const int Channels = GetChannelCount();
	const int FrameCount = GetFrameCount();		// // //
	int Row = m_iCenterRow - m_iLinesVisible / 2;

	CFont *pOldFont = pDC->SelectObject(&m_fontPattern);

	pDC->SetBkMode(TRANSPARENT);

	for (int i = 0; i < m_iLinesVisible; ++i)
		PrintRow(pDC, Row++, i, m_cpCursorPos.m_iFrame);		// // //

	// Last unvisible row
	ClearRow(pDC, m_iLinesVisible);

	pDC->SetWindowOrg(-m_iRowColumnWidth, 0);

	// Lines between channels
	int Offset = m_iChannelWidths[m_iFirstChannel];
	
	for (int i = m_iFirstChannel; i < Channels; ++i) {
		pDC->FillSolidRect(Offset - 1, 0, 1, m_iPatternHeight, m_colSeparator);
		Offset += m_iChannelWidths[i + 1];
	}

	// First line (after row number column)
	pDC->FillSolidRect(-1, 0, 1, m_iPatternHeight, m_colSeparator);

	// Restore
	pDC->SetWindowOrg(0, 0);
	pDC->SelectObject(pOldFont);

	++m_iFullRedraws;
}

void CPatternEditor::PerformQuickRedraw(CDC *pDC)
{
	// Draw specific parts of pattern area
	ASSERT(m_cpCursorPos.m_iFrame == m_iLastFrame);

	// Number of rows that has changed
	const int DiffRows = m_iCenterRow - m_iLastCenterRow;

	CFont *pOldFont = pDC->SelectObject(&m_fontPattern);

	ScrollPatternArea(pDC, DiffRows);

	// Play cursor
	if (theApp.IsPlaying() && !m_bFollowMode) {
		//PrintRow(pDC, m_iPlayRow, 
	}
	else if (!theApp.IsPlaying() && m_iLastPlayRow != -1) {
		if (m_iPlayFrame == m_cpCursorPos.m_iFrame) {
			int Line = RowToLine(m_iLastPlayRow);
			if (Line >= 0 && Line <= m_iLinesVisible) {
				// Erase 
				PrintRow(pDC, m_iLastPlayRow, Line, m_cpCursorPos.m_iFrame);
			}
		}
	}

	// Restore
	pDC->SetWindowOrg(0, 0);
	pDC->SelectObject(pOldFont);

	UpdateVerticalScroll();

	++m_iQuickRedraws;
}

void CPatternEditor::PrintRow(CDC *pDC, int Row, int Line, int Frame) const
{
	const int FrameCount = GetFrameCount();		// // //
	const int rEnd = (theApp.IsPlaying() && m_bFollowMode) ? std::max(m_iPlayRow + 1, m_iPatternLength) : m_iPatternLength;
	if (Row >= 0 && Row < rEnd) {
		DrawRow(pDC, Row, Line, Frame, false);
	}
	else if (theApp.GetSettings()->General.bFramePreview) {
		if (Row >= rEnd) { // first frame
			Row -= rEnd;
			Frame++;
		}
		while (Row >= GetCurrentPatternLength(Frame)) {		// // //
			Row -= GetCurrentPatternLength(Frame++);
			/*if (Frame >= FrameCount) {
				Frame = 0;
				// if (Row) Row--; else { ClearRow(pDC, Line); return; }
			}*/
		}
		while (Row < 0) {		// // //
			/*if (Frame <= 0) {
				Frame = FrameCount;
				// if (Row != -1) Row++; else { ClearRow(pDC, Line); return; }
			}*/
			Row += GetCurrentPatternLength(--Frame);
		}
		DrawRow(pDC, Row, Line, Frame, true);
	}
	else {
		ClearRow(pDC, Line);
	}
}

void CPatternEditor::MovePatternArea(CDC *pDC, int FromRow, int ToRow, int NumRows) const
{
	// Move a part of the pattern area
	const int Width = m_iRowColumnWidth + m_iPatternWidth - 1;
	const int SrcY = FromRow * m_iRowHeight;
	const int DestY = ToRow * m_iRowHeight;
	const int Height = NumRows * m_iRowHeight;
	pDC->BitBlt(1, DestY, Width, Height, pDC, 1, SrcY, SRCCOPY); 
}

void CPatternEditor::ScrollPatternArea(CDC *pDC, int Rows) const
{
	ASSERT(Rows < (m_iLinesVisible / 2));

	pDC->SetBkMode(TRANSPARENT);

	const int FrameCount = GetFrameCount();		// // //
	const int MiddleLine = m_iLinesVisible / 2;
	const int Frame = m_cpCursorPos.m_iFrame;		// // //

	const int FirstLineCount = MiddleLine;	// Lines above cursor
	const int SecondLineCount = MiddleLine - ((m_iLinesVisible & 1) ? 0 : 1);	// Lines below cursor

	// Move existing areas
	if (Rows > 0) {
		MovePatternArea(pDC, Rows, 0, FirstLineCount - Rows);
		MovePatternArea(pDC, MiddleLine + Rows + 1, MiddleLine + 1, SecondLineCount - Rows);
	}
	else if (Rows < 0) {
		MovePatternArea(pDC, 0, -Rows, FirstLineCount + Rows);
		MovePatternArea(pDC, MiddleLine + 1, MiddleLine - Rows + 1, SecondLineCount + Rows);
	}

	// Fill new sections
	if (Rows > 0) {
		// Above cursor
		for (int i = 0; i < Rows; ++i) {
			int Row = m_iDrawCursorRow - 1 - i;
			int Line = MiddleLine - 1 - i;
			PrintRow(pDC, Row, Line, Frame);
		}
		// Bottom of screen
		for (int i = 0; i < Rows; ++i) {
			int Row = m_iDrawCursorRow + SecondLineCount - i;
			int Line = m_iLinesVisible - 1 - i;
			PrintRow(pDC, Row, Line, Frame);
		}
	}
	else if (Rows < 0) {
		// Top of screen
		for (int i = 0; i < -Rows; ++i) {
			int Row = m_iDrawCursorRow - FirstLineCount + i;
			int Line = i;
			PrintRow(pDC, Row, Line, Frame);
		}
		// Below cursor
		for (int i = 0; i < -Rows; ++i) {
			int Row = m_iDrawCursorRow + 1 + i;
			int Line = MiddleLine + 1 + i;
			PrintRow(pDC, Row, Line, Frame);
		}
	}

	// Draw cursor line, draw separately to allow calling this with zero rows
	const int Row = m_iDrawCursorRow;
	PrintRow(pDC, Row, MiddleLine, Frame);
}

void CPatternEditor::ClearRow(CDC *pDC, int Line) const
{
	pDC->SetWindowOrg(0, 0);	

	int Offset = m_iRowColumnWidth;
	for (int i = m_iFirstChannel; i < m_iFirstChannel + m_iChannelsVisible; ++i) {
		pDC->FillSolidRect(Offset, Line * m_iRowHeight, m_iChannelWidths[i] - 1, m_iRowHeight, m_colEmptyBg);
		Offset += m_iChannelWidths[i];
	}

	// Row number
	pDC->FillSolidRect(1, Line * m_iRowHeight, m_iRowColumnWidth - 2, m_iRowHeight, m_colEmptyBg);
}

// // // gone

bool CPatternEditor::IsInRange(const CSelection &sel, int Frame, int Row, int Channel, cursor_column_t Column) const		// // //
{
	// Return true if cursor is in range of selection
	if (Channel <= sel.GetChanStart() && (Channel != sel.GetChanStart() || Column < sel.GetColStart()))
		return false;
	if (Channel >= sel.GetChanEnd() && (Channel != sel.GetChanEnd() || Column > sel.GetColEnd()))
		return false;

	const int Frames = GetFrameCount();
	int fStart = sel.GetFrameStart() % Frames;
	if (fStart < 0) fStart += Frames;
	int fEnd = sel.GetFrameEnd() % Frames;
	if (fEnd < 0) fEnd += Frames;
	Frame %= Frames;
	if (Frame < 0) Frame += Frames;

	bool InStart = Frame > fStart || (Frame == fStart && Row >= sel.GetRowStart());
	bool InEnd = Frame < fEnd || (Frame == fEnd && Row <= sel.GetRowEnd());
	if (fStart > fEnd || (fStart == fEnd && sel.GetRowStart() > sel.GetRowEnd())) // warp across first/last frame
		return InStart || InEnd;
	else
		return InStart && InEnd;
}

// Draw a single row
void CPatternEditor::DrawRow(CDC *pDC, int Row, int Line, int Frame, bool bPreview) const
{
	// Row is row from pattern to display
	// Line is (absolute) screen line

	const COLORREF GRAY_BAR_COLOR = 0x606060;
	const COLORREF SEL_DRAG_COL	  = 0xA08080;

	const unsigned int PREVIEW_SHADE_LEVEL = 70;

	// TODO use dependency injection on pSettings, so we can use a 
	// CPatternEditor object to preview our color picker.

	const CSettings *pSettings = theApp.GetSettings();		// // //

	COLORREF ColCursor	= pSettings->Appearance.iColCursor;
	COLORREF ColBg		= pSettings->Appearance.iColBackground;
	COLORREF ColHiBg	= pSettings->Appearance.iColBackgroundHilite;
	COLORREF ColHiBg2	= pSettings->Appearance.iColBackgroundHilite2;
	COLORREF ColSelect	= pSettings->Appearance.iColSelection;

	const bool bEditMode = m_pView->GetEditMode();

	const int Track = GetSelectedTrack();
	const int Channels = /*m_iFirstChannel +*/ m_iChannelsVisible;
	int OffsetX = m_iRowColumnWidth;

	stChanNote NoteData;

	// Start at row number column
	pDC->SetWindowOrg(0, 0);

	if (Frame != m_cpCursorPos.m_iFrame && !pSettings->General.bFramePreview) {
		ClearRow(pDC, Line);
		return;
	}

	// Highlight
	unsigned int Highlight = m_pDocument->GetHighlightState(Track, Frame, Row);		// // //

	// Clear
	pDC->FillSolidRect(1, Line * m_iRowHeight, m_iRowColumnWidth - 2, m_iRowHeight, ColBg);
	if (m_pDocument->GetBookmarkAt(Track, Frame, Row))
		pDC->FillSolidRect(1, Line * m_iRowHeight, m_iRowColumnWidth - 2, m_iRowHeight, ColHiBg);

	COLORREF TextColor;

	if (Highlight == 2)
		TextColor = pSettings->Appearance.iColPatternTextHilite2;
	else if (Highlight == 1)
		TextColor = pSettings->Appearance.iColPatternTextHilite;
	else
		TextColor = pSettings->Appearance.iColPatternText;

	if (bPreview) {
		ColHiBg2 = DIM(ColHiBg2, PREVIEW_SHADE_LEVEL);
		ColHiBg = DIM(ColHiBg, PREVIEW_SHADE_LEVEL);
		ColBg = DIM(ColBg, PREVIEW_SHADE_LEVEL);
		TextColor = DIM(TextColor, 70);
	}

	// // // 050B
	// Draw row marker
	if (!((Frame - m_pView->GetMarkerFrame()) % GetFrameCount()) && Row == m_pView->GetMarkerRow())
		GradientBar(pDC, 2, Line * m_iRowHeight, m_iRowColumnWidth - 5, m_iRowHeight, ColCursor, DIM(ColCursor, 30));

	// Draw row number
	pDC->SetTextAlign(TA_CENTER | TA_BASELINE);		// // //

	CString Text;

	if (pSettings->General.bRowInHex) {
		// // // Hex display
		Text.Format(_T("%02X"), Row);
		DrawChar(pDC, (m_iRowColumnWidth - m_iCharWidth) / 2, (Line + 1) * m_iRowHeight - m_iRowHeight / 8, Text[0], TextColor);
		DrawChar(pDC, (m_iRowColumnWidth + m_iCharWidth) / 2, (Line + 1) * m_iRowHeight - m_iRowHeight / 8, Text[1], TextColor);
	}
	else {
		// // // Decimal display
		Text.Format(_T("%03d"), Row);
		DrawChar(pDC, m_iRowColumnWidth / 2 - m_iCharWidth, (Line + 1) * m_iRowHeight - m_iRowHeight / 8, Text[0], TextColor);
		DrawChar(pDC, m_iRowColumnWidth / 2				  , (Line + 1) * m_iRowHeight - m_iRowHeight / 8, Text[1], TextColor);
		DrawChar(pDC, m_iRowColumnWidth / 2 + m_iCharWidth, (Line + 1) * m_iRowHeight - m_iRowHeight / 8, Text[2], TextColor);
	}

	pDC->SetTextAlign(TA_LEFT);		// // //

	COLORREF BackColor;
	if (Highlight == 2)
		BackColor = ColHiBg2;	// Highlighted row
	else if (Highlight == 1)
		BackColor = ColHiBg;	// Highlighted row
	else
		BackColor = ColBg;		// Normal

	if (!bPreview && Row == m_iDrawCursorRow) {
		// Cursor row
		if (!m_bHasFocus)
			BackColor = BLEND(GRAY_BAR_COLOR, BackColor, SHADE_LEVEL.UNFOCUSED);	// Gray
		else if (bEditMode)
			BackColor = BLEND(pSettings->Appearance.iColCurrentRowEdit, BackColor, SHADE_LEVEL.FOCUSED);		// Red
		else
			BackColor = BLEND(pSettings->Appearance.iColCurrentRowNormal, BackColor, SHADE_LEVEL.FOCUSED);		// Blue
	}

	const COLORREF SelectColor = DIM(BLEND(ColSelect, BackColor, SHADE_LEVEL.SELECT),		// // //
		((Frame == m_cpCursorPos.m_iFrame) ? 100 : PREVIEW_SHADE_LEVEL));
	const COLORREF DragColor = DIM(BLEND(SEL_DRAG_COL, BackColor, SHADE_LEVEL.SELECT),
		((Frame == m_cpCursorPos.m_iFrame) ? 100 : PREVIEW_SHADE_LEVEL));
	const COLORREF SelectEdgeCol = (m_iSelectionCondition == SEL_CLEAN /* || m_iSelectionCondition == SEL_UNKNOWN_SIZE*/ ) ?
		DIM(BLEND(SelectColor, 0xFFFFFF, SHADE_LEVEL.SELECT_EDGE), ((Frame == m_cpCursorPos.m_iFrame) ? 100 : PREVIEW_SHADE_LEVEL)) :
		0x0000FF;

	RowColorInfo_t colorInfo;

	colorInfo.Note = TextColor;

	if (Highlight == 2)
		colorInfo.Back = pSettings->Appearance.iColBackgroundHilite2;
	else if (Highlight == 1)
		colorInfo.Back = pSettings->Appearance.iColBackgroundHilite;
	else
		colorInfo.Back = pSettings->Appearance.iColBackground;

	colorInfo.Shaded = BLEND(TextColor, colorInfo.Back, SHADE_LEVEL.UNUSED);
	colorInfo.Compact = BLEND(TextColor, colorInfo.Back, SHADE_LEVEL.PREVIEW);		// // //

	if (!pSettings->Appearance.bPatternColor) {		// // //
		colorInfo.Instrument = colorInfo.Volume = colorInfo.Effect = colorInfo.Note;
	}
	else {
		colorInfo.Instrument = pSettings->Appearance.iColPatternInstrument;
		colorInfo.Volume = pSettings->Appearance.iColPatternVolume;
		colorInfo.Effect = pSettings->Appearance.iColPatternEffect;
	}

	if (bPreview) {
		colorInfo.Shaded	 = BLEND(colorInfo.Shaded, colorInfo.Back, SHADE_LEVEL.PREVIEW);
		colorInfo.Note		 = BLEND(colorInfo.Note, colorInfo.Back, SHADE_LEVEL.PREVIEW);
		colorInfo.Instrument = BLEND(colorInfo.Instrument, colorInfo.Back, SHADE_LEVEL.PREVIEW);
		colorInfo.Volume	 = BLEND(colorInfo.Volume, colorInfo.Back, SHADE_LEVEL.PREVIEW);
		colorInfo.Effect	 = BLEND(colorInfo.Effect, colorInfo.Back, SHADE_LEVEL.PREVIEW);
		colorInfo.Compact	 = BLEND(colorInfo.Compact, colorInfo.Back, SHADE_LEVEL.PREVIEW);		// // //
	}

	// Draw channels
	for (int i = m_iFirstChannel; i < m_iFirstChannel + m_iChannelsVisible; ++i) {
		int f = Frame % GetFrameCount();
		if (f < 0) f += GetFrameCount();

		m_pDocument->GetNoteData(Track, f, i, Row, &NoteData);

		pDC->SetWindowOrg(-OffsetX, - (signed)Line * m_iRowHeight);

		int PosX	 = m_iColumnSpacing;
		int SelStart = m_iColumnSpacing;
		int Columns	 = GetChannelColumns(i);		// // //
		int Width	 = m_iChannelWidths[i] - 1;		// Remove 1, spacing between channels

		if (BackColor == ColBg)
			pDC->FillSolidRect(0, 0, Width, m_iRowHeight, BackColor);
		else
			GradientBar(pDC, 0, 0, Width, m_iRowHeight, BackColor, ColBg);

		if (!m_bFollowMode && Row == m_iPlayRow && f == m_iPlayFrame && theApp.IsPlaying()) {
			// Play row
			GradientBar(pDC, 0, 0, Width, m_iRowHeight, pSettings->Appearance.iColCurrentRowPlaying, ColBg);		// // //
		}

		// Draw each column
		const int BorderWidth = (m_iSelectionCondition == SEL_NONTERMINAL_SKIP) ? 2 : 1;		// // //
		for (int _j = 0; _j <= Columns; ++_j) {
			cursor_column_t j = static_cast<cursor_column_t>(_j);
			int SelWidth = GetSelectWidth(m_bCompactMode ? C_NOTE : j);		// // //

			// Selection
			if (m_bSelecting) {		// // //
				if (IsInRange(m_selection, Frame, Row, i, j)) {		// // //
					pDC->FillSolidRect(SelStart - m_iColumnSpacing, 0, SelWidth, m_iRowHeight, SelectColor);

					// Outline
					if (Row == m_selection.GetRowStart() && !((f - m_selection.GetFrameStart()) % GetFrameCount()))
						pDC->FillSolidRect(SelStart - m_iColumnSpacing, 0, SelWidth, BorderWidth, SelectEdgeCol);
					if (Row == m_selection.GetRowEnd() && !((f - m_selection.GetFrameEnd()) % GetFrameCount()))
						pDC->FillSolidRect(SelStart - m_iColumnSpacing, m_iRowHeight - BorderWidth, SelWidth, BorderWidth, SelectEdgeCol);
					if (i == m_selection.GetChanStart() && (j == m_selection.GetColStart() || m_bCompactMode))		// // //
						pDC->FillSolidRect(SelStart - m_iColumnSpacing, 0, BorderWidth, m_iRowHeight, SelectEdgeCol);
					if (i == m_selection.GetChanEnd() && (j == m_selection.GetColEnd() || m_bCompactMode))		// // //
						pDC->FillSolidRect(SelStart - m_iColumnSpacing + SelWidth - BorderWidth, 0, BorderWidth, m_iRowHeight, SelectEdgeCol);
				}
			}

			// Dragging
			if (m_bDragging) {		// // //
				if (IsInRange(m_selDrag, Frame, Row, i, j)) {		// // //
					pDC->FillSolidRect(SelStart - m_iColumnSpacing, 0, SelWidth, m_iRowHeight, DragColor);
				}
			}

			bool bInvert = false;

			// Draw cursor box
			if (i == m_cpCursorPos.m_iChannel && j == m_cpCursorPos.m_iColumn && Row == m_iDrawCursorRow && !bPreview) {
				GradientBar(pDC, PosX - m_iColumnSpacing / 2, 0, GetColumnWidth(j), m_iRowHeight, ColCursor, ColBg);		// // //
				pDC->Draw3dRect(PosX - m_iColumnSpacing / 2, 0, GetColumnWidth(j), m_iRowHeight, ColCursor, DIM(ColCursor, 50));
				//pDC->Draw3dRect(PosX - m_iColumnSpacing / 2 - 1, -1, GetColumnWidth(j) + 2, m_iRowHeight + 2, ColCursor, DIM(ColCursor, 50));
				bInvert = true;
			}

			DrawCell(pDC, PosX - m_iColumnSpacing / 2, j, i, bInvert, &NoteData, &colorInfo);		// // //
			PosX += GetColumnSpace(j);
			if (!m_bCompactMode)		// // //
				SelStart += GetSelectWidth(j);
		}

		OffsetX += m_iChannelWidths[i];
	}
}

static const int HEIGHT_OFFSET = 6;

void CPatternEditor::DrawCell(CDC *pDC, int PosX, cursor_column_t Column, int Channel, bool bInvert, stChanNote *pNoteData, RowColorInfo_t *pColorInfo) const
{
	// Sharps
	static const char NOTES_A_SHARP[] = {'C', 'C', 'D', 'D', 'E', 'F', 'F', 'G', 'G', 'A', 'A', 'B'};
	static const char NOTES_B_SHARP[] = {'-', '#', '-', '#', '-', '-', '#', '-', '#', '-', '#', '-'};
	// Flats
	static const char NOTES_A_FLAT[] = {'C', 'D', 'D', 'E', 'E', 'F', 'G', 'G', 'A', 'A', 'B', 'B'};
	static const char NOTES_B_FLAT[] = {'-', 'b', '-', 'b', '-', '-', 'b', '-', 'b', '-', 'b', '-'};
	// Octaves
	static const char NOTES_C[] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9'};
	// Hex numbers
	static const char HEX[] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};

	const bool m_bDisplayFlat = theApp.GetSettings()->Appearance.bDisplayFlats;		// // //

	const char *NOTES_A = m_bDisplayFlat ? NOTES_A_FLAT : NOTES_A_SHARP;
	const char *NOTES_B = m_bDisplayFlat ? NOTES_B_FLAT : NOTES_B_SHARP;

	const CTrackerChannel *pTrackerChannel = m_pDocument->GetChannel(Channel);

	effect_t EffNumber = Column >= 4 ? pNoteData->EffNumber[(Column - 4) / 3] : EF_NONE;		// // //
	int EffParam  = Column >= 4 ? pNoteData->EffParam[(Column - 4) / 3] : 0;

	// Detect invalid note data
	if (pNoteData->Note > ECHO ||		// // //
		pNoteData->Octave > 8 ||
		EffNumber >= EF_COUNT || 
		pNoteData->Instrument > MAX_INSTRUMENTS && pNoteData->Instrument != HOLD_INSTRUMENT) {		// // // 050B
		if (Column == C_NOTE/* || Column == 4*/) {
			CString Text;
			Text.Format(_T("(invalid)"));
			pDC->SetTextColor(RGB(255, 0, 0));
			pDC->TextOut(PosX, -1, Text);
		}
		return;
	}

	COLORREF InstColor = pColorInfo->Instrument;
	COLORREF EffColor = pColorInfo->Effect;
	COLORREF DimInst = pColorInfo->Compact;		// // //
	COLORREF DimEff = pColorInfo->Compact;		// // //

	// Make non-available instruments red in the pattern editor
	if (pNoteData->Instrument < MAX_INSTRUMENTS && 
		(!m_pDocument->IsInstrumentUsed(pNoteData->Instrument) ||
		!pTrackerChannel->IsInstrumentCompatible(pNoteData->Instrument, m_pDocument->GetInstrumentType(pNoteData->Instrument)))) { // // //
		DimInst = InstColor = RGB(255, 0, 0);
	}

	// // // effects too
	if (EffNumber != EF_NONE) if (!pTrackerChannel->IsEffectCompatible(EffNumber, EffParam))
		DimEff = EffColor = RGB(255, 0, 0);		// // //

	// Compute font vertical position
	// TODO resize font about center = avg(cap, base + descender/2)?

	int PosY = m_iRowHeight;
	PosY -= PosY / HEIGHT_OFFSET;

	// Compute vertical position for empty bars
	const int halfX = m_iCharWidth / 2;
	const int halfY = m_iRowHeight - m_iRowHeight / 2;

	const int BAR_WIDTH = m_iRowHeight * 1 / 3;
	const int BAR_HEIGHT = std::max(m_iRowHeight / 12, 1);
	auto BAR = [&](int x) {
		pDC->FillSolidRect(
				x + halfX - BAR_WIDTH / 2,
				halfY - BAR_HEIGHT / 2,
				BAR_WIDTH,
				BAR_HEIGHT,
				pColorInfo->Shaded);
	};

	pDC->SetTextAlign(TA_CENTER | TA_BASELINE);		// // //

	switch (Column) {
		case C_NOTE:
			// Note and octave
			switch (pNoteData->Note) {
				case NONE:
					if (m_bCompactMode) {		// // //
						if (pNoteData->Instrument != MAX_INSTRUMENTS) {
							if (pNoteData->Instrument == HOLD_INSTRUMENT) {		// // // 050B
								DrawChar(pDC, PosX + m_iCharWidth * 3 / 2, PosY, '&', DimInst);
								DrawChar(pDC, PosX + m_iCharWidth * 5 / 2, PosY, '&', DimInst);
							}
							else {
								DrawChar(pDC, PosX + m_iCharWidth * 3 / 2, PosY, HEX[pNoteData->Instrument >> 4], DimInst);
								DrawChar(pDC, PosX + m_iCharWidth * 5 / 2, PosY, HEX[pNoteData->Instrument & 0x0F], DimInst);
							}
							break;
						}
						else if (pNoteData->Vol != MAX_VOLUME) {
							DrawChar(pDC, PosX + m_iCharWidth * 5 / 2, PosY, HEX[pNoteData->Vol], pColorInfo->Compact);
							break;
						}
						else {
							bool Found = false;
							for (unsigned int i = 0; i <= m_pDocument->GetEffColumns(GetSelectedTrack(), Channel); i++) {
								if (pNoteData->EffNumber[i] != EF_NONE) {
									DrawChar(pDC, PosX + m_iCharWidth / 2, PosY, EFF_CHAR[pNoteData->EffNumber[i]], DimEff);
									DrawChar(pDC, PosX + m_iCharWidth * 3 / 2, PosY, HEX[pNoteData->EffParam[i] >> 4], DimEff);
									DrawChar(pDC, PosX + m_iCharWidth * 5 / 2, PosY, HEX[pNoteData->EffParam[i] & 0x0F], DimEff);
									Found = true;
									break;
								}
							}
							if (Found) break;
						}
						BAR(PosX);
						BAR(PosX + m_iCharWidth);
						BAR(PosX + m_iCharWidth * 2);
					}
					else {
						BAR(PosX);
						BAR(PosX + m_iCharWidth);
						BAR(PosX + m_iCharWidth * 2);
					}
					break;		// // // same below
				case HALT:
					// Note stop
					GradientBar(pDC, PosX + 5, (m_iRowHeight / 2) - 2, m_iCharWidth * 3 - 11, m_iRowHeight / 4, pColorInfo->Note, pColorInfo->Back);
					break;
				case RELEASE:
					// Note release
					pDC->FillSolidRect(PosX + 5, m_iRowHeight / 2 - 3, m_iCharWidth * 3 - 11, 2, pColorInfo->Note);		// // //
					pDC->FillSolidRect(PosX + 5, m_iRowHeight / 2 + 1, m_iCharWidth * 3 - 11, 2, pColorInfo->Note);
					break;
				case ECHO:
					// // // Echo buffer access
					DrawChar(pDC, PosX + m_iCharWidth, PosY, _T('^'), pColorInfo->Note);
					DrawChar(pDC, PosX + m_iCharWidth * 2, PosY, NOTES_C[pNoteData->Octave], pColorInfo->Note);
					break;
				default:
					if (pTrackerChannel->GetID() == CHANID_NOISE) {
						// Noise
						char NoiseFreq = (pNoteData->Note - 1 + pNoteData->Octave * 12) & 0x0F;
						DrawChar(pDC, PosX + m_iCharWidth / 2, PosY, HEX[NoiseFreq], pColorInfo->Note);		// // //
						DrawChar(pDC, PosX + m_iCharWidth * 3 / 2, PosY, '-', pColorInfo->Note);
						DrawChar(pDC, PosX + m_iCharWidth * 5 / 2, PosY, '#', pColorInfo->Note);
					}
					else {
						// The rest
						DrawChar(pDC, PosX + m_iCharWidth / 2, PosY, NOTES_A[pNoteData->Note - 1], pColorInfo->Note);		// // //
						DrawChar(pDC, PosX + m_iCharWidth * 3 / 2, PosY, NOTES_B[pNoteData->Note - 1], pColorInfo->Note);
						DrawChar(pDC, PosX + m_iCharWidth * 5 / 2, PosY, NOTES_C[pNoteData->Octave], pColorInfo->Note);
					}
					break;
			}
			break;
		case C_INSTRUMENT1:
			// Instrument x0
			if (pNoteData->Instrument == MAX_INSTRUMENTS || pNoteData->Note == HALT || pNoteData->Note == RELEASE)
				BAR(PosX);
			else if (pNoteData->Instrument == HOLD_INSTRUMENT)		// // // 050B
				DrawChar(pDC, PosX + m_iCharWidth / 2, PosY, '&', InstColor);
			else
				DrawChar(pDC, PosX + m_iCharWidth / 2, PosY, HEX[pNoteData->Instrument >> 4], InstColor);		// // //
			break;
		case C_INSTRUMENT2:
			// Instrument 0x
			if (pNoteData->Instrument == MAX_INSTRUMENTS || pNoteData->Note == HALT || pNoteData->Note == RELEASE)
				BAR(PosX);
			else if (pNoteData->Instrument == HOLD_INSTRUMENT)		// // // 050B
				DrawChar(pDC, PosX + m_iCharWidth / 2, PosY, '&', InstColor);
			else
				DrawChar(pDC, PosX + m_iCharWidth / 2, PosY, HEX[pNoteData->Instrument & 0x0F], InstColor);		// // //
			break;
		case C_VOLUME: 
			// Volume
			if (pNoteData->Vol == MAX_VOLUME || pTrackerChannel->GetID() == CHANID_DPCM)
				BAR(PosX);
			else 
				DrawChar(pDC, PosX + m_iCharWidth / 2, PosY, HEX[pNoteData->Vol & 0x0F], pColorInfo->Volume);		// // //
			break;
		case C_EFF1_NUM: case C_EFF2_NUM: case C_EFF3_NUM: case C_EFF4_NUM:
			// Effect type
			if (EffNumber == 0)
				BAR(PosX);
			else
				DrawChar(pDC, PosX + m_iCharWidth / 2, PosY, EFF_CHAR[EffNumber], EffColor);		// // //
			break;
		case C_EFF1_PARAM1: case C_EFF2_PARAM1: case C_EFF3_PARAM1: case C_EFF4_PARAM1:
			// Effect param x
			if (EffNumber == 0)
				BAR(PosX);
			else
				DrawChar(pDC, PosX + m_iCharWidth / 2, PosY, HEX[(EffParam >> 4) & 0x0F], pColorInfo->Note);		// // //
			break;
		case C_EFF1_PARAM2: case C_EFF2_PARAM2: case C_EFF3_PARAM2: case C_EFF4_PARAM2:
			// Effect param y
			if (EffNumber == 0)
				BAR(PosX);
			else
				DrawChar(pDC, PosX + m_iCharWidth / 2, PosY, HEX[EffParam & 0x0F], pColorInfo->Note);		// // //
			break;
	}

	pDC->SetTextAlign(TA_LEFT);		// // //
	return;
}

void CPatternEditor::DrawHeader(CDC *pDC)
{
	// Draw the pattern header (channel names, meters...)

	const COLORREF TEXT_COLOR = 0x404040;

	CPoint ArrowPoints[3];

	CBrush HoverBrush((COLORREF)0xFFFFFF);
	CBrush BlackBrush((COLORREF)0x505050);
	CPen HoverPen(PS_SOLID, 1, (COLORREF)0x80A080);
	CPen BlackPen(PS_SOLID, 1, (COLORREF)0x808080);

	unsigned int Offset = m_iRowColumnWidth;
	unsigned int Track = GetSelectedTrack();

	CFont *pOldFont = pDC->SelectObject(&m_fontHeader);

	pDC->SetBkMode(TRANSPARENT);

	// Channel header background
	GradientRectTriple(pDC, 0, HEADER_CHAN_START, m_iWinWidth, HEADER_CHAN_HEIGHT,
					   m_colHead1, m_colHead2, m_pView->GetEditMode() ? m_colHead4 : m_colHead3);		// // //

	// Corner box
	pDC->Draw3dRect(0, HEADER_CHAN_START, m_iRowColumnWidth, HEADER_CHAN_HEIGHT, STATIC_COLOR_SCHEME.FRAME_LIGHT, STATIC_COLOR_SCHEME.FRAME_DARK);

	for (int i = 0; i < m_iChannelsVisible; ++i) {

		const int Channel = i + m_iFirstChannel;
		const bool bMuted = m_pView->IsChannelMuted(Channel);
		const bool Pushed = bMuted || (m_iChannelPushed == Channel) && m_bChannelPushed;

		// Frame
		if (Pushed) {
			GradientRectTriple(pDC, Offset, HEADER_CHAN_START, m_iChannelWidths[Channel], HEADER_CHAN_HEIGHT,
							   m_colHead1, m_colHead1, m_pView->GetEditMode() ? m_colHead4 : m_colHead3);
			pDC->Draw3dRect(Offset, HEADER_CHAN_START, m_iChannelWidths[Channel], HEADER_CHAN_HEIGHT, BLEND(STATIC_COLOR_SCHEME.FRAME_LIGHT, STATIC_COLOR_SCHEME.FRAME_DARK, 50), STATIC_COLOR_SCHEME.FRAME_DARK); 
		}
		else {
			if (m_pDocument->GetChannelType(Channel) == theApp.GetSoundGenerator()->GetRecordChannel())		// // //
				GradientRectTriple(pDC, Offset, HEADER_CHAN_START, m_iChannelWidths[Channel], HEADER_CHAN_HEIGHT,
								   m_colHead1, m_colHead2, m_colHead5);
			pDC->Draw3dRect(Offset, HEADER_CHAN_START, m_iChannelWidths[Channel], HEADER_CHAN_HEIGHT, STATIC_COLOR_SCHEME.FRAME_LIGHT, STATIC_COLOR_SCHEME.FRAME_DARK);
		}

		// Text
		CTrackerChannel *pChannel = m_pDocument->GetChannel(Channel);
		CString pChanName = (m_bCompactMode && m_iCharWidth < 6) ? _T("") :
			(m_bCompactMode || m_iCharWidth < 9) ? pChannel->GetShortName() : pChannel->GetChannelName();		// // //

		COLORREF HeadTextCol = bMuted ? STATIC_COLOR_SCHEME.CHANNEL_MUTED : STATIC_COLOR_SCHEME.CHANNEL_NORMAL;

		// Shadow
		if (m_bCompactMode)		// // //
			pDC->SetTextAlign(TA_CENTER);

		pDC->SetTextColor(BLEND(HeadTextCol, 0x00FFFFFF, SHADE_LEVEL.TEXT_SHADOW));
		pDC->TextOut(Offset + (m_bCompactMode ? GetColumnSpace(C_NOTE) / 2 + 1 : 10) + 1, HEADER_CHAN_START + 6 + (bMuted ? 1 : 0), pChanName);
		
		// Foreground
		if (m_iMouseHoverChan == Channel)
			HeadTextCol = BLEND(HeadTextCol, 0x0000FFFF, SHADE_LEVEL.HOVER);
		pDC->SetTextColor(HeadTextCol);
		pDC->TextOut(Offset + (m_bCompactMode ? GetColumnSpace(C_NOTE) / 2 + 1 : 10), HEADER_CHAN_START + 5, pChanName);		// // //
		
		if (!m_bCompactMode) {		// // //
			// Effect columns
			pDC->SetTextColor(TEXT_COLOR);
			pDC->SetTextAlign(TA_CENTER);
			for (unsigned int i = 1; i <= m_pDocument->GetEffColumns(Track, Channel); i++) {		// // //
				CString str;
				str.Format(_T("fx%d"), i + 1);
				pDC->TextOut(Offset + GetChannelWidth(i) - m_iCharWidth * 3 / 2, HEADER_CHAN_START + HEADER_CHAN_HEIGHT - 17, str);
			}

			// Arrows for expanding/removing fx columns
			if (m_pDocument->GetEffColumns(Track, Channel) > 0) {
				ArrowPoints[0].SetPoint(Offset + m_iCharWidth * 15 / 2 + m_iColumnSpacing * 3 + 2, HEADER_CHAN_START + 6);		// // //
				ArrowPoints[1].SetPoint(Offset + m_iCharWidth * 15 / 2 + m_iColumnSpacing * 3 + 2, HEADER_CHAN_START + 6 + 10);
				ArrowPoints[2].SetPoint(Offset + m_iCharWidth * 15 / 2 + m_iColumnSpacing * 3 - 3, HEADER_CHAN_START + 6 + 5);

				bool Hover = (m_iMouseHoverChan == Channel) && (m_iMouseHoverEffArrow == 1);
				CObject *pOldBrush = pDC->SelectObject(Hover ? &HoverBrush : &BlackBrush);
				CObject *pOldPen = pDC->SelectObject(Hover ? &HoverPen : &BlackPen);

				pDC->Polygon(ArrowPoints, 3);
				pDC->SelectObject(pOldBrush);
				pDC->SelectObject(pOldPen);
			}

			if (m_pDocument->GetEffColumns(Track, Channel) < (MAX_EFFECT_COLUMNS - 1)) {
				ArrowPoints[0].SetPoint(Offset + m_iCharWidth * 17 / 2 + m_iColumnSpacing * 3 - 2, HEADER_CHAN_START + 6);		// // //
				ArrowPoints[1].SetPoint(Offset + m_iCharWidth * 17 / 2 + m_iColumnSpacing * 3 - 2, HEADER_CHAN_START + 6 + 10);
				ArrowPoints[2].SetPoint(Offset + m_iCharWidth * 17 / 2 + m_iColumnSpacing * 3 + 3, HEADER_CHAN_START + 6 + 5);

				bool Hover = (m_iMouseHoverChan == Channel) && (m_iMouseHoverEffArrow == 2);
				CObject *pOldBrush = pDC->SelectObject(Hover ? &HoverBrush : &BlackBrush);
				CObject *pOldPen = pDC->SelectObject(Hover ? &HoverPen : &BlackPen);

				pDC->Polygon(ArrowPoints, 3);
				pDC->SelectObject(pOldBrush);
				pDC->SelectObject(pOldPen);
			}
		}

		Offset += m_iChannelWidths[Channel];
		pDC->SetTextAlign(TA_LEFT);		// // //
	}
	
	pDC->SelectObject(pOldFont);
}

void CPatternEditor::DrawMeters(CDC *pDC)
{
	const COLORREF COL_DARK			= 0x989C98;
	const COLORREF COL_LIGHT		= 0x20F040;
	const COLORREF COL_DARK_SHADOW  = DIM(COL_DARK, 80);
	const COLORREF DPCM_STATE_COLOR = 0x00404040;

	const int BAR_TOP	 = DPI::SY(5 + 18 + HEADER_CHAN_START);
	const int BAR_SIZE	 = m_bCompactMode ? (GetColumnSpace(C_NOTE) - 2) / 16 : (GetChannelWidth(0) - 6) / 16;		// // //
	const int BAR_LEFT	 = m_bCompactMode ? m_iRowColumnWidth + (GetColumnSpace(C_NOTE) - 16 * BAR_SIZE + 3) / 2 : m_iRowColumnWidth + 7;
	const int BAR_SPACE	 = 1;
	const int BAR_HEIGHT = DPI::SY(5);

	static COLORREF colors[15];
	static COLORREF colors_dim[15];
	static COLORREF colors_shadow[15];

	// // //

	if (!m_pDocument)
		return;

	int Offset = BAR_LEFT;

	CFont *pOldFont = pDC->SelectObject(&m_fontHeader);

	if (colors[0] == 0) {
		for (int i = 0; i < 15; ++i) {
			// Cache colors
			colors[i] = BLEND(COL_LIGHT, 0x00F0F0, (100 - (i * i) / 3));
			colors_shadow[i] = DIM(colors[i], 60);
			colors_dim[i] = DIM(colors[i], 90);
		}
	}

	for (int i = 0; i < m_iChannelsVisible; ++i) {
		int Channel = i + m_iFirstChannel;
		CTrackerChannel *pChannel = m_pDocument->GetChannel(Channel);
		int level = pChannel->GetVolumeMeter();

		for (int j = 0; j < 15; ++j) {
			int x = Offset + (j * BAR_SIZE);
			COLORREF shadowCol = j < level ? colors_shadow[j] : COL_DARK_SHADOW;		// // //
			if (BAR_SIZE > 2) {
				pDC->FillSolidRect(x + BAR_SIZE - 1, BAR_TOP + 1, BAR_SPACE, BAR_HEIGHT, shadowCol);
				pDC->FillSolidRect(x + 1, BAR_TOP + BAR_HEIGHT, BAR_SIZE - 1, 1, shadowCol);
				pDC->FillSolidRect(x, BAR_TOP, BAR_SIZE - BAR_SPACE, BAR_HEIGHT, j < level ? colors[j] : COL_DARK);
			}
			else {
				pDC->FillSolidRect(x, BAR_TOP, BAR_SIZE, BAR_HEIGHT + 1, shadowCol);
				pDC->FillSolidRect(x, BAR_TOP, BAR_SIZE, BAR_HEIGHT, j < level ? colors[j] : COL_DARK);
			}
			if (j < level && BAR_SIZE > 2)
				pDC->Draw3dRect(x, BAR_TOP, BAR_SIZE - BAR_SPACE, BAR_HEIGHT, colors[j], colors_dim[j]);
		}

		Offset += m_iChannelWidths[Channel];
	}

	// // //
#ifdef DRAW_REGS
	DrawRegisters(pDC);
#else
	DrawRegisters(m_pRegisterDC);
	// // //
	const int iBlitHeight = m_iWinHeight - HEADER_HEIGHT;
	const int iBlitWidth = m_iPatternWidth + m_iRowColumnWidth;
	pDC->BitBlt(iBlitWidth, HEADER_HEIGHT, m_iWinWidth - iBlitWidth, iBlitHeight, m_pRegisterDC, 0, 0, SRCCOPY);
#endif /* DRAW_REGS */

	pDC->SelectObject(pOldFont);
}

static double NoteFromFreq(double Freq)
{
	// Convert frequency to note number
	return 45.0 + 12.0 * (std::log(Freq / 440.0) / log(2.0));
}

static CString NoteToStr(int Note)
{
	int Octave = GET_OCTAVE(Note) + 1;		// // //
	int Index = GET_NOTE(Note) - 1;

	CString str;
	if (theApp.GetSettings()->Appearance.bDisplayFlats)
		str = stChanNote::NOTE_NAME_FLAT[Index];
	else
		str = stChanNote::NOTE_NAME[Index];
	str.AppendFormat("%i", Octave);
	return str;
}

static const COLORREF DECAY_COLOR[CRegisterState::DECAY_RATE + 1] = {		// // //
	0xFFFF80, 0xE6F993, 0xCCF3A6, 0xB3ECB9, 0x99E6CC, 0x80E0E0, 0x80D9E3, 0x80D3E6,
	0x80CCE9, 0x80C6EC, 0x80C0F0, 0x80B9F3, 0x80B3F6, 0x80ACF9, 0x80A6FC, 0x80A0FF,
}; // BLEND has lower precision

void CPatternEditor::DrawRegisters(CDC *pDC)
{
	if (!m_pDocument || !pDC) return;		// // //

	unsigned char reg[8] = {};
	unsigned char update[8] = {};
	CFont *pOldFont = pDC->SelectObject(&m_fontCourierNew);
	pDC->FillSolidRect(0, 0, m_iWinWidth, m_iWinHeight, m_colEmptyBg);		// // //

	if (!theApp.GetSettings()->Display.bRegisterState) {		// // //
		pDC->SelectObject(pOldFont);
		return;
	}

	pDC->SetBkMode(TRANSPARENT);		// // //

	const CSoundGen *pSoundGen = theApp.GetSoundGenerator();

	const int LINE_HEIGHT = DPI::SY(13);
	int x = DPI::SX(30);		// // //
	int y = DPI::SY(30) - LINE_HEIGHT * 2;
	int line = -1;		// // //
	CString text;

	const int BAR_OFFSET = LINE_HEIGHT * (3 + 8 +
		m_pDocument->ExpansionEnabled(SNDCHIP_VRC6) * 5 +
		m_pDocument->ExpansionEnabled(SNDCHIP_MMC5) * 4 +
		m_pDocument->ExpansionEnabled(SNDCHIP_N163) * 18 +
		m_pDocument->ExpansionEnabled(SNDCHIP_FDS) * 13 +
		m_pDocument->ExpansionEnabled(SNDCHIP_VRC7) * 9 +
		m_pDocument->ExpansionEnabled(SNDCHIP_S5B) * 8);		// // //
	int vis_line = 0;

	const auto DrawHeaderFunc = [&] (CString Text) {
		line += 2; y += LINE_HEIGHT * 2;
		pDC->MoveTo(x, y);
		pDC->SetBkColor(m_colEmptyBg);
		pDC->SetTextColor(0xFFAFAF);
		pDC->TextOut(x, y, Text + _T(" registers"));
	};

	const auto DrawRegFunc = [&] (CString Header, int Count) {
		++line; y += LINE_HEIGHT;
		pDC->SetBkColor(m_colEmptyBg);
		pDC->SetTextColor(0xFFAFAF);
		pDC->SetTextAlign(TA_UPDATECP);
		pDC->MoveTo(x, y);
		pDC->TextOut(0, 0, Header);
		for (int j = 0; j < Count; j++) {
			CString str;
			str.Format(_T(" $%02X"), reg[j]);
			pDC->SetTextColor(BLEND(0xC0C0C0, DECAY_COLOR[update[j] >> 4], 100 * (update[j] & 0x0F) / CRegisterState::DECAY_RATE));
			pDC->TextOut(0, 0, str);
		}
	};

	const auto DrawVolFunc = [&] (double Freq, int Volume) {
		pDC->FillSolidRect(x - 1, BAR_OFFSET + vis_line * 10 - 1, 6 * 108 + 3, 9, 0x808080);
		pDC->FillSolidRect(x, BAR_OFFSET + vis_line * 10, 6 * 108 + 1, 7, 0);
		for (int i = 0; i < 10; i++)
			pDC->SetPixelV(x + 72 * i, BAR_OFFSET + vis_line * 10 + 3, i == 4 ? 0x808080 : 0x303030);

		const double note = NoteFromFreq(Freq);
		const int note_conv = note >= 0 ? int(note + 0.5) : int(note - 0.5);
		if (Volume > 0xFF) Volume = 0xFF;
		if (note_conv >= -12 && note_conv <= 96 && Volume)		// // //
			pDC->FillSolidRect(29 + 6 * (note_conv + 12), BAR_OFFSET + vis_line * 10, 3, 7, RGB(Volume, Volume, Volume));
		++vis_line;
	};

	const auto DrawTextFunc = [&] (int xOffsNoDPI, CString text) {
		pDC->SetTextColor(0x808080);
		pDC->SetTextAlign(TA_NOUPDATECP);
		pDC->TextOut(x + DPI::SX(xOffsNoDPI), y, text);
	};

	const auto GetRegsFunc = [&] (unsigned Chip, std::function<int(int)> F, int Count) {
		for (int j = 0; j < Count; j++) {
			auto pState = pSoundGen->GetRegState(Chip, F(j));		// // //
			reg[j] = pState->GetValue();
			update[j] = pState->GetLastUpdatedTime() | (pState->GetNewValueTime() << 4);
		}
	};

	const auto GetPitchTextFunc = [] (int digits, int period, double freq) {
		const CString fmt = _T("pitch = $%0*X (%7.2fHz %s %+03i)");
		const double note = NoteFromFreq(freq);
		const int note_conv = note >= 0 ? int(note + 0.5) : int(note - 0.5);
		const int cents = int((note - double(note_conv)) * 100.0);
		
		CString str;
		if (freq != 0.)
			str.Format(fmt, digits, period, freq, NoteToStr(note_conv), cents);
		else
			str.Format(fmt, digits, period, 0., _T("---"), 0);
		return str;
	};

	// 2A03
	DrawHeaderFunc(_T("2A03"));		// // //

	for (int i = 0; i < 5; ++i) {
		GetRegsFunc(SNDCHIP_NONE, [&] (int x) { return 0x4000 + i * 4 + x; }, 4);
		text.Format(_T("$%04X:"), 0x4000 + i * 4);		// // //
		DrawRegFunc(text, 4);

		int period, vol;
		double freq = theApp.GetSoundGenerator()->GetChannelFrequency(SNDCHIP_NONE, i);		// // //
//		pDC->FillSolidRect(x + 200, y, x + 400, y + 18, m_colEmptyBg);

		switch (i) {
		case 0: case 1:
			period = reg[2] | ((reg[3] & 7) << 8);
			vol = reg[0] & 0x0F;
			text.Format(_T("%s, vol = %02i, duty = %i"), GetPitchTextFunc(3, period, freq), vol, reg[0] >> 6); break;
		case 2:
			period = reg[2] | ((reg[3] & 7) << 8);
			vol = reg[0] ? 15 : 0;
			text.Format(_T("%s"), GetPitchTextFunc(3, period, freq)); break;
		case 3:
			period = reg[2] & 0x0F;
			vol = reg[0] & 0x0F;
			text.Format(_T("pitch = $%01X, vol = %02i, mode = %i"), period, vol, reg[2] >> 7);
			period = (period << 4) | ((reg[2] & 0x80) >> 4);
			freq /= 16; break; // for display
		case 4:
			period = reg[0] & 0x0F;
			vol = 15 * !pSoundGen->PreviewDone();
			text.Format(_T("%s, %s, size = %i byte%c"), GetPitchTextFunc(1, period & 0x0F, freq),
				(reg[0] & 0x40) ? _T("looped") : _T("once"), (reg[3] << 4) | 1, reg[3] ? 's' : ' ');
			freq /= 16; break; // for display
		}
/*
		pDC->FillSolidRect(250 + i * 30, 0, 20, m_iWinHeight - HEADER_CHAN_HEIGHT, 0);
		pDC->FillSolidRect(250 + i * 30, (period >> 1), 20, 5, RGB(vol << 4, vol << 4, vol << 4));
*/
		DrawTextFunc(180, text);
		DrawVolFunc(freq, vol << 4);
	}

	text.Format(_T("position: %02i, delta = $%02X"), m_DPCMState.SamplePos, m_DPCMState.DeltaCntr);		// // //
	++line; y += LINE_HEIGHT;		// // //
	DrawTextFunc(180, text);

	if (m_pDocument->ExpansionEnabled(SNDCHIP_VRC6)) {
		DrawHeaderFunc(_T("VRC6"));		// // //

		// VRC6
		for (int i = 0; i < 3; ++i) {
			GetRegsFunc(SNDCHIP_VRC6, [&] (int x) { return 0x9000 + i * 0x1000 + x; }, 3);
			text.Format(_T("$%04X:"), 0x9000 + i * 0x1000);		// // //
			DrawRegFunc(text, 3);

			int period = (reg[1] | ((reg[2] & 15) << 8));
			int vol = (reg[0] & (i == 2 ? 0x3F : 0x0F));
			double freq = theApp.GetSoundGenerator()->GetChannelFrequency(SNDCHIP_VRC6, i);		// // //

			text.Format(_T("%s, vol = %02i"), GetPitchTextFunc(3, period, freq), vol);
			if (i != 2)
				text.AppendFormat(_T(", duty = %i"), (reg[0] >> 4) & 0x07);
			DrawTextFunc(180, text);
			DrawVolFunc(freq, vol << (i == 2 ? 3 : 4));
		}
	}

	if (m_pDocument->ExpansionEnabled(SNDCHIP_MMC5)) {		// // //
		DrawHeaderFunc(_T("MMC5"));		// // //

		// MMC5
		for (int i = 0; i < 2; ++i) {
			GetRegsFunc(SNDCHIP_MMC5, [&] (int x) { return 0x5000 + i * 4 + x; }, 4);
			text.Format(_T("$%04X:"), 0x5000 + i * 4);
			DrawRegFunc(text, 4);
			
			int period = (reg[2] | ((reg[3] & 7) << 8));
			int vol = (reg[0] & 0x0F);
			double freq = theApp.GetSoundGenerator()->GetChannelFrequency(SNDCHIP_MMC5, i);		// // //

			text.Format(_T("%s, vol = %02i, duty = %i"), GetPitchTextFunc(3, period, freq), vol, reg[0] >> 6);
			DrawTextFunc(180, text);
			DrawVolFunc(freq, vol << 4);
		}
	}

	if (m_pDocument->ExpansionEnabled(SNDCHIP_N163)) {
		DrawHeaderFunc(_T("N163"));		// // //

		const int wave_x = x + DPI::SX(300);

		// // // N163 wave
		const int N163_CHANS = m_pDocument->GetNamcoChannels();
		const int Length = 0x80 - 8 * N163_CHANS;

		y += 18;
		pDC->FillSolidRect(wave_x - 1, y - 1, 2 * Length + 2, 17, 0x808080);
		pDC->FillSolidRect(wave_x, y, 2 * Length, 15, 0);
		for (int i = 0; i < Length; i++) {
			auto pState = pSoundGen->GetRegState(SNDCHIP_N163, i);
			const int Hi = (pState->GetValue() >> 4) & 0x0F;
			const int Lo = pState->GetValue() & 0x0F;
			COLORREF Col = BLEND(
				0xC0C0C0, DECAY_COLOR[pState->GetNewValueTime()], 100 * pState->GetLastUpdatedTime() / CRegisterState::DECAY_RATE
			);
			pDC->FillSolidRect(wave_x + i * 2    , y + 15 - Lo, 1, Lo, Col);
			pDC->FillSolidRect(wave_x + i * 2 + 1, y + 15 - Hi, 1, Hi, Col);
		}
		for (int i = 0; i < N163_CHANS; ++i) {
			auto pPosState = pSoundGen->GetRegState(SNDCHIP_N163, 0x78 - i * 8 + 6);
			auto pLenState = pSoundGen->GetRegState(SNDCHIP_N163, 0x78 - i * 8 + 4);
			const int WavePos = pPosState->GetValue();
			const int WaveLen = 0x100 - (pLenState->GetValue() & 0xFC);
			const int NewTime = std::min(pPosState->GetNewValueTime(), pLenState->GetNewValueTime());
			const int UpdateTime = std::min(pPosState->GetLastUpdatedTime(), pLenState->GetLastUpdatedTime());
			pDC->FillSolidRect(wave_x, y + 20 + i * 5, Length * 2, 3, 0);
			pDC->FillSolidRect(wave_x + WavePos, y + 20 + i * 5, WaveLen, 3,
							   BLEND(0xC0C0C0, DECAY_COLOR[NewTime], 100 * UpdateTime / CRegisterState::DECAY_RATE));
		}
		y -= 18;

		double FreqCache[8] = { };
		int VolCache[8] = { };

		// N163
		for (int i = 0; i < 16; ++i) {
			GetRegsFunc(SNDCHIP_N163, [&] (int x) { return i * 8 + x; }, 8);
			text.Format(_T("$%02X:"), i * 8);
			DrawRegFunc(text, 8);

			int period = (reg[0] | (reg[2] << 8) | ((reg[4] & 0x03) << 16));
			int vol = (reg[7] & 0x0F);
			double freq = theApp.GetSoundGenerator()->GetChannelFrequency(SNDCHIP_N163, 15 - i);		// // //
			
			if (i >= 16 - N163_CHANS) {
				text.Format(_T("%s, vol = %02i"), GetPitchTextFunc(5, period, freq), vol);
				DrawTextFunc(300, text);
				FreqCache[15 - i] = freq;
				VolCache[15 - i] = vol << 4;
			}
		}
		
		for (int i = 0; i < N163_CHANS; ++i)		// // //
			DrawVolFunc(FreqCache[i], VolCache[i]);
	}

	if (m_pDocument->ExpansionEnabled(SNDCHIP_FDS)) {
		DrawHeaderFunc(_T("FDS"));		// // //

		// // // FDS wave
		const int wave_x = x + DPI::SX(180);
		const double xScale = 1, yScale = 0.5;

		y += 18 + LINE_HEIGHT * 2;
		pDC->FillSolidRect(wave_x - 1, y - 1, (int)(64*xScale) + 2, (int)(64*yScale)+2, 0x808080); // draw box
		pDC->FillSolidRect(wave_x, y, (int)(64*xScale), (int)(64*yScale)-1, 0);                    // fill box
		for (double i = 0; i < 64; i+=(1/xScale)) {
			auto pState = pSoundGen->GetRegState(SNDCHIP_FDS, 0x4040 + ((int)(i) & 0x3F));
			int state = pState->GetValue();
			COLORREF Col = BLEND(0xC0C0C0, DECAY_COLOR[pState->GetNewValueTime()], 100 * pState->GetLastUpdatedTime() / CRegisterState::DECAY_RATE);
			pDC->FillSolidRect(wave_x + (int)(i * xScale), y + (int)((0x3F - state) * yScale), 1, (int)(state* yScale) + 1, Col);
			pDC->FillSolidRect(wave_x + (int)(i * xScale), y + (int)((0x3F-state)*yScale), 1, 1, DIM(Col,(int)(100*(state*yScale-(int)(state*yScale))))); // antialiasing
		}
		y -= 18 + LINE_HEIGHT * 2;

		// other
		int period = (pSoundGen->GetReg(SNDCHIP_FDS, 0x4082) & 0xFF) | ((pSoundGen->GetReg(SNDCHIP_FDS, 0x4083) & 0x0F) << 8);
		int vol = (pSoundGen->GetReg(SNDCHIP_FDS, 0x4080) & 0x3F);
		double freq = theApp.GetSoundGenerator()->GetChannelFrequency(SNDCHIP_FDS, 0);		// // //

		int modperiod = (pSoundGen->GetReg(SNDCHIP_FDS, 0x4086) & 0xFF) | ((pSoundGen->GetReg(SNDCHIP_FDS, 0x4087) & 0x0F) << 8);
		int moddepth = (pSoundGen->GetReg(SNDCHIP_FDS, 0x4084) & 0x3F);
		double modfreq = theApp.GetSoundGenerator()->GetFDSModFrequency();

		CString FDStext;
		CString Modtext;

		FDStext.Format(_T("%s, vol = %02i"), GetPitchTextFunc(3, period, freq), vol);
		Modtext.Format(_T("modulation %s, depth = %02i"), GetPitchTextFunc(3, modperiod, modfreq), moddepth);

		for (int i = 0; i < 3; ++i) {
			GetRegsFunc(SNDCHIP_FDS, [&](int x) { return 0x4080 + i * 4 + x; }, 4);
			text.Format(_T("$%04X:"), 0x4080 + i * 4);
			DrawRegFunc(text, 4);
			switch (i) {
			case 0: DrawTextFunc(180, FDStext); break;
			case 1: DrawTextFunc(180, Modtext); break;
			}
		}
		
		DrawVolFunc(freq, vol << 3);
		line += 2; y += LINE_HEIGHT * 2;
	}

	if (m_pDocument->ExpansionEnabled(SNDCHIP_VRC7)) {		// // //
		DrawHeaderFunc(_T("VRC7"));		// // //
		
		GetRegsFunc(SNDCHIP_VRC7, [] (int x) { return x; }, 8);
		DrawRegFunc(_T("$00:"), 8);		// // //

		for (int i = 0; i < 6; ++i) {
			GetRegsFunc(SNDCHIP_VRC7, [&] (int x) { return i + (++x << 4); }, 3);
			text.Format(_T("$x%01X:"), i);
			DrawRegFunc(text, 3);

			int period = reg[0] | ((reg[1] & 0x01) << 8);
			int vol = 0x0F - (pSoundGen->GetReg(SNDCHIP_VRC7, i + 0x30) & 0x0F);
			double freq = theApp.GetSoundGenerator()->GetChannelFrequency(SNDCHIP_VRC7, i);		// // //

			text.Format(_T("%s, vol = %02i, patch = $%01X"), GetPitchTextFunc(3, period, freq), vol, reg[2] >> 4);
			DrawTextFunc(180, text);
			
			DrawVolFunc(freq, vol << 4);
		}
	}

	if (m_pDocument->ExpansionEnabled(SNDCHIP_S5B)) {		// // //
		DrawHeaderFunc(_T("5B"));		// // //

		// S5B
		for (int i = 0; i < 4; ++i) {
			GetRegsFunc(SNDCHIP_S5B, [&] (int x) { return i * 2 + x; }, 2);
			text.Format(_T("$%02X:"), i * 2);
			DrawRegFunc(text, 2);

			int period = reg[0] | ((reg[1] & 0x0F) << 8);
			int vol = pSoundGen->GetReg(SNDCHIP_S5B, 8 + i) & 0x0F;
			double freq = theApp.GetSoundGenerator()->GetChannelFrequency(SNDCHIP_S5B, i);		// // //

			if (i < 3)
				text.Format(_T("%s, vol = %02i, mode = %c%c%c"), GetPitchTextFunc(3, period, freq), vol,
					(pSoundGen->GetReg(SNDCHIP_S5B, 7) & (1 << i)) ? _T('-') : _T('T'),
					(pSoundGen->GetReg(SNDCHIP_S5B, 7) & (8 << i)) ? _T('-') : _T('N'),
					(pSoundGen->GetReg(SNDCHIP_S5B, 8 + i) & 0x10) ? _T('E') : _T('-'));
			else
				text.Format(_T("pitch = $%02X"), reg[0] & 0x1F);
			DrawTextFunc(180, text);

			if (i < 3)
				DrawVolFunc(freq, vol << 4);
		}

		for (int i = 0; i < 2; ++i) {
			GetRegsFunc(SNDCHIP_S5B, [&] (int x) { return i * 3 + x + 8; }, 3);
			text.Format(_T("$%02X:"), i * 3 + 8);
			DrawRegFunc(text, 3);
			
			if (i == 1) {
				int period = (reg[0] | (reg[1] << 8));
				double freq = theApp.GetSoundGenerator()->GetChannelFrequency(SNDCHIP_S5B, 3);		// // //
				if (freq != 0. && reg[1] == 0)
					text.Format(_T("%s, shape = $%01X"), GetPitchTextFunc(4, period, freq), reg[2]);
				else
					text.Format(_T("period = $%04X, shape = $%01X"), period, reg[2]);
				
				DrawTextFunc(180, text);
			}
		}
	}

	pDC->SelectObject(pOldFont);

	// Surrounding frame
//	pDC->Draw3dRect(20, 20, 200, line * 18 + 20, 0xA0A0A0, 0x505050);		// // //

}

// Draws a colored character
void CPatternEditor::DrawChar(CDC *pDC, int x, int y, TCHAR c, COLORREF Color) const
{
	pDC->SetTextColor(Color);
	pDC->TextOut(x, y, &c, 1);
	++m_iCharsDrawn;
}

void CPatternEditor::SetDPCMState(const stDPCMState &State)
{
	m_DPCMState = State;
}

////////////////////////////////////////////////////////////////////////////////////
// Private methods /////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////

unsigned int CPatternEditor::GetColumnWidth(cursor_column_t Column) const		// // //
{
	return m_iCharWidth * (Column == C_NOTE ? 3 : 1);
}

unsigned int CPatternEditor::GetColumnSpace(cursor_column_t Column) const		// // //
{
	int x = GetColumnWidth(Column);
	for (int i = 0; i < 7; i++)
		if (Column == GetCursorEndColumn(static_cast<column_t>(i))) return x + m_iColumnSpacing;
	return x;
}

unsigned int CPatternEditor::GetSelectWidth(cursor_column_t Column) const		// // //
{
	int x = GetColumnWidth(Column);
	for (int i = 0; i < 7; i++)
		if (Column == GetCursorStartColumn(static_cast<column_t>(i))) return x + m_iColumnSpacing;
	return x;
}

unsigned int CPatternEditor::GetChannelWidth(int EffColumns) const		// // //
{
	return m_iCharWidth * (9 + EffColumns * 3) + m_iColumnSpacing * (4 + EffColumns) - 1;
}

void CPatternEditor::UpdateVerticalScroll()
{
	// Vertical scroll bar
	SCROLLINFO si;

	si.cbSize = sizeof(SCROLLINFO);
	si.fMask = SIF_PAGE | SIF_POS | SIF_RANGE;
	si.nMin = 0;
	si.nMax = m_iPatternLength + theApp.GetSettings()->General.iPageStepSize - 2;
	si.nPos = m_iDrawCursorRow;
	si.nPage = theApp.GetSettings()->General.iPageStepSize;

	m_pView->SetScrollInfo(SB_VERT, &si);
}

void CPatternEditor::UpdateHorizontalScroll()
{
	// Horizontal scroll bar
	SCROLLINFO si;

	const int Channels = GetChannelCount();
	int ColumnCount = 0, CurrentColumn = 0;

	// Calculate cursor pos
	for (int i = 0; i < Channels; ++i) {
		if (i == m_cpCursorPos.m_iChannel)
			CurrentColumn = ColumnCount + m_cpCursorPos.m_iColumn;
		ColumnCount += GetChannelColumns(i) + 1;
	}

	si.cbSize = sizeof(SCROLLINFO);
	si.fMask = SIF_PAGE | SIF_POS | SIF_RANGE;
	si.nMin = 0;
	si.nMax = ColumnCount + COLUMNS - 2;
	si.nPos = CurrentColumn;
	si.nPage = COLUMNS;

	m_pView->SetScrollInfo(SB_HORZ, &si);
}

// Point to/from cursor translations

// // //

int CPatternEditor::GetChannelAtPoint(int PointX) const
{
	// Convert X position to channel number
	const int ChannelCount = GetChannelCount();

	if (PointX < m_iRowColumnWidth)
		return -1;	// -1 means row number column

	const int Offset = PointX - m_iRowColumnWidth + m_iChannelOffsets[m_iFirstChannel];
	for (int i = m_iFirstChannel; i < ChannelCount; ++i) {
		if (Offset >= m_iChannelOffsets[i] && Offset < (m_iChannelOffsets[i] + m_iChannelWidths[i]))
			return i;
	}

	return m_iFirstChannel + m_iChannelsVisible;
}

cursor_column_t CPatternEditor::GetColumnAtPoint(int PointX) const		// // //
{
	// Convert X position to column number
	const int ChannelCount = GetChannelCount();
	const int Channel = GetChannelAtPoint(PointX);

	if (Channel < 0 || ChannelCount <= 0)		// // //
		return C_NOTE;
	if (Channel >= ChannelCount)
		return GetChannelColumns(ChannelCount - 1);

	const int Offset = PointX - m_iRowColumnWidth + m_iChannelOffsets[m_iFirstChannel];
	int ColumnOffset = m_iChannelOffsets[Channel];
	for (unsigned i = 0; i <= GetChannelColumns(Channel); ++i) {
		ColumnOffset += GetColumnSpace(static_cast<cursor_column_t>(i));		// // //
		if (Offset <= ColumnOffset)
			return static_cast<cursor_column_t>(i);
	}

	return GetChannelColumns(Channel);
}

CCursorPos CPatternEditor::GetCursorAtPoint(const CPoint &point) const
{
	// // // Removed GetRowAtPoint and GetFrameAtPoint
	int Frame = m_cpCursorPos.m_iFrame;
	int Row = (point.y - HEADER_HEIGHT) / m_iRowHeight - (m_iLinesVisible / 2) + m_iCenterRow;
	
	if (theApp.GetSettings()->General.bFramePreview) {		// // // guarantees valid cursor position
		while (Row < 0) {
			Row += GetCurrentPatternLength(--Frame);
		}
		while (Row >= GetCurrentPatternLength(Frame)) {
			Row -= GetCurrentPatternLength(Frame++);
		}
	}

	return CCursorPos(Row, GetChannelAtPoint(point.x), GetColumnAtPoint(point.x), Frame); // // //
}

std::pair<CPatternIterator, CPatternIterator> CPatternEditor::GetIterators() const
{
	CCursorPos c_it {m_cpCursorPos}, c_end {m_cpCursorPos};
	return IsSelecting() ?
		CPatternIterator::FromSelection(m_selection, m_pDocument, GetSelectedTrack()) :
		CPatternIterator::FromCursor(m_cpCursorPos, m_pDocument, GetSelectedTrack());
}

cursor_column_t CPatternEditor::GetChannelColumns(int Channel) const
{
	// Return number of available columns in a channel
	if (m_bCompactMode) return C_NOTE;
	unsigned int Col = m_pDocument->GetEffColumns(GetSelectedTrack(), Channel);
	switch (Col) {
	case 0: return C_EFF1_PARAM2;
	case 1: return C_EFF2_PARAM2;
	case 2: return C_EFF3_PARAM2;
	case 3: return C_EFF4_PARAM2;
	default: return C_EFF1_PARAM2;
	}
	return C_NOTE;
}

int CPatternEditor::GetSelectedTrack() const
{
	return GetMainFrame()->GetSelectedTrack();
}

int CPatternEditor::GetChannelCount() const
{
	return m_pDocument->GetAvailableChannels();
}

int CPatternEditor::GetFrameCount() const		// // //
{
	return m_pDocument->GetFrameCount(GetSelectedTrack());
}

int CPatternEditor::RowToLine(int Row) const
{
	// Turn row number into line number
	const int MiddleLine = m_iLinesVisible / 2;
	return Row - m_iCenterRow + MiddleLine;
}

// Cursor movement

void CPatternEditor::CancelSelection()
{
	if (m_bSelecting)
		m_bSelectionInvalidated = true;

	m_bSelecting = false;
	m_bCurrentlySelecting = false;
	m_iWarpCount = 0;		// // //
	m_iDragBeginWarp = 0;		// // //
	m_selection.m_cpEnd = m_selection.m_cpStart = m_cpCursorPos;		// // //

	m_bDragStart = false;
	m_bDragging = false;
}

void CPatternEditor::SetSelectionStart(const CCursorPos &start)
{
	if (m_bSelecting)
		m_bSelectionInvalidated = true;

	CCursorPos Pos = start;		// // //
	Pos.m_iFrame %= GetFrameCount();
	Pos.m_iFrame += GetFrameCount() * (m_iWarpCount + (Pos.m_iFrame < 0));
	m_selection.m_cpStart = Pos;
}

void CPatternEditor::SetSelectionEnd(const CCursorPos &end)
{
	if (m_bSelecting)
		m_bSelectionInvalidated = true;

	CCursorPos Pos = end;		// // //
	Pos.m_iFrame %= GetFrameCount();
	Pos.m_iFrame += GetFrameCount() * (m_iWarpCount + (Pos.m_iFrame < 0));
	m_selection.m_cpEnd = Pos;
}

CPatternEditor::CSelectionGuard::CSelectionGuard(CPatternEditor *pEditor) : m_pPatternEditor(pEditor)		// // //
{
	// Call before cursor has moved
	if (IsShiftPressed() && !pEditor->m_bCurrentlySelecting && !pEditor->m_bSelecting) {
		pEditor->SetSelectionStart(pEditor->m_cpCursorPos);
		pEditor->m_bCurrentlySelecting = true;
		pEditor->m_bSelecting = true;
	}
}

CPatternEditor::CSelectionGuard::~CSelectionGuard()		// // //
{
	// Call after cursor has moved
	// If shift is not pressed, set selection starting point to current cursor position
	// If shift is pressed, update selection end point

	CSelection &Sel = m_pPatternEditor->m_selection;

	if (IsShiftPressed()) {
		m_pPatternEditor->SetSelectionEnd(m_pPatternEditor->m_cpCursorPos);
		if (m_pPatternEditor->m_bCompactMode) {		// // //
			m_pPatternEditor->m_bCompactMode = false;
			if (Sel.m_cpEnd.m_iChannel >= Sel.m_cpStart.m_iChannel) {
				Sel.m_cpEnd.m_iColumn = m_pPatternEditor->	GetChannelColumns(Sel.m_cpEnd.m_iChannel);
				Sel.m_cpStart.m_iColumn = C_NOTE;
			}
			else {
				Sel.m_cpEnd.m_iColumn = C_NOTE;
				Sel.m_cpStart.m_iColumn = m_pPatternEditor->GetChannelColumns(Sel.m_cpStart.m_iChannel);
			}
			m_pPatternEditor->m_bCompactMode = true;
		}
		m_pPatternEditor->UpdateSelectionCondition();		// // //
	}
	else {
		m_pPatternEditor->m_bCurrentlySelecting = false;

		if (theApp.GetSettings()->General.iEditStyle != EDIT_STYLE_IT || !m_pPatternEditor->m_bSelecting)
			m_pPatternEditor->CancelSelection();
	}

	const int Frames = m_pPatternEditor->GetFrameCount();
	if (Sel.GetFrameEnd() - Sel.GetFrameStart() > Frames ||		// // //
		(Sel.GetFrameEnd() - Sel.GetFrameStart() == Frames &&
		Sel.GetRowEnd() >= Sel.GetRowStart())) { // selection touches itself
			if (Sel.m_cpEnd.m_iFrame >= Frames) {
				Sel.m_cpEnd.m_iFrame -= Frames;
				m_pPatternEditor->m_iWarpCount = 0;
			}
			if (Sel.m_cpEnd.m_iFrame < 0) {
				Sel.m_cpEnd.m_iFrame += Frames;
				m_pPatternEditor->m_iWarpCount = 0;
			}
	}
}

void CPatternEditor::MoveDown(int Step)
{
	CSelectionGuard Guard {this};		// // //

	Step = (Step == 0) ? 1 : Step;
	MoveToRow(m_cpCursorPos.m_iRow + Step);
}

void CPatternEditor::MoveUp(int Step)
{
	CSelectionGuard Guard {this};		// // //

	Step = (Step == 0) ? 1 : Step;
	MoveToRow(m_cpCursorPos.m_iRow - Step);
}

void CPatternEditor::MoveLeft()
{
	CSelectionGuard Guard {this};		// // //

	ScrollLeft();
}

void CPatternEditor::MoveRight()
{
	CSelectionGuard Guard {this};		// // //

	ScrollRight();
}

void CPatternEditor::MoveToTop()
{
	CSelectionGuard Guard {this};		// // //

	MoveToRow(0);
}

void CPatternEditor::MoveToBottom()
{
	CSelectionGuard Guard {this};		// // //

	MoveToRow(m_iPatternLength - 1);
}

void CPatternEditor::NextChannel()
{
	MoveToChannel(m_cpCursorPos.m_iChannel + 1);
	m_cpCursorPos.m_iColumn = C_NOTE;

	CancelSelection();		// // //
}

void CPatternEditor::PreviousChannel()
{
	MoveToChannel(m_cpCursorPos.m_iChannel - 1);
	m_cpCursorPos.m_iColumn = C_NOTE;

	CancelSelection();		// // //
}

void CPatternEditor::FirstChannel()
{
	CSelectionGuard Guard {this};		// // //

	MoveToChannel(0);
	m_cpCursorPos.m_iColumn	= C_NOTE;
}

void CPatternEditor::LastChannel()
{
	CSelectionGuard Guard {this};		// // //

	MoveToChannel(GetChannelCount() - 1);
	m_cpCursorPos.m_iColumn	= C_NOTE;
}

void CPatternEditor::MoveChannelLeft()
{
	CSelectionGuard Guard {this};		// // //

	const int ChannelCount = GetChannelCount();

	// Wrapping
	if (--m_cpCursorPos.m_iChannel < 0)
		m_cpCursorPos.m_iChannel = ChannelCount - 1;

	cursor_column_t Columns = GetChannelColumns(m_cpCursorPos.m_iChannel);

	if (Columns < m_cpCursorPos.m_iColumn)
		m_cpCursorPos.m_iColumn = Columns;
}

void CPatternEditor::MoveChannelRight()
{
	CSelectionGuard Guard {this};		// // //

	const int ChannelCount = GetChannelCount();

	// Wrapping
	if (++m_cpCursorPos.m_iChannel > (ChannelCount - 1))
		m_cpCursorPos.m_iChannel = 0;

	cursor_column_t Columns = GetChannelColumns(m_cpCursorPos.m_iChannel);

	if (Columns < m_cpCursorPos.m_iColumn)
		m_cpCursorPos.m_iColumn = Columns;
}

void CPatternEditor::OnHomeKey()
{
	CSelectionGuard Guard {this};		// // //

	const bool bControl = IsControlPressed();

	if (bControl || theApp.GetSettings()->General.iEditStyle == EDIT_STYLE_FT2 || theApp.GetSettings()->General.iEditStyle == EDIT_STYLE_FT2_JP) {
		// Control or FT2 edit style
		MoveToTop();
	}
	else {
		if (GetColumn() != C_NOTE)
			MoveToColumn(C_NOTE);
		else if (GetChannel() != 0)
			MoveToChannel(0);
		else if (GetRow() != 0)
			MoveToRow(0);
	}
}

void CPatternEditor::OnEndKey()
{
	CSelectionGuard Guard {this};		// // //

	const bool bControl = IsControlPressed();
	const int Channels = GetChannelCount();
	const cursor_column_t Columns = GetChannelColumns(GetChannel());

	if (bControl || theApp.GetSettings()->General.iEditStyle == EDIT_STYLE_FT2 || theApp.GetSettings()->General.iEditStyle == EDIT_STYLE_FT2_JP) {
		// Control or FT2 edit style
		MoveToBottom();
	}
	else {
		if (GetColumn() != Columns)
			MoveToColumn(Columns);
		else if (GetChannel() != Channels - 1) {
			MoveToChannel(Channels - 1);
			MoveToColumn(GetChannelColumns(Channels - 1));
		}
		else if (GetRow() != m_iPatternLength - 1)
			MoveToRow(m_iPatternLength - 1);
	}
}

void CPatternEditor::MoveCursor(const CCursorPos &Pos)		// // //
{
	m_cpCursorPos = Pos;
}

void CPatternEditor::MoveToRow(int Row)
{
	if (theApp.IsPlaying() && m_bFollowMode)
		return;

	if (theApp.GetSettings()->General.bWrapFrames) {		// // //
		while (Row < 0) {
			MoveToFrame(m_cpCursorPos.m_iFrame - 1);
			Row += m_iPatternLength;
		}
		while (Row >= m_iPatternLength) {
			Row -= m_iPatternLength;
			MoveToFrame(m_cpCursorPos.m_iFrame + 1);
		}
	}
	else if (theApp.GetSettings()->General.bWrapCursor) {
		Row %= m_iPatternLength;
		if (Row < 0) Row += m_iPatternLength;
	}
	else
		Row = std::min(std::max(Row, 0), m_iPatternLength - 1);

	m_cpCursorPos.m_iRow = Row;
}

void CPatternEditor::MoveToFrame(int Frame)
{
	const int FrameCount = GetFrameCount();		// // //

	if (!m_bSelecting)
		m_iWarpCount = 0;		// // //

	if (theApp.GetSettings()->General.bWrapFrames) {
		if (m_bSelecting) {		// // //
			if (theApp.GetSettings()->General.bMultiFrameSel) {		// // //
				if (Frame < 0)
					m_iWarpCount--;
				else if (Frame / FrameCount > m_cpCursorPos.m_iFrame / FrameCount)
					m_iWarpCount++;
			}
		}
		Frame %= FrameCount;
		if (Frame < 0)
			Frame += FrameCount;
	}
	else
		Frame = std::min(std::max(Frame, 0), FrameCount - 1);

	if (m_bSelecting && !theApp.GetSettings()->General.bMultiFrameSel)		// // //
		m_selection.m_cpStart.m_iFrame = m_selection.m_cpEnd.m_iFrame = Frame;
	
	if (theApp.IsPlaying() && m_bFollowMode) {
		if (m_iPlayFrame != Frame) {
			theApp.GetSoundGenerator()->MoveToFrame(Frame);
			theApp.GetSoundGenerator()->ResetTempo();
		}
	}

	m_cpCursorPos.m_iFrame = Frame;		// // //
	UpdatePatternLength();		// // //
	// CancelSelection();
}

void CPatternEditor::MoveToChannel(int Channel)
{
	const int ChannelCount = GetChannelCount();

	if (Channel == m_cpCursorPos.m_iChannel)
		return;

	if (Channel < 0) {
		if (theApp.GetSettings()->General.bWrapCursor)
			Channel = ChannelCount - 1;
		else
			Channel = 0;
	}
	else if (Channel > ChannelCount - 1) {
		if (theApp.GetSettings()->General.bWrapCursor)
			Channel = 0;
		else
			Channel = ChannelCount - 1;
	}
	m_cpCursorPos.m_iChannel = Channel;
	m_cpCursorPos.m_iColumn = C_NOTE;
}

void CPatternEditor::MoveToColumn(cursor_column_t Column)
{
	m_cpCursorPos.m_iColumn = Column;
}

void CPatternEditor::NextFrame()
{
	CSelectionGuard Guard {this};		// // //

	MoveToFrame(m_cpCursorPos.m_iFrame + 1);
	CancelSelection();
}

void CPatternEditor::PreviousFrame()
{
	CSelectionGuard Guard {this};		// // //

	MoveToFrame(m_cpCursorPos.m_iFrame - 1);
	CancelSelection();
}

// Used by scrolling

void CPatternEditor::ScrollLeft()
{
	if (m_cpCursorPos.m_iColumn > 0)
		m_cpCursorPos.m_iColumn = static_cast<cursor_column_t>(m_cpCursorPos.m_iColumn - 1);
	else {
		if (m_cpCursorPos.m_iChannel > 0) {
			m_cpCursorPos.m_iChannel--;
			m_cpCursorPos.m_iColumn = m_iColumns[m_cpCursorPos.m_iChannel];
		}
		else {
			if (theApp.GetSettings()->General.bWrapCursor) {
				m_cpCursorPos.m_iChannel = GetChannelCount() - 1;
				m_cpCursorPos.m_iColumn = m_iColumns[m_cpCursorPos.m_iChannel];
			}
		}
	}
}

void CPatternEditor::ScrollRight()
{
	if (m_cpCursorPos.m_iColumn < m_iColumns[m_cpCursorPos.m_iChannel])
		m_cpCursorPos.m_iColumn = static_cast<cursor_column_t>(m_cpCursorPos.m_iColumn + 1);
	else {
		if (m_cpCursorPos.m_iChannel < GetChannelCount() - 1) {
			m_cpCursorPos.m_iChannel++;
			m_cpCursorPos.m_iColumn = C_NOTE;
		}
		else {
			if (theApp.GetSettings()->General.bWrapCursor) {
				m_cpCursorPos.m_iChannel = 0;
				m_cpCursorPos.m_iColumn = C_NOTE;
			}
		}
	}
}

void CPatternEditor::ScrollNextChannel()
{
	MoveToChannel(m_cpCursorPos.m_iChannel + 1);
	m_cpCursorPos.m_iColumn = C_NOTE;
}

void CPatternEditor::ScrollPreviousChannel()
{
	MoveToChannel(m_cpCursorPos.m_iChannel - 1);
	m_cpCursorPos.m_iColumn = C_NOTE;
}

// Mouse routines

bool CPatternEditor::IsOverHeader(const CPoint &point) const
{
	return point.y < HEADER_HEIGHT;
}

bool CPatternEditor::IsOverPattern(const CPoint &point) const
{
	return point.y >= HEADER_HEIGHT;
}

bool CPatternEditor::IsInsidePattern(const CPoint &point) const
{
	return point.x < (m_iPatternWidth + m_iRowColumnWidth);
}

bool CPatternEditor::IsInsideRowColumn(const CPoint &point) const
{
	return point.x < m_iRowColumnWidth;
}

void CPatternEditor::OnMouseDownHeader(const CPoint &point)
{
	// Channel headers
	const int ChannelCount = GetChannelCount();
	const int Channel = GetChannelAtPoint(point.x);
	const int Column = GetColumnAtPoint(point.x);

	if (Channel < 0 || Channel >= ChannelCount) {
		// Outside of the channel area
		m_pView->UnmuteAllChannels();
		return;
	}

	// Mute/unmute
	if (Column < 5) {
		m_iChannelPushed = Channel;
		m_bChannelPushed = true;
	}
	// Remove one track effect column
	else if (Column == 5) {
		DecreaseEffectColumn(Channel);
	}
	// Add one track effect column
	else if (Column == 6) {
		IncreaseEffectColumn(Channel);
	}
}

void CPatternEditor::OnMouseDownPattern(const CPoint &point)
{
	const int ChannelCount = GetChannelCount();
	const bool bShift = IsShiftPressed();
	const bool bControl = IsControlPressed();

	// Pattern area
	CCursorPos PointPos = GetCursorAtPoint(point);
	const int PatternLength = GetCurrentPatternLength(PointPos.m_iFrame);		// // //

	m_iDragBeginWarp = PointPos.m_iFrame / GetFrameCount();		// // //
	if (PointPos.m_iFrame % GetFrameCount() < 0) m_iDragBeginWarp--;

	if (bShift && !IsInRange(m_selection, PointPos.m_iFrame, PointPos.m_iRow, PointPos.m_iChannel, PointPos.m_iColumn)) {		// // //
		// Expand selection
		if (!PointPos.IsValid(PatternLength, ChannelCount))
			return;
		if (!m_bSelecting)
			SetSelectionStart(m_cpCursorPos);
		m_bSelecting = true;
		SetSelectionEnd(PointPos);
		m_bFullRowSelect = false;
		m_ptSelStartPoint = point;
		m_bMouseActive = true;
	}
	else {
		if (IsInsideRowColumn(point)) {
			// Row number column
			CancelSelection();
			PointPos.m_iRow = std::max(PointPos.m_iRow, 0);
			PointPos.m_iRow = std::min(PointPos.m_iRow, PatternLength - 1);		// // //
			m_selection.m_cpStart = CCursorPos(PointPos.m_iRow, 0, C_NOTE, PointPos.m_iFrame);		// // //
			m_selection.m_cpEnd = CCursorPos(PointPos.m_iRow, ChannelCount - 1, GetChannelColumns(ChannelCount - 1), PointPos.m_iFrame);
			m_bFullRowSelect = true;
			m_ptSelStartPoint = point;
			m_bMouseActive = true;
		}
		else if (IsInsidePattern(point)) {
			// Pattern area
			m_bFullRowSelect = false;

			if (!PointPos.IsValid(PatternLength, ChannelCount))
				return;

			if (IsSelecting()) {
				if (m_pView->GetEditMode() &&		// // //
					IsInRange(m_selection, PointPos.m_iFrame, PointPos.m_iRow, PointPos.m_iChannel, PointPos.m_iColumn)) {
					m_bDragStart = true;
				}
				else {
					m_bDragStart = false;
					m_bDragging = false;
					CancelSelection();
					m_iDragBeginWarp = PointPos.m_iFrame / GetFrameCount();		// // //
					if (PointPos.m_iFrame % GetFrameCount() < 0) m_iDragBeginWarp--;
				}
			}

			if (!m_bDragging && !m_bDragStart) {
				// Begin new selection
				if (bControl) {
					PointPos.m_iColumn = C_NOTE;
				}
				SetSelectionStart(PointPos);
				SetSelectionEnd(PointPos);
			}
			else {
				column_t Col = GetSelectColumn(PointPos.m_iColumn);
				m_cpDragPoint = CCursorPos(PointPos.m_iRow, PointPos.m_iChannel, GetCursorStartColumn(Col), PointPos.m_iFrame); // // //
			}

			m_ptSelStartPoint = point;
			m_bMouseActive = true;
		}
		else {
			// Clicked outside the patterns
			m_bDragStart = false;
			m_bDragging = false;
			CancelSelection();
		}
	}
}

void CPatternEditor::OnMouseDown(const CPoint &point)
{
	// Left mouse button down
	if (IsOverHeader(point)) {
		OnMouseDownHeader(point);
	}
	else if (IsOverPattern(point)) {
		OnMouseDownPattern(point);
	}
}

void CPatternEditor::OnMouseUp(const CPoint &point)
{
	GetMainFrame()->ResetFind();		// // //

	// Left mouse button released
	const int ChannelCount = GetChannelCount();
	const int PushedChannel = m_iChannelPushed;

	m_iScrolling = SCROLL_NONE;
	m_iChannelPushed = -1;

	if (IsOverHeader(point)) {
		const int Channel = GetChannelAtPoint(point.x);

		if (PushedChannel != -1 && PushedChannel == Channel)
			m_pView->ToggleChannel(PushedChannel);

		// Channel headers
		if (m_bDragging) {
			m_bDragging = false;
			m_bDragStart = false;
		}
	}
	else if (IsOverPattern(point)) {

		if (!m_bMouseActive)
			return;

		m_bMouseActive = false;

		// Pattern area
		CCursorPos PointPos = GetCursorAtPoint(point);
		PointPos.m_iFrame %= GetFrameCount(); 
		if (PointPos.m_iFrame < 0) PointPos.m_iFrame += GetFrameCount();		// // //
		const int PatternLength = GetCurrentPatternLength(PointPos.m_iFrame);		// // //

		if (IsInsideRowColumn(point)) {
			if (m_bDragging) {
				m_bDragging = false;
				m_bDragStart = false;
			}
			// Row column, move to clicked row
			if (m_bSelecting) {		// // //
				UpdateSelectionCondition();
				m_bSelectionInvalidated = true;
				return;
			}
			m_cpCursorPos.m_iRow = PointPos.m_iRow;
			m_cpCursorPos.m_iFrame = PointPos.m_iFrame;		// // //
			m_iDragBeginWarp = 0;		// // //
			return;
		}

		if (m_bDragStart && !m_bDragging) {
			m_bDragStart = false;
			CancelSelection();
		}

		if (m_bSelecting) {
			UpdateSelectionCondition();		// // //
			m_bSelectionInvalidated = true;
			return;
		}

		if (PointPos.IsValid(PatternLength, ChannelCount)) {		// // //
			m_cpCursorPos = PointPos;
			CancelSelection();		// // //
		}
	}
}

void CPatternEditor::BeginMouseSelection(const CPoint &point)
{
	CCursorPos PointPos = GetCursorAtPoint(point);

	// Enable selection only if in the pattern field
	if (IsInsidePattern(point)) {
		// Selection threshold
		if (abs(m_ptSelStartPoint.x - point.x) > m_iDragThresholdX || abs(m_ptSelStartPoint.y - point.y) > m_iDragThresholdY) {
			m_iSelectionCondition = SEL_CLEAN;		// // //
			m_bSelecting = true;
		}
	}
}

void CPatternEditor::ContinueMouseSelection(const CPoint &point)
{
	const bool bControl = IsControlPressed();
	const int ChannelCount = GetChannelCount();
	const int FrameCount = GetFrameCount();
	const int Track = GetSelectedTrack();

	CCursorPos PointPos = GetCursorAtPoint(point);

	// Selecting or dragging
	PointPos.m_iRow = std::max(PointPos.m_iRow, 0);
	PointPos.m_iRow = std::min(PointPos.m_iRow, GetCurrentPatternLength(PointPos.m_iFrame) - 1);		// // //
	PointPos.m_iChannel = std::max(PointPos.m_iChannel, 0);
	PointPos.m_iChannel = std::min(PointPos.m_iChannel, ChannelCount - 1);

	if (m_bDragStart) {
		// Dragging
		if (abs(m_ptSelStartPoint.x - point.x) > m_iDragThresholdX || abs(m_ptSelStartPoint.y - point.y) > m_iDragThresholdY) {
			// Initiate OLE drag & drop
			PointPos = GetCursorAtPoint(m_ptSelStartPoint);		// // //
			CSelection Original = m_selection;
			if (m_selection.m_cpEnd < m_selection.m_cpStart) {
				m_selection.m_cpStart = m_selection.m_cpEnd;
			}
			m_selection.m_cpEnd = PointPos;
			while (m_selection.m_cpEnd < m_selection.m_cpStart)
				m_selection.m_cpEnd.m_iFrame += FrameCount;
			while (!GetSelectionSize()) // correction for overlapping selection
				m_selection.m_cpEnd.m_iFrame -= FrameCount;
			int ChanOffset = PointPos.m_iChannel - Original.GetChanStart();
			int RowOffset = GetSelectionSize() - 1;
			m_selection = Original;
			m_bDragStart = false;
			m_pView->BeginDragData(ChanOffset, RowOffset);
		}
	}
	else if (!m_pView->IsDragging()) {
		// Expand selection
		if (bControl || m_bCompactMode) {		// // //
			bool Compact = m_bCompactMode; // temp
			m_bCompactMode = false;
			if (PointPos.m_iChannel >= m_selection.m_cpStart.m_iChannel) {
				PointPos.m_iColumn = GetChannelColumns(PointPos.m_iChannel);
				m_selection.m_cpStart.m_iColumn = C_NOTE;
			}
			else {
				PointPos.m_iColumn = C_NOTE;
				m_selection.m_cpStart.m_iColumn = GetChannelColumns(m_selection.m_cpStart.m_iChannel);
			}
			m_bSelectionInvalidated = true;
			m_bCompactMode = Compact;
		}

		// Full row selection
		if (m_bFullRowSelect) {
			m_selection.m_cpEnd.m_iRow = PointPos.m_iRow;
			m_selection.m_cpEnd.m_iFrame = PointPos.m_iFrame;		// // //
			m_selection.m_cpEnd.m_iColumn = GetChannelColumns(GetChannelCount() - 1);
		}
		else
			SetSelectionEnd(PointPos);

		int Warp = PointPos.m_iFrame / FrameCount;		// // //
		if (PointPos.m_iFrame % FrameCount < 0) Warp--;
		m_iWarpCount = Warp - m_iDragBeginWarp;

		m_selection.m_cpEnd.m_iFrame %= FrameCount;
		m_selection.m_cpEnd.m_iFrame += FrameCount * (m_iWarpCount + (m_selection.m_cpEnd.m_iFrame < 0));

		if (m_selection.GetFrameEnd() - m_selection.GetFrameStart() > FrameCount ||		// // //
			(m_selection.GetFrameEnd() - m_selection.GetFrameStart() == FrameCount &&
			m_selection.GetRowEnd() >= m_selection.GetRowStart())) {
				if (m_selection.m_cpEnd.m_iFrame >= FrameCount) {
					m_selection.m_cpEnd.m_iFrame -= FrameCount;
					m_iWarpCount = 0;
				}
				if (m_selection.m_cpEnd.m_iFrame < 0) {
					m_selection.m_cpEnd.m_iFrame += FrameCount;
					m_iWarpCount = 0;
				}
		}

		if (!theApp.GetSettings()->General.bMultiFrameSel) {		// // //
			m_selection.m_cpEnd.m_iFrame %= FrameCount;
			if (m_selection.m_cpEnd.m_iFrame < 0) m_selection.m_cpEnd.m_iFrame += FrameCount;
			m_selection.m_cpStart.m_iFrame %= FrameCount;
			if (m_selection.m_cpStart.m_iFrame < 0) m_selection.m_cpStart.m_iFrame += FrameCount;

			if (m_selection.m_cpEnd.m_iFrame > m_selection.m_cpStart.m_iFrame) {
				m_selection.m_cpEnd.m_iFrame = m_selection.m_cpStart.m_iFrame;
				m_selection.m_cpEnd.m_iRow = GetCurrentPatternLength(m_cpCursorPos.m_iFrame) - 1;
			}
			else if (m_selection.m_cpEnd.m_iFrame < m_selection.m_cpStart.m_iFrame) {
				m_selection.m_cpEnd.m_iFrame = m_selection.m_cpStart.m_iFrame;
				m_selection.m_cpEnd.m_iRow = 0;
			}
		}

		// Selection has changed
		m_bSelectionInvalidated = true;
	}
}

void CPatternEditor::OnMouseMove(UINT nFlags, const CPoint &point)
{
	// Move movement, called only when lbutton is active

	bool WasPushed = m_bChannelPushed;

	if (IsOverHeader(point) && point.y > 0) {
		const int Channel = GetChannelAtPoint(point.x);
		m_bChannelPushed = m_iChannelPushed == Channel;
	}
	else {
		m_bChannelPushed = false;
	}

	if (m_iChannelPushed != -1 && WasPushed != m_bChannelPushed)
		InvalidateHeader();

	// Check if selection is ongoing, otherwise return
	if (!m_bMouseActive)
		return;

	if (IsSelecting())
		ContinueMouseSelection(point);
	else
		BeginMouseSelection(point);

	// Auto-scrolling
	if (m_bSelecting) {
		AutoScroll(point, nFlags);
	}
}

void CPatternEditor::OnMouseDblClk(const CPoint &point)
{
	// Mouse double click
	const int ChannelCount = GetChannelCount();
	const bool bShift = IsShiftPressed();

	m_bMouseActive = false;

	if (IsOverHeader(point)) {
		// Channel headers
		int Channel = GetChannelAtPoint(point.x);
		int Column = GetColumnAtPoint(point.x);

		if (Channel < 0 || Channel >= ChannelCount)
			return;

		// Solo
		if (Column < 5) {
			m_pView->SoloChannel(Channel);
		}
		// Remove one track effect column
		else if (Column == 5) {
			DecreaseEffectColumn(Channel);
		}
		// Add one track effect column
		else if (Column == 6) {
			IncreaseEffectColumn(Channel);
		}
	}
	else if (IsOverPattern(point) && m_pView->GetStepping() != 0) {		// // //
		if (bShift)
			return;
		if (IsInsideRowColumn(point))
			// Select whole frame
			SelectAllChannels();
		else if (IsInsidePattern(point))
			// Select whole channel
			SelectChannel();
	}
}

void CPatternEditor::OnMouseScroll(int Delta)
{
	// Mouse scroll wheel
	if (theApp.IsPlaying() && m_bFollowMode)
		return;

	if (Delta != 0) {
		int ScrollLength = (Delta < 0) ? theApp.GetSettings()->General.iPageStepSize : -theApp.GetSettings()->General.iPageStepSize;
		m_cpCursorPos.m_iRow += ScrollLength;

		if (theApp.GetSettings()->General.bWrapFrames) {		// // //
			while (m_cpCursorPos.m_iRow < 0) {
				if (m_cpCursorPos.m_iFrame == 0 && m_bSelecting) ++m_iDragBeginWarp;
				MoveToFrame(m_cpCursorPos.m_iFrame - 1);
				m_cpCursorPos.m_iRow += m_iPatternLength;
			}
			while (m_cpCursorPos.m_iRow > (m_iPatternLength - 1)) {
				m_cpCursorPos.m_iRow -= m_iPatternLength;
				MoveToFrame(m_cpCursorPos.m_iFrame + 1);
				if (m_cpCursorPos.m_iFrame == 0 && m_bSelecting) --m_iDragBeginWarp;
			}
		}
		else m_cpCursorPos.m_iRow = std::min(std::max(m_cpCursorPos.m_iRow, 0), m_iPatternLength - 1);

		m_iCenterRow = m_cpCursorPos.m_iRow;
		if (theApp.GetSettings()->General.iEditStyle != EDIT_STYLE_IT && m_bSelecting == false)
			CancelSelection();
	}
}

void CPatternEditor::OnMouseRDown(const CPoint &point)
{
	// Right mouse button down
	const int ChannelCount = GetChannelCount();

	if (IsOverPattern(point)) {
		// Pattern area
		CCursorPos PointPos = GetCursorAtPoint(point);
		if (PointPos.IsValid(GetCurrentPatternLength(PointPos.m_iFrame), ChannelCount)) {		// // //
			m_cpCursorPos = PointPos;
		}
	}
}

void CPatternEditor::DragPaste(const CPatternClipData *pClipData, const CSelection *pDragTarget, bool bMix)
{
	// Paste drag'n'drop selections

	// Set cursor location
	m_cpCursorPos = pDragTarget->m_cpStart;

	Paste(pClipData, bMix ? PASTE_MIX : PASTE_DEFAULT, PASTE_DRAG);		// // //

	// // // Update selection
	m_selection.m_cpStart = pDragTarget->m_cpStart;
	m_selection.m_cpEnd = pDragTarget->m_cpEnd;
	if (pDragTarget->m_cpStart.m_iFrame < 0) {
		const int Frames = GetFrameCount();
		m_cpCursorPos.m_iFrame += Frames;
		m_selection.m_cpStart.m_iFrame += Frames;
		m_selection.m_cpEnd.m_iFrame += Frames;
	}
	m_bSelectionInvalidated = true;
}

bool CPatternEditor::OnMouseHover(UINT nFlags, const CPoint &point)
{
	// Mouse hovering
	const int Track = GetSelectedTrack();
	const int ChannelCount = GetChannelCount();
	bool bRedraw = false;

	if (IsOverHeader(point)) {
		int Channel = GetChannelAtPoint(point.x);
		int Column = GetColumnAtPoint(point.x);

		if (Channel < 0 || Channel >= ChannelCount) {
			bRedraw = m_iMouseHoverEffArrow != 0;
			m_iMouseHoverEffArrow = 0;
			return bRedraw;
		}

		bRedraw = m_iMouseHoverChan != Channel;
		m_iMouseHoverChan = Channel;

		if (Column == 5) {
			if (m_pDocument->GetEffColumns(Track, Channel) > 0) {
				bRedraw = m_iMouseHoverEffArrow != 1;
				m_iMouseHoverEffArrow = 1;
			}
		}
		else if (Column == 6) {
			if (m_pDocument->GetEffColumns(Track, Channel) < (MAX_EFFECT_COLUMNS - 1)) {
				bRedraw = m_iMouseHoverEffArrow != 2;
				m_iMouseHoverEffArrow = 2;
			}
		}
		else {
			bRedraw = m_iMouseHoverEffArrow != 0 || bRedraw;
			m_iMouseHoverEffArrow = 0;
		}
	}
	else if (IsOverPattern(point)) {
		bRedraw = (m_iMouseHoverEffArrow != 0) || (m_iMouseHoverChan != -1);
		m_iMouseHoverChan = -1;
		m_iMouseHoverEffArrow = 0;
	}

	return bRedraw;
}

bool CPatternEditor::OnMouseNcMove()
{
	bool bRedraw = (m_iMouseHoverEffArrow != 0) || (m_iMouseHoverChan != -1) || m_bChannelPushed;
	m_iMouseHoverEffArrow = 0;
	m_iMouseHoverChan = -1;
	return bRedraw;	
}

bool CPatternEditor::CancelDragging()
{
	bool WasDragging = m_bDragging || m_bDragStart;
	m_bDragging = false;
	m_bDragStart = false;
	if (WasDragging)
		m_bSelecting = false;
	return WasDragging;
}

int CPatternEditor::GetFrame() const
{
	return m_cpCursorPos.m_iFrame;		// // //
}

int CPatternEditor::GetChannel() const
{
	return m_cpCursorPos.m_iChannel;
}

int CPatternEditor::GetRow() const
{
	return m_cpCursorPos.m_iRow;
}

cursor_column_t CPatternEditor::GetColumn() const
{
	return m_cpCursorPos.m_iColumn;
}

CCursorPos CPatternEditor::GetCursor() const		// // //
{
	return m_cpCursorPos;
}

// Copy and paste ///////////////////////////////////////////////////////////////////////////////////////////

CPatternClipData *CPatternEditor::CopyEntire() const
{
	const int Track = GetSelectedTrack();
	const int ChannelCount = GetChannelCount();
	const int Rows = m_pDocument->GetPatternLength(Track);
	const int Frame = m_cpCursorPos.m_iFrame;		// // //
	
	CPatternClipData *pClipData = new CPatternClipData(ChannelCount, Rows);

	pClipData->ClipInfo.Channels = ChannelCount;
	pClipData->ClipInfo.Rows = Rows;

	for (int i = 0; i < ChannelCount; ++i) {
		for (int j = 0; j < Rows; ++j) {
			m_pDocument->GetNoteData(Track, Frame, i, j, pClipData->GetPattern(i, j));
		}
	}
	
	return pClipData;
}

CPatternClipData *CPatternEditor::Copy() const
{
	// Copy selection
	CPatternIterator it = GetIterators().first;		// // //
	const int Channels	= m_selection.GetChanEnd() - m_selection.GetChanStart() + 1;
	const int Rows		= GetSelectionSize();		// // //
	stChanNote NoteData;

	CPatternClipData *pClipData = new CPatternClipData(Channels, Rows);
	pClipData->ClipInfo.Channels	= Channels;		// // //
	pClipData->ClipInfo.Rows		= Rows;
	pClipData->ClipInfo.StartColumn	= GetSelectColumn(m_selection.GetColStart());		// // //
	pClipData->ClipInfo.EndColumn	= GetSelectColumn(m_selection.GetColEnd());		// // //
	
	for (int r = 0; r < Rows; r++) {		// // //
		for (int i = 0; i < Channels; ++i) {
			stChanNote *Target = pClipData->GetPattern(i, r);
			it.Get(i + m_selection.GetChanStart(), &NoteData);
			/*CopyNoteSection(Target, &NoteData, PASTE_DEFAULT,
				i == 0 ? ColStart : COLUMN_NOTE, i == Channels - 1 ? ColEnd : COLUMN_EFF4);*/
			memcpy(Target, &NoteData, sizeof(stChanNote));
			// the clip data should store the entire field;
			// other methods should check ClipInfo.StartColumn and ClipInfo.EndColumn before operating
		}
		++it;
	}

	return pClipData;
}

CPatternClipData *CPatternEditor::CopyRaw() const		// // //
{
	return CopyRaw(m_selection);
}

CPatternClipData *CPatternEditor::CopyRaw(const CSelection &Sel) const		// // //
{
	const int Track = GetSelectedTrack();
	CCursorPos c_it, c_end;
	Sel.Normalize(c_it, c_end);
	CPatternIterator it {m_pDocument, Track, c_it};
	CPatternIterator end {m_pDocument, Track, c_end};

	const int Frames	= m_pDocument->GetFrameCount(Track);
	const int Length	= m_pDocument->GetPatternLength(Track);
	const int Rows		= (end.m_iFrame - it.m_iFrame) * Length + (end.m_iRow - it.m_iRow) + 1;

	const int cBegin	= it.m_iChannel;
	const int Channels	= end.m_iChannel - cBegin + 1;

	CPatternClipData *pClipData = new CPatternClipData(Channels, Rows);
	pClipData->ClipInfo.Channels	= Channels;
	pClipData->ClipInfo.Rows		= Rows;
	pClipData->ClipInfo.StartColumn	= GetSelectColumn(it.m_iColumn);
	pClipData->ClipInfo.EndColumn	= GetSelectColumn(end.m_iColumn);
	
	const int PackedPos = (it.m_iFrame + Frames) * Length + it.m_iRow;
	for (int r = 0; r < Rows; r++) for (int i = 0; i < Channels; ++i)
		m_pDocument->GetNoteData(Track, (PackedPos + r) / Length % Frames, i + cBegin, (PackedPos + r) % Length, pClipData->GetPattern(i, r));

	return pClipData;
}

void CPatternEditor::Cut()
{
	// Cut selection
}

void CPatternEditor::PasteEntire(const CPatternClipData *pClipData)
{
	// Paste entire
	const int Track = GetSelectedTrack();
	const int Frame = m_cpCursorPos.m_iFrame;		// // //
	for (int i = 0; i < pClipData->ClipInfo.Channels; ++i) {
		for (int j = 0; j < pClipData->ClipInfo.Rows; ++j) {
			m_pDocument->SetNoteData(Track, Frame, i, j, pClipData->GetPattern(i, j));
		}
	}
}

void CPatternEditor::Paste(const CPatternClipData *pClipData, const paste_mode_t PasteMode, const paste_pos_t PastePos)		// // //
{
	// // // Paste
	const unsigned int Track		= GetSelectedTrack();
	const unsigned int ChannelCount = GetChannelCount();

	const unsigned int Channels	   = (PastePos == PASTE_FILL) ?
		m_selection.GetChanEnd() - m_selection.GetChanStart() + 1 : pClipData->ClipInfo.Channels;
	const unsigned int Rows		   = (PastePos == PASTE_FILL) ? GetSelectionSize() : pClipData->ClipInfo.Rows;
	const column_t StartColumn = pClipData->ClipInfo.StartColumn;
	const column_t EndColumn   = pClipData->ClipInfo.EndColumn;
	
	bool AtSel = (PastePos == PASTE_SELECTION || PastePos == PASTE_FILL);
	const int f = AtSel ? m_selection.GetFrameStart() : (PastePos == PASTE_DRAG ? m_selDrag.GetFrameStart() : m_cpCursorPos.m_iFrame);
	const unsigned int r = AtSel ? m_selection.GetRowStart() : (PastePos == PASTE_DRAG ? m_selDrag.GetRowStart() : m_cpCursorPos.m_iRow);
	const unsigned int c = AtSel ? m_selection.GetChanStart() : (PastePos == PASTE_DRAG ? m_selDrag.GetChanStart() : m_cpCursorPos.m_iChannel);
	const unsigned int CEnd = std::min(Channels + c, ChannelCount);

	CPatternIterator it = CPatternIterator(m_pDocument, Track, CCursorPos(r, c, GetCursorStartColumn(StartColumn), f));		// // //
	stChanNote NoteData, Source;

	const unsigned int FrameLength = m_pDocument->GetPatternLength(Track);

	if (PasteMode == PASTE_INSERT) {		// // //
		CPatternIterator front = CPatternIterator(m_pDocument, Track, CCursorPos(FrameLength - 1, c, GetCursorStartColumn(StartColumn), f));
		CPatternIterator back = CPatternIterator(m_pDocument, Track, CCursorPos(FrameLength - 1 - Rows, c, GetCursorEndColumn(EndColumn), f));
		front.m_iFrame = back.m_iFrame = f; // do not warp
		front.m_iRow = FrameLength - 1;
		back.m_iRow = FrameLength - 1 - Rows;
		while (back.m_iRow >= static_cast<int>(r)) {
			for (unsigned int i = c; i < CEnd; i++) {
				back.Get(i, &Source);
				front.Get(i, &NoteData);
				CopyNoteSection(&NoteData, &Source, PasteMode, (i == c) ? StartColumn : COLUMN_NOTE,
					std::min((i == Channels + c - 1) ? EndColumn : COLUMN_EFF4,
								static_cast<column_t>(COLUMN_EFF1 + m_pDocument->GetEffColumns(Track, i))));
				front.Set(i, &NoteData);
			}
			front.m_iRow--;
			back.m_iRow--;
		}
		m_selection.m_cpEnd.m_iRow = std::min(m_selection.m_cpStart.m_iRow + static_cast<int>(Rows), GetCurrentPatternLength(f)) - 1;
	}

	// Special, single channel and effect columns only
	if (Channels == 1 && StartColumn >= COLUMN_EFF1) {
		const unsigned int ColStart = std::max(GetSelectColumn(AtSel ? m_selection.GetColStart() : m_cpCursorPos.m_iColumn), COLUMN_EFF1);
		for (unsigned int j = 0; j < Rows; ++j) {
			it.Get(c, &NoteData);
			Source = *(pClipData->GetPattern(0, j % pClipData->ClipInfo.Rows));
			for (unsigned int i = StartColumn - COLUMN_EFF1; i <= EndColumn - COLUMN_EFF1; ++i) {		// // //
				const unsigned int Offset = i - StartColumn + ColStart;
				if (Offset > m_pDocument->GetEffColumns(Track, c)) break;
				bool Protected = false;
				switch (PasteMode) {
				case PASTE_MIX:
					if (NoteData.EffNumber[Offset] != EF_NONE) Protected = true;
					// continue
				case PASTE_OVERWRITE:
					if (Source.EffNumber[i] == EF_NONE) Protected = true;
				}
				if (!Protected) {
					NoteData.EffNumber[Offset] = Source.EffNumber[i];
					NoteData.EffParam[Offset] = Source.EffParam[i];
				}
			}
			it.Set(c, &NoteData);
			if ((++it).m_iRow == 0) { // end of frame reached
				if ((!theApp.GetSettings()->General.bOverflowPaste && PasteMode != PASTE_OVERFLOW) ||
					PasteMode == PASTE_INSERT) break;
			}
			if (it.m_iFrame == f && it.m_iRow == r) break;
		}
		return;
	}

	for (unsigned int j = 0; j < Rows; ++j) {
		for (unsigned int i = c; i < CEnd; ++i) {
			int cGet = (i - c) % pClipData->ClipInfo.Channels;
			const column_t ColEnd = std::min((i == Channels + c - 1) ? EndColumn : COLUMN_EFF4,
				static_cast<column_t>(COLUMN_EFF1 + m_pDocument->GetEffColumns(Track, i)));
			it.Get(i, &NoteData);
			Source = *(pClipData->GetPattern(cGet, j % pClipData->ClipInfo.Rows));
			CopyNoteSection(&NoteData, &Source, PasteMode, (!cGet) ? StartColumn : COLUMN_NOTE, ColEnd);
			it.Set(i, &NoteData);
		}
		if ((++it).m_iRow == 0) { // end of frame reached
			if ((!theApp.GetSettings()->General.bOverflowPaste && PasteMode != PASTE_OVERFLOW) ||
				PasteMode == PASTE_INSERT) break;
		}
		if (!((it.m_iFrame - f) % GetFrameCount()) && it.m_iRow == r) break;
	}
}

void CPatternEditor::PasteRaw(const CPatternClipData *pClipData)		// // //
{
	PasteRaw(pClipData, GetIterators().first);
}

void CPatternEditor::PasteRaw(const CPatternClipData *pClipData, const CCursorPos &Pos)		// // //
{
	const int Track = GetSelectedTrack();
	CPatternIterator it {m_pDocument, Track, Pos};
	const int Frames = m_pDocument->GetFrameCount(Track);
	const int Length = m_pDocument->GetPatternLength(Track);

	const int Rows = pClipData->ClipInfo.Rows;
	const int Channels = pClipData->ClipInfo.Channels;
	const column_t StartColumn = pClipData->ClipInfo.StartColumn;
	const column_t EndColumn = pClipData->ClipInfo.EndColumn;

	stChanNote Target { }, Source { };
	const int PackedPos = (Pos.m_iFrame + GetFrameCount()) * Length + Pos.m_iRow;
	for (int i = 0; i < Channels; ++i) for (int r = 0; r < Rows; r++) {
		int c = i + Pos.m_iChannel;
		if (c == GetChannelCount()) return;
		m_pDocument->GetNoteData(Track, (PackedPos + r) / Length % Frames, c, (PackedPos + r) % Length, &Target);
		Source = *(pClipData->GetPattern(i, r));
		CopyNoteSection(&Target, &Source, PASTE_DEFAULT, (i == 0) ? StartColumn : COLUMN_NOTE,
			std::min((i == Channels + Pos.m_iChannel - 1) ? EndColumn : COLUMN_EFF4,
						static_cast<column_t>(COLUMN_EFF1 + m_pDocument->GetEffColumns(Track, c))));
		m_pDocument->SetNoteData(Track, (PackedPos + r) / Length % Frames, c, (PackedPos + r) % Length, &Target);
	}
}

bool CPatternEditor::IsSelecting() const
{
	return m_bSelecting;
}

void CPatternEditor::SelectChannel()
{
	// Select entire channel
	m_bSelecting = true;
	SetSelectionStart(CCursorPos(0, m_cpCursorPos.m_iChannel, C_NOTE, m_cpCursorPos.m_iFrame));		// // //
	SetSelectionEnd(CCursorPos(m_iPatternLength - 1, m_cpCursorPos.m_iChannel, GetChannelColumns(m_cpCursorPos.m_iChannel), m_cpCursorPos.m_iFrame));
}

void CPatternEditor::SelectAllChannels()
{
	// Select all channels
	m_bSelecting = true;
	SetSelectionStart(CCursorPos(0, 0, C_NOTE, m_cpCursorPos.m_iFrame));		// // //
	SetSelectionEnd(CCursorPos(m_iPatternLength - 1, GetChannelCount() - 1, GetChannelColumns(GetChannelCount() - 1), m_cpCursorPos.m_iFrame));
}

void CPatternEditor::SelectAll()
{
	bool selectAll = false;

	if (m_bSelecting) {
		if (m_selection.GetChanStart() == m_cpCursorPos.m_iChannel && m_selection.GetChanEnd() == m_cpCursorPos.m_iChannel) {
			if (m_selection.GetRowStart() == 0 && m_selection.GetRowEnd() == m_iPatternLength - 1) {
				if (m_selection.GetColStart() == 0 && m_selection.GetColEnd() == GetChannelColumns(m_cpCursorPos.m_iChannel))
					selectAll = true;
			}
		}
	}

	if (selectAll)
		SelectAllChannels();
	else
		SelectChannel();
}

int CPatternEditor::GetSelectionSize() const		// // //
{
	if (!m_bSelecting) return 0;

	int Rows = 0;		// // //
	const int FrameBegin = m_selection.GetFrameStart();
	const int FrameEnd = m_selection.GetFrameEnd();
	if (FrameEnd - FrameBegin > GetFrameCount() || // selection overlaps itself
		(FrameEnd - FrameBegin == GetFrameCount() && m_selection.GetRowEnd() >= m_selection.GetRowStart()))
		return 0;

	for (int i = FrameBegin; i <= FrameEnd; ++i) {
		const int PatternLength = GetCurrentPatternLength(i);
		Rows += PatternLength;
		if (i == FrameBegin)
			Rows -= std::max(std::min(m_selection.GetRowStart(), PatternLength), 0);
		if (i == FrameEnd)
			Rows -= std::max(PatternLength - m_selection.GetRowEnd() - 1, 0);
	}

	return Rows;
}

sel_condition_t CPatternEditor::GetSelectionCondition() const		// // //
{
	if (!m_bSelecting)
		return SEL_CLEAN;
	return GetSelectionCondition(m_selection);
}

sel_condition_t CPatternEditor::GetSelectionCondition(const CSelection &Sel) const
{
	const int Track = GetSelectedTrack();
	const int Frames = GetFrameCount();
	unsigned char Lo[MAX_PATTERN], Hi[MAX_PATTERN];

	if (!theApp.GetSettings()->General.bShowSkippedRows) {
		auto it = CPatternIterator::FromSelection(Sel, m_pDocument, GetSelectedTrack());
		stChanNote Note;
		for (; it.first <= it.second; ++it.first) {
			// bool HasSkip = false;
			for (int i = 0; i < GetChannelCount(); i++) {
				it.first.Get(i, &Note);
				for (unsigned int c = 0; c <= m_pDocument->GetEffColumns(Track, i); c++) switch (Note.EffNumber[c]) {
				case EF_JUMP: case EF_SKIP: case EF_HALT:
					if (Sel.IsColumnSelected(static_cast<column_t>(COLUMN_EFF1 + c), i))
						return it.first == it.second ? SEL_TERMINAL_SKIP : SEL_NONTERMINAL_SKIP;
					/*else if (it != End)
						HasSkip = true;*/
				}
			}
			/*if (HasSkip)
				return SEL_UNKNOWN_SIZE;*/
		}
	}

	for (int c = Sel.GetChanStart(); c <= Sel.GetChanEnd(); c++) {
		memset(Lo, 255, MAX_PATTERN);
		memset(Hi, 0, MAX_PATTERN);

		for (int i = Sel.GetFrameStart(); i <= Sel.GetFrameEnd(); i++) {
			int Pattern = m_pDocument->GetPatternAtFrame(Track, (i + Frames) % Frames, c);
			int RBegin = i == Sel.GetFrameStart() ? Sel.GetRowStart() : 0;
			int REnd = i == Sel.GetFrameEnd() ? Sel.GetRowEnd() : GetCurrentPatternLength(i) - 1;
			if (Lo[Pattern] <= Hi[Pattern] && RBegin <= Hi[Pattern] && REnd >= Lo[Pattern])
				return SEL_REPEATED_ROW;
			Lo[Pattern] = std::min(Lo[Pattern], static_cast<unsigned char>(RBegin));
			Hi[Pattern] = std::max(Hi[Pattern], static_cast<unsigned char>(REnd));
		}
	}

	return SEL_CLEAN;
}

void CPatternEditor::UpdateSelectionCondition()		// // //
{
	m_iSelectionCondition = GetSelectionCondition();
}

// Other ////////////////////////////////////////////////////////////////////////////////////////////////////

int CPatternEditor::GetCurrentPatternLength(int Frame) const		// // //
{
	return m_pDocument->GetCurrentPatternLength(GetSelectedTrack(), Frame);
}

void CPatternEditor::SetHighlight(const stHighlight Hl)		// // //
{
	m_vHighlight = Hl;
}

void CPatternEditor::SetFollowMove(bool bEnable)
{
	m_bFollowMode = bEnable;
}

void CPatternEditor::SetCompactMode(bool bEnable)		// // //
{
	if (m_bCompactMode != bEnable) {
		CalculatePatternLayout();
		m_bCompactMode = bEnable;
	}
	if (bEnable)
		MoveToColumn(C_NOTE);
}

void CPatternEditor::SetFocus(bool bFocus)
{
	m_bHasFocus = bFocus;
}

void CPatternEditor::IncreaseEffectColumn(int Channel)
{
	const int Columns = m_pDocument->GetEffColumns(GetSelectedTrack(), Channel) + 1;
	GetMainFrame()->AddAction(new CPActionEffColumn {Channel, Columns});		// // //
}

void CPatternEditor::DecreaseEffectColumn(int Channel)
{
	const int Columns = m_pDocument->GetEffColumns(GetSelectedTrack(), Channel) - 1;
	if (GetMainFrame()->AddAction(new CPActionEffColumn {Channel, Columns}))		// // //
		if (static_cast<int>(m_cpCursorPos.m_iColumn) > Columns * 3 + 6)		// // //
			m_cpCursorPos.m_iColumn = static_cast<cursor_column_t>(m_cpCursorPos.m_iColumn - 3);
}

bool CPatternEditor::IsPlayCursorVisible() const
{
	if (m_iPlayFrame > (m_cpCursorPos.m_iFrame + 1))
		return false;

	if (m_iPlayFrame < (m_cpCursorPos.m_iFrame - 1))
		return false;

	if (m_iPlayFrame != (m_cpCursorPos.m_iFrame + 1) && m_iPlayFrame != (m_cpCursorPos.m_iFrame - 1)) {
		
		if (m_iPlayRow > (m_iCenterRow + (m_iLinesFullVisible / 2) + 1))
			return false;

		if (m_iPlayRow < (m_iCenterRow - (m_iLinesFullVisible / 2) - 1))
			return false;
	}

	return true;
}

void CPatternEditor::AutoScroll(const CPoint &point, UINT nFlags)
{
	CCursorPos PointPos = GetCursorAtPoint(point);
	const int Channels = GetChannelCount();

	m_ptScrollMousePos = point;
	m_nScrollFlags = nFlags;

	m_iScrolling = SCROLL_NONE;		// // //

	int Row = (point.y - HEADER_HEIGHT) / m_iRowHeight - (m_iLinesVisible / 2);		// // //

	if (Row > (m_iLinesFullVisible / 2) - 3) {
		m_iScrolling |= SCROLL_DOWN;		// // //
	}
	else if (Row <= -(m_iLinesFullVisible / 2)) {
		m_iScrolling |= SCROLL_UP;		// // //
	}

	if (m_bFullRowSelect) return;		// // //

	if (PointPos.m_iChannel >= (m_iFirstChannel + m_iChannelsVisible - 1) && m_iChannelsVisible < GetChannelCount()) {		// // //
		if (m_cpCursorPos.m_iChannel < Channels - 1)		// // //
			m_iScrolling |= SCROLL_RIGHT;		// // //
	}
	else if (PointPos.m_iChannel < m_iFirstChannel) {
		if (m_cpCursorPos.m_iChannel > 0)
			m_iScrolling |= SCROLL_LEFT;		// // //
	}
}

bool CPatternEditor::ScrollTimerCallback()
{
	const int Channels = GetChannelCount();

	if (!m_iScrolling) return false;		// // //

	switch (m_iScrolling & 0x03) {		// // //
	case SCROLL_UP:
		m_cpCursorPos.m_iRow--;
		m_iCenterRow--;
		break;
	case SCROLL_DOWN:
		m_cpCursorPos.m_iRow++;
		m_iCenterRow++;
		break;
	}
	if (m_cpCursorPos.m_iRow == GetCurrentPatternLength(m_cpCursorPos.m_iFrame)) {		// // //
		m_iCenterRow = m_cpCursorPos.m_iRow = 0;
		if (++m_cpCursorPos.m_iFrame == GetFrameCount())
			m_cpCursorPos.m_iFrame = 0;
	}
	else if (m_cpCursorPos.m_iRow == -1) {
		m_iCenterRow = m_cpCursorPos.m_iRow = GetCurrentPatternLength(--m_cpCursorPos.m_iFrame) - 1;
		if (m_cpCursorPos.m_iFrame < 0)
			m_cpCursorPos.m_iFrame += GetFrameCount();
	}

	switch (m_iScrolling & 0x0C) {		// // //
	case SCROLL_RIGHT:
		if (m_iFirstChannel + m_iChannelsFullVisible < Channels) {
			m_iFirstChannel++;
			if (m_cpCursorPos.m_iChannel < m_iFirstChannel)
				m_cpCursorPos.m_iChannel++;
			InvalidateBackground();
		}
		break;
	case SCROLL_LEFT:
		if (m_iFirstChannel > 0) {
			m_iFirstChannel--;
			if (m_cpCursorPos.m_iChannel >= m_iFirstChannel + m_iChannelsFullVisible)
				m_cpCursorPos.m_iChannel--;
			InvalidateBackground();
		}
		break;
	}

	if (m_bSelecting && !m_bDragging)
		OnMouseMove(m_nScrollFlags, m_ptScrollMousePos);

	return true;
}

void CPatternEditor::OnVScroll(UINT nSBCode, UINT nPos)
{
	int PageSize = theApp.GetSettings()->General.iPageStepSize;

	switch (nSBCode) {
		case SB_LINEDOWN: 
			MoveToRow(m_cpCursorPos.m_iRow + 1);
			break;
		case SB_LINEUP:
			MoveToRow(m_cpCursorPos.m_iRow - 1);
			break;
		case SB_PAGEDOWN:
			MoveToRow(m_cpCursorPos.m_iRow + PageSize);
			break;
		case SB_PAGEUP:
			MoveToRow(m_cpCursorPos.m_iRow - PageSize);
			break;
		case SB_TOP:
			MoveToRow(0);
			break;
		case SB_BOTTOM:	
			MoveToRow(m_iPatternLength - 1);
			break;
		case SB_THUMBPOSITION:
		case SB_THUMBTRACK:
			MoveToRow(nPos);
			break;
	}

	if (!m_bSelecting)
		CancelSelection();
}

void CPatternEditor::OnHScroll(UINT nSBCode, UINT nPos)
{
	const int Channels = GetChannelCount();
	unsigned int count = 0;

	switch (nSBCode) {
		case SB_LINERIGHT: 
			ScrollRight();
			break;
		case SB_LINELEFT: 
			ScrollLeft();
			break;
		case SB_PAGERIGHT: 
			ScrollNextChannel(); 
			break;
		case SB_PAGELEFT: 
			ScrollPreviousChannel(); 
			break;
		case SB_RIGHT: 
			LastChannel(); 
			break;
		case SB_LEFT: 
			FirstChannel(); 
			break;
		case SB_THUMBPOSITION:
		case SB_THUMBTRACK:
			for (int i = 0; i < Channels; ++i) {
				for (unsigned j = 0, Count = GetChannelColumns(i); j <= Count; ++j) {
					if (count++ == nPos) {
						MoveToChannel(i);
						MoveToColumn(static_cast<cursor_column_t>(j));
						goto outer;		// // //
					}
				}
			}
		outer:;
	}

	if (!m_bSelecting)
		CancelSelection();
}

void CPatternEditor::SetBlockStart()
{
	if (!m_bSelecting) {
		m_bSelecting = true;
		SetBlockEnd();
	}
	SetSelectionStart(m_cpCursorPos);
}

void CPatternEditor::SetBlockEnd()
{
	if (!m_bSelecting) {
		m_bSelecting = true;
		SetBlockStart();
	}
	SetSelectionEnd(m_cpCursorPos);
}

CSelection CPatternEditor::GetSelection() const
{
	return m_selection;
}

void CPatternEditor::SetSelection(const CSelection &selection)
{
	// Allow external set selection
	m_selection = selection;
	m_bSelecting = true;
	m_bSelectionInvalidated = true;
}

void CPatternEditor::SetSelection(int Scope)		// // //
{
	int Vert = Scope & 0x0F;
	int Horz = Scope & 0xF0;
	if (!Vert || !Horz) {
		CancelSelection();
		return;
	}

	m_selection.m_cpStart = m_cpCursorPos;
	m_selection.m_cpEnd = m_cpCursorPos;
	if (Vert >= SEL_SCOPE_VTRACK) {
		m_selection.m_cpStart.m_iFrame = 0;
		m_selection.m_cpEnd.m_iFrame = GetFrameCount() - 1;
	}
	if (Vert >= SEL_SCOPE_VFRAME) {
		m_selection.m_cpStart.m_iRow = 0;
		m_selection.m_cpEnd.m_iRow = GetCurrentPatternLength(m_selection.m_cpEnd.m_iFrame) - 1;
	}
	if (Horz >= SEL_SCOPE_HFRAME) {
		m_selection.m_cpStart.m_iChannel = 0;
		m_selection.m_cpEnd.m_iChannel = GetChannelCount() - 1;
	}
	if (Horz >= SEL_SCOPE_HCHAN) {
		m_selection.m_cpStart.m_iColumn = C_NOTE;
		m_selection.m_cpEnd.m_iColumn = GetChannelColumns(m_selection.m_cpEnd.m_iChannel);
	}

	m_bSelecting = true;
	m_iSelectionCondition = GetSelectionCondition();
	m_bSelectionInvalidated = true;
}

void CPatternEditor::GetVolumeColumn(CString &str) const
{
	// Copy the volume column as text

	const int Track = GetSelectedTrack();
	const int Channel = m_selection.GetChanStart();
	auto it = GetIterators();
	stChanNote NoteData;

	if (Channel < 0 || Channel >= GetChannelCount())
		return;
	
	int vol = MAX_VOLUME - 1;		// // //
	CPatternIterator s {it.first};
	do {
		if (--s.m_iFrame < 0) break;
		s.Get(Channel, &NoteData);
		if (NoteData.Vol != MAX_VOLUME) {
			vol = NoteData.Vol;
			break;
		}
	} while (s.m_iFrame > 0 || s.m_iRow > 0);
	
	str.Empty();
	for (; it.first <= it.second; ++it.first) {
		it.first.Get(Channel, &NoteData);
		if (NoteData.Vol != MAX_VOLUME)
			vol = NoteData.Vol;
		str.AppendFormat(_T("%i "), vol);
	}
}

void CPatternEditor::GetSelectionAsText(CString &str) const		// // //
{
	// Copy selection as text

	const int Track = GetSelectedTrack();
	const int Channel = m_selection.GetChanStart() + !m_selection.IsColumnSelected(COLUMN_VOLUME, m_selection.GetChanStart()); // // //
	stChanNote NoteData;

	if (Channel < 0 || Channel >= GetChannelCount() || !m_bSelecting)
		return;

	auto it = GetIterators();
	str.Empty();

	int Row = 0;
	int Size = m_bSelecting ? (GetSelectionSize() - 1) : (it.second.m_iRow - it.first.m_iRow + 1);
	int HexLength = 0;
	do HexLength++; while (Size >>= 4);
	if (HexLength < 2) HexLength = 2;

	CString Header(_T(' '), HexLength + 3);
	Header.Append(_T("# "));
	for (int i = it.first.m_iChannel; i <= it.second.m_iChannel; ++i) {
		Header.AppendFormat(_T(": %-13s"), m_pDocument->GetChannel(i)->GetChannelName());
		unsigned Columns = m_pDocument->GetEffColumns(Track, i);
		if (i == it.second.m_iChannel)
			Columns = std::min(Columns, static_cast<unsigned>(std::max(0, static_cast<int>(GetSelectColumn(it.second.m_iColumn)) - 3)));
		for (unsigned j = 0; j < Columns; j++)
			Header.AppendFormat(_T("fx%d "), j + 2);
	}
	str = Header.TrimRight() + _T("\r\n");
	
	static const int COLUMN_CHAR_POS[] = {0, 4, 7, 9, 13, 17, 21};
	static const int COLUMN_CHAR_LEN[] = {3, 2, 1, 3, 3, 3, 3};
	const int Last = m_pDocument->GetEffColumns(Track, it.second.m_iChannel) + 3;
	const unsigned BegCol = GetSelectColumn(it.first.m_iColumn);
	const unsigned EndCol = GetSelectColumn(it.second.m_iColumn);
	for (; it.first <= it.second; ++it.first) {
		CString line;
		line.AppendFormat(_T("ROW %0*X"), HexLength, Row++);
		for (int i = it.first.m_iChannel; i <= it.second.m_iChannel; ++i) {
			it.first.Get(i, &NoteData);
			CString Row = CTextExport::ExportCellText(NoteData, m_pDocument->GetEffColumns(Track, i) + 1, i == CHANID_NOISE);
			if (i == it.first.m_iChannel) for (unsigned c = 0; c < BegCol; ++c)
				for (int j = 0; j < COLUMN_CHAR_LEN[c]; ++j) Row.SetAt(COLUMN_CHAR_POS[c] + j, ' ');
			if (i == it.second.m_iChannel && EndCol < COLUMN_EFF4)
				Row = Row.Left(COLUMN_CHAR_POS[EndCol + 1] - 1);
			line.AppendFormat(_T(" : %s"), Row);
		}
		str.Append(line);
		str.Append(_T("\r\n"));
	}
}

void CPatternEditor::GetSelectionAsPPMCK(CString &str) const		// // //
{
	// Returns a PPMCK MML translation of copied pattern

	stChanNote NoteData { };

	auto it = GetIterators();
	str.Empty();

	for (int c = it.first.m_iChannel; c <= it.second.m_iChannel; ++c) {
		int Type = m_pDocument->GetChannelType(c);
		switch (m_pDocument->GetChipType(c)) {
		case SNDCHIP_NONE: Type += 'A' - CHANID_SQUARE1; break;
		case SNDCHIP_VRC6: Type += 'M' - CHANID_VRC6_PULSE1; break;
		case SNDCHIP_VRC7: Type += 'G' - CHANID_VRC7_CH1; break;
		case SNDCHIP_FDS:  Type += 'F' - CHANID_FDS; break;
		case SNDCHIP_MMC5: Type += 'a' - CHANID_MMC5_SQUARE1; break;
		case SNDCHIP_N163: Type += 'P' - CHANID_N163_CH1; break;
		case SNDCHIP_S5B:  Type += 'X' - CHANID_S5B_CH1; break;
		}
		str.AppendFormat(_T("%c\t"), Type);

		int o = -1;
		int len = -1;
		bool first = true;
		stChanNote current { };
		current.Note = HALT;
		stChanNote echo[ECHO_BUFFER_LENGTH + 1] { };

		for (CPatternIterator s {it.first}; s <= it.second; ++s) {
			len++;
			s.Get(c, &NoteData);
			bool dump = NoteData.Note != NONE || NoteData.Vol != MAX_VOLUME;
			bool fin = s.m_iFrame == it.second.m_iFrame && s.m_iRow == it.second.m_iRow;

			if (dump || fin) {
				bool push = current.Note != NONE && current.Note != RELEASE;

				if (current.Vol != MAX_VOLUME)
					str.AppendFormat(_T("v%i"), current.Vol);

				if (current.Note == ECHO) {
					current.Note   = echo[current.Octave].Note;
					current.Octave = echo[current.Octave].Octave;
				}

				if (push) {
					for (int i = ECHO_BUFFER_LENGTH - 1; i >= 0; i--)
						echo[i + 1] = echo[i];
					echo[0] = current;
				}

				if (!first || (NoteData.Note != NONE)) switch (current.Note) {
				case NONE: str.Append(_T("w")); break;
				case RELEASE: str.Append(_T("k")); break;
				case HALT: str.Append(_T("r")); break;
				default:
					if (o == -1) {
						o = current.Octave;
						str.AppendFormat(_T("o%i"), o);
					}
					else {
						while (o < current.Octave) {
							o++;
							str.Append(_T(">"));
						}
						while (o > current.Octave) {
							o--;
							str.Append(_T("<"));
						}
					}
					str.AppendFormat(_T("%c"), (current.Note * 7 + 18) / 12 % 7 + 'a');
					if ((current.Note * 7 + 6) % 12 >= 7) str.Append(_T("#"));
				}

				if (fin) len++;
				while (len >= 32) {
					len -= 16;
					str.Append(_T("1^"));
				}
				int l = 16;
				while (l) {
					if (!(len & l)) {
						l >>= 1;
						continue;
					}
					str.AppendFormat(_T("%i"), 16 / l);
					do {
						len -= l;
						l >>= 1;
						if (len & l) {
							str.Append(_T("."));
						}
					} while (len & l);
					if (len) str.Append(_T("^"));
				}

				current = NoteData;
			}

			first = false;
		}
		str.Append(_T("\r\n"));
	}
}


// OLE support

void CPatternEditor::BeginDrag(const CPatternClipData *pClipData)
{
	m_iDragChannels = pClipData->ClipInfo.Channels - 1;
	m_iDragRows = pClipData->ClipInfo.Rows - 1;
	m_iDragStartCol = GetCursorStartColumn(pClipData->ClipInfo.StartColumn);
	m_iDragEndCol = GetCursorEndColumn(pClipData->ClipInfo.EndColumn);

	m_iDragOffsetChannel = pClipData->ClipInfo.OleInfo.ChanOffset;
	m_iDragOffsetRow = pClipData->ClipInfo.OleInfo.RowOffset;

	m_bDragging = true;
}

void CPatternEditor::EndDrag()
{
	m_bDragging = false;
}

bool CPatternEditor::PerformDrop(const CPatternClipData *pClipData, bool bCopy, bool bCopyMix)
{
	// Drop selection onto pattern, returns true if drop was successful

	const int Channels = GetChannelCount();

	m_bDragging = false;
	m_bDragStart = false;

	if (m_bSelecting && m_selection.IsSameStartPoint(m_selDrag)) {
		// Drop area is same as select area
		m_bSelectionInvalidated = true;
		return false;
	}

	if (m_selDrag.GetChanStart() >= Channels || m_selDrag.GetChanEnd() < 0) {		// // //
		// Completely outside of visible area
		CancelSelection();
		return false;
	}

	if (m_selDrag.m_cpStart.m_iChannel < 0/* || m_selDrag.m_cpStart.m_iRow < 0*/) {

		// Clip if selection is less than zero as this is not handled by the paste routine
		int ChannelOffset = (m_selDrag.m_cpStart.m_iChannel < 0) ? -m_selDrag.m_cpStart.m_iChannel : 0;
		int RowOffset = (m_selDrag.m_cpStart.m_iRow < 0) ? -m_selDrag.m_cpStart.m_iRow : 0;

		int NewChannels = pClipData->ClipInfo.Channels - ChannelOffset;
		int NewRows = pClipData->ClipInfo.Rows - RowOffset;

		CPatternClipData *pClipped = new CPatternClipData(NewChannels, NewRows);

		pClipped->ClipInfo = pClipData->ClipInfo;
		pClipped->ClipInfo.Channels = NewChannels;
		pClipped->ClipInfo.Rows = NewRows;

		for (int c = 0; c < NewChannels; ++c) {
			for (int r = 0; r < NewRows; ++r) {
				*pClipped->GetPattern(c, r) = *pClipData->GetPattern(c + ChannelOffset, r + RowOffset);
			}
		}

		if (m_selDrag.m_cpStart.m_iChannel < 0) {
			m_selDrag.m_cpStart.m_iChannel = 0;
			m_selDrag.m_cpStart.m_iColumn = C_NOTE;
		}

		m_selDrag.m_cpStart.m_iRow = std::max(m_selDrag.m_cpStart.m_iRow, 0);

		SAFE_RELEASE(pClipData);
		pClipData = pClipped;
	}

	if (m_selDrag.m_cpEnd.m_iChannel > Channels - 1) {
		m_selDrag.m_cpEnd.m_iChannel = Channels - 1;
		m_selDrag.m_cpEnd.m_iColumn = GetChannelColumns(Channels);
	}

	// // //
	m_selDrag.m_cpEnd.m_iColumn = std::min(m_selDrag.m_cpEnd.m_iColumn, C_EFF4_PARAM2);	// TODO remove hardcoded number

	// Paste

	bool bDelete = !bCopy;
	bool bMix = IsShiftPressed();

	m_bSelecting = true;

	CPatternAction *pAction = new CPatternAction(CPatternAction::ACT_DRAG_AND_DROP);
	pAction->SetDragAndDrop(pClipData, bDelete, bMix, &m_selDrag);
	GetMainFrame()->AddAction(pAction);

	return true;
}

void CPatternEditor::UpdateDrag(const CPoint &point)
{
	CCursorPos PointPos = GetCursorAtPoint(point);

	cursor_column_t ColumnStart = m_iDragStartCol;
	cursor_column_t ColumnEnd = m_iDragEndCol;

	if (m_iDragChannels == 0 && GetSelectColumn(m_iDragStartCol) >= COLUMN_EFF1) {
		// Allow dragging between effect columns in the same channel
		if (GetSelectColumn(PointPos.m_iColumn) >= COLUMN_EFF1) {
			ColumnStart = static_cast<cursor_column_t>(PointPos.m_iColumn - (PointPos.m_iColumn - 1) % 3);
		}
		else {
			ColumnStart = C_EFF1_NUM;
		}
		ColumnEnd = static_cast<cursor_column_t>(ColumnStart + (m_iDragEndCol - m_iDragStartCol));
	}

	CPatternIterator cpBegin(m_pDocument, GetSelectedTrack(), CCursorPos(PointPos.m_iRow - m_iDragOffsetRow,		// // //
		PointPos.m_iChannel - m_iDragOffsetChannel, ColumnStart, PointPos.m_iFrame));
	CPatternIterator cpEnd = cpBegin;
	cpEnd += GetSelectionSize() - 1;
	cpEnd.m_iChannel += m_iDragChannels;
	cpEnd.m_iColumn = ColumnEnd;
	m_selDrag.m_cpStart = static_cast<CCursorPos>(cpBegin);
	m_selDrag.m_cpEnd = static_cast<CCursorPos>(cpEnd);

	AutoScroll(point, 0);
}

bool CPatternEditor::IsShiftPressed()
{
	return (::GetKeyState(VK_SHIFT) & 0x80) == 0x80;
}

bool CPatternEditor::IsControlPressed()
{
	return (::GetKeyState(VK_CONTROL) & 0x80) == 0x80;
}

CMainFrame *CPatternEditor::GetMainFrame() const
{
	return static_cast<CMainFrame*>(m_pView->GetParentFrame());
}
