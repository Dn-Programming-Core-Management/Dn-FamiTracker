/*
** FamiTracker - NES/Famicom sound tracker
** Copyright (C) 2005-2020 Jonathan Liss
**
** 0CC-FamiTracker is (C) 2014-2018 HertzDevil
**
** Dn-FamiTracker is (C) 2020-2023 D.P.C.M.
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

#include "SimpleFile.h"

// // // File load / store

CSimpleFile::CSimpleFile(LPCTSTR lpszFileName, UINT nOpenFlags) :
	CFile(lpszFileName, nOpenFlags)
{
}

void CSimpleFile::WriteChar(char Value)
{
	Write(&Value, sizeof(char));
}

void CSimpleFile::WriteShort(short Value)
{
	Write(&Value, sizeof(short));
}

void CSimpleFile::WriteInt(int Value)
{
	Write(&Value, sizeof(int));
}

void CSimpleFile::WriteString(CString Str)
{
	int Len = Str.GetLength();
	WriteInt(Len);
	Write(CT2CA(Str).m_psz, Len);
}

void CSimpleFile::WriteStringNull(CString Str)
{
	CT2CA s(Str);
	Write(s, static_cast<UINT>(strlen(s.m_psz) + 1));
}

char CSimpleFile::ReadChar()
{
	char Value;
	Read(&Value, sizeof(char));
	return Value;
}

short CSimpleFile::ReadShort()
{
	short Value;
	Read(&Value, sizeof(short));
	return Value;
}

int CSimpleFile::ReadInt()
{
	int Value;
	Read(&Value, sizeof(int));
	return Value;
}

CString CSimpleFile::ReadString()
{
	const int Size = ReadInt();
	LPSTR Buf = new char[Size + 1];
	Buf[Size] = '\0';
	return CString(CA2CT(Buf));
}

CString CSimpleFile::ReadStringNull()
{
	CStringA str;
	while (char ch = ReadChar())
		str += ch;
	return CString(CA2CT(str));
}
