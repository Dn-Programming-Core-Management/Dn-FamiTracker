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

#pragma once

#include <memory>		// // //

// DPCM sample class

class CDSample {
public:
	// Constructor
	CDSample(unsigned int Size = 0U);		// // //

	// Copy constructor
	CDSample(const CDSample &sample);
	CDSample &operator=(const CDSample &sample);

	// Set sample data and size, the object will own the memory area assigned
	void SetData(unsigned int Size, char *pData);

	// Get sample size
	unsigned int GetSize() const;

	// Get sample data
	char *GetData() const;

	// Set sample name
	void SetName(const char *pName);

	// Get sample name
	const char *GetName() const;

public:
	// Max size of a sample as supported by the NES, in bytes
	static const int MAX_SIZE = 0x0FF1;
	// Size of sample name
	static const int MAX_NAME_SIZE = 256;

private:
	// Sample data
	unsigned int m_iSampleSize;
	std::unique_ptr<char[]> m_pSampleData;		// // //
	std::unique_ptr<char[]> m_pName;		// // //
};
