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

enum color_items_t {
	COL_BACKGROUND,
	COL_BACKGROUND_HILITE,
	COL_BACKGROUND_HILITE2,
	COL_PATTERN_TEXT,
	COL_PATTERN_TEXT_HILITE,
	COL_PATTERN_TEXT_HILITE2,
	COL_PATTERN_INSTRUMENT,
	COL_PATTERN_VOLUME,
	COL_PATTERN_EFF_NUM,
	COL_SELECTION,
	COL_CURSOR,
	COL_CURRENT_ROW_NORMAL,
	COL_CURRENT_ROW_EDIT,
	COL_CURRENT_ROW_PLAYING,
	COLOR_ITEM_COUNT
};

#include "ColorScheme.h"

// CConfigAppearance dialog

class CConfigAppearance : public CPropertyPage
{
	DECLARE_DYNAMIC(CConfigAppearance)

public:
	CConfigAppearance();
	virtual ~CConfigAppearance();

	void AddFontName(char *Name);

// Dialog Data
	enum { IDD = IDD_CONFIG_APPEARANCE };

protected:
	static int CALLBACK EnumFontFamExProc(ENUMLOGFONTEX *lpelfe, NEWTEXTMETRICEX *lpntme, DWORD FontType, LPARAM lParam);

	static const TCHAR *COLOR_ITEMS[];
	static const int NUM_COLOR_SCHEMES;

	static const COLOR_SCHEME *COLOR_SCHEMES[];

	static const int FONT_SIZES[];
	static const int FONT_SIZE_COUNT;

	static const char SETTING_SEPARATOR[];		// // // 050B
	static const char HEX_PREFIX[];		// // // 050B

protected:
	void SelectColorScheme(const COLOR_SCHEME *pColorScheme);

	void SetColor(int Index, int Color);
	int GetColor(int Index) const;

	void ExportSettings(const char *Path) const;		// // // 050B
	void ImportSettings(const char *Path);		// // // 050B

protected:
	CString		m_strFont;
	
	int			m_rowHeight;
	int			fontPercent;
	CComboBox	fontPercentList;
	
	int			m_iSelectedItem;
	bool		m_bPatternColors;
	bool		m_bDisplayFlats;

	int			m_iColors[COLOR_ITEM_COUNT];

	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnPaint();
	virtual BOOL OnInitDialog();
	virtual BOOL OnApply();
	afx_msg void OnCbnSelchangeFont();
	virtual BOOL OnSetActive();
	afx_msg void OnBnClickedPickCol();
	afx_msg void OnCbnSelchangeColItem();
	afx_msg void OnCbnSelchangeScheme();
	afx_msg void OnCbnSelchangeFontSize();
	afx_msg void OnBnClickedPatterncolors();
	afx_msg void OnBnClickedDisplayFlats();
	afx_msg void OnCbnEditchangeFontSize();

	afx_msg void OnBnClickedButtonAppearanceSave();		// // // 050B
	afx_msg void OnBnClickedButtonAppearanceLoad();		// // // 050B
	
	afx_msg void OnCbnSelchangeFontPercent();
	afx_msg void OnCbnEditchangeFontPercent();
	void onChangeFontPercent(CString text);
};
