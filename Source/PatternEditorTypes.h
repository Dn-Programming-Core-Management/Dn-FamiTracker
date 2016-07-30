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

#include <utility>

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

// Cursor columns
enum cursor_column_t : unsigned int {		// // // moved from FamiTrackerDoc.h
	C_NOTE,
	C_INSTRUMENT1,
	C_INSTRUMENT2,
	C_VOLUME,
	C_EFF1_NUM,
	C_EFF1_PARAM1,
	C_EFF1_PARAM2,
	C_EFF2_NUM,
	C_EFF2_PARAM1,
	C_EFF2_PARAM2,
	C_EFF3_NUM,
	C_EFF3_PARAM1,
	C_EFF3_PARAM2,
	C_EFF4_NUM,
	C_EFF4_PARAM1,
	C_EFF4_PARAM2
};

// Column layout
enum column_t : unsigned int {
	COLUMN_NOTE,
	COLUMN_INSTRUMENT,
	COLUMN_VOLUME,
	COLUMN_EFF1,
	COLUMN_EFF2,
	COLUMN_EFF3,
	COLUMN_EFF4
};
const unsigned int COLUMNS = 7;		// // // moved from FamiTrackerDoc.h

// // // moved from PatternEditor.h

inline column_t GetSelectColumn(cursor_column_t Column)
{
	// Return first column for a specific column field
	static const column_t COLUMNS[] = {
		COLUMN_NOTE, 
		COLUMN_INSTRUMENT, COLUMN_INSTRUMENT,
		COLUMN_VOLUME,
		COLUMN_EFF1, COLUMN_EFF1, COLUMN_EFF1,
		COLUMN_EFF2, COLUMN_EFF2, COLUMN_EFF2,
		COLUMN_EFF3, COLUMN_EFF3, COLUMN_EFF3,
		COLUMN_EFF4, COLUMN_EFF4, COLUMN_EFF4
	};

	ASSERT(Column >= 0 && Column < sizeof(COLUMNS));
	return COLUMNS[Column];
}

inline cursor_column_t GetCursorStartColumn(column_t Column)
{
	static const cursor_column_t COL_START[] = {
		C_NOTE, C_INSTRUMENT1, C_VOLUME, C_EFF1_NUM, C_EFF2_NUM, C_EFF3_NUM, C_EFF4_NUM
	};

	ASSERT(Column >= 0 && Column < COLUMNS);		// // //
	return COL_START[Column];
}

inline cursor_column_t GetCursorEndColumn(column_t Column)
{
	static const cursor_column_t COL_END[] = {
		C_NOTE, C_INSTRUMENT2, C_VOLUME, C_EFF1_PARAM2, C_EFF2_PARAM2, C_EFF3_PARAM2, C_EFF4_PARAM2
	};

	ASSERT(Column >= 0 && Column < COLUMNS);		// // //
	return COL_END[Column];
}

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

class CFamiTrackerDoc;		// // //
class stChanNote;

// Class used by clipboard
class CPatternClipData
{
public:
	CPatternClipData();
	CPatternClipData(int Channels, int Rows);
	~CPatternClipData();

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

	stChanNote *pPattern = nullptr;	// Pattern data
	int Size = 0;					// Pattern data size, in rows * columns
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

	bool IsSameStartPoint(const CSelection &selection) const;
	bool IsColumnSelected(column_t Column, int Channel) const;

	void Normalize(CCursorPos &Begin, CCursorPos &End) const;		// // //
	CSelection GetNormalized() const;		// // //
public:
	CCursorPos m_cpStart;
	CCursorPos m_cpEnd;
};

class CPatternIterator : public CCursorPos {		// // //
public:
	CPatternIterator(const CPatternIterator &it);
	CPatternIterator(CFamiTrackerDoc *const pDoc, int Track, const CCursorPos &Pos);
	CPatternIterator(const CFamiTrackerDoc *const pDoc, int Track, const CCursorPos &Pos);

	static std::pair<CPatternIterator, CPatternIterator> FromCursor(const CCursorPos &Pos, CFamiTrackerDoc *const pDoc, int Track);
	static std::pair<CPatternIterator, CPatternIterator> FromSelection(const CSelection &Sel, CFamiTrackerDoc *const pDoc, int Track);
	
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

protected:
	CFamiTrackerDoc *m_pDocument;
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