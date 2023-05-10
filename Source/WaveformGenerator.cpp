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

#include "WaveformGenerator.h"

//
// CWavegenParam class
//

CWavegenParam::CWavegenParam(wavegen_param_type_t Type, const char *Name) :
	m_iParamType(Type),
	m_pParamName(Name)
{
}

CWavegenParam::~CWavegenParam()
{
}

wavegen_param_type_t CWavegenParam::GetType() const
{
	return m_iParamType;
}

const char* CWavegenParam::GetName() const
{
	return m_pParamName;
}

//
// CWavegenParamUnsigned class
//

CWavegenParamUnsigned::CWavegenParamUnsigned(const char *Name) :
	CWavegenParam(WGPARAM_UNSIGNED, Name),
	m_iData(0)
{
}

unsigned int CWavegenParamUnsigned::GetValue() const
{
	return m_iData;
}

bool CWavegenParamUnsigned::SetValue(const void *Val)
{
	m_iData = *reinterpret_cast<const unsigned int*>(Val);
	return true;
}

//
// CWavegenParamFloat class
//

CWavegenParamFloat::CWavegenParamFloat(const char *Name) :
	CWavegenParam(WGPARAM_FLOAT, Name),
	m_fData(0.f)
{
}

float CWavegenParamFloat::GetValue() const
{
	return m_fData;
}

bool CWavegenParamFloat::SetValue(const void *Val)
{
	m_fData = *reinterpret_cast<const float*>(Val);
	return true;
}

//
// CWavegenParamBoolean class
//

CWavegenParamBoolean::CWavegenParamBoolean(const char *Name) :
	CWavegenParam(WGPARAM_BOOLEAN, Name),
	m_bData(false)
{
}

bool CWavegenParamBoolean::GetValue() const
{
	return m_bData;
}

bool CWavegenParamBoolean::SetValue(const void *Val)
{
	m_bData = *reinterpret_cast<const bool*>(Val);
	return true;
}

//
// CWavegenParamString class
//

const size_t CWavegenParamString::MAX_LENGTH = 1024;

CWavegenParamString::CWavegenParamString(const char *Name) :
	CWavegenParam(WGPARAM_STRING, Name)
{
}

const char *CWavegenParamString::GetValue() const
{
	return m_pData.c_str();
}

bool CWavegenParamString::SetValue(const void *Val)
{
	char Dest[MAX_LENGTH] = { };
	char *d = Dest;
	const char *Src = reinterpret_cast<const char*>(Val);
	size_t count = 0;
	while (*Src) {
		if (++count == MAX_LENGTH)
			return false;
		*d++ = *Src++;
	}
	m_pData = Dest;
	return true;
}
