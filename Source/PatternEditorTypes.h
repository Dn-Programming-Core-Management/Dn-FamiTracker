/*
** FamiTracker - NES/Famicom sound tracker
** Copyright (C) 2005-2014  Jonathan Liss
**
** 0CC-FamiTracker is (C) 2014-2015 HertzDevil
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

// // // Scroll modes
#define SCROLL_NONE  0x00
#define SCROLL_UP    0x01
#define SCROLL_DOWN  0x02
#define SCROLL_RIGHT 0x04
#define SCROLL_LEFT  0x08

// // // Selection scope
#define SEL_SCOPE_NONE   0x00
#define SEL_SCOPE_VROW   0x01
#define SEL_SCOPE_VFRAME 0x02
#define SEL_SCOPE_VTRACK 0x03
#define SEL_SCOPE_HCOL   0x10
#define SEL_SCOPE_HCHAN  0x20
#define SEL_SCOPE_HFRAME 0x30

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
	PASTE_DEFAULT,
	PASTE_MIX,
	PASTE_OVERWRITE,
	PASTE_INSERT,
	PASTE_OVERFLOW // forced overflow paste, used for undo
};

enum paste_pos_t : unsigned int {
	PASTE_CURSOR,
	PASTE_SELECTION,
	PASTE_FILL,
	PASTE_DRAG
};

// // // Selection condition
enum sel_condition_t {
	SEL_CLEAN,				// safe for operations
	SEL_REPEATED_ROW,		// same row included in multiple frames
	// SEL_UNKNOWN_SIZE,	// skip effect outside selection, unused
	SEL_NONTERMINAL_SKIP,	// skip effect on non-terminal row
	SEL_TERMINAL_SKIP		// skip effect on last row
};

class CPatternEditor;		// // //

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
		column_t StartColumn;	// // // Start column in first channel
		column_t EndColumn;		// // // End column in last channel
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
	CCursorPos(int Row, int Channel, cursor_column_t Column, int Frame);		// // //
	const CCursorPos& operator=(const CCursorPos &pos);
	bool operator !=(const CCursorPos &other) const;
	bool operator <(const CCursorPos &other) const;
	bool operator <=(const CCursorPos &other) const;
	bool IsValid(int RowCount, int ChannelCount) const;		// // //

public:
	int m_iFrame;		// // //
	int m_iRow;
	cursor_column_t m_iColumn;		// // //
	int m_iChannel;
};

class CPatternIterator : public CCursorPos {		// // //
public:
	CPatternIterator(const CPatternIterator &it);
	CPatternIterator(CPatternEditor *pEditor, unsigned int Track, const CCursorPos &Pos);
	CPatternIterator(const CPatternEditor *const pEditor, unsigned int Track, const CCursorPos &Pos);
	
	void Get(int Channel, stChanNote *pNote) const;
	void Set(int Channel, const stChanNote *pNote);
	void Step();
	CPatternIterator &operator+=(const int Rows);
	CPatternIterator &operator-=(const int Rows);
	CPatternIterator &operator++();
	CPatternIterator operator++(int);
	CPatternIterator &operator--();
	CPatternIterator operator--(int);
	bool operator ==(const CPatternIterator &other) const;

private:
	void Warp();

public:
	int m_iTrack;

private:
	CFamiTrackerDoc *const m_pDocument;
	const CPatternEditor *const m_pPatternEditor;
};

// Selection
class CSelection {
public:
	int  GetRowStart() const;
	int  GetRowEnd() const;
	cursor_column_t GetColStart() const;		// // //
	cursor_column_t GetColEnd() const;		// // //
	int  GetChanStart() const;
	int  GetChanEnd() const;
	int  GetFrameStart() const;		// // //
	int  GetFrameEnd() const;		// // //
	// // // gone
	bool IsSameStartPoint(const CSelection &selection) const;
	bool IsColumnSelected(column_t Column, int Channel) const;
	// // //
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