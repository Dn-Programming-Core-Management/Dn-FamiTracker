/*
** Dn-FamiTracker - NES/Famicom sound tracker
** Copyright (C) 2020-2025 D.P.C.M.
** FamiTracker Copyright (C) 2005-2020 Jonathan Liss
** 0CC-FamiTracker Copyright (C) 2014-2018 HertzDevil
**
** This program is free software: you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation, either version 3 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program. If not, see https://www.gnu.org/licenses/.
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
