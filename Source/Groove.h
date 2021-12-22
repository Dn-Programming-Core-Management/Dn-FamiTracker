/*
** FamiTracker - NES/Famicom sound tracker
** Copyright (C) 2005-2015 Jonathan Liss
**
** 0CC-FamiTracker is (C) 2014-2018 HertzDevil
**
** Dn-FamiTracker is (C) 2020-2021 D.P.C.M.
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

#pragma once

#define MAX_GROOVE_SIZE 128

class CGroove
{
public:
	CGroove(int Speed = 0);

	void Copy(const CGroove *Source);
	void Clear(unsigned char Speed);
	unsigned char GetEntry(int Index) const;
	void SetEntry(unsigned char Index, unsigned char Value);
	unsigned char GetSize() const;
	void SetSize(unsigned char Size);
	float GetAverage() const;
private:
	unsigned char m_iLength;
	unsigned char m_iEntry[MAX_GROOVE_SIZE];
};