/*
** FamiTracker - NES/Famicom sound tracker
** Copyright (C) 2005-2020 Jonathan Liss
**
** 0CC-FamiTracker is (C) 2014-2018 HertzDevil
**
** Dn-FamiTracker is (C) 2020-2023 D.P.C.M.
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


class CSequence;
class CSeqInstrument;
class CDSample;

/*
	\brief A resource interface which allows instrument resources to be obtained from themselves,
	allowing uniform access regardless of the mechanism that actually owns these resources.
*/
class CInstrumentManagerInterface
{
public:
	/*!	\brief Obtains a sequence resource.
		\todo Replace this method with a const getter.
		\param InstType The instrument type, which should be a member of inst_type_t.
		\param SeqType The sequence type.
		\param Index The sequence index.
		\return Pointer to the sequence.
	*/
	virtual CSequence *GetSequence(int InstType, int SeqType, int Index) const = 0;
	/*!	\brief Puts a sequence into the resource container.
		\param InstType The instrument type, which should be a member of inst_type_t.
		\param SeqType The sequence type.
		\param Index The sample index.
		\param pSeq Pointer to the sequence.
	*/
	virtual void SetSequence(int InstType, int SeqType, int Index, CSequence *pSeq) = 0;
	/*!	\brief Adds a sequence into the resource container.
		\param InstType The instrument type, which should be a member of inst_type_t.
		\param SeqType The sequence type.
		\param pSeq Pointer to the sequence.
		\param pInst Pointer to the current instrument, required to return the correct sequence index.
		\return The index of the sequence, or -1 if it is not insered.
	*/
	virtual int AddSequence(int InstType, int SeqType, CSequence *pSeq, CSeqInstrument *pInst) = 0;
	/*!	\brief Accesses a DPCM sample resource.
		\param Index The sample index.
		\return Pointer to the sample.
	*/
	virtual const CDSample *GetDSample(int Index) const = 0;
	/*!	\brief Puts a DPCM sample into the resource container.
		\param Index The sample index.
		\param pSamp Pointer to the sample.
	*/
	virtual void SetDSample(int Index, CDSample *pSamp) = 0;
	/*!	\brief Adds a DPCM sample into the resource container.
		\param pSamp Pointer to the sample.
		\return The index of the sample, or -1 if it is not insered.
	*/
	virtual int AddDSample(CDSample *pSamp) = 0;
	/*!	\brief Notifies that an instrument has been changed.
		\details This method does nothing if the instrument manager is not linked to a document.
	*/
	virtual void InstrumentChanged() const = 0;
};
