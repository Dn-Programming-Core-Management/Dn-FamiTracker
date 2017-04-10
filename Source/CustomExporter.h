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

#include "stdafx.h"
#include "FamiTrackerDoc.h"
#include "FamiTracker.h"

class CCustomExporter
{
public:
	CCustomExporter( void );
	CCustomExporter( const CCustomExporter& other );
	CCustomExporter& operator=( const CCustomExporter& other );
	~CCustomExporter( void );
	bool load( CString FileName );
	CString const& getName( void ) const;
	CString const& getExt( void ) const;
	bool Export( CFamiTrackerDocInterface const* doc, const char* fileName ) const;

private:
	CString m_name;
	CString m_ext;
	CString m_dllFilePath;
	HINSTANCE m_dllHandle;
	int *m_referenceCount;
	const char* (__cdecl *m_GetExt)( void );
	const char* (__cdecl *m_GetName)( void );
	bool (__cdecl *m_Export)( FamitrackerDocInterface const* iface, const char* fileName );
	void incReferenceCount( void );
	void decReferenceCount( void );
};
