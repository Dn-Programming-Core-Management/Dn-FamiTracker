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

//header file for CustomExporters plugin container.
//who: Derek Andrews <derek.george.andrews@gmail.com>
//why: This class loads custom exporter plugins and allows the
//user to select an active plugin to use.

#pragma once
#include "stdafx.h"
#include "CustomExporter.h"

class CCustomExporters
{
public:
	CCustomExporters( CString PluginPath );
	~CCustomExporters( void );

	void GetNames( CStringArray& names ) const;
	void SetCurrentExporter( CString name );
	CCustomExporter& GetCurrentExporter( void ) const;

private:
	void FindCustomExporters( CString PluginPath );

	CArray< CCustomExporter, CCustomExporter& > m_customExporters;
	CCustomExporter* m_currentExporter;
	
};
