/*
** FamiTracker - NES/Famicom sound tracker
** Copyright (C) 2005-2014  Jonathan Liss
**
** 0CC-FamiTracker is (C) 2014-2018 HertzDevil
**
** Dn-FamiTracker is (C) 2020-2021 D.P.C.M.
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
