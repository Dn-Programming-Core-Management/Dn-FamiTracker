/*
** FamiTracker - NES/Famicom sound tracker
** Copyright (C) 2005-2020 Jonathan Liss
**
** 0CC-FamiTracker is (C) 2014-2018 HertzDevil
**
** Dn-FamiTracker is (C) 2020-2022 D.P.C.M.
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


class CBookmark;

/*!
	\brief A class that manages a dynamic list of bookmarks for a single track.
*/
class CBookmarkCollection
{
public:
	/*!	\brief Constructor of the bookmark collection. */
	CBookmarkCollection();
	
	/*!	\brief Gets the number of bookmarks in the collection.
		\return The number of bookmarks stored.
	*/
	unsigned GetCount() const;
	/*!	\brief Obtains a bookmark at a given position.
		\details The bookmark is owned by the collection. The bookmark shall not be deallocated via
		the returned pointer.
		\param Index Position index.
		\return Pointer to the bookmark.
	*/
	CBookmark *GetBookmark(unsigned Index) const;

	/*!	\brief Adds a bookmark to the collection.
		\param pMark Pointer to the bookmark.
		\return Whether the bookmark is successfully added.
	*/
	bool AddBookmark(CBookmark *const pMark);
	/*!	\brief Replaces a bookmark to the collection at a given position.
		\param Index Position index.
		\param pMark Pointer to the bookmark.
		\return Whether the bookmark is different from the original one at the given position.
	*/
	bool SetBookmark(unsigned Index, CBookmark *const pMark);
	/*!	\brief Inserts a bookmark to the collection at a given position.
		\param Index Position index.
		\param pMark Pointer to the bookmark.
		\return Whether the bookmark is added.
	*/
	bool InsertBookmark(unsigned Index, CBookmark *const pMark);
	/*!	\brief Removes a bookmark to the collection at a given position.
		\param Index Position index.
		\return Whether a bookmark is removed.
	*/
	bool RemoveBookmark(unsigned Index);
	/*!	\brief Removes all bookmarks in the collection.
		\return Whether any bookmark is removed.
	*/
	bool ClearBookmarks();
	/*!	\brief Swaps the positions of two bookmarks in the collection.
		\param A Position index.
		\param B Position index.
		\return Whether the position of any bookmark is changed.
	*/
	bool SwapBookmarks(unsigned A, unsigned B);
	
	/*!	\brief Inserts frames into the current track, shifting the positions of all bookmarks below.
		\details This method does not perform validation on the frame values.
		\param Frame The frame index.
		\param Count Number of frames to insert.
	*/
	void InsertFrames(unsigned Frame, unsigned Count);
	/*!	\brief Removes frames into the current track, deleting bookmarks in these frames and shifting
		the positions of all bookmarks below.
		\details This method does not perform validation on the frame values.
		\param Frame The frame index.
		\param Count Number of frames to remove.
	*/
	void RemoveFrames(unsigned Frame, unsigned Count);
	/*!	\brief Exchanges the frame values of all bookmarks in either of the given frames.
		\param A Frame index.
		\param B Frame index.
	*/
	void SwapFrames(unsigned A, unsigned B);
	
	/*!	\brief Locates a bookmark in the collection.
		\param pMark Pointer to the bookmark.
		\return The bookmark index, or -1 if the bookmark is not contained by the collection.
	*/
	int GetBookmarkIndex(const CBookmark *const pMark) const;
	/*!	\brief Locates the first bookmark below a given position.
		\details This method returns the bookmark with the least index if multiple bookmarks for the
		same location exist. It always returns a bookmark whose position is not equal to the given
		values whenever possible.
		\param Frame The frame index.
		\param Row The row index.
		\return Pointer to the first bookmark, or nullptr if no bookmarks exist.
	*/
	CBookmark *FindNext(unsigned Frame, unsigned Row) const;
	/*!	\brief Locates the first bookmark above a given position.
		\details This method returns the bookmark with the least index if multiple bookmarks for the
		same location exist. It always returns a bookmark whose position is not equal to the given
		values whenever possible.
		\param Frame The frame index.
		\param Row The row index.
		\return Pointer to the first bookmark, or nullptr if no bookmarks exist.
	*/
	CBookmark *FindPrevious(unsigned Frame, unsigned Row) const;
	/*!	\brief Locates the bookmark at a given position.
		\details This method returns the bookmark with the least index if multiple bookmarks for the
		same location exist.
		\param Frame The frame index.
		\param Row The row index.
		\return Pointer to the bookmark, or nullptr if no bookmarks exist at the position.
	*/
	CBookmark *FindAt(unsigned Frame, unsigned Row) const;
	/*!	\brief Removes all bookmarks at a given position.
		\param Frame The frame index.
		\param Row The row index.
	*/
	void RemoveAt(unsigned Frame, unsigned Row);

	/*!	\brief Sorts the contained bookmarks by their names.
		\details The method performs a stable sort; relative positions of equal bookmarks remain
		unchanged after calling this method.
		\param Desc Whether bookmarks are sorted descending (true) or ascending (false).
		\return Whether the position of any bookmark is changed.
	*/
	bool SortByName(bool Desc);
	/*!	\brief Sorts the contained bookmarks by their positions.
		\details The method performs a stable sort; relative positions of equal bookmarks remain
		unchanged after calling this method.
		\param Desc Whether bookmarks are sorted descending (true) or ascending (false).
		\return Whether the position of any bookmark is changed.
	*/
	bool SortByPosition(bool Desc);

private:
	std::vector<std::unique_ptr<CBookmark>> m_pBookmark;
};
