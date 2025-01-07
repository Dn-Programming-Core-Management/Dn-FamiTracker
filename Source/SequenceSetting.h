/*
** Dn-FamiTracker - NES/Famicom sound tracker
** Copyright (C) 2020-2025 D.P.C.M.
** FamiTracker Copyright (C) 2005-2020 Jonathan Liss
** 0CC-FamiTracker Copyright (C) 2014-2018 HertzDevil
**
** This program is free software: you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation, either version 3 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program. If not, see https://www.gnu.org/licenses/.
*/

#pragma once

class CSequenceSetting : public CWnd
{
	DECLARE_DYNAMIC(CSequenceSetting)
	CSequenceSetting(CWnd *pParent);
	virtual ~CSequenceSetting();
public:
	void Setup(CFont *pFont);
	void SelectSequence(CSequence *pSequence, int Type, int InstrumentType);

private:
	CWnd *m_pParent;
	CMenu m_menuPopup;
	CFont *m_pFont;
	CSequence *m_pSequence;

	static const UINT MENU_ID_BASE, MENU_ID_MAX;		// // //

	int m_iInstType;
	int m_iType;
	bool m_bMouseOver;

protected:
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnPaint();
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnMenuSettingChanged(UINT ID);		// // //
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnMouseLeave();
};
