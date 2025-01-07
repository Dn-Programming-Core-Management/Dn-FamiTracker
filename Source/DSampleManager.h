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

#include "DSample.h"

class CDSampleManager
{
public:
	CDSampleManager();
	const CDSample *GetDSample(unsigned Index) const;
	bool SetDSample(unsigned Index, CDSample *pSamp);
	bool IsSampleUsed(unsigned Index) const;
	unsigned int GetSampleCount() const;
	unsigned int GetFirstFree() const;
	unsigned int GetTotalSize() const;
	static const unsigned MAX_DSAMPLES;
private:
	std::vector<std::unique_ptr<CDSample>> m_pDSample;
	unsigned int m_iTotalSize;
};
