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

#pragma once

class CSequenceManager;
class CInstrumentManager;
class CDSampleManager;
class CBookmarkManager;

class CFTMComponentInterface
{
public:
	virtual CSequenceManager *const GetSequenceManager(int InstType) const = 0;
	virtual CInstrumentManager *const GetInstrumentManager() const = 0;
	virtual CDSampleManager *const GetDSampleManager() const = 0;
	virtual CBookmarkManager *const GetBookmarkManager() const = 0;

	virtual void Modify(bool Change) = 0;
	virtual void ModifyIrreversible() = 0;
};
