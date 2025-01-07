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

#include "DetuneTable.h"
#include "APU/APU.h"
#include <cmath>

// // // detune table class

CDetuneTable::CDetuneTable(type_t Type, int Low, int High, double A440_Note) :
	m_iType(Type),
	m_iRangeLow(Low),
	m_iRangeHigh(High),
	m_iRegisterValue(96, 0U),
	m_iOffsetValue(96, 0U),
	m_dA440_NOTE(A440_Note)
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

double CDetuneTable::NoteToFreq(double Note) const
{
	return 440.0 * std::pow(2.0, (Note - m_dA440_NOTE) / 12.0);
}

double CDetuneTable::FreqToNote(double Freq) const
{
	return m_dA440_NOTE + std::log2(Freq / 440.0) * 12.0;
}

unsigned int CDetuneTable::FrequencyToPeriod(double Freq, int Octave, int NamcoChannels) const
{
	return 0;
}

double CDetuneTable::PeriodToFrequency(unsigned int Period, int Octave, int NamcoChannels) const
{
	return 0.0;
}

// https://www.nesdev.org/wiki/APU_Pulse#Sequencer_behavior
// no compensation is done for triangle
CDetuneNTSC::CDetuneNTSC(double A440_Note) :
	CDetuneTable(DETUNE_NTSC, 0, 0x7FF, A440_Note)
{
	SetGenerator([this](double x) { return FrequencyToPeriod(x, 1, 0); });
	SetFrequencyFunc([this](double x) { return PeriodToFrequency(static_cast<int>(x), 1, 0); });
	GenerateRegisters();
}

// period = CPU_clk / (16 * frequency) - 1
unsigned int CDetuneNTSC::FrequencyToPeriod(double Freq, int Octave, int NamcoChannels) const
{
	return std::lround((CAPU::BASE_FREQ_NTSC / (Freq * 16.0)) - 1.0);
}

// frequency = CPU_clk / 16 * (period + 1)
double CDetuneNTSC::PeriodToFrequency(unsigned int Period, int Octave, int NamcoChannels) const
{
	return CAPU::BASE_FREQ_NTSC / (16.0 * ((double)Period + 1.0));
}

// same as CDetuneNTSC, but with different clock frequency
CDetunePAL::CDetunePAL(double A440_Note) :
	CDetuneTable(DETUNE_PAL, 0, 0x7FF, A440_Note)
{
	SetGenerator([this](double x) { return FrequencyToPeriod(x, 1, 0); });
	SetFrequencyFunc([this](double x) { return PeriodToFrequency(static_cast<int>(x), 1, 0); });
	GenerateRegisters();
}

unsigned int CDetunePAL::FrequencyToPeriod(double Freq, int Octave, int NamcoChannels) const
{
	return std::lround((CAPU::BASE_FREQ_PAL / (Freq * 16.0)) - 1.0);
}

double CDetunePAL::PeriodToFrequency(unsigned int Period, int Octave, int NamcoChannels) const
{
	return CAPU::BASE_FREQ_PAL / (16.0 * ((double)Period + 1.0));
}

// https://www.nesdev.org/wiki/VRC6_audio#Sawtooth_Channel
CDetuneSaw::CDetuneSaw(double A440_Note) :
	CDetuneTable(DETUNE_SAW, 0, 0xFFF, A440_Note)
{
	SetGenerator([this](double x) { return FrequencyToPeriod(x, 1, 0); });
	SetFrequencyFunc([this](double x) { return PeriodToFrequency(static_cast<int>(x), 1, 0); });
	GenerateRegisters();
}

// period = (CPU_clk / (14 * frequency)) - 1
unsigned int CDetuneSaw::FrequencyToPeriod(double Freq, int Octave, int NamcoChannels) const
{
	return std::lround((CAPU::BASE_FREQ_NTSC / (Freq * 14.0)) - 1.0);
}

// frequency = CPU_clk / (14 * (period + 1))
double CDetuneSaw::PeriodToFrequency(unsigned int Period, int Octave, int NamcoChannels) const
{
	return CAPU::BASE_FREQ_NTSC / (14.0 * ((double)Period + 1.0));
}

// https://www.nesdev.org/wiki/VRC7_audio#Channels
CDetuneVRC7::CDetuneVRC7(double A440_Note) :
	CDetuneTable(DETUNE_VRC7, 0, 0x1FF, A440_Note)
{
	SetNoteCount(12);
	SetGenerator([this](double x) { return FrequencyToPeriod(x, 1, 0); });
	SetFrequencyFunc([this](double x) { return PeriodToFrequency(static_cast<int>(x), 1, 0); });
	GenerateRegisters();
}

unsigned int CDetuneVRC7::FrequencyToPeriod(double Freq, int Octave, int NamcoChannels) const
{
	return std::lround((Freq * std::pow(2, (19 - Octave))) / (CAPU::BASE_FREQ_VRC7 / 72.0));
}

// frequency = (VRC7_Xclock / 72 * period) / 2^(19 - octave - 1)
double CDetuneVRC7::PeriodToFrequency(unsigned int Period, int Octave, int NamcoChannels) const
{
	return ((CAPU::BASE_FREQ_VRC7 / 72.0) * (double)Period) / std::pow(2, (19 - Octave - 1));
}

CDetuneFDS::CDetuneFDS(double A440_Note) :
	CDetuneTable(DETUNE_FDS, 0, 0xFFF, A440_Note)
{
	SetGenerator([this](double x) { return FrequencyToPeriod(x, 1, 0); });
	SetFrequencyFunc([this](double x) { return PeriodToFrequency(static_cast<int>(x), 1, 0); });
	GenerateRegisters();
}

unsigned int CDetuneFDS::FrequencyToPeriod(double Freq, int Octave, int NamcoChannels) const
{
	return std::lround((Freq * 65536.0 * 16.0) / (CAPU::BASE_FREQ_NTSC));
}

double CDetuneFDS::PeriodToFrequency(unsigned int Period, int Octave, int NamcoChannels) const
{
	return (CAPU::BASE_FREQ_NTSC * (double)Period) / (65536.0);
}

// https://www.nesdev.org/wiki/Namco_163_audio#Frequency
CDetuneN163::CDetuneN163(double A440_Note) :
	CDetuneTable(DETUNE_N163, 0, 0xFFFF, A440_Note),
	m_iChannelCount(1)
{
	SetGenerator([this](double x) { return FrequencyToPeriod(x, 1, m_iChannelCount); });
	SetFrequencyFunc([this](double x) { return PeriodToFrequency(static_cast<int>(x), 1, m_iChannelCount); });
	GenerateRegisters();
}

// period = (frequency * 15 * (65536 * 4) * channel_count) / CPU_clk
unsigned int CDetuneN163::FrequencyToPeriod(double Freq, int Octave, int NamcoChannels) const
{
	return std::lround((Freq * 15.0 * 262144.0 * NamcoChannels) / CAPU::BASE_FREQ_NTSC);
}

// frequency = (CPU_clk * period) / (15 * (65536 * 4) * channel_count)
double CDetuneN163::PeriodToFrequency(unsigned int Period, int Octave, int NamcoChannels) const
{
	return std::lround((CAPU::BASE_FREQ_NTSC * (double)Period) / (15.0 * 262144.0 * NamcoChannels));
}

void CDetuneN163::SetChannelCount(unsigned Count) // special
{
	m_iChannelCount = Count;
}

// https://www.nesdev.org/wiki/Sunsoft_5B_audio#Sound
CDetuneS5B::CDetuneS5B(double A440_Note) :
	CDetuneTable(DETUNE_S5B, 0, 0xFFF, A440_Note)
{
	SetGenerator([this](double x) { return FrequencyToPeriod(x, 1, 0); });
	SetFrequencyFunc([this](double x) { return PeriodToFrequency(static_cast<int>(x), 1, 0); });
	GenerateRegisters();
}

// in the driver, S5B's note LUT uses the regular NTSC note LUT +- 1, since it's off-by-one
// so for compatibility, the frequency is doubled
// period = CPU_clk / (frequency * 16)
unsigned int CDetuneS5B::FrequencyToPeriod(double Freq, int Octave, int NamcoChannels) const
{
	return std::lround(CAPU::BASE_FREQ_NTSC / (Freq * 16.0));
}

// frequency = CPU_clk / (16 * period)
double CDetuneS5B::PeriodToFrequency(unsigned int Period, int Octave, int NamcoChannels) const
{
	return CAPU::BASE_FREQ_NTSC / (16.0 * ((double)Period));
}
