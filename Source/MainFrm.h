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


// CMainFrame, the main window class

#include "InstrumentEditDlg.h"
#include "PerformanceDlg.h"
#include "DialogReBar.h"
#include "ControlPanelDlg.h"
#include "CustomControls.h"
#include "FindDlg.h"		// // //

enum frame_edit_pos_t { 
	FRAME_EDIT_POS_TOP, 
	FRAME_EDIT_POS_LEFT
};

enum control_panel_pos_t		// // // 050B
{
	CONTROL_PANEL_POS_TOP,
	CONTROL_PANEL_POS_LEFT,
	CONTROL_PANEL_POS_RIGHT,
};

enum {
	WM_USER_DISPLAY_MESSAGE_STRING = WM_USER,
	WM_USER_DISPLAY_MESSAGE_ID
};

class CVisualizerWnd;
class CInstrumentFileTree;
class CAction;
class CActionHandler;
class CFrameEditor;
class CGrooveDlg;		// // //
class CBookmarkDlg;
class CSwapDlg;

class CMainFrame : public CFrameWnd
{
protected: // create from serialization only
	CMainFrame();
	DECLARE_DYNCREATE(CMainFrame)

// Attributes
public:
	CFrameEditor *GetFrameEditor() const;

// Operations
public:
	void	SetStatusText(LPCTSTR Text,...);
	void	ChangeNoteState(int Note);
	
	// Indicators & controls
	void	SetIndicatorTime(int Min, int Sec, int MSec);
	void	SetIndicatorPos(int Frame, int Row);
	void	SetSongInfo(const char *pName, const char *pArtist, const char *pCopyright);
	void	SetupColors();
	void	DisplayOctave();

	void	UpdateTrackBox();
	void	UpdateControls();
	void	ResizeFrameWindow();

	void	SetHighlightRows(stHighlight Hl);		// // //

	void	UpdateMenus();
	void	UpdateMenu(CMenu *pMenu);		// // //

	// Instrument
	void	OpenInstrumentEditor();
	void	CloseInstrumentEditor();
	void	UpdateInstrumentList();
	void	SelectInstrument(int Index);
	int		GetSelectedInstrument() const;
	void	SwapInstruments(int First, int Second);

	// Track
	int		GetSelectedTrack() const;
	void	SelectTrack(unsigned int Track);

	// Octave (moved from CFamiTrackerView)
	int		GetSelectedOctave() const;		// // // 050B
	void	SelectOctave(int Octave);		// // // 050B

	// Undo/redo
	bool	AddAction(CAction *pAction);
	CAction *GetLastAction(int Filter) const;
	void	ResetUndo();

	bool	ChangeAllPatterns() const;
	
	void	CloseGrooveSettings();		// // //
	void	CloseBookmarkSettings();		// // //
	void	UpdateBookmarkList(int Pos = -1);		// // //

	void	ResetFind();		// // //

	bool	TypeInstrumentNumber(int Digit);		// // //

// Overrides
public:

// Implementation
public:
	virtual ~CMainFrame();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

private:
	bool	CreateDialogPanels();
	bool	CreateToolbars();
	bool	CreateInstrumentToolbar();
	bool	CreateVisualizerWindow();
	void	SetSpeed(int Speed);
	void	SetTempo(int Tempo);
	void	SetRowCount(int Count);
	void	SetFrameCount(int Count);

	// Instrument list operations
	void	NewInstrument(int ChipType);
	void	ClearInstrumentList();
	void	GetInstrumentName(char *pText) const;
	void	SetInstrumentName(char *pText);

	void	SetFrameEditorPosition(int Position);
	void	SetControlPanelPosition(control_panel_pos_t Position);		// // // 050B
	void	SelectInstrumentFolder();

	bool	CheckRepeat() const;

	void	CheckAudioStatus();

	void	ShowInstrumentNumberText();		// // //

private:  // control bar embedded members
	CStatusBar			m_wndStatusBar;
	CToolBar			m_wndToolBar;
	CReBar				m_wndToolBarReBar;
	CDialogReBar		m_wndOctaveBar;
	CDialogBar			m_wndControlBar;	// Parent to frame editor and settings/instrument editor
	CDialogBar			m_wndVerticalControlBar;	// Parent to large frame editor
	CDialogBar			m_wndFindControlBar;		// // // Parent to find / replace dialog
	CControlPanelDlg	m_wndDialogBar;

	CControlPanelDlg	m_wndFrameControls;		// Contains +, - and change all

	CWnd				m_wndInstToolBarWnd;
	CToolBar			m_wndInstToolBar;
	CReBarCtrl			m_wndInstToolReBar;
	CInstrumentEditDlg	m_wndInstEdit;

	CFrameEditor		*m_pFrameEditor;
	CInstrumentList		*m_pInstrumentList;
	CImageList			*m_pImageList;
	CVisualizerWnd		*m_pVisualizerWnd;
	CGrooveDlg			*m_pGrooveDlg;			// // //
	CFindDlg			*m_pFindDlg;			// // //
	CBookmarkDlg		*m_pBookmarkDlg;		// // //
	CPerformanceDlg		*m_pPerformanceDlg;		// // //

	CLockedEdit			*m_pLockedEditSpeed;
	CLockedEdit			*m_pLockedEditTempo;
	CLockedEdit			*m_pLockedEditLength;
	CLockedEdit			*m_pLockedEditFrames;
	CLockedEdit			*m_pLockedEditStep;
	CLockedEdit			*m_pLockedEditHighlight1;		// // //
	CLockedEdit			*m_pLockedEditHighlight2;		// // //
	CButton				*m_pButtonGroove;		// // //
	CButton				*m_pButtonFixTempo;		// // //

	CBannerEdit			*m_pBannerEditName;
	CBannerEdit			*m_pBannerEditArtist;
	CBannerEdit			*m_pBannerEditCopyright;

	CBitmap				m_bmToolbar;			// main toolbar
	CImageList			m_ilToolBar;

	CBitmap				m_bmInstToolbar;		// instrument toolbar
	CImageList			m_ilInstToolBar;

	CActionHandler		*m_pActionHandler;

	int					m_iFrameEditorPos;
	control_panel_pos_t	m_iControlPanelPos;		// // // 050B

	CInstrumentFileTree	*m_pInstrumentFileTree;

	// State variables
	int					m_iOctave;					// Selected octave
	int					m_iInstrument;				// Selected instrument
	int					m_iTrack;					// Selected track

	int					m_iInstNumDigit;			// // //
	int					m_iInstNumCurrent;

public:
	virtual BOOL Create(LPCTSTR lpszClassName, LPCTSTR lpszWindowName, DWORD dwStyle = WS_OVERLAPPEDWINDOW, const RECT& rect = rectDefault, CWnd* pParentWnd = NULL, LPCTSTR lpszMenuName = NULL, DWORD dwExStyle = 0, CCreateContext* pContext = NULL);

// Generated message map functions
protected:
	DECLARE_MESSAGE_MAP()
public:
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	virtual BOOL DestroyWindow();
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg LRESULT OnMenuChar(UINT nChar, UINT nFlags, CMenu* pMenu);
	afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg void OnShowWindow(BOOL bShow, UINT nStatus);
	virtual BOOL OnNotify(WPARAM wParam, LPARAM lParam, LRESULT* pResult);
	afx_msg BOOL OnCopyData(CWnd* pWnd, COPYDATASTRUCT* pCopyDataStruct);
	afx_msg void OnDeltaposTempoSpin(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnDeltaposSpeedSpin(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnDeltaposRowsSpin(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnDeltaposFrameSpin(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnDeltaposKeyStepSpin(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnInstNameChange();
	afx_msg void OnTrackerTogglePlay();
	afx_msg void OnTrackerPlay();
	afx_msg void OnTrackerPlayStart();
	afx_msg void OnTrackerPlayCursor();
	afx_msg void OnTrackerPlaypattern();
	afx_msg void OnTrackerStop();
	afx_msg void OnTrackerKillsound();
	afx_msg void OnCreateNSF();
	afx_msg void OnCreateWAV();
	afx_msg void OnNextFrame();
	afx_msg void OnPrevFrame();
	afx_msg void OnBnClickedIncFrame();
	afx_msg void OnBnClickedDecFrame();
	afx_msg void OnKeyRepeat();
	afx_msg void OnEnKeyStepChange();
	afx_msg void OnHelpPerformance();
	afx_msg void OnUpdateSBTempo(CCmdUI *pCmdUI);
	afx_msg void OnUpdateSBPosition(CCmdUI *pCmdUI);
	afx_msg void OnUpdateSBInstrument(CCmdUI *pCmdUI);
	afx_msg void OnUpdateSBOctave(CCmdUI *pCmdUI);
	afx_msg void OnUpdateSBFrequency(CCmdUI *pCmdUI);
	afx_msg void OnUpdateSBChip(CCmdUI *pCmdUI);
	afx_msg void OnUpdateKeyStepEdit(CCmdUI *pCmdUI);
	afx_msg void OnUpdateSpeedEdit(CCmdUI *pCmdUI);
	afx_msg void OnUpdateTempoEdit(CCmdUI *pCmdUI);
	afx_msg void OnUpdateRowsEdit(CCmdUI *pCmdUI);
	afx_msg void OnUpdateFramesEdit(CCmdUI *pCmdUI);
	afx_msg void OnUpdateKeyRepeat(CCmdUI *pCmdUI);
	afx_msg void OnUpdateInsertFrame(CCmdUI *pCmdUI);
	afx_msg void OnUpdateRemoveFrame(CCmdUI *pCmdUI);
	afx_msg void OnUpdateDuplicateFrame(CCmdUI *pCmdUI);
	afx_msg void OnUpdateModuleMoveframedown(CCmdUI *pCmdUI);
	afx_msg void OnUpdateModuleMoveframeup(CCmdUI *pCmdUI);
	afx_msg void OnUpdateInstrumentNew(CCmdUI *pCmdUI);
	afx_msg void OnUpdateInstrumentRemove(CCmdUI *pCmdUI);
	afx_msg void OnUpdateInstrumentClone(CCmdUI *pCmdUI);
	afx_msg void OnUpdateInstrumentDeepClone(CCmdUI *pCmdUI);
	afx_msg void OnUpdateInstrumentEdit(CCmdUI *pCmdUI);
	afx_msg void OnUpdateInstrumentLoad(CCmdUI *pCmdUI);
	afx_msg void OnUpdateInstrumentSave(CCmdUI *pCmdUI);
	afx_msg void OnUpdateNextSong(CCmdUI *pCmdUI);
	afx_msg void OnUpdatePrevSong(CCmdUI *pCmdUI);
	afx_msg void OnUpdateViewControlpanel(CCmdUI *pCmdUI);
	afx_msg void OnUpdateEditCut(CCmdUI *pCmdUI);
	afx_msg void OnUpdateEditCopy(CCmdUI *pCmdUI);
	afx_msg void OnUpdateEditPaste(CCmdUI *pCmdUI);
	afx_msg void OnUpdateEditDelete(CCmdUI *pCmdUI);
	afx_msg void OnUpdateEditEnablemidi(CCmdUI *pCmdUI);
	afx_msg void OnUpdateSelectionEnabled(CCmdUI *pCmdUI);
	afx_msg void OnTimer(UINT nIDEvent);
	afx_msg void OnFileGeneralsettings();
	afx_msg void OnEnSongNameChange();
	afx_msg void OnEnSongArtistChange();
	afx_msg void OnEnSongCopyrightChange();
	afx_msg void OnFileImportText();
	afx_msg void OnFileExportText();
	afx_msg void OnEnKillfocusTempo();
	afx_msg void OnModuleInsertFrame();
	afx_msg void OnModuleRemoveFrame();
	afx_msg void OnModuleDuplicateFrame();
	afx_msg void OnModuleDuplicateFramePatterns();
	afx_msg void OnModuleChannels();
	afx_msg void OnModuleComments();
	afx_msg void OnModuleModuleproperties();
	afx_msg void OnModuleMoveframedown();
	afx_msg void OnModuleMoveframeup();
	afx_msg void OnLoadInstrument();
	afx_msg void OnSaveInstrument();
	afx_msg void OnEditInstrument();
	afx_msg void OnAddInstrument();
	afx_msg void OnRemoveInstrument();
	afx_msg void OnCloneInstrument();
	afx_msg void OnDeepCloneInstrument();
	afx_msg void OnBnClickedEditInst();
	afx_msg void OnCbnSelchangeSong();
	afx_msg void OnCbnSelchangeOctave();
	afx_msg void OnRemoveFocus();
	afx_msg void OnNextSong();
	afx_msg void OnPrevSong();
	afx_msg void OnTrackerSwitchToInstrument();
	afx_msg void OnUpdateTrackerSwitchToInstrument(CCmdUI *pCmdUI);
	afx_msg void OnClickedFollow();
	afx_msg void OnToggleFollow();
	afx_msg void OnViewControlpanel();
	// // //
	afx_msg void OnTrackerDisplayRegisterState();
	afx_msg void OnSelectPatternEditor();
	afx_msg void OnSelectFrameEditor();
	afx_msg void OnHelpEffecttable();
	afx_msg void OnHelpFAQ();
	afx_msg void OnDestroy();
	afx_msg void OnNextInstrument();
	afx_msg void OnPrevInstrument();
	afx_msg void OnNewInstrumentMenu(NMHDR * pNotifyStruct, LRESULT * result);
	afx_msg void OnLoadInstrumentMenu(NMHDR * pNotifyStruct, LRESULT * result);
	afx_msg void OnAddInstrument2A03();
	afx_msg void OnAddInstrumentVRC6();
	afx_msg void OnAddInstrumentVRC7();
	afx_msg void OnAddInstrumentFDS();
	afx_msg void OnAddInstrumentMMC5();
	afx_msg void OnAddInstrumentN163();
	afx_msg void OnAddInstrumentS5B();
	afx_msg void OnEditUndo();
	afx_msg void OnEditRedo();
	afx_msg void OnEditCut();
	afx_msg void OnEditCopy();
	afx_msg void OnEditPaste();
	afx_msg void OnEditDelete();
	afx_msg void OnEditSelectall();
	afx_msg void OnEditExpandpatterns();
	afx_msg void OnEditShrinkpatterns();
	afx_msg void OnEditClearPatterns();
	afx_msg void OnEditRemoveUnusedInstruments();
	afx_msg void OnEditRemoveUnusedPatterns();
	afx_msg void OnEditMergeDuplicatedPatterns();
	afx_msg void OnEditEnableMIDI();
	afx_msg void OnUpdateEditUndo(CCmdUI *pCmdUI);
	afx_msg void OnUpdateEditRedo(CCmdUI *pCmdUI);
	afx_msg void OnDecayFast();
	afx_msg void OnDecaySlow();
	afx_msg void OnFrameeditorTop();
	afx_msg void OnFrameeditorLeft();
	afx_msg void OnUpdateFrameeditorTop(CCmdUI *pCmdUI);
	afx_msg void OnUpdateFrameeditorLeft(CCmdUI *pCmdUI);
	afx_msg void OnToggleSpeed();
	afx_msg LRESULT OnDisplayMessageString(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnDisplayMessageID(WPARAM wParam, LPARAM lParam);

	// // // Moved from CFamiTrackerView
	afx_msg void OnNextOctave();
	afx_msg void OnPreviousOctave();
	afx_msg void OnTrackerPal();
	afx_msg void OnTrackerNtsc();
	afx_msg void OnSpeedDefault();
	afx_msg void OnSpeedCustom();
	afx_msg void OnUpdateTrackerPal(CCmdUI *pCmdUI);
	afx_msg void OnUpdateTrackerNtsc(CCmdUI *pCmdUI);
	afx_msg void OnUpdateSpeedDefault(CCmdUI *pCmdUI);
	afx_msg void OnUpdateSpeedCustom(CCmdUI *pCmdUI);

	// // //
	afx_msg void OnToggleGroove();
	afx_msg void OnToggleFixTempo();
	afx_msg void OnClickedCompact();
	afx_msg void OnTypeInstrumentNumber();
	afx_msg void OnToggleCompact();
	afx_msg void OnUpdateHighlight1(CCmdUI *pCmdUI);
	afx_msg void OnUpdateHighlight2(CCmdUI *pCmdUI);
	afx_msg void OnDeltaposHighlightSpin1(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnDeltaposHighlightSpin2(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnFileExportRows();
	afx_msg void OnEditCopyAsText();
	afx_msg void OnEditCopyAsVolumeSequence();
	afx_msg void OnEditCopyAsPPMCK();
	afx_msg void OnEditPasteOverwrite();
	afx_msg void OnEditSelectnone();
	afx_msg void OnEditSelectrow();
	afx_msg void OnEditSelectcolumn();
	afx_msg void OnEditSelectpattern();
	afx_msg void OnEditSelectframe();
	afx_msg void OnEditSelectchannel();
	afx_msg void OnEditSelecttrack();
	afx_msg void OnEditSelectother();
	afx_msg void OnEditFindToggle();
	afx_msg void OnFindNext();
	afx_msg void OnFindPrevious();
	afx_msg void OnEditGoto();
	afx_msg void OnEditSwapChannels();
	afx_msg void OnEditStretchpatterns();
	afx_msg void OnEditTransposeCustom();
	afx_msg void OnEditRemoveUnusedSamples();
	afx_msg void OnEditPopulateUniquePatterns();
	afx_msg void OnModuleDuplicateCurrentPattern();
	afx_msg void OnModuleGrooveSettings();
	afx_msg void OnModuleBookmarkSettings();
	afx_msg void OnModuleEstimateSongLength();
	afx_msg void OnTrackerPlayMarker();		// // // 050B
	afx_msg void OnTrackerSetMarker();		// // // 050B
	afx_msg void OnTrackerDisplayAverageBPM();		// // // 050B
	afx_msg void OnTrackerDisplayChannelState();		// // // 050B
	afx_msg void OnToggleMultiplexer();
	afx_msg void OnUpdatePatternEditorSelected(CCmdUI *pCmdUI);
	afx_msg void OnUpdateEditCopySpecial(CCmdUI *pCmdUI);
	afx_msg void OnUpdateEditPasteOverwrite(CCmdUI *pCmdUI);
	afx_msg void OnUpdateSelectMultiFrame(CCmdUI *pCmdUI);
	afx_msg void OnUpdateToggleFollow(CCmdUI *pCmdUI);
	afx_msg void OnUpdateToggleCompact(CCmdUI *pCmdUI);
	afx_msg void OnUpdateToggleFixTempo(CCmdUI *pCmdUI);
	afx_msg void OnUpdateGrooveEdit(CCmdUI *pCmdUI);
	afx_msg void OnUpdateEditFindToggle(CCmdUI *pCmdUI);
	afx_msg void OnUpdateTrackerPlayMarker(CCmdUI *pCmdUI);		// // // 050B
	afx_msg void OnUpdateDisplayAverageBPM(CCmdUI *pCmdUI);		// // // 050B
	afx_msg void OnUpdateDisplayChannelState(CCmdUI *pCmdUI);		// // // 050B
	afx_msg void OnUpdateDisplayRegisterState(CCmdUI *pCmdUI);
	afx_msg void OnUpdateDecayFast(CCmdUI *pCmdUI);		// // // 050B
	afx_msg void OnUpdateDecaySlow(CCmdUI *pCmdUI);		// // // 050B
	afx_msg void OnUpdateCurrentSelectionEnabled(CCmdUI *pCmdUI);

	afx_msg void OnEasterEggKraid1();		// Easter Egg
	afx_msg void OnEasterEggKraid2();
	afx_msg void OnEasterEggKraid3();
	afx_msg void OnEasterEggKraid4();
	afx_msg void OnEasterEggKraid5();
private:
	int					m_iKraidCounter;
protected:
	virtual void OnUpdateFrameTitle(BOOL bAddToTitle);
public:
	afx_msg void OnViewToolbar();
};
