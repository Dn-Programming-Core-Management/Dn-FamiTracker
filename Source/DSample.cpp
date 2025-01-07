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

#include "DSample.h"

/*
 * CDSample
 *
 * This class is used to store delta encoded samples (DPCM).
 *
 */

CDSample::CDSample(unsigned int Size) :
	m_iSampleSize(Size),
	m_pSampleData(new char[Size]),
	m_pName(new char[MAX_NAME_SIZE]())
{
}

CDSample::CDSample(const CDSample &sample) :		// // //
	m_iSampleSize(sample.m_iSampleSize),
	m_pSampleData(new char[sample.m_iSampleSize]),
	m_pName(new char[MAX_NAME_SIZE])
{
	memcpy(m_pSampleData.get(), sample.m_pSampleData.get(), m_iSampleSize);
	strncpy_s(m_pName.get(), MAX_NAME_SIZE, sample.m_pName.get(), MAX_NAME_SIZE);
}

#pragma warning ( disable : 4717 ) // "recursive on all control paths, function will cause runtime stack overflow"

CDSample &CDSample::operator=(const CDSample &sample)
{
	/*
	m_iSampleSize = sample.m_iSampleSize;
	m_pSampleData.reset(new char[sample.m_iSampleSize]);
	memcpy(m_pSampleData.get(), sample.m_pSampleData.get(), m_iSampleSize);
	strncpy(m_pName.get(), sample.m_pName.get(), MAX_NAME_SIZE);
	*/
	CDSample temp(sample);
	*this = std::move(temp);
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
	strncpy_s(m_pName.get(), MAX_NAME_SIZE, pName, MAX_NAME_SIZE);
}

const char *CDSample::GetName() const
{
	return m_pName.get();
}
