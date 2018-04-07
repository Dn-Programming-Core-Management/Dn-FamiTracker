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
