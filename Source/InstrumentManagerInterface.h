/*
** FamiTracker - NES/Famicom sound tracker
** Copyright (C) 2005-2014  Jonathan Liss
**
** 0CC-FamiTracker is (C) 2014-2015 HertzDevil
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


class CSequence;
class CDSample;

/*
	\brief A resource interface which allows instrument resources to be obtained from themselves,
	allowing uniform access regardless of the mechanism that actually owns these resources.
*/
class CInstrumentManagerInterface
{
public:
	virtual CSequence *GetSequence(int InstType, int SeqType, int Index) const = 0;
	virtual void SetSequence(int InstType, int SeqType, int Index, CSequence *pSeq) = 0;
	virtual const CDSample *GetDSample(int Index) const = 0;
	virtual void SetDSample(int Index, CDSample *pSamp) = 0;
};
