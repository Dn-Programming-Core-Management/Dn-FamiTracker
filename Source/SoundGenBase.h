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

// // // tentative base class for CSoundGen, might become CSoundDriverBase later

class stChanNote;

class CSoundGenBase {
public:
	virtual void OnTick() = 0;
	virtual void OnStepRow() = 0;

	virtual void OnPlayNote(int chan, const stChanNote &note) = 0;
	virtual void OnUpdateRow(int frame, int row) = 0;

	virtual bool IsChannelMuted(int chan) const = 0; // TODO: remove
	virtual bool ShouldStopPlayer() const = 0;

	virtual int GetArpNote(int chan) const { // TODO: remove
		return -1;
	}
};
