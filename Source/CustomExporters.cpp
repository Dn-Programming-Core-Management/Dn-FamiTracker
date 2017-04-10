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

#include "CustomExporters.h"

CCustomExporters::CCustomExporters( CString PluginPath )
: m_currentExporter( NULL )
{
	FindCustomExporters( PluginPath );
}

CCustomExporters::~CCustomExporters( void )
{

}

void CCustomExporters::GetNames( CStringArray& names ) const
{
	names.RemoveAll();
	for( int i = 0; i < m_customExporters.GetCount(); ++i )
	{
		names.Add( m_customExporters[ i ].getName() );
	}
}

void CCustomExporters::SetCurrentExporter( CString name )
{
	for( int i = 0; i < m_customExporters.GetCount(); ++i )
	{
		if( m_customExporters[ i ].getName() == name )
		{
			m_currentExporter = &m_customExporters[ i ];
			break;
		}
	}
}

CCustomExporter& CCustomExporters::GetCurrentExporter( void ) const
{
	return *m_currentExporter;
}

void CCustomExporters::FindCustomExporters( CString PluginPath )
{
	CFileFind finder;

	CString path = PluginPath + _T("\\*.dll");
	BOOL bWorking = finder.FindFile( path );
	while (bWorking)
	{
		bWorking = finder.FindNextFile();
		CString fileName = finder.GetFileName();
		CString filePath = finder.GetFilePath();
		
		CCustomExporter customExporter;

		if( customExporter.load( filePath ) )
		{
			m_customExporters.Add( customExporter );
		}

		//AfxMessageBox(finder.GetFileName());
	}
}
