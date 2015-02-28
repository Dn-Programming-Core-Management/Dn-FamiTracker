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

#include <algorithm>
#include <cmath>
#include "stdafx.h"
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
#include "APU/APU.h"
#include "APU/Noise.h"		// // //
#include "APU/DPCM.h"		// // //

/*
 * CPatternEditor
 * This is the pattern editor. This class is not derived from any MFC class.
 *
 */

// Define pattern layout here

const int CPatternEditor::HEADER_HEIGHT		 = 36;
const int CPatternEditor::HEADER_CHAN_START	 = 0;
const int CPatternEditor::HEADER_CHAN_HEIGHT = 36;
const int CPatternEditor::ROW_COLUMN_WIDTH	 = 32;
const int CPatternEditor::ROW_HEIGHT		 = 12;

// Pattern header font
LPCTSTR CPatternEditor::DEFAULT_HEADER_FONT = _T("Tahoma");

const int CPatternEditor::DEFAULT_FONT_SIZE			= 12;
const int CPatternEditor::DEFAULT_HEADER_FONT_SIZE	= 11;

// Channel layout
static const int COLUMN_SPACING = 4; // 0CC: change
static const int CHAR_WIDTH		= 10;

static const unsigned int COLUMN_SPACE[] = {
	CHAR_WIDTH * 3 + COLUMN_SPACING,
	CHAR_WIDTH, CHAR_WIDTH + COLUMN_SPACING, 
	CHAR_WIDTH + COLUMN_SPACING,  
	CHAR_WIDTH, CHAR_WIDTH, CHAR_WIDTH + COLUMN_SPACING,
	CHAR_WIDTH, CHAR_WIDTH, CHAR_WIDTH + COLUMN_SPACING,
	CHAR_WIDTH, CHAR_WIDTH, CHAR_WIDTH + COLUMN_SPACING,
	CHAR_WIDTH, CHAR_WIDTH, CHAR_WIDTH + COLUMN_SPACING
};

static const unsigned int COLUMN_WIDTH[] = {
	CHAR_WIDTH * 3,
	CHAR_WIDTH, CHAR_WIDTH, 
	CHAR_WIDTH,
	CHAR_WIDTH, CHAR_WIDTH, CHAR_WIDTH,
	CHAR_WIDTH, CHAR_WIDTH, CHAR_WIDTH,
	CHAR_WIDTH, CHAR_WIDTH, CHAR_WIDTH,
	CHAR_WIDTH, CHAR_WIDTH, CHAR_WIDTH
};

static const unsigned int SELECT_WIDTH[] = {
	CHAR_WIDTH * 3 + COLUMN_SPACING,
	CHAR_WIDTH + COLUMN_SPACING, CHAR_WIDTH,
	CHAR_WIDTH + COLUMN_SPACING,  
	CHAR_WIDTH + COLUMN_SPACING, CHAR_WIDTH, CHAR_WIDTH,
	CHAR_WIDTH + COLUMN_SPACING, CHAR_WIDTH, CHAR_WIDTH,
	CHAR_WIDTH + COLUMN_SPACING, CHAR_WIDTH, CHAR_WIDTH,
	CHAR_WIDTH + COLUMN_SPACING, CHAR_WIDTH, CHAR_WIDTH
};

const int CPatternEditor::CHANNEL_WIDTH = CHAR_WIDTH * 9 + COLUMN_SPACING * 4 - 1;		// // //

// CPatternEditor

CPatternEditor::CPatternEditor() :
	// Pointers
	m_pDocument(NULL),
	m_pView(NULL),
	m_pPatternDC(NULL),
	m_pHeaderDC(NULL),
	m_pPatternBmp(NULL),
	m_pHeaderBmp(NULL),
	// Drawing
	m_iWinWidth(0),
	m_iWinHeight(0),
	m_bPatternInvalidated(false),
	m_bCursorInvalidated(false),
	m_bBackgroundInvalidated(false),
	m_bHeaderInvalidated(false),
	m_bSelectionInvalidated(false),
	m_iCenterRow(0),
	m_iCurrentFrame(0),
	m_iPatternLength(0),
	m_iPrevPatternLength(0),
	m_iNextPatternLength(0),
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
	m_iPatternFontSize(ROW_HEIGHT),
	m_iDrawCursorRow(0),
	m_iDrawFrame(0),
	m_bFollowMode(true),
	m_bHasFocus(false),
	m_iHighlight(CFamiTrackerDoc::DEFAULT_FIRST_HIGHLIGHT),
	m_iHighlightSecond(CFamiTrackerDoc::DEFAULT_SECOND_HIGHLIGHT),
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
	m_iDragStartCol(0),
	m_iDragEndCol(0),
	m_iDragOffsetChannel(0),
	m_iDragOffsetRow(0),
	m_nScrollFlags(0),
	m_iScrolling(SCROLL_NONE),
	m_iCurrentHScrollPos(0),
	// Benchmarking
	m_iRedraws(0),
	m_iFullRedraws(0),
	m_iQuickRedraws(0),
	m_iHeaderRedraws(0),
	m_iPaints(0),
	m_iErases(0),
	m_iBuffers(0),
	m_iCharsDrawn(0),
	m_bCompactMode(false)		// // //
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
	SAFE_RELEASE(m_pHeaderDC);
	SAFE_RELEASE(m_pHeaderBmp);
}

// Drawing

void CPatternEditor::ApplyColorScheme()
{
	// The color scheme has changed
	//

	const CSettings *pSettings = theApp.GetSettings();

	LOGFONT LogFont;
	LPCTSTR	FontName = pSettings->Appearance.strFont;		// // //
	LPCTSTR	HeaderFace = DEFAULT_HEADER_FONT;

	COLORREF ColBackground = pSettings->Appearance.iColBackground;

	// Fetch font size
	m_iPatternFontSize = pSettings->Appearance.iFontSize;		// // //
	m_iRowHeight = m_iPatternFontSize;

	CalcLayout();

	// Create pattern font
	memset(&LogFont, 0, sizeof LOGFONT);
	memcpy(LogFont.lfFaceName, FontName, _tcslen(FontName));

	LogFont.lfHeight = -m_iPatternFontSize;
//	LogFont.lfHeight = -MulDiv(12, _dpiY, 96);
	LogFont.lfQuality = DRAFT_QUALITY;
	LogFont.lfPitchAndFamily = DEFAULT_PITCH | FF_DONTCARE;

	// Remove old font
	if (m_fontPattern.m_hObject != NULL)
		m_fontPattern.DeleteObject();

	m_fontPattern.CreateFontIndirect(&LogFont);

	// Create header font
	memset(&LogFont, 0, sizeof LOGFONT);
	memcpy(LogFont.lfFaceName, HeaderFace, _tcslen(HeaderFace));

	LogFont.lfHeight = -DEFAULT_HEADER_FONT_SIZE;
	//LogFont.lfWeight = 550;
	LogFont.lfPitchAndFamily = VARIABLE_PITCH | FF_SWISS;

	// Remove old font
	if (m_fontHeader.m_hObject != NULL)
		m_fontHeader.DeleteObject();

	m_fontHeader.CreateFontIndirect(&LogFont);

	if (m_fontCourierNew.m_hObject == NULL)		// // // smaller
		m_fontCourierNew.CreateFont(14, 0, 0, 0, 0, FALSE, FALSE, FALSE, 0, 0, 0, DRAFT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, _T("Courier New"));

	// Cache some colors
	m_colSeparator	= BLEND(ColBackground, (ColBackground ^ 0xFFFFFF), SHADE_LEVEL.SEPARATOR);
	m_colEmptyBg	= DIM(theApp.GetSettings()->Appearance.iColBackground, SHADE_LEVEL.EMPTY_BG);

	m_colHead1 = GetSysColor(COLOR_3DFACE);
	m_colHead2 = GetSysColor(COLOR_BTNHIGHLIGHT);
	m_colHead3 = GetSysColor(COLOR_APPWORKSPACE);
	m_colHead4 = BLEND(m_colHead3, 0x4040F0, 80);

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
	m_iCurrentFrame = 0;
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
	m_iPatternLength = GetCurrentPatternLength(m_iCurrentFrame);

	if (theApp.GetSettings()->General.bFramePreview) {
		m_iPrevPatternLength = GetCurrentPatternLength(m_iCurrentFrame - 1);
		m_iNextPatternLength = GetCurrentPatternLength(m_iCurrentFrame + 1);
	}
	else
		m_iPrevPatternLength = m_iNextPatternLength = m_iPatternLength;
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

	// Adjust selection
	if (m_bSelecting) {
		if (m_selection.GetRowStart() > m_iPatternLength - 1) {
			CancelSelection();
		}
		else if (m_selection.GetRowEnd() >= m_iPatternLength) {
			SetSelectionEnd(CCursorPos(m_iPatternLength - 1, m_selection.GetChanEnd(), m_selection.GetColEnd()));
		}
	}

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
	if (m_iLastFrame != m_iCurrentFrame) {
		UpdatePatternLength();
		bDrawPattern = true;
		bQuickRedraw = false;
	}

	if (m_iLastPlayRow != m_iPlayRow) {
		if (theApp.IsPlaying() && !m_bFollowMode) {
			if (abs(m_iPlayFrame - m_iCurrentFrame) < 2) {
				bDrawPattern = true;
				bQuickRedraw = false;
			}
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
	m_iLastFrame = m_iCurrentFrame;
	m_iLastFirstChannel = m_iFirstChannel;
	m_iLastPlayRow = m_iPlayRow;

	//
	// Draw pattern header, when needed
	//

	if (m_bHeaderInvalidated) {
		// Pattern header
		DrawHeader(m_pHeaderDC);
		DrawMeters(m_pHeaderDC);
		++m_iHeaderRedraws;
	}

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
	const int iBlitWidth = m_iPatternWidth + ROW_COLUMN_WIDTH;

//	if (iBlitWidth > m_iWinWidth)
//		iBlitWidth = m_iWinWidth;

	// Pattern area
	if (pDC->RectVisible(GetPatternRect()))
		pDC->BitBlt(0, HEADER_HEIGHT, iBlitWidth, iBlitHeight, m_pPatternDC, 0, 0, SRCCOPY);

	// Header area
	if (pDC->RectVisible(GetHeaderRect()))
		pDC->BitBlt(0, 0, iBlitWidth, HEADER_HEIGHT, m_pHeaderDC, 0, 0, SRCCOPY);

	// Background
	if (pDC->RectVisible(GetUnbufferedRect()))
		DrawUnbufferedArea(pDC);

	int Line = 1;

#ifdef _DEBUG
	pDC->SetBkColor(DEFAULT_COLOR_SCHEME.CURSOR);
	pDC->SetTextColor(DEFAULT_COLOR_SCHEME.TEXT_HILITE);
	pDC->TextOut(m_iWinWidth - 70, 42, _T("DEBUG"));
	pDC->TextOut(m_iWinWidth - 70, 62, _T("DEBUG"));
	pDC->TextOut(m_iWinWidth - 70, 82, _T("DEBUG"));
#else 
#ifndef RELEASE_BUILD
	//pDC->SetBkColor(DEFAULT_COLOR_SCHEME.CURSOR);
	//pDC->SetTextColor(DEFAULT_COLOR_SCHEME.TEXT_HILITE);
	//pDC->TextOut(m_iWinWidth - 110, m_iWinHeight - 20 * Line++, _T("Release build"));
#endif
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
	int offset = m_iWinWidth - 160;
	Text.Format(_T("BETA %i (%s)"), VERSION_WIP, __DATE__);
	pDC->SetTextColor(0x00FFFF);
	pDC->SetBkMode(TRANSPARENT);
	pDC->TextOut(offset, m_iWinHeight - 24 - 18 * line++, Text);

#ifndef RELEASE_BUILD
	Text.Format(_T("Dev build"));
	pDC->TextOut(offset, m_iWinHeight - 24 - 18 * line++, Text);
#endif

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
	return CRect(0, 0, m_iPatternWidth + ROW_COLUMN_WIDTH, m_iWinHeight);
}

CRect CPatternEditor::GetHeaderRect() const
{
	return CRect(0, 0, m_iPatternWidth + ROW_COLUMN_WIDTH, HEADER_HEIGHT); 
}

CRect CPatternEditor::GetPatternRect() const
{
	// Return the rect with pattern and header only
	return CRect(0, HEADER_HEIGHT, m_iPatternWidth + ROW_COLUMN_WIDTH, m_iWinHeight);
}

CRect CPatternEditor::GetUnbufferedRect() const
{
	return CRect(m_iPatternWidth + ROW_COLUMN_WIDTH, 0, m_iWinWidth, m_iWinHeight);
}

CRect CPatternEditor::GetInvalidatedRect() const
{
	if (m_bHeaderInvalidated)
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
		if (m_bCompactMode) Width = (3 * CHAR_WIDTH + COLUMN_SPACING);
		else Width = CHAR_WIDTH * 9 + COLUMN_SPACING * 4 + m_pDocument->GetEffColumns(Track, i) * (3 * CHAR_WIDTH + COLUMN_SPACING);
		m_iChannelWidths[i] = Width + 1;
		m_iColumns[i] = m_bCompactMode ? 1 : GetChannelColumns(i);		// // //
		m_iChannelOffsets[i] = Offset;
		Offset += m_iChannelWidths[i];
	}

	// Calculate pattern width and height
	bool HiddenChannels = false;
	int LastChannel = ChannelCount;

	m_iPatternWidth = 0;
	for (int i = m_iFirstChannel; i < ChannelCount; ++i) {
		m_iPatternWidth += m_iChannelWidths[i];
		if ((m_iPatternWidth + ROW_COLUMN_WIDTH) >= WinWidth) {
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

	const int Track = GetSelectedTrack();
	const int Frames = m_pDocument->GetFrameCount(Track);
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
	if (m_iCurrentFrame >= Frames)
		m_iCurrentFrame = Frames - 1;

	// Ignore user cursor moves if the player is playing
	if (theApp.IsPlaying()) {

		const CSoundGen *pSoundGen = theApp.GetSoundGenerator();
		// Store a synchronized copy of frame & row position from player
		m_iPlayFrame = pSoundGen->GetPlayerFrame();
		m_iPlayRow = pSoundGen->GetPlayerRow();
		
		if (m_bFollowMode) {
			m_cpCursorPos.m_iRow = m_iPlayRow;
			m_iCurrentFrame = m_iPlayFrame;
		}

		SetSelectionStart();
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
	if (bCreateBuffers) {

		// Allocate backbuffer
		SAFE_RELEASE(m_pPatternBmp);
		SAFE_RELEASE(m_pHeaderBmp);
		SAFE_RELEASE(m_pPatternDC);
		SAFE_RELEASE(m_pHeaderDC);

		m_pPatternBmp = new CBitmap;
		m_pHeaderBmp = new CBitmap;
		m_pPatternDC = new CDC;
		m_pHeaderDC = new CDC;

		int Width  = ROW_COLUMN_WIDTH + m_iPatternWidth;
		int Height = m_iPatternHeight;

		// Setup pattern dc
		m_pPatternBmp->CreateCompatibleBitmap(pDC, Width, Height);
		m_pPatternDC->CreateCompatibleDC(pDC);
		m_pPatternDC->SelectObject(m_pPatternBmp);

		// Setup header dc
		m_pHeaderBmp->CreateCompatibleBitmap(pDC, Width, HEADER_HEIGHT);
		m_pHeaderDC->CreateCompatibleDC(pDC);
		m_pHeaderDC->SelectObject(m_pHeaderBmp);

		++m_iBuffers;
	}

	++m_iErases;
}

void CPatternEditor::DrawUnbufferedArea(CDC *pDC)
{
	// This part of the surface doesn't contain anything useful

	if (m_iPatternWidth < m_iWinWidth) {
		int Width = m_iWinWidth - m_iPatternWidth - ROW_COLUMN_WIDTH;
		if (m_iPatternLength > 1)
			Width -= ::GetSystemMetrics(SM_CXVSCROLL);

		// Channel header background
		GradientRectTriple(pDC, m_iPatternWidth + ROW_COLUMN_WIDTH, HEADER_CHAN_START, Width, HEADER_HEIGHT, m_colHead1, m_colHead2, m_pView->GetEditMode() ? m_colHead4 : m_colHead3);
		pDC->Draw3dRect(m_iPatternWidth + ROW_COLUMN_WIDTH, HEADER_CHAN_START, Width, HEADER_HEIGHT, STATIC_COLOR_SCHEME.FRAME_LIGHT, STATIC_COLOR_SCHEME.FRAME_DARK);

		// The big empty area
		pDC->FillSolidRect(m_iPatternWidth + ROW_COLUMN_WIDTH, HEADER_HEIGHT, Width, m_iWinHeight - HEADER_HEIGHT, m_colEmptyBg);	
	}
}

void CPatternEditor::PerformFullRedraw(CDC *pDC)
{
	// Draw entire pattern area
	
	const int Track = GetSelectedTrack();
	const int Channels = GetChannelCount();
	const int FrameCount = m_pDocument->GetFrameCount(Track);
	int Row = m_iCenterRow - m_iLinesVisible / 2;

	CFont *pOldFont = pDC->SelectObject(&m_fontPattern);

	pDC->SetBkMode(TRANSPARENT);

	for (int i = 0; i < m_iLinesVisible; ++i) {
		PrintRow(pDC, Row, i, m_iCurrentFrame);
		++Row;
	}

	// Last unvisible row
	ClearRow(pDC, m_iLinesVisible);

	pDC->SetWindowOrg(-ROW_COLUMN_WIDTH, 0);

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
	ASSERT(m_iCurrentFrame == m_iLastFrame);

	// Number of rows that has changed
	const int DiffRows = m_iCenterRow - m_iLastCenterRow;

	CFont *pOldFont = pDC->SelectObject(&m_fontPattern);

	ScrollPatternArea(pDC, DiffRows);

	// Play cursor
	if (theApp.IsPlaying() && !m_bFollowMode) {
		//PrintRow(pDC, m_iPlayRow, 
	}
	else if (!theApp.IsPlaying() && m_iLastPlayRow != -1) {
		if (m_iPlayFrame == m_iCurrentFrame) {
			int Line = RowToLine(m_iLastPlayRow);
			if (Line >= 0 && Line <= m_iLinesVisible) {
				// Erase 
				PrintRow(pDC, m_iLastPlayRow, Line, m_iCurrentFrame);
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
	const int Track = GetSelectedTrack();
	const int FrameCount = m_pDocument->GetFrameCount(Track);

	// // // preview more than just two frames
	if (Row >= 0 && Row < m_iPatternLength) {
		DrawRow(pDC, Row, Line, Frame, false);
	}
	else if (theApp.GetSettings()->General.bFramePreview) {
		while (Row >= GetCurrentPatternLength(Frame)) {		// // //
			Row -= GetCurrentPatternLength(Frame);
			Frame++;
			if (Frame >= FrameCount) {
				Frame = 0;
				// if (Row) Row--; else { ClearRow(pDC, Line); return; }
			}
		}
		while (Row < 0) {		// // //
			Frame--;
			if (Frame < 0) {
				Frame = FrameCount - 1;
				// if (Row != -1) Row++; else { ClearRow(pDC, Line); return; }
			}
			Row += GetCurrentPatternLength(Frame);
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
	const int Width = ROW_COLUMN_WIDTH + m_iPatternWidth - 1;
	const int SrcY = FromRow * m_iRowHeight;
	const int DestY = ToRow * m_iRowHeight;
	const int Height = NumRows * m_iRowHeight;
	pDC->BitBlt(1, DestY, Width, Height, pDC, 1, SrcY, SRCCOPY); 
}

void CPatternEditor::ScrollPatternArea(CDC *pDC, int Rows) const
{
	ASSERT(Rows < (m_iLinesVisible / 2));

	pDC->SetBkMode(TRANSPARENT);

	const int Track = GetSelectedTrack();
	const int FrameCount = m_pDocument->GetFrameCount(Track);
	const int MiddleLine = m_iLinesVisible / 2;

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
			PrintRow(pDC, Row, Line, m_iCurrentFrame);
		}
		// Bottom of screen
		for (int i = 0; i < Rows; ++i) {
			int Row = m_iDrawCursorRow + SecondLineCount - i;
			int Line = m_iLinesVisible - 1 - i;
			PrintRow(pDC, Row, Line, m_iCurrentFrame);
		}
	}
	else if (Rows < 0) {
		// Top of screen
		for (int i = 0; i < -Rows; ++i) {
			int Row = m_iDrawCursorRow - FirstLineCount + i;
			int Line = i;
			PrintRow(pDC, Row, Line, m_iCurrentFrame);
		}
		// Below cursor
		for (int i = 0; i < -Rows; ++i) {
			int Row = m_iDrawCursorRow + 1 + i;
			int Line = MiddleLine + 1 + i;
			PrintRow(pDC, Row, Line, m_iCurrentFrame);
		}
	}

	// Draw cursor line, draw separately to allow calling this with zero rows
	const int Row = m_iDrawCursorRow;
	PrintRow(pDC, Row, MiddleLine, m_iCurrentFrame);
}

void CPatternEditor::ClearRow(CDC *pDC, int Line) const
{
	pDC->SetWindowOrg(0, 0);	

	int Offset = ROW_COLUMN_WIDTH;
	for (int i = m_iFirstChannel; i < m_iFirstChannel + m_iChannelsVisible; ++i) {
		pDC->FillSolidRect(Offset, Line * m_iRowHeight, m_iChannelWidths[i] - 1, m_iRowHeight, m_colEmptyBg);
		Offset += m_iChannelWidths[i];
	}

	// Row number
	pDC->FillSolidRect(1, Line * m_iRowHeight, ROW_COLUMN_WIDTH - 2, m_iRowHeight, m_colEmptyBg);
}

static bool IsTopEdge(const CSelection &sel, int Channel, int Row, int Column)
{
	return Row == sel.GetRowStart();
}

static bool IsBottomEdge(const CSelection &sel, int Channel, int Row, int Column)
{
	return Row == sel.GetRowEnd();
}

static bool IsLeftEdge(const CSelection &sel, int Channel, int Row, int Column)
{
	return Channel == sel.GetChanStart() && Column == sel.GetColStart();
}

static bool IsRightEdge(const CSelection &sel, int Channel, int Row, int Column)
{
	return Channel == sel.GetChanEnd() && Column == sel.GetColEnd();
}

static bool IsInRange(const CSelection &sel, int Channel, int Row, int Column)
{
	// Return true if cursor is in range of selection
	if (Row >= sel.GetRowStart() && Row <= sel.GetRowEnd()) {
		if (Channel == sel.GetChanStart() && Channel == sel.GetChanEnd()) {
			return (Column >= sel.GetColStart() && Column <= sel.GetColEnd());
		}
		else if (Channel == sel.GetChanStart() && Channel != sel.GetChanEnd()) {
			return (Column >= sel.GetColStart());
		}
		else if (Channel == sel.GetChanEnd() && Channel != sel.GetChanStart()) {
			return (Column <= sel.GetColEnd());
		}
		else if (Channel >= sel.GetChanStart() && Channel < sel.GetChanEnd()) {
			return true;
		}
	}
	return false;
}

// Draw a single row
void CPatternEditor::DrawRow(CDC *pDC, int Row, int Line, int Frame, bool bPreview) const
{
	// Row is row from pattern to display
	// Line is (absolute) screen line

	const COLORREF RED_BAR_COLOR  = 0x302080;
	const COLORREF BLUE_BAR_COLOR = 0xA02030;
	const COLORREF GRAY_BAR_COLOR = 0x606060;
	const COLORREF SEL_DRAG_COL	  = 0xA08080;

	const unsigned int PREVIEW_SHADE_LEVEL = 70;

	const CSettings *pSettings = theApp.GetSettings();

	COLORREF ColCursor	= theApp.GetSettings()->Appearance.iColCursor;
	COLORREF ColBg		= theApp.GetSettings()->Appearance.iColBackground;
	COLORREF ColHiBg	= theApp.GetSettings()->Appearance.iColBackgroundHilite;
	COLORREF ColHiBg2	= theApp.GetSettings()->Appearance.iColBackgroundHilite2;
	COLORREF ColSelect	= theApp.GetSettings()->Appearance.iColSelection;

	const bool bEditMode = m_pView->GetEditMode();

	const int Track = GetSelectedTrack();
	const int Channels = /*m_iFirstChannel +*/ m_iChannelsVisible;
	int OffsetX = ROW_COLUMN_WIDTH;

	stChanNote NoteData;

	// Start at row number column
	pDC->SetWindowOrg(0, 0);

	if (Frame != m_iCurrentFrame && !theApp.GetSettings()->General.bFramePreview) {
		ClearRow(pDC, Line);
		return;
	}

	// Highlight
	bool bHighlight		  = (m_iHighlight > 0) ? !(Row % m_iHighlight) : false;
	bool bSecondHighlight = (m_iHighlightSecond > 0) ? !(Row % m_iHighlightSecond) : false;

	// Clear
	pDC->FillSolidRect(1, Line * m_iRowHeight, ROW_COLUMN_WIDTH - 2, m_iRowHeight, ColBg);

	COLORREF TextColor;

	if (bSecondHighlight)
		TextColor = theApp.GetSettings()->Appearance.iColPatternTextHilite2;
	else if (bHighlight)
		TextColor = theApp.GetSettings()->Appearance.iColPatternTextHilite;
	else
		TextColor = theApp.GetSettings()->Appearance.iColPatternText;

	if (bPreview) {
		ColHiBg2 = DIM(ColHiBg2, PREVIEW_SHADE_LEVEL);
		ColHiBg = DIM(ColHiBg, PREVIEW_SHADE_LEVEL);
		ColBg = DIM(ColBg, PREVIEW_SHADE_LEVEL);
		TextColor = DIM(TextColor, 70);
	}

	// Draw row number
	pDC->SetTextColor(TextColor);
	pDC->SetTextAlign(TA_CENTER);		// // //

	CString Text;

	if (theApp.GetSettings()->General.bRowInHex) {
		// // // Hex display
		Text.Format(_T("%X"), Row >> 4);
		pDC->TextOut((ROW_COLUMN_WIDTH - CHAR_WIDTH) / 2, Line * m_iRowHeight - 1, Text);
		Text.Format(_T("%X"), Row & 0x0F);
		pDC->TextOut((ROW_COLUMN_WIDTH + CHAR_WIDTH) / 2, Line * m_iRowHeight - 1, Text);
	}
	else {
		// // // Decimal display
		Text.Format(_T("%d"), Row / 100 % 10);
		pDC->TextOut(ROW_COLUMN_WIDTH / 2 - CHAR_WIDTH, Line * m_iRowHeight - 1, Text);
		Text.Format(_T("%d"), Row / 10 % 10);
		pDC->TextOut(ROW_COLUMN_WIDTH / 2, Line * m_iRowHeight - 1, Text);
		Text.Format(_T("%d"), Row % 10);
		pDC->TextOut(ROW_COLUMN_WIDTH / 2 + CHAR_WIDTH, Line * m_iRowHeight - 1, Text);
	}

	pDC->SetTextAlign(TA_LEFT);		// // //

	COLORREF BackColor;
	if (bSecondHighlight)
		BackColor = ColHiBg2;	// Highlighted row
	else if (bHighlight)
		BackColor = ColHiBg;	// Highlighted row
	else
		BackColor = ColBg;		// Normal

	if (!bPreview && Row == m_iDrawCursorRow) {
		// Cursor row
		if (!m_bHasFocus)
			BackColor = BLEND(GRAY_BAR_COLOR, BackColor, SHADE_LEVEL.UNFOCUSED);	// Gray
		else if (bEditMode)
			BackColor = BLEND(RED_BAR_COLOR, BackColor, SHADE_LEVEL.FOCUSED);		// Red
		else
			BackColor = BLEND(BLUE_BAR_COLOR, BackColor, SHADE_LEVEL.FOCUSED);		// Blue
	}

	const COLORREF SelectColor = BLEND(ColSelect, BackColor, SHADE_LEVEL.SELECT);
	const COLORREF DragColor = BLEND(SEL_DRAG_COL, BackColor, SHADE_LEVEL.SELECT);
	const COLORREF SelectEdgeCol = BLEND(SelectColor, 0xFFFFFF, SHADE_LEVEL.SELECT_EDGE);

	RowColorInfo_t colorInfo;

	colorInfo.Note = TextColor;

	if (bSecondHighlight)
		colorInfo.Back = pSettings->Appearance.iColBackgroundHilite2;
	else if (bHighlight)
		colorInfo.Back = pSettings->Appearance.iColBackgroundHilite;
	else
		colorInfo.Back = pSettings->Appearance.iColBackground;

	colorInfo.Shaded = BLEND(TextColor, colorInfo.Back, SHADE_LEVEL.UNUSED);

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
	}

	// Draw channels
	for (int i = m_iFirstChannel; i < m_iFirstChannel + m_iChannelsVisible; ++i) {

		m_pDocument->GetNoteData(Track, Frame, i, Row, &NoteData);

		pDC->SetWindowOrg(-OffsetX, - (signed)Line * m_iRowHeight);

		int PosX	 = COLUMN_SPACING;
		int SelStart = COLUMN_SPACING;
		int Columns	 = m_bCompactMode ? 1 : GetChannelColumns(i);		// // //
		int Width	 = m_iChannelWidths[i] - 1;		// Remove 1, spacing between channels

		if (BackColor == ColBg)
			pDC->FillSolidRect(0, 0, Width, m_iRowHeight, BackColor);
		else
			GradientBar(pDC, 0, 0, Width, m_iRowHeight, BackColor, ColBg);

		if (!m_bFollowMode && Row == m_iPlayRow && Frame == m_iPlayFrame && theApp.IsPlaying()) {
			// Play row
			GradientBar(pDC, 0, 0, Width, m_iRowHeight, ROW_PLAY_COLOR, ColBg);
		}

		// Draw each column
		for (int j = 0; j < Columns; ++j) {

			// Selection
			if (m_bSelecting && !bPreview) {
				if (IsInRange(m_selection, i, Row, j)) {
					pDC->FillSolidRect(SelStart - COLUMN_SPACING, 0, SELECT_WIDTH[j], m_iRowHeight, SelectColor);

					// Outline
					if (IsTopEdge(m_selection, i, Row, j))
						pDC->FillSolidRect(SelStart - COLUMN_SPACING, 0, SELECT_WIDTH[j], 1, SelectEdgeCol);
					if (IsBottomEdge(m_selection, i, Row, j))
						pDC->FillSolidRect(SelStart - COLUMN_SPACING, m_iRowHeight - 1, SELECT_WIDTH[j], 1, SelectEdgeCol);
					if (IsLeftEdge(m_selection, i, Row, j))
						pDC->FillSolidRect(SelStart - COLUMN_SPACING, 0, 1, m_iRowHeight, SelectEdgeCol);
					if (IsRightEdge(m_selection, i, Row, j))
						pDC->FillSolidRect(SelStart - COLUMN_SPACING + SELECT_WIDTH[j] - 1, 0, 1, m_iRowHeight, SelectEdgeCol);
				}
			}

			// Dragging
			if (m_bDragging && !bPreview) {
				if (IsInRange(m_selDrag, i, Row, j)) {
					pDC->FillSolidRect(SelStart - COLUMN_SPACING, 0, SELECT_WIDTH[j], m_iRowHeight, DragColor);
				}
			}

			bool bInvert = false;

			// Draw cursor box
			if (i == m_cpCursorPos.m_iChannel && j == m_cpCursorPos.m_iColumn && Row == m_iDrawCursorRow && !bPreview) {
				GradientBar(pDC, PosX - COLUMN_SPACING / 2, 0, COLUMN_WIDTH[j], m_iRowHeight, ColCursor, ColBg);		// // //
				pDC->Draw3dRect(PosX - COLUMN_SPACING / 2, 0, COLUMN_WIDTH[j], m_iRowHeight, ColCursor, DIM(ColCursor, 50));
				bInvert = true;
			}

			DrawCell(pDC, PosX - COLUMN_SPACING / 2, j, i, bInvert, &NoteData, &colorInfo);		// // //
			PosX += COLUMN_SPACE[j];
			SelStart += SELECT_WIDTH[j];
			
		}

		OffsetX += m_iChannelWidths[i];
	}
}

void CPatternEditor::DrawCell(CDC *pDC, int PosX, int Column, int Channel, bool bInvert, stChanNote *pNoteData, RowColorInfo_t *pColorInfo) const
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

	int EffNumber = Column >= 4 ? pNoteData->EffNumber[(Column - 4) / 3] : 0;		// // //
	int EffParam  = Column >= 4 ? pNoteData->EffParam[(Column - 4) / 3] : 0;

	// Detect invalid note data
	if (pNoteData->Note > ECHO ||		// // //
		pNoteData->Octave > 8 ||
		EffNumber >= EF_COUNT || 
		pNoteData->Instrument > MAX_INSTRUMENTS) {
		if (Column == 0/* || Column == 4*/) {
			CString Text;
			Text.Format(_T("(invalid)"));
			pDC->SetTextColor(RED(255));
			pDC->TextOut(PosX, -1, Text);
		}
		return;
	}

	COLORREF InstColor = pColorInfo->Instrument;
	COLORREF EffColor = pColorInfo->Effect;
	COLORREF DimInst = pColorInfo->Shaded;		// // //
	COLORREF DimEff = pColorInfo->Shaded;		// // //

	// Make non-available instruments red in the pattern editor
	if (pNoteData->Instrument < MAX_INSTRUMENTS && 
		(!m_pDocument->IsInstrumentUsed(pNoteData->Instrument) || !pTrackerChannel->IsInstrumentCompatible(pNoteData->Instrument, m_pDocument))) {
		DimInst = InstColor = RED(255);
	}

	// // // effects too
	if (!pTrackerChannel->IsEffectCompatible(EffNumber, EffParam)) {
		DimEff = EffColor = RED(255);		// // //
	}

	int PosY = -2;
	// // // PosX -= 1;

#define BAR(x, y) pDC->FillSolidRect((x) + CHAR_WIDTH / 2 - 2, (y) + (m_iRowHeight / 2) + 2, 4, 1, pColorInfo->Shaded) // // //
	
	pDC->SetTextAlign(TA_CENTER);		// // //

	switch (Column) {
		case 0:
			// Note and octave
			switch (pNoteData->Note) {
				case NONE:
					if (m_bCompactMode) {		// // //
						if (pNoteData->Instrument != MAX_INSTRUMENTS) {
							DrawChar(pDC, PosX + CHAR_WIDTH * 3 / 2, PosY, HEX[pNoteData->Instrument >> 4], DimInst);
							DrawChar(pDC, PosX + CHAR_WIDTH * 5 / 2, PosY, HEX[pNoteData->Instrument & 0x0F], DimInst);
							break;
						}
						else if (pNoteData->Vol != MAX_VOLUME) {
							DrawChar(pDC, PosX + CHAR_WIDTH * 5 / 2, PosY, HEX[pNoteData->Vol], pColorInfo->Shaded);
							break;
						}
						else {
							bool Found = false;
							for (unsigned int i = 0; i <= m_pDocument->GetEffColumns(GetSelectedTrack(), Channel); i++) {
								if (pNoteData->EffNumber[i] != EF_NONE) {
									DrawChar(pDC, PosX + CHAR_WIDTH / 2, PosY, EFF_CHAR[pNoteData->EffNumber[i] - 1], DimEff);
									DrawChar(pDC, PosX + CHAR_WIDTH * 3 / 2, PosY, HEX[pNoteData->EffParam[i] >> 4], DimEff);
									DrawChar(pDC, PosX + CHAR_WIDTH * 5 / 2, PosY, HEX[pNoteData->EffParam[i] & 0x0F], DimEff);
									Found = true;
									break;
								}
							}
							if (Found) break;
						}
						BAR(PosX, PosY);
						BAR(PosX + CHAR_WIDTH, PosY);
						BAR(PosX + CHAR_WIDTH * 2, PosY);
					}
					else {
						BAR(PosX, PosY);
						BAR(PosX + CHAR_WIDTH, PosY);
						BAR(PosX + CHAR_WIDTH * 2, PosY);
					}
					break;		// // // same below
				case HALT:
					// Note stop
					GradientBar(pDC, PosX + 5, (m_iRowHeight / 2) - 2, CHAR_WIDTH * 3 - 11, m_iRowHeight / 4, pColorInfo->Note, pColorInfo->Back);
					break;
				case RELEASE:
					// Note release
					pDC->FillSolidRect(PosX + 5, m_iRowHeight / 2 - 3, CHAR_WIDTH * 3 - 11, 2, pColorInfo->Note);		// // //
					pDC->FillSolidRect(PosX + 5, m_iRowHeight / 2 + 1, CHAR_WIDTH * 3 - 11, 2, pColorInfo->Note);
					break;
				case ECHO:
					// // // Echo buffer access
					DrawChar(pDC, PosX + CHAR_WIDTH, PosY, _T('^'), pColorInfo->Note);
					DrawChar(pDC, PosX + CHAR_WIDTH * 2, PosY, NOTES_C[pNoteData->Octave], pColorInfo->Note);
					break;
				default:
					if (pTrackerChannel->GetID() == CHANID_NOISE) {
						// Noise
						char NoiseFreq = (pNoteData->Note - 1 + pNoteData->Octave * 12) & 0x0F;
						DrawChar(pDC, PosX + CHAR_WIDTH / 2, PosY, HEX[NoiseFreq], pColorInfo->Note);		// // //
						DrawChar(pDC, PosX + CHAR_WIDTH * 3 / 2, PosY, '-', pColorInfo->Note);
						DrawChar(pDC, PosX + CHAR_WIDTH * 5 / 2, PosY, '#', pColorInfo->Note);
					}
					else {
						// The rest
						DrawChar(pDC, PosX + CHAR_WIDTH / 2, PosY, NOTES_A[pNoteData->Note - 1], pColorInfo->Note);		// // //
						DrawChar(pDC, PosX + CHAR_WIDTH * 3 / 2, PosY, NOTES_B[pNoteData->Note - 1], pColorInfo->Note);
						DrawChar(pDC, PosX + CHAR_WIDTH * 5 / 2, PosY, NOTES_C[pNoteData->Octave], pColorInfo->Note);
					}
					break;
			}
			break;
		case 1:
			// Instrument x0
			if (pNoteData->Instrument == MAX_INSTRUMENTS || pNoteData->Note == HALT || pNoteData->Note == RELEASE)
				BAR(PosX, PosY);
			else
				DrawChar(pDC, PosX + CHAR_WIDTH / 2, PosY, HEX[pNoteData->Instrument >> 4], InstColor);		// // //
			break;
		case 2:
			// Instrument 0x
			if (pNoteData->Instrument == MAX_INSTRUMENTS || pNoteData->Note == HALT || pNoteData->Note == RELEASE)
				BAR(PosX, PosY);
			else
				DrawChar(pDC, PosX + CHAR_WIDTH / 2, PosY, HEX[pNoteData->Instrument & 0x0F], InstColor);		// // //
			break;
		case 3: 
			// Volume
			if (pNoteData->Vol == MAX_VOLUME || pTrackerChannel->GetID() == CHANID_DPCM)
				BAR(PosX, PosY);
			else 
				DrawChar(pDC, PosX + CHAR_WIDTH / 2, PosY, HEX[pNoteData->Vol & 0x0F], pColorInfo->Volume);		// // //
			break;
		case 4: case 7: case 10: case 13:
			// Effect type
			if (EffNumber == 0)
				BAR(PosX, PosY);
			else {
				DrawChar(pDC, PosX + CHAR_WIDTH / 2, PosY, EFF_CHAR[EffNumber - 1], EffColor);		// // //
			}
			break;
		case 5: case 8: case 11: case 14:
			// Effect param x
			if (EffNumber == 0)
				BAR(PosX, PosY);
			else
				DrawChar(pDC, PosX + CHAR_WIDTH / 2, PosY, HEX[(EffParam >> 4) & 0x0F], pColorInfo->Note);		// // //
			break;
		case 6: case 9: case 12: case 15:
			// Effect param y
			if (EffNumber == 0)
				BAR(PosX, PosY);
			else
				DrawChar(pDC, PosX + CHAR_WIDTH / 2, PosY, HEX[EffParam & 0x0F], pColorInfo->Note);		// // //
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

	unsigned int Offset = ROW_COLUMN_WIDTH;
	unsigned int Track = GetSelectedTrack();

	CFont *pOldFont = pDC->SelectObject(&m_fontHeader);

	pDC->SetBkMode(TRANSPARENT);

	// Channel header background
	GradientRectTriple(pDC, 0, HEADER_CHAN_START, m_iPatternWidth + ROW_COLUMN_WIDTH, HEADER_CHAN_HEIGHT, m_colHead1, m_colHead2, m_pView->GetEditMode() ? m_colHead4 : m_colHead3);

	// Corner box
	pDC->Draw3dRect(0, HEADER_CHAN_START, ROW_COLUMN_WIDTH, HEADER_CHAN_HEIGHT, STATIC_COLOR_SCHEME.FRAME_LIGHT, STATIC_COLOR_SCHEME.FRAME_DARK);

	for (int i = 0; i < m_iChannelsVisible; ++i) {

		const int Channel = i + m_iFirstChannel;
		const bool bMuted = m_pView->IsChannelMuted(Channel);
		const bool Pushed = bMuted || (m_iChannelPushed == Channel) && m_bChannelPushed;

		// Frame
		if (Pushed) {
			GradientRectTriple(pDC, Offset, HEADER_CHAN_START, m_iChannelWidths[Channel], HEADER_CHAN_HEIGHT, m_colHead1, m_colHead1, m_pView->GetEditMode() ? m_colHead4 : m_colHead3);
			pDC->Draw3dRect(Offset, HEADER_CHAN_START, m_iChannelWidths[Channel], HEADER_CHAN_HEIGHT, BLEND(STATIC_COLOR_SCHEME.FRAME_LIGHT, STATIC_COLOR_SCHEME.FRAME_DARK, 50), STATIC_COLOR_SCHEME.FRAME_DARK); 
		}
		else
			pDC->Draw3dRect(Offset, HEADER_CHAN_START, m_iChannelWidths[Channel], HEADER_CHAN_HEIGHT, STATIC_COLOR_SCHEME.FRAME_LIGHT, STATIC_COLOR_SCHEME.FRAME_DARK);

		// Text
		CTrackerChannel *pChannel = m_pDocument->GetChannel(Channel);
		CString pChanName;		// // //
		if (m_bCompactMode) {
			switch (pChannel->GetID()) {
			case CHANID_SQUARE1: pChanName = _T("PU1"); break;
			case CHANID_SQUARE2: pChanName = _T("PU2"); break;
			case CHANID_TRIANGLE: pChanName = _T("TRI"); break;
			case CHANID_NOISE: pChanName = _T("NOI"); break;
			case CHANID_DPCM: pChanName = _T("DMC"); break;
			default:
				int Type = m_pDocument->GetChannelType(Channel);
				switch (pChannel->GetChip()) {
				case SNDCHIP_VRC6: pChanName.Format(_T("V%d"), Type - CHANID_VRC6_PULSE1 + 1); break;
				case SNDCHIP_MMC5: pChanName.Format(_T("PU%d"), Type - CHANID_MMC5_SQUARE1 + 3); break;
				case SNDCHIP_N163: pChanName.Format(_T("N%d"), Type - CHANID_N163_CHAN1 + 1); break;
				case SNDCHIP_FDS: pChanName = _T("WAV"); break;
				case SNDCHIP_VRC7: pChanName.Format(_T("FM%d"), Type - CHANID_VRC7_CH1 + 1); break;
				case SNDCHIP_S5B: pChanName.Format(_T("5B%d"), Type - CHANID_S5B_CH1 + 1); break;
				}
			}
		}
		else
			pChanName = pChannel->GetChannelName();

		COLORREF HeadTextCol = bMuted ? STATIC_COLOR_SCHEME.CHANNEL_MUTED : STATIC_COLOR_SCHEME.CHANNEL_NORMAL;

		// Shadow
		pDC->SetTextColor(BLEND(HeadTextCol, 0x00FFFFFF, SHADE_LEVEL.TEXT_SHADOW));
		if (m_bCompactMode)		// // //
			pDC->SetTextAlign(TA_CENTER);
		pDC->TextOut(Offset + (m_bCompactMode ? 19 : 11), HEADER_CHAN_START + 6 + (bMuted ? 1 : 0), pChanName);

		if (m_iMouseHoverChan == Channel)
			HeadTextCol = BLEND(HeadTextCol, 0x0000FFFF, SHADE_LEVEL.HOVER);

		// Foreground
		pDC->SetTextColor(HeadTextCol);
		pDC->TextOut(Offset + (m_bCompactMode ? 18 : 10), HEADER_CHAN_START + 5, pChanName);		// // //
		
		if (!m_bCompactMode) {		// // //
			// Effect columns
			pDC->SetTextColor(TEXT_COLOR);
			pDC->SetTextAlign(TA_CENTER);
			if (m_pDocument->GetEffColumns(Track, Channel) > 0)
				pDC->TextOut(Offset + CHANNEL_WIDTH + COLUMN_SPACING + 6, HEADER_CHAN_START + HEADER_CHAN_HEIGHT - 17, _T("fx2"));
			if (m_pDocument->GetEffColumns(Track, Channel) > 1)
				pDC->TextOut(Offset + CHANNEL_WIDTH + COLUMN_SPACING * 2 + CHAR_WIDTH * 3 + 6, HEADER_CHAN_START + HEADER_CHAN_HEIGHT - 17, _T("fx3"));
			if (m_pDocument->GetEffColumns(Track, Channel) > 2)
				pDC->TextOut(Offset + CHANNEL_WIDTH + COLUMN_SPACING * 3 + (CHAR_WIDTH * 2) * 3 + 6, HEADER_CHAN_START + HEADER_CHAN_HEIGHT - 17, _T("fx4"));

			// Arrows for expanding/removing fx columns
			if (m_pDocument->GetEffColumns(Track, Channel) > 0) {
				ArrowPoints[0].SetPoint(Offset + CHAR_WIDTH * 15 / 2 + COLUMN_SPACING * 3 + 2, HEADER_CHAN_START + 6);		// // //
				ArrowPoints[1].SetPoint(Offset + CHAR_WIDTH * 15 / 2 + COLUMN_SPACING * 3 + 2, HEADER_CHAN_START + 6 + 10);
				ArrowPoints[2].SetPoint(Offset + CHAR_WIDTH * 15 / 2 + COLUMN_SPACING * 3 - 3, HEADER_CHAN_START + 6 + 5);

				bool Hover = (m_iMouseHoverChan == Channel) && (m_iMouseHoverEffArrow == 1);
				CObject *pOldBrush = pDC->SelectObject(Hover ? &HoverBrush : &BlackBrush);
				CObject *pOldPen = pDC->SelectObject(Hover ? &HoverPen : &BlackPen);

				pDC->Polygon(ArrowPoints, 3);
				pDC->SelectObject(pOldBrush);
				pDC->SelectObject(pOldPen);
			}

			if (m_pDocument->GetEffColumns(Track, Channel) < (MAX_EFFECT_COLUMNS - 1)) {
				ArrowPoints[0].SetPoint(Offset + CHAR_WIDTH * 17 / 2 + COLUMN_SPACING * 3 - 2, HEADER_CHAN_START + 6);		// // //
				ArrowPoints[1].SetPoint(Offset + CHAR_WIDTH * 17 / 2 + COLUMN_SPACING * 3 - 2, HEADER_CHAN_START + 6 + 10);
				ArrowPoints[2].SetPoint(Offset + CHAR_WIDTH * 17 / 2 + COLUMN_SPACING * 3 + 3, HEADER_CHAN_START + 6 + 5);

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

	const int BAR_TOP	 = 5 + 18 + HEADER_CHAN_START;
	const int BAR_LEFT	 = ROW_COLUMN_WIDTH + (m_bCompactMode ? 2 : 7);
	const int BAR_SIZE	 = m_bCompactMode ? 2 : (CHANNEL_WIDTH - 9) / 16;		// // //
	const int BAR_SPACE	 = 1;
	const int BAR_HEIGHT = 5;

	static COLORREF colors[15];
	static COLORREF colors_dim[15];
	static COLORREF colors_shadow[15];

	// TODO Remove static variables
	static int LastSamplePos, LastDeltaPos;

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
			if (j < level) {
				pDC->FillSolidRect(x + BAR_SIZE - 1, BAR_TOP + 1, 1, BAR_HEIGHT, colors_shadow[j]);
				pDC->FillSolidRect(x + 1, BAR_TOP + BAR_HEIGHT, BAR_SIZE - 1, 1, colors_shadow[j]);
				pDC->FillSolidRect(CRect(x, BAR_TOP, x + (BAR_SIZE - BAR_SPACE), BAR_TOP + BAR_HEIGHT), colors[j]);
				pDC->Draw3dRect(CRect(x, BAR_TOP, x + (BAR_SIZE - BAR_SPACE), BAR_TOP + BAR_HEIGHT), colors[j], colors_dim[j]);
			}
			else {
				pDC->FillSolidRect(x + BAR_SIZE - 1, BAR_TOP + 1, BAR_SPACE, BAR_HEIGHT, COL_DARK_SHADOW);
				pDC->FillSolidRect(x + 1, BAR_TOP + BAR_HEIGHT, BAR_SIZE - 1, 1, COL_DARK_SHADOW);
				pDC->FillSolidRect(CRect(x, BAR_TOP, x + (BAR_SIZE - BAR_SPACE), BAR_TOP + BAR_HEIGHT), COL_DARK);
			}
		}

		Offset += m_iChannelWidths[Channel];
	}

	// DPCM
	if (m_DPCMState.SamplePos != LastSamplePos || m_DPCMState.DeltaCntr != LastDeltaPos) {
		if (theApp.GetMainWnd()->GetMenu()->GetMenuState(ID_TRACKER_DPCM, MF_BYCOMMAND) == MF_CHECKED) {

			pDC->SetBkMode(TRANSPARENT);
			pDC->SetBkColor(DPCM_STATE_COLOR);
			pDC->SetTextColor(DPCM_STATE_COLOR);

			COLORREF iHeadCol1 = GetSysColor(COLOR_3DFACE);
			COLORREF iHeadCol2 = GetSysColor(COLOR_BTNHIGHLIGHT);
			COLORREF iHeadCol3 = GetSysColor(COLOR_APPWORKSPACE);

			if (m_pView->GetEditMode())
				iHeadCol3 = BLEND(iHeadCol3, 0x0000FF, SHADE_LEVEL.EDIT_MODE);

			GradientRectTriple(pDC, Offset + 10, 0, 150, HEADER_CHAN_HEIGHT, iHeadCol1, iHeadCol2, iHeadCol3);

			pDC->FillSolidRect(Offset + 10, 0, 150, 1, STATIC_COLOR_SCHEME.FRAME_LIGHT);
			pDC->FillSolidRect(Offset + 10, HEADER_CHAN_HEIGHT - 1, 150, 1, STATIC_COLOR_SCHEME.FRAME_DARK);

			CString Text;
			Text.Format(_T("Sample position: %02X"), m_DPCMState.SamplePos);
			pDC->TextOut(Offset + 20, 3, Text);

			Text.Format(_T("Delta counter: %02X"), m_DPCMState.DeltaCntr);
			pDC->TextOut(Offset + 20, 17, Text);

			LastSamplePos = m_DPCMState.SamplePos;
			LastDeltaPos = m_DPCMState.DeltaCntr;
		}
	}

#ifdef DRAW_REGS
	DrawRegisters(pDC);
#else
	if (theApp.GetMainWnd()->GetMenu()->GetMenuState(ID_TRACKER_DISPLAYREGISTERSTATE, MF_BYCOMMAND) == MF_CHECKED) {
		DrawRegisters(pDC);
	}
#endif /* DRAW_REGS */

	pDC->SelectObject(pOldFont);
}

static double NoteFromFreq(double Freq)
{
	// Convert frequency to note number
	return 45.0 + 12.0 * (std::log(Freq / 440.0) / log(2.0));
}

static double FreqFromNote(double Note, double Base)
{
	// Return frequency for a note number
	return Base * std::pow(2.0, Note / 12.0);
}

static double RegToFreqVRC7(int Reg, int Octave)		// // //
{
	return 49716.0 * Reg / (1 << (19 - Octave));
}

static double RegToFreqN163(int Reg, int Div)		// // //
{
	// Div = N163 channels * wave length
	const double BASE_FREQ_NTSC = 236250000.0 / 132.0;
	return BASE_FREQ_NTSC * Reg / 15.0 / (1 << 16) / Div;
}

static double RegToFreq(int Reg, int Chip, int Param)		// // //
{
	// Return NES period converted to frequency
	const double BASE_FREQ_NTSC = 236250000.0 / 132.0;
	const double BASE_FREQ_PAL  = 4433618.75 * 6.0 / 16.0;
	switch (Chip) {
	case SNDCHIP_NONE: return BASE_FREQ_NTSC / 16.0 / (Reg + 1.0); break;
	case SNDCHIP_2A07: return BASE_FREQ_PAL  / 16.0 / (Reg + 1.0); break;
	case SNDCHIP_VRC6: return BASE_FREQ_NTSC / 14.0 / (Reg + 1.0); break;
	case SNDCHIP_VRC7: return RegToFreqVRC7(Reg, Param); break;
	case SNDCHIP_FDS: return BASE_FREQ_NTSC * Reg / (1 << 20); break;
	case SNDCHIP_N163: return RegToFreqN163(Reg, Param); break;
	case SNDCHIP_S5B: return BASE_FREQ_NTSC / 16.0 / (Reg + 1.0); break;
	default: return 0.0;
	}
}

static double RegToFreq(int Reg, int Chip)		// // //
{
	ASSERT(Chip != SNDCHIP_VRC7 && Chip != SNDCHIP_N163);
	return RegToFreq(Reg, Chip, 1);
}

static CString NoteToStr(int Note)
{
	const CString NOTES_S[] = {"C-", "C#", "D-", "D#", "E-", "F-", "F#", "G-", "G#", "A-", "A#", "B-"};		// // //
	const CString NOTES_F[] = {"C-", "Db", "D-", "Eb", "E-", "F-", "Gb", "G-", "Ab", "A-", "Bb", "B-"};		// // //
	
	int Octave = Note / 12 + 1;
	int Index = Note % 12;
	if (Index % 12 < 0) {
		Octave--;
		Index += 12;
	}

	CString str;
	if (theApp.GetSettings()->Appearance.bDisplayFlats)
		str = NOTES_F[Index];
	else
		str = NOTES_S[Index];
	str.AppendFormat("%i", Octave);
	return str;
}

void CPatternEditor::DrawRegisters(CDC *pDC)
{
	// Display 2a03 registers
	const CSoundGen *pSoundGen = theApp.GetSoundGenerator();

	unsigned char reg0, reg1, reg2, reg3, reg4, reg5, reg6, reg7;
	int line = 0, vis_line = 0;
	CFont *pOldFont = pDC->SelectObject(&m_fontCourierNew);
	pDC->FillSolidRect(ROW_COLUMN_WIDTH + m_iPatternWidth + 30, HEADER_HEIGHT + 30, m_iWinWidth, m_iWinHeight, m_colEmptyBg); // // //

	CString text(_T("2A03 registers"));
	pDC->SetBkColor(m_colEmptyBg);
	pDC->SetTextColor(0xFFAFAF);
	pDC->SetBkMode(TRANSPARENT);		// // //
	pDC->TextOut(ROW_COLUMN_WIDTH + m_iPatternWidth + 30, HEADER_HEIGHT + 30 + (line++) * 13, text);		// // //

	vis_line = 14;		// // //
	vis_line += (m_pDocument->ExpansionEnabled(SNDCHIP_VRC6)) ? 6 : 0;
	vis_line += (m_pDocument->ExpansionEnabled(SNDCHIP_MMC5)) ? 5 : 0;
	vis_line += (m_pDocument->ExpansionEnabled(SNDCHIP_N163)) ? 22 : 0;
	vis_line += (m_pDocument->ExpansionEnabled(SNDCHIP_FDS)) ? 16 : 0;
	vis_line += (m_pDocument->ExpansionEnabled(SNDCHIP_VRC7)) ? 11 : 0;
	vis_line += (m_pDocument->ExpansionEnabled(SNDCHIP_S5B)) ? 10 : 0;

	// 2A03
	for (int i = 0; i < 5; ++i) {
		reg0 = pSoundGen->GetReg(SNDCHIP_NONE, i * 4 + 0);
		reg1 = pSoundGen->GetReg(SNDCHIP_NONE, i * 4 + 1);
		reg2 = pSoundGen->GetReg(SNDCHIP_NONE, i * 4 + 2);
		reg3 = pSoundGen->GetReg(SNDCHIP_NONE, i * 4 + 3);

		pDC->SetBkColor(m_colEmptyBg);
		pDC->SetTextColor(0xC0C0C0);

		int x = ROW_COLUMN_WIDTH + m_iPatternWidth + 30;
		int y = HEADER_HEIGHT + 30 + line++ * 13;		// // //

		text.Format(_T("$%04X: $%02X $%02X $%02X $%02X"), 0x4000 + i * 4, reg0, reg1, reg2, reg3);
		pDC->TextOut(x, y, text);

		int period = (reg2 | ((reg3 & 7) << 8));
		int vol = (reg0 & 0x0F);

		double freq;		// // //
		if (i == 4)
			freq = 236250000.0 / 1056.0 / CDPCM::DMC_PERIODS_NTSC[reg0 & 0x0F];
		else if (i == 3)
			freq = 4 * (m_pDocument->GetMachine() == PAL ? CNoise::NOISE_PERIODS_PAL[0x0F - period] : CNoise::NOISE_PERIODS_NTSC[0x0F - period]);
		else
			freq = RegToFreq(period, m_pDocument->GetMachine() == PAL ? SNDCHIP_2A07 : SNDCHIP_NONE);
		if (i == 2) freq /= 2;
		double note = NoteFromFreq(freq);
		int note_conv = note >= 0 ? int(note + 0.5) : int(note - 0.5);		// // //
		int cents = int((note - double(note_conv)) * 100.0);

//		pDC->FillSolidRect(x + 200, y, x + 400, y + 18, m_colEmptyBg);
		pDC->SetTextColor(0x808080);

		switch (i) {
			case 0:
			case 1:
				if (period < 8) {
					freq = 0;
					note_conv = 0;
					cents = 0;
				}
				text.Format(_T("pitch = $%03X (%7.2fHz %s %+03i), vol = %02i, duty = %i"),		// // //
					period, freq, NoteToStr(note_conv), cents, vol, reg0 >> 6);
				break;
			case 2:
				if (period == 0) {
					freq = 0;
					note_conv = 0;
					cents = 0;
				}
				text.Format(_T("pitch = $%03X (%7.2fHz %s %+03i)"),		// // //
					period, freq, NoteToStr(note_conv), cents);
				break;
			case 3:
				text.Format(_T("pitch = $%01X, vol = %02i, mode = %i"), reg2 & 0x0F, vol, reg2 >> 7);		// // //
				break;
			case 4:
				text.Format(_T("pitch = $%01X"), reg0 & 0x0F);		// // //
				if (reg0 & 0x40) {
					text.AppendFormat(_T(" (%7.2fBps %s %+03i)"),		// // //
						freq, NoteToStr(note_conv), cents);
				}
				text.AppendFormat(_T(", size = %i byte%c"), (reg3 << 4) | 1, reg3 ? 's' : ' ');
				break;
			default:
				text.Format(_T(""));
		}

		pDC->TextOut(x + 180, y, text);		// // //

		if (i == 2)
			vol = (reg0 != 0) ? 15 : 0;
		else if (i == 3) {
			period = ((reg2 & 15) << 4) | ((reg2 & 0x80) << 1);
		}
		else if (i == 4) {
			period = (reg0 & 0x0F) << 4;
			vol = 15 * !pSoundGen->PreviewDone();
		}
/*
		pDC->FillSolidRect(ROW_COLUMN_WIDTH + m_iPatternWidth + 250 + i * 30, HEADER_CHAN_HEIGHT, 20, m_iWinHeight - HEADER_CHAN_HEIGHT, 0);
		pDC->FillSolidRect(ROW_COLUMN_WIDTH + m_iPatternWidth + 250 + i * 30, HEADER_CHAN_HEIGHT + (period >> 1), 20, 5, RGB(vol << 4, vol << 4, vol << 4));
*/
		DrawNoteBar(pDC, ROW_COLUMN_WIDTH + m_iPatternWidth + 30, HEADER_HEIGHT + vis_line * 10);
		if (note_conv >= -12 && note_conv <= 96 && vol)		// // //
			pDC->FillSolidRect(ROW_COLUMN_WIDTH + m_iPatternWidth + 29 + 6 * (note_conv + 12), HEADER_HEIGHT + vis_line++ * 10, 3, 7, RGB(vol << 4, vol << 4, vol << 4));
		else vis_line++;
	}

	if (m_pDocument->ExpansionEnabled(SNDCHIP_VRC6)) {

		line++;

		CString text(_T("VRC6 registers"));
		pDC->SetBkColor(m_colEmptyBg);
		pDC->SetTextColor(0xFFAFAF);
		pDC->TextOut(ROW_COLUMN_WIDTH + m_iPatternWidth + 30, HEADER_HEIGHT + 30 + (line++) * 13, text);		// // //

		// VRC6
		for (int i = 0; i < 3; ++i) {
			reg0 = pSoundGen->GetReg(SNDCHIP_VRC6, i * 3 + 0);
			reg1 = pSoundGen->GetReg(SNDCHIP_VRC6, i * 3 + 1);
			reg2 = pSoundGen->GetReg(SNDCHIP_VRC6, i * 3 + 2);

			pDC->SetBkColor(m_colEmptyBg);
			pDC->SetTextColor(0xC0C0C0);

			int x = ROW_COLUMN_WIDTH + m_iPatternWidth + 30;		// // //
			int y = HEADER_HEIGHT + 30 + line++ * 13;		// // //

			text.Format(_T("$%04X: $%02X $%02X $%02X"), 0x9000 + i * 0x1000, reg0, reg1, reg2);
			pDC->TextOut(x, y, text);

			int period = (reg1 | ((reg2 & 15) << 8));
			int vol = (reg0 & 0x0F);

			double freq = RegToFreq(period, (i == 2) ? SNDCHIP_VRC6 : SNDCHIP_NONE);		// // //
			double note = NoteFromFreq(freq);
			int note_conv = note >= 0 ? int(note + 0.5) : int(note - 0.5);		// // //
			int cents = int((note - double(note_conv)) * 100.0);
			pDC->SetTextColor(0x808080);

			if (period == 0) {
				freq = 0;
				note_conv = 0;
				cents = 0;
			}

			switch (i) {		// // //
				case 0:
				case 1:
					text.Format(_T("pitch = $%03X (%7.2fHz %s %+03i), vol = %02i, duty = %i"),
						period, freq, NoteToStr(note_conv), cents, vol, (reg0 >> 4) & 0x07);
					break;
				case 2:
					text.Format(_T("pitch = $%03X (%7.2fHz %s %+03i), vol = %02i"),
						period, freq, NoteToStr(note_conv), cents, vol);
					break;
				default:
					text.Format(_T(""));
			}
			pDC->TextOut(x + 180, y, text);		// // //

			if (i == 2)
				vol = reg0 >> 1;
			
			DrawNoteBar(pDC, ROW_COLUMN_WIDTH + m_iPatternWidth + 30, HEADER_HEIGHT + vis_line * 10);
			if (note_conv >= -12 && note_conv <= 96 && vol)		// // //
				pDC->FillSolidRect(ROW_COLUMN_WIDTH + m_iPatternWidth + 29 + 6 * (note_conv + 12), HEADER_HEIGHT + vis_line++ * 10, 3, 7, RGB(vol << 4, vol << 4, vol << 4));
			else vis_line++;
		}
	}

	if (m_pDocument->ExpansionEnabled(SNDCHIP_MMC5)) {		// // //

		line++;

		CString text(_T("MMC5 registers"));
		pDC->SetBkColor(m_colEmptyBg);
		pDC->SetTextColor(0xFFAFAF);
		pDC->TextOut(ROW_COLUMN_WIDTH + m_iPatternWidth + 30, HEADER_HEIGHT + 30 + (line++) * 13, text);

		// MMC5
		for (int i = 0; i < 2; ++i) {
			reg0 = pSoundGen->GetReg(SNDCHIP_MMC5, i * 4 + 0);
			reg1 = pSoundGen->GetReg(SNDCHIP_MMC5, i * 4 + 1);
			reg2 = pSoundGen->GetReg(SNDCHIP_MMC5, i * 4 + 2);
			reg3 = pSoundGen->GetReg(SNDCHIP_MMC5, i * 4 + 3);

			pDC->SetBkColor(m_colEmptyBg);
			pDC->SetTextColor(0xC0C0C0);

			int x = ROW_COLUMN_WIDTH + m_iPatternWidth + 30;
			int y = HEADER_HEIGHT + 30 + line++ * 13;

			text.Format(_T("$%04X: $%02X $%02X $%02X $%02X"), 0x5000 + i * 4, reg0, reg1, reg2, reg3);
			pDC->TextOut(x, y, text);
			
			int period = (reg2 | ((reg3 & 7) << 8));
			int vol = (reg0 & 0x0F);

			double freq = RegToFreq(period, SNDCHIP_NONE);
			double note = NoteFromFreq(freq);
			int note_conv = note >= 0 ? int(note + 0.5) : int(note - 0.5);
			int cents = int((note - double(note_conv)) * 100.0);
			pDC->SetTextColor(0x808080);

			if (period == 0) {
				freq = 0;
				note_conv = 0;
				cents = 0;
			}
			text.Format(_T("pitch = $%03X (%7.2fHz %s %+03i), vol = %02i, duty = %i"), 
				period, freq, NoteToStr(note_conv), cents, vol, reg0 >> 6);
			pDC->TextOut(x + 180, y, text);
			
			DrawNoteBar(pDC, ROW_COLUMN_WIDTH + m_iPatternWidth + 30, HEADER_HEIGHT + vis_line * 10);
			if (note_conv >= -12 && note_conv <= 96 && vol)
				pDC->FillSolidRect(ROW_COLUMN_WIDTH + m_iPatternWidth + 29 + 6 * (note_conv + 12), HEADER_HEIGHT + vis_line++ * 10, 3, 7, RGB(vol << 4, vol << 4, vol << 4));
			else vis_line++;
		}
	}

	if (m_pDocument->ExpansionEnabled(SNDCHIP_N163)) {

		line++;

		CString text(_T("N163 registers"));
		pDC->SetBkColor(m_colEmptyBg);
		pDC->SetTextColor(0xFFAFAF);
		pDC->TextOut(ROW_COLUMN_WIDTH + m_iPatternWidth + 30, HEADER_HEIGHT + 30 + (line++) * 13, text);		// // //

		// // // N163 wave
		int Length = 0x80 - 8 * m_pDocument->GetNamcoChannels();
		int x = ROW_COLUMN_WIDTH + m_iPatternWidth + 30 + 300;		// // //
		int y = HEADER_HEIGHT + 30 + line * 13;		// // //
		pDC->FillSolidRect(x - 1, y - 1, 2 * Length + 2, 17, 0x808080);
		pDC->FillSolidRect(x, y, 2 * Length, 15, 0);
		for (int i = 0; i < Length; i++) {
			int Hi = pSoundGen->GetReg(SNDCHIP_N163, i) >> 4;
			int Lo = pSoundGen->GetReg(SNDCHIP_N163, i) & 0x0F;
			pDC->FillSolidRect(x + i * 2    , y + 15 - Lo, 1, Lo, 0xFFFFFF);
			pDC->FillSolidRect(x + i * 2 + 1, y + 15 - Hi, 1, Hi, 0xFFFFFF);
		}
		for (int i = 0; i < m_pDocument->GetNamcoChannels(); i++) {
			int WavePos = pSoundGen->GetReg(SNDCHIP_N163, 0x78 - i * 8 + 6);
			int WaveLen = 0x100 - (pSoundGen->GetReg(SNDCHIP_N163, 0x78 - i * 8 + 4) & 0xFC);
			pDC->FillSolidRect(x, y + 20 + i * 5, Length * 2, 3, 0);
			pDC->FillSolidRect(x + WavePos, y + 20 + i * 5, WaveLen, 3, 0xFFFFFF);
		}

		// N163
		for (int i = 0; i < 16; ++i) {
			reg0 = pSoundGen->GetReg(SNDCHIP_N163, i * 8 + 0);
			reg1 = pSoundGen->GetReg(SNDCHIP_N163, i * 8 + 1);
			reg2 = pSoundGen->GetReg(SNDCHIP_N163, i * 8 + 2);
			reg3 = pSoundGen->GetReg(SNDCHIP_N163, i * 8 + 3);
			reg4 = pSoundGen->GetReg(SNDCHIP_N163, i * 8 + 4);
			reg5 = pSoundGen->GetReg(SNDCHIP_N163, i * 8 + 5);
			reg6 = pSoundGen->GetReg(SNDCHIP_N163, i * 8 + 6);
			reg7 = pSoundGen->GetReg(SNDCHIP_N163, i * 8 + 7);
			int period = (reg0 | (reg2 << 8) | ((reg4 & 0x03) << 16));
			int vol = (reg7 & 0x0F);

			pDC->SetBkColor(m_colEmptyBg);
			pDC->SetTextColor(0xC0C0C0);

			int x = ROW_COLUMN_WIDTH + m_iPatternWidth + 30;		// // //
			int y = HEADER_HEIGHT + 30 + line++ * 13;		// // //

			text.Format(_T("$%02X: $%02X $%02X $%02X $%02X $%02X $%02X $%02X $%02X"), i * 8, reg0, reg1, reg2, reg3, reg4, reg5, reg6, reg7);
			pDC->TextOut(x, y, text);

			if (i < 8) continue;		// // //

			double freq = RegToFreqN163(period, m_pDocument->GetNamcoChannels() * (256 - (reg4 & 0xFC)));		// // //
			double note;
			int note_conv, cents;
			if (period) {
				note = NoteFromFreq(freq);
				note_conv = note >= 0 ? int(note + 0.5) : int(note - 0.5);		// // //
				cents = int((note - double(note_conv)) * 100.0);
			}
			else {
				note = note_conv = cents = 0;
			}
			pDC->SetTextColor(0x808080);
			text.Format(_T("pitch = $%05X (%7.2fHz %s %+03i), vol = %02i"),
				period, freq, NoteToStr(note_conv), cents, vol);
			pDC->TextOut(x + 300, y, text);
		}
		
		for (int i = 0; i < m_pDocument->GetNamcoChannels(); ++i) {		// // //
			reg0 = pSoundGen->GetReg(SNDCHIP_N163, 0x78 - i * 8 + 0);
			reg1 = pSoundGen->GetReg(SNDCHIP_N163, 0x78 - i * 8 + 1);
			reg2 = pSoundGen->GetReg(SNDCHIP_N163, 0x78 - i * 8 + 2);
			reg3 = pSoundGen->GetReg(SNDCHIP_N163, 0x78 - i * 8 + 3);
			reg4 = pSoundGen->GetReg(SNDCHIP_N163, 0x78 - i * 8 + 4);
			reg5 = pSoundGen->GetReg(SNDCHIP_N163, 0x78 - i * 8 + 5);
			reg6 = pSoundGen->GetReg(SNDCHIP_N163, 0x78 - i * 8 + 6);
			reg7 = pSoundGen->GetReg(SNDCHIP_N163, 0x78 - i * 8 + 7);
			int period = (reg0 | (reg2 << 8) | ((reg4 & 0x03) << 16));
			int vol = (reg7 & 0x0F);

			double freq = RegToFreqN163(period, m_pDocument->GetNamcoChannels() * (256 - (reg4 & 0xFC)));
			double note;
			int note_conv, cents;
			if (period) {
				note = NoteFromFreq(freq);
				note_conv = note >= 0 ? int(note + 0.5) : int(note - 0.5);
				cents = int((note - double(note_conv)) * 100.0);
			}
			else {
				note = note_conv = cents = 0;
			}
				
			DrawNoteBar(pDC, ROW_COLUMN_WIDTH + m_iPatternWidth + 30, HEADER_HEIGHT + vis_line * 10);
			if (note_conv >= -12 && note_conv <= 96 && vol)
				pDC->FillSolidRect(ROW_COLUMN_WIDTH + m_iPatternWidth + 29 + 6 * (note_conv + 12), HEADER_HEIGHT + vis_line++ * 10, 3, 7, RGB(vol << 4, vol << 4, vol << 4));			
			else vis_line++;
		}
	}

	if (m_pDocument->ExpansionEnabled(SNDCHIP_FDS)) {

		line++;

		CString text(_T("FDS registers"));
		pDC->SetBkColor(m_colEmptyBg);
		pDC->SetTextColor(0xFFAFAF);
		pDC->TextOut(ROW_COLUMN_WIDTH + m_iPatternWidth + 30, HEADER_HEIGHT + 30 + (line++) * 13, text);		// // //

		for (int i = 0; i < 11; ++i) {
			reg0 = pSoundGen->GetReg(SNDCHIP_FDS, i);

			pDC->SetBkColor(m_colEmptyBg);
			pDC->SetTextColor(0xC0C0C0);

			int x = ROW_COLUMN_WIDTH + m_iPatternWidth + 30;		// // //
			int y = HEADER_HEIGHT + 30 + line++ * 13;		// // //

			text.Format(_T("$%04X: $%02X"), 0x4080 + i, reg0);
			pDC->TextOut(x, y, text);
		}

		int period = pSoundGen->GetReg(SNDCHIP_FDS, 2) | (pSoundGen->GetReg(SNDCHIP_FDS, 3) << 8);
		int vol = (pSoundGen->GetReg(SNDCHIP_FDS, 0) & 0x3F);

		double freq = RegToFreq(period, SNDCHIP_FDS) / 4.0;		// // //
		double note;
		int note_conv, cents;
		if (period) {
			note = NoteFromFreq(freq);
			note_conv = note >= 0 ? int(note + 0.5) : int(note - 0.5);		// // //
			cents = int((note - double(note_conv)) * 100.0);
		}
		else {
			note = note_conv = cents = 0;
		}
		pDC->SetTextColor(0x808080);
		text.Format(_T("pitch = $%03X (%7.2fHz %s %+03i), vol = %02i"),
			period, freq, NoteToStr(note_conv), cents, vol);
		pDC->TextOut(ROW_COLUMN_WIDTH + m_iPatternWidth + 30 + 180, HEADER_HEIGHT + 30 + (line - 11) * 13, text);		// // //
		
		DrawNoteBar(pDC, ROW_COLUMN_WIDTH + m_iPatternWidth + 30, HEADER_HEIGHT + vis_line * 10);		// // //
		if (note_conv >= -12 && note_conv <= 96 && vol) {
			if (vol == 32)
				pDC->FillSolidRect(ROW_COLUMN_WIDTH + m_iPatternWidth + 29 + 6 * (note_conv + 12), HEADER_HEIGHT + vis_line++ * 10, 3, 7, 0xFFFFFF);
			else
				pDC->FillSolidRect(ROW_COLUMN_WIDTH + m_iPatternWidth + 29 + 6 * (note_conv + 12), HEADER_HEIGHT + vis_line++ * 10, 3, 7, RGB(vol << 3, vol << 3, vol << 3));
		}
		else vis_line++;
	}

	if (m_pDocument->ExpansionEnabled(SNDCHIP_VRC7)) {		// // //

		line++;

		CString text(_T("VRC7 registers"));
		pDC->SetBkColor(m_colEmptyBg);
		pDC->SetTextColor(0xFFAFAF);
		pDC->TextOut(ROW_COLUMN_WIDTH + m_iPatternWidth + 30, HEADER_HEIGHT + 30 + (line++) * 13, text);

		reg0 = pSoundGen->GetReg(SNDCHIP_VRC7, 0);
		reg1 = pSoundGen->GetReg(SNDCHIP_VRC7, 1);
		reg2 = pSoundGen->GetReg(SNDCHIP_VRC7, 2);
		reg3 = pSoundGen->GetReg(SNDCHIP_VRC7, 3);
		reg4 = pSoundGen->GetReg(SNDCHIP_VRC7, 4);
		reg5 = pSoundGen->GetReg(SNDCHIP_VRC7, 5);
		reg6 = pSoundGen->GetReg(SNDCHIP_VRC7, 6);
		reg7 = pSoundGen->GetReg(SNDCHIP_VRC7, 7);

		pDC->SetBkColor(m_colEmptyBg);
		pDC->SetTextColor(0xC0C0C0);

		text.Format(_T("$00: $%02X $%02X $%02X $%02X $%02X $%02X $%02X $%02X"), reg0, reg1, reg2, reg3, reg4, reg5, reg6, reg7);
		pDC->TextOut(ROW_COLUMN_WIDTH + m_iPatternWidth + 30, HEADER_HEIGHT + 30 + (line++) * 13, text);

		for (int i = 0; i < 6; ++i) {
			reg0 = pSoundGen->GetReg(SNDCHIP_VRC7, i + 0x10);
			reg1 = pSoundGen->GetReg(SNDCHIP_VRC7, i + 0x20);
			reg2 = pSoundGen->GetReg(SNDCHIP_VRC7, i + 0x30);

			pDC->SetBkColor(m_colEmptyBg);
			pDC->SetTextColor(0xC0C0C0);

			int x = ROW_COLUMN_WIDTH + m_iPatternWidth + 30;
			int y = HEADER_HEIGHT + 30 + line++ * 13;

			text.Format(_T("$x%01X: $%02X $%02X $%02X"), i, reg0, reg1, reg2);
			pDC->TextOut(x, y, text);

			int period = reg0 | ((reg1 & 0x01) << 8);
			int octave = (reg1 & 0x0E) >> 1;
			int vol = 0x0F - (pSoundGen->GetReg(SNDCHIP_VRC7, i + 0x30) & 0x0F);
			int inst = reg2 >> 4;

			double freq = RegToFreqVRC7(period, octave);
			double note;
			int note_conv, cents;
			if (period) {
				if (!inst) {
					/* frequency correction
					const int freqMult[] = {1, 2, 4, 6, 8, 10, 12, 14, 16, 18, 20, 20, 24, 24, 30, 30};
					int Modulator = freqMult[pSoundGen->GetReg(SNDCHIP_VRC7, 0) & 0x0F];
					int Carrier = freqMult[pSoundGen->GetReg(SNDCHIP_VRC7, 1) & 0x0F];
					int c;
					while ( Modulator != 0 ) {
						c = Modulator; Modulator = Carrier % Modulator; Carrier = c;
					}
					freq *= Carrier / 2.0;
					*/
				}
				note = NoteFromFreq(freq);
				note_conv = note >= 0 ? int(note + 0.5) : int(note - 0.5);
				cents = int((note - double(note_conv)) * 100.0);
			}
			else {
				note = note_conv = cents = 0;
			}
			pDC->SetTextColor(0x808080);
			text.Format(_T("pitch = $%03X (%7.2fHz %s %+03i), vol = %02i, patch = $%01X"),
				period, freq, NoteToStr(note_conv), cents, vol, inst);
			pDC->TextOut(x + 180, y, text);
			
			DrawNoteBar(pDC, ROW_COLUMN_WIDTH + m_iPatternWidth + 30, HEADER_HEIGHT + vis_line * 10);
			if (note_conv >= -12 && note_conv <= 96 && vol)
				pDC->FillSolidRect(ROW_COLUMN_WIDTH + m_iPatternWidth + 29 + 6 * (note_conv + 12), HEADER_HEIGHT + vis_line++ * 10, 3, 7, RGB(vol << 4, vol << 4, vol << 4));
			else vis_line++;
		}
	}

	if (m_pDocument->ExpansionEnabled(SNDCHIP_S5B)) {		// // //

		line++;

		CString text(_T("5B registers"));
		pDC->SetBkColor(m_colEmptyBg);
		pDC->SetTextColor(0xFFAFAF);
		pDC->TextOut(ROW_COLUMN_WIDTH + m_iPatternWidth + 30, HEADER_HEIGHT + 30 + (line++) * 13, text);

		// S5B
		for (int i = 0; i < 4; ++i) {
			reg0 = pSoundGen->GetReg(SNDCHIP_S5B, i * 2 + 0);
			reg1 = pSoundGen->GetReg(SNDCHIP_S5B, i * 2 + 1);

			pDC->SetBkColor(m_colEmptyBg);
			pDC->SetTextColor(0xC0C0C0);

			int x = ROW_COLUMN_WIDTH + m_iPatternWidth + 30;
			int y = HEADER_HEIGHT + 30 + line++ * 13;

			text.Format(_T("$%02X: $%02X $%02X"), i * 2, reg0, reg1);
			pDC->TextOut(x, y, text);

			int period = (reg0 | ((reg1 & 0x0F) << 8));
			int vol = pSoundGen->GetReg(SNDCHIP_S5B, 8 + i) & 0x0F;

			double freq = RegToFreq(period - 1, SNDCHIP_NONE) / 2;
			double note = NoteFromFreq(freq);
			int note_conv = note >= 0 ? int(note + 0.5) : int(note - 0.5);
			int cents = int((note - double(note_conv)) * 100.0);
			pDC->SetTextColor(0x808080);

			if (period == 0) {
				freq = 0;
				note_conv = 0;
				cents = 0;
			}

			if (i < 3) {
				text.Format(_T("pitch = $%03X (%7.2fHz %s %+03i), vol = %02i, mode = %c%c%c"),
					period, freq, NoteToStr(note_conv), cents, vol,
					(pSoundGen->GetReg(SNDCHIP_S5B, 7) & (1 << i)) ? _T('-') : _T('T'),
					(pSoundGen->GetReg(SNDCHIP_S5B, 7) & (8 << i)) ? _T('-') : _T('N'),
					(pSoundGen->GetReg(SNDCHIP_S5B, 8 + i) & 0x10) ? _T('E') : _T('-'));
			}
			else {
				text.Format(_T("pitch = $%02X"), reg0 & 0x1F);
			}
			pDC->TextOut(x + 180, y, text);
			if (i < 3) {
				DrawNoteBar(pDC, ROW_COLUMN_WIDTH + m_iPatternWidth + 30, HEADER_HEIGHT + vis_line * 10);
				if (note_conv >= -12 && note_conv <= 96 && vol)		// // //
					pDC->FillSolidRect(ROW_COLUMN_WIDTH + m_iPatternWidth + 29 + 6 * (note_conv + 12), HEADER_HEIGHT + vis_line++ * 10, 3, 7, RGB(vol << 4, vol << 4, vol << 4));
				else vis_line++;
			}
		}

		for (int i = 0; i < 2; ++i) {
			reg0 = pSoundGen->GetReg(SNDCHIP_S5B, i * 3 + 8);
			reg1 = pSoundGen->GetReg(SNDCHIP_S5B, i * 3 + 9);
			reg2 = pSoundGen->GetReg(SNDCHIP_S5B, i * 3 + 10);

			pDC->SetBkColor(m_colEmptyBg);
			pDC->SetTextColor(0xC0C0C0);

			int x = ROW_COLUMN_WIDTH + m_iPatternWidth + 30;
			int y = HEADER_HEIGHT + 30 + line++ * 13;

			text.Format(_T("$%02X: $%02X $%02X $%02X"), i * 3 + 8, reg0, reg1, reg2);
			pDC->TextOut(x, y, text);
			
			if (i == 1) {
				pDC->SetTextColor(0x808080);
				int period = (reg0 | (reg1 << 8));
				double freq, note;
				int note_conv, cents;
				if ((reg2 & 0x08) && !(reg2 & 0x01) && reg0 && !reg1) {
					freq = RegToFreq(period - 1, SNDCHIP_NONE) / 2;
					if (reg2 & 0x02) freq /= 32;	// triangle
					else freq /= 16;				// sawtooth
					note = NoteFromFreq(freq);
					note_conv = note >= 0 ? int(note + 0.5) : int(note - 0.5);
					cents = int((note - double(note_conv)) * 100.0);

					text.Format(_T("pitch = $%04X (%7.2fHz %s %+03i), shape = $%01X"),
						period, freq, NoteToStr(note_conv), cents, reg2);
				}
				else
					text.Format(_T("period = $%04X, shape = $%01X"), period, reg2);

				pDC->TextOut(x + 180, y, text);
			}
		}
	}

	pDC->SelectObject(pOldFont);

	// Surrounding frame
//	pDC->Draw3dRect(ROW_COLUMN_WIDTH + m_iPatternWidth + 20, HEADER_HEIGHT + 20, 200, line * 18 + 20, 0xA0A0A0, 0x505050);

}

void CPatternEditor::DrawNoteBar(CDC *pDC, int x, int y)		// // //
{
	pDC->FillSolidRect(x - 1, y - 1, 6 * 108 + 3, 9, 0x808080);
	pDC->FillSolidRect(x, y, 6 * 108 + 1, 7, 0);
	for (int i = 0; i < 10; i++)
		pDC->SetPixelV(x + 72 * i, y + 3, i == 4 ? 0x808080 : 0x303030);
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
		ColumnCount += GetChannelColumns(i);
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

int CPatternEditor::GetRowAtPoint(int PointY) const
{
	return (PointY - HEADER_HEIGHT) / m_iRowHeight - (m_iLinesVisible / 2) + m_iCenterRow;
}

int CPatternEditor::GetChannelAtPoint(int PointX) const
{
	// Convert X position to channel number
	const int ChannelCount = GetChannelCount();

	if (PointX < ROW_COLUMN_WIDTH)
		return -1;	// -1 means row number column

	const int Offset = PointX - ROW_COLUMN_WIDTH + m_iChannelOffsets[m_iFirstChannel];
	for (int i = m_iFirstChannel; i < ChannelCount; ++i) {
		if (Offset >= m_iChannelOffsets[i] && Offset < (m_iChannelOffsets[i] + m_iChannelWidths[i]))
			return i;
	}

	return m_iFirstChannel + m_iChannelsVisible;
}

int CPatternEditor::GetColumnAtPoint(int PointX) const
{
	// Convert X position to column number
	const int ChannelCount = GetChannelCount();
	const int Channel = GetChannelAtPoint(PointX);

	if (Channel < 0)
		return 0;
	if (Channel >= ChannelCount)
		return GetChannelColumns(ChannelCount - 1) - 1;

	const int Offset = PointX - ROW_COLUMN_WIDTH + m_iChannelOffsets[m_iFirstChannel];
	int ColumnOffset = m_iChannelOffsets[Channel];
	for (int i = 0; i < GetChannelColumns(Channel); ++i) {
		ColumnOffset += COLUMN_SPACE[i];
		if (Offset <= ColumnOffset)
			return i;
	}

	return GetChannelColumns(Channel);
}

CCursorPos CPatternEditor::GetCursorAtPoint(const CPoint &point) const
{
	return CCursorPos(GetRowAtPoint(point.y), GetChannelAtPoint(point.x), GetColumnAtPoint(point.x));
}

int CPatternEditor::GetSelectColumn(int Column)
{
	// Return first column for a specific column field
	static const int COLUMNS[] = {
		COLUMN_NOTE, 
		COLUMN_INSTRUMENT, COLUMN_INSTRUMENT,
		COLUMN_VOLUME,
		COLUMN_EFF1, COLUMN_EFF1, COLUMN_EFF1,
		COLUMN_EFF2, COLUMN_EFF2, COLUMN_EFF2,
		COLUMN_EFF3, COLUMN_EFF3, COLUMN_EFF3,
		COLUMN_EFF4, COLUMN_EFF4, COLUMN_EFF4
	};

	ASSERT(Column >= 0 && Column < 16);

	return COLUMNS[Column];
}

int CPatternEditor::GetCursorStartColumn(int Column) const
{
	static const int COL_START[] = {
		0, 1, 3, 4, 7, 10, 13
	};

	ASSERT(Column >= 0 && Column < 16);

	return COL_START[Column];
}

int CPatternEditor::GetCursorEndColumn(int Column) const
{
	static const int COL_END[] = {
		0, 2, 3, 6, 9, 12, 15
	};

	ASSERT(Column >= 0 && Column < 16);

	return COL_END[Column];
}

int CPatternEditor::GetChannelColumns(int Channel) const
{
	// Return number of available columns in a channel
	return m_pDocument->GetEffColumns(GetSelectedTrack(), Channel) * 3 + COLUMNS;
}

int CPatternEditor::GetSelectedTrack() const
{
	return GetMainFrame()->GetSelectedTrack();
}

int CPatternEditor::GetChannelCount() const
{
	return m_pDocument->GetAvailableChannels();
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
	m_selection.SetStart(m_cpCursorPos);
	m_selection.SetEnd(m_cpCursorPos);		// // //

	m_bDragStart = false;
	m_bDragging = false;
}

void CPatternEditor::SetSelectionStart()
{
	m_cpSelCursor = m_cpCursorPos;

	if (m_bSelecting)
		m_bSelectionInvalidated = true;
}

void CPatternEditor::SetSelectionStart(const CCursorPos &start)
{
	if (m_bSelecting)
		m_bSelectionInvalidated = true;

	m_selection.SetStart(start);
}

void CPatternEditor::SetSelectionEnd(const CCursorPos &end)
{
	if (m_bSelecting)
		m_bSelectionInvalidated = true;

	m_selection.SetEnd(end);
}

void CPatternEditor::UpdateSelection()
{
	// Call after cursor has moved
	// If shift is not pressed, set selection starting point to current cursor position
	// If shift is pressed, update selection end point

	const bool bShift = IsShiftPressed();

	if (bShift) {
		if (!m_bCurrentlySelecting && !m_bSelecting) {
			SetSelectionStart(m_cpSelCursor);
		}

		m_bCurrentlySelecting = true;
		m_bSelecting = true;
		SetSelectionEnd(m_cpCursorPos);
		m_bSelectionInvalidated = true;
	}
	else {
		m_bCurrentlySelecting = false;

		if (theApp.GetSettings()->General.iEditStyle != EDIT_STYLE3 || m_bSelecting == false)
			CancelSelection();

		SetSelectionStart();
	}
}

void CPatternEditor::MoveDown(int Step)
{
	Step = (Step == 0) ? 1 : Step;
	MoveToRow(m_cpCursorPos.m_iRow + Step);
	UpdateSelection();
}

void CPatternEditor::MoveUp(int Step)
{
	Step = (Step == 0) ? 1 : Step;
	MoveToRow(m_cpCursorPos.m_iRow - Step);
	UpdateSelection();
}

void CPatternEditor::MoveLeft()
{
	ScrollLeft();
	UpdateSelection();
}

void CPatternEditor::MoveRight()
{
	ScrollRight();
	UpdateSelection();
}

void CPatternEditor::MoveToTop()
{
	MoveToRow(0);
	UpdateSelection();
}

void CPatternEditor::MoveToBottom()
{
	MoveToRow(m_iPatternLength - 1);
	UpdateSelection();
}

void CPatternEditor::NextChannel()
{
	MoveToChannel(m_cpCursorPos.m_iChannel + 1);
	m_cpCursorPos.m_iColumn = 0;

	m_bCurrentlySelecting = false;
	SetSelectionStart();
}

void CPatternEditor::PreviousChannel()
{
	MoveToChannel(m_cpCursorPos.m_iChannel - 1);
	m_cpCursorPos.m_iColumn = 0;

	m_bCurrentlySelecting = false;
	SetSelectionStart();
}

void CPatternEditor::FirstChannel()
{
	MoveToChannel(0);
	m_cpCursorPos.m_iColumn	= 0;
	UpdateSelection();
}

void CPatternEditor::LastChannel()
{
	MoveToChannel(GetChannelCount() - 1);
	m_cpCursorPos.m_iColumn	= 0;
	UpdateSelection();
}

void CPatternEditor::MoveChannelLeft()
{
	const int ChannelCount = GetChannelCount();

	// Wrapping
	if (--m_cpCursorPos.m_iChannel < 0)
		m_cpCursorPos.m_iChannel = ChannelCount - 1;

	int Columns = GetChannelColumns(m_cpCursorPos.m_iChannel) - 1;

	if (Columns < m_cpCursorPos.m_iColumn)
		m_cpCursorPos.m_iColumn = Columns;

	UpdateSelection();
}

void CPatternEditor::MoveChannelRight()
{
	const int ChannelCount = GetChannelCount();

	// Wrapping
	if (++m_cpCursorPos.m_iChannel > (ChannelCount - 1))
		m_cpCursorPos.m_iChannel = 0;

	int Columns = GetChannelColumns(m_cpCursorPos.m_iChannel) - 1;

	if (Columns < m_cpCursorPos.m_iColumn)
		m_cpCursorPos.m_iColumn = Columns;

	UpdateSelection();
}

void CPatternEditor::OnHomeKey()
{
	const bool bControl = IsControlPressed();

	if (bControl || theApp.GetSettings()->General.iEditStyle == EDIT_STYLE1) {
		// Control or FT2 edit style
		MoveToTop();
	}
	else {
		if (GetColumn() != 0)
			MoveToColumn(0);
		else if (GetChannel() != 0)
			MoveToChannel(0);
		else if (GetRow() != 0)
			MoveToRow(0);
	}

	UpdateSelection();
}

void CPatternEditor::OnEndKey()
{
	const bool bControl = IsControlPressed();
	const int Channels = GetChannelCount();
	const int Columns = GetChannelColumns(GetChannel());

	if (bControl || theApp.GetSettings()->General.iEditStyle == EDIT_STYLE1) {
		// Control or FT2 edit style
		MoveToBottom();
	}
	else {
		if (GetColumn() != Columns - 1)
			MoveToColumn(Columns - 1);
		else if (GetChannel() != Channels - 1) {
			MoveToChannel(Channels - 1);
			MoveToColumn(GetChannelColumns(Channels - 1) - 1);
		}
		else if (GetRow() != m_iPatternLength - 1)
			MoveToRow(m_iPatternLength - 1);
	}

	UpdateSelection();
}

void CPatternEditor::MoveToRow(int Row)
{
	const int PatternLength = GetCurrentPatternLength(m_iCurrentFrame);		// // //

	if (theApp.IsPlaying() && m_bFollowMode)
		return;

	if (theApp.GetSettings()->General.bWrapFrames) {
		while (Row < 0) {
			MoveToFrame(m_iCurrentFrame - 1);
			Row += GetCurrentPatternLength(m_iCurrentFrame);
		}
		while (Row >= GetCurrentPatternLength(m_iCurrentFrame)) {
			Row -= GetCurrentPatternLength(m_iCurrentFrame);
			MoveToFrame(m_iCurrentFrame + 1);
		}
	}
	else if (theApp.GetSettings()->General.bWrapCursor) {
		Row %= PatternLength;
		if (Row < 0) Row += PatternLength;
	}
	else
		Row = std::min(std::max(Row, 0), PatternLength - 1);

	m_cpCursorPos.m_iRow = Row;
}

void CPatternEditor::MoveToFrame(int Frame)
{
	const int Track = GetSelectedTrack();
	const int FrameCount = m_pDocument->GetFrameCount(Track);

	if (m_iCurrentFrame == Frame)
		return;

	if (Frame < 0) {
		if (theApp.GetSettings()->General.bWrapFrames)
			Frame = FrameCount - 1;
		else
			Frame = 0;
	}
	if (Frame > FrameCount - 1) {
		if (theApp.GetSettings()->General.bWrapFrames)
			Frame = 0;
		else
			Frame = FrameCount - 1;
	}
	
	if (theApp.IsPlaying() && m_bFollowMode) {
		if (m_iPlayFrame != Frame) {
			theApp.GetSoundGenerator()->MoveToFrame(Frame);
			theApp.GetSoundGenerator()->ResetTempo();
		}
	}

	m_iCurrentFrame = Frame;
	CancelSelection();
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
	m_cpCursorPos.m_iColumn = 0;
}

void CPatternEditor::MoveToColumn(int Column)
{
	m_cpCursorPos.m_iColumn = Column;
}

void CPatternEditor::NextFrame()
{
	MoveToFrame(m_iCurrentFrame + 1);
	CancelSelection();
	UpdateSelection();
}

void CPatternEditor::PreviousFrame()
{
	MoveToFrame(m_iCurrentFrame - 1);
	CancelSelection();
	UpdateSelection();
}

// Used by scrolling

void CPatternEditor::ScrollLeft()
{
	if (m_cpCursorPos.m_iColumn > 0)
		m_cpCursorPos.m_iColumn--;
	else {
		if (m_cpCursorPos.m_iChannel > 0) {
			m_cpCursorPos.m_iChannel--;
			m_cpCursorPos.m_iColumn = m_iColumns[m_cpCursorPos.m_iChannel] - 1;
		}
		else {
			if (theApp.GetSettings()->General.bWrapCursor) {
				m_cpCursorPos.m_iChannel = GetChannelCount() - 1;
				m_cpCursorPos.m_iColumn = m_iColumns[m_cpCursorPos.m_iChannel] - 1;
			}
		}
	}
}

void CPatternEditor::ScrollRight()
{
	if (m_cpCursorPos.m_iColumn < m_iColumns[m_cpCursorPos.m_iChannel] - 1)
		m_cpCursorPos.m_iColumn++;
	else {
		if (m_cpCursorPos.m_iChannel < GetChannelCount() - 1) {
			m_cpCursorPos.m_iChannel++;
			m_cpCursorPos.m_iColumn = 0;
		}
		else {
			if (theApp.GetSettings()->General.bWrapCursor) {
				m_cpCursorPos.m_iChannel = 0;
				m_cpCursorPos.m_iColumn = 0;
			}
		}
	}
}

void CPatternEditor::ScrollNextChannel()
{
	MoveToChannel(m_cpCursorPos.m_iChannel + 1);
	m_cpCursorPos.m_iColumn = 0;
}

void CPatternEditor::ScrollPreviousChannel()
{
	MoveToChannel(m_cpCursorPos.m_iChannel - 1);
	m_cpCursorPos.m_iColumn = 0;
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
	return point.x < (m_iPatternWidth + ROW_COLUMN_WIDTH);
}

bool CPatternEditor::IsInsideRowColumn(const CPoint &point) const
{
	return point.x < ROW_COLUMN_WIDTH;
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

	if (bShift && !m_selection.IsWithin(PointPos)) {
		// Expand selection
		if (!PointPos.IsValid(m_iPatternLength, ChannelCount))
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
			PointPos.m_iRow = std::min(PointPos.m_iRow, m_iPatternLength - 1);
			SetSelectionStart(CCursorPos(PointPos.m_iRow, 0, 0));
			SetSelectionEnd(CCursorPos(PointPos.m_iRow, ChannelCount - 1, GetChannelColumns(ChannelCount - 1)));
			m_bFullRowSelect = true;
			m_ptSelStartPoint = point;
			m_bMouseActive = true;
		}
		else if (IsInsidePattern(point)) {
			// Pattern area
			m_bFullRowSelect = false;

			if (!PointPos.IsValid(m_iPatternLength, ChannelCount))
				return;

			if (IsSelecting()) {
				if (m_selection.IsWithin(PointPos)) {
					if (m_pView->GetEditMode())		// // //
						m_bDragStart = true;
				}
				else {
					m_bDragStart = false;
					m_bDragging = false;
					CancelSelection();
				}
			}

			if (!m_bDragging && !m_bDragStart) {
				// Begin new selection
				if (bControl) {
					PointPos.m_iColumn = 0;
				}
				SetSelectionStart(PointPos);
				SetSelectionEnd(PointPos);
			}
			else
				m_cpDragPoint = CCursorPos(PointPos.m_iRow, PointPos.m_iChannel, GetSelectColumn(PointPos.m_iColumn));

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

		if (IsInsideRowColumn(point)) {
			if (m_bDragging) {
				m_bDragging = false;
				m_bDragStart = false;
			}
			// Row column, move to clicked row
			if (PointPos.m_iRow < 0 || m_bSelecting)
				return;
			m_cpCursorPos.m_iRow = PointPos.m_iRow;
			return;
		}

		if (m_bDragStart && !m_bDragging) {
			m_bDragStart = false;
			CancelSelection();
		}

		if (m_bSelecting)
			return;

		if (PointPos.IsValid(m_iPatternLength, ChannelCount)) {
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
		if (abs(m_ptSelStartPoint.x - point.x) > m_iDragThresholdX || abs(m_ptSelStartPoint.y - point.y) > m_iDragThresholdY)
			m_bSelecting = true;
	}
}

void CPatternEditor::ContinueMouseSelection(const CPoint &point)
{
	const bool bControl = IsControlPressed();
	const int ChannelCount = GetChannelCount();
	const int Track = GetSelectedTrack();

	CCursorPos PointPos = GetCursorAtPoint(point);

	// Selecting or dragging
	PointPos.m_iRow = std::max(PointPos.m_iRow, 0);
	PointPos.m_iRow = std::min(PointPos.m_iRow, m_iPatternLength - 1);
	PointPos.m_iChannel = std::max(PointPos.m_iChannel, 0);
	PointPos.m_iChannel = std::min(PointPos.m_iChannel, ChannelCount - 1);

	if (m_bDragStart) {
		// Dragging
		if (abs(m_ptSelStartPoint.x - point.x) > m_iDragThresholdX || abs(m_ptSelStartPoint.y - point.y) > m_iDragThresholdY) {
			// Initiate OLE drag & drop
			int ChanOffset = PointPos.m_iChannel - m_selection.GetChanStart();
			int RowOffset = PointPos.m_iRow - m_selection.GetRowStart();
			m_bDragStart = false;
			m_pView->BeginDragData(ChanOffset, RowOffset);
		}
	}
	else if (!m_pView->IsDragging()) {
		// Expand selection
		if (bControl) {
			if (PointPos.m_iChannel >= m_selection.m_cpStart.m_iChannel) {
				PointPos.m_iColumn = m_pDocument->GetEffColumns(Track, PointPos.m_iChannel) * 3 + 4;
				m_selection.m_cpStart.m_iColumn = 0;
				m_bSelectionInvalidated = true;
			}
			else {
				PointPos.m_iColumn = 0;
				m_selection.m_cpStart.m_iColumn = m_pDocument->GetEffColumns(Track, m_selection.m_cpStart.m_iChannel) * 3 + 4;
				m_bSelectionInvalidated = true;
			}
		}

		// Full row selection
		if (m_bFullRowSelect)
			m_selection.m_cpEnd.m_iRow = PointPos.m_iRow;
		else
			SetSelectionEnd(PointPos);

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

		if (m_cpCursorPos.m_iRow > (m_iPatternLength - 1)) {
			if (theApp.GetSettings()->General.bWrapFrames) {
				m_cpCursorPos.m_iRow %= m_iPatternLength;
				MoveToFrame(m_iCurrentFrame + 1);
			}
			else
				m_cpCursorPos.m_iRow = (m_iPatternLength - 1);
		}
		else if (m_cpCursorPos.m_iRow < 0) {
			if (theApp.GetSettings()->General.bWrapFrames && m_iPrevPatternLength > 0) {
				m_cpCursorPos.m_iRow = abs((m_cpCursorPos.m_iRow + m_iPrevPatternLength) % m_iPrevPatternLength);
				MoveToFrame(m_iCurrentFrame - 1);
			}
			else
				m_cpCursorPos.m_iRow = 0;
		}

		m_iCenterRow = m_cpCursorPos.m_iRow;
	}
}

void CPatternEditor::OnMouseRDown(const CPoint &point)
{
	// Right mouse button down
	const int ChannelCount = GetChannelCount();

	if (IsOverPattern(point)) {
		// Pattern area
		CCursorPos PointPos = GetCursorAtPoint(point);
		if (PointPos.IsValid(m_iPatternLength, ChannelCount)) {
			m_cpCursorPos = PointPos;
		}
	}
}

void CPatternEditor::DragPaste(const CPatternClipData *pClipData, const CSelection *pDragTarget, bool bMix)
{
	// Paste drag'n'drop selections

	// Set cursor location
	m_cpCursorPos = pDragTarget->m_cpStart;

	Paste(pClipData, bMix ? PASTE_MIX : PASTE_DEFAULT);		// // //

	// Update selection
	SetSelectionStart(pDragTarget->m_cpStart);
	SetSelectionEnd(pDragTarget->m_cpEnd);
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
	return m_iCurrentFrame;
}

int CPatternEditor::GetChannel() const
{
	return m_cpCursorPos.m_iChannel;
}

int CPatternEditor::GetRow() const
{
	return m_cpCursorPos.m_iRow;
}

int CPatternEditor::GetColumn() const
{
	return m_cpCursorPos.m_iColumn;
}

// Copy and paste ///////////////////////////////////////////////////////////////////////////////////////////

CPatternClipData *CPatternEditor::CopyEntire() const
{
	const int Track = GetSelectedTrack();
	const int ChannelCount = GetChannelCount();
	const int Rows = m_pDocument->GetPatternLength(Track);
	
	CPatternClipData *pClipData = new CPatternClipData(ChannelCount, Rows);

	pClipData->ClipInfo.Channels = ChannelCount;
	pClipData->ClipInfo.Rows = Rows;

	for (int i = 0; i < ChannelCount; ++i) {
		for (int j = 0; j < Rows; ++j) {
			m_pDocument->GetNoteData(Track, m_iCurrentFrame, i, j, pClipData->GetPattern(i, j));
		}
	}
	
	return pClipData;
}

CPatternClipData *CPatternEditor::Copy() const
{
	// Copy selection

	const int Track			= GetSelectedTrack();
	const int ChannelCount	= GetChannelCount();
	const int Channels		= m_selection.GetChanEnd() - m_selection.GetChanStart() + 1;
	const int Rows			= m_selection.GetRowEnd() - m_selection.GetRowStart() + 1;

	CPatternClipData *pClipData = new CPatternClipData(Channels, Rows);

	pClipData->ClipInfo.Channels	= m_selection.GetChanEnd() - m_selection.GetChanStart() + 1;
	pClipData->ClipInfo.StartColumn	= GetSelectColumn(m_selection.GetColStart());
	pClipData->ClipInfo.EndColumn	= GetSelectColumn(m_selection.GetColEnd());
	pClipData->ClipInfo.Rows		= m_selection.GetRowEnd() - m_selection.GetRowStart() + 1;

	int Channel = 0;
	for (int i = std::max(m_selection.GetChanStart(), 0); i <= std::min(m_selection.GetChanEnd(), ChannelCount - 1); ++i) {
		int Row = 0;
		for (int j = std::max(m_selection.GetRowStart(), 0); j <= std::min(m_selection.GetRowEnd(), m_iPatternLength - 1); ++j) {

			stChanNote NoteData;
			m_pDocument->GetNoteData(Track, m_iCurrentFrame, i, j, &NoteData);

			stChanNote *pClipNote = pClipData->GetPattern(Channel, Row);

			if (m_selection.IsColumnSelected(COLUMN_NOTE, i)) {
				pClipNote->Note = NoteData.Note;
				pClipNote->Octave = NoteData.Octave;
			}
			if (m_selection.IsColumnSelected(COLUMN_INSTRUMENT, i)) {
				pClipNote->Instrument = NoteData.Instrument;
			}
			if (m_selection.IsColumnSelected(COLUMN_VOLUME, i)) {
				pClipNote->Vol = NoteData.Vol;
			}
			if (m_selection.IsColumnSelected(COLUMN_EFF1, i)) {
				pClipNote->EffNumber[0] = NoteData.EffNumber[0];
				pClipNote->EffParam[0] = NoteData.EffParam[0];
			}
			if (m_selection.IsColumnSelected(COLUMN_EFF2, i)) {
				pClipNote->EffNumber[1] = NoteData.EffNumber[1];
				pClipNote->EffParam[1] = NoteData.EffParam[1];
			}
			if (m_selection.IsColumnSelected(COLUMN_EFF3, i)) {
				pClipNote->EffNumber[2] = NoteData.EffNumber[2];
				pClipNote->EffParam[2] = NoteData.EffParam[2];
			}
			if (m_selection.IsColumnSelected(COLUMN_EFF4, i)) {
				pClipNote->EffNumber[3] = NoteData.EffNumber[3];
				pClipNote->EffParam[3] = NoteData.EffParam[3];
			}

			++Row;
		}
		++Channel;
	}

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
	for (int i = 0; i < pClipData->ClipInfo.Channels; ++i) {
		for (int j = 0; j < pClipData->ClipInfo.Rows; ++j) {
			m_pDocument->SetNoteData(Track, m_iCurrentFrame, i, j, pClipData->GetPattern(i, j));
		}
	}
}

void CPatternEditor::Paste(const CPatternClipData *pClipData, paste_mode_t Mode)		// // //
{
	// // // Paste
	const int Track			= GetSelectedTrack();
	const int ChannelCount  = GetChannelCount();
	const int PatternLength = m_pDocument->GetPatternLength(Track);

	const int Channels	  = pClipData->ClipInfo.Channels;
	const int Rows		  = pClipData->ClipInfo.Rows;
	const int StartColumn = pClipData->ClipInfo.StartColumn;
	const int EndColumn	  = pClipData->ClipInfo.EndColumn;
	
	const int PasteMode = Mode & 0xFF;		// // //
	const int PasteAttribute = Mode >> 8;

	stChanNote NoteData;

	int f = m_iCurrentFrame;
	int r = m_cpCursorPos.m_iRow;

	// Special, single channel and effect columns only
	if (Channels == 1 && StartColumn >= COLUMN_EFF1) {
		for (int j = 0; j < Rows; ++j) {
			if (r < 0 || r >= PatternLength) {
				if (!theApp.GetSettings()->General.bOverflowPaste) continue;
			}

			const stChanNote *pClipPattern = pClipData->GetPattern(0, j);

			m_pDocument->GetNoteData(Track, f, m_cpCursorPos.m_iChannel, j + m_cpCursorPos.m_iRow, &NoteData);

			for (int i = StartColumn - COLUMN_EFF1; i < (EndColumn - COLUMN_EFF1 + 1); ++i) {
				int Offset = (GetSelectColumn(m_cpCursorPos.m_iColumn) - COLUMN_EFF1) + (i - (StartColumn - COLUMN_EFF1));
				if ((PasteMode == PASTE_DEFAULT ||
					(PasteMode == PASTE_MIX && pClipPattern->EffNumber[i] != EF_NONE && NoteData.EffNumber[Offset] == EF_NONE) ||
					(PasteMode == PASTE_OVERWRITE && pClipPattern->EffNumber[i] != EF_NONE))
					&& Offset < 4 && Offset >= 0) {
					NoteData.EffNumber[Offset] = pClipPattern->EffNumber[i];
					NoteData.EffParam[Offset] = pClipPattern->EffParam[i];
				}
			}

			m_pDocument->SetNoteData(Track, f, m_cpCursorPos.m_iChannel, j + m_cpCursorPos.m_iRow, &NoteData);
		}
		return;
	}	

	for (int i = 0; i < Channels; ++i) {
		if ((i + m_cpCursorPos.m_iChannel) < 0 || (i + m_cpCursorPos.m_iChannel) >= ChannelCount)
			continue;

		for (int j = 0; j < Rows; ++j) {
			if ((j + m_cpCursorPos.m_iRow) < 0 || (j + m_cpCursorPos.m_iRow) >= PatternLength)
				continue;

			m_pDocument->GetNoteData(Track, f, i + m_cpCursorPos.m_iChannel, j + m_cpCursorPos.m_iRow, &NoteData);

			const stChanNote *pClipNote = pClipData->GetPattern(i, j);

			bool Skip;
			// Note & octave
			if ((i != 0 || StartColumn <= COLUMN_NOTE) && (i != (Channels - 1) || EndColumn >= COLUMN_NOTE)) {
				if (PasteMode == PASTE_DEFAULT ||
					(PasteMode == PASTE_MIX && pClipNote->Note != NONE && NoteData.Note == NONE) ||
					(PasteMode == PASTE_OVERWRITE && pClipNote->Note != NONE)) {
					NoteData.Note = pClipNote->Note;
					NoteData.Octave = pClipNote->Octave;
				}
			}
			// Instrument
			if ((i != 0 || StartColumn <= COLUMN_INSTRUMENT) && (i != (Channels - 1) || EndColumn >= COLUMN_INSTRUMENT)) {
				if (PasteMode == PASTE_DEFAULT ||
					(PasteMode == PASTE_MIX && pClipNote->Instrument < MAX_INSTRUMENTS && NoteData.Instrument == MAX_INSTRUMENTS) ||
					(PasteMode == PASTE_OVERWRITE && pClipNote->Instrument < MAX_INSTRUMENTS))
					NoteData.Instrument = pClipNote->Instrument;
			}
			// Volume
			if ((i != 0 || StartColumn <= COLUMN_VOLUME) && (i != (Channels - 1) || EndColumn >= COLUMN_VOLUME)) {
				if (PasteMode == PASTE_DEFAULT ||
					(PasteMode == PASTE_MIX && pClipNote->Vol < MAX_VOLUME && NoteData.Vol == MAX_VOLUME) ||
					(PasteMode == PASTE_OVERWRITE && pClipNote->Vol < MAX_VOLUME))
					NoteData.Vol = pClipNote->Vol;
			}
			// Effects
			for (int k = 0; k < MAX_EFFECT_COLUMNS; ++k) {
				if ((i != 0 || StartColumn <= (COLUMN_EFF1 + k)) && (i != (Channels - 1) || EndColumn >= (COLUMN_EFF1 + k))) {
					if (PasteMode == PASTE_DEFAULT ||
						(PasteMode == PASTE_MIX && pClipNote->EffNumber[k] != EF_NONE && NoteData.EffNumber[k] == EF_NONE) ||
						(PasteMode == PASTE_OVERWRITE && pClipNote->EffNumber[k] != EF_NONE)) {
						NoteData.EffNumber[k] = pClipNote->EffNumber[k];
						NoteData.EffParam[k] = pClipNote->EffParam[k];
					}
				}
			}

			m_pDocument->SetNoteData(Track, f, i + m_cpCursorPos.m_iChannel, j + m_cpCursorPos.m_iRow, &NoteData);
		}
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
	SetSelectionStart(CCursorPos(0, m_cpCursorPos.m_iChannel, 0));
	SetSelectionEnd(CCursorPos(m_iPatternLength - 1, m_cpCursorPos.m_iChannel, GetChannelColumns(m_cpCursorPos.m_iChannel) - 1));
}

void CPatternEditor::SelectAllChannels()
{
	// Select all channels
	m_bSelecting = true;
	SetSelectionStart(CCursorPos());
	SetSelectionEnd(CCursorPos(m_iPatternLength - 1, GetChannelCount() - 1, GetChannelColumns(GetChannelCount() - 1) - 1));
}

void CPatternEditor::SelectAll()
{
	bool selectAll = false;

	if (m_bSelecting) {
		if (m_selection.GetChanStart() == m_cpCursorPos.m_iChannel && m_selection.GetChanEnd() == m_cpCursorPos.m_iChannel) {
			if (m_selection.GetRowStart() == 0 && m_selection.GetRowEnd() == m_iPatternLength - 1) {
				if (m_selection.GetColStart() == 0 && m_selection.GetColEnd() == GetChannelColumns(m_cpCursorPos.m_iChannel) - 1)
					selectAll = true;
			}
		}
	}

	if (selectAll)
		SelectAllChannels();
	else
		SelectChannel();
}

// Other ////////////////////////////////////////////////////////////////////////////////////////////////////

int CPatternEditor::GetCurrentPatternLength(unsigned int Frame) const
{
	const int Track = GetSelectedTrack();
	const int Channels = GetChannelCount();
	const int PatternLength = m_pDocument->GetPatternLength(Track);	// default length
	
	int HaltPoint = PatternLength;

	if (!theApp.GetSettings()->General.bFramePreview || Frame >= m_pDocument->GetFrameCount(Track))
		return PatternLength;
	else
		return m_pDocument->GetFrameLength(Track, Frame);		// // // moved
}

void CPatternEditor::SetHighlight(int Rows, int SecondRows)
{
	m_iHighlight = Rows;
	m_iHighlightSecond = SecondRows;
}

void CPatternEditor::SetFollowMove(bool bEnable)
{
	m_bFollowMode = bEnable;
}

void CPatternEditor::SetCompactMode(bool bEnable)		// // //
{
	m_bCompactMode = bEnable;
	CalculatePatternLayout();
}

void CPatternEditor::SetFocus(bool bFocus)
{
	m_bHasFocus = bFocus;
}

void CPatternEditor::IncreaseEffectColumn(int Channel)
{
	const int Columns = m_pDocument->GetEffColumns(GetSelectedTrack(), Channel);
	if (Columns < (MAX_EFFECT_COLUMNS - 1)) {
		CPatternAction *pAction = new CPatternAction(CPatternAction::ACT_EXPAND_COLUMNS);
		pAction->SetClickedChannel(Channel);
		GetMainFrame()->AddAction(pAction);
	}
}

void CPatternEditor::DecreaseEffectColumn(int Channel)
{
	const int Columns = m_pDocument->GetEffColumns(GetSelectedTrack(), Channel);
	if (Columns > 0) {
		CPatternAction *pAction = new CPatternAction(CPatternAction::ACT_SHRINK_COLUMNS);
		pAction->SetClickedChannel(Channel);
		GetMainFrame()->AddAction(pAction);
		if (m_cpCursorPos.m_iColumn > Columns * 3 + 3)		// // //
			m_cpCursorPos.m_iColumn -= 3;
	}
}

bool CPatternEditor::IsPlayCursorVisible() const
{
	if (m_iPlayFrame > (m_iCurrentFrame + 1))
		return false;

	if (m_iPlayFrame < (m_iCurrentFrame - 1))
		return false;

	if (m_iPlayFrame != (m_iCurrentFrame + 1) && m_iPlayFrame != (m_iCurrentFrame - 1)) {
		
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

	if ((PointPos.m_iRow - m_iCenterRow) > (m_iLinesFullVisible / 2) - 3) {
		if (m_cpCursorPos.m_iRow < m_iPatternLength && m_iCenterRow < (m_iPatternLength - (m_iLinesFullVisible / 2) + 2))
			m_iScrolling = SCROLL_DOWN;
		else
			m_iScrolling = SCROLL_NONE;
	}
	else if ((PointPos.m_iRow - m_iCenterRow) <= -(m_iLinesFullVisible / 2)) {
		if (m_cpCursorPos.m_iRow > 0 && m_iCenterRow > (m_iLinesFullVisible / 2))
			m_iScrolling = SCROLL_UP;
		else
			m_iScrolling = SCROLL_NONE;
	}
	else if (PointPos.m_iChannel >= (m_iFirstChannel + m_iChannelsVisible - 1) && m_iChannelsVisible < GetChannelCount()) {		// // //
		if (m_cpCursorPos.m_iChannel < Channels)
			m_iScrolling = SCROLL_RIGHT;
		else
			m_iScrolling = SCROLL_NONE;
	}
	else if (PointPos.m_iChannel < m_iFirstChannel) {
		if (m_cpCursorPos.m_iChannel > 0)
			m_iScrolling = SCROLL_LEFT;
		else
			m_iScrolling = SCROLL_NONE;
	}
	else
		m_iScrolling = SCROLL_NONE;
}

bool CPatternEditor::ScrollTimerCallback()
{
	const int Channels = GetChannelCount();

	switch (m_iScrolling) {
	case SCROLL_UP:
		m_cpCursorPos.m_iRow--;
		m_iCenterRow--;
		break;
	case SCROLL_DOWN:
		m_cpCursorPos.m_iRow++;
		m_iCenterRow++;
		break;
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
	default:
		return false;
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
			FirstChannel(); 
			break;
		case SB_LEFT: 
			LastChannel(); 
			break;
		case SB_THUMBPOSITION:
		case SB_THUMBTRACK:
			for (int i = 0; i < Channels; ++i) {
				for (int j = 0; j < GetChannelColumns(i); ++j) {
					if (count++ == nPos) {
						MoveToChannel(i);
						MoveToColumn(j);
						if (!m_bSelecting)
							CancelSelection();
						return;
					}
				}
			}
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

void CPatternEditor::GetVolumeColumn(CString &str) const
{
	// Copy the volume column as text

	const int Track = GetSelectedTrack();
	const int Channel = m_selection.GetChanStart();

	if (Channel < 0 || Channel >= GetChannelCount())
		return;
	
	str.Empty();

	int vol = 0;
	for (int i = std::max(m_selection.GetRowStart(), 0); i <= std::min(m_selection.GetRowEnd(), m_iPatternLength); ++i) {
		stChanNote NoteData;
		m_pDocument->GetNoteData(Track, m_iCurrentFrame, Channel, i, &NoteData);
		if (m_selection.IsColumnSelected(COLUMN_VOLUME, Channel)) {
			if (NoteData.Vol != MAX_VOLUME)
				vol = NoteData.Vol;
			str.AppendFormat(_T("%i "), vol);
		}
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

	if (m_selDrag.GetChanStart() >= Channels || m_selDrag.GetChanEnd() < 0 || m_selDrag.GetRowStart() >= m_iPatternLength || m_selDrag.GetRowEnd() < 0) {
		// Completely outside of visible area
		CancelSelection();
		return false;
	}

	if (m_selDrag.m_cpStart.m_iChannel < 0 || m_selDrag.m_cpStart.m_iRow < 0) {

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
			m_selDrag.m_cpStart.m_iColumn = 0;
		}

		m_selDrag.m_cpStart.m_iRow = std::max(m_selDrag.m_cpStart.m_iRow, 0);

		SAFE_RELEASE(pClipData);
		pClipData = pClipped;
	}

	if (m_selDrag.m_cpEnd.m_iChannel > Channels - 1) {
		m_selDrag.m_cpEnd.m_iChannel = Channels - 1;
		m_selDrag.m_cpEnd.m_iColumn = GetChannelColumns(Channels);
	}

	m_selDrag.m_cpEnd.m_iRow = std::min(m_selDrag.m_cpEnd.m_iRow, m_iPatternLength - 1);
	m_selDrag.m_cpEnd.m_iColumn = std::min(m_selDrag.m_cpEnd.m_iColumn, 15);	// TODO remove hardcoded number

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

	int ColumnStart = m_iDragStartCol;
	int ColumnEnd = m_iDragEndCol;

	if (m_iDragChannels == 0 && GetSelectColumn(m_iDragStartCol) >= COLUMN_EFF1) {
		// Allow dragging between effect columns in the same channel
		if (GetSelectColumn(PointPos.m_iColumn) >= COLUMN_EFF1) {
			ColumnStart = PointPos.m_iColumn - (((PointPos.m_iColumn - 1) % (MAX_EFFECT_COLUMNS - 1)));
		}
		else {
			ColumnStart = MAX_EFFECT_COLUMNS;
		}
		ColumnEnd = ColumnStart + (m_iDragEndCol - m_iDragStartCol);
	}

	m_selDrag.m_cpStart = CCursorPos(PointPos.m_iRow - m_iDragOffsetRow, PointPos.m_iChannel - m_iDragOffsetChannel, ColumnStart);
	m_selDrag.m_cpEnd = CCursorPos(m_iDragRows + PointPos.m_iRow - m_iDragOffsetRow, m_iDragChannels + PointPos.m_iChannel - m_iDragOffsetChannel, ColumnEnd);

	AutoScroll(point, 0);
}

bool CPatternEditor::IsShiftPressed() const
{
	return (::GetKeyState(VK_SHIFT) & 0x80) == 0x80;
}

bool CPatternEditor::IsControlPressed() const
{
	return (::GetKeyState(VK_CONTROL) & 0x80) == 0x80;
}

CMainFrame *CPatternEditor::GetMainFrame() const
{
	return static_cast<CMainFrame*>(m_pView->GetParentFrame());
}
