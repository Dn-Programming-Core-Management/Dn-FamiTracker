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
#include <vector>		// // //
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
#include "TextExporter.h"		// // //
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
const int CPatternEditor::ROW_HEIGHT		 = 12;

// Pattern header font
LPCTSTR CPatternEditor::DEFAULT_HEADER_FONT = _T("Tahoma");

const int CPatternEditor::DEFAULT_FONT_SIZE			= 12;
const int CPatternEditor::DEFAULT_HEADER_FONT_SIZE	= 11;

// // //

void CopyNoteSection(stChanNote *Target, stChanNote *Source, paste_mode_t Mode, int Begin, int End)		// // //
{
	bool Protected[7] = {};
	static const char Offset[7] = {0, 3, 2, 4, 5, 6, 7};
	for (int i = 0; i < 7; i++) {
		const unsigned char TByte = *(reinterpret_cast<unsigned char*>(Target) + Offset[i]); // skip octave byte
		const unsigned char SByte = *(reinterpret_cast<unsigned char*>(Source) + Offset[i]);
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
		int Temp = End; End = Begin; Begin = Temp;
	}

	for (int i = Begin; i <= End; i++) if (!Protected[i]) switch (i) {
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
	m_iCurrentFrame(0),
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
	m_vHighlight(CFamiTrackerDoc::DEFAULT_HIGHLIGHT),		// // //
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

	const CSettings *pSettings = theApp.GetSettings();

	LOGFONT LogFont;
	LPCTSTR	FontName = pSettings->Appearance.strFont;		// // //
	LPCTSTR	HeaderFace = DEFAULT_HEADER_FONT;

	COLORREF ColBackground = pSettings->Appearance.iColBackground;

	// Fetch font size
	m_iPatternFontSize = pSettings->Appearance.iFontSize;		// // //
	m_iCharWidth = m_iPatternFontSize - 1;
	m_iColumnSpacing = (m_iPatternFontSize + 1) / 3;
	m_iRowColumnWidth = m_iCharWidth * 3 + 2;
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

	// Adjust selection
	// 0CC: check if this can be removed
	if (m_bSelecting) {
		/*if (m_selection.GetRowStart() > m_iPatternLength - 1) {
			CancelSelection();
		}*/
		/*else if (m_selection.GetRowEnd() >= m_iPatternLength) {
			SetSelectionEnd(CCursorPos(m_iPatternLength - 1, m_selection.GetChanEnd(), m_selection.GetColEnd(), m_iCurrentFrame)); // // //
		}*/
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
	m_iLastFrame = m_iCurrentFrame;
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
	else if (theApp.GetMainWnd()->GetMenu()->GetMenuState(ID_TRACKER_DISPLAYREGISTERSTATE, MF_BYCOMMAND) == MF_CHECKED)
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

	for (int i = 0; i < m_iLinesVisible; ++i) {
		PrintRow(pDC, Row, i, m_iCurrentFrame);
		++Row;
	}

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

	int Offset = m_iRowColumnWidth;
	for (int i = m_iFirstChannel; i < m_iFirstChannel + m_iChannelsVisible; ++i) {
		pDC->FillSolidRect(Offset, Line * m_iRowHeight, m_iChannelWidths[i] - 1, m_iRowHeight, m_colEmptyBg);
		Offset += m_iChannelWidths[i];
	}

	// Row number
	pDC->FillSolidRect(1, Line * m_iRowHeight, m_iRowColumnWidth - 2, m_iRowHeight, m_colEmptyBg);
}

// // // gone

bool CPatternEditor::IsInRange(const CSelection &sel, int Frame, int Row, int Channel, int Column) const		// // //
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

	const CSettings *pSettings = theApp.GetSettings();

	COLORREF ColCursor	= theApp.GetSettings()->Appearance.iColCursor;
	COLORREF ColBg		= theApp.GetSettings()->Appearance.iColBackground;
	COLORREF ColHiBg	= theApp.GetSettings()->Appearance.iColBackgroundHilite;
	COLORREF ColHiBg2	= theApp.GetSettings()->Appearance.iColBackgroundHilite2;
	COLORREF ColSelect	= theApp.GetSettings()->Appearance.iColSelection;

	const bool bEditMode = m_pView->GetEditMode();

	const int Track = GetSelectedTrack();
	const int Channels = /*m_iFirstChannel +*/ m_iChannelsVisible;
	int OffsetX = m_iRowColumnWidth;

	stChanNote NoteData;

	// Start at row number column
	pDC->SetWindowOrg(0, 0);

	if (Frame != m_iCurrentFrame && !theApp.GetSettings()->General.bFramePreview) {
		ClearRow(pDC, Line);
		return;
	}

	// Highlight
	unsigned int Highlight = m_pDocument->GetHighlightAtRow(Track, Frame, Row);		// // //

	// Clear
	pDC->FillSolidRect(1, Line * m_iRowHeight, m_iRowColumnWidth - 2, m_iRowHeight, ColBg);
	if (Highlight >> 7)
		pDC->FillSolidRect(1, Line * m_iRowHeight, m_iRowColumnWidth - 2, m_iRowHeight, ColHiBg);
	Highlight &= 0x7F;

	COLORREF TextColor;

	if (Highlight == 2)
		TextColor = theApp.GetSettings()->Appearance.iColPatternTextHilite2;
	else if (Highlight == 1)
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
	pDC->SetTextAlign(TA_CENTER | TA_BASELINE);		// // //

	CString Text;

	if (theApp.GetSettings()->General.bRowInHex) {
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
		((Frame == m_iCurrentFrame) ? 100 : PREVIEW_SHADE_LEVEL));
	const COLORREF DragColor = DIM(BLEND(SEL_DRAG_COL, BackColor, SHADE_LEVEL.SELECT),
		((Frame == m_iCurrentFrame) ? 100 : PREVIEW_SHADE_LEVEL));
	const COLORREF SelectEdgeCol = (m_iSelectionCondition == SEL_CLEAN /* || m_iSelectionCondition == SEL_UNKNOWN_SIZE*/ ) ?
		DIM(BLEND(SelectColor, 0xFFFFFF, SHADE_LEVEL.SELECT_EDGE), ((Frame == m_iCurrentFrame) ? 100 : PREVIEW_SHADE_LEVEL)) :
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
		int Columns	 = m_bCompactMode ? 1 : GetChannelColumns(i);		// // //
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
		for (int j = 0; j < Columns; ++j) {
			int SelWidth = GetSelectWidth(j);		// // //

			// Selection
			if (m_bSelecting) {		// // //
				if (IsInRange(m_selection, Frame, Row, i, j)) {		// // //
					pDC->FillSolidRect(SelStart - m_iColumnSpacing, 0, SelWidth, m_iRowHeight, SelectColor);

					// Outline
					if (Row == m_selection.GetRowStart() && !((f - m_selection.GetFrameStart()) % GetFrameCount()))
						pDC->FillSolidRect(SelStart - m_iColumnSpacing, 0, SelWidth, BorderWidth, SelectEdgeCol);
					if (Row == m_selection.GetRowEnd() && !((f - m_selection.GetFrameEnd()) % GetFrameCount()))
						pDC->FillSolidRect(SelStart - m_iColumnSpacing, m_iRowHeight - BorderWidth, SelWidth, BorderWidth, SelectEdgeCol);
					if (i == m_selection.GetChanStart() && j == m_selection.GetColStart())
						pDC->FillSolidRect(SelStart - m_iColumnSpacing, 0, BorderWidth, m_iRowHeight, SelectEdgeCol);
					if (i == m_selection.GetChanEnd() && j == m_selection.GetColEnd())
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
			SelStart += GetSelectWidth(j);
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
	COLORREF DimInst = pColorInfo->Compact;		// // //
	COLORREF DimEff = pColorInfo->Compact;		// // //

	// Make non-available instruments red in the pattern editor
	if (pNoteData->Instrument < MAX_INSTRUMENTS && 
		(!m_pDocument->IsInstrumentUsed(pNoteData->Instrument) || !pTrackerChannel->IsInstrumentCompatible(pNoteData->Instrument, m_pDocument))) {
		DimInst = InstColor = RED(255);
	}

	// // // effects too
	if (EffNumber != EF_NONE) if (!pTrackerChannel->IsEffectCompatible(EffNumber, EffParam))
		DimEff = EffColor = RED(255);		// // //

	int PosY = m_iRowHeight - m_iRowHeight / 8;		// // //
	// // // PosX -= 1;

#define BARLENGTH (m_iRowHeight > 6 ? 4 : 2)		// // //
#define BAR(x, y) pDC->FillSolidRect((x) + m_iCharWidth / 2 - BARLENGTH / 2, (y) - m_iRowHeight / 2 + m_iRowHeight / 8, BARLENGTH, 1, pColorInfo->Shaded)

	pDC->SetTextAlign(TA_CENTER | TA_BASELINE);		// // //

	switch (Column) {
		case 0:
			// Note and octave
			switch (pNoteData->Note) {
				case NONE:
					if (m_bCompactMode) {		// // //
						if (pNoteData->Instrument != MAX_INSTRUMENTS) {
							DrawChar(pDC, PosX + m_iCharWidth * 3 / 2, PosY, HEX[pNoteData->Instrument >> 4], DimInst);
							DrawChar(pDC, PosX + m_iCharWidth * 5 / 2, PosY, HEX[pNoteData->Instrument & 0x0F], DimInst);
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
									DrawChar(pDC, PosX + m_iCharWidth / 2, PosY, EFF_CHAR[pNoteData->EffNumber[i] - 1], DimEff);
									DrawChar(pDC, PosX + m_iCharWidth * 3 / 2, PosY, HEX[pNoteData->EffParam[i] >> 4], DimEff);
									DrawChar(pDC, PosX + m_iCharWidth * 5 / 2, PosY, HEX[pNoteData->EffParam[i] & 0x0F], DimEff);
									Found = true;
									break;
								}
							}
							if (Found) break;
						}
						BAR(PosX, PosY);
						BAR(PosX + m_iCharWidth, PosY);
						BAR(PosX + m_iCharWidth * 2, PosY);
					}
					else {
						BAR(PosX, PosY);
						BAR(PosX + m_iCharWidth, PosY);
						BAR(PosX + m_iCharWidth * 2, PosY);
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
		case 1:
			// Instrument x0
			if (pNoteData->Instrument == MAX_INSTRUMENTS || pNoteData->Note == HALT || pNoteData->Note == RELEASE)
				BAR(PosX, PosY);
			else
				DrawChar(pDC, PosX + m_iCharWidth / 2, PosY, HEX[pNoteData->Instrument >> 4], InstColor);		// // //
			break;
		case 2:
			// Instrument 0x
			if (pNoteData->Instrument == MAX_INSTRUMENTS || pNoteData->Note == HALT || pNoteData->Note == RELEASE)
				BAR(PosX, PosY);
			else
				DrawChar(pDC, PosX + m_iCharWidth / 2, PosY, HEX[pNoteData->Instrument & 0x0F], InstColor);		// // //
			break;
		case 3: 
			// Volume
			if (pNoteData->Vol == MAX_VOLUME || pTrackerChannel->GetID() == CHANID_DPCM)
				BAR(PosX, PosY);
			else 
				DrawChar(pDC, PosX + m_iCharWidth / 2, PosY, HEX[pNoteData->Vol & 0x0F], pColorInfo->Volume);		// // //
			break;
		case 4: case 7: case 10: case 13:
			// Effect type
			if (EffNumber == 0)
				BAR(PosX, PosY);
			else {
				DrawChar(pDC, PosX + m_iCharWidth / 2, PosY, EFF_CHAR[EffNumber - 1], EffColor);		// // //
			}
			break;
		case 5: case 8: case 11: case 14:
			// Effect param x
			if (EffNumber == 0)
				BAR(PosX, PosY);
			else
				DrawChar(pDC, PosX + m_iCharWidth / 2, PosY, HEX[(EffParam >> 4) & 0x0F], pColorInfo->Note);		// // //
			break;
		case 6: case 9: case 12: case 15:
			// Effect param y
			if (EffNumber == 0)
				BAR(PosX, PosY);
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
	GradientRectTriple(pDC, 0, HEADER_CHAN_START, m_iWinWidth, HEADER_CHAN_HEIGHT, m_colHead1, m_colHead2, m_pView->GetEditMode() ? m_colHead4 : m_colHead3);		// // //

	// Corner box
	pDC->Draw3dRect(0, HEADER_CHAN_START, m_iRowColumnWidth, HEADER_CHAN_HEIGHT, STATIC_COLOR_SCHEME.FRAME_LIGHT, STATIC_COLOR_SCHEME.FRAME_DARK);

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
				case SNDCHIP_N163: pChanName.Format(_T("N%d"), Type - CHANID_N163_CH1 + 1); break;
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
		if (m_bCompactMode)		// // //
			pDC->SetTextAlign(TA_CENTER);

		pDC->SetTextColor(BLEND(HeadTextCol, 0x00FFFFFF, SHADE_LEVEL.TEXT_SHADOW));
		pDC->TextOut(Offset + (m_bCompactMode ? GetColumnSpace(0) / 2 : 10) + 1, HEADER_CHAN_START + 6 + (bMuted ? 1 : 0), pChanName);
		
		// Foreground
		if (m_iMouseHoverChan == Channel)
			HeadTextCol = BLEND(HeadTextCol, 0x0000FFFF, SHADE_LEVEL.HOVER);
		pDC->SetTextColor(HeadTextCol);
		pDC->TextOut(Offset + (m_bCompactMode ? GetColumnSpace(0) / 2 : 10), HEADER_CHAN_START + 5, pChanName);		// // //
		
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

	const int BAR_TOP	 = 5 + 18 + HEADER_CHAN_START;
	const int BAR_SIZE	 = m_bCompactMode ? (GetColumnSpace(0) - 2) / 16 : (GetChannelWidth(0) - 6) / 16;		// // //
	const int BAR_LEFT	 = m_bCompactMode ? m_iRowColumnWidth + (GetColumnSpace(0) - 16 * BAR_SIZE + 3) / 2 : m_iRowColumnWidth + 7;
	const int BAR_SPACE	 = 1;
	const int BAR_HEIGHT = 5;

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
	if (!m_pDocument || !pDC) return;		// // //

	// Display 2a03 registers
	const CSoundGen *pSoundGen = theApp.GetSoundGenerator();

	unsigned char reg[8] = {};
	int line = 0, vis_line = 0;
	CFont *pOldFont = pDC->SelectObject(&m_fontCourierNew);
	pDC->FillSolidRect(0, 0, m_iWinWidth, m_iWinHeight, m_colEmptyBg);		// // //

	if (theApp.GetMainWnd()->GetMenu()->GetMenuState(ID_TRACKER_DISPLAYREGISTERSTATE, MF_BYCOMMAND) == MF_UNCHECKED) { // // //
		pDC->SelectObject(pOldFont);
		return;
	}

	CString text(_T("2A03 registers"));
	pDC->SetBkColor(m_colEmptyBg);
	pDC->SetTextColor(0xFFAFAF);
	pDC->SetBkMode(TRANSPARENT);		// // //
	pDC->TextOut(30, 30 + (line++) * 13, text);		// // //

	vis_line = 15 + m_pDocument->ExpansionEnabled(SNDCHIP_VRC6) * 6
				  + m_pDocument->ExpansionEnabled(SNDCHIP_MMC5) * 5
				  + m_pDocument->ExpansionEnabled(SNDCHIP_N163) * 22
				  + m_pDocument->ExpansionEnabled(SNDCHIP_FDS) * 16
				  + m_pDocument->ExpansionEnabled(SNDCHIP_VRC7) * 11
				  + m_pDocument->ExpansionEnabled(SNDCHIP_S5B) * 10;		// // //

	// 2A03
	for (int i = 0; i < 5; ++i) {
		for (int j = 0; j < 4; j++)		// // //
			reg[j] = pSoundGen->GetReg(SNDCHIP_NONE, i * 4 + j);

		pDC->SetBkColor(m_colEmptyBg);
		pDC->SetTextColor(0xC0C0C0);

		int x = 30;		// // //
		int y = 30 + line++ * 13;

		text.Format(_T("$%04X: $%02X $%02X $%02X $%02X"), 0x4000 + i * 4, reg[0], reg[1], reg[2], reg[3]);
		pDC->TextOut(x, y, text);

		int period = (reg[2] | ((reg[3] & 7) << 8));
		int vol = (reg[0] & 0x0F);

		double freq;		// // //
		if (i == 4)
			freq = 236250000.0 / 1056.0 / CDPCM::DMC_PERIODS_NTSC[reg[0] & 0x0F];
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
					period, freq, NoteToStr(note_conv), cents, vol, reg[0] >> 6);
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
				text.Format(_T("pitch = $%01X, vol = %02i, mode = %i"), reg[2] & 0x0F, vol, reg[2] >> 7);		// // //
				break;
			case 4:
				text.Format(_T("pitch = $%01X"), reg[0] & 0x0F);		// // //
				if (reg[0] & 0x40) {
					text.AppendFormat(_T(" (%7.2fBps %s %+03i)"),		// // //
						freq, NoteToStr(note_conv), cents);
				}
				text.AppendFormat(_T(", size = %i byte%c"), (reg[3] << 4) | 1, reg[3] ? 's' : ' ');
				break;
			default:
				text.Format(_T(""));
		}

		pDC->TextOut(x + 180, y, text);		// // //

		if (i == 2)
			vol = (reg[0] != 0) ? 15 : 0;
		else if (i == 3) {
			period = ((reg[2] & 15) << 4) | ((reg[2] & 0x80) << 1);
		}
		else if (i == 4) {
			period = (reg[0] & 0x0F) << 4;
			vol = 15 * !pSoundGen->PreviewDone();
		}
/*
		pDC->FillSolidRect(250 + i * 30, 0, 20, m_iWinHeight - HEADER_CHAN_HEIGHT, 0);
		pDC->FillSolidRect(250 + i * 30, (period >> 1), 20, 5, RGB(vol << 4, vol << 4, vol << 4));
*/
		DrawNoteBar(pDC, 30, vis_line * 10);
		if (note_conv >= -12 && note_conv <= 96 && vol)		// // //
			pDC->FillSolidRect(29 + 6 * (note_conv + 12), vis_line++ * 10, 3, 7, RGB(vol << 4, vol << 4, vol << 4));
		else vis_line++;
	}

	text.Format(_T("position: %02i, delta = $%02X"), m_DPCMState.SamplePos, m_DPCMState.DeltaCntr);		// // //
	pDC->TextOut(30 + 180, 30 + line++ * 13, text);

	if (m_pDocument->ExpansionEnabled(SNDCHIP_VRC6)) {

		line++;

		CString text(_T("VRC6 registers"));
		pDC->SetBkColor(m_colEmptyBg);
		pDC->SetTextColor(0xFFAFAF);
		pDC->TextOut(30, 30 + (line++) * 13, text);		// // //

		// VRC6
		for (int i = 0; i < 3; ++i) {
			for (int j = 0; j < 3; j++)
				reg[j] = pSoundGen->GetReg(SNDCHIP_VRC6, i * 3 + j);

			pDC->SetBkColor(m_colEmptyBg);
			pDC->SetTextColor(0xC0C0C0);

			int x = 30;		// // //
			int y = 30 + line++ * 13;

			text.Format(_T("$%04X: $%02X $%02X $%02X"), 0x9000 + i * 0x1000, reg[0], reg[1], reg[2]);
			pDC->TextOut(x, y, text);

			int period = (reg[1] | ((reg[2] & 15) << 8));
			int vol = (reg[0] & (i == 2 ? 0x3F : 0x0F));

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
						period, freq, NoteToStr(note_conv), cents, vol, (reg[0] >> 4) & 0x07);
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
				vol = reg[0] >> 1;
			
			DrawNoteBar(pDC, 30, vis_line * 10);
			if (note_conv >= -12 && note_conv <= 96 && vol)		// // //
				pDC->FillSolidRect(29 + 6 * (note_conv + 12), vis_line++ * 10, 3, 7, RGB(vol << 4, vol << 4, vol << 4));
			else vis_line++;
		}
	}

	if (m_pDocument->ExpansionEnabled(SNDCHIP_MMC5)) {		// // //

		line++;

		CString text(_T("MMC5 registers"));
		pDC->SetBkColor(m_colEmptyBg);
		pDC->SetTextColor(0xFFAFAF);
		pDC->TextOut(30, 30 + (line++) * 13, text);

		// MMC5
		for (int i = 0; i < 2; ++i) {
			for (int j = 0; j < 4; j++)
				reg[j] = pSoundGen->GetReg(SNDCHIP_MMC5, i * 4 + j);

			pDC->SetBkColor(m_colEmptyBg);
			pDC->SetTextColor(0xC0C0C0);

			int x = 30;
			int y = 30 + line++ * 13;

			text.Format(_T("$%04X: $%02X $%02X $%02X $%02X"), 0x5000 + i * 4, reg[0], reg[1], reg[2], reg[3]);
			pDC->TextOut(x, y, text);
			
			int period = (reg[2] | ((reg[3] & 7) << 8));
			int vol = (reg[0] & 0x0F);

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
				period, freq, NoteToStr(note_conv), cents, vol, reg[0] >> 6);
			pDC->TextOut(x + 180, y, text);
			
			DrawNoteBar(pDC, 30, vis_line * 10);
			if (note_conv >= -12 && note_conv <= 96 && vol)
				pDC->FillSolidRect(29 + 6 * (note_conv + 12), vis_line++ * 10, 3, 7, RGB(vol << 4, vol << 4, vol << 4));
			else vis_line++;
		}
	}

	if (m_pDocument->ExpansionEnabled(SNDCHIP_N163)) {

		line++;

		CString text(_T("N163 registers"));
		pDC->SetBkColor(m_colEmptyBg);
		pDC->SetTextColor(0xFFAFAF);
		pDC->TextOut(30, 30 + (line++) * 13, text);		// // //

		// // // N163 wave
		int Length = 0x80 - 8 * m_pDocument->GetNamcoChannels();
		int x = 30 + 300;		// // //
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
			for (int j = 0; j < 8; j++)		// // //
				reg[j] = pSoundGen->GetReg(SNDCHIP_N163, i * 8 + j);

			int period = (reg[0] | (reg[2] << 8) | ((reg[4] & 0x03) << 16));
			int vol = (reg[7] & 0x0F);

			pDC->SetBkColor(m_colEmptyBg);
			pDC->SetTextColor(0xC0C0C0);

			int x = 30;		// // //
			int y = 30 + line++ * 13;

			text.Format(_T("$%02X: $%02X $%02X $%02X $%02X $%02X $%02X $%02X $%02X"), i * 8, reg[0], reg[1], reg[2], reg[3], reg[4], reg[5], reg[6], reg[7]);
			pDC->TextOut(x, y, text);

			if (i < 16 - m_pDocument->GetNamcoChannels()) continue;		// // //

			double freq = RegToFreqN163(period, m_pDocument->GetNamcoChannels() * (256 - (reg[4] & 0xFC)));		// // //
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
			for (int j = 0; j < 8; j++)		// // //
				reg[j] = pSoundGen->GetReg(SNDCHIP_N163, 0x78 - i * 8 + j);
			int period = (reg[0] | (reg[2] << 8) | ((reg[4] & 0x03) << 16));
			int vol = (reg[7] & 0x0F);

			double freq = RegToFreqN163(period, m_pDocument->GetNamcoChannels() * (256 - (reg[4] & 0xFC)));
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
				
			DrawNoteBar(pDC, 30, vis_line * 10);
			if (note_conv >= -12 && note_conv <= 96 && vol)
				pDC->FillSolidRect(29 + 6 * (note_conv + 12), vis_line++ * 10, 3, 7, RGB(vol << 4, vol << 4, vol << 4));			
			else vis_line++;
		}
	}

	if (m_pDocument->ExpansionEnabled(SNDCHIP_FDS)) {

		line++;

		CString text(_T("FDS registers"));
		pDC->SetBkColor(m_colEmptyBg);
		pDC->SetTextColor(0xFFAFAF);
		pDC->TextOut(30, 30 + (line++) * 13, text);		// // //

		for (int i = 0; i < 11; ++i) {
			reg[0] = pSoundGen->GetReg(SNDCHIP_FDS, i);

			pDC->SetBkColor(m_colEmptyBg);
			pDC->SetTextColor(0xC0C0C0);

			int x = 30;		// // //
			int y = 30 + line++ * 13;

			text.Format(_T("$%04X: $%02X"), 0x4080 + i, reg[0]);
			pDC->TextOut(x, y, text);
		}

		int period = pSoundGen->GetReg(SNDCHIP_FDS, 2) | ((pSoundGen->GetReg(SNDCHIP_FDS, 3) & 0x0F) << 8);
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
		pDC->TextOut(30 + 180, HEADER_HEIGHT + 30 + (line - 11) * 13, text);		// // //
		
		DrawNoteBar(pDC, 30, vis_line * 10);		// // //
		if (note_conv >= -12 && note_conv <= 96 && vol) {
			if (vol == 32)
				pDC->FillSolidRect(29 + 6 * (note_conv + 12), vis_line++ * 10, 3, 7, 0xFFFFFF);
			else
				pDC->FillSolidRect(29 + 6 * (note_conv + 12), vis_line++ * 10, 3, 7, RGB(vol << 3, vol << 3, vol << 3));
		}
		else vis_line++;
	}

	if (m_pDocument->ExpansionEnabled(SNDCHIP_VRC7)) {		// // //

		line++;

		CString text(_T("VRC7 registers"));
		pDC->SetBkColor(m_colEmptyBg);
		pDC->SetTextColor(0xFFAFAF);
		pDC->TextOut(30, 30 + (line++) * 13, text);
		
		for (int j = 0; j < 8; j++)
			reg[j] = pSoundGen->GetReg(SNDCHIP_VRC7, j);

		pDC->SetBkColor(m_colEmptyBg);
		pDC->SetTextColor(0xC0C0C0);

		text.Format(_T("$00: $%02X $%02X $%02X $%02X $%02X $%02X $%02X $%02X"), reg[0], reg[1], reg[2], reg[3], reg[4], reg[5], reg[6], reg[7]);
		pDC->TextOut(30, 30 + (line++) * 13, text);

		for (int i = 0; i < 6; ++i) {
			for (int j = 0; j < 3; j++)
				reg[j] = pSoundGen->GetReg(SNDCHIP_VRC7, i + 0x10 + 0x10 * j);

			pDC->SetBkColor(m_colEmptyBg);
			pDC->SetTextColor(0xC0C0C0);

			int x = 30;
			int y = 30 + line++ * 13;

			text.Format(_T("$x%01X: $%02X $%02X $%02X"), i, reg[0], reg[1], reg[2]);
			pDC->TextOut(x, y, text);

			int period = reg[0] | ((reg[1] & 0x01) << 8);
			int octave = (reg[1] & 0x0E) >> 1;
			int vol = 0x0F - (pSoundGen->GetReg(SNDCHIP_VRC7, i + 0x30) & 0x0F);
			int inst = reg[2] >> 4;

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
			
			DrawNoteBar(pDC, 30, vis_line * 10);
			if (note_conv >= -12 && note_conv <= 96 && vol)
				pDC->FillSolidRect(29 + 6 * (note_conv + 12), vis_line++ * 10, 3, 7, RGB(vol << 4, vol << 4, vol << 4));
			else vis_line++;
		}
	}

	if (m_pDocument->ExpansionEnabled(SNDCHIP_S5B)) {		// // //

		line++;

		CString text(_T("5B registers"));
		pDC->SetBkColor(m_colEmptyBg);
		pDC->SetTextColor(0xFFAFAF);
		pDC->TextOut(30, 30 + (line++) * 13, text);

		// S5B
		for (int i = 0; i < 4; ++i) {
			reg[0] = pSoundGen->GetReg(SNDCHIP_S5B, i * 2 + 0);
			reg[1] = pSoundGen->GetReg(SNDCHIP_S5B, i * 2 + 1);

			pDC->SetBkColor(m_colEmptyBg);
			pDC->SetTextColor(0xC0C0C0);

			int x = 30;
			int y = 30 + line++ * 13;

			text.Format(_T("$%02X: $%02X $%02X"), i * 2, reg[0], reg[1]);
			pDC->TextOut(x, y, text);

			int period = (reg[0] | ((reg[1] & 0x0F) << 8));
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
				text.Format(_T("pitch = $%02X"), reg[0] & 0x1F);
			}
			pDC->TextOut(x + 180, y, text);
			if (i < 3) {
				DrawNoteBar(pDC, 30, vis_line * 10);
				if (note_conv >= -12 && note_conv <= 96 && vol)		// // //
					pDC->FillSolidRect(29 + 6 * (note_conv + 12), vis_line++ * 10, 3, 7, RGB(vol << 4, vol << 4, vol << 4));
				else vis_line++;
			}
		}

		for (int i = 0; i < 2; ++i) {
			for (int j = 0; j < 3; j++)
				reg[j] = pSoundGen->GetReg(SNDCHIP_S5B, i * 3 + 8 + j);

			pDC->SetBkColor(m_colEmptyBg);
			pDC->SetTextColor(0xC0C0C0);

			int x = 30;
			int y = 30 + line++ * 13;

			text.Format(_T("$%02X: $%02X $%02X $%02X"), i * 3 + 8, reg[0], reg[1], reg[2]);
			pDC->TextOut(x, y, text);
			
			if (i == 1) {
				pDC->SetTextColor(0x808080);
				int period = (reg[0] | (reg[1] << 8));
				double freq, note;
				int note_conv, cents;
				if ((reg[2] & 0x08) && !(reg[2] & 0x01) && reg[0] && !reg[1]) {
					freq = RegToFreq(period - 1, SNDCHIP_NONE) / 2;
					if (reg[2] & 0x02) freq /= 32;	// triangle
					else freq /= 16;				// sawtooth
					note = NoteFromFreq(freq);
					note_conv = note >= 0 ? int(note + 0.5) : int(note - 0.5);
					cents = int((note - double(note_conv)) * 100.0);

					text.Format(_T("pitch = $%04X (%7.2fHz %s %+03i), shape = $%01X"),
						period, freq, NoteToStr(note_conv), cents, reg[2]);
				}
				else
					text.Format(_T("period = $%04X, shape = $%01X"), period, reg[2]);

				pDC->TextOut(x + 180, y, text);
			}
		}
	}

	pDC->SelectObject(pOldFont);

	// Surrounding frame
//	pDC->Draw3dRect(20, 20, 200, line * 18 + 20, 0xA0A0A0, 0x505050);		// // //

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

unsigned int CPatternEditor::GetColumnWidth(int Column) const		// // //
{
	return m_iCharWidth * (!Column ? 3 : 1);
}

unsigned int CPatternEditor::GetColumnSpace(int Column) const		// // //
{
	int x = GetColumnWidth(Column);
	for (int i = 0; i < 7; i++)
		if (Column == CPatternEditor::GetCursorEndColumn(i)) return x + m_iColumnSpacing;
	return x;
}

unsigned int CPatternEditor::GetSelectWidth(int Column) const		// // //
{
	int x = GetColumnWidth(Column);
	for (int i = 0; i < 7; i++)
		if (Column == CPatternEditor::GetCursorStartColumn(i)) return x + m_iColumnSpacing;
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

int CPatternEditor::GetColumnAtPoint(int PointX) const
{
	// Convert X position to column number
	const int ChannelCount = GetChannelCount();
	const int Channel = GetChannelAtPoint(PointX);

	if (Channel < 0)
		return 0;
	if (Channel >= ChannelCount)
		return GetChannelColumns(ChannelCount - 1) - 1;

	const int Offset = PointX - m_iRowColumnWidth + m_iChannelOffsets[m_iFirstChannel];
	int ColumnOffset = m_iChannelOffsets[Channel];
	for (int i = 0; i < GetChannelColumns(Channel); ++i) {
		ColumnOffset += GetColumnSpace(i);		// // //
		if (Offset <= ColumnOffset)
			return i;
	}

	return GetChannelColumns(Channel);
}

CCursorPos CPatternEditor::GetCursorAtPoint(const CPoint &point) const
{
	// // // Removed GetRowAtPoint and GetFrameAtPoint
	int Frame = m_iCurrentFrame;
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

CPatternIterator CPatternEditor::GetStartIterator() const		// // //
{
	const CCursorPos Pos(m_selection.GetRowStart(), m_selection.GetChanStart(), m_selection.GetColStart(), m_selection.GetFrameStart());
	return CPatternIterator(this, GetSelectedTrack(), m_bSelecting ? Pos : m_cpCursorPos);
}

CPatternIterator CPatternEditor::GetEndIterator() const
{
	const CCursorPos Pos(m_selection.GetRowEnd(), m_selection.GetChanEnd(), m_selection.GetColEnd(), m_selection.GetFrameEnd());
	return CPatternIterator(this, GetSelectedTrack(), m_bSelecting ? Pos : m_cpCursorPos);
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

int CPatternEditor::GetCursorStartColumn(int Column)
{
	static const int COL_START[] = {
		0, 1, 3, 4, 7, 10, 13
	};

	ASSERT(Column >= 0 && Column < sizeof(COL_START) / sizeof(int));		// // //

	return COL_START[Column];
}

int CPatternEditor::GetCursorEndColumn(int Column)
{
	static const int COL_END[] = {
		0, 2, 3, 6, 9, 12, 15
	};

	ASSERT(Column >= 0 && Column < sizeof(COL_END) / sizeof(int));		// // //

	return COL_END[Column];
}

int CPatternEditor::GetChannelColumns(int Channel) const
{
	// Return number of available columns in a channel
	return m_bCompactMode ? 1 : m_pDocument->GetEffColumns(GetSelectedTrack(), Channel) * 3 + COLUMNS;		// // //
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

void CPatternEditor::UpdateSelection()
{
	// Call after cursor has moved
	// If shift is not pressed, set selection starting point to current cursor position
	// If shift is pressed, update selection end point

	const bool bShift = IsShiftPressed();

	if (bShift) {
		if (!m_bCurrentlySelecting && !m_bSelecting) {
			//SetSelectionStart(m_cpCursorPos);		// // //
		}

		m_bCurrentlySelecting = true;
		m_bSelecting = true;
		SetSelectionEnd(m_cpCursorPos);
		m_iSelectionCondition = GetSelectionCondition();		// // //
		m_bSelectionInvalidated = true;
	}
	else {
		m_bCurrentlySelecting = false;

		if (theApp.GetSettings()->General.iEditStyle != EDIT_STYLE_IT || m_bSelecting == false)
			CancelSelection();
	}

	const int Frames = GetFrameCount();
	if (m_selection.GetFrameEnd() - m_selection.GetFrameStart() > Frames ||		// // //
		(m_selection.GetFrameEnd() - m_selection.GetFrameStart() == Frames &&
		m_selection.GetRowEnd() >= m_selection.GetRowStart())) { // selection touches itself
			if (m_selection.m_cpEnd.m_iFrame >= Frames) {
				m_selection.m_cpEnd.m_iFrame -= Frames;
				m_iWarpCount = 0;
			}
			if (m_selection.m_cpEnd.m_iFrame < 0) {
				m_selection.m_cpEnd.m_iFrame += Frames;
				m_iWarpCount = 0;
			}
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

	CancelSelection();		// // //
}

void CPatternEditor::PreviousChannel()
{
	MoveToChannel(m_cpCursorPos.m_iChannel - 1);
	m_cpCursorPos.m_iColumn = 0;

	CancelSelection();		// // //
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

	if (bControl || theApp.GetSettings()->General.iEditStyle == EDIT_STYLE_FT2) {
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

	if (bControl || theApp.GetSettings()->General.iEditStyle == EDIT_STYLE_FT2) {
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
	if (theApp.IsPlaying() && m_bFollowMode)
		return;

	if (m_cpCursorPos.m_iFrame == 0 && m_cpCursorPos.m_iRow >= 0 && Row < 0) {		// // //
		m_iWarpCount--;
		if (!m_bSelecting && IsShiftPressed()) // special case
			m_bSelecting = true;
	}
	if (m_cpCursorPos.m_iFrame == GetFrameCount() - 1 && m_cpCursorPos.m_iRow < m_iPatternLength && Row >= m_iPatternLength) {
		m_iWarpCount++;
		if (!m_bSelecting && IsShiftPressed())
			m_bSelecting = true;
	}

	if (theApp.GetSettings()->General.bWrapFrames) {		// // //
		while (Row < 0) {
			MoveToFrame(m_iCurrentFrame - 1);
			Row += m_iPatternLength;
		}
		while (Row >= m_iPatternLength) {
			Row -= m_iPatternLength;
			MoveToFrame(m_iCurrentFrame + 1);
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

	if (!((m_iCurrentFrame - Frame) % FrameCount))
		return;

	/*if (m_bSelecting) {		// // //
		if (Frame < 0)
			m_iWarpCount--;
		else if (Frame / FrameCount > m_iCurrentFrame / FrameCount)
			m_iWarpCount++;
	}
	else
		m_iWarpCount = 0;*/

	if (theApp.GetSettings()->General.bWrapFrames) {
		Frame %= FrameCount;
		if (Frame < 0)
			Frame += FrameCount;
	}
	else
		Frame = std::min(std::max(Frame, 0), FrameCount - 1);
	
	if (theApp.IsPlaying() && m_bFollowMode) {
		if (m_iPlayFrame != Frame) {
			theApp.GetSoundGenerator()->MoveToFrame(Frame);
			theApp.GetSoundGenerator()->ResetTempo();
		}
	}

	m_cpCursorPos.m_iFrame = m_iCurrentFrame = Frame;		// // //
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
			m_selection.m_cpStart = CCursorPos(PointPos.m_iRow, 0, 0, PointPos.m_iFrame);		// // //
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
					PointPos.m_iColumn = 0;
				}
				SetSelectionStart(PointPos);
				SetSelectionEnd(PointPos);
			}
			else
				m_cpDragPoint = CCursorPos(PointPos.m_iRow, PointPos.m_iChannel, GetSelectColumn(PointPos.m_iColumn), PointPos.m_iFrame); // // //

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
				m_iSelectionCondition = GetSelectionCondition();
				m_bSelectionInvalidated = true;
				return;
			}
			m_cpCursorPos.m_iRow = PointPos.m_iRow;
			m_cpCursorPos.m_iFrame = m_iCurrentFrame = PointPos.m_iFrame;		// // //
			m_iDragBeginWarp = 0;		// // //
			return;
		}

		if (m_bDragStart && !m_bDragging) {
			m_bDragStart = false;
			CancelSelection();
		}

		if (m_bSelecting) {
			m_iSelectionCondition = GetSelectionCondition();		// // //
			m_bSelectionInvalidated = true;
			return;
		}

		if (PointPos.IsValid(PatternLength, ChannelCount)) {		// // //
			m_cpCursorPos = PointPos;
			m_iCurrentFrame = PointPos.m_iFrame;		// // //
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
		if (bControl) {
			if (PointPos.m_iChannel >= m_selection.m_cpStart.m_iChannel) {
				PointPos.m_iColumn = GetChannelColumns(PointPos.m_iChannel) - 1;		// // //
				m_selection.m_cpStart.m_iColumn = 0;
				m_bSelectionInvalidated = true;
			}
			else {
				PointPos.m_iColumn = 0;
				m_selection.m_cpStart.m_iColumn = GetChannelColumns(m_selection.m_cpStart.m_iChannel) - 1;		// // //
				m_bSelectionInvalidated = true;
			}
		}

		// Full row selection
		if (m_bFullRowSelect) {
			m_selection.m_cpEnd.m_iRow = PointPos.m_iRow;
			m_selection.m_cpEnd.m_iFrame = PointPos.m_iFrame;		// // //
			m_selection.m_cpEnd.m_iColumn = GetChannelColumns(GetChannelCount() - 1) - 1;
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
			if (m_selection.m_cpEnd.m_iFrame > m_selection.m_cpStart.m_iFrame) {
				m_selection.m_cpEnd.m_iFrame = m_selection.m_cpStart.m_iFrame;
				m_selection.m_cpEnd.m_iRow = GetCurrentPatternLength(m_iCurrentFrame) - 1;
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
				if (m_iCurrentFrame == 0 && m_bSelecting) m_iDragBeginWarp++;
				MoveToFrame(m_iCurrentFrame - 1);
				m_cpCursorPos.m_iRow += m_iPatternLength;
			}
			while (m_cpCursorPos.m_iRow > (m_iPatternLength - 1)) {
				m_cpCursorPos.m_iRow -= m_iPatternLength;
				MoveToFrame(m_iCurrentFrame + 1);
				if (m_iCurrentFrame == 0 && m_bSelecting) m_iDragBeginWarp--;
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
	CPatternIterator it = GetStartIterator();		// // //
	const int Channels	= m_selection.GetChanEnd() - m_selection.GetChanStart() + 1;
	const int Rows		= GetSelectionSize();		// // //
	stChanNote NoteData;

	CPatternClipData *pClipData = new CPatternClipData(Channels, Rows);
	pClipData->ClipInfo.Channels	= Channels;		// // //
	pClipData->ClipInfo.Rows		= Rows;
	pClipData->ClipInfo.StartColumn	= GetSelectColumn(m_selection.GetColStart());		// // //
	pClipData->ClipInfo.EndColumn	= GetSelectColumn(m_selection.GetColEnd());		// // //
	
	int Channel = 0, Row = 0;
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
		it++;
	}

	return pClipData;
}

CPatternClipData *CPatternEditor::CopyRaw() const		// // //
{
	CPatternIterator it = GetStartIterator();
	CPatternIterator end = GetEndIterator();
	const int Track		= GetSelectedTrack();
	const int Frames	= m_pDocument->GetFrameCount(Track);
	const int Length	= m_pDocument->GetPatternLength(Track);
	const int Rows		= (end.m_iFrame - it.m_iFrame) * Length + (end.m_iRow - it.m_iRow) + 1;

	const int cBegin	= m_selection.GetChanStart();
	const int Channels	= m_selection.GetChanEnd() - cBegin + 1;

	CPatternClipData *pClipData = new CPatternClipData(Channels, Rows);
	pClipData->ClipInfo.Channels	= Channels;
	pClipData->ClipInfo.Rows		= Rows;
	pClipData->ClipInfo.StartColumn	= GetSelectColumn(m_selection.GetColStart());
	pClipData->ClipInfo.EndColumn	= GetSelectColumn(m_selection.GetColEnd());
	
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
	for (int i = 0; i < pClipData->ClipInfo.Channels; ++i) {
		for (int j = 0; j < pClipData->ClipInfo.Rows; ++j) {
			m_pDocument->SetNoteData(Track, m_iCurrentFrame, i, j, pClipData->GetPattern(i, j));
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
	const unsigned int StartColumn = pClipData->ClipInfo.StartColumn;
	const unsigned int EndColumn   = pClipData->ClipInfo.EndColumn;
	
	bool AtSel = (PastePos == PASTE_SELECTION || PastePos == PASTE_FILL);
	const int f = AtSel ? m_selection.GetFrameStart() : (PastePos == PASTE_DRAG ? m_selDrag.GetFrameStart() : m_cpCursorPos.m_iFrame);
	const unsigned int r = AtSel ? m_selection.GetRowStart() : (PastePos == PASTE_DRAG ? m_selDrag.GetRowStart() : m_cpCursorPos.m_iRow);
	const unsigned int c = AtSel ? m_selection.GetChanStart() : (PastePos == PASTE_DRAG ? m_selDrag.GetChanStart() : m_cpCursorPos.m_iChannel);
	const unsigned int CEnd = std::min(Channels + c, ChannelCount);

	CPatternIterator it = CPatternIterator(this, Track, CCursorPos(r, c, StartColumn, f));		// // //
	stChanNote NoteData, Source;

	const unsigned int FrameLength = m_pDocument->GetPatternLength(Track);

	if (PasteMode == PASTE_INSERT) {		// // //
		CPatternIterator front = CPatternIterator(this, Track, CCursorPos(FrameLength - 1, c, StartColumn, f));
		CPatternIterator back = CPatternIterator(this, Track, CCursorPos(FrameLength - 1 - Rows, c, StartColumn, f));
		front.m_iFrame = back.m_iFrame = f; // do not warp
		front.m_iRow = FrameLength - 1;
		back.m_iRow = FrameLength - 1 - Rows;
		while (back.m_iRow >= static_cast<int>(r)) {
			for (unsigned int i = c; i < CEnd; i++) {
				back.Get(i, &Source);
				front.Get(i, &NoteData);
				CopyNoteSection(&NoteData, &Source, PasteMode, (i == c) ? StartColumn : COLUMN_NOTE,
					std::min((i == Channels + c - 1) ? EndColumn : COLUMN_EFF4, COLUMN_EFF1 + m_pDocument->GetEffColumns(Track, i)));
				front.Set(i, &NoteData);
			}
			front.m_iRow--;
			back.m_iRow--;
		}
		m_selection.m_cpEnd.m_iRow = std::min(m_selection.m_cpStart.m_iRow + static_cast<int>(Rows), GetCurrentPatternLength(f)) - 1;
	}

	// Special, single channel and effect columns only
	if (Channels == 1 && StartColumn >= COLUMN_EFF1) {
		const unsigned int ColStart = GetSelectColumn(AtSel ? m_selection.GetColStart() : m_cpCursorPos.m_iColumn);
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
			const int ColEnd = std::min((i == Channels + c - 1) ? EndColumn : COLUMN_EFF4,
				COLUMN_EFF1 + m_pDocument->GetEffColumns(Track, i));
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
	CPatternIterator it = GetStartIterator();
	const int Track = GetSelectedTrack();
	const int Frames = m_pDocument->GetFrameCount(Track);
	const int Length = m_pDocument->GetPatternLength(Track);

	const int Rows = pClipData->ClipInfo.Rows;
	const int Channels = pClipData->ClipInfo.Channels;
	const unsigned int StartColumn = pClipData->ClipInfo.StartColumn;
	const unsigned int EndColumn = pClipData->ClipInfo.EndColumn;

	stChanNote Target = BLANK_NOTE;
	stChanNote Source = BLANK_NOTE;
	const int PackedPos = (m_selection.GetFrameStart() + GetFrameCount()) * Length + m_selection.GetRowStart();
	for (int i = 0; i < Channels; ++i) for (int r = 0; r < Rows; r++) {
		if (i + m_selection.GetChanStart() == GetChannelCount()) return;
		m_pDocument->GetNoteData(Track, (PackedPos + r) / Length % Frames, i + m_selection.GetChanStart(), (PackedPos + r) % Length, &Target);
		Source = *(pClipData->GetPattern(i, r));
		CopyNoteSection(&Target, &Source, PASTE_DEFAULT, (i == 0) ? StartColumn : COLUMN_NOTE,
			std::min((i == Channels + m_selection.GetChanStart() - 1) ? EndColumn : COLUMN_EFF4, COLUMN_EFF1 + m_pDocument->GetEffColumns(Track, i)));
		m_pDocument->SetNoteData(Track, (PackedPos + r) / Length % Frames, i + m_selection.GetChanStart(), (PackedPos + r) % Length, &Target);
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
	SetSelectionStart(CCursorPos(0, m_cpCursorPos.m_iChannel, 0, m_iCurrentFrame));		// // //
	SetSelectionEnd(CCursorPos(m_iPatternLength - 1, m_cpCursorPos.m_iChannel, GetChannelColumns(m_cpCursorPos.m_iChannel) - 1, m_iCurrentFrame));
}

void CPatternEditor::SelectAllChannels()
{
	// Select all channels
	m_bSelecting = true;
	SetSelectionStart(CCursorPos(0, 0, 0, m_iCurrentFrame));		// // //
	SetSelectionEnd(CCursorPos(m_iPatternLength - 1, GetChannelCount() - 1, GetChannelColumns(GetChannelCount() - 1) - 1, m_iCurrentFrame));
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
	const int Track = GetSelectedTrack();
	const int Frames = GetFrameCount();
	unsigned char Lo[MAX_PATTERN], Hi[MAX_PATTERN];

	if (!theApp.GetSettings()->General.bShowSkippedRows) {
		CPatternIterator it = GetStartIterator();
		const CPatternIterator End = GetEndIterator();
		stChanNote Note;
		for (; it <= End; it++) {
			// bool HasSkip = false;
			for (int i = 0; i < GetChannelCount(); i++) {
				it.Get(i, &Note);
				for (unsigned int c = 0; c <= m_pDocument->GetEffColumns(Track, i); c++) switch (Note.EffNumber[c]) {
				case EF_JUMP: case EF_SKIP: case EF_HALT:
					if (m_selection.IsColumnSelected(COLUMN_EFF1 + c, i))
						return (it == End) ? SEL_TERMINAL_SKIP : SEL_NONTERMINAL_SKIP;
					/*else if (it != End)
						HasSkip = true;*/
				}
			}
			/*if (HasSkip)
				return SEL_UNKNOWN_SIZE;*/
		}
	}

	for (int c = m_selection.GetChanStart(); c <= m_selection.GetChanEnd(); c++) {
		memset(Lo, 255, MAX_PATTERN);
		memset(Hi, 0, MAX_PATTERN);

		for (int i = m_selection.GetFrameStart(); i <= m_selection.GetFrameEnd(); i++) {
			int Pattern = m_pDocument->GetPatternAtFrame(Track, (i + Frames) % Frames, c);
			int RBegin = (i == m_selection.GetFrameStart()) ? m_selection.GetRowStart() : 0;
			int REnd = (i == m_selection.GetFrameEnd()) ? m_selection.GetRowEnd() : GetCurrentPatternLength(i) - 1;
			if (Lo[Pattern] <= Hi[Pattern] && RBegin <= Hi[Pattern] && REnd >= Lo[Pattern])
				return SEL_REPEATED_ROW;
			Lo[Pattern] = std::min(Lo[Pattern], static_cast<unsigned char>(RBegin));
			Hi[Pattern] = std::max(Hi[Pattern], static_cast<unsigned char>(REnd));
		}
	}

	return SEL_CLEAN;
}

// Other ////////////////////////////////////////////////////////////////////////////////////////////////////

int CPatternEditor::GetCurrentPatternLength(int Frame) const		// // //
{
	const int Track = GetSelectedTrack();
	const int Channels = GetChannelCount();
	const int PatternLength = m_pDocument->GetPatternLength(Track);	// default length

	if (theApp.GetSettings()->General.bShowSkippedRows)		// // //
		return PatternLength;
	else {		// // //
		Frame %= GetFrameCount();
		if (Frame < 0) Frame += GetFrameCount();
		return m_pDocument->GetFrameLength(Track, Frame);		// // // moved
	}
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
		MoveToColumn(COLUMN_NOTE);
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

	m_iScrolling = SCROLL_NONE;		// // //

	int Row = (point.y - HEADER_HEIGHT) / m_iRowHeight - (m_iLinesVisible / 2);		// // //

	if (Row > (m_iLinesFullVisible / 2) - 3) {
		m_iScrolling += SCROLL_DOWN;		// // //
	}
	else if (Row <= -(m_iLinesFullVisible / 2)) {
		m_iScrolling += SCROLL_UP;		// // //
	}

	if (m_bFullRowSelect) return;		// // //

	if (PointPos.m_iChannel >= (m_iFirstChannel + m_iChannelsVisible - 1) && m_iChannelsVisible < GetChannelCount()) {		// // //
		if (m_cpCursorPos.m_iChannel < Channels - 1)		// // //
			m_iScrolling += SCROLL_RIGHT;		// // //
	}
	else if (PointPos.m_iChannel < m_iFirstChannel) {
		if (m_cpCursorPos.m_iChannel > 0)
			m_iScrolling += SCROLL_LEFT;		// // //
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
	if (m_cpCursorPos.m_iRow == GetCurrentPatternLength(m_iCurrentFrame)) {		// // //
		m_iCenterRow = m_cpCursorPos.m_iRow = 0;
		m_iCurrentFrame++;
		if (m_iCurrentFrame == GetFrameCount())
			m_iCurrentFrame = 0;
	}
	else if (m_cpCursorPos.m_iRow == -1) {
		m_iCenterRow = m_cpCursorPos.m_iRow = GetCurrentPatternLength(--m_iCurrentFrame) - 1;
		if (m_iCurrentFrame == -1)
			m_iCurrentFrame = GetFrameCount() - 1;
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
		m_selection.m_cpStart.m_iColumn = 0;
		m_selection.m_cpEnd.m_iColumn = GetChannelColumns(m_selection.m_cpEnd.m_iChannel) - 1;
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
	const CPatternIterator End = GetEndIterator();
	stChanNote NoteData;

	if (Channel < 0 || Channel >= GetChannelCount())
		return;
	
	int vol = MAX_VOLUME - 1;		// // //
	CPatternIterator it = GetStartIterator();
	do {
		it--;
		if (it.m_iFrame < 0) break;
		it.Get(Channel, &NoteData);
		if (NoteData.Vol != MAX_VOLUME) {
			vol = NoteData.Vol;
			break;
		}
	} while (it.m_iFrame > 0 || it.m_iRow > 0);
	
	str.Empty();
	for (CPatternIterator it = GetStartIterator(); it <= End; it++) {
		it.Get(Channel, &NoteData);
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

	CPatternIterator it = GetStartIterator();
	CPatternIterator end = GetEndIterator();
	str.Empty();

	int Row = 0;
	int Size = m_bSelecting ? (GetSelectionSize() - 1) : (end.m_iRow - it.m_iRow + 1);
	int HexLength = 0;
	do {
		HexLength++;
		Size >>= 4;
	} while (Size);
	if (HexLength < 2) HexLength = 2;
	const int Last = static_cast<int>(m_pDocument->GetEffColumns(Track, end.m_iChannel)) + COLUMN_EFF1 - GetSelectColumn(end.m_iColumn);
	for (CPatternIterator it = GetStartIterator(); it <= end; it++) {
		CString line;
		line.AppendFormat(_T("ROW %0*X"), HexLength, Row++);
		for (int i = it.m_iChannel; i <= end.m_iChannel; i++) {
			it.Get(i, &NoteData);
			line.AppendFormat(_T(" : %s"), CTextExport::ExportCellText(NoteData, m_pDocument->GetEffColumns(Track, i) + 1, i == CHANID_NOISE));
		}
		for (int i = 0; i < std::min(3, GetSelectColumn(it.m_iColumn)); i++)
			for (int j = 0; j < 3; j++) line.SetAt(7 + 3 * i + j + HexLength, ' ');
		for (int i = 3; i < std::min(static_cast<int>(m_pDocument->GetEffColumns(Track, i)) + 4, GetSelectColumn(it.m_iColumn)); i++)
			for (int j = 0; j < 3; j++) line.SetAt(4 * i + j + HexLength, ' ');
		str.Append(line.Left(line.GetLength() - 4 * std::max(0, std::min(3, Last)) + std::max(0, 3 - GetSelectColumn(end.m_iColumn))));
		str.Append(_T("\r\n"));
	}
}

void CPatternEditor::GetSelectionAsPPMCK(CString &str) const		// // //
{
	// Returns a PPMCK MML translation of copied pattern
	AfxMessageBox(_T("This feature has been ported directly from official FamiTracker 0.3.5, and will be improved in a future version. Do NOT expect this feature to work properly."));

	int i, j;
	stChanNote NoteData = BLANK_NOTE;

	str.Empty();

	int Octave, Note, Duration;

	const CPatternIterator End = GetEndIterator();
	for (int c = m_selection.GetChanStart(); c <= m_selection.GetChanEnd(); c++) {
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
		stChanNote current = BLANK_NOTE;
		current.Note = HALT;
		stChanNote echo[ECHO_BUFFER_LENGTH + 1];
		for (int i = 0; i <= ECHO_BUFFER_LENGTH; i++)
			echo[i] = BLANK_NOTE;

		for (CPatternIterator it = GetStartIterator(); it <= End; it++) {
			len++;
			it.Get(c, &NoteData);
			bool dump = NoteData.Note != NONE || NoteData.Vol != MAX_VOLUME;
			bool fin = it.m_iFrame == End.m_iFrame && it.m_iRow == End.m_iRow;

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
	str.Append(_T(";;\r\n"));
	for (i = m_selection.GetChanStart(); i <= m_selection.GetChanEnd(); i++) {
		if (i < 0 || i >(signed)m_pDocument->GetAvailableChannels())
			continue;

		switch (i) {
		case 0: str.Append("A "); break;
		case 1: str.Append("B "); break;
		case 2: str.Append("C "); break;
		case 3: str.Append("D "); break;
		case 4: str.Append("E "); break;
		}

		Octave = -1;
		Note = -1;

		for (j = m_selection.GetRowStart(); j <= (m_selection.GetRowEnd() + 1); j++) {
			if (j < 0 || j > m_iPatternLength)
				continue;

			m_pDocument->GetNoteData(GetSelectedTrack(), m_iCurrentFrame, i, j, &NoteData);
			if (m_selection.IsColumnSelected(COLUMN_NOTE, i)) {
				int n = NoteData.Note;
				int o = NoteData.Octave;

				bool DumpNote = false;

				// First note
				/*
				if (Note == -1) {
				Note = n;
				Octave = o;
				Duration = 0;
				}
				*/
				if (j == m_selection.GetRowEnd())
					DumpNote = true;

				//if (n > 0 && n != Note) {
				if (n > 0 || Note == -1) {
					if (Note == -1) {
						Note = n;
						Octave = o;
						Duration = 0;
					}
					else
						DumpNote = true;
				}
				else {
					Duration++;
				}

				if (DumpNote) {
					/*
					if (o != Octave && n > 0) {
					MML.AppendFormat("o%i", Octave);
					Octave = o;
					}
					*/
					switch (Note) {
					case 0:  str.Append("r"); break;
					case 1:  str.Append("c"); break;
					case 2:  str.Append("c#"); break;
					case 3:  str.Append("d"); break;
					case 4:  str.Append("d#"); break;
					case 5:  str.Append("e"); break;
					case 6:  str.Append("f"); break;
					case 7:  str.Append("f#"); break;
					case 8:  str.Append("g"); break;
					case 9:  str.Append("g#"); break;
					case 10: str.Append("a"); break;
					case 11: str.Append("a#"); break;
					case 12: str.Append("b"); break;
					}

					switch (Duration) {
					case 0: str.Append("16"); break;		// 16th note
					case 1: str.Append("8"); break;			// 8th note
					case 2: str.Append("8."); break;		// 8th dotted
					case 3: str.Append("4"); break;			// 4th note

//					case 4: str.Append("4"); break;			// 4th note
//					case 5: str.Append("4"); break;			// 4th note						
//					case 6: str.Append("4"); break;			// 4th note

					case 7: str.Append("2"); break;			// 2th note
					case 15: str.Append("1"); break;		// 1th note

					}

					Note = n;
					Duration = 0;
				}
			}
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

	// // //
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

	CPatternIterator cpBegin(this, GetSelectedTrack(), CCursorPos(PointPos.m_iRow - m_iDragOffsetRow,		// // //
		PointPos.m_iChannel - m_iDragOffsetChannel, ColumnStart, PointPos.m_iFrame));
	CPatternIterator cpEnd = cpBegin;
	cpEnd += GetSelectionSize() - 1;
	cpEnd.m_iChannel += m_iDragChannels;
	cpEnd.m_iColumn = ColumnEnd;
	m_selDrag.m_cpStart = static_cast<CCursorPos>(cpBegin);
	m_selDrag.m_cpEnd = static_cast<CCursorPos>(cpEnd);

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