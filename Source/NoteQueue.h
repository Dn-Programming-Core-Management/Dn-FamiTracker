/*
** FamiTracker - NES/Famicom sound tracker
** Copyright (C) 2005-2014  Jonathan Liss
**
** 0CC-FamiTracker is (C) 2014-2016 HertzDevil
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
#include <unordered_map>
#include <memory>

/*!
	\brief A queue which automatically reassigns notes in the same logical track to different
	physical channels. The same note coming from the same channel in a track may only be played on
	one physical channel.
*/
class CNoteChannelQueue
{
public:
	/*!	\brief Constructor of the note queue for a single track.
		\param Ch List of channel indices for this queue. */
	CNoteChannelQueue(std::vector<unsigned> Ch);
	
	/*!	\brief Triggers a note on a given channel.
		\param Note The note number.
		\param Channel The channel index. */
	virtual unsigned Trigger(int Note, unsigned Channel);
	/*!	\brief Releases a note on a given channel.
		\param Note The note number.
		\param Channel The channel index. */
	virtual unsigned Release(int Note, unsigned Channel);
	/*!	\brief Cuts a note on a given channel.
		\param Note The note number.
		\param Channel The channel index. */
	virtual unsigned Cut(int Note, unsigned Channel);
	/*!	\brief Stops whatever is played from a specific channel. 
		\param Channel The channel index.
		\return A vector containing the physical channel indices that have a note halted. */
	std::vector<unsigned> StopChannel(unsigned Channel);
	/*!	\brief Stops all currently playing notes. */
	void StopAll();
	
	/*!	\brief Stops accepting notes from a given channel.
		\param Channel The channel index. */
	void MuteChannel(unsigned Channel);
	/*!	\brief Resumes accepting notes from a given channel.
		\param Channel The channel index. */
	void UnmuteChannel(unsigned Channel);

private:
	enum class note_state_t;

	const int m_iChannelCount;

	std::vector<unsigned> m_iChannelMapID;
	std::vector<unsigned> m_iCurrentNote;
	std::vector<bool> m_bChannelMute;

	std::unordered_map<int, note_state_t> m_iNoteState;
	std::unordered_map<int, int> m_iNotePriority;
	std::unordered_map<int, unsigned> m_iNoteChannel;
};

/*!
	\brief The actual note queue that keeps track of multiple logical tracks.
*/
class CNoteQueue
{
public:
	/*!	\brief Constructor of the note queue for all channels. */
	CNoteQueue();

	/*!	\brief Adds physical channels as a single logical track.
		\param Ch List of channel indices for the queue. */
	void AddMap(std::vector<unsigned> Ch);
	/*!	\brief Removes all logical tracks. */
	void ClearMaps();

	unsigned Trigger(int Note, unsigned Channel);
	unsigned Release(int Note, unsigned Channel);
	unsigned Cut(int Note, unsigned Channel);
	std::vector<unsigned> StopChannel(unsigned Channel);
	void StopAll();
	
	void MuteChannel(unsigned Channel);
	void UnmuteChannel(unsigned Channel);

private:
	std::unordered_map<unsigned, std::shared_ptr<CNoteChannelQueue>> m_Part;
};
