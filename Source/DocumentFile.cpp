/*
** FamiTracker - NES/Famicom sound tracker
** Copyright (C) 2005-2015 Jonathan Liss
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
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
** Library General Public License for more details. To obtain a
** copy of the GNU Library General Public License, write to the Free
** Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
**
** Any permitted reproduction of these routines, in whole or in part,
** must bear this legend.
*/

#include "stdafx.h"
#include "ModuleException.h"
#include "DocumentFile.h"

//
// This class is based on CFile and has some simple extensions to create and read FTM files
//

// No unicode allowed here

// Class constants
const unsigned int CDocumentFile::FILE_VER		 = 0x0440;			// Current file version (4.40)
const unsigned int CDocumentFile::COMPATIBLE_FORWARD_VER = 0x450;	// Compatible forwards compatible file version (4.50)
const unsigned int CDocumentFile::COMPATIBLE_VER = 0x0100;			// Compatible file version (1.0)

const char *CDocumentFile::FILE_HEADER_ID = "FamiTracker Module";
const char *CDocumentFile::FILE_END_ID	  = "END";

const unsigned int CDocumentFile::MAX_BLOCK_SIZE = 0x80000;
const unsigned int CDocumentFile::BLOCK_SIZE = 0x10000;

// CDocumentFile

CDocumentFile::CDocumentFile() : 
	m_pBlockData(NULL),
	m_cBlockID(new char[16])
{
}

CDocumentFile::~CDocumentFile()
{
	SAFE_RELEASE_ARRAY(m_pBlockData);
	SAFE_RELEASE_ARRAY(m_cBlockID);
}

bool CDocumentFile::Finished() const
{
	return m_bFileDone;
}

bool CDocumentFile::BeginDocument()
{
	try {
		Write(FILE_HEADER_ID, int(strlen(FILE_HEADER_ID)));
		Write(&FILE_VER, sizeof(int));
	}
	catch (CFileException *e) {
		e->Delete();
		return false;
	}

	return true;
}

bool CDocumentFile::EndDocument()
{
	try {
		Write(FILE_END_ID, int(strlen(FILE_END_ID)));
	}
	catch (CFileException *e) {
		e->Delete();
		return false;
	}
	
	return true;
}

void CDocumentFile::CreateBlock(const char *ID, int Version)
{
	memset(m_cBlockID, 0, 16);
	strcpy(m_cBlockID, ID);

	m_iBlockPointer = 0;
	m_iBlockSize	= 0;
	m_iBlockVersion = Version & 0xFFFF;

	m_iMaxBlockSize = BLOCK_SIZE;

	m_pBlockData = new char[m_iMaxBlockSize];

	ASSERT(m_pBlockData != NULL);
}

void CDocumentFile::ReallocateBlock()
{
	int OldSize = m_iMaxBlockSize;
	m_iMaxBlockSize += BLOCK_SIZE;
	char *pData = new char[m_iMaxBlockSize];
	ASSERT(pData != NULL);
	memcpy(pData, m_pBlockData, OldSize);
	SAFE_RELEASE_ARRAY(m_pBlockData);
	m_pBlockData = pData;
}

void CDocumentFile::WriteBlock(const char *pData, unsigned int Size)
{
	ASSERT(m_pBlockData != NULL);

	unsigned int WritePtr = 0;

	// Allow block to grow in size

	unsigned Previous = m_iBlockPointer;
	while (Size > 0) {
		unsigned int WriteSize = (Size > BLOCK_SIZE) ? BLOCK_SIZE : Size;

		if ((m_iBlockPointer + WriteSize) >= m_iMaxBlockSize)
			ReallocateBlock();

		memcpy(m_pBlockData + m_iBlockPointer, pData + WritePtr, WriteSize);
		m_iBlockPointer += WriteSize;
		Size -= WriteSize;
		WritePtr += WriteSize;
	}
	m_iPreviousPointer = Previous;
}

template<class T> void CDocumentFile::WriteBlockData(T Value)
{
	WriteBlock(reinterpret_cast<const char*>(&Value), sizeof(Value));
}

void CDocumentFile::WriteBlockInt(int Value)
{
	WriteBlockData(Value);
}

void CDocumentFile::WriteBlockChar(char Value)
{
	WriteBlockData(Value);
}

void CDocumentFile::WriteString(CString String)
{
	int len = String.GetLength();

	for (int i = 0; i < len; ++i)
		WriteBlockChar(String.GetAt(i));

	// End of string
	WriteBlockChar(0);
}


// adapted from hertzdevil/25d77a
void CDocumentFile::WriteString(std::string_view sv) {
	WriteBlock((const char *)sv.data(), (int)sv.size());
	WriteBlockChar(0);
}


bool CDocumentFile::FlushBlock()
{
	if (!m_pBlockData)
		return false;

	try {
		Write(m_cBlockID, 16);
		Write(&m_iBlockVersion, sizeof(m_iBlockVersion));
		Write(&m_iBlockPointer, sizeof(m_iBlockPointer));
		Write(m_pBlockData, m_iBlockPointer);
	}
	catch (CFileException *e) {
		e->Delete();
		return false;
	}

	SAFE_RELEASE_ARRAY(m_pBlockData);

	return true;
}

void CDocumentFile::ValidateFile()
{
	// Checks if loaded file is valid

	char Buffer[256];

	// Check ident string
	Read(Buffer, int(strlen(FILE_HEADER_ID)));

	CModuleException *e = new CModuleException();		// // // blank

	if (memcmp(Buffer, FILE_HEADER_ID, strlen(FILE_HEADER_ID)) != 0)
		RaiseModuleException("File is not a FamiTracker module");

	// Read file version
	Read(Buffer, 4);
	m_iFileVersion = (Buffer[3] << 24) | (Buffer[2] << 16) | (Buffer[1] << 8) | Buffer[0];
	
	// // // Older file version
	if (GetFileVersion() < COMPATIBLE_VER) {
		e->AppendError("FamiTracker module version too old (0x%X), expected 0x%X or above", GetFileVersion(), COMPATIBLE_VER);
		e->Raise();
	}
	// // // File version is too new
	if (GetFileVersion() > COMPATIBLE_FORWARD_VER) {		// // // 050B
		e->AppendError("FamiTracker module version too new (0x%X), expected 0x%X or below", GetFileVersion(), FILE_VER);
		e->Raise();
	}

	m_bFileDone = false;
	m_bIncomplete = false;
	delete e;
}

unsigned int CDocumentFile::GetFileVersion() const
{
	return m_iFileVersion & 0xFFFF;
}

bool CDocumentFile::ReadBlock()
{
	int BytesRead;

	m_iBlockPointer = 0;
	
	memset(m_cBlockID, 0, 16);

	BytesRead = Read(m_cBlockID, 16);
	Read(&m_iBlockVersion, sizeof(int));
	Read(&m_iBlockSize, sizeof(int));

	if (m_iBlockSize > 50000000) {
		// File is probably corrupt
		memset(m_cBlockID, 0, 16);
		return true;
	}

	SAFE_RELEASE_ARRAY(m_pBlockData);
	m_pBlockData = new char[m_iBlockSize];

	Read(m_pBlockData, m_iBlockSize);

	if (strcmp(m_cBlockID, FILE_END_ID) == 0)
		m_bFileDone = true;

	if (BytesRead == 0)
		m_bFileDone = true;
/*
	if (GetPosition() == GetLength() && !m_bFileDone) {
		// Parts of file is missing
		m_bIncomplete = true;
		memset(m_cBlockID, 0, 16);
		return true;
	}
*/
	return false;
}

char *CDocumentFile::GetBlockHeaderID() const
{
	return m_cBlockID;
}

int CDocumentFile::GetBlockVersion() const
{
	return m_iBlockVersion;
}

void CDocumentFile::RollbackPointer(int count)
{
	m_iBlockPointer -= count;
	m_iPreviousPointer = m_iBlockPointer; // ?
	m_iFilePosition -= count;		// // //
	m_iPreviousPosition -= count;
}

int CDocumentFile::GetBlockInt()
{
	int Value;
	memcpy(&Value, m_pBlockData + m_iBlockPointer, sizeof(Value));
	m_iPreviousPointer = m_iBlockPointer;
	m_iBlockPointer += sizeof(Value);
	m_iPreviousPosition = m_iFilePosition;		// // //
	m_iFilePosition += sizeof(Value);
	return Value;
}

char CDocumentFile::GetBlockChar()
{
	char Value;
	memcpy(&Value, m_pBlockData + m_iBlockPointer, sizeof(Value));
	m_iPreviousPointer = m_iBlockPointer;
	m_iBlockPointer += sizeof(Value);
	m_iPreviousPosition = m_iFilePosition;		// // //
	m_iFilePosition += sizeof(Value);
	return Value;
}

CString CDocumentFile::ReadString()
{
	/*
	char str[1024], c;
	int str_ptr = 0;

	while ((c = GetBlockChar()) && (str_ptr < 1023))
		str[str_ptr++] = c;

	str[str_ptr++] = 0;

	return CString(str);
	*/

	CString str;
	char c;
	int str_ptr = 0;

	unsigned int Previous = m_iBlockPointer;
	while (str_ptr++ < 65536 && (c = GetBlockChar()))
		str.AppendChar(c);
	m_iPreviousPointer = Previous;
	
	return str;
}

void CDocumentFile::GetBlock(void *Buffer, int Size)
{
	ASSERT(Size < MAX_BLOCK_SIZE);
	ASSERT(Buffer != NULL);

	memcpy(Buffer, m_pBlockData + m_iBlockPointer, Size);
	m_iPreviousPointer = m_iBlockPointer;
	m_iBlockPointer += Size;
	m_iPreviousPosition = m_iFilePosition;		// // //
	m_iFilePosition += Size;
}

bool CDocumentFile::BlockDone() const
{
	return (m_iBlockPointer >= m_iBlockSize);
}

int CDocumentFile::GetBlockPos() const
{
	return m_iBlockPointer;
}

int CDocumentFile::GetBlockSize() const
{
	return m_iBlockSize;
}

bool CDocumentFile::IsFileIncomplete() const
{
	return m_bIncomplete;
}

CModuleException *CDocumentFile::GetException() const		// // //
{
	CModuleException *e = new CModuleException();
	SetDefaultFooter(e);
	return e;
}

void CDocumentFile::SetDefaultFooter(CModuleException *e) const		// // //
{
	char Buffer[128] = {};
	sprintf_s(Buffer, sizeof(Buffer), "At address 0x%X in %s block,\naddress 0x%llX in file",
			  m_iPreviousPointer, m_cBlockID, m_iPreviousPosition);
	std::string str(Buffer);
	e->SetFooter(str);
}

void CDocumentFile::RaiseModuleException(std::string Msg) const		// // //
{
	CModuleException *e = GetException();
	e->AppendError(Msg);
	e->Raise();
}

UINT CDocumentFile::Read(void *lpBuf, UINT nCount)		// // //
{
	m_iPreviousPosition = m_iFilePosition;
	m_iFilePosition = GetPosition();
	return CFile::Read(lpBuf, nCount);
}

void CDocumentFile::Write(const void *lpBuf, UINT nCount)		// // //
{
	m_iPreviousPosition = m_iFilePosition;
	m_iFilePosition = GetPosition();
	CFile::Write(lpBuf, nCount);
}
