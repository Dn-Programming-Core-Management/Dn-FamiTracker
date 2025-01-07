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

// CSampleView control

class CSampleEditorView : public CStatic {
	DECLARE_DYNAMIC(CSampleEditorView)
protected:
	DECLARE_MESSAGE_MAP()
public:
	CSampleEditorView();
	virtual ~CSampleEditorView();

	void	DrawPlayCursor(int Pos);
	void	DrawStartCursor();
	void	ExpandSample(CDSample *pSample, int Start);
	void	UpdateInfo();
	void	OnHome();
	void	OnEnd();
	void	OnRight();
	void	OnLeft();

	int		GetStartOffset() const { return m_iStartCursor; };
	int		GetSelStart() const { return (m_iSelStart < m_iSelEnd) ? m_iSelStart : m_iSelEnd; };
	int		GetSelEnd() const { return (m_iSelStart > m_iSelEnd) ? m_iSelStart : m_iSelEnd; };
	int		GetBlock(int Pixel) const;
	int		GetPixel(int Block) const;

	bool	HasSelection() const;

	void	SetZoom(float Factor);
	float	GetZoom() const;		// // //
	float	GetMaxZoomFactor() const;

private:
	void	SetScroll(UINT nPos);

private:
	static const float MAX_FACTOR, MIN_FACTOR;		// // //

	int *m_pSamples;

	int m_iSize;
	int m_iBlockSize;
	int m_iSelStart;
	int m_iSelEnd;
	int m_iStartCursor;
	double m_dSampleStep;
	bool m_bClicked;

	int m_iViewStart;
	int m_iViewEnd;
	int m_iScrollMax;
	
	float m_fZoom;

	CRect	m_clientRect;
	CDC		m_dcCopy;
	CBitmap m_bmpCopy;

	CPen *m_pSolidPen;
	CPen *m_pDashedPen;
	CPen *m_pGrayDashedPen;
	CPen *m_pDarkGrayDashedPen;

	CScrollBar *m_pScrollBar;

protected:
	afx_msg void OnPaint();
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnContextMenu(CWnd* /*pWnd*/, CPoint /*point*/);
	virtual void PreSubclassWindow();
public:
	afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
};
