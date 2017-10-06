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

#include <vector>		// // //
#include <memory>		// // //
#include <map>		// // //

// Helper classes/objects for NSF compiling

//
// Chunk data classes
//

enum chunk_type_t {
	CHUNK_NONE,		// // //
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

// // //
struct stChunkLabel {
	chunk_type_t Type = CHUNK_NONE;
	unsigned Param1 = 0;
	unsigned Param2 = 0;
	unsigned Param3 = 0;
	friend constexpr bool
	operator==(const stChunkLabel &l, const stChunkLabel &r) noexcept {
		return std::tie(l.Type, l.Param1, l.Param2, l.Param3) ==
			std::tie(r.Type, r.Param1, r.Param2, r.Param3);
	}
	friend constexpr bool
	operator<(const stChunkLabel &l, const stChunkLabel &r) noexcept {
		return std::tie(l.Type, l.Param1, l.Param2, l.Param3) <
			std::tie(r.Type, r.Param1, r.Param2, r.Param3);
	}
};

class CChunkData
{
protected:		// // //
	CChunkData() = default;

public:
	virtual ~CChunkData() = default;
	virtual int GetSize() const = 0;
	virtual unsigned short GetData() const = 0;
};

class CChunkDataByte : public CChunkData
{
public:
	CChunkDataByte(unsigned char data) : CChunkData(), m_data(data) {}
	int GetSize() const override { return 1; }

	unsigned short GetData() const override { return m_data; };
	unsigned char m_data;
};

class CChunkDataWord : public CChunkData
{
public:
	CChunkDataWord(unsigned short data) : CChunkData(), m_data(data) { }
	int GetSize() const override { return 2; }

	unsigned short GetData() const override { return m_data; };
	unsigned short m_data;
};

class CChunkDataPointer : public CChunkData
{
public:
	CChunkDataPointer(const stChunkLabel &label) : CChunkData(), m_Label(label) { }		// // //
	int GetSize() const override { return 2; }
	unsigned short GetData() const override { return ref; };

	stChunkLabel m_Label;
	unsigned short ref = 0xFFFF;
};

class CChunkDataBank : public CChunkData
{
public:
	CChunkDataBank(const stChunkLabel &label, int bank) : CChunkData(), m_Label(label), m_bank(bank) { }		// // //
	int GetSize() const override { return 1; }
	unsigned short GetData() const override { return m_bank; };

	stChunkLabel m_Label;		// // // Reference to a label which belongs to the bank this data should point to
	unsigned int m_bank;
};

class CChunkDataString : public CChunkData
{
public:
	CChunkDataString(const std::vector<char> &data) : CChunkData(), m_vData(data) {}
	int GetSize() const override { return m_vData.size(); }
	unsigned short GetData() const override { return 0; };	// Invalid for this type

	std::vector<char> m_vData;
};

//
// Chunk class
//

class CChunk
{
public:
	explicit CChunk(const stChunkLabel &label);		// // //

	void			Clear();

	chunk_type_t	GetType() const;
	const stChunkLabel &GetLabel() const;		// // //
	void			SetBank(unsigned char Bank);
	unsigned char	GetBank() const;

	int				GetLength() const;
	unsigned short	GetData(int index) const;
	unsigned short	GetDataSize(int index) const;
	unsigned int	CountDataSize() const;

	void			StoreByte(unsigned char data);
	void			StoreWord(unsigned short data);
	void			StorePointer(const stChunkLabel &label);		// // //
	void			StoreBankReference(const stChunkLabel &label, int bank);		// // //
	void			StoreString(const std::vector<char> &data);

	void			ChangeByte(int index, unsigned char data);
	void			SetupBankData(int index, unsigned char bank);

	stChunkLabel	GetDataPointerTarget(int index) const;		// // //
	void			SetDataPointerTarget(int index, const stChunkLabel &label);		// // //
	
	bool			IsDataPointer(int index) const;
	bool			IsDataBank(int index) const;

	unsigned char	GetStringData(int index, int pos) const;
	const std::vector<char> &GetStringData(int index) const;

	void			AssignLabels(std::map<stChunkLabel, int> &labelMap);		// // //

private:
	template <typename T>		// // //
	T &GetChunkData(std::size_t index) const;
	template <typename T, typename... Args>
	void DoAddChunk(Args&&... args);

	std::vector<std::unique_ptr<CChunkData>> m_vChunkData;		// // // List of data stored in this chunk

	stChunkLabel m_stChunkLabel;		// // // Label of this chunk
	unsigned char m_iBank = 0;		// The bank this chunk will be stored in
	unsigned m_iTotalSize = 0;		// // // cache
};
