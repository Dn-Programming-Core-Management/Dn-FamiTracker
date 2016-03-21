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


#include <string>
#include <vector>
#include <exception>
#include <memory>

/*!
	\brief An exception object raised while reading and writing FTM files.
*/
class CModuleException : std::exception
{
public:
	/*! \brief Constructor of the exception object with an empty message. */
	CModuleException();

	/*! \brief Raises the exception object.
		\details All derived classes must override this method with the exact same function body in order
		to throw polymorphically.
		\warning Visual C++ 2010 does not support the `[[noreturn]]` attribute.
	*/
	__declspec(noreturn) virtual void raise() { throw this; }; // microsoft

	/*!	\brief Obtains the error description.
		\details The description consists of zero or more lines followed by the footer specified in the
		constructor.
		\return The error string.
	*/
	const std::string get_error() const;
	/*!	\brief Appends an error string to the exception.
		\param line A string representing one line of the error description.
	*/
	void add_string(std::string line);
	/*!	\brief Appends a formatted error string to the exception.
		\param line A string representing one line of the error description.
	*/
	void add_fmt(std::string fmt, ...);
	/*!	\brief Sets the footer string of the error message.
		\param fmt The format specifier.
		\param ... Extra arguments for the formatted string.
	*/
	void set_footer(std::string footer);

private:
	std::vector<std::unique_ptr<std::string>> m_strError;
	std::unique_ptr<std::string> m_strFooter;
	static const int MAX_ERROR_STRLEN;
};