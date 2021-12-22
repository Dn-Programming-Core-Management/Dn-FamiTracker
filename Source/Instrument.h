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

#pragma once
#pragma warning ( disable : 4351 )		// // // "new behaviour: elements of array [...] will be default initialized"

#include "CustomExporterInterfaces.h"

// Instrument types
enum inst_type_t {
	INST_NONE = 0,
	INST_2A03 = 1,
	INST_VRC6,
	INST_VRC7,
	INST_FDS,
	INST_N163,
	INST_S5B
};

// External classes
class CChunk;
class CFile;
class CDocumentFile;
class CInstrumentManagerInterface;		// // // break cyclic dependencies

// Instrument file load/store
class CInstrumentFile : public CFile
{
public:
	CInstrumentFile(LPCTSTR lpszFileName, UINT nOpenFlags) : CFile(lpszFileName, nOpenFlags) {};
	void WriteInt(unsigned int Value);
	void WriteChar(unsigned char Value);
	unsigned int ReadInt();
	unsigned char ReadChar();
};

// Instrument base class
class CInstrument {
public:
	CInstrument(inst_type_t type);										// // // ctor with instrument type
	virtual CInstrument* Clone() const = 0;								// // // virtual copy ctor
	virtual ~CInstrument();
	void SetName(const char *Name);
	void GetName(char *Name) const;
	const char* GetName() const;
	void RegisterManager(CInstrumentManagerInterface *pManager);		// // //
public:
	virtual inst_type_t GetType() const;								// // // Returns instrument type
	virtual void Setup() = 0;											// Setup some initial values
	virtual void Store(CDocumentFile *pDocFile) = 0;					// Saves the instrument to the module
	virtual bool Load(CDocumentFile *pDocFile) = 0;						// Loads the instrument from a module
	virtual void SaveFile(CInstrumentFile *pFile) = 0;					// Saves to an FTI file
	virtual bool LoadFile(CInstrumentFile *pFile, int iVersion) = 0;	// Loads from an FTI file
	virtual int Compile(CChunk *pChunk, int Index) = 0;					// // // Compiles the instrument for NSF generation
	virtual bool CanRelease() const = 0;
protected:
	virtual void CloneFrom(const CInstrument *pInst);					// // // virtual copying
	void InstrumentChanged() const;
public:
	static const int INST_NAME_MAX = 128;
protected:
	char m_cName[INST_NAME_MAX];
	inst_type_t m_iType;		// // //
	CInstrumentManagerInterface *m_pInstManager;		// // //
};
