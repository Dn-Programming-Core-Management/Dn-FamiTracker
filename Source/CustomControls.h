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


// Various custom controls

class CMainFrame;

// The instrument list
class CInstrumentList : public CListCtrl {
	DECLARE_DYNAMIC(CInstrumentList)
protected:
	DECLARE_MESSAGE_MAP()
public:
	CInstrumentList(CMainFrame *pMainFrame);

	int GetInstrumentIndex(int Selection) const;
	int FindInstrument(int Index) const;
	void SelectInstrument(int Index);
	void SelectNextItem();
	void SelectPreviousItem();
	void InsertInstrument(int Index);
	void RemoveInstrument(int Index);
	void SetInstrumentName(int Index, TCHAR *pName);

private:
	CMainFrame *m_pMainFrame;
	CImageList *m_pDragImage;
	UINT m_nDragIndex;
	UINT m_nDropIndex;
	bool m_bDragging;

public:
	afx_msg void OnContextMenu(CWnd*, CPoint);
	afx_msg void OnLvnBeginlabeledit(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnNMClick(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnLvnKeydown(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnLvnEndlabeledit(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnLvnItemchanged(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnNMDblclk(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnLvnBegindrag(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
};

// Edit controls that can be enabled by double clicking
class CLockedEdit : public CEdit {
	DECLARE_DYNAMIC(CLockedEdit)
protected:
	DECLARE_MESSAGE_MAP()
public:
	CLockedEdit() : m_bUpdate(false), m_iValue(0) {
	};
	bool IsEditable() const;
	bool Update();
	int GetValue() const;
private:
	void OnLButtonDblClk(UINT nFlags, CPoint point);
	void OnSetFocus(CWnd* pOldWnd);
	void OnKillFocus(CWnd* pNewWnd);
private:
	bool m_bUpdate;
	int m_iValue;
public:
	virtual BOOL PreTranslateMessage(MSG* pMsg);
};


// Edit controls that displays a banner when empty
class CBannerEdit : public CEdit {
	DECLARE_DYNAMIC(CBannerEdit)
protected:
	DECLARE_MESSAGE_MAP()
public:
	CBannerEdit(UINT nID) : CEdit() { m_strText.LoadString(nID); };
protected:
	CString m_strText;
protected:
	static const TCHAR BANNER_FONT[];
	static const COLORREF BANNER_COLOR;
public:
	afx_msg void OnPaint();
	afx_msg void OnSetFocus(CWnd* pOldWnd);
	afx_msg void OnKillFocus(CWnd* pNewWnd);
};
