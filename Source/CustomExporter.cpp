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

#include "CustomExporter.h"
#include "CustomExporter_C_Interface.h"

CCustomExporter::CCustomExporter( void )
: m_name( "CustomExporter" ), m_ext( ".asm" ), m_dllFilePath( "" ), m_dllHandle( NULL ), m_referenceCount( NULL ),
  m_GetName( NULL ), m_Export( NULL )
{

}

CCustomExporter::CCustomExporter( CCustomExporter const& other )
: m_name( "CustomExporter" ), m_ext( ".asm" ), m_dllFilePath( "" ), m_dllHandle( NULL ), m_referenceCount( NULL ),
  m_GetName( NULL ), m_Export( NULL )
{
	//copy everything over
	m_referenceCount = other.m_referenceCount;
	m_dllHandle = other.m_dllHandle;
	m_name = other.m_name;
	m_ext = other.m_ext;
	m_dllFilePath = other.m_dllFilePath;
	m_GetName = other.m_GetName;
	m_Export = other.m_Export;

	//increment reference count of dll being copied
	incReferenceCount();
}

CCustomExporter& CCustomExporter::operator=( CCustomExporter const& other )
{
	//guard against self assignment 
	if( this == &other ) return *this;

	//decrement reference count of currently held dll.
	decReferenceCount();
	
	//copy everything over
	m_referenceCount = other.m_referenceCount;
	m_dllHandle = other.m_dllHandle;
	m_name = other.m_name;
	m_ext = other.m_ext;
	m_dllFilePath = other.m_dllFilePath;
	m_GetName = other.m_GetName;
	m_Export = other.m_Export;

	//increment reference count of dll being copied
	incReferenceCount();

	return *this;
}

void CCustomExporter::incReferenceCount( void )
{
	if( NULL != m_referenceCount )
	{
		( *m_referenceCount)++;
	}
	else
	{
		m_referenceCount = new int( 1 );
	}
}

void CCustomExporter::decReferenceCount( void )
{
	if( NULL != m_referenceCount )
	{
		( *m_referenceCount)--;

		if( *m_referenceCount == 0 )
		{
			if( NULL != m_dllHandle )
			{
				FreeLibrary( m_dllHandle );
				m_dllHandle = NULL;
			}

			delete m_referenceCount;
			m_referenceCount = NULL;
		}
	}
}

CCustomExporter::~CCustomExporter( void )
{
	decReferenceCount();
}

CString const& CCustomExporter::getName( void ) const
{
	return m_name;
}

CString const& CCustomExporter::getExt( void ) const
{
	return m_ext;
}


bool CCustomExporter::load( CString FileName )
{
	if( NULL != m_dllHandle )
	{
		FreeLibrary( m_dllHandle );
	}

	m_dllHandle = LoadLibrary( FileName );
	m_dllFilePath = FileName;
	incReferenceCount();

	if( NULL != m_dllHandle )
	{
		m_GetExt = (const char* (__cdecl *)( void ))GetProcAddress( m_dllHandle, "GetExt" );

		if( NULL != m_GetExt )
		{
			m_ext = m_GetExt();
		}
		else
		{
			decReferenceCount();
			return false;
		}

		m_GetName = (const char* (__cdecl *)( void ))GetProcAddress( m_dllHandle, "GetName" );

		if( NULL != m_GetName )
		{
			m_name = m_GetName();
		}
		else
		{
			decReferenceCount();
			return false;
		}

		m_Export = (bool (__cdecl *)( FamitrackerDocInterface const* iface, const char* fileName ))GetProcAddress( m_dllHandle, "Export" );

		if( NULL == m_Export )
		{
			decReferenceCount();
			return false;
		}
	}

	return true;
}

bool CCustomExporter::Export( CFamiTrackerDocInterface const* doc, const char* fileName ) const
{
	if( NULL != m_Export )
	{
		FamitrackerDocInterface iface;
		SetDoc(const_cast<CFamiTrackerDocInterface*>(doc));
		GetInterface(&iface);

		return m_Export( &iface, fileName );
	}
	else
	{
		return false;
	}
}
