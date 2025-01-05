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


#include "stdafx.h"

/*!
	\brief An extension of the MFC file class with methods for writing and reading in different
	data types.
	\details This class replaces CInstrumentFile.
*/
class CSimpleFile : public CFile
{
public:
	CSimpleFile(LPCTSTR lpszFileName, UINT nOpenFlags);

	void	WriteChar(char Value);
	void	WriteShort(short Value);
	void	WriteInt(int Value);
	void	WriteString(CString Str);
	void	WriteStringNull(CString Buf);

	char	ReadChar();
	short	ReadShort();
	int		ReadInt();
	CString	ReadString();
	CString ReadStringNull();
};
