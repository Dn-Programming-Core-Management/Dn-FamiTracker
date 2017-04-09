/*
** FamiTracker - NES/Famicom sound tracker
** Copyright (C) 2005-2013  Jonathan Liss
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

#include "DSample.h"

/*
 * CDSample
 *
 * This class is used to store delta encoded samples (DPCM).
 *
 */

CDSample::CDSample(unsigned int Size) :
	m_iSampleSize(Size),
	m_pSampleData(new char[Size])
{
}

CDSample::CDSample(const CDSample &sample) :		// // //
	m_iSampleSize(sample.m_iSampleSize),
	m_pSampleData(new char[sample.m_iSampleSize]),
	m_sName(sample.m_sName)
{
	memcpy(m_pSampleData.get(), sample.m_pSampleData.get(), m_iSampleSize);
}

CDSample &CDSample::operator=(const CDSample &sample)
{
	/*
	m_iSampleSize = sample.m_iSampleSize;
	m_pSampleData.reset(new char[sample.m_iSampleSize]);
	memcpy(m_pSampleData.get(), sample.m_pSampleData.get(), m_iSampleSize);
	strncpy(m_sName.get(), sample.m_sName.get(), MAX_NAME_SIZE);
	*/
	CDSample temp(sample);
	*this = std::move(temp);
	return *this;
}

CDSample &CDSample::operator=(CDSample &&sample)		// // //
{
	m_iSampleSize = sample.m_iSampleSize;
	m_pSampleData.swap(sample.m_pSampleData);
	m_sName.swap(sample.m_sName);
	return *this;
}

void CDSample::SetData(unsigned int Size, char *pData)
{
	m_pSampleData.reset(pData);		// // //
	m_iSampleSize = Size;
}

unsigned int CDSample::GetSize() const
{
	return m_iSampleSize;
}

char *CDSample::GetData() const
{
	return m_pSampleData.get();
}

void CDSample::SetName(const char *pName)
{
	m_sName = pName;
	if (m_sName.size() >= MAX_NAME_SIZE)
		m_sName.resize(MAX_NAME_SIZE - 1); // null character
}

const char *CDSample::GetName() const
{
	return m_sName.c_str();
}
