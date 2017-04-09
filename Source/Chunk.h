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

// std::vector is required by this header file
#include <vector>		// // //
#include "stdafx.h"		// // //


// Helper classes/objects for NSF compiling

//
// Chunk data classes
//

class CChunkData
{
public:
	CChunkData() {}
	virtual ~CChunkData() {}
	virtual int GetSize() const = 0;
	virtual unsigned short GetData() const = 0;
};

class CChunkDataByte : public CChunkData
{
public:
	CChunkDataByte(unsigned char data) : CChunkData(), m_data(data) {}
	int GetSize() const { return 1; }
	unsigned short GetData() const { return m_data; };
	unsigned char m_data;
};

class CChunkDataWord : public CChunkData
{
public:
	CChunkDataWord(unsigned short data) : CChunkData(), m_data(data) {}
	int GetSize() const { return 2; }
	unsigned short GetData() const { return m_data; };
	unsigned short m_data;
};

class CChunkDataReference : public CChunkData
{
public:
	CChunkDataReference(CStringA refName) : CChunkData(), m_refName(refName), ref(-1) {}
	int GetSize() const { return 2; }
	unsigned short GetData() const { return ref; };
	CStringA m_refName;
	unsigned short ref;
};

class CChunkDataBank : public CChunkData
{
public:
	CChunkDataBank(CStringA bankOf, int bank) : CChunkData(), m_bankOf(bankOf), m_bank(bank) {}
	int GetSize() const { return 1; }
	unsigned short GetData() const { return m_bank; };
	CStringA m_bankOf;	// Reference to a label which belongs to the bank this data should point to
	unsigned int m_bank;
};

class CChunkDataString : public CChunkData
{
public:
	CChunkDataString(const std::vector<char> &data) : CChunkData(), m_vData(data) {}
	int GetSize() const { return m_vData.size(); }
	unsigned short GetData() const { return 0; };	// Invalid for this type
	std::vector<char> m_vData;
};


enum chunk_type_t { 
	CHUNK_HEADER,
	CHUNK_SEQUENCE, 
	CHUNK_INSTRUMENT_LIST, 
	CHUNK_INSTRUMENT, 
	CHUNK_SAMPLE_LIST, 
	CHUNK_SAMPLE_POINTERS,
	CHUNK_GROOVE_LIST,		// // //
	CHUNK_GROOVE,		// // //
	CHUNK_SONG_LIST,
	CHUNK_SONG,
	CHUNK_FRAME_LIST,
	CHUNK_FRAME,
	CHUNK_PATTERN,
	CHUNK_WAVETABLE,
	CHUNK_WAVES,
	CHUNK_CHANNEL_MAP,
	CHUNK_CHANNEL_TYPES
};

//
// Chunk class
//

class CChunk
{
public:
	CChunk(chunk_type_t Type, CStringA label);
	~CChunk();

	void			Clear();

	chunk_type_t	GetType() const;
	LPCSTR			GetLabel() const;
	void			SetBank(unsigned char Bank);
	unsigned char	GetBank() const;

	int				GetLength() const;
	unsigned short	GetData(int index) const;
	unsigned short	GetDataSize(int index) const;

	void			StoreByte(unsigned char data);
	void			StoreWord(unsigned short data);
	void			StoreReference(CStringA refName);
	void			StoreBankReference(CStringA refName, int bank);
	void			StoreString(const std::vector<char> &data);

	void			ChangeByte(int index, unsigned char data);
	void			SetupBankData(int index, unsigned char bank);

	unsigned char	GetStringData(int index, int pos) const;
	LPCSTR			GetDataRefName(int index) const;
	
	bool			IsDataReference(int index) const;
	bool			IsDataBank(int index) const;

	const std::vector<char> &GetStringData(int index) const;

	void			UpdateDataRefName(int index, CStringA &name);

	unsigned int	CountDataSize() const;

	void			AssignLabels(CMap<CStringA, LPCSTR, int, int> &labelMap);

private:
	std::vector<CChunkData*> m_vChunkData;	// List of data stored in this chunk

	CStringA m_strLabel;		// Label of this chunk
	unsigned char m_iBank;		// The bank this chunk will be stored in
	chunk_type_t m_iType;		// Chunk type
};
