/*
** FamiTracker - NES/Famicom sound tracker
** Copyright (C) 2005-2020 Jonathan Liss
**
** 0CC-FamiTracker is (C) 2014-2018 HertzDevil
**
** Dn-FamiTracker is (C) 2020-2024 D.P.C.M.
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


#include "stdafx.h"
#include "NoteQueue.h"

enum class CNoteChannelQueue::note_state_t {HOLD, RELEASE};

CNoteChannelQueue::CNoteChannelQueue(std::vector<unsigned> Ch) :
	m_iChannelMapID(Ch), m_iChannelCount(static_cast<int>(Ch.size())),
	m_iCurrentNote(static_cast<unsigned>(Ch.size()), -1), m_bChannelMute(static_cast<int>(Ch.size()))
{
}

unsigned CNoteChannelQueue::Trigger(int Note, unsigned Channel)
{
	const auto AddNote = [&] (int Index) -> unsigned {
		m_iCurrentNote[Index] = Note;
		m_iNoteState[Note] = note_state_t::HOLD;
		int p = 0;
		for (const auto &x : m_iNotePriority)
			if (x.second > p) p = x.second;
		m_iNotePriority[Note] = p + 1;
		m_iNoteChannel[Note] = Channel;
		return m_iChannelMapID[Index];
	};

	if (!m_iNoteState.count(Note)) {
		int Pos = -1;
		for (int i = 0; i < m_iChannelCount; ++i)
			if (m_iChannelMapID[i] == Channel) { Pos = i; break; }
		for (int i = 0; i < m_iChannelCount; ++i) {
			if (!m_bChannelMute[Pos] && m_iCurrentNote[Pos] == -1)
				return AddNote(Pos);
			if (++Pos >= m_iChannelCount) Pos = 0;
		}
	}

	if (!m_iNoteState.count(Note)) {
		int p = 0x7FFFFFFF;
		int c = -1;
		for (int i = 0; i < m_iChannelCount; ++i) {
			if (m_bChannelMute[i]) continue;
			int n = m_iCurrentNote[i];
			if (n && m_iNoteState[n] == note_state_t::RELEASE) {
				int pnew = m_iNotePriority[n];
				if (pnew < p) {
					p = pnew; c = i;
				}
			}
		}
		if (c == -1) for (int i = 0; i < m_iChannelCount; ++i) {
			if (m_bChannelMute[i]) continue;
			int n = m_iCurrentNote[i];
			if (n && m_iNoteState[n] == note_state_t::HOLD) {
				int pnew = m_iNotePriority[n];
				if (pnew < p) {
					p = pnew; c = i;
				}
			}
		}
		if (c != -1) {
			Cut(m_iCurrentNote[c], 0);
			return AddNote(c);
		}
	}

	for (int i = 0; i < m_iChannelCount; ++i)
		if (m_iCurrentNote[i] == Note && m_iNoteChannel[m_iCurrentNote[i]] == Channel)
			return AddNote(i);

	return -1;
}

unsigned CNoteChannelQueue::Release(int Note, unsigned Channel)
{
	auto it = m_iNoteState.find(Note);
	if (it != m_iNoteState.end()) {
		if (it->second == note_state_t::HOLD)
			it->second = note_state_t::RELEASE;
		for (int i = 0; i < m_iChannelCount; ++i)
			if (m_iCurrentNote[i] == Note)
				return m_iChannelMapID[i];
	}
	return -1;
}

unsigned CNoteChannelQueue::Cut(int Note, unsigned Channel)
{
	auto it = m_iNoteState.find(Note);
	if (it != m_iNoteState.end()) {
		m_iNoteState.erase(it);
		auto pit = m_iNotePriority.find(Note);
		if (pit != m_iNotePriority.end()) m_iNotePriority.erase(pit);
		auto pit2 = m_iNoteChannel.find(Note);
		if (pit2 != m_iNoteChannel.end()) m_iNoteChannel.erase(pit2);
		for (int i = 0; i < m_iChannelCount; ++i)
			if (m_iCurrentNote[i] == Note) {
				m_iCurrentNote[i] = -1; return m_iChannelMapID[i];
			}
	}
	return -1;
}

std::vector<unsigned> CNoteChannelQueue::StopChannel(unsigned Channel)
{
	std::unordered_map<int, unsigned> m {m_iNoteChannel};
	std::vector<unsigned> v;

	for (const auto &x : m) if (x.second == Channel) {
		Cut(x.first, x.second);
		v.push_back(x.second);
	}

	return v;
}

void CNoteChannelQueue::StopAll()
{
	std::unordered_map<int, unsigned> m {m_iNoteChannel};
	for (const auto &x : m)
		Cut(x.first, x.second);
}

void CNoteChannelQueue::MuteChannel(unsigned Channel)
{
	for (int i = 0; i < m_iChannelCount; ++i)
		if (m_iChannelMapID[i] == Channel && !m_bChannelMute[i]) {
			StopChannel(i);
			m_bChannelMute[i] = true;
		}
}

void CNoteChannelQueue::UnmuteChannel(unsigned Channel)
{
	for (int i = 0; i < m_iChannelCount; ++i)
		if (m_iChannelMapID[i] == Channel && m_bChannelMute[i])
			m_bChannelMute[i] = false;
}



CNoteQueue::CNoteQueue() : m_Part()
{
}

void CNoteQueue::AddMap(std::vector<unsigned> Ch)
{
	auto ptr = std::make_shared<CNoteChannelQueue>(Ch);
	for (const auto &x : Ch)
		m_Part[x] = ptr;
}

void CNoteQueue::ClearMaps()
{
	m_Part.clear();
}

unsigned CNoteQueue::Trigger(int Note, unsigned Channel)
{
	auto it = m_Part.find(Channel);
	if (it != m_Part.end()) {
		int ret = it->second->Trigger(Note, Channel);
		if (ret != -1) return ret;
	}
	return -1;
}

unsigned CNoteQueue::Release(int Note, unsigned Channel)
{
	for (const auto &it : m_Part) {
		int ret = it.second->Release(Note, Channel);
		if (ret != -1) return ret;
	}
	return -1;
}

unsigned CNoteQueue::Cut(int Note, unsigned Channel)
{
	for (const auto &it : m_Part) {
		int ret = it.second->Cut(Note, Channel);
		if (ret != -1) return ret;
	}
	return -1;
}

std::vector<unsigned> CNoteQueue::StopChannel(unsigned Channel)
{
	std::vector<unsigned> v;
	for (const auto &it : m_Part) {
		auto ret = it.second->StopChannel(Channel);
		v.insert(v.end(), ret.begin(), ret.end());
	}
	return v;
}

void CNoteQueue::StopAll()
{
	for (const auto &it : m_Part)
		it.second->StopAll();
}

void CNoteQueue::MuteChannel(unsigned Channel)
{
	for (const auto &it : m_Part)
		it.second->MuteChannel(Channel);
}

void CNoteQueue::UnmuteChannel(unsigned Channel)
{
	for (const auto &it : m_Part)
		it.second->UnmuteChannel(Channel);
}
