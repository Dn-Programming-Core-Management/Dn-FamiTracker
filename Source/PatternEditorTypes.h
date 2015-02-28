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


// Helper types for the pattern editor

// Scroll modes
enum scroll_t {
	SCROLL_NONE, 
	SCROLL_UP, 
	SCROLL_DOWN, 
	SCROLL_RIGHT, 
	SCROLL_LEFT
};

// Column layout
enum column_t {
	COLUMN_NOTE, 
	COLUMN_INSTRUMENT, 
	COLUMN_VOLUME, 
	COLUMN_EFF1, 
	COLUMN_EFF2, 
	COLUMN_EFF3, 
	COLUMN_EFF4
};

// // // Paste modes
enum paste_mode_t : unsigned int {
	PASTE_DEFAULT	= 0x0000,
	PASTE_MIX		= 0x0001,
	PASTE_OVERWRITE	= 0x0002,
	PASTE_INSERT	= 0x0003,
	PASTE_REPLACE	= 0x0004,
	PASTE_CURSOR	= 0x0000,
	PASTE_SELECTION	= 0x0100,
	PASTE_FILL		= 0x0200
};

// Class used by clipboard
class CPatternClipData
{
public:
	CPatternClipData() : pPattern(NULL), Size(0) {
		memset(&ClipInfo, 0, sizeof(ClipInfo));
	}
	CPatternClipData(int Channels, int Rows) {
		memset(&ClipInfo, 0, sizeof(ClipInfo));
		Size = Channels * Rows;
		pPattern = new stChanNote[Size];
	}
	~CPatternClipData() {
		SAFE_RELEASE_ARRAY(pPattern);
	}

	SIZE_T GetAllocSize() const;	// Get clip data size in bytes
	void ToMem(HGLOBAL hMem);		// Copy structures to memory
	void FromMem(HGLOBAL hMem);		// Copy structures from memory
	
	stChanNote *GetPattern(int Channel, int Row);
	const stChanNote *GetPattern(int Channel, int Row) const;

private:
	// Do not make copies
	CPatternClipData(const CPatternClipData &obj) {};

public:
	struct {
		int Channels;			// Number of channels
		int Rows;				// Number of rows
		int StartColumn;		// Start column in first channel
		int EndColumn;			// End column in last channel
		struct {				// OLE drag and drop info
			int ChanOffset;
			int RowOffset;
		} OleInfo;
	} ClipInfo;

	stChanNote *pPattern;		// Pattern data
	int Size;					// Pattern data size, in rows * columns
};


// Cursor position
class CCursorPos {
public:
	CCursorPos();
	CCursorPos(int Row, int Channel, int Column);
	const CCursorPos& operator=(const CCursorPos &pos);
	bool operator !=(const CCursorPos &other) const;
	bool IsValid(int RowCount, int ChannelCount) const;

public:
	int m_iRow;
	int m_iColumn;
	int m_iChannel;
};


// Selection
class CSelection {
public:
	int  GetRowStart() const;
	int  GetRowEnd() const;
	int  GetColStart() const;
	int  GetColEnd() const;
	int  GetChanStart() const;
	int  GetChanEnd() const;
	bool IsWithin(const CCursorPos &pos) const;
	bool IsSingleChannel() const;
	bool IsSameStartPoint(const CSelection &selection) const;
	bool IsColumnSelected(int Column, int Channel) const;

	void Clear();
	void SetStart(const CCursorPos &pos);
	void SetEnd(const CCursorPos &pos);

	bool operator !=(const CSelection &other) const;

public:
	CCursorPos m_cpStart;
	CCursorPos m_cpEnd;
};
/*
// Pattern layout
class CPatternEditorLayout {
public:
	CPatternEditorLayout();

	void SetSize(int Width, int Height);
	void CalculateLayout();

private:
	int		m_iPatternWidth;				// Width of channels in pattern area
	int		m_iPatternHeight;				// Height of channels in pattern area
	int		m_iLinesVisible;				// Number of lines visible on screen (may include one incomplete line)
	int		m_iLinesFullVisible;			// Number of lines full visible on screen
	int		m_iChannelsVisible;				// Number of channels visible on screen (may include one incomplete channel)
	int		m_iChannelsFullVisible;			// Number of channels full visible on screen
	int		m_iFirstChannel;				// First drawn channel
	int		m_iRowHeight;					// Height of each row in pixels

	int		m_iChannelWidths[MAX_CHANNELS];	// Cached width in pixels of each channel
	int		m_iChannelOffsets[MAX_CHANNELS];// Cached x position of channels
	int		m_iColumns[MAX_CHANNELS];		// Cached number of columns in each channel
};

*/