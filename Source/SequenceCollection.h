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
class CFamiTrackerDocInterface;

/*!
	\brief A collection of sequence objects.
	\details All sequences of a single instrument type and sequence type are grouped into
*/
class CSequenceCollection
{
public:
	/*! \brief Constructor of the sequence collection. */
	CSequenceCollection();
	/*! \brief Destructor of the sequence collection. */
	~CSequenceCollection();
	
	/*! \brief Obtains a modifiable sequence at a given index, creating the object if it
		does not exist.
		\param Index The index value of the sequence.
		\returns A pointer to the sequence object.
	*/
	CSequence *GetSequence(unsigned int Index);
	/*!	\brief Replaces a sequence in the collection.
		\param Index The index value of the sequence.
		\param Seq A pointer to the new sequence object.
	*/
	void SetSequence(unsigned int Index, CSequence *Seq);
	/*! \brief Obtains a constant sequence at a given index.
		\param Index The index value of the sequence.
		\returns A pointer to the sequence object, or \b nullptr if it does not exist.
	*/
	const CSequence *GetSequence(unsigned int Index) const;
	/*! \brief Obtains a free sequence.
		\returns An index to the first sequence which has no entries, or -1 if none of the
		sequences is free.
	*/
	unsigned int GetFirstFree() const;
	/*! \brief Obtains an unused sequence.
		\param pDoc A pointer to a document containing instrument definitions.
		\returns An index to the first sequence which has no entries and is not used by any
		sequence instrument, or -1 if all of the sequences are used.
	*/
	// unsigned int GetFirstUnused(CFamiTrackerDocInterface *pDoc) const;
	
	/*! \brief Removes all sequence objects contained in the collection. */
	void RemoveAll();
	/*! \brief The maximum number of sequences a collection can contain.
		\todo Replace MAX_SEQUENCES defined in FamiTrackerTypes.h with this
	*/
	static const int MAX_SEQUENCES;

private:
	/*! \brief A dynamically allocated array of pointers to sequence objects. */
	CSequence **m_pSequence;
};
