/*
** FamiTracker - NES/Famicom sound tracker
** Copyright (C) 2005-2014  Jonathan Liss
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


#pragma once

#include <vector>
#include <memory>

class CBookmarkCollection;

/*!
	\brief A container for all bookmarks used in a document.
	\details It manages all bookmark collections, and provides methods to synchronize with the
	document's tracks.
*/
class CBookmarkManager
{
public:
	/*!	\brief Constructor of the bookmark manager. */
	CBookmarkManager(unsigned Count = 0U);
	/*!	\brief Removes all bookmarks from the collections. */
	void ClearAll();

	/*!	\brief Obtains a bookmark collection.
		\param Track The track index.
		\return Pointer to the bookmark collection.
	*/
	CBookmarkCollection *GetCollection(unsigned Track) const;
	/*!	\brief Obtains a bookmark collection, and then releases ownership of the collection.
		\details Used in importing FTMs.
		\param Track The track index.
		\return Pointer to the bookmark collection.
	*/
	CBookmarkCollection *PopCollection(unsigned Track);
	/*!	\brief Replaces a bookmark collection in the manager.
		\param Track the track index.
		\param pCol Pointer to the collection.
	*/
	void SetCollection(unsigned Track, CBookmarkCollection *const pCol);
	/*!	\brief Obtains the total number of bookmarks in all collections.
		\return The number of bookmarks.
	*/
	unsigned int GetBookmarkCount() const;

	/*!	\brief Inserts a track at a given position, creating an empty collection for the new track.
		\details The bookmark manager always manages the same number of bookmark collections; the
		last collection will be deleted.
		\param Track The track index.
	*/
	void InsertTrack(unsigned Track);
	/*!	\brief Removes a track at a given position.
		\param Track The track index.
	*/
	void RemoveTrack(unsigned Track);
	/*!	\brief Exchanges the positions of two bookmark collections.
		\param A Track index.
		\param B Track index.
	*/
	void SwapTracks(unsigned A, unsigned B);

private:
	std::vector<std::unique_ptr<CBookmarkCollection>> m_pCollection;
};
