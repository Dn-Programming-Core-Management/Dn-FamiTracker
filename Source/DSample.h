/*
** FamiTracker - NES/Famicom sound tracker
** Copyright (C) 2005-2014  Jonathan Liss
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


// DPCM sample class

class CDSample {
public:
	// Empty constructor
	CDSample();

	// Unnamed sample constructor
	CDSample(unsigned int Size, char *pData = NULL);

	// Copy constructor
	CDSample(CDSample &sample);

	// Destructor
	~CDSample();

	// Copy from existing sample
	void Copy(const CDSample *pDSample);
	
	// Allocate memory, optionally copy data
	void Allocate(unsigned int iSize, const char *pData = NULL);

	// Clear sample data
	void Clear();

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
	char		 *m_pSampleData;
	char		 m_Name[MAX_NAME_SIZE];
};
