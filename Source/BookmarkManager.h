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
