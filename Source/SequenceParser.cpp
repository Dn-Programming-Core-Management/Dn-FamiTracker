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

#include "stdafx.h"
#include "SequenceParser.h"
#include "Sequence.h"
#include <regex>

// // //

std::string CSeqConversionDefault::ToString(char Value) const
{
	return std::to_string(Value);
}

bool CSeqConversionDefault::ToValue(const std::string &String)
{
	m_bReady = false;
	if (String == "$$") {
		m_bHex = true;
		return true;
	}
	auto b = String.begin(), e = String.end();

	if (!GetNextTerm(b, e, m_iCurrentValue))
		return false;
	m_iTargetValue = m_iCurrentValue;
	m_iRepeat = m_iValueDiv = 1;
	if (b != e && *b == ':') {
		if (++b == e)
			return false;
		if (!GetNextInteger(b, e, m_iValueDiv))
			return false;
		if (b != e && *b == ':') {
			if (++b == e)
				return false;
			if (!GetNextTerm(b, e, m_iTargetValue))
				return false;
		}
	}
	if (b != e && *b == '\'') {
		if (++b == e)
			return false;
		if (!GetNextInteger(b, e, m_iRepeat))
			return false;
	}
	if (b != e || m_iRepeat <= 0)
		return false;
	m_iValueInc = m_iTargetValue - m_iCurrentValue;
	m_iRepeatCounter = m_iCounter = m_iValueMod = 0;
	return m_bReady = true;
}

bool CSeqConversionDefault::IsReady() const
{
	return m_bReady;
}

char CSeqConversionDefault::GetValue()
{
	// do not use float division
	int Val = m_iCurrentValue;
	if (Val > m_iMaxValue) Val = m_iMaxValue;
	if (Val < m_iMinValue) Val = m_iMinValue;
	if (++m_iRepeatCounter >= m_iRepeat) {
		m_iValueMod += m_iValueInc;
		m_iCurrentValue += m_iValueMod / m_iValueDiv;
		m_iValueMod %= m_iValueDiv;
		m_iRepeatCounter = 0;
		if (++m_iCounter >= m_iValueDiv)
			m_bReady = false;
	}
	return Val;
}

void CSeqConversionDefault::OnStart()
{
	m_bHex = false;
}

void CSeqConversionDefault::OnFinish()
{
	m_bReady = false;
}

bool CSeqConversionDefault::GetNextInteger(std::string::const_iterator &b, std::string::const_iterator &e, int &Out, bool Signed) const
{
	std::smatch m;

	try {
		if (m_bHex) {
			static const std::regex HEX_RE {R"(^([\+-]?)[0-9A-Fa-f]+)"};
			if (!std::regex_search(b, e, m, HEX_RE))
				return false;
			if (Signed && !m[1].length())
				return false;
			Out = std::stoi(m.str(), nullptr, 16);
		}
		else {
			static const std::regex NUMBER_RE {R"(^([\+-]?)[0-9]+)"};
			static const std::regex HEX_PREFIX_RE {R"(([\+-]?)[\$x]([0-9A-Fa-f]+))"}; // do not allow 0x prefix
			if (std::regex_search(b, e, m, HEX_PREFIX_RE)) {
				if (Signed && !m[1].length())
					return false;
				Out = std::stoi(m.str(2), nullptr, 16);
				if (m.str(1) == "-")
					Out = -Out;
			}
			else if (std::regex_search(b, e, m, NUMBER_RE)) {
				if (Signed && !m[1].length())
					return false;
				Out = std::stoi(m.str());
			}
			else
				return false;
		}
	}
	catch (std::out_of_range &) {
		return false;
	}

	b = m.suffix().first;
	return true;
}

bool CSeqConversionDefault::GetNextTerm(std::string::const_iterator &b, std::string::const_iterator &e, int &Out)
{
	return GetNextInteger(b, e, Out);
}



std::string CSeqConversion5B::ToString(char Value) const
{
	std::string Str = std::to_string(Value & 0x1F);
	if (Value & S5B_MODE_SQUARE)
		Str.push_back('t');
	if (Value & S5B_MODE_NOISE)
		Str.push_back('n');
	if (Value & S5B_MODE_ENVELOPE)
		Str.push_back('e');
	return Str;
}

bool CSeqConversion5B::ToValue(const std::string &String)
{
	m_iEnableFlags = -1;
	return CSeqConversionDefault::ToValue(String);
}

char CSeqConversion5B::GetValue()
{
	return CSeqConversionDefault::GetValue() | m_iEnableFlags;
}

bool CSeqConversion5B::GetNextTerm(std::string::const_iterator &b, std::string::const_iterator &e, int &Out)
{
	if (!GetNextInteger(b, e, Out))
		return false;

	static const std::regex S5B_FLAGS_RE {R"(^[TtNnEe]*)"};
	std::smatch m;
	if (std::regex_search(b, e, m, S5B_FLAGS_RE)) {
		if (m_iEnableFlags == -1) {
			m_iEnableFlags = 0;
			if (m.str().find_first_of("Tt") != std::string::npos)
				m_iEnableFlags |= S5B_MODE_SQUARE;
			if (m.str().find_first_of("Nn") != std::string::npos)
				m_iEnableFlags |= S5B_MODE_NOISE;
			if (m.str().find_first_of("Ee") != std::string::npos)
				m_iEnableFlags |= S5B_MODE_ENVELOPE;
		}
		b = m.suffix().first;
	}
	return true;
}



std::string CSeqConversionArpScheme::ToString(char Value) const		// // //
{
	int Offset = m_iMinValue + ((Value - m_iMinValue) & 0x3F);
	char Scheme = Value & 0xC0;
	if (!Offset) {
		switch (Scheme) {
		case ARPSCHEME_MODE_X: return "x";
		case ARPSCHEME_MODE_Y: return "y";
		case ARPSCHEME_MODE_NEG_Y: return "-y";
		default: return "0";
		}
	}
	std::string Str = std::to_string(Offset);
	switch (Scheme) {
	case ARPSCHEME_MODE_X: Str += "+x"; break;
	case ARPSCHEME_MODE_Y: Str += "+y"; break;
	case ARPSCHEME_MODE_NEG_Y: Str += "-y"; break;
	}
	return Str;
}

bool CSeqConversionArpScheme::ToValue(const std::string &String)
{
	m_iArpSchemeFlag = -1;
	return CSeqConversionDefault::ToValue(String);
}

char CSeqConversionArpScheme::GetValue()
{
	return (CSeqConversionDefault::GetValue() & 0x3F) | m_iArpSchemeFlag;
}

bool CSeqConversionArpScheme::GetNextTerm(std::string::const_iterator &b, std::string::const_iterator &e, int &Out)
{
	const auto SchemeFunc = [&] (const std::string &str) {
		if (m_iArpSchemeFlag != -1) return;
		m_iArpSchemeFlag = 0;
		if (str == "x" || str == "+x")
			m_iArpSchemeFlag = static_cast<unsigned char>(ARPSCHEME_MODE_X);
		else if (str == "y" || str == "+y")
			m_iArpSchemeFlag = static_cast<unsigned char>(ARPSCHEME_MODE_Y);
		else if (str == "-y")
			m_iArpSchemeFlag = static_cast<unsigned char>(ARPSCHEME_MODE_NEG_Y);
	};

	std::smatch m;
	static const std::regex SCHEME_HEAD_RE {R"(^(x|y|-y))"};
	static const std::regex SCHEME_TAIL_RE {R"(^(\+x|\+y|-y)?)"};
	
	if (std::regex_search(b, e, m, SCHEME_HEAD_RE)) {
		SchemeFunc(m.str());
		b = m.suffix().first;
		Out = 0;
		GetNextInteger(b, e, Out, true); // optional
	}
	else {
		if (!GetNextInteger(b, e, Out))
			return false;
		if (std::regex_search(b, e, m, SCHEME_TAIL_RE)) {
			SchemeFunc(m.str()); // optional
			b = m.suffix().first;
		}
	}
	return true;
}



std::string CSeqConversionArpFixed::ToString(char Value) const
{
	stChanNote Note;
	Note.Note = GET_NOTE(static_cast<unsigned char>(Value));
	Note.Octave = GET_OCTAVE(static_cast<unsigned char>(Value));
	return std::string {Note.ToString().GetBuffer()};
}

bool CSeqConversionArpFixed::GetNextTerm(std::string::const_iterator &b, std::string::const_iterator &e, int &Out)
{
	auto _b(b), _e(e);
	if (CSeqConversionDefault::GetNextTerm(_b, _e, Out)) {
		b = _b;
		e = _e;
		return true;
	}

	std::smatch m;
	static const std::regex FIXED_RE {R"(([A-Ga-g])([+\-#b]*)([0-9]+))"};
	static const int NOTE_OFFSET[] = {9, 11, 0, 2, 4, 5, 7};
	if (!std::regex_match(b, e, m, FIXED_RE))
		return false;

	char ch = m.str(1)[0];
	int Note = NOTE_OFFSET[ch >= 'a' ? ch - 'a' : ch - 'A'];
	for (const auto &acc : m.str(2)) {
		switch (acc) {
		case '+': case '#': ++Note; break;
		case '-': case 'b': --Note; break;
		}
	}
	
	try {
		Note += NOTE_RANGE * std::stoi(m.str(3));
	}
	catch (std::out_of_range &) {
		return false;
	}

	Out = Note;
	b = m.suffix().first;
	return true;
}



void CSequenceParser::SetSequence(CSequence *pSeq)
{
	ASSERT(pSeq != nullptr);
	m_pSequence = pSeq;
}

void CSequenceParser::SetConversion(CSeqConversionBase *pConv)
{
	ASSERT(pConv != nullptr);
	m_pConversion.reset(pConv);
}

void CSequenceParser::ParseSequence(const std::string &String)
{
	auto Setting = static_cast<seq_setting_t>(m_pSequence->GetSetting());
	m_pSequence->Clear();
	m_pSequence->SetSetting(Setting);
	m_iPushedCount = 0;
	static const std::regex SPLIT_RE {R"(\S+)"};

	const auto PushFunc = [&] () {
		while (m_pConversion->IsReady()) {
			m_pSequence->SetItem(m_iPushedCount, m_pConversion->GetValue());
			if (++m_iPushedCount >= MAX_SEQUENCE_ITEMS)
				break;
		}
	};

	int Loop = -1, Release = -1;

	m_pConversion->OnStart();
	PushFunc();
	for (auto it = std::sregex_iterator {String.begin(), String.end(), SPLIT_RE},
			  end = std::sregex_iterator { }; it != end; ++it) {
		if (it->str() == "|") {
			Loop = m_iPushedCount; continue;
		}
		if (it->str() == "/") {
			Release = m_iPushedCount; continue;
		}
		if (m_pConversion->ToValue(it->str()))
			PushFunc();
	}
	m_pConversion->OnFinish();
	PushFunc();
	m_pSequence->SetItemCount(m_iPushedCount);
	m_pSequence->SetLoopPoint(Loop);
	m_pSequence->SetReleasePoint(Release);
}

std::string CSequenceParser::PrintSequence() const
{
	std::string str;

	const int Loop = m_pSequence->GetLoopPoint();
	const int Release = m_pSequence->GetReleasePoint();

	for (int i = 0, Count = m_pSequence->GetItemCount(); i < Count; ++i) {
		if (i == Loop)
			str.append("| ");
		if (i == Release)
			str.append("/ ");
		str.append(m_pConversion->ToString(m_pSequence->GetItem(i)));
		str.push_back(' ');
	}

	return str;
}
