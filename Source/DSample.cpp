/*
** FamiTracker - NES/Famicom sound tracker
** Copyright (C) 2005-2013  Jonathan Liss
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
#include "DSample.h"

/*
 * CDSample
 *
 * This class is used to store delta encoded samples (DPCM).
 *
 */

CDSample::CDSample() : 
	m_iSampleSize(0), 
	m_pSampleData(NULL) 
{
	memset(m_Name, 0, MAX_NAME_SIZE);
}

CDSample::CDSample(unsigned int Size, char *pData) : 
	m_iSampleSize(Size), 
	m_pSampleData(pData)
{
	if (m_pSampleData == NULL)
		m_pSampleData = new char[Size];

	memset(m_Name, 0, MAX_NAME_SIZE);
}

CDSample::CDSample(CDSample &sample) : 
	m_iSampleSize(sample.m_iSampleSize), 
	m_pSampleData(new char[sample.m_iSampleSize])
{
	// Should never be empty
	ASSERT(sample.m_iSampleSize != 0);
	memcpy(m_pSampleData, sample.m_pSampleData, m_iSampleSize);
	strncpy(m_Name, sample.m_Name, MAX_NAME_SIZE);
}

CDSample::~CDSample()
{
	SAFE_RELEASE_ARRAY(m_pSampleData);
}

void CDSample::Copy(const CDSample *pDSample) 
{
	ASSERT(pDSample != NULL);

	SAFE_RELEASE_ARRAY(m_pSampleData);

	m_iSampleSize = pDSample->m_iSampleSize;
	m_pSampleData = new char[m_iSampleSize];

	memcpy(m_pSampleData, pDSample->m_pSampleData, m_iSampleSize);
	strncpy(m_Name, pDSample->m_Name, MAX_NAME_SIZE);
}

void CDSample::Allocate(unsigned int iSize, const char *pData)
{
	SAFE_RELEASE_ARRAY(m_pSampleData);

	m_pSampleData = new char[iSize];
	m_iSampleSize = iSize;

	if (pData != NULL)
		memcpy(m_pSampleData, pData, iSize);
}

void CDSample::Clear()
{
	SAFE_RELEASE_ARRAY(m_pSampleData);
	m_iSampleSize = 0;
}

void CDSample::SetData(unsigned int Size, char *pData)
{
	ASSERT(pData != NULL);
	
	SAFE_RELEASE_ARRAY(m_pSampleData);		// // //

	m_pSampleData = pData;
	m_iSampleSize = Size;
}

unsigned int CDSample::GetSize() const
{
	return m_iSampleSize;
}

char *CDSample::GetData() const
{
	return m_pSampleData;
}

void CDSample::SetName(const char *pName)
{
	ASSERT(pName != NULL);
	strncpy(m_Name, pName, MAX_NAME_SIZE);
}

const char *CDSample::GetName() const
{
	return m_Name;
}
