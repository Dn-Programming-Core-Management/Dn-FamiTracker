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

#include "stdafx.h"
#include "FamiTracker.h"
#include "ConfigAppearance.h"
#include "Settings.h"
#include "Graphics.h"
#include <fstream>
#include <string>
#include "NumConv.h"		// // //
#include "utils/ftmath.h"
#include <algorithm>
#include <array>

const TCHAR *CConfigAppearance::COLOR_ITEMS[] = {
	_T("Background"), 
	_T("Highlighted background"),
	_T("Highlighted background 2"),
	_T("Pattern text"), 
	_T("Highlighted pattern text"),
	_T("Highlighted pattern text 2"),
	_T("Instrument column"),
	_T("Volume column"),
	_T("Effect number column"),
	_T("Selection"),
	_T("Cursor"),
	_T("Current row (normal mode)"),		// // //
	_T("Current row (edit mode)"),
	_T("Current row (playing)")
};

const char CConfigAppearance::SETTING_SEPARATOR[] = " : ";		// // // 050B
const char CConfigAppearance::HEX_PREFIX[] = "0x";		// // // 050B

// Pre-defined color schemes
const COLOR_SCHEME *CConfigAppearance::COLOR_SCHEMES[] = {
	&DEFAULT_COLOR_SCHEME,
	&MONOCHROME_COLOR_SCHEME,
	&RENOISE_COLOR_SCHEME,
	&WHITE_COLOR_SCHEME,
	&SATURDAY_COLOR_SCHEME		// // //
};

const int CConfigAppearance::NUM_COLOR_SCHEMES = sizeof(COLOR_SCHEMES) / sizeof(COLOR_SCHEME*);

const int CConfigAppearance::FONT_SIZES[] = {10, 11, 12, 14, 16, 18, 20, 22};
const int CConfigAppearance::FONT_SIZE_COUNT = sizeof(FONT_SIZES) / sizeof(int);

constexpr int
LowPercent {80},
HiPercent {125},
StepPercent {5},
nPercent {(HiPercent - LowPercent) / StepPercent + 1};

constexpr auto GetPercentages() {
	std::array<int, nPercent> percentages{};
	for (int i = 0; i < nPercent; i++) {
		percentages[i] = LowPercent + StepPercent * i;
	}
	return percentages;
}


constexpr auto FONT_PERCENTAGES = GetPercentages();

const long MIN_PERCENT = 50;
const long MAX_PERCENT = 200;
const int LEN_PERCENT = 3;	// len(200)

int CALLBACK CConfigAppearance::EnumFontFamExProc(ENUMLOGFONTEX *lpelfe, NEWTEXTMETRICEX *lpntme, DWORD FontType, LPARAM lParam)
{
	if (lpelfe->elfLogFont.lfCharSet == ANSI_CHARSET && lpelfe->elfFullName[0] != '@' &&
		strlen((char*)lpelfe->elfFullName) < LF_FACESIZE)		// // //
		reinterpret_cast<CConfigAppearance*>(lParam)->AddFontName((char*)&lpelfe->elfFullName);

	return 1;
}

// CConfigAppearance dialog

IMPLEMENT_DYNAMIC(CConfigAppearance, CPropertyPage)
CConfigAppearance::CConfigAppearance()
	: CPropertyPage(CConfigAppearance::IDD)
{
}

CConfigAppearance::~CConfigAppearance()
{
}

BEGIN_MESSAGE_MAP(CConfigAppearance, CPropertyPage)
	ON_WM_PAINT()
	
	ON_CBN_SELCHANGE(IDC_SCHEME, OnCbnSelchangeScheme)
	
	ON_CBN_SELCHANGE(IDC_COL_ITEM, OnCbnSelchangeColItem)

	ON_BN_CLICKED(IDC_PICK_COL, OnBnClickedPickCol)
	
	ON_BN_CLICKED(IDC_PATTERNCOLORS, OnBnClickedPatterncolors)

	ON_BN_CLICKED(IDC_DISPLAYFLATS, OnBnClickedDisplayFlats)

	ON_CBN_SELCHANGE(IDC_FONT, OnCbnSelchangeFont)
	
	ON_CBN_SELCHANGE(IDC_FONT_SIZE, OnCbnSelchangeFontSize)
	ON_CBN_EDITCHANGE(IDC_FONT_SIZE, OnCbnEditchangeFontSize)

	ON_CBN_SELCHANGE(IDC_FONT_PERCENT, OnCbnSelchangeFontPercent)
	ON_CBN_EDITCHANGE(IDC_FONT_PERCENT, OnCbnEditchangeFontPercent)
	
	ON_BN_CLICKED(IDC_BUTTON_APPEARANCE_SAVE, OnBnClickedButtonAppearanceSave)
	ON_BN_CLICKED(IDC_BUTTON_APPEARANCE_LOAD, OnBnClickedButtonAppearanceLoad)
END_MESSAGE_MAP()


// CConfigAppearance message handlers

void CConfigAppearance::OnPaint()
{
	CPaintDC dc(this); // device context for painting
	// Do not call CPropertyPage::OnPaint() for painting messages

	CRect Rect, ParentRect;
	GetWindowRect(ParentRect);

	CWnd *pWnd = GetDlgItem(IDC_COL_PREVIEW);
	pWnd->GetWindowRect(Rect);

	Rect.top -= ParentRect.top;
	Rect.bottom -= ParentRect.top;
	Rect.left -= ParentRect.left;
	Rect.right -= ParentRect.left;

	CBrush BrushColor;
	BrushColor.CreateSolidBrush(m_iColors[m_iSelectedItem]);

	// Solid color box
	CBrush *pOldBrush = dc.SelectObject(&BrushColor);
	dc.Rectangle(Rect);
	dc.SelectObject(pOldBrush);


	// **** Pattern row editor preview ****

	pWnd = GetDlgItem(IDC_PREVIEW);
	pWnd->GetWindowRect(Rect);

	Rect.top -= ParentRect.top;
	Rect.bottom -= ParentRect.top;// - 16;
	Rect.left -= ParentRect.left;
	Rect.right -= ParentRect.left;

	int WinHeight = Rect.bottom - Rect.top;
	int WinWidth = Rect.right - Rect.left;

	// Create font
	CFont Font, *OldFont;
	LOGFONT LogFont;

	memset(&LogFont, 0, sizeof(LOGFONT));
	strcpy_s(LogFont.lfFaceName, LF_FACESIZE, m_strFont.GetBuffer());

	const int fontSize = calculateFontSize(m_rowHeight, this->fontPercent);
	LogFont.lfHeight = -fontSize;
	LogFont.lfPitchAndFamily = VARIABLE_PITCH | FF_SWISS;

	Font.CreateFontIndirect(&LogFont);

	OldFont = dc.SelectObject(&Font);

	// Draw background
	dc.FillSolidRect(Rect, GetColor(COL_BACKGROUND));
	dc.SetBkMode(TRANSPARENT);

	COLORREF ShadedCol = DIM(GetColor(COL_PATTERN_TEXT), 50);
	COLORREF ShadedHiCol = DIM(GetColor(COL_PATTERN_TEXT_HILITE), 50);

	int iRowSize = m_rowHeight;
	int iRows = (WinHeight - 12) / iRowSize;// 12;

	COLORREF CursorCol = GetColor(COL_CURSOR);
	COLORREF CursorShadedCol = DIM(CursorCol, 50);
	COLORREF BgCol = GetColor(COL_BACKGROUND);
	COLORREF HilightBgCol = GetColor(COL_BACKGROUND_HILITE);
	COLORREF Hilight2BgCol = GetColor(COL_BACKGROUND_HILITE2);

	// TODO don't use hard-coded increments, disgusting
	// Use a CPatternEditor object to draw.
	for (int i = 0; i < iRows; ++i) {

		int OffsetTop = Rect.top + (i * iRowSize) + 6;
		int OffsetLeft = Rect.left + 9;

		if (OffsetTop > (Rect.bottom - iRowSize))
			break;

		if (i % 4 == 0) {
			if (i % 8 == 0)
				GradientBar(&dc, Rect.left, OffsetTop, Rect.right - Rect.left, iRowSize, Hilight2BgCol, BgCol);
			else
				GradientBar(&dc, Rect.left, OffsetTop, Rect.right - Rect.left, iRowSize, HilightBgCol, BgCol);

			if (i == 0) {
				dc.SetTextColor(GetColor(COL_PATTERN_TEXT_HILITE));
				GradientBar(&dc, Rect.left + 5, OffsetTop, 40, iRowSize, CursorCol, GetColor(COL_BACKGROUND));
				dc.Draw3dRect(Rect.left + 5, OffsetTop, 40, iRowSize, CursorCol, CursorShadedCol);
			}
			else
				dc.SetTextColor(ShadedHiCol);
		}
		else {
			dc.SetTextColor(ShadedCol);
		}

#define BAR(x, y) dc.FillSolidRect((x) + 3, (y) + (iRowSize / 2) + 1, 10 - 7, 1, ShadedCol)

		if (i == 0) {
			dc.TextOut(OffsetLeft, OffsetTop - 2, _T("C"));
			dc.TextOut(OffsetLeft + 12, OffsetTop - 2, _T("-"));
			dc.TextOut(OffsetLeft + 24, OffsetTop - 2, _T("4"));
		}
		else {
			BAR(OffsetLeft, OffsetTop - 2);
			BAR(OffsetLeft + 12, OffsetTop - 2);
			BAR(OffsetLeft + 24, OffsetTop - 2);
		}

		if ((i & 3) == 0) {
			dc.SetTextColor(ShadedHiCol);
		}
		else {
			dc.SetTextColor(ShadedCol);
		}

		BAR(OffsetLeft + 40, OffsetTop - 2);
		BAR(OffsetLeft + 52, OffsetTop - 2);
		BAR(OffsetLeft + 68, OffsetTop - 2);
		BAR(OffsetLeft + 84, OffsetTop - 2);
		BAR(OffsetLeft + 96, OffsetTop - 2);
		// Removed call to BAR() because it overflows available space.
	}

	dc.SelectObject(OldFont);
}

BOOL CConfigAppearance::OnInitDialog()
{
	CPropertyPage::OnInitDialog();

	const CSettings *pSettings = theApp.GetSettings();
	m_strFont = pSettings->Appearance.strFont;		// // //

	CDC *pDC = GetDC();
	if (pDC != NULL) {
		LOGFONT LogFont;
		memset(&LogFont, 0, sizeof(LOGFONT));
		LogFont.lfCharSet = DEFAULT_CHARSET;
		EnumFontFamiliesEx(pDC->m_hDC, &LogFont, (FONTENUMPROC)EnumFontFamExProc, (LPARAM)this, 0);
		ReleaseDC(pDC);
	}

	CComboBox *pFontList = static_cast<CComboBox*>(GetDlgItem(IDC_FONT));
	CComboBox *pFontSizeList = static_cast<CComboBox*>(GetDlgItem(IDC_FONT_SIZE));
	this->fontPercentList.SubclassDlgItem(IDC_FONT_PERCENT, this);
	CComboBox *pItemsBox = static_cast<CComboBox*>(GetDlgItem(IDC_COL_ITEM));
	
	// Select font.
	// (Cannot perform in AddFontName, since fonts are processed
	// non-sequentially, and assigning selection focus midway through is wrong.)
	pFontList->SelectString(0, m_strFont);

	// Color types

	for (int i = 0; i < COLOR_ITEM_COUNT; ++i) {
		pItemsBox->AddString(COLOR_ITEMS[i]);
	}

	pItemsBox->SelectString(0, COLOR_ITEMS[0]);

	m_iSelectedItem = 0;

	m_iColors[COL_BACKGROUND]			= pSettings->Appearance.iColBackground;
	m_iColors[COL_BACKGROUND_HILITE]	= pSettings->Appearance.iColBackgroundHilite;
	m_iColors[COL_BACKGROUND_HILITE2]	= pSettings->Appearance.iColBackgroundHilite2;
	m_iColors[COL_PATTERN_TEXT]			= pSettings->Appearance.iColPatternText;
	m_iColors[COL_PATTERN_TEXT_HILITE]	= pSettings->Appearance.iColPatternTextHilite;
	m_iColors[COL_PATTERN_TEXT_HILITE2]	= pSettings->Appearance.iColPatternTextHilite2;
	m_iColors[COL_PATTERN_INSTRUMENT]	= pSettings->Appearance.iColPatternInstrument;
	m_iColors[COL_PATTERN_VOLUME]		= pSettings->Appearance.iColPatternVolume;
	m_iColors[COL_PATTERN_EFF_NUM]		= pSettings->Appearance.iColPatternEffect;
	m_iColors[COL_SELECTION]			= pSettings->Appearance.iColSelection;
	m_iColors[COL_CURSOR]				= pSettings->Appearance.iColCursor;
	m_iColors[COL_CURRENT_ROW_NORMAL]	= pSettings->Appearance.iColCurrentRowNormal;		// // //
	m_iColors[COL_CURRENT_ROW_EDIT]		= pSettings->Appearance.iColCurrentRowEdit;
	m_iColors[COL_CURRENT_ROW_PLAYING]	= pSettings->Appearance.iColCurrentRowPlaying;

	m_rowHeight	= pSettings->Appearance.rowHeight;		// // //
	this->fontPercent = pSettings->Appearance.fontPercent;

	m_bPatternColors = pSettings->Appearance.bPatternColor;		// // //
	m_bDisplayFlats = pSettings->Appearance.bDisplayFlats;		// // //

	
	// Load color schemes
	pItemsBox = static_cast<CComboBox*>(GetDlgItem(IDC_SCHEME));

	for (int i = 0; i < NUM_COLOR_SCHEMES; ++i) {
		pItemsBox->AddString(COLOR_SCHEMES[i]->NAME);
	}


	// Load font sizes
	TCHAR txtBuf[16];

	for (int i = 0; i < FONT_SIZE_COUNT; ++i) {
		_itot_s(FONT_SIZES[i], txtBuf, 16, 10);
		pFontSizeList->AddString(txtBuf);
	}		// // //
	_itot_s(m_rowHeight, txtBuf, 16, 10);
	pFontSizeList->SelectString(0, txtBuf);
	pFontSizeList->SetWindowText(txtBuf);
	


	// Load font percentages
	CString percentStr;

	for (int percent : FONT_PERCENTAGES) {
		percentStr.Format(_T("%d"), percent);
		fontPercentList.AddString(percentStr);
	}
	percentStr.Format(_T("%d"), this->fontPercent);
	fontPercentList.SelectString(0, percentStr);
	fontPercentList.SetWindowText(percentStr);

	fontPercentList.LimitText(LEN_PERCENT);


	return TRUE;  // FALSE if you set the focus to a control
}


void CConfigAppearance::AddFontName(char *Name)
{
	CComboBox *pFontList = static_cast<CComboBox*>(GetDlgItem(IDC_FONT));

	pFontList->AddString(Name);
}

BOOL CConfigAppearance::OnApply()
{
	CSettings *pSettings = theApp.GetSettings();

	pSettings->Appearance.strFont	 = m_strFont;		// // //
	pSettings->Appearance.rowHeight	 = m_rowHeight;		// // //
	pSettings->Appearance.fontPercent = this->fontPercent;

	pSettings->Appearance.bPatternColor = m_bPatternColors;		// // //
	pSettings->Appearance.bDisplayFlats = m_bDisplayFlats;		// // //

	pSettings->Appearance.iColBackground			= m_iColors[COL_BACKGROUND];
	pSettings->Appearance.iColBackgroundHilite		= m_iColors[COL_BACKGROUND_HILITE];
	pSettings->Appearance.iColBackgroundHilite2		= m_iColors[COL_BACKGROUND_HILITE2];
	pSettings->Appearance.iColPatternText			= m_iColors[COL_PATTERN_TEXT];
	pSettings->Appearance.iColPatternTextHilite		= m_iColors[COL_PATTERN_TEXT_HILITE];
	pSettings->Appearance.iColPatternTextHilite2	= m_iColors[COL_PATTERN_TEXT_HILITE2];
	pSettings->Appearance.iColPatternInstrument		= m_iColors[COL_PATTERN_INSTRUMENT];
	pSettings->Appearance.iColPatternVolume			= m_iColors[COL_PATTERN_VOLUME];
	pSettings->Appearance.iColPatternEffect			= m_iColors[COL_PATTERN_EFF_NUM];
	pSettings->Appearance.iColSelection				= m_iColors[COL_SELECTION];
	pSettings->Appearance.iColCursor				= m_iColors[COL_CURSOR];
	pSettings->Appearance.iColCurrentRowNormal		= m_iColors[COL_CURRENT_ROW_NORMAL];		// // //
	pSettings->Appearance.iColCurrentRowEdit		= m_iColors[COL_CURRENT_ROW_EDIT];
	pSettings->Appearance.iColCurrentRowPlaying		= m_iColors[COL_CURRENT_ROW_PLAYING];

	theApp.ReloadColorScheme();

	return CPropertyPage::OnApply();
}

void CConfigAppearance::OnCbnSelchangeFont()
{
	CComboBox *pFontList = static_cast<CComboBox*>(GetDlgItem(IDC_FONT));
	pFontList->GetLBText(pFontList->GetCurSel(), m_strFont);
	RedrawWindow();
	SetModified();
}

BOOL CConfigAppearance::OnSetActive()
{
	CheckDlgButton(IDC_PATTERNCOLORS, m_bPatternColors);
	CheckDlgButton(IDC_DISPLAYFLATS, m_bDisplayFlats);
	return CPropertyPage::OnSetActive();
}

void CConfigAppearance::OnBnClickedPickCol()
{
	CColorDialog ColorDialog;

	ColorDialog.m_cc.Flags |= CC_FULLOPEN | CC_RGBINIT;
	ColorDialog.m_cc.rgbResult = m_iColors[m_iSelectedItem];
	ColorDialog.DoModal();

	m_iColors[m_iSelectedItem] = ColorDialog.GetColor();

	SetModified();
	RedrawWindow();
}

void CConfigAppearance::OnCbnSelchangeColItem()
{
	CComboBox *List = static_cast<CComboBox*>(GetDlgItem(IDC_COL_ITEM));
	m_iSelectedItem = List->GetCurSel();
	RedrawWindow();
}

void CConfigAppearance::OnCbnSelchangeScheme()
{
	CComboBox *pList = static_cast<CComboBox*>(GetDlgItem(IDC_SCHEME));
	
	SelectColorScheme(COLOR_SCHEMES[pList->GetCurSel()]);

	SetModified();
	RedrawWindow();
}

void CConfigAppearance::SelectColorScheme(const COLOR_SCHEME *pColorScheme)
{
	CComboBox *pFontList = static_cast<CComboBox*>(GetDlgItem(IDC_FONT));
	CComboBox *pFontSizeList = static_cast<CComboBox*>(GetDlgItem(IDC_FONT_SIZE));

	SetColor(COL_BACKGROUND, pColorScheme->BACKGROUND);
	SetColor(COL_BACKGROUND_HILITE, pColorScheme->BACKGROUND_HILITE);
	SetColor(COL_BACKGROUND_HILITE2, pColorScheme->BACKGROUND_HILITE2);
	SetColor(COL_PATTERN_TEXT, pColorScheme->TEXT_NORMAL);
	SetColor(COL_PATTERN_TEXT_HILITE, pColorScheme->TEXT_HILITE);
	SetColor(COL_PATTERN_TEXT_HILITE2, pColorScheme->TEXT_HILITE2);
	SetColor(COL_PATTERN_INSTRUMENT, pColorScheme->TEXT_INSTRUMENT);
	SetColor(COL_PATTERN_VOLUME, pColorScheme->TEXT_VOLUME);
	SetColor(COL_PATTERN_EFF_NUM, pColorScheme->TEXT_EFFECT);
	SetColor(COL_SELECTION, pColorScheme->SELECTION);
	SetColor(COL_CURSOR, pColorScheme->CURSOR);
	SetColor(COL_CURRENT_ROW_NORMAL, pColorScheme->ROW_NORMAL);		// // //
	SetColor(COL_CURRENT_ROW_EDIT, pColorScheme->ROW_EDIT);
	SetColor(COL_CURRENT_ROW_PLAYING, pColorScheme->ROW_PLAYING);

	m_strFont = pColorScheme->FONT_FACE;
	pFontList->SelectString(0, m_strFont);
	
	m_rowHeight = pColorScheme->FONT_SIZE;
	pFontSizeList->SelectString(0, MakeIntString(m_rowHeight));
	
	this->fontPercent = pColorScheme->FONT_PERCENT;
	this->fontPercentList.SelectString(0, MakeIntString(fontPercent));
}

void CConfigAppearance::SetColor(int Index, int Color)
{
	m_iColors[Index] = Color;
}

int CConfigAppearance::GetColor(int Index) const
{
	return m_iColors[Index];
}

void CConfigAppearance::OnCbnSelchangeFontSize()
{
	CString str;
	CComboBox *pFontSizeList = static_cast<CComboBox*>(GetDlgItem(IDC_FONT_SIZE));
	pFontSizeList->GetLBText(pFontSizeList->GetCurSel(), str);
	m_rowHeight = _ttoi(str);
	RedrawWindow();
	SetModified();
}

void CConfigAppearance::OnCbnEditchangeFontSize()		// // //
{
	CComboBox *pFontSizeList = static_cast<CComboBox*>(GetDlgItem(IDC_FONT_SIZE));
	CString str;
	pFontSizeList->GetWindowText(str);
	int newSize = _ttoi(str);
	if (newSize < 5 || newSize > 30) return; // arbitrary
	m_rowHeight = newSize;
	RedrawWindow();
	SetModified();
}


void CConfigAppearance::OnCbnSelchangeFontPercent() {
	CString text;
	this->fontPercentList.GetLBText(fontPercentList.GetCurSel(), text);
	onChangeFontPercent(text);
}

void CConfigAppearance::OnCbnEditchangeFontPercent() {
	CString text;
	this->fontPercentList.GetWindowText(text);
	onChangeFontPercent(text);
}

void CConfigAppearance::onChangeFontPercent(CString text) {
	auto maybe = parseLong(text);
	if (!maybe) return;

	long percent = *maybe;
	percent = std::clamp(percent, MIN_PERCENT, MAX_PERCENT);
	//if (percent < MIN_PERCENT || percent > MAX_PERCENT) return;

	this->fontPercent = percent;
	RedrawWindow();
	SetModified();
}


void CConfigAppearance::OnBnClickedPatterncolors()
{
	m_bPatternColors = IsDlgButtonChecked(IDC_PATTERNCOLORS) != 0;
	SetModified();
}

void CConfigAppearance::OnBnClickedDisplayFlats()
{
	m_bDisplayFlats = IsDlgButtonChecked(IDC_DISPLAYFLATS) != 0;
	SetModified();
}

void CConfigAppearance::OnBnClickedButtonAppearanceSave()		// // // 050B
{
	CString fileFilter = LoadDefaultFilter(IDS_FILTER_TXT, _T(".txt"));
	CFileDialog fileDialog {FALSE, NULL, _T("Theme.txt"), OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, fileFilter};
	if (fileDialog.DoModal() == IDOK)
		ExportSettings(fileDialog.GetPathName().GetBuffer());
}

void CConfigAppearance::OnBnClickedButtonAppearanceLoad()		// // // 050B
{
	CString fileFilter = LoadDefaultFilter(IDS_FILTER_TXT, _T(".txt"));
	CFileDialog fileDialog {TRUE, NULL, _T("Theme.txt"), OFN_HIDEREADONLY, fileFilter};
	if (fileDialog.DoModal() == IDOK) {
		ImportSettings(fileDialog.GetPathName().GetBuffer());
		static_cast<CComboBox*>(GetDlgItem(IDC_FONT))->SelectString(0, m_strFont);
		static_cast<CComboBox*>(GetDlgItem(IDC_FONT_SIZE))->SelectString(0, MakeIntString(m_rowHeight));
		this->fontPercentList.SelectString(0, MakeIntString(fontPercent));
		RedrawWindow();
		SetModified();
	}
}

void CConfigAppearance::ExportSettings(const char *Path) const		// // // 050B
{
	if (auto file = std::fstream {Path, std::ios_base::out}) {
		file << "# 0CC-FamiTracker appearance" << std::endl;
		for (size_t i = 0; i < std::size(m_iColors); ++i)
			file << COLOR_ITEMS[i] << SETTING_SEPARATOR << HEX_PREFIX << conv::from_uint_hex(m_iColors[i], 6) << std::endl;
		file << "Pattern colors" << SETTING_SEPARATOR << m_bPatternColors << std::endl;
		file << "Flags" << SETTING_SEPARATOR << m_bDisplayFlats << std::endl;
		file << "Font" << SETTING_SEPARATOR << m_strFont << std::endl;
		file << "Font size" << SETTING_SEPARATOR << m_rowHeight << std::endl;
		file << "Font percent" << SETTING_SEPARATOR << this->fontPercent << std::endl;
	}
}

void CConfigAppearance::ImportSettings(const char *Path)		// // // 050B
{
	std::fstream file {Path, std::ios_base::in};
	std::string Line;

	// Reset settings to default, if missing from .txt.
	this->fontPercent = _PERCENT;

	while (true) {
		std::getline(file, Line);
		if (!file) break;
		size_t Pos = Line.find(SETTING_SEPARATOR);
		if (Pos == std::string::npos)
			continue;
		auto sv = std::string_view(Line).substr(Pos + std::size(SETTING_SEPARATOR) - 1);		// // //

		for (size_t i = 0; i < sizeof(m_iColors) / sizeof(*m_iColors); ++i) {
			if (Line.find(COLOR_ITEMS[i]) == std::string::npos)
				continue;
			size_t n = sv.find(HEX_PREFIX);
			if (n == std::string_view::npos)
				continue;
			if (auto c = conv::to_uint32(sv.substr(n + std::size(HEX_PREFIX) - 1), 16))
				m_iColors[i] = *c;
		}

		if (Line.find("Pattern colors") != std::string::npos) {
			if (auto x = conv::to_uint(sv))
				m_bPatternColors = (bool)*x;
		}
		else if (Line.find("Flags") != std::string::npos) {
			if (auto x = conv::to_uint(sv))
				m_bDisplayFlats = (bool)*x;
		}
		else if (Line.find("Font size") != std::string::npos) {
			if (auto x = conv::to_uint(sv))
				m_rowHeight = *x;
		}
		else if (Line.find("Font percent")  != std::string::npos) {
			if (auto x = conv::to_uint(sv))
				this->fontPercent = *x;
		}
		else if (Line.find("Font") != std::string::npos)
			m_strFont = sv.data();
	}

	file.close();
}
