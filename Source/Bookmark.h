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

#pragma once


#include "FamiTrackerTypes.h" // PatternData
#include "PatternData.h" // stHighlight

#include <string>

class CBookmark
{
public:
	CBookmark(unsigned Frame = 0, unsigned Row = 0);
	unsigned Distance(const CBookmark &other) const;
	bool IsEqual(const CBookmark &other) const; // == overridden
	bool operator==(const CBookmark &other) const;
	bool operator<(const CBookmark &other) const;

	unsigned	m_iFrame;
	unsigned	m_iRow;
	stHighlight	m_Highlight;
	bool		m_bPersist;
	std::string	m_sName;
};
