/*
** FamiTracker - NES/Famicom sound tracker
** Copyright (C) 2005-2014  Jonathan Liss
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
	/*!	\brief Constructor of the detune table.
		\param Type An index representing the detune table type.
		\param Low The smallest register value this table supports. 
		\param High The greatest register value this table supports. */
	CDetuneTable(type_t Type, int Low, int High);

	typedef std::function<double(double)> GenFunc;

public:
	/*!	\brief Gets the detune table type.
		\return An index representing the detune table type. */
	type_t GetType() const;
	/*!	\brief Gets the size of the detune table.
		\return The number of notes in the detune table. */
	size_t GetNoteCount() const;
	/*!	\brief Gets the number of used entries in the detune table.
		\return The number of notes with a non-zero detune value in the table. */
	size_t GetUsedCount() const;

	/*!	\brief Obtains the actual register value representing a given pitch.
		\param Note The note index.
		\return Register value. */
	int GetRegisterValue(unsigned Note) const;
	/*!	\brief Obtains the detune offset for a given pitch.
		\param Note The note index.
		\return The offset value. */
	int GetOffsetValue(unsigned Note) const;
	/*!	\brief Updates the detune offset for a given pitch.
		\param Note The note index.
		\param Offset The new offset value. */
	void SetOffset(unsigned Note, int Offset);
	/*!	\brief Generates the base register values of the detune table.
		\param LowestNote The note index for the first entry of the detune table. */
	void GenerateRegisters(double LowestNote = 0.);

	/*!	\brief Converts a note index to a register value for the detune table.
		\param Note The note index, which may be a fractional number.
		\return Register value. */
	double GetDefaultReg(double Note) const;

	/*!	\brief Converts a note index to its frequency.
		\param Note The note index, which may be a fractional number.
		\return The note's frequency. */
	static double NoteToFreq(double Note);
	/*!	\brief Converts a frequency value to a note index.
		\param Freq The frequency value.
		\return The note index, which may be a fractional number. */
	static double FreqToNote(double Freq);

protected:
	void SetNoteCount(size_t Count);
	void SetGenerator(GenFunc f);
	void SetFrequencyFunc(GenFunc f);

protected:
	const type_t m_iType;
	const int m_iRangeLow, m_iRangeHigh;
	std::vector<int> m_iRegisterValue;
	std::vector<int> m_iOffsetValue;
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
