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

#include "WaveformGenerator.h"
#include "WavegenBuiltin.h"
#include <math.h> // sine

//
// CWavegenImp class
//

const char *CWavegenImp::DEFAULT_ERROR = "Waveform generation failed.";

CWavegenImp::CWavegenImp(const char *Name) :
	m_bSuccess(false),
	m_pName(Name),
	m_pError(&DEFAULT_ERROR)
{
}

bool CWavegenImp::CreateWaves(float *const Dest, unsigned int Size, unsigned int Count)
{
	const char *err = CreateWavesInternal(Dest, Size, Count);
	if (err != nullptr)
		m_pError = &err;
	return (m_bSuccess = err == nullptr);
}

const char *CWavegenImp::GetGeneratorName() const
{
	return m_pName;
}

const char *CWavegenImp::GetStatus() const
{
	return m_bSuccess ? nullptr : *m_pError;
}

//
// CWavegenSingle class
//

const char *CWavegenSingle::COUNT_ERROR = "Wave count of this waveform generator must be 1.";

CWavegenSingle::CWavegenSingle(const char *Name) :
	CWavegenImp(Name)
{
}

bool CWavegenSingle::CreateWaves(float *const Dest, unsigned int Size, unsigned int Index)
{
	if (Index != 1 && Index != -1) {
		m_pError = &COUNT_ERROR;
		return (m_bSuccess = false);
	}
	return CWavegenImp::CreateWaves(Dest, Size, Index);
}

unsigned int CWavegenSingle::GetCount() const
{
	return 1U;
}

//
// CWavegenSine class
//

CWavegenSine::CWavegenSine() :
	CWavegenSingle("Sine wave")
{
}

const char *CWavegenSine::CreateWavesInternal(float *const Dest, unsigned int Size, unsigned int Index) const
{
	float *t = Dest;
	for (unsigned int i = 0; i < Size; ++i)
		*t++ = sinf(6.28318531f * i / Size);

	return nullptr;
}

CWavegenParam *CWavegenSine::GetParameter(unsigned int Index) const
{
	return nullptr;
}

//
// CWavegenSawtooth class
//

CWavegenSawtooth::CWavegenSawtooth() :
	CWavegenSingle("Triangle wave")
{
}

const char *CWavegenSawtooth::CreateWavesInternal(float *const Dest, unsigned int Size, unsigned int Index) const
{
	float *t = Dest;
	for (unsigned int i = 0; i < Size; ++i)
		*t++ = -1.f + 2.f * i / (Size - 1);

	return nullptr;
}

CWavegenParam *CWavegenSawtooth::GetParameter(unsigned int Index) const
{
	return nullptr;
}

//
// CWavegenTriangle class
//

CWavegenTriangle::CWavegenTriangle() :
	CWavegenSingle("Triangle wave")
{
}

const char *CWavegenTriangle::CreateWavesInternal(float *const Dest, unsigned int Size, unsigned int Index) const
{
	float *t = Dest;
	const unsigned int Half = Size >> 1;
	for (unsigned int i = 0; i < Half; ++i)
		*t++ = -1.f + 2.f * i / (Half - 1);
	for (unsigned int i = 0; i < Size - Half; ++i)
		*t++ = 1.f - 2.f * i / (Half - 1);

	return nullptr;
}

CWavegenParam *CWavegenTriangle::GetParameter(unsigned int Index) const
{
	return nullptr;
}


//
// CWavegenPulse class
//

const char *CWavegenPulse::PULSE_WIDTH_ERROR = "Pulse width must be between 0 and 1.";

CWavegenPulse::CWavegenPulse() :
	CWavegenSingle("Pulse wave")
{
	m_pPulseWidth = new CWavegenParamFloat("Pulse width");
}

CWavegenPulse::~CWavegenPulse()
{
	if (m_pPulseWidth != nullptr)
		delete m_pPulseWidth;
}

const char *CWavegenPulse::CreateWavesInternal(float *const Dest, unsigned int Size, unsigned int Index) const
{
	float width = m_pPulseWidth->GetValue();
	if (width > 1 || width < 0)
		return PULSE_WIDTH_ERROR;
	float *t = Dest;
	const float thresh = Size - m_pPulseWidth->GetValue() * Size;
	for (unsigned int i = 0; i < Size; ++i)
		*t++ = i >= thresh ? 1.f : -1.f;

	return nullptr;
}

CWavegenParam *CWavegenPulse::GetParameter(unsigned int Index) const
{
	switch (Index) {
	case 0U: return m_pPulseWidth;
	}
	return nullptr;
}