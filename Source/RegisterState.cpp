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

#include "stdafx.h"
#include "RegisterState.h"

CRegisterLogger::CRegisterLogger() :
	m_mRegister(),
	m_iPort(0),
	m_bAutoIncrement(false),
	m_bBlocked(false)
{
}

void CRegisterLogger::Reset()
{
	for (auto &r : m_mRegister)
		r.second.Reset();
}

bool CRegisterLogger::AddRegisterRange(unsigned Low, unsigned High)
{
	for (unsigned i = Low; i <= High; ++i)
		if (m_mRegister.find(i) != m_mRegister.end()) // conflict
			return false;

	for (unsigned i = Low; i <= High; ++i)
		m_mRegister[i] = CRegisterState { };
	m_mWarpValues[++High] = Low;
	return true;
}

bool CRegisterLogger::SetPort(unsigned Address)
{
	m_iPort = Address;
	return m_mRegister.find(m_iPort) != m_mRegister.end();
}

void CRegisterLogger::SetAutoincrement(bool Enable)
{
	m_bAutoIncrement = Enable;
}

bool CRegisterLogger::Write(uint8_t Value)
{
	if (m_mRegister.find(m_iPort) == m_mRegister.end())
		return false;

	m_mRegister.at(m_iPort).Update(Value);
	if (m_bAutoIncrement)
		if (m_mWarpValues.find(++m_iPort) != m_mWarpValues.end())
			m_iPort = m_mWarpValues[m_iPort];

	return true;
}

CRegisterState *CRegisterLogger::GetRegister(unsigned Address)
{
	if (m_mRegister.find(Address) == m_mRegister.end())
		return nullptr;
	return &m_mRegister.at(Address);
}

void CRegisterLogger::Step()
{
	for (auto &r : m_mRegister)
		r.second.Step();
}

CRegisterLoggerBlock::CRegisterLoggerBlock(CRegisterLogger *Logger) :
	m_pLogger(Logger),
	m_iPort(Logger->m_iPort),
	m_bAutoIncrement(Logger->m_bAutoIncrement),
	m_bBlocked(Logger->m_bBlocked)
{
	Logger->m_bBlocked = true;
}

CRegisterLoggerBlock::~CRegisterLoggerBlock()
{
	m_pLogger->m_iPort = m_iPort;
	m_pLogger->m_bAutoIncrement = m_bAutoIncrement;
	m_pLogger->m_bBlocked = m_bBlocked;
}
