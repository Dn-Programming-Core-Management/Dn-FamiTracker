/*
** FamiTracker - NES/Famicom sound tracker
** Copyright (C) 2005-2020 Jonathan Liss
**
** 0CC-FamiTracker is (C) 2014-2018 HertzDevil
**
** Dn-FamiTracker is (C) 2020-2024 D.P.C.M.
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


// Default font
const static TCHAR *FONT_FACE = _T("Verdana");
const static int FONT_SIZE	  = 12;
const static int FONT_RATIO = 100;

// Static colors
const struct {
	static const COLORREF CHANNEL_NORMAL	= 0x00202020;	// Normal channel name
	static const COLORREF CHANNEL_MUTED		= 0x002020E0;	// Muted channel name
	static const COLORREF FRAME_LIGHT		= 0x00FFFFFF;	// 3D frame
	static const COLORREF FRAME_DARK		= 0x00808080;	// 3D frame
} STATIC_COLOR_SCHEME;

// Blending levels
const struct {
	static const int SEPARATOR		= 75;	// Channel separators
	static const int EMPTY_BG		= 70;	// Empty background
	static const int UNFOCUSED		= 50;	// Unfocused cursor
	static const int FOCUSED		= 80;	// Focused cursor
	static const int SELECT			= 80;	// Selection box
	static const int SELECT_EDGE	= 70;	// Edge of selection box
	static const int UNUSED			= 30;	// Empty pattern
	static const int PREVIEW		= 50;	// Pattern preview
	static const int TEXT_SHADOW	= 20;
	static const int HOVER			= 80;
	static const int EDIT_MODE		= 80;
} SHADE_LEVEL;

// Custom colors
struct COLOR_SCHEME {
	const TCHAR		*NAME;
	const COLORREF	BACKGROUND;
	const COLORREF	BACKGROUND_HILITE;
	const COLORREF	BACKGROUND_HILITE2;
	const COLORREF	TEXT_NORMAL;
	const COLORREF	TEXT_HILITE;
	const COLORREF	TEXT_HILITE2;
	const COLORREF	TEXT_INSTRUMENT;
	const COLORREF	TEXT_VOLUME;
	const COLORREF	TEXT_EFFECT;
	const COLORREF	SELECTION;
	const COLORREF	CURSOR;
	const COLORREF	ROW_NORMAL;		// // //
	const COLORREF	ROW_EDIT;		// // //
	const COLORREF	ROW_PLAYING;		// // //
	const TCHAR		*FONT_FACE;
	const int		FONT_SIZE;
	const int FONT_PERCENT = 100;
};

// Default
const COLOR_SCHEME DEFAULT_COLOR_SCHEME = {
	_T("Default"),		// Name
	0x00000000,			// Background color
	0x00001010,			// Highlighted background color
	0x00002020,			// Highlighted background color 2
	0x0000FF00,			// Normal text color
	0x0000F0F0,			// Highlighted text color
	0x0060FFFF,			// Highlighted text color 2
	0x0080FF80,			// Instrument color
	0x00FF8080,			// Volume color
	0x008080FF,			// Effect color
	0x00F27D86,			// Selection color
	0x00808080,			// Cursor color
	0x00A02030,			// // // Current row
	0x00302080,			// // // Current row (edit mode)
	0x00400050,			// // // Current row (playing)
	_T("Verdana"),		// Font
	12					// Font size
};

// Monochrome
const COLOR_SCHEME MONOCHROME_COLOR_SCHEME = {
	_T("Monochrome"),	// Name
	0x00181818,			// Background color
	0x00202020,			// Highlighted background color
	0x00303030,			// Highlighted background color 2
	0x00C0C0C0,			// Normal text color
	0x00F0F0F0,			// Highlighted text color
	0x00FFFFFF,			// Highlighted text color 2
	0x0080FF80,			// Instrument color
	0x00FF8080,			// Volume color
	0x008080FF,			// Effect color
	0x00454550,			// Selection color
	0x00908080,			// Cursor color
	0x00A02030,			// // // Current row
	0x00302080,			// // // Current row (edit mode)
	0x00400050,			// // // Current row (playing)
	_T("Fixedsys"),		// Font
	12					// Font size
};

// Renoise
const COLOR_SCHEME RENOISE_COLOR_SCHEME = {
	_T("Renoise"),		// Name
	0x00131313,			// Background color
	0x00231A18,			// Highlighted background color
	0x00342b29,			// Highlighted background color 2
	0x00FBF4F0,			// Normal text color
	0x00FFD6B9,			// Highlighted text color
	0x00FFF6C9,			// Highlighted text color 2
	0x0080FF80,			// Instrument color
	0x00FF8080,			// Volume color
	0x008080FF,			// Effect color
	0x00FF8080,			// Selection color
	0x00707070,			// Cursor color
	0x00A02030,			// // // Current row
	0x00302080,			// // // Current row (edit mode)
	0x00400050,			// // // Current row (playing)
	_T("Fixedsys"),		// Font
	12					// Font size
};

// White
const COLOR_SCHEME WHITE_COLOR_SCHEME = {
	_T("White"),		// Name
	0x00FFFFFF,			// Background color
	0x00FFFFFF,			// Highlighted background color
	0x00FFF0FF,			// Highlighted background color 2
	0x00000000,			// Normal text color
	0x00FF0000,			// Highlighted text color
	0x00FF2020,			// Highlighted text color 2
	0x00000000,			// Instrument color
	0x00000000,			// Volume color
	0x00000000,			// Effect color
	0x00FF8080,			// Selection color
	0x00D0A0A0,			// Cursor color
	0x00A02030,			// // // Current row
	0x00302080,			// // // Current row (edit mode)
	0x00400050,			// // // Current row (playing)
	_T("Courier"),		// Font
	12					// Font size
};

// // // Saturday
const COLOR_SCHEME SATURDAY_COLOR_SCHEME = {
	_T("Saturday"),		// Name
	0x00080004,			// Background color
	0x0030141C,			// Highlighted background color
	0x00401828,			// Highlighted background color 2
	0x00F0C0D8,			// Normal text color
	0x0080E0F0,			// Highlighted text color
	0x00E0FFFF,			// Highlighted text color 2
	0x00F088C0,			// Instrument color
	0x00F0C0D8,			// Volume color
	0x00F088C0,			// Effect color
	0x00602040,			// Selection color
	0x00508090,			// Cursor color
	0x00D01030,			// // // Current row
	0x000030C0,			// // // Current row (edit mode)
	0x00500070,			// // // Current row (playing)
	_T("Courier"),		// Font
	11					// Font size
};


// honestly belongs in CPatternEditor. ConfigAppearance should use
// CPatternEditor to draw the preview.
// Maybe CPatternEditor and CFrameEditor should use/mixin a Grid class.

const int _PERCENT = 100;

inline int calculateFontSize(const int rowHeight, const int fontPercent) {
	return (rowHeight * fontPercent + _PERCENT / 2) / _PERCENT;
}

