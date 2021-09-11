/*
** FamiTracker - NES/Famicom sound tracker
** Copyright (C) 2005-2014  Jonathan Liss
**
** 0CC-FamiTracker is (C) 2014-2016 HertzDevil
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
#include <string>
#include "Bookmark.h"
#include "FamiTrackerTypes.h"

CBookmark::CBookmark(unsigned Frame, unsigned Row) :
	m_iFrame(Frame),
	m_iRow(Row),
	m_bPersist(false),
	m_sName("Bookmark")
{
	m_Highlight.First = 4;
	m_Highlight.Second = 16;
	m_Highlight.Offset = 0;
}

unsigned CBookmark::Distance(const CBookmark &other) const
{
	const int ALL_ROWS = MAX_PATTERN * MAX_PATTERN_LENGTH;
	int Dist =
		(((int)m_iFrame - (int)other.m_iFrame) * MAX_PATTERN_LENGTH
			+ (int)m_iRow - (int)other.m_iRow)
		% ALL_ROWS;
	if (Dist < 0) Dist += ALL_ROWS;
	return static_cast<unsigned>(Dist);
}

bool CBookmark::IsEqual(const CBookmark &other) const
{
	return *this == other && m_bPersist == other.m_bPersist && m_sName == other.m_sName &&
		!memcmp(&m_Highlight, &other.m_Highlight, sizeof(stHighlight));
}

bool CBookmark::operator==(const CBookmark &other) const
{
	return m_iFrame == other.m_iFrame && m_iRow == other.m_iRow;
}

bool CBookmark::operator<(const CBookmark &other) const
{
	return m_iFrame < other.m_iFrame || (m_iFrame == other.m_iFrame && m_iRow < other.m_iRow);
}
