/*
** FamiTracker - NES/Famicom sound tracker
** Copyright (C) 2005-2014  Jonathan Liss
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


// CDocumentFile, class for reading/writing document files

class CModuleException;

class CDocumentFile : public CFile
{
public:
	CDocumentFile();
	virtual ~CDocumentFile();

	bool		Finished() const;

	// Write functions
	bool		BeginDocument();
	bool		EndDocument();

	void		CreateBlock(const char *ID, int Version);
	void		WriteBlock(const char *pData, unsigned int Size);
	void		WriteBlockInt(int Value);
	void		WriteBlockChar(char Value);
	void		WriteString(CString String);
	bool		FlushBlock();

	// Read functions
	void		ValidateFile();		// // //
	unsigned int GetFileVersion() const;

	bool		ReadBlock();
	void		GetBlock(void *Buffer, int Size);
	int			GetBlockVersion() const;
	bool		BlockDone() const;
	char		*GetBlockHeaderID() const;
	int			GetBlockInt();
	char		GetBlockChar();

	int			GetBlockPos() const;
	int			GetBlockSize() const;

	CString		ReadString();

	void		RollbackPointer(int count);	// avoid this

	bool		IsFileIncomplete() const;

	// // // exception
	CModuleException GetException() const;
	void SetDefaultFooter(CModuleException &e) const;
	__declspec(noreturn) void RaiseModuleException(std::string Msg) const;

	// // // Overrides
	virtual UINT Read(void* lpBuf, UINT nCount);
	virtual void Write(const void* lpBuf, UINT nCount);

public:
	// Constants
	static const unsigned int FILE_VER;
	static const unsigned int COMPATIBLE_VER;

	static const char *FILE_HEADER_ID;
	static const char *FILE_END_ID;

	static const unsigned int MAX_BLOCK_SIZE;
	static const unsigned int BLOCK_SIZE;

private:
	template<class T> void WriteBlockData(T Value);

protected:
	void ReallocateBlock();

protected:
	unsigned int	m_iFileVersion;
	bool			m_bFileDone;
	bool			m_bIncomplete;

	char			*m_cBlockID;
	unsigned int	m_iBlockSize;
	unsigned int	m_iBlockVersion;
	char			*m_pBlockData;

	unsigned int	m_iMaxBlockSize;

	unsigned int	m_iBlockPointer;
	unsigned int	m_iPreviousPointer;		// // //
	ULONGLONG		m_iFilePosition, m_iPreviousPosition;		// // //
};
