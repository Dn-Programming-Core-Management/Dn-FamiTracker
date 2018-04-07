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
