/*
** FamiTracker - NES/Famicom sound tracker
** Copyright (C) 2005-2014  Jonathan Liss
**
** 0CC-FamiTracker is (C) 2014-2016 HertzDevil
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


#pragma once

#include <vector>
#include <functional>

class CDetuneTable
{
public:
	enum type_t {
		DETUNE_NTSC,
		DETUNE_PAL,
		DETUNE_SAW,
		DETUNE_VRC7,
		DETUNE_FDS,
		DETUNE_N163,
		DETUNE_S5B
	};

protected:
	CDetuneTable(type_t Type, unsigned Low, unsigned High);
	typedef std::function<double(double)> GenFunc;

public:
	type_t GetType() const;
	size_t GetNoteCount() const;
	void SetNoteCount(size_t Count);
	unsigned GetValue(unsigned Note) const;
	void SetValue(unsigned Note, unsigned Value);

	void SetGenerator(GenFunc f);
	void SetFrequencyFunc(GenFunc f);
	void Generate(double LowestNote = 0.);

	double GetDefaultReg(double Note) const;
	static double NoteToFreq(double Note);
	static double FreqToNote(double Freq);

protected:
	const type_t m_iType;
	const unsigned m_iRangeLow, m_iRangeHigh;
	std::vector<unsigned> m_iRegisterValue;
	GenFunc m_fFunction, m_fInvFunction;

protected:
	static const double BASE_FREQ_NTSC;

private:
	static const unsigned A440_NOTE;
};

class CDetuneNTSC : public CDetuneTable
{
public:
	CDetuneNTSC();
};

class CDetunePAL : public CDetuneTable
{
public:
	CDetunePAL();
};

class CDetuneSaw : public CDetuneTable
{
public:
	CDetuneSaw();
};

class CDetuneFDS : public CDetuneTable
{
public:
	CDetuneFDS();
};

class CDetuneN163 : public CDetuneTable
{
public:
	CDetuneN163();
	void SetChannelCount(unsigned Count);
private:
	unsigned m_iChannelCount;
};

class CDetuneVRC7 : public CDetuneTable
{
public:
	CDetuneVRC7();
};

class CDetuneS5B : public CDetuneTable
{
public:
	CDetuneS5B();
};
