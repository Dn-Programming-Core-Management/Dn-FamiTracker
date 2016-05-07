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

#include "stdafx.h"
#include "FamiTrackerDoc.h"
#include "PatternEditor.h"

// CCursorPos /////////////////////////////////////////////////////////////////////

CCursorPos::CCursorPos() : m_iRow(0), m_iChannel(0), m_iColumn(C_NOTE), m_iFrame(0)		// // //
{
}

CCursorPos::CCursorPos(int Row, int Channel, cursor_column_t Column, int Frame) :		// // //
	m_iRow(Row), m_iChannel(Channel), m_iColumn(Column), m_iFrame(Frame)
{
}

const CCursorPos& CCursorPos::operator=(const CCursorPos &pos) 
{
	// Copy position
	m_iRow = pos.m_iRow;
	m_iColumn = pos.m_iColumn;
	m_iChannel = pos.m_iChannel;
	m_iFrame = pos.m_iFrame;		// // //
	return *this;
}

bool CCursorPos::operator!=(const CCursorPos &other) const
{
	// Unequality check
	return (m_iRow != other.m_iRow) || (m_iChannel != other.m_iChannel)		// // //
		|| (m_iColumn != other.m_iColumn) || (m_iFrame != other.m_iFrame);
}

bool CCursorPos::operator<(const CCursorPos &other) const		// // //
{
	return m_iFrame < other.m_iFrame || (m_iFrame == other.m_iFrame && m_iRow < other.m_iRow);
}

bool CCursorPos::operator<=(const CCursorPos &other) const		// // //
{
	return !(other < *this);
}

bool CCursorPos::IsValid(int RowCount, int ChannelCount) const		// // //
{
	// Check if a valid pattern position
	//if (m_iFrame < -FrameCount || m_iFrame >= 2 * FrameCount)		// // //
	//	return false;
	if (m_iChannel < 0 || m_iChannel >= ChannelCount)
		return false;
	if (m_iRow < 0 || m_iRow >= RowCount)
		return false;
	if (m_iColumn < C_NOTE || m_iColumn > C_EFF4_PARAM2)		// // //
		return false;

	return true;
}

// CSelection /////////////////////////////////////////////////////////////////////

int CSelection::GetRowStart() const 
{
	if (m_cpEnd.m_iFrame > m_cpStart.m_iFrame)		// // //
		return m_cpStart.m_iRow;
	if (m_cpEnd.m_iFrame < m_cpStart.m_iFrame)
		return m_cpEnd.m_iRow;

	return (m_cpEnd.m_iRow > m_cpStart.m_iRow ?  m_cpStart.m_iRow : m_cpEnd.m_iRow);
}

int CSelection::GetRowEnd() const 
{
	if (m_cpEnd.m_iFrame > m_cpStart.m_iFrame)		// // //
		return m_cpEnd.m_iRow;
	if (m_cpEnd.m_iFrame < m_cpStart.m_iFrame)
		return m_cpStart.m_iRow;

	return (m_cpEnd.m_iRow > m_cpStart.m_iRow ? m_cpEnd.m_iRow : m_cpStart.m_iRow);
}

cursor_column_t CSelection::GetColStart() const 
{
	cursor_column_t Col = C_NOTE;
	if (m_cpStart.m_iChannel == m_cpEnd.m_iChannel)
		Col = (m_cpEnd.m_iColumn > m_cpStart.m_iColumn ? m_cpStart.m_iColumn : m_cpEnd.m_iColumn); 
	else if (m_cpEnd.m_iChannel > m_cpStart.m_iChannel)
		Col = m_cpStart.m_iColumn;
	else 
		Col = m_cpEnd.m_iColumn;
	switch (Col) {
		case C_INSTRUMENT2: Col = C_INSTRUMENT1; break;
		case C_EFF1_PARAM1: case C_EFF1_PARAM2: Col = C_EFF1_NUM; break;
		case C_EFF2_PARAM1: case C_EFF2_PARAM2: Col = C_EFF2_NUM; break;
		case C_EFF3_PARAM1: case C_EFF3_PARAM2: Col = C_EFF3_NUM; break;
		case C_EFF4_PARAM1: case C_EFF4_PARAM2: Col = C_EFF4_NUM; break;
	}
	return Col;
}

cursor_column_t CSelection::GetColEnd() const 
{
	cursor_column_t Col = C_NOTE;
	if (m_cpStart.m_iChannel == m_cpEnd.m_iChannel)
		Col = (m_cpEnd.m_iColumn > m_cpStart.m_iColumn ? m_cpEnd.m_iColumn : m_cpStart.m_iColumn); 
	else if (m_cpEnd.m_iChannel > m_cpStart.m_iChannel)
		Col = m_cpEnd.m_iColumn;
	else
		Col = m_cpStart.m_iColumn;
	switch (Col) {
		case C_INSTRUMENT1: Col = C_INSTRUMENT2; break;						// Instrument
		case C_EFF1_NUM: case C_EFF1_PARAM1: Col = C_EFF1_PARAM2; break;	// Eff 1
		case C_EFF2_NUM: case C_EFF2_PARAM1: Col = C_EFF2_PARAM2; break;	// Eff 2
		case C_EFF3_NUM: case C_EFF3_PARAM1: Col = C_EFF3_PARAM2; break;	// Eff 3
		case C_EFF4_NUM: case C_EFF4_PARAM1: Col = C_EFF4_PARAM2; break;	// Eff 4
	}
	return Col;	
}

int CSelection::GetChanStart() const 
{
	return (m_cpEnd.m_iChannel > m_cpStart.m_iChannel) ? m_cpStart.m_iChannel : m_cpEnd.m_iChannel; 
}

int CSelection::GetChanEnd() const 
{
	return (m_cpEnd.m_iChannel > m_cpStart.m_iChannel) ? m_cpEnd.m_iChannel : m_cpStart.m_iChannel; 
}

int CSelection::GetFrameStart() const		// // //
{
	return (m_cpEnd.m_iFrame > m_cpStart.m_iFrame) ? m_cpStart.m_iFrame : m_cpEnd.m_iFrame;
}

int CSelection::GetFrameEnd() const		// // //
{
	return (m_cpEnd.m_iFrame > m_cpStart.m_iFrame) ? m_cpEnd.m_iFrame : m_cpStart.m_iFrame;
}

bool CSelection::IsSameStartPoint(const CSelection &selection) const
{
	return GetChanStart() == selection.GetChanStart() &&
		GetRowStart() == selection.GetRowStart() &&
		GetColStart() == selection.GetColStart() &&
		GetFrameStart() == selection.GetFrameStart();		// // //
}

bool CSelection::IsColumnSelected(column_t Column, int Channel) const
{
	column_t SelStart = CPatternEditor::GetSelectColumn(GetColStart());		// // //
	column_t SelEnd = CPatternEditor::GetSelectColumn(GetColEnd());

	return (Channel > GetChanStart() || (Channel == GetChanStart() && Column >= SelStart))		// // //
		&& (Channel < GetChanEnd() || (Channel == GetChanEnd() && Column <= SelEnd));
}

void CSelection::Normalize(CCursorPos &Begin, CCursorPos &End) const		// // //
{
	CCursorPos Temp {GetRowStart(), GetChanStart(), GetColStart(), GetFrameStart()};
	std::swap(End, CCursorPos {
		m_cpStart.m_iRow + m_cpEnd.m_iRow - Temp.m_iRow,
		m_cpStart.m_iChannel + m_cpEnd.m_iChannel - Temp.m_iChannel,
		static_cast<cursor_column_t>(m_cpStart.m_iColumn + m_cpEnd.m_iColumn - Temp.m_iColumn),
		m_cpStart.m_iFrame + m_cpEnd.m_iFrame - Temp.m_iFrame
	});
	std::swap(Begin, Temp);
}

// CPatternClipData ////////////////////////////////////////////////////////////

SIZE_T CPatternClipData::GetAllocSize() const
{
	return sizeof(ClipInfo) + Size * sizeof(stChanNote);
}

void CPatternClipData::ToMem(HGLOBAL hMem) 
{
	// From CPatternClipData to memory
	ASSERT(hMem != NULL);
	ASSERT(pPattern != NULL);

	BYTE *pByte = (BYTE*)::GlobalLock(hMem);

	if (pByte != NULL) {
		memcpy(pByte, &ClipInfo, sizeof(ClipInfo));
		memcpy(pByte + sizeof(ClipInfo), pPattern, Size * sizeof(stChanNote));

		::GlobalUnlock(hMem);
	}
}

void CPatternClipData::FromMem(HGLOBAL hMem)
{
	// From memory to CPatternClipData
	ASSERT(hMem != NULL);
	ASSERT(pPattern == NULL);

	BYTE *pByte = (BYTE*)::GlobalLock(hMem);

	if (pByte != NULL) {
		memcpy(&ClipInfo, pByte, sizeof(ClipInfo));
	
		Size = ClipInfo.Channels * ClipInfo.Rows;
		pPattern = new stChanNote[Size];
		memcpy(pPattern, pByte + sizeof(ClipInfo), Size * sizeof(stChanNote));

		::GlobalUnlock(hMem);
	}
}

stChanNote *CPatternClipData::GetPattern(int Channel, int Row)
{
	ASSERT(Channel < ClipInfo.Channels);
	ASSERT(Row < ClipInfo.Rows);

	return pPattern + (Channel * ClipInfo.Rows + Row);
}

const stChanNote *CPatternClipData::GetPattern(int Channel, int Row) const
{
	ASSERT(Channel < ClipInfo.Channels);
	ASSERT(Row < ClipInfo.Rows);

	return pPattern + (Channel * ClipInfo.Rows + Row);
}

// // // CPatternIterator //////////////////////////////////////////////////////

CPatternIterator::CPatternIterator(const CPatternIterator &it) :
	m_iTrack(it.m_iTrack),
	m_pDocument(it.m_pDocument),
	m_pPatternEditor(it.m_pPatternEditor),
	CCursorPos(static_cast<const CCursorPos &>(it))
{
}

CPatternIterator::CPatternIterator(CPatternEditor *pEditor, unsigned int Track, const CCursorPos &Pos) :
	m_iTrack(Track),
	m_pDocument(CFamiTrackerDoc::GetDoc()),
	m_pPatternEditor(pEditor),
	CCursorPos(Pos)
{
	Warp();
}

CPatternIterator::CPatternIterator(const CPatternEditor *pEditor, unsigned int Track, const CCursorPos &Pos) :
	m_iTrack(Track),
	m_pDocument(CFamiTrackerDoc::GetDoc()),
	m_pPatternEditor(pEditor),
	CCursorPos(Pos)
{
	Warp();
}

void CPatternIterator::Get(int Channel, stChanNote *pNote) const
{
	int Frame = m_iFrame % m_pDocument->GetFrameCount(m_iTrack);
	if (Frame < 0) Frame += m_pDocument->GetFrameCount(m_iTrack);
	m_pDocument->GetNoteData(m_iTrack, Frame, Channel, m_iRow, pNote);
}

void CPatternIterator::Set(int Channel, const stChanNote *pNote)
{
	int Frame = m_iFrame % m_pDocument->GetFrameCount(m_iTrack);
	if (Frame < 0) Frame += m_pDocument->GetFrameCount(m_iTrack);
	m_pDocument->SetNoteData(m_iTrack, Frame, Channel, m_iRow, pNote);
}

void CPatternIterator::Step() // resolves skip effects
{
	stChanNote Note;

	for (int i = m_pDocument->GetChannelCount() - 1; i >= 0; --i) {
		Get(i, &Note);
		for (int c = m_pDocument->GetEffColumns(m_iTrack, i); c >= 0; --c) {
			if (Note.EffNumber[c] == EF_JUMP) {
				m_iFrame = Note.EffParam[c];
				if (m_iFrame >= static_cast<int>(m_pDocument->GetFrameCount(m_iTrack)))
					m_iFrame = m_pDocument->GetFrameCount(m_iTrack) - 1;
				m_iRow = 0;
				return;
			}
		}
	}
	for (int i = m_pDocument->GetChannelCount() - 1; i >= 0; i--) {
		Get(i, &Note);
		for (int c = m_pDocument->GetEffColumns(m_iTrack, i); c >= 0; --c) {
			if (Note.EffNumber[c] == EF_SKIP) {
				++m_iFrame;
				m_iRow = 0;
				return;
			}
		}
	}
	++m_iRow;
	Warp();
}

CPatternIterator& CPatternIterator::operator+=(const int Rows)
{
	m_iRow += Rows;
	Warp();
	return *this;
}

CPatternIterator& CPatternIterator::operator-=(const int Rows)
{
	return operator+=(-Rows);
}

CPatternIterator& CPatternIterator::operator++()
{
	return operator+=(1);
}

CPatternIterator CPatternIterator::operator++(int)
{
	CPatternIterator tmp(*this);
	operator+=(1);
	return tmp;
}

CPatternIterator& CPatternIterator::operator--()
{
	return operator+=(-1);
}

CPatternIterator CPatternIterator::operator--(int)
{
	CPatternIterator tmp(*this);
	operator+=(-1);
	return tmp;
}

bool CPatternIterator::operator==(const CPatternIterator &other) const
{
	return m_iFrame == other.m_iFrame && m_iRow == other.m_iRow;
}

void CPatternIterator::Warp()
{
	while (true) {
		const int Length = m_pPatternEditor->GetCurrentPatternLength(m_iFrame);
		if (m_iRow >= Length) {
			m_iRow -= Length;
			m_iFrame++;
		}
		else break;
	}
	while (m_iRow < 0)
		m_iRow += m_pPatternEditor->GetCurrentPatternLength(--m_iFrame);
	//m_iFrame %= m_pDocument->GetFrameCount(m_iTrack);
	//if (m_iFrame < 0) m_iFrame += m_pDocument->GetFrameCount(m_iTrack);
}

// CPatternEditorLayout ////////////////////////////////////////////////////////
/*
CPatternEditorLayout::CPatternEditorLayout()
{
}

void CPatternEditorLayout::SetSize(int Width, int Height)
{
//	m_iWinWidth = width;
//	m_iWinHeight = height - ::GetSystemMetrics(SM_CYHSCROLL);
}

void CPatternEditorLayout::CalculateLayout()
{
	// Calculate pattern layout. Must be called when layout or window size has changed

}
*/
