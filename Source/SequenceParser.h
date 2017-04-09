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


#pragma once

// // // 050B

#include <string>
#include <memory>

class CSequence;

/*!
	\brief A realization of the CSequenceStringConversion class from FamiTracker 0.5.0 beta, which
	handles conversion between sequence values and MML string terms.
*/
class CSeqConversionBase
{
public:
	/*!	\brief Converts a sequence value to a string.
		\param Value Input value from the sequence.
		\return A unique string representation of the sequence value. */
	virtual std::string ToString(char Value) const = 0; // TODO: maybe use a different class for this

	/*!	\brief Converts an MML string term to a sequence value.
		\details A string term may represent any number of sequence values, including zero.
		\param String A single MML string term.
		\return True if the string is a valid representation. */
	virtual bool ToValue(const std::string &String) = 0;
	/*!	\brief Checks the availability of the current string conversion.
		\details For each valid MML string term, **as well as before and after converting an MML
		string**, a new value is expected as long as this method returns true.
		\return True if a new value is ready for use. */
	virtual bool IsReady() const = 0;
	/*!	\brief Extracts a value from a string conversion.
		\returns The sequence value. */
	virtual char GetValue() = 0;

	/*!	\brief Called before converting an MML string. */
	virtual void OnStart() { }
	/*!	\brief Called after converting an MML string. */
	virtual void OnFinish() { }
};

class CSeqConversionDefault : public CSeqConversionBase
{
public:
	CSeqConversionDefault() { }
	CSeqConversionDefault(int Min, int Max) : m_iMinValue(Min), m_iMaxValue(Max) { }

public:
	std::string ToString(char Value) const override;
	bool ToValue(const std::string &String) override;
	bool IsReady() const override;
	char GetValue() override;
	void OnStart() override;
	void OnFinish() override;

protected:
	bool GetNextInteger(std::string::const_iterator &b, std::string::const_iterator &e, int &Out, bool Signed = false) const;
	virtual bool GetNextTerm(std::string::const_iterator &b, std::string::const_iterator &e, int &Out);

protected:
	const int m_iMinValue = INT32_MIN;
	const int m_iMaxValue = INT32_MAX;

private:
	bool m_bReady = false;
	bool m_bHex;

	int m_iCurrentValue, m_iCounter, m_iTargetValue;
	int m_iValueDiv, m_iValueMod, m_iValueInc;
	int m_iRepeat, m_iRepeatCounter;
};

class CSeqConversion5B : public CSeqConversionDefault
{
public:
	CSeqConversion5B() : CSeqConversionDefault(0, 0x1F) { }
	std::string ToString(char Value) const override;
	bool ToValue(const std::string &String) override;
	char GetValue() override;
protected:
	bool GetNextTerm(std::string::const_iterator &b, std::string::const_iterator &e, int &Out) override;
private:
	char m_iEnableFlags;
};

class CSeqConversionArpScheme : public CSeqConversionDefault		// // //
{
public:
	CSeqConversionArpScheme(int Min) : CSeqConversionDefault(Min, Min + 63) { }
	std::string ToString(char Value) const override;
	bool ToValue(const std::string &String) override;
	char GetValue() override;
protected:
	bool GetNextTerm(std::string::const_iterator &b, std::string::const_iterator &e, int &Out) override;
private:
	char m_iArpSchemeFlag;
};

class CSeqConversionArpFixed : public CSeqConversionDefault		// // //
{
public:
	CSeqConversionArpFixed() : CSeqConversionDefault(0, 95) { }
	std::string ToString(char Value) const override;
protected:
	bool GetNextTerm(std::string::const_iterator &b, std::string::const_iterator &e, int &Out) override;
};

/*!
	\brief A class which uses a conversion object to translate between string representations and
	instrument sequence values.
*/
class CSequenceParser
{
public:
	/*!	\brief Constructor of the sequence parser. */
	CSequenceParser() { }

	/*!	\brief Changes the instrument sequence used by the parser.
		\param pSeq Pointer to the new sequence. */
	void SetSequence(CSequence *pSeq);
	/*!	\brief Changes the conversion algorithm.
		\details The sequence parser owns the conversion object and handles its deletion.
		\param pConv Pointer to the new sequence conversion object. */
	void SetConversion(CSeqConversionBase *pConv);
	/*!	\brief Updates the current instrument sequence from an MML string.
		\param String the input string. */
	void ParseSequence(const std::string &String);
	/*!	\brief Obtains a string representation of the current instrument sequence.
		\return An MML string which represents the sequence. */
	std::string PrintSequence() const;

private:
	unsigned int m_iPushedCount = 0;
	CSequence *m_pSequence = nullptr;
	std::unique_ptr<CSeqConversionBase> m_pConversion = nullptr;
};
