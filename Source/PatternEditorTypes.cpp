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
#include "FamiTrackerDoc.h"
#include "PatternEditor.h"

// CCursorPos /////////////////////////////////////////////////////////////////////

CCursorPos::CCursorPos() : m_iRow(0), m_iChannel(0), m_iColumn(0) 
{
}

CCursorPos::CCursorPos(int Row, int Channel, int Column) : m_iRow(Row), m_iChannel(Channel), m_iColumn(Column) 
{
}

const CCursorPos& CCursorPos::operator=(const CCursorPos &pos) 
{
	// Copy position
	m_iRow = pos.m_iRow;
	m_iColumn = pos.m_iColumn;
	m_iChannel = pos.m_iChannel;
	return *this;
}

bool CCursorPos::operator !=(const CCursorPos &other) const
{
	// Unequality check
	return (m_iRow != other.m_iRow) || (m_iChannel != other.m_iChannel) || (m_iColumn != other.m_iColumn);
}

bool CCursorPos::IsValid(int RowCount, int ChannelCount) const
{
	// Check if a valid pattern position
	if (m_iChannel < 0 || m_iChannel >= ChannelCount)
		return false;
	if (m_iRow < 0 || m_iRow >= RowCount)
		return false;
	if (m_iColumn < 0)
		return false;

	return true;
}

// CSelection /////////////////////////////////////////////////////////////////////

int CSelection::GetRowStart() const 
{
	return (m_cpEnd.m_iRow > m_cpStart.m_iRow ?  m_cpStart.m_iRow : m_cpEnd.m_iRow);
}

int CSelection::GetRowEnd() const 
{
	return (m_cpEnd.m_iRow > m_cpStart.m_iRow ? m_cpEnd.m_iRow : m_cpStart.m_iRow);
}

int CSelection::GetColStart() const 
{
	int Col = 0;
	if (m_cpStart.m_iChannel == m_cpEnd.m_iChannel)
		Col = (m_cpEnd.m_iColumn > m_cpStart.m_iColumn ? m_cpStart.m_iColumn : m_cpEnd.m_iColumn); 
	else if (m_cpEnd.m_iChannel > m_cpStart.m_iChannel)
		Col = m_cpStart.m_iColumn;
	else 
		Col = m_cpEnd.m_iColumn;
	switch (Col) {
		case 2: Col = 1; break;
		case 5: case 6: Col = 4; break;
		case 8: case 9: Col = 7; break;
		case 11: case 12: Col = 10; break;
		case 14: case 15: Col = 13; break;
	}
	return Col;
}

int CSelection::GetColEnd() const 
{
	int Col = 0;
	if (m_cpStart.m_iChannel == m_cpEnd.m_iChannel)
		Col = (m_cpEnd.m_iColumn > m_cpStart.m_iColumn ? m_cpEnd.m_iColumn : m_cpStart.m_iColumn); 
	else if (m_cpEnd.m_iChannel > m_cpStart.m_iChannel)
		Col = m_cpEnd.m_iColumn;
	else
		Col = m_cpStart.m_iColumn;
	switch (Col) {
		case 1: Col = 2; break;					// Instrument
		case 4: case 5: Col = 6; break;			// Eff 1
		case 7: case 8: Col = 9; break;			// Eff 2
		case 10: case 11: Col = 12; break;		// Eff 3
		case 13: case 14: Col = 15; break;		// Eff 4
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

bool CSelection::IsWithin(const CCursorPos &pos) const 
{
	if (pos.m_iRow >= GetRowStart() && pos.m_iRow <= GetRowEnd()) {
		if (pos.m_iChannel == GetChanStart() && pos.m_iChannel == GetChanEnd()) {
			return (pos.m_iColumn >= GetColStart() && pos.m_iColumn <= GetColEnd());
		}
		else if (pos.m_iChannel == GetChanStart() && pos.m_iChannel != GetChanEnd()) {
			return (pos.m_iColumn >= GetColStart());
		}
		else if (pos.m_iChannel == GetChanEnd() && pos.m_iChannel != GetChanStart()) {
			return (pos.m_iColumn <= GetColEnd());
		}
		else if (pos.m_iChannel >= GetChanStart() && pos.m_iChannel < GetChanEnd()) {
			return true;
		}
	}
	return false;
}

bool CSelection::IsSingleChannel() const 
{
	return (m_cpStart.m_iChannel == m_cpEnd.m_iChannel);
}

bool CSelection::IsSameStartPoint(const CSelection &selection) const
{
	return GetChanStart() == selection.GetChanStart() &&
		GetRowStart() == selection.GetRowStart() &&
		GetColStart() == selection.GetColStart();
}

bool CSelection::IsColumnSelected(int Column, int Channel) const
{
	int SelColStart = GetColStart();
	int SelColEnd	= GetColEnd();

	if (Channel > GetChanStart() && Channel < GetChanEnd())
		return true;

	// 0 = Note (0)
	// 1, 2 = Instrument (1)
	// 3 = Volume (2)
	// 4, 5, 6 = Effect 1 (3)
	// 7, 8, 9 = Effect 1 (4)
	// 10, 11, 12 = Effect 1 (5)
	// 13, 14, 15 = Effect 1 (6)

	int SelStart = CPatternEditor::GetSelectColumn(SelColStart);
	int SelEnd = CPatternEditor::GetSelectColumn(SelColEnd);
	
	if (Channel == GetChanStart() && Channel == GetChanEnd()) {
		if (Column >= SelStart && Column <= SelEnd)
			return true;
	}
	else if (Channel == GetChanStart()) {
		if (Column >= SelStart)
			return true;
	}
	else if (Channel == GetChanEnd()) {
		if (Column <= SelEnd)
			return true;
	}

	return false;
}

void CSelection::Clear()
{
	m_cpStart = CCursorPos();
	m_cpEnd = CCursorPos();
}

void CSelection::SetStart(const CCursorPos &pos) 
{
	m_cpStart = pos;
}

void CSelection::SetEnd(const CCursorPos &pos) 
{
	m_cpEnd = pos;
}

bool CSelection::operator !=(const CSelection &other) const
{
	return (m_cpStart != other.m_cpStart) || (m_cpEnd != other.m_cpEnd);
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
