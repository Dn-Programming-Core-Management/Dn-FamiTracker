/*
** FamiTracker - NES/Famicom sound tracker
** Copyright (C) 2005-2014  Jonathan Liss
**
** 0CC-FamiTracker is (C) 2014-2017 HertzDevil
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

#include "SimpleFile.h"

// // // File load / store

CSimpleFile::~CSimpleFile()
{
	if (m_fFile.is_open())
		m_fFile.close();
}

CSimpleFile::operator bool() const
{
	return m_fFile.is_open() && (bool)m_fFile;
}

void CSimpleFile::Close()
{
	m_fFile.close();
}

void CSimpleFile::WriteChar(char Value)
{
	m_fFile.put(Value);
}

void CSimpleFile::WriteShort(short Value)
{
	m_fFile.put(static_cast<char>(Value));
	m_fFile.put(static_cast<char>(Value >> 8));
}

void CSimpleFile::WriteInt(int Value)
{
	m_fFile.put(static_cast<char>(Value));
	m_fFile.put(static_cast<char>(Value >> 8));
	m_fFile.put(static_cast<char>(Value >> 16));
	m_fFile.put(static_cast<char>(Value >> 24));
}

void CSimpleFile::WriteBytes(const char *pBuf, size_t count)
{
	m_fFile.write(pBuf, count);
}

void CSimpleFile::WriteString(std::string_view sv)
{
	int Len = sv.size();
	WriteInt(Len);
	WriteBytes(sv.data(), Len);
}

void CSimpleFile::WriteStringNull(std::string_view sv)
{
	WriteBytes(sv.data(), sv.size());
	m_fFile.put('\0');
}

char CSimpleFile::ReadChar()
{
	char buf[1];
	m_fFile.read(buf, 1);
	return buf[0];
}

short CSimpleFile::ReadShort()
{
	char buf[2];
	m_fFile.read(buf, 2);
	return buf[0] | (buf[1] << 8);
}

int CSimpleFile::ReadInt()
{
	char buf[4];
	m_fFile.read(buf, 4);
	return buf[0] | (buf[1] << 8) | (buf[2] << 16) | (buf[3] << 24);
}

void CSimpleFile::ReadBytes(char *pBuf, size_t count) {
	m_fFile.read(pBuf, count);
}

std::string CSimpleFile::ReadString()
{
	const int Size = ReadInt();
	std::string str(Size, '\0');
	m_fFile.read(&str[0], Size);
	return str;
}

std::string CSimpleFile::ReadStringNull()
{
	std::string str;
	while (true) {
		char ch = m_fFile.get();
		if (!ch || !m_fFile)
			break;
		str += ch;
	}
	return str;
}
