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


// CFamiTrackerView, the document view class

#include <afxmt.h>	// Include synchronization objects
#include <unordered_map>		// // //

#include "PatternEditorTypes.h"		// // //
#include "FamiTrackerViewMessage.h"		// // //

// External classes
class CFamiTrackerDoc;
class CPatternEditor;
class CFrameEditor;
class CAction;

// TODO move general tracker state variables to the mainframe instead of the view, such as selected octave, instrument etc

class CFamiTrackerView : public CView
{
protected: // create from serialization only
	CFamiTrackerView();
	DECLARE_DYNCREATE(CFamiTrackerView)

public:
	static CFamiTrackerView *GetView();

// Attributes
public:
	CFamiTrackerDoc* GetDocument() const;

//
// Public functions
//
public:

	// Instruments
	bool		 SwitchToInstrument() const { return m_bSwitchToInstrument; };
	void		 SwitchToInstrument(bool Switch) { m_bSwitchToInstrument = Switch; };

	// Scrolling/viewing no-editing functions
	void		 MoveCursorNextChannel();
	void		 MoveCursorPrevChannel();

	void		 SelectNextFrame();
	void		 SelectPrevFrame();
	void		 SelectFirstFrame();
	void		 SelectLastFrame();
	void		 SelectFrame(unsigned int Frame);
	void		 SelectRow(unsigned int Row);		// // //
	void		 SelectChannel(unsigned int Channel);
	 
	unsigned int GetSelectedFrame() const;
	unsigned int GetSelectedChannel() const;
	unsigned int GetSelectedRow() const; 

	void		 SetFollowMode(bool Mode);
	bool		 GetFollowMode() const;
	void		 SetCompactMode(bool Mode);		// // //
	int			 GetSelectedChipType() const;
	void		 SetOctave(unsigned int iOctave);
	unsigned int GetOctave() const { return m_iOctave; };
	bool		 GetEditMode() const { return m_bEditEnable; };
	void		 SetStepping(int Step);
	unsigned int GetStepping() const { return m_iInsertKeyStepping; };
	paste_pos_t  GetPastePos() const { return m_iPastePos; };		// // //

	// Player callback (TODO move to new interface)
	void		 PlayerTick();
	bool		 PlayerGetNote(int Track, int Frame, int Channel, int Row, stChanNote &NoteData);
	void		 PlayerPlayNote(int Channel, stChanNote *pNote);

	void		 MakeSilent();
	void		 RegisterKeyState(int Channel, int Note);

	// Note preview
	bool		 PreviewNote(unsigned char Key);
	void		 PreviewRelease(unsigned char Key);

	// Mute methods
	void		 SoloChannel(unsigned int Channel);
	void		 ToggleChannel(unsigned int Channel);
	void		 SoloChip(unsigned int Channel);		// // //
	void		 ToggleChip(unsigned int Channel);		// // //
	void		 UnmuteAllChannels();
	bool		 IsChannelMuted(unsigned int Channel) const;
	void		 SetChannelMute(int Channel, bool bMute);

	// For UI updates
	bool		 IsSelecting() const;
	bool		 IsClipboardAvailable() const;

	// General
	void		 SetupColors();

	bool		 DoRelease() const;

	void		 EditReplace(stChanNote &Note);		// // //

	CPatternEditor *GetPatternEditor() const { return m_pPatternEditor; }

	// OLE
	void		 BeginDragData(int ChanOffset, int RowOffset);
	bool		 IsDragging() const;

	// Update methods
	void		 TrackChanged(unsigned int Track);

	// Auto-arpeggio
	int			 GetAutoArpeggio(unsigned int Channel);

//
// Private functions
//
private:

	CFrameEditor *GetFrameEditor() const;

	// Drawing
	void	UpdateMeters();
	
	void	InvalidateCursor();
	void	InvalidateHeader();
	void	InvalidatePatternEditor();
	void	InvalidateFrameEditor();

	void	RedrawPatternEditor();
	void	RedrawFrameEditor();

	void	PeriodicUpdate();

	// Instruments
	void		 SetInstrument(int Instrument);
	unsigned int GetInstrument() const;

	// General
	void	StepDown();

	// Input key handling
	void	HandleKeyboardInput(unsigned char Key);		// // //
	void	TranslateMidiMessage();

	void	OnKeyDirUp();
	void	OnKeyDirDown();
	void	OnKeyDirLeft();
	void	OnKeyDirRight();
	// // //
	void	OnKeyTab();
	void	OnKeyPageUp();
	void	OnKeyPageDown();
	void	OnKeyDelete();
	void	OnKeyInsert();
	void	OnKeyBackspace();
	void	OnKeyHome();
	void	OnKeyEnd();

	// Input handling
	void	KeyIncreaseAction();
	void	KeyDecreaseAction();
	
	int		TranslateKey(unsigned char Key) const;
	int		TranslateKeyDefault(unsigned char Key) const;
	int		TranslateKeyModplug(unsigned char Key) const;
	int		TranslateKeyAzerty(unsigned char Key) const;
	
	bool	CheckClearKey(unsigned char Key) const;
	bool	CheckHaltKey(unsigned char Key) const;
	bool	CheckReleaseKey(unsigned char Key) const;
	bool	CheckRepeatKey(unsigned char Key) const;
	bool	CheckEchoKey(unsigned char Key) const;		// // //

	bool	PreventRepeat(unsigned char Key, bool Insert);
	void	RepeatRelease(unsigned char Key);

	bool	EditInstrumentColumn(stChanNote &Note, int Value, bool &StepDown, bool &MoveRight, bool &MoveLeft);
	bool	EditVolumeColumn(stChanNote &Note, int Value, bool &bStepDown);
	bool	EditEffNumberColumn(stChanNote &Note, unsigned char nChar, int EffectIndex, bool &bStepDown);
	bool	EditEffParamColumn(stChanNote &Note, int Value, int EffectIndex, bool &bStepDown, bool &bMoveRight, bool &bMoveLeft);

	void	InsertNote(int Note, int Octave, int Channel, int Velocity);

	// MIDI keyboard emulation
	void	HandleKeyboardNote(char nChar, bool Pressed);

	// MIDI note functions
	void	TriggerMIDINote(unsigned int Channel, unsigned int MidiNote, unsigned int Velocity, bool Insert);
	void	ReleaseMIDINote(unsigned int Channel, unsigned int MidiNote, bool InsertCut);
	void	CutMIDINote(unsigned int Channel, unsigned int MidiNote, bool InsertCut);

	// Note handling
	void	PlayNote(unsigned int Channel, unsigned int Note, unsigned int Octave, unsigned int Velocity);
	void	ReleaseNote(unsigned int Channel);
	void	HaltNote(unsigned int Channel);
	void	HaltNoteSingle(unsigned int Channel);
	
	void	UpdateArpDisplay();
	
	// Mute methods
	bool	IsChannelSolo(unsigned int Channel) const;
	bool	IsChipSolo(unsigned int Chip) const;		// // //

	// Other
	bool	AddAction(CAction *pAction) const;
	CString	GetEffectHint(const stChanNote &Note, int Column) const;		// // //

#ifdef EXPORT_TEST
	void	DrawExportTestProgress();
#endif /* EXPORT_TEST */
	// // //
	// Keyboard
	bool	IsShiftPressed() const;
	bool	IsControlPressed() const;

	// Update timer
#if 0
	static UINT ThreadProcFunc(LPVOID pParam);

	bool	StartTimerThread();
	void	EndTimerThread();
	UINT	ThreadProc();
#endif

//
// Constants
//
public:
	static const TCHAR CLIPBOARD_ID[];

//
// View variables
//
private:
	// General
	bool				m_bHasFocus;
	UINT				m_iClipboard;
	int					m_iMenuChannel;							// Which channel a popup-menu belongs to

	// Cursor & editing
	unsigned int		m_iMoveKeyStepping;						// Number of rows to jump when moving
	unsigned int		m_iInsertKeyStepping;					// Number of rows to move when inserting notes
	unsigned int		m_iOctave;								// Selected octave	 (TODO move to mainframe)
	bool				m_bEditEnable;							// Edit is enabled
	bool				m_bSwitchToInstrument;					// Select active instrument
	bool				m_bFollowMode;							// Follow mode, default true
	bool				m_bCompactMode;							// // // Compact mode, default false
	bool				m_bMaskInstrument;						// Ignore instrument column on new notes
	bool				m_bMaskVolume;							// Ignore volume column on new notes
	paste_pos_t			m_iPastePos;							// // // Paste position

	// Playing
	bool				m_bMuteChannels[MAX_CHANNELS];			// Muted channels
	int					m_iSwitchToInstrument;

	// Auto arpeggio
	char				m_iAutoArpNotes[128];
	int					m_iAutoArpPtr;
	int					m_iLastAutoArpPtr;
	int					m_iAutoArpKeyCount;
	unsigned int		m_iArpeggiate[MAX_CHANNELS];

	// Window size
	unsigned int		m_iWindowWidth;							// Width of view area
	unsigned int		m_iWindowHeight;						// Height of view area

	// Input
	char				m_cKeyList[256];
	unsigned int		m_iKeyboardNote;
	int					m_iLastNote;							// Last note added to pattern
	int					m_iLastInstrument;						// Last instrument added to pattern
	int					m_iLastVolume;							// Last volume added to pattern
	effect_t			m_iLastEffect;							// Last effect number added to pattern
	int					m_iLastEffectParam;						// Last effect parameter added to pattern
	std::unordered_map<unsigned char, int> m_iNoteCorrection;	// // // correction from changing octaves

	// MIDI
	unsigned int		m_iLastMIDINote;
	unsigned int		m_iActiveNotes[MAX_CHANNELS];

	// Drawing
	CPatternEditor		*m_pPatternEditor;						// Pointer to the pattern editor object

	// OLE support
	COleDropTarget		m_DropTarget;
	int					m_nDropEffect;
	bool				m_bDropMix;								// Copy and mix
	bool				m_bDragSource;							// This window is drag source
	bool				m_bDropped;								// Drop was performed on this window

	// Thread synchronization
	mutable CCriticalSection m_csDrawLock;						// Lock for DCs

// Operations
public:

// Overrides
public:

// Implementation
public:
	virtual ~CFamiTrackerView();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

// Generated message map functions
protected:
	DECLARE_MESSAGE_MAP()
public:
	virtual void OnDraw(CDC* /*pDC*/);
	virtual void CalcWindowRect(LPRECT lpClientRect, UINT nAdjustType = adjustBorder);
	virtual void OnUpdate(CView* /*pSender*/, LPARAM /*lHint*/, CObject* /*pHint*/);
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg void OnKeyUp(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg BOOL OnMouseWheel(UINT nFlags, short zDelta, CPoint pt);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnXButtonDown(UINT nFlags, UINT nButton, CPoint point);		// // //
	afx_msg void OnEditCopy();
	afx_msg void OnEditCut();
	afx_msg void OnEditPaste();
	afx_msg void OnEditPasteMix();		// // // case
	afx_msg void OnEditDelete();
	afx_msg void OnEditInstrumentMask();
	afx_msg void OnEditVolumeMask();
	afx_msg void OnEditPasteoverwrite();
	afx_msg void OnTrackerEdit();
	afx_msg void OnTransposeDecreasenote();
	afx_msg void OnTransposeDecreaseoctave();
	afx_msg void OnTransposeIncreasenote();
	afx_msg void OnTransposeIncreaseoctave();
	afx_msg void OnDecreaseValues();
	afx_msg void OnIncreaseValues();
	afx_msg void OnUpdateEditCut(CCmdUI *pCmdUI);
	afx_msg void OnUpdateEditCopy(CCmdUI *pCmdUI);
	afx_msg void OnUpdateEditPaste(CCmdUI *pCmdUI);
	afx_msg void OnUpdateEditDelete(CCmdUI *pCmdUI);
	afx_msg void OnUpdateTrackerEdit(CCmdUI *pCmdUI);
	afx_msg void OnUpdateEditPasteoverwrite(CCmdUI *pCmdUI);
	afx_msg void OnUpdateEditInstrumentMask(CCmdUI *pCmdUI);
	afx_msg void OnUpdateEditVolumeMask(CCmdUI *pCmdUI);
	afx_msg void OnIncreaseStepSize();
	afx_msg void OnDecreaseStepSize();
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
	afx_msg void OnEditSelectall();
	afx_msg void OnKillFocus(CWnd* pNewWnd);
	afx_msg void OnSetFocus(CWnd* pOldWnd);
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnTrackerPlayrow();
	afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg void OnRButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnTrackerToggleChannel();
	afx_msg void OnTrackerSoloChannel();
	afx_msg void OnTrackerUnmuteAllChannels();
	afx_msg void OnNextOctave();
	afx_msg void OnPreviousOctave();
	afx_msg void OnSysKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg void OnEditInterpolate();
	afx_msg void OnEditReplaceInstrument();
	afx_msg void OnEditReverse();
	afx_msg void OnNcMouseMove(UINT nHitTest, CPoint point);
	afx_msg void OnOneStepUp();
	afx_msg void OnOneStepDown();
	afx_msg void OnBlockStart();
	afx_msg void OnBlockEnd();
	afx_msg void OnPickupRow();
	afx_msg LRESULT OnUserMidiEvent(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnUserPlayerEvent(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnUserNoteEvent(WPARAM wParam, LPARAM lParam);
	virtual void OnInitialUpdate();
	virtual DROPEFFECT OnDragEnter(COleDataObject* pDataObject, DWORD dwKeyState, CPoint point);
	virtual void OnDragLeave();
	virtual DROPEFFECT OnDragOver(COleDataObject* pDataObject, DWORD dwKeyState, CPoint point);
	virtual BOOL OnDrop(COleDataObject* pDataObject, DROPEFFECT dropEffect, CPoint point);
	afx_msg void OnDestroy();
	// // //
	afx_msg LRESULT OnUserDumpInst(WPARAM wParam, LPARAM lParam);
	afx_msg void OnTrackerDetune();
	afx_msg void OnTrackerGroove();
	afx_msg void OnUpdateFind(CCmdUI *pCmdUI);
	afx_msg void OnCoarseDecreaseValues();
	afx_msg void OnCoarseIncreaseValues();
	afx_msg void OnEditCopyAsText();
	afx_msg void OnEditCopyAsVolumeSequence();
	afx_msg void OnEditCopyAsPPMCK();
	afx_msg void OnEditSelectnone();
	afx_msg void OnEditSelectrow();
	afx_msg void OnEditSelectcolumn();
	afx_msg void OnEditSelectpattern();
	afx_msg void OnEditSelectframe();
	afx_msg void OnEditSelectchannel();
	afx_msg void OnEditSelecttrack();
	afx_msg void OnEditExpandPatterns();
	afx_msg void OnEditShrinkPatterns();
	afx_msg void OnEditStretchPatterns();
	afx_msg void OnEditPasteOverwrite();
	afx_msg void OnEditPasteInsert();
	afx_msg void OnEditPasteSpecialCursor();
	afx_msg void OnEditPasteSpecialSelection();
	afx_msg void OnEditPasteSpecialFill();
	afx_msg void OnUpdatePasteSpecial(CCmdUI *pCmdUI);
	afx_msg void OnUpdateDisableWhilePlaying(CCmdUI *pCmdUI);
	afx_msg void OnBookmarksToggle();
	afx_msg void OnBookmarksNext();
	afx_msg void OnBookmarksPrevious();
	afx_msg void OnTrackerToggleChip();
	afx_msg void OnTrackerSoloChip();
	afx_msg void OnTrackerRecordToInst();
	afx_msg void OnTrackerRecorderSettings();
	afx_msg void OnRecallChannelState();
};

#ifndef _DEBUG  // debug version in FamiTrackerView.cpp
inline CFamiTrackerDoc* CFamiTrackerView::GetDocument() const
   { return reinterpret_cast<CFamiTrackerDoc*>(m_pDocument); }
#endif

