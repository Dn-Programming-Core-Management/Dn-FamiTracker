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

#include "DetuneTable.h"
#include <cmath>

// // // detune table class

const unsigned CDetuneTable::A440_NOTE = 45U;
const double CDetuneTable::BASE_FREQ_NTSC = 630000000. / 352.;

CDetuneTable::CDetuneTable(type_t Type, int Low, int High) :
	m_iType(Type),
	m_iRangeLow(Low),
	m_iRangeHigh(High),
	m_iRegisterValue(96, 0U),
	m_iOffsetValue(96, 0U)
{
}

CDetuneTable::type_t CDetuneTable::GetType() const
{
	return m_iType;
}

size_t CDetuneTable::GetNoteCount() const
{
	return m_iRegisterValue.size();
}

void CDetuneTable::SetNoteCount(size_t Count)
{
	m_iRegisterValue.resize(Count);
	m_iOffsetValue.resize(Count);
}

int CDetuneTable::GetRegisterValue(unsigned Note) const
{
	size_t Index = Note % GetNoteCount();
	int Val = m_iRegisterValue[Index] + m_iOffsetValue[Index];
	if (Val < m_iRangeLow) Val = m_iRangeLow;
	if (Val > m_iRangeHigh) Val = m_iRangeHigh;
	return Val;
}

int CDetuneTable::GetOffsetValue(unsigned Note) const
{
	return m_iOffsetValue[Note % GetNoteCount()];
}

void CDetuneTable::SetOffset(unsigned Note, int Value)
{
	m_iOffsetValue[Note % GetNoteCount()] = Value;
}

void CDetuneTable::SetGenerator(GenFunc f)
{
	m_fFunction = f;
}

void CDetuneTable::SetFrequencyFunc(GenFunc f)
{
	m_fInvFunction = f; // might not be needed
}

void CDetuneTable::GenerateRegisters(double LowestNote)
{
	const size_t Count = GetNoteCount();
	for (size_t i = 0; i < Count; ++i)
		m_iRegisterValue[i] = static_cast<unsigned>(GetDefaultReg(LowestNote++) + .5);
}

double CDetuneTable::GetDefaultReg(double Note) const
{
	double Val = m_fFunction(NoteToFreq(Note));
	if (Val < m_iRangeLow) Val = m_iRangeLow;
	if (Val > m_iRangeHigh) Val = m_iRangeHigh;
	return Val;
}

double CDetuneTable::NoteToFreq(double Note)
{
	return 440. * std::pow(2., (Note - A440_NOTE) / 12.);
}

double CDetuneTable::FreqToNote(double Freq)
{
	return static_cast<double>(A440_NOTE) + std::log2(Freq / 440.) * 12.;
}

CDetuneNTSC::CDetuneNTSC() :
	CDetuneTable(DETUNE_NTSC, 0, 0x7FF)
{
	SetGenerator([] (double x) { return BASE_FREQ_NTSC / 16. / x - 1.; });
	SetFrequencyFunc([] (double x) { return BASE_FREQ_NTSC / 16. / (x + 1.); });
	GenerateRegisters();
}

CDetunePAL::CDetunePAL() :
	CDetuneTable(DETUNE_PAL, 0, 0x7FF)
{
	SetGenerator([] (double x) { return 266017125. / 16. / 16. / x - 1.; });
	SetFrequencyFunc([] (double x) { return 266017125. / 16. / 16. / (x + 1.); });
	GenerateRegisters();
}

CDetuneSaw::CDetuneSaw() :
	CDetuneTable(DETUNE_SAW, 0, 0xFFF)
{
	SetGenerator([] (double x) { return BASE_FREQ_NTSC / 14. / x - 1.; });
	SetFrequencyFunc([] (double x) { return BASE_FREQ_NTSC / 14. / (x + 1.); });
	GenerateRegisters();
}

CDetuneVRC7::CDetuneVRC7() :
	CDetuneTable(DETUNE_VRC7, 0, 0x1FF)
{
	SetNoteCount(12);
	SetGenerator([] (double x) { return x / 49716. * (1 << 18); });		// provisional
	SetFrequencyFunc([] (double x) { return 49716. * std::fmod(x, 512.) / (1 << (18 - (static_cast<int>(x) >> 9))); });
	GenerateRegisters();
}

CDetuneFDS::CDetuneFDS() :
	CDetuneTable(DETUNE_FDS, 0, 0xFFF)
{
	SetGenerator([] (double x) { return x / BASE_FREQ_NTSC * (1 << 20); });
	SetFrequencyFunc([] (double x) { return BASE_FREQ_NTSC * x / (1 << 20); });
	GenerateRegisters();
}

CDetuneN163::CDetuneN163() :
	CDetuneTable(DETUNE_N163, 0, 0xFFFF),
	m_iChannelCount(1)
{
	SetGenerator([&] (double x) { return x / BASE_FREQ_NTSC * 15. * (1 << 18) * m_iChannelCount; });
	SetFrequencyFunc([&] (double x) { return BASE_FREQ_NTSC * x / 15. / (1 << 18) / m_iChannelCount; });
	GenerateRegisters();
}

void CDetuneN163::SetChannelCount(unsigned Count) // special
{
	m_iChannelCount = Count;
}

CDetuneS5B::CDetuneS5B() :
	CDetuneTable(DETUNE_S5B, 0, 0xFFF)
{
	SetGenerator([] (double x) { return BASE_FREQ_NTSC / 16. / x; });
	SetFrequencyFunc([] (double x) { return BASE_FREQ_NTSC / 16. / x; });
	GenerateRegisters();
}
