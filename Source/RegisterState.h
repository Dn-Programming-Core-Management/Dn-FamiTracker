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

#include <unordered_map>

/*!
	\brief A class which manages writes to a single APU register.
*/
class CRegisterState
{
public:
	/*!	\brief Constructor of the register state. */
	CRegisterState() : m_iValue(0), m_iWriteClock(0), m_iNewClock(0) { }

	/*!	\brief Resets the register's content. */
	void Reset() { m_iValue = m_iWriteClock = m_iNewClock = 0; }

	/*!	\brief Writes a value to the register.
		\param Val The new register value. */
	void Update(uint8_t Val) {
		if (m_iValue != Val) m_iNewClock = DECAY_RATE;
		m_iValue = Val; m_iWriteClock = DECAY_RATE;
	}

	/*!	\brief Obtains the register value.
		\return The register value. */
	uint8_t GetValue() const { return m_iValue; }

	/*!	\brief Obtains the number of ticks since the last time the register value was updated.
		\return Number of elapsed ticks. */
	unsigned int GetLastUpdatedTime() const { return DECAY_RATE - m_iWriteClock; }

	/*!	\brief Obtains the number of ticks since the last time a new register value was written.
		\return Number of elapsed ticks. */
	unsigned int GetNewValueTime() const { return DECAY_RATE - m_iNewClock; }

	/*!	\brief Steps one tick and updates the register state's time information. */
	void Step() { if (m_iWriteClock) --m_iWriteClock; if (m_iNewClock) --m_iNewClock; }

public:
	static const unsigned int DECAY_RATE = 15;

private:
	uint8_t m_iValue;
	unsigned int m_iWriteClock;
	unsigned int m_iNewClock;
};

/*!
	\brief A class which logs writes to all registers of a sound chip.
*/
class CRegisterLogger
{
public:
	friend class CRegisterLoggerBlock;

	/*!	\brief Constructor of the register logger. */
	CRegisterLogger();

	/*!	\brief Resets the values of all registers. */
	void Reset();

	/*!	\brief Adds a range of register addresses, creating a state object for each register.
		\details The added addresses are not allowed to overlap previously added addresses. Each
		contiguous address range added in this way forms a region for the address port to wrap
		around when auto-incrementing after writes.
		\param Low The smallest address value.
		\param High The highest address value.
		\return Whether the registers were successfully added. */
	bool AddRegisterRange(unsigned Low, unsigned High);

	/*!	\brief Changes the address value for all future register writes.
		\param Address The new address value.
		\return True if the register at the given address exists. */
	bool SetPort(unsigned Address);

	/*!	\brief Changes the auto-increment setting of the register logger.
		\param Whether new writes increment the address port. */
	void SetAutoincrement(bool Enable);

	/*!	\brief Writes a value to the current port.
		\param Value The new register value.
		\return Whether the write succeeded. */
	bool Write(uint8_t Value);

	/*!	\brief Obtains a register object.
		\param Address The address value of the register.
		\param The register state object, or nullptr if the given address does not exist. */
	CRegisterState *GetRegister(unsigned Address);

	/*!	\brief Steps one tick and updates the time information of all registers. */
	void Step();

protected:
	std::unordered_map<unsigned, CRegisterState> m_mRegister;
	std::unordered_map<unsigned, unsigned> m_mWarpValues;
	unsigned int m_iPort;
	bool m_bAutoIncrement;
	bool m_bBlocked;
};

/*!
	\brief A class which allows internal changes to the register state without causing external changes
	due to logging.
	\details A register logger is blocked during the lifetime of a CRegisterLoggerBlock object. The
	address port of the logger before blocking remains after the block's destruction.
*/
class CRegisterLoggerBlock
{
public:
	CRegisterLoggerBlock(CRegisterLogger *Logger);
	~CRegisterLoggerBlock();

private:
	CRegisterLogger *m_pLogger;
	unsigned int m_iPort;
	bool m_bAutoIncrement;
	bool m_bBlocked;
};
