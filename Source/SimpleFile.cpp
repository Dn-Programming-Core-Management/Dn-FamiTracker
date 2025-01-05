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
