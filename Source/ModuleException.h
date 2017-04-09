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

#include <string>
#include <vector>
#include <exception>

#include "stdafx.h"
#include "FamiTracker.h"
#include "Settings.h"

/*!
	\brief An exception object raised while reading and writing FTM files.
*/
class CModuleException : std::exception
{
public:
	/*!	\brief Constructor of the exception object with an empty message. */
	CModuleException() = default;
	/*! \brief Virtual destructor. */
	virtual ~CModuleException() { }

	/*!	\brief Raises the exception object.
		\details All derived classes must override this method with the exact same function body in order
		to throw polymorphically. */
	[[noreturn]] virtual void Raise() { throw this; };

	/*!	\brief Obtains the error description.
		\details The description consists of zero or more lines followed by the footer specified in the
		constructor. This exception object does not use std::exception::what.
		\return The error string. */
	const std::string GetErrorString() const;
	/*!	\brief Appends a formatted error string to the exception.
		\param fmt The format specifier.
		\param ... Extra arguments for the formatted string. */
	template <typename... T>
	void AppendError(const std::string &fmt, T... args)
	{
		const size_t MAX_ERROR_STRLEN = 256;
		char buf[MAX_ERROR_STRLEN] = { };
		_sntprintf_s(buf, MAX_ERROR_STRLEN, _TRUNCATE, fmt.c_str(), args...);
		m_strError.emplace_back(buf);
	}
	/*!	\brief Sets the footer string of the error message.
		\param footer The new footer string. */
	void SetFooter(const std::string &footer);

public:
	/*!	\brief Validates a numerical value so that it lies within the interval [Min, Max].
		\details This method may throw a CModuleException object and automatically supply a suitable
		error message based on the value description. This method handles signed and unsigned types
		properly. Errors may be ignored if the current module error level is low enough.
		\param Value The value to check against.
		\param Min The minimum value permitted, inclusive.
		\param Max The maximum value permitted, inclusive.
		\param Desc A description of the checked value.
		\param fmt Print format specifier for the value type.
		\return The value argument, if the method returns.
	*/
	template <module_error_level_t l = MODULE_ERROR_DEFAULT, typename T, typename U, typename V>
	static T AssertRangeFmt(T Value, U Min, V Max, std::string Desc)
	{
		if (l > theApp.GetSettings()->Version.iErrorLevel)
			return Value;
		if (!(Value >= Min && Value <= Max)) {
			std::string msg = Desc + " out of range: expected ["
				+ std::to_string(Min) + ","
				+ std::to_string(Max) + "], got "
				+ std::to_string(Value);
			CModuleException *e = new CModuleException();
			e->AppendError(msg);
			e->Raise();
		}
		return Value;
	}

private:
	std::vector<std::string> m_strError;
	std::string m_strFooter;
};