/*
** FamiTracker - NES/Famicom sound tracker
** Copyright (C) 2005-2015 Jonathan Liss
**
** 0CC-FamiTracker is (C) 2014-2018 HertzDevil
**
** Dn-FamiTracker is (C) 2020-2021 D.P.C.M.
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

#include "stdafx.h"
#include "version.h"		// // //
#include <algorithm>
#include "FamiTracker.h"
#include "FamiTrackerDoc.h"
#include "FamiTrackerView.h"
#include "MainFrm.h"
#include "ExportDialog.h"
#include "CreateWaveDlg.h"
#include "InstrumentEditDlg.h"
#include "ModulePropertiesDlg.h"
#include "ChannelsDlg.h"
#include "VisualizerWnd.h"
#include "TextExporter.h"
#include "ConfigGeneral.h"
#include "ConfigVersion.h"		// // //
#include "ConfigAppearance.h"
#include "ConfigMIDI.h"
#include "ConfigSound.h"
#include "ConfigShortcuts.h"
#include "ConfigWindow.h"
#include "ConfigMixer.h"
#include "ConfigEmulation.h"	// // !!
#include "ConfigGUI.h"
#include "Settings.h"
#include "Accelerator.h"
#include "SoundGen.h"
#include "MIDI.h"
#include "TrackerChannel.h"
#include "CommentsDlg.h"
#include "InstrumentFileTree.h"
#include "PatternAction.h"
#include "FrameAction.h"
#include "PatternEditor.h"
#include "FrameEditor.h"
#include "APU/APU.h"
#include "GrooveDlg.h"		// // //
#include "GotoDlg.h"		// // //
#include "BookmarkDlg.h"	// // //
#include "SwapDlg.h"		// // //
#include "SpeedDlg.h"		// // //
#include "TransposeDlg.h"	// // //
#include "DPI.h"		// // //
#include "HistoryFileDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

static UINT indicators[] =
{
	ID_SEPARATOR,           // status line indicator
	ID_INDICATOR_CHIP,
	ID_INDICATOR_INSTRUMENT,
	ID_INDICATOR_OCTAVE,
	ID_INDICATOR_RATE,
	ID_INDICATOR_TEMPO,
	ID_INDICATOR_TIME,
	ID_INDICATOR_POS,
	ID_INDICATOR_CAPS,
	ID_INDICATOR_NUM,
	ID_INDICATOR_SCRL,
};

// Timers
enum {
	TMR_WELCOME,
	TMR_AUDIO_CHECK,
	TMR_AUTOSAVE
};

// Repeat config
const int REPEAT_DELAY = 20;
const int REPEAT_TIME = 200;

// CMainFrame

IMPLEMENT_DYNCREATE(CMainFrame, CFrameWnd)

// CMainFrame construction/destruction

CMainFrame::CMainFrame() :
	m_pVisualizerWnd(NULL),
	m_pFrameEditor(NULL),
	m_pGrooveDlg(NULL),			// // //
	m_pFindDlg(NULL),			// // //
	m_pBookmarkDlg(NULL),		// // //
	m_pPerformanceDlg(NULL),	// // //
	m_pImageList(NULL),
	m_pLockedEditSpeed(NULL),
	m_pLockedEditTempo(NULL),
	m_pLockedEditLength(NULL),
	m_pLockedEditFrames(NULL),
	m_pLockedEditStep(NULL),
	m_pLockedEditHighlight1(NULL),		// // //
	m_pLockedEditHighlight2(NULL),		// // //
	m_pButtonGroove(NULL),		// // //
	m_pButtonFixTempo(NULL),		// // //
	m_pBannerEditName(NULL),
	m_pBannerEditArtist(NULL),
	m_pBannerEditCopyright(NULL),
	m_pInstrumentList(NULL),
	m_history(NULL),
	m_iFrameEditorPos(FRAME_EDIT_POS_TOP),
	m_pInstrumentFileTree(NULL),
	m_iInstrument(0),
	m_iTrack(0),
	m_iOctave(3),
	m_iInstNumDigit(0),		// // //
	m_iInstNumCurrent(MAX_INSTRUMENTS),		// // //
	m_iKraidCounter(0)		// // // Easter Egg
{
}

CMainFrame::~CMainFrame()
{
	SAFE_RELEASE(m_pImageList);
	SAFE_RELEASE(m_pLockedEditSpeed);
	SAFE_RELEASE(m_pLockedEditTempo);
	SAFE_RELEASE(m_pLockedEditLength);
	SAFE_RELEASE(m_pLockedEditFrames);
	SAFE_RELEASE(m_pLockedEditStep);
	SAFE_RELEASE(m_pLockedEditHighlight1);		// // //
	SAFE_RELEASE(m_pLockedEditHighlight2);		// // //
	SAFE_RELEASE(m_pButtonGroove);		// // //
	SAFE_RELEASE(m_pButtonFixTempo);		// // //
	SAFE_RELEASE(m_pBannerEditName);
	SAFE_RELEASE(m_pBannerEditArtist);
	SAFE_RELEASE(m_pBannerEditCopyright);
	SAFE_RELEASE(m_pFrameEditor);
	SAFE_RELEASE(m_pGrooveDlg);			// // //
	SAFE_RELEASE(m_pFindDlg);			// // //
	SAFE_RELEASE(m_pBookmarkDlg);			// // //
	SAFE_RELEASE(m_pPerformanceDlg);		// // //
	SAFE_RELEASE(m_pInstrumentList);
	SAFE_RELEASE(m_pVisualizerWnd);
	SAFE_RELEASE(m_history);
	SAFE_RELEASE(m_pInstrumentFileTree);
}

BEGIN_MESSAGE_MAP(CMainFrame, CFrameWnd)
	ON_WM_CREATE()
	ON_WM_SIZE()
	ON_WM_TIMER()
	ON_WM_ERASEBKGND()
	ON_WM_SHOWWINDOW()
	ON_WM_DESTROY()
	ON_WM_COPYDATA()
	ON_COMMAND(ID_FILE_GENERALSETTINGS, OnFileGeneralsettings)
	ON_COMMAND(ID_FILE_IMPORTTEXT, OnFileImportText)
	ON_COMMAND(ID_FILE_EXPORTTEXT, OnFileExportText)
	ON_COMMAND(ID_FILE_CREATE_NSF, OnCreateNSF)
	ON_COMMAND(ID_FILE_CREATEWAV, OnCreateWAV)
	ON_COMMAND(ID_EDIT_UNDO, OnEditUndo)
	ON_COMMAND(ID_EDIT_REDO, OnEditRedo)
	ON_COMMAND(ID_EDIT_CUT, OnEditCut)
	ON_COMMAND(ID_EDIT_COPY, OnEditCopy)
	ON_COMMAND(ID_EDIT_PASTE, OnEditPaste)
	ON_COMMAND(ID_EDIT_DELETE, OnEditDelete)
	ON_COMMAND(ID_EDIT_SELECTALL, OnEditSelectall)
	ON_COMMAND(ID_EDIT_ENABLEMIDI, OnEditEnableMIDI)
	ON_COMMAND(ID_EDIT_EXPANDPATTERNS, OnEditExpandpatterns)
	ON_COMMAND(ID_EDIT_SHRINKPATTERNS, OnEditShrinkpatterns)
	ON_COMMAND(ID_EDIT_CLEARPATTERNS, OnEditClearPatterns)
	ON_COMMAND(ID_CLEANUP_REMOVEUNUSEDINSTRUMENTS, OnEditRemoveUnusedInstruments)
	ON_COMMAND(ID_CLEANUP_REMOVEUNUSEDPATTERNS, OnEditRemoveUnusedPatterns)
	ON_COMMAND(ID_CLEANUP_MERGEDUPLICATEDPATTERNS, OnEditMergeDuplicatedPatterns)
	ON_COMMAND(ID_INSTRUMENT_NEW, OnAddInstrument)
	ON_COMMAND(ID_INSTRUMENT_REMOVE, OnRemoveInstrument)
	ON_COMMAND(ID_INSTRUMENT_CLONE, OnCloneInstrument)
	ON_COMMAND(ID_INSTRUMENT_DEEPCLONE, OnDeepCloneInstrument)
	ON_COMMAND(ID_INSTRUMENT_SAVE, OnSaveInstrument)
	ON_COMMAND(ID_INSTRUMENT_LOAD, OnLoadInstrument)
	ON_COMMAND(ID_INSTRUMENT_EDIT, OnEditInstrument)
	ON_COMMAND(ID_INSTRUMENT_ADD_2A03, OnAddInstrument2A03)
	ON_COMMAND(ID_INSTRUMENT_ADD_VRC6, OnAddInstrumentVRC6)
	ON_COMMAND(ID_INSTRUMENT_ADD_VRC7, OnAddInstrumentVRC7)
	ON_COMMAND(ID_INSTRUMENT_ADD_FDS, OnAddInstrumentFDS)
	ON_COMMAND(ID_INSTRUMENT_ADD_MMC5, OnAddInstrumentMMC5)
	ON_COMMAND(ID_INSTRUMENT_ADD_N163, OnAddInstrumentN163)
	ON_COMMAND(ID_INSTRUMENT_ADD_S5B, OnAddInstrumentS5B)
	ON_COMMAND(ID_MODULE_MODULEPROPERTIES, OnModuleModuleproperties)
	ON_COMMAND(ID_MODULE_CHANNELS, OnModuleChannels)
	ON_COMMAND(ID_MODULE_COMMENTS, OnModuleComments)
	ON_COMMAND(ID_MODULE_INSERTFRAME, OnModuleInsertFrame)
	ON_COMMAND(ID_MODULE_REMOVEFRAME, OnModuleRemoveFrame)
	ON_COMMAND(ID_MODULE_DUPLICATEFRAME, OnModuleDuplicateFrame)
	ON_COMMAND(ID_MODULE_DUPLICATEFRAMEPATTERNS, OnModuleDuplicateFramePatterns)
	ON_COMMAND(ID_MODULE_MOVEFRAMEDOWN, OnModuleMoveframedown)
	ON_COMMAND(ID_MODULE_MOVEFRAMEUP, OnModuleMoveframeup)
	ON_COMMAND(ID_TRACKER_PLAY, OnTrackerPlay)
	ON_COMMAND(ID_TRACKER_PLAY_START, OnTrackerPlayStart)
	ON_COMMAND(ID_TRACKER_PLAY_CURSOR, OnTrackerPlayCursor)
	ON_COMMAND(ID_TRACKER_STOP, OnTrackerStop)
	ON_COMMAND(ID_TRACKER_TOGGLE_PLAY, OnTrackerTogglePlay)
	ON_COMMAND(ID_TRACKER_PLAYPATTERN, OnTrackerPlaypattern)
	ON_COMMAND(ID_TRACKER_KILLSOUND, OnTrackerKillsound)
	ON_COMMAND(ID_TRACKER_SWITCHTOTRACKINSTRUMENT, OnTrackerSwitchToInstrument)
	// // //
	ON_COMMAND(ID_TRACKER_DISPLAYREGISTERSTATE, OnTrackerDisplayRegisterState)
	ON_COMMAND(ID_VIEW_CONTROLPANEL, OnViewControlpanel)
	ON_COMMAND(ID_HELP, CFrameWnd::OnHelp)
	ON_COMMAND(ID_HELP_FINDER, CFrameWnd::OnHelpFinder)
	ON_COMMAND(ID_HELP_PERFORMANCE, OnHelpPerformance)
	ON_COMMAND(ID_HELP_EFFECTTABLE, &CMainFrame::OnHelpEffecttable)
	ON_COMMAND(ID_HELP_FAQ, &CMainFrame::OnHelpFAQ)
	ON_COMMAND(ID_DEFAULT_HELP, CFrameWnd::OnHelpFinder)
	ON_COMMAND(ID_CONTEXT_HELP, CFrameWnd::OnContextHelp)
	ON_COMMAND(ID_FRAMEEDITOR_TOP, OnFrameeditorTop)
	ON_COMMAND(ID_FRAMEEDITOR_LEFT, OnFrameeditorLeft)
	ON_COMMAND(ID_NEXT_FRAME, OnNextFrame)
	ON_COMMAND(ID_PREV_FRAME, OnPrevFrame)
	ON_COMMAND(IDC_KEYREPEAT, OnKeyRepeat)
	ON_COMMAND(ID_NEXT_SONG, OnNextSong)
	ON_COMMAND(ID_PREV_SONG, OnPrevSong)
	ON_COMMAND(IDC_FOLLOW_TOGGLE, OnToggleFollow)
	ON_COMMAND(ID_FOCUS_PATTERN_EDITOR, OnSelectPatternEditor)
	ON_COMMAND(ID_FOCUS_FRAME_EDITOR, OnSelectFrameEditor)
	ON_COMMAND(ID_CMD_NEXT_INSTRUMENT, OnNextInstrument)
	ON_COMMAND(ID_CMD_PREV_INSTRUMENT, OnPrevInstrument)
	ON_COMMAND(ID_TOGGLE_SPEED, OnToggleSpeed)
	ON_COMMAND(ID_DECAY_FAST, OnDecayFast)
	ON_COMMAND(ID_DECAY_SLOW, OnDecaySlow)
	ON_BN_CLICKED(IDC_FRAME_INC, OnBnClickedIncFrame)
	ON_BN_CLICKED(IDC_FRAME_DEC, OnBnClickedDecFrame)
	ON_BN_CLICKED(IDC_FOLLOW, OnClickedFollow)
	ON_NOTIFY(UDN_DELTAPOS, IDC_SPEED_SPIN, OnDeltaposSpeedSpin)
	ON_NOTIFY(UDN_DELTAPOS, IDC_TEMPO_SPIN, OnDeltaposTempoSpin)
	ON_NOTIFY(UDN_DELTAPOS, IDC_ROWS_SPIN, OnDeltaposRowsSpin)
	ON_NOTIFY(UDN_DELTAPOS, IDC_FRAME_SPIN, OnDeltaposFrameSpin)
	ON_NOTIFY(UDN_DELTAPOS, IDC_KEYSTEP_SPIN, OnDeltaposKeyStepSpin)
	ON_EN_CHANGE(IDC_INSTNAME, OnInstNameChange)
	ON_EN_CHANGE(IDC_KEYSTEP, OnEnKeyStepChange)
	ON_EN_CHANGE(IDC_SONG_NAME, OnEnSongNameChange)
	ON_EN_CHANGE(IDC_SONG_ARTIST, OnEnSongArtistChange)
	ON_EN_CHANGE(IDC_SONG_COPYRIGHT, OnEnSongCopyrightChange)
	ON_EN_SETFOCUS(IDC_KEYREPEAT, OnRemoveFocus)
	ON_UPDATE_COMMAND_UI(ID_EDIT_UNDO, OnUpdateEditUndo)
	ON_UPDATE_COMMAND_UI(ID_EDIT_REDO, OnUpdateEditRedo)
	ON_UPDATE_COMMAND_UI(ID_EDIT_CUT, OnUpdateEditCut)
	ON_UPDATE_COMMAND_UI(ID_EDIT_COPY, OnUpdateEditCopy)
	ON_UPDATE_COMMAND_UI(ID_EDIT_PASTE, OnUpdateEditPaste)
	ON_UPDATE_COMMAND_UI(ID_EDIT_DELETE, OnUpdateEditDelete)
	ON_UPDATE_COMMAND_UI(ID_EDIT_ENABLEMIDI, OnUpdateEditEnablemidi)
	ON_UPDATE_COMMAND_UI(ID_MODULE_INSERTFRAME, OnUpdateInsertFrame)
	ON_UPDATE_COMMAND_UI(ID_MODULE_REMOVEFRAME, OnUpdateRemoveFrame)
	ON_UPDATE_COMMAND_UI(ID_MODULE_DUPLICATEFRAME, OnUpdateDuplicateFrame)
	ON_UPDATE_COMMAND_UI(ID_MODULE_MOVEFRAMEDOWN, OnUpdateModuleMoveframedown)
	ON_UPDATE_COMMAND_UI(ID_MODULE_MOVEFRAMEUP, OnUpdateModuleMoveframeup)
	ON_UPDATE_COMMAND_UI(ID_INSTRUMENT_NEW, OnUpdateInstrumentNew)
	ON_UPDATE_COMMAND_UI(ID_INSTRUMENT_REMOVE, OnUpdateInstrumentRemove)
	ON_UPDATE_COMMAND_UI(ID_INSTRUMENT_CLONE, OnUpdateInstrumentClone)
	ON_UPDATE_COMMAND_UI(ID_INSTRUMENT_DEEPCLONE, OnUpdateInstrumentDeepClone)
	ON_UPDATE_COMMAND_UI(ID_INSTRUMENT_EDIT, OnUpdateInstrumentEdit)
	ON_UPDATE_COMMAND_UI(ID_INSTRUMENT_LOAD, OnUpdateInstrumentLoad)
	ON_UPDATE_COMMAND_UI(ID_INSTRUMENT_SAVE, OnUpdateInstrumentSave)
	ON_UPDATE_COMMAND_UI(ID_INDICATOR_INSTRUMENT, OnUpdateSBInstrument)
	ON_UPDATE_COMMAND_UI(ID_INDICATOR_OCTAVE, OnUpdateSBOctave)
	ON_UPDATE_COMMAND_UI(ID_INDICATOR_RATE, OnUpdateSBFrequency)
	ON_UPDATE_COMMAND_UI(ID_INDICATOR_TEMPO, OnUpdateSBTempo)
	ON_UPDATE_COMMAND_UI(ID_INDICATOR_CHIP, OnUpdateSBChip)
	ON_UPDATE_COMMAND_UI(IDC_KEYSTEP, OnUpdateKeyStepEdit)
	ON_UPDATE_COMMAND_UI(IDC_KEYREPEAT, OnUpdateKeyRepeat)
	ON_UPDATE_COMMAND_UI(IDC_SPEED, OnUpdateSpeedEdit)
	ON_UPDATE_COMMAND_UI(IDC_TEMPO, OnUpdateTempoEdit)
	ON_UPDATE_COMMAND_UI(IDC_ROWS, OnUpdateRowsEdit)
	ON_UPDATE_COMMAND_UI(IDC_FRAMES, OnUpdateFramesEdit)
	ON_UPDATE_COMMAND_UI(ID_NEXT_SONG, OnUpdateNextSong)
	ON_UPDATE_COMMAND_UI(ID_PREV_SONG, OnUpdatePrevSong)
	ON_UPDATE_COMMAND_UI(ID_TRACKER_SWITCHTOTRACKINSTRUMENT, OnUpdateTrackerSwitchToInstrument)
	ON_UPDATE_COMMAND_UI(ID_VIEW_CONTROLPANEL, OnUpdateViewControlpanel)
	ON_UPDATE_COMMAND_UI(ID_EDIT_EXPANDPATTERNS, OnUpdateSelectionEnabled)
	ON_UPDATE_COMMAND_UI(ID_EDIT_SHRINKPATTERNS, OnUpdateSelectionEnabled)
	ON_UPDATE_COMMAND_UI(ID_FRAMEEDITOR_TOP, OnUpdateFrameeditorTop)
	ON_UPDATE_COMMAND_UI(ID_FRAMEEDITOR_LEFT, OnUpdateFrameeditorLeft)
	ON_CBN_SELCHANGE(IDC_SUBTUNE, OnCbnSelchangeSong)
	ON_CBN_SELCHANGE(IDC_OCTAVE, OnCbnSelchangeOctave)
	ON_MESSAGE(WM_USER_DISPLAY_MESSAGE_STRING, OnDisplayMessageString)
	ON_MESSAGE(WM_USER_DISPLAY_MESSAGE_ID, OnDisplayMessageID)
	ON_COMMAND(ID_VIEW_TOOLBAR, &CMainFrame::OnViewToolbar)

	// // //
	ON_BN_CLICKED(IDC_BUTTON_GROOVE, OnToggleGroove)
	ON_BN_CLICKED(IDC_BUTTON_FIXTEMPO, OnToggleFixTempo)
	ON_BN_CLICKED(IDC_CHECK_COMPACT, OnClickedCompact)
	ON_COMMAND(ID_CMD_INST_NUM, OnTypeInstrumentNumber)
	ON_COMMAND(IDC_COMPACT_TOGGLE, OnToggleCompact)
	ON_UPDATE_COMMAND_UI(IDC_HIGHLIGHT1, OnUpdateHighlight1)
	ON_UPDATE_COMMAND_UI(IDC_HIGHLIGHT2, OnUpdateHighlight2)
	ON_NOTIFY(UDN_DELTAPOS, IDC_HIGHLIGHTSPIN1, OnDeltaposHighlightSpin1)
	ON_NOTIFY(UDN_DELTAPOS, IDC_HIGHLIGHTSPIN2, OnDeltaposHighlightSpin2)
	ON_COMMAND(ID_FILE_EXPORTROWS, OnFileExportRows)
	ON_COMMAND(ID_COPYAS_TEXT, OnEditCopyAsText)
	ON_COMMAND(ID_COPYAS_VOLUMESEQUENCE, OnEditCopyAsVolumeSequence)
	ON_COMMAND(ID_COPYAS_PPMCK, OnEditCopyAsPPMCK)
	ON_COMMAND(ID_EDIT_PASTEOVERWRITE, OnEditPasteOverwrite)
	ON_COMMAND(ID_SELECT_NONE, OnEditSelectnone)
	ON_COMMAND(ID_SELECT_ROW, OnEditSelectrow)
	ON_COMMAND(ID_SELECT_COLUMN, OnEditSelectcolumn)
	ON_COMMAND(ID_SELECT_PATTERN, OnEditSelectpattern)
	ON_COMMAND(ID_SELECT_FRAME, OnEditSelectframe)
	ON_COMMAND(ID_SELECT_CHANNEL, OnEditSelectchannel)
	ON_COMMAND(ID_SELECT_TRACK, OnEditSelecttrack)
	ON_COMMAND(ID_SELECT_OTHER, OnEditSelectother)
	ON_COMMAND(ID_EDIT_FIND_TOGGLE, OnEditFindToggle)
	ON_COMMAND(ID_FIND_NEXT, OnFindNext)
	ON_COMMAND(ID_FIND_PREVIOUS, OnFindPrevious)
	ON_COMMAND(ID_EDIT_GOTO, OnEditGoto)
	ON_COMMAND(ID_EDIT_SWAPCHANNELS, OnEditSwapChannels)
	ON_COMMAND(ID_EDIT_STRETCHPATTERNS, OnEditStretchpatterns)
	ON_COMMAND(ID_TRANSPOSE_CUSTOM, OnEditTransposeCustom)
	ON_COMMAND(ID_CLEANUP_REMOVEUNUSEDDPCMSAMPLES, OnEditRemoveUnusedSamples)
	ON_COMMAND(ID_CLEANUP_POPULATEUNIQUEPATTERNS, OnEditPopulateUniquePatterns)
	ON_COMMAND(ID_MODULE_DUPLICATECURRENTPATTERN, OnModuleDuplicateCurrentPattern)
	ON_COMMAND(ID_MODULE_GROOVE, OnModuleGrooveSettings)
	ON_COMMAND(ID_MODULE_BOOKMARK, OnModuleBookmarkSettings)
	ON_COMMAND(ID_MODULE_ESTIMATESONGLENGTH, OnModuleEstimateSongLength)
	ON_COMMAND(ID_TRACKER_PLAY_MARKER, OnTrackerPlayMarker)		// // // 050B
	ON_COMMAND(ID_TRACKER_SET_MARKER, OnTrackerSetMarker)		// // // 050B
	ON_COMMAND(ID_VIEW_AVERAGEBPM, OnTrackerDisplayAverageBPM)		// // // 050B
	ON_COMMAND(ID_TOGGLE_MULTIPLEXER, OnToggleMultiplexer)
	ON_UPDATE_COMMAND_UI(IDC_FOLLOW_TOGGLE, OnUpdateToggleFollow)
	ON_UPDATE_COMMAND_UI(IDC_COMPACT_TOGGLE, OnUpdateToggleCompact)
	ON_UPDATE_COMMAND_UI(IDC_BUTTON_FIXTEMPO, OnUpdateToggleFixTempo)
	ON_UPDATE_COMMAND_UI(IDC_BUTTON_GROOVE, OnUpdateGrooveEdit)
	ON_UPDATE_COMMAND_UI(ID_COPYAS_TEXT, OnUpdateEditCopySpecial)
	ON_UPDATE_COMMAND_UI(ID_COPYAS_VOLUMESEQUENCE, OnUpdateEditCopySpecial)
	ON_UPDATE_COMMAND_UI(ID_COPYAS_PPMCK, OnUpdateEditCopySpecial)
	ON_UPDATE_COMMAND_UI(ID_EDIT_PASTEOVERWRITE, OnUpdateEditPasteOverwrite)
	ON_UPDATE_COMMAND_UI(ID_SELECT_ROW, OnUpdatePatternEditorSelected)
	ON_UPDATE_COMMAND_UI(ID_SELECT_COLUMN, OnUpdatePatternEditorSelected)
	ON_UPDATE_COMMAND_UI(ID_SELECT_CHANNEL, OnUpdateSelectMultiFrame)
	ON_UPDATE_COMMAND_UI(ID_SELECT_TRACK, OnUpdateSelectMultiFrame)
//	ON_UPDATE_COMMAND_UI(ID_SELECT_OTHER, OnUpdateCurrentSelectionEnabled)
	ON_UPDATE_COMMAND_UI(ID_EDIT_FIND_TOGGLE, OnUpdateEditFindToggle)
	ON_UPDATE_COMMAND_UI(ID_EDIT_INTERPOLATE, OnUpdateSelectionEnabled)
	ON_UPDATE_COMMAND_UI(ID_EDIT_REVERSE, OnUpdateSelectionEnabled)
	ON_UPDATE_COMMAND_UI(ID_EDIT_REPLACEINSTRUMENT, OnUpdateSelectionEnabled)
	ON_UPDATE_COMMAND_UI(ID_EDIT_STRETCHPATTERNS, OnUpdateSelectionEnabled)
	ON_UPDATE_COMMAND_UI(ID_TRACKER_PLAY_MARKER, OnUpdateTrackerPlayMarker)		// // // 050B
	ON_UPDATE_COMMAND_UI(ID_VIEW_AVERAGEBPM, OnUpdateDisplayAverageBPM)		// // // 050B
	ON_UPDATE_COMMAND_UI(ID_TRACKER_DISPLAYREGISTERSTATE, OnUpdateDisplayRegisterState)
	ON_UPDATE_COMMAND_UI(ID_DECAY_FAST, OnUpdateDecayFast)		// // // 050B
	ON_UPDATE_COMMAND_UI(ID_DECAY_SLOW, OnUpdateDecaySlow)		// // // 050B
	ON_COMMAND(ID_KRAID1, OnEasterEggKraid1)		// Easter Egg
	ON_COMMAND(ID_KRAID2, OnEasterEggKraid2)
	ON_COMMAND(ID_KRAID3, OnEasterEggKraid3)
	ON_COMMAND(ID_KRAID4, OnEasterEggKraid4)
	ON_COMMAND(ID_KRAID5, OnEasterEggKraid5)
	// // // From CFamiTrackerView
	ON_COMMAND(ID_CMD_OCTAVE_NEXT, OnNextOctave)
	ON_COMMAND(ID_CMD_OCTAVE_PREVIOUS, OnPreviousOctave)
	ON_COMMAND(ID_TRACKER_PAL, OnTrackerPal)
	ON_COMMAND(ID_TRACKER_NTSC, OnTrackerNtsc)
	ON_COMMAND(ID_SPEED_DEFAULT, OnSpeedDefault)
	ON_COMMAND(ID_SPEED_CUSTOM, OnSpeedCustom)
	ON_UPDATE_COMMAND_UI(ID_TRACKER_PAL, OnUpdateTrackerPal)
	ON_UPDATE_COMMAND_UI(ID_TRACKER_NTSC, OnUpdateTrackerNtsc)
	ON_UPDATE_COMMAND_UI(ID_SPEED_DEFAULT, OnUpdateSpeedDefault)
	ON_UPDATE_COMMAND_UI(ID_SPEED_CUSTOM, OnUpdateSpeedCustom)
	END_MESSAGE_MAP()


BOOL CMainFrame::Create(LPCTSTR lpszClassName, LPCTSTR lpszWindowName, DWORD dwStyle , const RECT& rect , CWnd* pParentWnd , LPCTSTR lpszMenuName , DWORD dwExStyle , CCreateContext* pContext)
{
	CSettings *pSettings = theApp.GetSettings();
	RECT newrect;

	// Load stored position
	newrect.bottom	= pSettings->WindowPos.iBottom;
	newrect.left	= pSettings->WindowPos.iLeft;
	newrect.right	= pSettings->WindowPos.iRight;
	newrect.top		= pSettings->WindowPos.iTop;

	return CFrameWnd::Create(lpszClassName, lpszWindowName, dwStyle, newrect, pParentWnd, lpszMenuName, dwExStyle, pContext);
}

int CMainFrame::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	// Get the DPI setting
	if (CDC *pDC = GetDC()) {		// // //
		DPI::SetScale(pDC->GetDeviceCaps(LOGPIXELSX), pDC->GetDeviceCaps(LOGPIXELSY));
		ReleaseDC(pDC);
	}

	m_history = new History();

	if (CFrameWnd::OnCreate(lpCreateStruct) == -1)
		return -1;

	if (!CreateToolbars())
		return -1;

	if (!m_wndStatusBar.Create(this) || !m_wndStatusBar.SetIndicators(indicators, sizeof(indicators) / sizeof(UINT))) {
		TRACE("Failed to create status bar\n");
		return -1;      // fail to create
	}
	m_wndStatusBar.SetPaneInfo(1, ID_INDICATOR_CHIP, SBPS_NORMAL, 250);		// // //
	m_wndStatusBar.SetPaneInfo(2, ID_INDICATOR_INSTRUMENT, SBPS_NORMAL, 100);		// // //

	if (!CreateDialogPanels())
		return -1;

	if (!CreateInstrumentToolbar()) {
		TRACE("Failed to create instrument toolbar\n");
		return -1;      // fail to create
	}

	/*
	// TODO: Delete these three lines if you don't want the toolbar to be dockable
	m_wndToolBar.EnableDocking(CBRS_ALIGN_ANY);
	EnableDocking(CBRS_ALIGN_ANY);
	DockControlBar(&m_wndToolBar);
	*/

	if (!CreateVisualizerWindow()) {
		TRACE("Failed to create sample window\n");
		return -1;      // fail to create
	}

	// Initial message, 100ms
	SetTimer(TMR_WELCOME, 100, NULL);

	// Periodic audio check, 500ms
	SetTimer(TMR_AUDIO_CHECK, 500, NULL);

	// Auto save
#ifdef AUTOSAVE
	SetTimer(TMR_AUTOSAVE, 1000, NULL);
#endif

	m_wndOctaveBar.CheckDlgButton(IDC_FOLLOW, theApp.GetSettings()->FollowMode);
	m_wndOctaveBar.CheckDlgButton(IDC_CHECK_COMPACT, false);		// // //
	m_wndOctaveBar.SetDlgItemInt(IDC_HIGHLIGHT1, CPatternData::DEFAULT_HIGHLIGHT.First, 0);		// // //
	m_wndOctaveBar.SetDlgItemInt(IDC_HIGHLIGHT2, CPatternData::DEFAULT_HIGHLIGHT.Second, 0);

	UpdateMenus();

	// Setup saved frame editor position
	SetFrameEditorPosition(theApp.GetSettings()->FrameEditPos);

#ifdef _DEBUG
	m_strTitle.Append(_T(" [DEBUG]"));
#endif
#ifdef WIP
	m_strTitle.Append(_T(" [BETA]"));
#endif

	if (AfxGetResourceHandle() != AfxGetInstanceHandle()) {		// // //
		// Prevent confusing bugs while developing
		m_strTitle.Append(_T(" [Localization enabled]"));
	}

	return 0;
}

bool CMainFrame::CreateToolbars()
{
	REBARBANDINFO rbi1;

	if (!m_wndToolBarReBar.Create(this)) {
		TRACE("Failed to create rebar\n");
		return false;      // fail to create
	}

	// Add the toolbar
	if (!m_wndToolBar.CreateEx(this, TBSTYLE_FLAT | TBSTYLE_TRANSPARENT, WS_CHILD | WS_VISIBLE | CBRS_TOP | CBRS_TOOLTIPS | CBRS_FLYBY | CBRS_SIZE_DYNAMIC) ||
		!m_wndToolBar.LoadToolBar(IDR_MAINFRAME))  {
		TRACE("Failed to create toolbar\n");
		return false;      // fail to create
	}

	m_wndToolBar.SetBarStyle(CBRS_ALIGN_TOP | CBRS_SIZE_DYNAMIC | CBRS_TOOLTIPS | CBRS_FLYBY);

	if (!m_wndOctaveBar.Create(this, (UINT)IDD_OCTAVE, CBRS_TOOLTIPS | CBRS_FLYBY, IDD_OCTAVE)) {
		TRACE("Failed to create octave bar\n");
		return false;      // fail to create
	}

	rbi1.cbSize		= sizeof(REBARBANDINFO);
	rbi1.fMask		= RBBIM_CHILD | RBBIM_CHILDSIZE | RBBIM_STYLE | RBBIM_SIZE;
	rbi1.fStyle		= RBBS_GRIPPERALWAYS;		// // // 050B
	rbi1.hwndChild	= m_wndToolBar;
	rbi1.cxMinChild	= DPI::SX(554);
	rbi1.cyMinChild	= DPI::SY(22);
	rbi1.cx			= DPI::SX(496);

	if (!m_wndToolBarReBar.GetReBarCtrl().InsertBand(-1, &rbi1)) {
		TRACE("Failed to create rebar\n");
		return false;      // fail to create
	}

	rbi1.cbSize		= sizeof(REBARBANDINFO);
	rbi1.fMask		= RBBIM_CHILD | RBBIM_CHILDSIZE | RBBIM_STYLE | RBBIM_SIZE;
	rbi1.fStyle		= RBBS_GRIPPERALWAYS;		// // // 050B
	rbi1.hwndChild	= m_wndOctaveBar;
	rbi1.cxMinChild	= DPI::SX(460);		// // //
	rbi1.cyMinChild	= DPI::SY(22);
	rbi1.cx			= DPI::SX(100);

	if (!m_wndToolBarReBar.GetReBarCtrl().InsertBand(-1, &rbi1)) {
		TRACE("Failed to create rebar\n");
		return false;      // fail to create
	}

	m_wndToolBarReBar.GetReBarCtrl().MinimizeBand(0);

	HBITMAP hbm = (HBITMAP)::LoadImage(AfxGetInstanceHandle(), MAKEINTRESOURCE(IDB_TOOLBAR_256), IMAGE_BITMAP, DPI::SX(352), DPI::SY(16), LR_CREATEDIBSECTION);
	m_bmToolbar.Attach(hbm);

	m_ilToolBar.Create(DPI::SX(16), DPI::SY(16), ILC_COLOR8 | ILC_MASK, 4, 4);
	m_ilToolBar.Add(&m_bmToolbar, RGB(192, 192, 192));
	m_wndToolBar.GetToolBarCtrl().SetImageList(&m_ilToolBar);

	return true;
}

bool CMainFrame::CreateDialogPanels()
{
	//CDialogBar

	// Top area
	if (!m_wndControlBar.Create(this, IDD_MAINBAR, CBRS_TOP | CBRS_TOOLTIPS | CBRS_FLYBY, IDD_MAINBAR)) {
		TRACE("Failed to create frame main bar\n");
		return false;
	}

	/////////
	if (!m_wndVerticalControlBar.Create(this, IDD_MAINBAR, CBRS_LEFT | CBRS_TOOLTIPS | CBRS_FLYBY, IDD_MAINBAR)) {
		TRACE("Failed to create frame main bar\n");
		return false;
	}

	m_wndVerticalControlBar.ShowWindow(SW_HIDE);
	/////////

	// Create frame controls
	m_wndFrameControls.SetFrameParent(this);
	m_wndFrameControls.Create(IDD_FRAMECONTROLS, &m_wndControlBar);
	m_wndFrameControls.ShowWindow(SW_SHOW);

	// Create frame editor
	m_pFrameEditor = new CFrameEditor(this);

	CRect rect(12, 12, m_pFrameEditor->CalcWidth(CHANNELS_DEFAULT), 162);
	DPI::ScaleRect(rect);		// // //

	if (!m_pFrameEditor->CreateEx(WS_EX_STATICEDGE, NULL, _T(""), WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_HSCROLL, rect, (CWnd*)&m_wndControlBar, 0)) {
		TRACE("Failed to create pattern window\n");
		return false;
	}

	// // // Find / replace panel
	m_pFindDlg = new CFindDlg();
	if (!m_wndFindControlBar.Create(this, IDD_MAINBAR, CBRS_RIGHT | CBRS_TOOLTIPS | CBRS_FLYBY, IDD_MAINBAR)) {
		TRACE("Failed to create frame main bar\n");
		return false;
	}
	m_wndFindControlBar.ShowWindow(SW_HIDE);
	if (!m_pFindDlg->Create(IDD_FIND, &m_wndFindControlBar)) {
		TRACE("Failed to create find / replace dialog\n");
		return false;
	}
	m_pFindDlg->ShowWindow(SW_SHOW);

	m_wndDialogBar.SetFrameParent(this);

	if (!m_wndDialogBar.Create(IDD_MAINFRAME, &m_wndControlBar)) {
		TRACE("Failed to create dialog bar\n");
		return false;
	}

	m_wndDialogBar.ShowWindow(SW_SHOW);

	// Subclass edit boxes
	m_pLockedEditSpeed	= new CLockedEdit();
	m_pLockedEditTempo	= new CLockedEdit();
	m_pLockedEditLength = new CLockedEdit();
	m_pLockedEditFrames = new CLockedEdit();
	m_pLockedEditStep	= new CLockedEdit();
	m_pLockedEditHighlight1 = new CLockedEdit();		// // //
	m_pLockedEditHighlight2 = new CLockedEdit();		// // //
	m_pButtonGroove		= new CButton();		// // //
	m_pButtonFixTempo	= new CButton();		// // //

	m_pLockedEditSpeed->SubclassDlgItem(IDC_SPEED, &m_wndDialogBar);
	m_pLockedEditTempo->SubclassDlgItem(IDC_TEMPO, &m_wndDialogBar);
	m_pLockedEditLength->SubclassDlgItem(IDC_ROWS, &m_wndDialogBar);
	m_pLockedEditFrames->SubclassDlgItem(IDC_FRAMES, &m_wndDialogBar);
	m_pLockedEditStep->SubclassDlgItem(IDC_KEYSTEP, &m_wndDialogBar);
	m_pLockedEditHighlight1->SubclassDlgItem(IDC_HIGHLIGHT1, &m_wndOctaveBar);		// // //
	m_pLockedEditHighlight2->SubclassDlgItem(IDC_HIGHLIGHT2, &m_wndOctaveBar);		// // //
	m_pButtonGroove->SubclassDlgItem(IDC_BUTTON_GROOVE, &m_wndDialogBar);		// // //
	m_pButtonFixTempo->SubclassDlgItem(IDC_BUTTON_FIXTEMPO, &m_wndDialogBar);		// // //

	// Subclass and setup the instrument list

	m_pInstrumentList = new CInstrumentList(this);
	m_pInstrumentList->SubclassDlgItem(IDC_INSTRUMENTS, &m_wndDialogBar);

	SetupColors();

	m_pImageList = new CImageList();
	m_pImageList->Create(16, 16, ILC_COLOR32, 1, 1);
	m_pImageList->Add(theApp.LoadIcon(IDI_INST_2A03));
	m_pImageList->Add(theApp.LoadIcon(IDI_INST_VRC6));
	m_pImageList->Add(theApp.LoadIcon(IDI_INST_VRC7));
	m_pImageList->Add(theApp.LoadIcon(IDI_INST_FDS));
	m_pImageList->Add(theApp.LoadIcon(IDI_INST_N163));
	m_pImageList->Add(theApp.LoadIcon(IDI_INST_S5B));		// // //

	m_pInstrumentList->SetImageList(m_pImageList, LVSIL_NORMAL);
	m_pInstrumentList->SetImageList(m_pImageList, LVSIL_SMALL);

	m_pInstrumentList->SendMessage(LVM_SETEXTENDEDLISTVIEWSTYLE, 0, LVS_EX_FULLROWSELECT);

	// Title, author & copyright
	m_pBannerEditName = new CBannerEdit(IDS_INFO_TITLE);
	m_pBannerEditArtist = new CBannerEdit(IDS_INFO_AUTHOR);
	m_pBannerEditCopyright = new CBannerEdit(IDS_INFO_COPYRIGHT);

	m_pBannerEditName->SubclassDlgItem(IDC_SONG_NAME, &m_wndDialogBar);
	m_pBannerEditArtist->SubclassDlgItem(IDC_SONG_ARTIST, &m_wndDialogBar);
	m_pBannerEditCopyright->SubclassDlgItem(IDC_SONG_COPYRIGHT, &m_wndDialogBar);

	m_pBannerEditName->SetLimitText(31);
	m_pBannerEditArtist->SetLimitText(31);
	m_pBannerEditCopyright->SetLimitText(31);

	CEdit *pInstName = static_cast<CEdit*>(m_wndDialogBar.GetDlgItem(IDC_INSTNAME));
	pInstName->SetLimitText(CInstrument::INST_NAME_MAX - 1);

	// New instrument editor

#ifdef NEW_INSTRUMENTPANEL
/*
	if (!m_wndInstrumentBar.Create(this, IDD_INSTRUMENTPANEL, CBRS_RIGHT | CBRS_TOOLTIPS | CBRS_FLYBY, IDD_INSTRUMENTPANEL)) {
		TRACE("Failed to create frame instrument bar\n");
	}

	m_wndInstrumentBar.ShowWindow(SW_SHOW);
*/
#endif

	// Frame bar
/*
	if (!m_wndFrameBar.Create(this, IDD_FRAMEBAR, CBRS_LEFT | CBRS_TOOLTIPS | CBRS_FLYBY, IDD_FRAMEBAR)) {
		TRACE("Failed to create frame bar\n");
	}

	m_wndFrameBar.ShowWindow(SW_SHOW);
*/

	return true;
}

bool CMainFrame::CreateVisualizerWindow()
{
	CRect rect;		// // // 050B
	m_wndDialogBar.GetDlgItem(IDC_MAINFRAME_VISUALIZER)->GetWindowRect(&rect);
	GetDesktopWindow()->MapWindowPoints(&m_wndDialogBar, &rect);

	// Create the sample graph window
	m_pVisualizerWnd = new CVisualizerWnd();

	if (!m_pVisualizerWnd->CreateEx(WS_EX_STATICEDGE, NULL, _T("Samples"), WS_CHILD | WS_VISIBLE, rect, (CWnd*)&m_wndDialogBar, 0))
		return false;

	// Assign this to the sound generator
	CSoundGen *pSoundGen = theApp.GetSoundGenerator();

	if (pSoundGen)
		pSoundGen->SetVisualizerWindow(m_pVisualizerWnd);

	return true;
}

bool CMainFrame::CreateInstrumentToolbar()
{
	// Setup the instrument toolbar
	REBARBANDINFO rbi;

	if (!m_wndInstToolBarWnd.CreateEx(0, NULL, _T(""), WS_CHILD | WS_VISIBLE, DPI::Rect(310, 173, 184, 26), (CWnd*)&m_wndDialogBar, 0))
		return false;

	if (!m_wndInstToolReBar.Create(WS_CHILD | WS_VISIBLE, CRect(0, 0, 0, 0), &m_wndInstToolBarWnd, AFX_IDW_REBAR))
		return false;

	if (!m_wndInstToolBar.CreateEx(&m_wndInstToolReBar, TBSTYLE_FLAT | TBSTYLE_TRANSPARENT, WS_CHILD | WS_VISIBLE | CBRS_TOP | CBRS_TOOLTIPS | CBRS_FLYBY | CBRS_SIZE_DYNAMIC) || !m_wndInstToolBar.LoadToolBar(IDT_INSTRUMENT))
		return false;

	m_wndInstToolBar.GetToolBarCtrl().SetExtendedStyle(TBSTYLE_EX_DRAWDDARROWS);

	// Set 24-bit icons
	HBITMAP hbm = (HBITMAP)::LoadImage(AfxGetInstanceHandle(), MAKEINTRESOURCE(IDB_TOOLBAR_INST_256), IMAGE_BITMAP, DPI::SX(96), DPI::SY(16), LR_CREATEDIBSECTION);
	m_bmInstToolbar.Attach(hbm);
	m_ilInstToolBar.Create(DPI::SX(16), DPI::SY(16), ILC_COLOR24 | ILC_MASK, 4, 4);
	m_ilInstToolBar.Add(&m_bmInstToolbar, RGB(255, 0, 255));
	m_wndInstToolBar.GetToolBarCtrl().SetImageList(&m_ilInstToolBar);

	rbi.cbSize		= sizeof(REBARBANDINFO);
	rbi.fMask		= RBBIM_CHILD | RBBIM_CHILDSIZE | RBBIM_STYLE | RBBIM_TEXT;
	rbi.fStyle		= RBBS_NOGRIPPER;
	rbi.cxMinChild	= DPI::SX(300);
	rbi.cyMinChild	= DPI::SY(30);
	rbi.lpText		= "";
	rbi.cch			= 7;
	rbi.cx			= DPI::SX(300);
	rbi.hwndChild	= m_wndInstToolBar;

	m_wndInstToolReBar.InsertBand(-1, &rbi);

	// Route messages to this window
	m_wndInstToolBar.SetOwner(this);

	// Turn add new instrument button into a drop-down list
	m_wndInstToolBar.SetButtonStyle(0, TBBS_DROPDOWN);
	// Turn load instrument button into a drop-down list
	m_wndInstToolBar.SetButtonStyle(4, TBBS_DROPDOWN);

	return true;
}

void CMainFrame::ResizeFrameWindow()
{
	// Called when the number of channels has changed
	ASSERT(m_pFrameEditor != NULL);

	CFamiTrackerDoc *pDocument = static_cast<CFamiTrackerDoc*>(GetActiveDocument());

	if (pDocument != NULL) {

		int Channels = pDocument->GetAvailableChannels();
		int Height = 0, Width = 0;

		// make sure m_iMaxChannelView is updated
		m_pFrameEditor->m_iMaxChannelView = theApp.GetSettings()->GUI.iMaxChannelView;

		// Located to the right
		if (m_iFrameEditorPos == FRAME_EDIT_POS_TOP) {
			// Frame editor window
			Height = CFrameEditor::DEFAULT_HEIGHT;		// // // 050B
			Width = m_pFrameEditor->CalcWidth(Channels);

			m_pFrameEditor->MoveWindow(DPI::Rect(12, 12, Width, Height));		// // //

			// Move frame controls
			m_wndFrameControls.MoveWindow(DPI::Rect(10, Height + 21, 150, 26));		// // //
		}
		// Located to the left
		else {
			// Frame editor window
			CRect rect;
			m_wndVerticalControlBar.GetClientRect(&rect);

			Height = rect.Height() - DPI::SY(CPatternEditor::HEADER_HEIGHT_NODPI - 2);		// // //
			Width = m_pFrameEditor->CalcWidth(Channels);

			m_pFrameEditor->MoveWindow(DPI::SX(2), DPI::SY(CPatternEditor::HEADER_HEIGHT_NODPI + 1), DPI::SX(Width), Height);		// // //

			// Move frame controls
			m_wndFrameControls.MoveWindow(DPI::Rect(4, 10, 150, 26));
		}

		// Vertical control bar
		m_wndVerticalControlBar.m_sizeDefault.cx = DPI::SX(Width + 4);		// // // 050B
		m_wndVerticalControlBar.CalcFixedLayout(TRUE, FALSE);
		RecalcLayout();
	}


	int DialogStartPos = 0;
	if (m_iFrameEditorPos == FRAME_EDIT_POS_TOP) {
		CRect FrameEditorRect;
		m_pFrameEditor->GetClientRect(&FrameEditorRect);
		DialogStartPos = FrameEditorRect.right + DPI::SX(32);		// // // 050B
	}

	CRect ParentRect;
	m_wndControlBar.GetClientRect(&ParentRect);
	m_wndDialogBar.MoveWindow(DialogStartPos, DPI::SY(2), ParentRect.Width() - DialogStartPos, ParentRect.Height() - DPI::SY(4));		// // //

	CRect ControlRect;		// // //
	m_wndDialogBar.GetDlgItem(IDC_MAINFRAME_INST_TOOLBAR)->GetWindowRect(&ControlRect);
	GetDesktopWindow()->MapWindowPoints(&m_wndDialogBar, ControlRect);
	m_wndInstToolBarWnd.MoveWindow(ControlRect);

	m_pFrameEditor->RedrawWindow();
}

void CMainFrame::SetupColors()
{
	ASSERT(m_pInstrumentList != NULL);

	// Instrument list colors
	m_pInstrumentList->SetBkColor(theApp.GetSettings()->Appearance.iColBackground);
	m_pInstrumentList->SetTextBkColor(theApp.GetSettings()->Appearance.iColBackground);
	m_pInstrumentList->SetTextColor(theApp.GetSettings()->Appearance.iColPatternText);
	m_pInstrumentList->RedrawWindow();
}

void CMainFrame::SetTempo(int Tempo)
{
	CFamiTrackerDoc *pDoc = static_cast<CFamiTrackerDoc*>(GetActiveDocument());
	int MinTempo = pDoc->GetSpeedSplitPoint();
	Tempo = std::max(Tempo, MinTempo);
	Tempo = std::min(Tempo, MAX_TEMPO);
	pDoc->SetSongTempo(m_iTrack, Tempo);
	theApp.GetSoundGenerator()->ResetTempo();

	if (m_wndDialogBar.GetDlgItemInt(IDC_TEMPO) != Tempo)
		m_wndDialogBar.SetDlgItemInt(IDC_TEMPO, Tempo, FALSE);
}

void CMainFrame::SetSpeed(int Speed)
{
	CFamiTrackerDoc *pDoc = static_cast<CFamiTrackerDoc*>(GetActiveDocument());
	int MaxSpeed = pDoc->GetSongTempo(m_iTrack) ? pDoc->GetSpeedSplitPoint() - 1 : 0xFF;
	if (pDoc->GetSongGroove(m_iTrack)) {		// // //
		Speed = std::max(Speed, 0);
		Speed = std::min(Speed, MAX_GROOVE - 1);
	}
	else {
		Speed = std::max(Speed, MIN_SPEED);
		Speed = std::min(Speed, MaxSpeed);		// // //
	}
	pDoc->SetSongSpeed(m_iTrack, Speed);
	theApp.GetSoundGenerator()->ResetTempo();

	if (m_wndDialogBar.GetDlgItemInt(IDC_SPEED) != Speed)
		m_wndDialogBar.SetDlgItemInt(IDC_SPEED, Speed, FALSE);
}

void CMainFrame::SetRowCount(int Count)
{
	CFamiTrackerDoc *pDoc = static_cast<CFamiTrackerDoc*>(GetActiveDocument());
	ASSERT_VALID(pDoc);

	if (!pDoc->IsFileLoaded())
		return;

	AddAction(new CPActionPatternLen {std::min(std::max(Count, 1), MAX_PATTERN_LENGTH)});		// // //

	if (m_wndDialogBar.GetDlgItemInt(IDC_ROWS) != Count)
		m_wndDialogBar.SetDlgItemInt(IDC_ROWS, Count, FALSE);
}

void CMainFrame::SetFrameCount(int Count)
{
	CFamiTrackerDoc *pDoc = static_cast<CFamiTrackerDoc*>(GetActiveDocument());
	ASSERT_VALID(pDoc);

	if (!pDoc->IsFileLoaded())
		return;

	Count = std::max(Count, 1);
	Count = std::min(Count, MAX_FRAMES);

	AddAction(new CFActionFrameCount {std::min(std::max(Count, 1), MAX_FRAMES)});		// // //

	if (m_wndDialogBar.GetDlgItemInt(IDC_FRAMES) != Count)
		m_wndDialogBar.SetDlgItemInt(IDC_FRAMES, Count, FALSE);
}

void CMainFrame::UpdateControls()
{
	m_wndDialogBar.UpdateDialogControls(&m_wndDialogBar, TRUE);
}

void CMainFrame::SetHighlightRows(stHighlight Hl)
{
	m_wndOctaveBar.SetDlgItemInt(IDC_HIGHLIGHT1, Hl.First);
	m_wndOctaveBar.SetDlgItemInt(IDC_HIGHLIGHT2, Hl.Second);
}

void CMainFrame::DisplayOctave()
{
	CComboBox *pOctaveList = static_cast<CComboBox*>(m_wndOctaveBar.GetDlgItem(IDC_OCTAVE));
	CFamiTrackerView *pView	= static_cast<CFamiTrackerView*>(GetActiveView());
	pOctaveList->SetCurSel(GetSelectedOctave());		// // //
}

// CMainFrame diagnostics

#ifdef _DEBUG
void CMainFrame::AssertValid() const
{
	CFrameWnd::AssertValid();
}

void CMainFrame::Dump(CDumpContext& dc) const
{
	CFrameWnd::Dump(dc);
}

#endif //_DEBUG


// CMainFrame message handlers

template <typename... T>
void CMainFrame::SetStatusText(LPCTSTR Text, T... args)		// // //
{
	if (!Text)
		return;

	char Buf[512] = { };
	_sntprintf_s(Buf, sizeof(Buf), _TRUNCATE, Text, args...);
	m_wndStatusBar.SetWindowText(Buf);
}

void CMainFrame::ClearInstrumentList()
{
	// Remove all items from the instrument list
	m_pInstrumentList->DeleteAllItems();
	SetInstrumentName(_T(""));
}

void CMainFrame::NewInstrument(int ChipType)
{
	// Add new instrument to module
	CFamiTrackerDoc *pDoc = static_cast<CFamiTrackerDoc*>(GetActiveDocument());

	int Index = pDoc->AddInstrument(CFamiTrackerDoc::NEW_INST_NAME, ChipType);

	if (Index == -1) {
		// Out of slots
		AfxMessageBox(IDS_INST_LIMIT, MB_ICONERROR);
		return;
	}

	// Add to list and select
	pDoc->UpdateAllViews(NULL, UPDATE_INSTRUMENT);
	SelectInstrument(Index);
}

void CMainFrame::UpdateInstrumentList()
{
	// Rewrite the instrument list
	ClearInstrumentList();

	for (int i = 0; i < MAX_INSTRUMENTS; ++i) {
		m_pInstrumentList->InsertInstrument(i);
	}
}

void CMainFrame::SelectInstrument(int Index)
{
	// Set the selected instrument
	//
	// This might get called with non-existing instruments, in that case
	// set that instrument and clear the selection in the instrument list
	//

	CFamiTrackerDoc *pDoc = static_cast<CFamiTrackerDoc*>(GetActiveDocument());

	if (Index == -1)
		return;

	int ListCount = m_pInstrumentList->GetItemCount();

	// No instruments added
	if (ListCount == 0) {
		m_iInstrument = 0;
		SetInstrumentName(_T(""));
		return;
	}

	if (pDoc->IsInstrumentUsed(Index)) {
		// Select instrument in list
		m_pInstrumentList->SelectInstrument(Index);

		// Set instrument name
		TCHAR Text[CInstrument::INST_NAME_MAX];
		pDoc->GetInstrumentName(Index, Text);
		SetInstrumentName(Text);

		// Update instrument editor
		if (m_wndInstEdit.IsOpened())
			m_wndInstEdit.SetCurrentInstrument(Index);
	}
	else {
		// Remove selection
		m_pInstrumentList->SetSelectionMark(-1);
		m_pInstrumentList->SetItemState(m_iInstrument, 0, LVIS_SELECTED | LVIS_FOCUSED);
		SetInstrumentName(_T(""));
		CloseInstrumentEditor();
	}

	// Save selected instrument
	m_iInstrument = Index;
}

int CMainFrame::GetSelectedInstrument() const
{
	// Returns selected instrument
	return m_iInstrument;
}

void CMainFrame::SwapInstruments(int First, int Second)
{
	// Swap two instruments
	CFamiTrackerDoc *pDoc = static_cast<CFamiTrackerDoc*>(GetActiveDocument());

	pDoc->SwapInstruments(First, Second);
	pDoc->SetModifiedFlag();
	UpdateInstrumentList();
	pDoc->UpdateAllViews(NULL, UPDATE_PATTERN);

	SelectInstrument(Second);
}

void CMainFrame::OnNextInstrument()
{
	// Select next instrument in the list
	m_pInstrumentList->SelectNextItem();
}

void CMainFrame::OnPrevInstrument()
{
	// Select previous instrument in the list
	m_pInstrumentList->SelectPreviousItem();
}

void CMainFrame::OnNextOctave()		// // //
{
	int Octave = GetSelectedOctave();
	if (Octave < 7)
		SelectOctave(Octave + 1);
}

void CMainFrame::OnPreviousOctave()		// // //
{
	int Octave = GetSelectedOctave();
	if (Octave > 0)
		SelectOctave(Octave - 1);
}

static const int INST_DIGITS = 2;		// // //

void CMainFrame::OnTypeInstrumentNumber()		// // //
{
	m_iInstNumDigit = 1;
	m_iInstNumCurrent = 0;
	ShowInstrumentNumberText();
}

bool CMainFrame::TypeInstrumentNumber(int Digit)		// // //
{
	if (!m_iInstNumDigit) return false;
	if (Digit != -1) {
		m_iInstNumCurrent |= Digit << (4 * (INST_DIGITS - m_iInstNumDigit++));
		ShowInstrumentNumberText();
		if (m_iInstNumDigit > INST_DIGITS) {
			if (m_iInstNumCurrent >= 0 && m_iInstNumCurrent < MAX_INSTRUMENTS)
				SelectInstrument(m_iInstNumCurrent);
			else {
				SetMessageText(IDS_INVALID_INST_NUM);
				MessageBeep(MB_ICONWARNING);
			}
			m_iInstNumDigit = 0;
		}
	}
	else {
		SetMessageText(IDS_INVALID_INST_NUM);
		MessageBeep(MB_ICONWARNING);
		m_iInstNumDigit = 0;
	}
	return true;
}

void CMainFrame::ShowInstrumentNumberText()
{
	CString msg, digit;
	for (int i = 0; i < INST_DIGITS; ++i)
		if (i >= m_iInstNumDigit - 1)
			digit.AppendChar(_T('_'));
		else
			digit.AppendFormat("%X", (m_iInstNumCurrent >> (4 * (INST_DIGITS - i - 1))) & 0xF);
	AfxFormatString1(msg, IDS_TYPE_INST_NUM, digit);
	SetMessageText(msg);
}

void CMainFrame::GetInstrumentName(char *pText) const
{
	m_wndDialogBar.GetDlgItem(IDC_INSTNAME)->GetWindowText(pText, CInstrument::INST_NAME_MAX);
}

void CMainFrame::SetInstrumentName(char *pText)
{
	m_wndDialogBar.GetDlgItem(IDC_INSTNAME)->SetWindowText(pText);
}

void CMainFrame::SetIndicatorTime(int Min, int Sec, int MSec)
{
	static int LMin, LSec, LMSec;

	if (Min != LMin || Sec != LSec || MSec != LMSec) {
		LMin = Min;
		LSec = Sec;
		LMSec = MSec;
		CString String;
		String.Format(_T("%02i:%02i:%01i0"), Min, Sec, MSec);
		m_wndStatusBar.SetPaneText(6, String);
	}
}

void CMainFrame::SetIndicatorPos(int Frame, int Row)
{
	CString String;
	if (theApp.GetSettings()->General.bRowInHex)		// // //
		String.Format(_T("%02X / %02X"), Row, Frame);
	else
		String.Format(_T("%02i / %02i"), Row, Frame);
	m_wndStatusBar.SetPaneText(7, String);
}

void CMainFrame::OnSize(UINT nType, int cx, int cy)
{
	CFrameWnd::OnSize(nType, cx, cy);

	if (m_wndToolBarReBar.m_hWnd == NULL)
		return;

	m_wndToolBarReBar.GetReBarCtrl().MinimizeBand(0);

	if (nType != SIZE_MINIMIZED)
		ResizeFrameWindow();
}

void CMainFrame::OnInstNameChange()
{
	CFamiTrackerDoc *pDoc = static_cast<CFamiTrackerDoc*>(GetActiveDocument());

	int SelIndex = m_pInstrumentList->GetSelectionMark();
	int SelInstIndex = m_pInstrumentList->GetInstrumentIndex(SelIndex);

	if (SelIndex == -1 || m_pInstrumentList->GetItemCount() == 0)
		return;

	if (SelInstIndex != m_iInstrument)	// Instrument selection out of sync, ignore name change
		return;

	if (!pDoc->IsInstrumentUsed(m_iInstrument))
		return;

	TCHAR Text[CInstrument::INST_NAME_MAX];
	GetInstrumentName(Text);

	// Update instrument list & document
	m_pInstrumentList->SetInstrumentName(m_iInstrument, Text);
	pDoc->SetInstrumentName(m_iInstrument, T2A(Text));
}

void CMainFrame::OnAddInstrument2A03()
{
	NewInstrument(SNDCHIP_NONE);
}

void CMainFrame::OnAddInstrumentVRC6()
{
	NewInstrument(SNDCHIP_VRC6);
}

void CMainFrame::OnAddInstrumentVRC7()
{
	NewInstrument(SNDCHIP_VRC7);
}

void CMainFrame::OnAddInstrumentFDS()
{
	NewInstrument(SNDCHIP_FDS);
}

void CMainFrame::OnAddInstrumentMMC5()
{
	NewInstrument(SNDCHIP_MMC5);
}

void CMainFrame::OnAddInstrumentN163()
{
	NewInstrument(SNDCHIP_N163);
}

void CMainFrame::OnAddInstrumentS5B()
{
	NewInstrument(SNDCHIP_S5B);
}

void CMainFrame::OnAddInstrument()
{
	// Add new instrument to module

	// Chip type depends on selected channel
	CFamiTrackerView *pView = static_cast<CFamiTrackerView*>(GetActiveView());
	int ChipType = pView->GetSelectedChipType();
	NewInstrument(ChipType);
}

void CMainFrame::OnRemoveInstrument()
{
	CFamiTrackerDoc *pDoc = static_cast<CFamiTrackerDoc*>(GetActiveDocument());

	// No instruments in list
	if (m_pInstrumentList->GetItemCount() == 0)
		return;

	int Instrument = m_iInstrument;
	int SelIndex = m_pInstrumentList->GetSelectionMark();

	ASSERT(pDoc->IsInstrumentUsed(Instrument));

	CloseInstrumentEditor();

	// Remove from document
	pDoc->RemoveInstrument(Instrument);
	pDoc->UpdateAllViews(NULL, UPDATE_INSTRUMENT);

	// Remove from list
	m_pInstrumentList->RemoveInstrument(Instrument);

	int Count = m_pInstrumentList->GetItemCount();

	// Select a new instrument
	int NewSelInst = 0;

	if (Count == 0) {
		NewSelInst = 0;
	}
	else if (Count == SelIndex) {
		NewSelInst = m_pInstrumentList->GetInstrumentIndex(SelIndex - 1);
	}
	else {
		NewSelInst = m_pInstrumentList->GetInstrumentIndex(SelIndex);
	}

	SelectInstrument(NewSelInst);
}

void CMainFrame::OnCloneInstrument()
{
	CFamiTrackerDoc *pDoc = static_cast<CFamiTrackerDoc*>(GetActiveDocument());

	// No instruments in list
	if (m_pInstrumentList->GetItemCount() == 0)
		return;

	int Slot = pDoc->CloneInstrument(m_iInstrument);

	if (Slot == -1) {
		AfxMessageBox(IDS_INST_LIMIT, MB_ICONERROR);
		return;
	}

	pDoc->UpdateAllViews(NULL, UPDATE_INSTRUMENT);
	SelectInstrument(Slot);
}

void CMainFrame::OnDeepCloneInstrument()
{
	CFamiTrackerDoc *pDoc = static_cast<CFamiTrackerDoc*>(GetActiveDocument());

	// No instruments in list
	if (m_pInstrumentList->GetItemCount() == 0)
		return;

	int Slot = pDoc->DeepCloneInstrument(m_iInstrument);

	if (Slot == -1) {
		AfxMessageBox(IDS_INST_LIMIT, MB_ICONERROR);
		return;
	}

	m_pInstrumentList->InsertInstrument(Slot);
	SelectInstrument(Slot);
}

void CMainFrame::OnBnClickedEditInst()
{
	OpenInstrumentEditor();
}

void CMainFrame::OnEditInstrument()
{
	OpenInstrumentEditor();
}

void CMainFrame::OnLoadInstrument()
{
	// Loads an instrument from a file

	CString filter = LoadDefaultFilter(IDS_FILTER_FTI, _T(".fti"));
	CFamiTrackerDoc *pDoc = static_cast<CFamiTrackerDoc*>(GetActiveDocument());
	CFileDialog FileDialog(TRUE, _T("fti"), 0, OFN_HIDEREADONLY | OFN_ALLOWMULTISELECT, filter);

	FileDialog.m_pOFN->lpstrInitialDir = theApp.GetSettings()->GetPath(PATH_FTI);

	if (FileDialog.DoModal() == IDCANCEL)
		return;

	POSITION pos (FileDialog.GetStartPosition());

	// Load multiple files
	while (pos) {
		CString csFileName(FileDialog.GetNextPathName(pos));
		int Index = pDoc->LoadInstrument(csFileName);
		if (Index == -1)
			return;
		SelectInstrument(Index);		// // //
		m_pInstrumentList->InsertInstrument(Index);
	}

	if (FileDialog.GetFileName().GetLength() == 0)		// // //
		theApp.GetSettings()->SetPath(FileDialog.GetPathName() + _T("\\"), PATH_FTI);
	else
		theApp.GetSettings()->SetPath(FileDialog.GetPathName(), PATH_FTI);
}

void CMainFrame::OnSaveInstrument()
{
#ifdef DISABLE_SAVE		// // //
	SetMessageText(IDS_DISABLE_SAVE);
	return;
#endif
	// Saves instrument to a file

	CFamiTrackerDoc *pDoc = static_cast<CFamiTrackerDoc*>(GetActiveDocument());
	CFamiTrackerView *pView = static_cast<CFamiTrackerView*>(GetActiveView());

	char Name[256];
	CString String;

	int Index = GetSelectedInstrument();

	if (Index == -1)
		return;

	if (!pDoc->IsInstrumentUsed(Index))
		return;

	pDoc->GetInstrumentName(Index, Name);

	// Remove bad characters
	for (char *ptr = Name; *ptr != 0; ++ptr) {
		if (*ptr == '/')
			*ptr = ' ';
	}

	CString filter = LoadDefaultFilter(IDS_FILTER_FTI, _T(".fti"));
	CFileDialog FileDialog(FALSE, _T("fti"), Name, OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, filter);

	FileDialog.m_pOFN->lpstrInitialDir = theApp.GetSettings()->GetPath(PATH_FTI);

	if (FileDialog.DoModal() == IDCANCEL)
		return;

	pDoc->SaveInstrument(GetSelectedInstrument(), FileDialog.GetPathName());

	theApp.GetSettings()->SetPath(FileDialog.GetPathName(), PATH_FTI);

	if (m_pInstrumentFileTree)
		m_pInstrumentFileTree->Changed();
}

void CMainFrame::OnDeltaposSpeedSpin(NMHDR *pNMHDR, LRESULT *pResult)
{
	CFamiTrackerDoc *pDoc = CFamiTrackerDoc::GetDoc();		// // //
	int NewSpeed = pDoc->GetSongSpeed(m_iTrack) - ((NMUPDOWN*)pNMHDR)->iDelta;

	if (!pDoc->GetSongGroove(m_iTrack)) {
		SetSpeed(NewSpeed);
		return;
	}
	else if (((NMUPDOWN*)pNMHDR)->iDelta < 0) {
		for (unsigned int i = pDoc->GetSongSpeed(m_iTrack) + 1; i < MAX_GROOVE; i++)
			if (pDoc->GetGroove(i) != NULL) { SetSpeed(i); return; }
	}
	else if (((NMUPDOWN*)pNMHDR)->iDelta > 0) {
		for (unsigned int i = pDoc->GetSongSpeed(m_iTrack) - 1; i >= 0; i--)
			if (pDoc->GetGroove(i) != NULL) { SetSpeed(i); return; }
	}
}

void CMainFrame::OnDeltaposTempoSpin(NMHDR *pNMHDR, LRESULT *pResult)
{
	int NewTempo = CFamiTrackerDoc::GetDoc()->GetSongTempo(m_iTrack) - ((NMUPDOWN*)pNMHDR)->iDelta;
	SetTempo(NewTempo);
}

void CMainFrame::OnDeltaposRowsSpin(NMHDR *pNMHDR, LRESULT *pResult)
{
	int NewRows = CFamiTrackerDoc::GetDoc()->GetPatternLength(m_iTrack) - ((NMUPDOWN*)pNMHDR)->iDelta;
	SetRowCount(NewRows);
}

void CMainFrame::OnDeltaposFrameSpin(NMHDR *pNMHDR, LRESULT *pResult)
{
	int NewFrames = CFamiTrackerDoc::GetDoc()->GetFrameCount(m_iTrack) - ((NMUPDOWN*)pNMHDR)->iDelta;
	SetFrameCount(NewFrames);
}

void CMainFrame::OnTrackerPlay()
{
	// Play
	theApp.StartPlayer(MODE_PLAY);
}

void CMainFrame::OnTrackerPlaypattern()
{
	// Loop pattern
	theApp.StartPlayer(MODE_PLAY_REPEAT);
}

void CMainFrame::OnTrackerPlayStart()
{
	// Play from start of song
	theApp.StartPlayer(MODE_PLAY_START);
}

void CMainFrame::OnTrackerPlayCursor()
{
	// Play from cursor
	theApp.StartPlayer(MODE_PLAY_CURSOR);
}

void CMainFrame::OnTrackerPlayMarker()		// // // 050B
{
	// Play from row marker
	if (static_cast<CFamiTrackerView*>(GetActiveView())->IsMarkerValid())
		theApp.StartPlayer(MODE_PLAY_MARKER);
}

void CMainFrame::OnUpdateTrackerPlayMarker(CCmdUI *pCmdUI)		// // // 050B
{
	pCmdUI->Enable(static_cast<CFamiTrackerView*>(GetActiveView())->IsMarkerValid() ? TRUE : FALSE);
}

void CMainFrame::OnTrackerSetMarker()		// // // 050B
{
	auto pView = static_cast<CFamiTrackerView*>(GetActiveView());
	int Frame = pView->GetSelectedFrame();
	int Row = pView->GetSelectedRow();

	if (Frame == pView->GetMarkerFrame() && Row == pView->GetMarkerRow())
		pView->SetMarker(-1, -1);
	else
		pView->SetMarker(Frame, Row);
}

void CMainFrame::OnTrackerTogglePlay()
{
	// Toggle playback
	theApp.TogglePlayer();
}

void CMainFrame::OnTrackerStop()
{
	// Stop playback
	theApp.StopPlayer();
}

void CMainFrame::OnTrackerKillsound()
{
	theApp.LoadSoundConfig();
	theApp.SilentEverything();
}

bool CMainFrame::CheckRepeat() const
{
	static UINT LastTime, RepeatCounter;
	UINT CurrentTime = GetTickCount();

	if ((CurrentTime - LastTime) < REPEAT_TIME) {
		if (RepeatCounter < REPEAT_DELAY)
			RepeatCounter++;
	}
	else {
		RepeatCounter = 0;
	}

	LastTime = CurrentTime;

	return RepeatCounter == REPEAT_DELAY;
}

void CMainFrame::OnBnClickedIncFrame()
{
	if (static_cast<CFamiTrackerView*>(GetActiveView())->GetSelectedFrame() != GetFrameEditor()->GetEditFrame())		// // //
		return;
	int Add = (CheckRepeat() ? 4 : 1);
	if (ChangeAllPatterns())
		AddAction(new CFActionChangePatternAll {Add});		// // //
	else
		AddAction(new CFActionChangePattern {Add});
}

void CMainFrame::OnBnClickedDecFrame()
{
	if (static_cast<CFamiTrackerView*>(GetActiveView())->GetSelectedFrame() != GetFrameEditor()->GetEditFrame())		// // //
		return;
	int Remove = -(CheckRepeat() ? 4 : 1);
	if (ChangeAllPatterns())
		AddAction(new CFActionChangePatternAll {Remove});		// // //
	else
		AddAction(new CFActionChangePattern {Remove});
}

bool CMainFrame::ChangeAllPatterns() const
{
	return m_wndFrameControls.IsDlgButtonChecked(IDC_CHANGE_ALL) != 0 && !GetFrameEditor()->IsSelecting();		// // //
}

void CMainFrame::OnKeyRepeat()
{
	theApp.GetSettings()->General.bKeyRepeat = (m_wndDialogBar.IsDlgButtonChecked(IDC_KEYREPEAT) == 1);
}

void CMainFrame::OnDeltaposKeyStepSpin(NMHDR *pNMHDR, LRESULT *pResult)
{
	int Pos = m_wndDialogBar.GetDlgItemInt(IDC_KEYSTEP) - ((NMUPDOWN*)pNMHDR)->iDelta;
	Pos = std::max(Pos, 0);
	Pos = std::min(Pos, MAX_PATTERN_LENGTH);
	m_wndDialogBar.SetDlgItemInt(IDC_KEYSTEP, Pos);
}

void CMainFrame::OnEnKeyStepChange()
{
	int Step = m_wndDialogBar.GetDlgItemInt(IDC_KEYSTEP);
	Step = std::max(Step, 0);
	Step = std::min(Step, MAX_PATTERN_LENGTH);
	static_cast<CFamiTrackerView*>(GetActiveView())->SetStepping(Step);
}

void CMainFrame::OnCreateNSF()
{
	CExportDialog ExportDialog(this);
	ExportDialog.DoModal();
}

void CMainFrame::OnCreateWAV()
{
#ifdef DISABLE_SAVE		// // //
	SetMessageText(IDS_DISABLE_SAVE);
	return;
#endif

	CCreateWaveDlg WaveDialog;
	WaveDialog.ShowDialog();
}

void CMainFrame::OnNextFrame()
{
	static_cast<CFamiTrackerView*>(GetActiveView())->SelectNextFrame();
}

void CMainFrame::OnPrevFrame()
{
	static_cast<CFamiTrackerView*>(GetActiveView())->SelectPrevFrame();
}

void CMainFrame::OnHelpPerformance()
{
	if (m_pPerformanceDlg == NULL)		// // //
		m_pPerformanceDlg = new CPerformanceDlg();
	if (!m_pPerformanceDlg->m_hWnd)
		m_pPerformanceDlg->Create(IDD_PERFORMANCE, this);
	if (!m_pPerformanceDlg->IsWindowVisible())
		m_pPerformanceDlg->CenterWindow();
	m_pPerformanceDlg->ShowWindow(SW_SHOW);
	m_pPerformanceDlg->SetFocus();
}

void CMainFrame::OnUpdateSBInstrument(CCmdUI *pCmdUI)
{
	CString String;
	String.Format(_T("%02X"), GetSelectedInstrument());		// // //
	unsigned int Split = static_cast<CFamiTrackerView*>(GetActiveView())->GetSplitInstrument();
	if (Split != MAX_INSTRUMENTS) {
		CString Orig = String;
		String.Format(_T("%02X / %s"), Split, Orig);
	}
	CString msg;
	AfxFormatString1(msg, ID_INDICATOR_INSTRUMENT, String);
	pCmdUI->Enable();
	pCmdUI->SetText(msg);
}

void CMainFrame::OnUpdateSBOctave(CCmdUI *pCmdUI)
{
	CString String;
	AfxFormatString1(String, ID_INDICATOR_OCTAVE, MakeIntString(GetSelectedOctave()));		// // //
	pCmdUI->Enable();
	pCmdUI->SetText(String);
}

void CMainFrame::OnUpdateSBFrequency(CCmdUI *pCmdUI)
{
	CFamiTrackerDoc *pDoc = static_cast<CFamiTrackerDoc*>(GetActiveDocument());
	machine_t Machine = pDoc->GetMachine();
	int EngineSpeed = pDoc->GetEngineSpeed();
	CString String;

	if (EngineSpeed == 0)
		EngineSpeed = (Machine == NTSC) ? CAPU::FRAME_RATE_NTSC : CAPU::FRAME_RATE_PAL;

	String.Format(_T("%i Hz"), EngineSpeed);

	pCmdUI->Enable();
	pCmdUI->SetText(String);
}

void CMainFrame::OnUpdateSBTempo(CCmdUI *pCmdUI)
{
	static int Highlight = m_wndOctaveBar.GetDlgItemInt(IDC_HIGHLIGHT1);

	CSoundGen *pSoundGen = theApp.GetSoundGenerator();
	if (pSoundGen && !pSoundGen->IsBackgroundTask()) {
		CString String;
		String.Format(_T("%.2f BPM"), pSoundGen->GetCurrentBPM());		// // //
		pCmdUI->Enable();
		pCmdUI->SetText(String);
	}
}

void CMainFrame::OnUpdateSBChip(CCmdUI *pCmdUI)
{
	CString String;

	CFamiTrackerDoc *pDoc = static_cast<CFamiTrackerDoc*>(GetActiveDocument());
	int Chip = pDoc->GetExpansionChip();

	if (!Chip)		// // //
		String = _T("No expansion chip");
	else if (!(Chip & (Chip - 1)))
		switch (Chip) {
			case SNDCHIP_VRC6:
				String = _T(" Konami VRC6");
				break;
			case SNDCHIP_VRC7:
				String = _T(" Konami VRC7");
				break;
			case SNDCHIP_FDS:
				String = _T(" Nintendo FDS");
				break;
			case SNDCHIP_MMC5:
				String = _T(" Nintendo MMC5");
				break;
			case SNDCHIP_N163:
				String = _T(" Namco 163");
				break;
			case SNDCHIP_S5B:
				String = _T(" Sunsoft 5B");
				break;
		}
	else {
		for (int i = 0; i < 6; i++)	if (Chip & (1 << i)) switch (i) {
			case 0:
				String += _T(" + VRC6");
				break;
			case 1:
				String += _T(" + VRC7");
				break;
			case 2:
				String += _T(" + FDS");
				break;
			case 3:
				String += _T(" + MMC5");
				break;
			case 4:
				String += _T(" + N163");
				break;
			case 5:
				String += _T(" + S5B");
				break;
		}
		String.Delete(0, 3);
	}

	pCmdUI->Enable();
	pCmdUI->SetText(String);
}

void CMainFrame::OnUpdateInsertFrame(CCmdUI *pCmdUI)
{
	CFamiTrackerDoc* pDoc = static_cast<CFamiTrackerDoc*>(GetActiveDocument());

	if (!pDoc->IsFileLoaded())
		return;

	pCmdUI->Enable(pDoc->GetFrameCount(m_iTrack) < MAX_FRAMES);
}

void CMainFrame::OnUpdateRemoveFrame(CCmdUI *pCmdUI)
{
	CFamiTrackerDoc* pDoc = static_cast<CFamiTrackerDoc*>(GetActiveDocument());

	if (!pDoc->IsFileLoaded())
		return;

	pCmdUI->Enable(pDoc->GetFrameCount(m_iTrack) > 1);
}

void CMainFrame::OnUpdateDuplicateFrame(CCmdUI *pCmdUI)
{
	CFamiTrackerDoc* pDoc = static_cast<CFamiTrackerDoc*>(GetActiveDocument());

	if (!pDoc->IsFileLoaded())
		return;

	pCmdUI->Enable(pDoc->GetFrameCount(m_iTrack) < MAX_FRAMES);
}

void CMainFrame::OnUpdateModuleMoveframedown(CCmdUI *pCmdUI)
{
	CFamiTrackerDoc* pDoc = static_cast<CFamiTrackerDoc*>(GetActiveDocument());
	CFamiTrackerView *pView = static_cast<CFamiTrackerView*>(GetActiveView());

	if (!pDoc->IsFileLoaded())
		return;

	pCmdUI->Enable(!(pView->GetSelectedFrame() == (pDoc->GetFrameCount(m_iTrack) - 1)));
}

void CMainFrame::OnUpdateModuleMoveframeup(CCmdUI *pCmdUI)
{
	CFamiTrackerView *pView = static_cast<CFamiTrackerView*>(GetActiveView());
	pCmdUI->Enable(pView->GetSelectedFrame() > 0);
}

void CMainFrame::OnUpdateInstrumentNew(CCmdUI *pCmdUI)
{
	pCmdUI->Enable(m_pInstrumentList->GetItemCount() < MAX_INSTRUMENTS);
}

void CMainFrame::OnUpdateInstrumentRemove(CCmdUI *pCmdUI)
{
	pCmdUI->Enable(m_pInstrumentList->GetItemCount() > 0);
}

void CMainFrame::OnUpdateInstrumentClone(CCmdUI *pCmdUI)
{
	pCmdUI->Enable(m_pInstrumentList->GetItemCount() > 0 && m_pInstrumentList->GetItemCount() < MAX_INSTRUMENTS);
}

void CMainFrame::OnUpdateInstrumentDeepClone(CCmdUI *pCmdUI)
{
	pCmdUI->Enable(m_pInstrumentList->GetItemCount() > 0 && m_pInstrumentList->GetItemCount() < MAX_INSTRUMENTS);
}

void CMainFrame::OnUpdateInstrumentLoad(CCmdUI *pCmdUI)
{
	pCmdUI->Enable(m_pInstrumentList->GetItemCount() < MAX_INSTRUMENTS);
}

void CMainFrame::OnUpdateInstrumentSave(CCmdUI *pCmdUI)
{
	pCmdUI->Enable(m_pInstrumentList->GetItemCount() > 0);
}

void CMainFrame::OnUpdateInstrumentEdit(CCmdUI *pCmdUI)
{
	pCmdUI->Enable(m_pInstrumentList->GetItemCount() > 0);
}

void CMainFrame::OnTimer(UINT_PTR nIDEvent)
{
	CString text, str;
	switch (nIDEvent) {
		// Welcome message
		case TMR_WELCOME:
			str.Format(_T("%i.%i.%i.%i"), VERSION);		// // //
			AfxFormatString1(text, IDS_WELCOME_VER_FORMAT, str);
			SetMessageText(text);
			KillTimer(TMR_WELCOME);
			break;
		// Check sound player
		case TMR_AUDIO_CHECK:
			CheckAudioStatus();
			break;
#ifdef AUTOSAVE
		// Auto save
		case TMR_AUTOSAVE: {
			/*
				CFamiTrackerDoc *pDoc = dynamic_cast<CFamiTrackerDoc*>(GetActiveDocument());
				if (pDoc != NULL)
					pDoc->AutoSave();
					*/
			}
			break;
#endif
	}

	CFrameWnd::OnTimer(nIDEvent);
}

void CMainFrame::OnUpdateKeyStepEdit(CCmdUI *pCmdUI)
{
	pCmdUI->SetText(MakeIntString(static_cast<CFamiTrackerView*>(GetActiveView())->GetStepping()));
}

void CMainFrame::OnUpdateSpeedEdit(CCmdUI *pCmdUI)
{
	if (!m_pLockedEditSpeed->IsEditable()) {
		if (m_pLockedEditSpeed->Update())
			SetSpeed(m_pLockedEditSpeed->GetValue());
		else {
			pCmdUI->SetText(MakeIntString(static_cast<CFamiTrackerDoc*>(GetActiveDocument())->GetSongSpeed(m_iTrack)));
		}
	}
}

void CMainFrame::OnUpdateTempoEdit(CCmdUI *pCmdUI)
{
	if (!m_pLockedEditTempo->IsEditable()) {
		if (m_pLockedEditTempo->Update())
			SetTempo(m_pLockedEditTempo->GetValue());
		else {
			pCmdUI->SetText(MakeIntString(static_cast<CFamiTrackerDoc*>(GetActiveDocument())->GetSongTempo(m_iTrack)));
		}
	}
}

void CMainFrame::OnUpdateRowsEdit(CCmdUI *pCmdUI)
{
	if (!m_pLockedEditLength->IsEditable()) {
		if (m_pLockedEditLength->Update())
			SetRowCount(m_pLockedEditLength->GetValue());
		else {
			pCmdUI->SetText(MakeIntString(static_cast<CFamiTrackerDoc*>(GetActiveDocument())->GetPatternLength(m_iTrack)));
		}
	}
}

void CMainFrame::OnUpdateFramesEdit(CCmdUI *pCmdUI)
{
	if (!m_pLockedEditFrames->IsEditable()) {
		if (m_pLockedEditFrames->Update())
			SetFrameCount(m_pLockedEditFrames->GetValue());
		else {
			pCmdUI->SetText(MakeIntString(static_cast<CFamiTrackerDoc*>(GetActiveDocument())->GetFrameCount(m_iTrack)));
		}
	}
}

void CMainFrame::OnUpdateHighlight1(CCmdUI *pCmdUI)		// // //
{
	if (!m_pLockedEditHighlight1->IsEditable()) {
		CFamiTrackerDoc *pDoc = static_cast<CFamiTrackerDoc*>(GetActiveDocument());
		if (m_pLockedEditHighlight1->Update())
			AddAction(new CPActionHighlight {stHighlight {
				std::max(0, std::min(MAX_PATTERN_LENGTH, m_pLockedEditHighlight1->GetValue())),
				pDoc->GetHighlight().Second,
				0
			}});
		else
			pCmdUI->SetText(MakeIntString(pDoc->GetHighlight().First));
	}
}

void CMainFrame::OnUpdateHighlight2(CCmdUI *pCmdUI)		// // //
{
	if (!m_pLockedEditHighlight2->IsEditable()) {
		CFamiTrackerDoc *pDoc = static_cast<CFamiTrackerDoc*>(GetActiveDocument());
		if (m_pLockedEditHighlight2->Update())
			AddAction(new CPActionHighlight {stHighlight {
				pDoc->GetHighlight().First,
				std::max(0, std::min(MAX_PATTERN_LENGTH, m_pLockedEditHighlight2->GetValue())),
				0
			}});
		else
			pCmdUI->SetText(MakeIntString(pDoc->GetHighlight().Second));
	}
}

void CMainFrame::OnFileGeneralsettings()
{
	CString Title;
	GetMessageString(IDS_CONFIG_WINDOW, Title);
	CPropertySheet ConfigWindow(Title, this, 0);

	CConfigGeneral		TabGeneral;
	CConfigVersion		TabVersion;		// // //
	CConfigAppearance	TabAppearance;
	CConfigMIDI			TabMIDI;
	CConfigSound		TabSound;
	CConfigShortcuts	TabShortcuts;
	CConfigMixer		TabMixer;
	CConfigEmulation	TabEmulation;	// // !!
	CConfigGUI			TabGUI;

	ConfigWindow.m_psh.dwFlags	&= ~PSH_HASHELP;
	TabGeneral.m_psp.dwFlags	&= ~PSP_HASHELP;
	TabVersion.m_psp.dwFlags	&= ~PSP_HASHELP;
	TabAppearance.m_psp.dwFlags &= ~PSP_HASHELP;
	TabMIDI.m_psp.dwFlags		&= ~PSP_HASHELP;
	TabSound.m_psp.dwFlags		&= ~PSP_HASHELP;
	TabShortcuts.m_psp.dwFlags	&= ~PSP_HASHELP;
	TabMixer.m_psp.dwFlags		&= ~PSP_HASHELP;
	TabEmulation.m_psp.dwFlags	&= ~PSP_HASHELP;
	TabGUI.m_psp.dwFlags		&= ~PSP_HASHELP;

	ConfigWindow.AddPage(&TabGeneral);
	ConfigWindow.AddPage(&TabVersion);
	ConfigWindow.AddPage(&TabAppearance);
	ConfigWindow.AddPage(&TabMIDI);
	ConfigWindow.AddPage(&TabSound);
	ConfigWindow.AddPage(&TabShortcuts);
	ConfigWindow.AddPage(&TabMixer);
	ConfigWindow.AddPage(&TabEmulation);
	ConfigWindow.AddPage(&TabGUI);

	ConfigWindow.DoModal();
}

void CMainFrame::SetSongInfo(const char *pName, const char *pArtist, const char *pCopyright)
{
	m_wndDialogBar.SetDlgItemText(IDC_SONG_NAME, pName);
	m_wndDialogBar.SetDlgItemText(IDC_SONG_ARTIST, pArtist);
	m_wndDialogBar.SetDlgItemText(IDC_SONG_COPYRIGHT, pCopyright);
}

void CMainFrame::OnEnSongNameChange()
{
	char Text[32];
	m_wndDialogBar.GetDlgItemText(IDC_SONG_NAME, Text, 32);
	static_cast<CFamiTrackerDoc*>(GetActiveDocument())->SetSongName(Text);
}

void CMainFrame::OnEnSongArtistChange()
{
	char Text[32];
	m_wndDialogBar.GetDlgItemText(IDC_SONG_ARTIST, Text, 32);
	static_cast<CFamiTrackerDoc*>(GetActiveDocument())->SetSongArtist(Text);
}

void CMainFrame::OnEnSongCopyrightChange()
{
	char Text[32];
	m_wndDialogBar.GetDlgItemText(IDC_SONG_COPYRIGHT, Text, 32);
	static_cast<CFamiTrackerDoc*>(GetActiveDocument())->SetSongCopyright(Text);
}

void CMainFrame::ChangeNoteState(int Note)
{
	m_wndInstEdit.ChangeNoteState(Note);
}

void CMainFrame::OpenInstrumentEditor()
{
	// Bring up the instrument editor for the selected instrument
	CFamiTrackerDoc	*pDoc = static_cast<CFamiTrackerDoc*>(GetActiveDocument());

	int Instrument = GetSelectedInstrument();

	if (pDoc->IsInstrumentUsed(Instrument)) {
		if (m_wndInstEdit.IsOpened() == false) {
			m_wndInstEdit.SetInstrumentManager(pDoc->GetInstrumentManager());		// // //
			m_wndInstEdit.Create(IDD_INSTRUMENT, this);
			m_wndInstEdit.SetCurrentInstrument(Instrument);
			m_wndInstEdit.ShowWindow(SW_SHOW);
		}
		else
			m_wndInstEdit.SetCurrentInstrument(Instrument);
		m_wndInstEdit.UpdateWindow();
	}
}

void CMainFrame::CloseInstrumentEditor()
{
	// Close the instrument editor, in case it was opened
	if (m_wndInstEdit.IsOpened())
		m_wndInstEdit.DestroyWindow();
}

void CMainFrame::CloseGrooveSettings()		// // //
{
	if (m_pGrooveDlg != NULL) {
		m_pGrooveDlg->DestroyWindow();
		SAFE_RELEASE(m_pGrooveDlg);
	}
}

void CMainFrame::CloseBookmarkSettings()		// // //
{
	if (m_pBookmarkDlg != NULL) {
		m_pBookmarkDlg->DestroyWindow();
		SAFE_RELEASE(m_pBookmarkDlg);
	}
}

void CMainFrame::UpdateBookmarkList(int Pos)		// // //
{
	if (m_pBookmarkDlg != NULL) {
		if (m_pBookmarkDlg->IsWindowVisible()) {
			m_pBookmarkDlg->LoadBookmarks(m_iTrack);
			m_pBookmarkDlg->SelectBookmark(Pos);
		}
	}
}

void CMainFrame::OnUpdateKeyRepeat(CCmdUI *pCmdUI)
{
	if (theApp.GetSettings()->General.bKeyRepeat)
		pCmdUI->SetCheck(1);
	else
		pCmdUI->SetCheck(0);
}

void CMainFrame::OnFileImportText()
{
	CString fileFilter = LoadDefaultFilter(IDS_FILTER_TXT, _T(".txt"));
	CFileDialog FileDialog(TRUE, 0, 0, OFN_HIDEREADONLY, fileFilter);

	if (GetActiveDocument()->SaveModified() == 0)
		return;

	if (FileDialog.DoModal() == IDCANCEL)
		return;

	CTextExport Exporter;
	CFamiTrackerDoc	*pDoc = static_cast<CFamiTrackerDoc*>(GetActiveDocument());

	CString sResult;		// // //
	do sResult = Exporter.ImportFile(FileDialog.GetPathName(), pDoc);
	while (sResult == _T("Retry"));
	if (sResult.GetLength() > 0)
	{
		AfxMessageBox(sResult, MB_OK | MB_ICONEXCLAMATION);
	}

	SetSongInfo(pDoc->GetSongName(), pDoc->GetSongArtist(), pDoc->GetSongCopyright());
	pDoc->SetModifiedFlag(TRUE);
	// TODO figure out how to handle this case, call OnInitialUpdate??
	//pDoc->UpdateAllViews(NULL, CHANGED_ERASE);		// Remove
	pDoc->UpdateAllViews(NULL, UPDATE_PROPERTIES);
	pDoc->UpdateAllViews(NULL, UPDATE_INSTRUMENT);
	//pDoc->UpdateAllViews(NULL, UPDATE_ENTIRE);		// TODO Remove
	theApp.GetSoundGenerator()->DocumentPropertiesChanged(pDoc);
	pDoc->SetExceededFlag(false);			// // //
}

void CMainFrame::OnFileExportText()
{
#ifdef DISABLE_SAVE		// // //
	SetMessageText(IDS_DISABLE_SAVE);
	return;
#endif

	CFamiTrackerDoc	*pDoc = static_cast<CFamiTrackerDoc*>(GetActiveDocument());
	CString	DefFileName = pDoc->GetFileTitle();

	CString fileFilter = LoadDefaultFilter(IDS_FILTER_TXT, _T(".txt"));
	HistoryFileDlg FileDialog(PATH_EXPORT, FALSE, _T(".txt"), DefFileName, OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, fileFilter);
	// FileDialog.m_pOFN->lpstrInitialDir = theApp.GetSettings()->GetPath(PATH_NSF);

	if (FileDialog.DoModal() == IDCANCEL)
		return;

	CTextExport Exporter;
	CString sResult = Exporter.ExportFile(FileDialog.GetPathName(), pDoc);
	if (sResult.GetLength() > 0)
	{
		AfxMessageBox(sResult, MB_OK | MB_ICONEXCLAMATION);
	}
}

void CMainFrame::OnFileExportRows()		// // //
{
#ifdef DISABLE_SAVE		// // //
	SetMessageText(IDS_DISABLE_SAVE);
	return;
#endif

	CFamiTrackerDoc	*pDoc = (CFamiTrackerDoc*)GetActiveDocument();
	CString	DefFileName = pDoc->GetFileTitle();

	HistoryFileDlg FileDialog(PATH_EXPORT, FALSE, _T(".csv"), DefFileName, OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, _T("Comma-separated values (*.csv)|*.csv|All files|*.*||"));
	// FileDialog.m_pOFN->lpstrInitialDir = theApp.GetSettings()->GetPath(PATH_NSF);

	if (FileDialog.DoModal() == IDCANCEL)
		return;

	CTextExport Exporter;
	CString sResult = Exporter.ExportRows(FileDialog.GetPathName(), pDoc);
	if (sResult.GetLength() > 0)
	{
		AfxMessageBox(sResult, MB_OK | MB_ICONEXCLAMATION);
	}
}

BOOL CMainFrame::DestroyWindow()
{
	// Store window position

	CRect WinRect;
	int State = STATE_NORMAL;

	GetWindowRect(WinRect);

	if (IsZoomed()) {
		State = STATE_MAXIMIZED;
		// Ignore window position if maximized
		WinRect.top = theApp.GetSettings()->WindowPos.iTop;
		WinRect.bottom = theApp.GetSettings()->WindowPos.iBottom;
		WinRect.left = theApp.GetSettings()->WindowPos.iLeft;
		WinRect.right = theApp.GetSettings()->WindowPos.iRight;
	}

	if (IsIconic()) {
		WinRect.top = WinRect.left = 100;
		WinRect.bottom = 920;
		WinRect.right = 950;
		DPI::ScaleRect(WinRect);		// // // 050B
	}

	// Save window position
	theApp.GetSettings()->SetWindowPos(WinRect.left, WinRect.top, WinRect.right, WinRect.bottom, State);

	return CFrameWnd::DestroyWindow();
}

BOOL CMainFrame::OnEraseBkgnd(CDC* pDC)
{
	return FALSE;
}

void CMainFrame::OnTrackerSwitchToInstrument()
{
	CFamiTrackerView *pView	= static_cast<CFamiTrackerView*>(GetActiveView());
	pView->SwitchToInstrument(!pView->SwitchToInstrument());
}

// // //

void CMainFrame::OnTrackerDisplayAverageBPM()		// // // 050B
{
	theApp.GetSettings()->Display.bAverageBPM = !theApp.GetSettings()->Display.bAverageBPM;
}

void CMainFrame::OnTrackerDisplayRegisterState()
{
	theApp.GetSettings()->Display.bRegisterState = !theApp.GetSettings()->Display.bRegisterState;		// // //
}

void CMainFrame::OnUpdateDisplayAverageBPM(CCmdUI *pCmdUI)		// // // 050B
{
	pCmdUI->SetCheck(theApp.GetSettings()->Display.bAverageBPM ? MF_CHECKED : MF_UNCHECKED);
}

void CMainFrame::OnUpdateDisplayRegisterState(CCmdUI *pCmdUI)		// // //
{
	pCmdUI->SetCheck(theApp.GetSettings()->Display.bRegisterState ? MF_CHECKED : MF_UNCHECKED);
}

void CMainFrame::OnUpdateTrackerSwitchToInstrument(CCmdUI *pCmdUI)
{
	CFamiTrackerView *pView	= static_cast<CFamiTrackerView*>(GetActiveView());
	pCmdUI->SetCheck(pView->SwitchToInstrument() ? 1 : 0);
}

void CMainFrame::OnModuleModuleproperties()
{
	// Display module properties dialog
	CModulePropertiesDlg propertiesDlg;
	propertiesDlg.DoModal();
}

void CMainFrame::OnModuleChannels()
{
	CChannelsDlg channelsDlg;
	channelsDlg.DoModal();
}

void CMainFrame::OnModuleComments()
{
	CCommentsDlg commentsDlg;
	CFamiTrackerDoc	*pDoc = static_cast<CFamiTrackerDoc*>(GetActiveDocument());		// // //
	commentsDlg.SetComment(pDoc->GetComment());
	commentsDlg.SetShowOnLoad(pDoc->ShowCommentOnOpen());
	if (commentsDlg.DoModal() == IDOK && commentsDlg.IsChanged())
		pDoc->SetComment(commentsDlg.GetComment(), commentsDlg.GetShowOnLoad());
}

void CMainFrame::OnModuleGrooveSettings()		// // //
{
	if (m_pGrooveDlg == NULL) {
		m_pGrooveDlg = new CGrooveDlg();
		m_pGrooveDlg->Create(IDD_GROOVE, this);
	}
	if (!m_pGrooveDlg->IsWindowVisible())
		m_pGrooveDlg->CenterWindow();
	m_pGrooveDlg->ShowWindow(SW_SHOW);
	m_pGrooveDlg->SetFocus();
}

void CMainFrame::OnModuleBookmarkSettings()		// // //
{
	if (m_pBookmarkDlg == NULL) {
		m_pBookmarkDlg = new CBookmarkDlg();
		m_pBookmarkDlg->Create(IDD_BOOKMARKS, this);
	}
	if (!m_pBookmarkDlg->IsWindowVisible())
		m_pBookmarkDlg->CenterWindow();
	m_pBookmarkDlg->ShowWindow(SW_SHOW);
	m_pBookmarkDlg->SetFocus();
	m_pBookmarkDlg->SetManager(static_cast<CFamiTrackerDoc*>(GetActiveDocument())->GetBookmarkManager());
	m_pBookmarkDlg->LoadBookmarks(m_iTrack);
}

void CMainFrame::OnModuleEstimateSongLength()		// // //
{
	CFamiTrackerDoc	*pDoc = static_cast<CFamiTrackerDoc*>(GetActiveDocument());
	double Intro = pDoc->GetStandardLength(m_iTrack, 0);
	double Loop = pDoc->GetStandardLength(m_iTrack, 1) - Intro;
	Intro = Intro - Loop;
	int Rate = pDoc->GetFrameRate();

	CString str = _T("");
	str.Format(_T("Estimated duration:\nIntro: %lld:%02lld.%02lld (%lld ticks)\nLoop: %lld:%02lld.%02lld (%lld ticks)\n"),
			   static_cast<long long>(Intro + .5 / 6000) / 60,
			   static_cast<long long>(Intro + .005) % 60,
			   static_cast<long long>(Intro * 100 + .5) % 100,
			   static_cast<long long>(Intro * Rate + .5),
			   static_cast<long long>(Loop + .5 / 6000) / 60,
			   static_cast<long long>(Loop + .005) % 60,
			   static_cast<long long>(Loop * 100 + .5) % 100,
			   static_cast<long long>(Loop * Rate + .5));
	str.Append(_T("Tick counts are subject to rounding errors!"));
	AfxMessageBox(str);
}

void CMainFrame::UpdateTrackBox()
{
	// Fill the track box with all songs
	CComboBox		*pTrackBox	= static_cast<CComboBox*>(m_wndDialogBar.GetDlgItem(IDC_SUBTUNE));
	CFamiTrackerDoc	*pDoc		= static_cast<CFamiTrackerDoc*>(GetActiveDocument());
	CString			Text;

	ASSERT(pTrackBox != NULL);
	ASSERT(pDoc != NULL);

	pTrackBox->ResetContent();

	int Count = pDoc->GetTrackCount();

	for (int i = 0; i < Count; ++i) {
		Text.Format(_T("#%i %s"), i + 1, pDoc->GetTrackTitle(i).GetString());
		pTrackBox->AddString(Text);
	}

	if (GetSelectedTrack() > (Count - 1))
		SelectTrack(Count - 1);

	pTrackBox->SetCurSel(GetSelectedTrack());
}

void CMainFrame::OnCbnSelchangeSong()
{
	CComboBox *pTrackBox = static_cast<CComboBox*>(m_wndDialogBar.GetDlgItem(IDC_SUBTUNE));
	int Track = pTrackBox->GetCurSel();
	SelectTrack(Track);
	GetActiveView()->SetFocus();
}

void CMainFrame::OnCbnSelchangeOctave()
{
	CComboBox *pTrackBox	= static_cast<CComboBox*>(m_wndOctaveBar.GetDlgItem(IDC_OCTAVE));
	CFamiTrackerView *pView	= static_cast<CFamiTrackerView*>(GetActiveView());
	unsigned int Octave		= pTrackBox->GetCurSel();

	if (GetSelectedOctave() != Octave)		// // //
		SelectOctave(Octave);
}

void CMainFrame::OnRemoveFocus()
{
	GetActiveView()->SetFocus();
}

void CMainFrame::OnNextSong()
{
	CFamiTrackerDoc *pDoc = static_cast<CFamiTrackerDoc*>(GetActiveDocument());
	int Tracks = pDoc->GetTrackCount();
	int Track = GetSelectedTrack();
	if (Track < (Tracks - 1))
		SelectTrack(Track + 1);
}

void CMainFrame::OnPrevSong()
{
	int Track = GetSelectedTrack();
	if (Track > 0)
		SelectTrack(Track - 1);
}

void CMainFrame::OnUpdateNextSong(CCmdUI *pCmdUI)
{
	CFamiTrackerDoc *pDoc = static_cast<CFamiTrackerDoc*>(GetActiveDocument());
	int Tracks = pDoc->GetTrackCount();

	if (GetSelectedTrack() < (Tracks - 1))
		pCmdUI->Enable(TRUE);
	else
		pCmdUI->Enable(FALSE);
}

void CMainFrame::OnUpdatePrevSong(CCmdUI *pCmdUI)
{
	if (GetSelectedTrack() > 0)
		pCmdUI->Enable(TRUE);
	else
		pCmdUI->Enable(FALSE);
}

void CMainFrame::OnClickedFollow()
{
	CFamiTrackerView *pView	= static_cast<CFamiTrackerView*>(GetActiveView());
	bool FollowMode = m_wndOctaveBar.IsDlgButtonChecked(IDC_FOLLOW) != 0;
	theApp.GetSettings()->FollowMode = FollowMode;
	pView->SetFollowMode(FollowMode);
	pView->SetFocus();
}

void CMainFrame::OnToggleFollow()
{
	m_wndOctaveBar.CheckDlgButton(IDC_FOLLOW, !m_wndOctaveBar.IsDlgButtonChecked(IDC_FOLLOW));
	OnClickedFollow();
}

void CMainFrame::OnUpdateToggleFollow(CCmdUI *pCmdUI)		// // //
{
	pCmdUI->SetCheck(m_wndOctaveBar.IsDlgButtonChecked(IDC_FOLLOW) != 0);
}

void CMainFrame::OnClickedCompact()		// // //
{
	CFamiTrackerView *pView	= static_cast<CFamiTrackerView*>(GetActiveView());
	bool CompactMode = m_wndOctaveBar.IsDlgButtonChecked(IDC_CHECK_COMPACT) != 0;
	pView->SetCompactMode(CompactMode);
	pView->SetFocus();
}

void CMainFrame::OnToggleCompact()		// // //
{
	m_wndOctaveBar.CheckDlgButton(IDC_CHECK_COMPACT, !m_wndOctaveBar.IsDlgButtonChecked(IDC_CHECK_COMPACT));
	OnClickedCompact();
}

void CMainFrame::OnUpdateToggleCompact(CCmdUI *pCmdUI)		// // //
{
	pCmdUI->SetCheck(m_wndOctaveBar.IsDlgButtonChecked(IDC_CHECK_COMPACT) != 0);
}

void CMainFrame::OnViewControlpanel()
{
	if (m_wndControlBar.IsVisible()) {
		m_wndControlBar.ShowWindow(SW_HIDE);
	}
	else {
		m_wndControlBar.ShowWindow(SW_SHOW);
		m_wndControlBar.UpdateWindow();
	}

	RecalcLayout();
	ResizeFrameWindow();
}

void CMainFrame::OnUpdateViewControlpanel(CCmdUI *pCmdUI)
{
	pCmdUI->SetCheck(m_wndControlBar.IsVisible());
}

void CMainFrame::OnDeltaposHighlightSpin1(NMHDR *pNMHDR, LRESULT *pResult)		// // //
{
	if (CFamiTrackerDoc *pDoc = static_cast<CFamiTrackerDoc*>(GetActiveDocument())) {
		stHighlight Hl = pDoc->GetHighlight();
		Hl.First = std::max(0, std::min(MAX_PATTERN_LENGTH, Hl.First - ((NMUPDOWN*)pNMHDR)->iDelta));
		AddAction(new CPActionHighlight {Hl});
		theApp.GetSoundGenerator()->SetHighlightRows(Hl.First);		// // //
	}
}

void CMainFrame::OnDeltaposHighlightSpin2(NMHDR *pNMHDR, LRESULT *pResult)		// // //
{
	if (CFamiTrackerDoc *pDoc = static_cast<CFamiTrackerDoc*>(GetActiveDocument())) {
		stHighlight Hl = pDoc->GetHighlight();
		Hl.Second = std::max(0, std::min(MAX_PATTERN_LENGTH, Hl.Second - ((NMUPDOWN*)pNMHDR)->iDelta));
		AddAction(new CPActionHighlight {Hl});
	}
}

void CMainFrame::OnSelectPatternEditor()
{
	GetActiveView()->SetFocus();
}

void CMainFrame::OnSelectFrameEditor()
{
	m_pFrameEditor->EnableInput();
}

void CMainFrame::OnModuleInsertFrame()
{
	AddAction(new CFActionAddFrame { });		// // //
}

void CMainFrame::OnModuleRemoveFrame()
{
	AddAction(new CFActionRemoveFrame { });		// // //
}

void CMainFrame::OnModuleDuplicateFrame()
{
	AddAction(new CFActionDuplicateFrame { });		// // //
}

void CMainFrame::OnModuleDuplicateFramePatterns()
{
	AddAction(new CFActionCloneFrame { });		// // //
}

void CMainFrame::OnModuleMoveframedown()
{
	Action *pAction = new CFActionMoveDown { };		// // //
	if (AddAction(pAction)) {
		static_cast<CFamiTrackerView*>(GetActiveView())->SelectNextFrame();
		pAction->SaveRedoState(this);
	}
}

void CMainFrame::OnModuleMoveframeup()
{
	Action *pAction = new CFActionMoveUp { };		// // //
	if (AddAction(pAction)) {
		static_cast<CFamiTrackerView*>(GetActiveView())->SelectPrevFrame();
		pAction->SaveRedoState(this);
	}
}

void CMainFrame::OnModuleDuplicateCurrentPattern()		// // //
{
	AddAction(new CFActionClonePatterns { });
}

// UI updates

void CMainFrame::OnUpdateEditCut(CCmdUI *pCmdUI)
{
	if (GetFocus() == GetActiveView())
		static_cast<CFamiTrackerView*>(GetActiveView())->OnUpdateEditCut(pCmdUI);
	else if (GetFocus() == GetFrameEditor())
		pCmdUI->Enable(TRUE);
}

void CMainFrame::OnUpdateEditCopy(CCmdUI *pCmdUI)
{
	CFamiTrackerView *pView	= static_cast<CFamiTrackerView*>(GetActiveView());
	pCmdUI->Enable((pView->IsSelecting() || GetFocus() == m_pFrameEditor) ? 1 : 0);
}

void CMainFrame::OnUpdatePatternEditorSelected(CCmdUI *pCmdUI)		// // //
{
	pCmdUI->Enable(GetFocus() == GetActiveView() ? 1 : 0);
}

void CMainFrame::OnUpdateEditCopySpecial(CCmdUI *pCmdUI)		// // //
{
	CFamiTrackerView *pView = static_cast<CFamiTrackerView*>(GetActiveView());
	pCmdUI->Enable((pView->IsSelecting() && GetFocus() == GetActiveView()) ? 1 : 0);
}

void CMainFrame::OnUpdateSelectMultiFrame(CCmdUI *pCmdUI)		// // //
{
	pCmdUI->Enable(theApp.GetSettings()->General.bMultiFrameSel ? TRUE : FALSE);
}

void CMainFrame::OnUpdateEditPaste(CCmdUI *pCmdUI)
{
	if (GetFocus() == GetActiveView())
		pCmdUI->Enable(static_cast<CFamiTrackerView*>(GetActiveView())->IsClipboardAvailable() ? 1 : 0);
	else if (GetFocus() == GetFrameEditor())
		pCmdUI->Enable(GetFrameEditor()->IsClipboardAvailable() ? 1 : 0);
}

void CMainFrame::OnUpdateEditDelete(CCmdUI *pCmdUI)
{
	CFamiTrackerView *pView	= static_cast<CFamiTrackerView*>(GetActiveView());
	pCmdUI->Enable((pView->IsSelecting() || GetFocus() == m_pFrameEditor) ? 1 : 0);
}

void CMainFrame::OnHelpEffecttable()
{
	// Display effect table in help
	HtmlHelp((DWORD_PTR)_T("effect_list.htm"), HH_DISPLAY_TOPIC);
}

void CMainFrame::OnHelpFAQ()
{
	// Display FAQ in help
	HtmlHelp((DWORD_PTR)_T("faq.htm"), HH_DISPLAY_TOPIC);
}

CFrameEditor *CMainFrame::GetFrameEditor() const
{
	return m_pFrameEditor;
}

CVisualizerWnd *CMainFrame::GetVisualizerWnd() const {		// // //
	return m_pVisualizerWnd;
}

void CMainFrame::OnEditEnableMIDI()
{
	theApp.GetMIDI()->ToggleInput();
}

void CMainFrame::OnUpdateEditEnablemidi(CCmdUI *pCmdUI)
{
	pCmdUI->Enable(theApp.GetMIDI()->IsAvailable());
	pCmdUI->SetCheck(theApp.GetMIDI()->IsOpened());
}

void CMainFrame::OnShowWindow(BOOL bShow, UINT nStatus)
{
	CFrameWnd::OnShowWindow(bShow, nStatus);

	if (bShow == TRUE) {
		// Set the window state as saved in settings
		if (theApp.GetSettings()->WindowPos.iState == STATE_MAXIMIZED)
			CFrameWnd::ShowWindow(SW_MAXIMIZE);
	}
}

void CMainFrame::OnDestroy()
{
	TRACE("FrameWnd: Destroying main frame window\n");

	CSoundGen *pSoundGen = theApp.GetSoundGenerator();

	KillTimer(TMR_AUDIO_CHECK);

	// Clean up sound stuff
	if (pSoundGen && pSoundGen->IsRunning()) {
		// Remove visualizer window from sound generator
		pSoundGen->SetVisualizerWindow(NULL);
		// Kill the sound interface since the main window is being destroyed
		CEvent *pSoundEvent = new CEvent(FALSE, FALSE);
		pSoundGen->PostGuiMessage(WM_USER_CLOSE_SOUND, (WPARAM)pSoundEvent, NULL);
		// Wait for sound to close
		DWORD dwResult = ::WaitForSingleObject(pSoundEvent->m_hObject, CSoundGen::AUDIO_TIMEOUT + 1000);

		if (dwResult != WAIT_OBJECT_0) {
			// The CEvent object will leak if this happens, but the program won't crash
			TRACE(_T("MainFrame: Error while waiting for sound to close!\n"));
		}
		else
			delete pSoundEvent;
	}

	CFrameWnd::OnDestroy();
}

int CMainFrame::GetSelectedTrack() const
{
	return m_iTrack;
}

void CMainFrame::SelectTrack(unsigned int Track)
{
	// Select a new track
	ASSERT(Track < MAX_TRACKS);

	CFamiTrackerDoc *pDoc = static_cast<CFamiTrackerDoc*>(GetActiveDocument());
	CComboBox *pTrackBox = static_cast<CComboBox*>(m_wndDialogBar.GetDlgItem(IDC_SUBTUNE));
	CFamiTrackerView *pView = static_cast<CFamiTrackerView*>(GetActiveView());

	m_iTrack = Track;

	if (theApp.IsPlaying() && Track != theApp.GetSoundGenerator()->GetPlayerTrack())		// // // 050B
		theApp.ResetPlayer();

	pTrackBox->SetCurSel(m_iTrack);
	//pDoc->UpdateAllViews(NULL, CHANGED_TRACK);

	ResetUndo();		// // // 050B
	UpdateControls();

	pView->TrackChanged(m_iTrack);
	GetFrameEditor()->CancelSelection();		// // //
	GetFrameEditor()->Invalidate();
	OnUpdateFrameTitle(TRUE);

	if (m_pBookmarkDlg != NULL)		// // //
		m_pBookmarkDlg->LoadBookmarks(m_iTrack);
}

int CMainFrame::GetSelectedOctave() const		// // // 050B
{
	return m_iOctave;
}

void CMainFrame::SelectOctave(int Octave)		// // // 050B
{
	static_cast<CFamiTrackerView*>(GetActiveView())->AdjustOctave(Octave - GetSelectedOctave());
	m_iOctave = Octave;
	DisplayOctave();
}

BOOL CMainFrame::OnNotify(WPARAM wParam, LPARAM lParam, LRESULT* pResult)
{
	LPNMTOOLBAR lpnmtb = (LPNMTOOLBAR) lParam;

	// Handle new instrument menu
	switch (((LPNMHDR)lParam)->code) {
		case TBN_DROPDOWN:
			switch (lpnmtb->iItem) {
				case ID_INSTRUMENT_NEW:
					OnNewInstrumentMenu((LPNMHDR)lParam, pResult);
					return FALSE;
				case ID_INSTRUMENT_LOAD:
					OnLoadInstrumentMenu((LPNMHDR)lParam, pResult);
					return FALSE;
			}
	}

	return CFrameWnd::OnNotify(wParam, lParam, pResult);
}

void CMainFrame::OnNewInstrumentMenu(NMHDR* pNotifyStruct, LRESULT* result)
{
	CRect rect;
	::GetWindowRect(pNotifyStruct->hwndFrom, &rect);

	CMenu menu;
	menu.CreatePopupMenu();

	CFamiTrackerDoc *pDoc = static_cast<CFamiTrackerDoc*>(GetActiveDocument());
	CFamiTrackerView *pView = static_cast<CFamiTrackerView*>(GetActiveView());

	int Chip = pDoc->GetExpansionChip();
	int SelectedChip = pDoc->GetChannel(pView->GetSelectedChannel())->GetChip();	// where the cursor is located

	menu.AppendMenu(MF_STRING, ID_INSTRUMENT_ADD_2A03, _T("New 2A03 instrument"));

	if (Chip & SNDCHIP_VRC6)
		menu.AppendMenu(MF_STRING, ID_INSTRUMENT_ADD_VRC6, _T("New VRC6 instrument"));
	if (Chip & SNDCHIP_VRC7)
		menu.AppendMenu(MF_STRING, ID_INSTRUMENT_ADD_VRC7, _T("New VRC7 instrument"));
	if (Chip & SNDCHIP_FDS)
		menu.AppendMenu(MF_STRING, ID_INSTRUMENT_ADD_FDS, _T("New FDS instrument"));
	if (Chip & SNDCHIP_MMC5)
		menu.AppendMenu(MF_STRING, ID_INSTRUMENT_ADD_MMC5, _T("New MMC5 instrument"));
	if (Chip & SNDCHIP_N163)
		menu.AppendMenu(MF_STRING, ID_INSTRUMENT_ADD_N163, _T("New Namco instrument"));
	if (Chip & SNDCHIP_S5B)
		menu.AppendMenu(MF_STRING, ID_INSTRUMENT_ADD_S5B, _T("New Sunsoft instrument"));

	switch (SelectedChip) {
		case SNDCHIP_NONE:
			menu.SetDefaultItem(ID_INSTRUMENT_ADD_2A03);
			break;
		case SNDCHIP_VRC6:
			menu.SetDefaultItem(ID_INSTRUMENT_ADD_VRC6);
			break;
		case SNDCHIP_VRC7:
			menu.SetDefaultItem(ID_INSTRUMENT_ADD_VRC7);
			break;
		case SNDCHIP_FDS:
			menu.SetDefaultItem(ID_INSTRUMENT_ADD_FDS);
			break;
		case SNDCHIP_MMC5:
			menu.SetDefaultItem(ID_INSTRUMENT_ADD_MMC5);
			break;
		case SNDCHIP_N163:
			menu.SetDefaultItem(ID_INSTRUMENT_ADD_N163);
			break;
		case SNDCHIP_S5B:
			menu.SetDefaultItem(ID_INSTRUMENT_ADD_S5B);
			break;
	}

	menu.TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, rect.left, rect.bottom, this);
}

void CMainFrame::OnLoadInstrumentMenu(NMHDR * pNotifyStruct, LRESULT * result)
{
	CRect rect;
	::GetWindowRect(pNotifyStruct->hwndFrom, &rect);

	// Build menu tree
	if (!m_pInstrumentFileTree) {
		m_pInstrumentFileTree = new CInstrumentFileTree();
		m_pInstrumentFileTree->BuildMenuTree(theApp.GetSettings()->InstrumentMenuPath);
	}
	else if (m_pInstrumentFileTree->ShouldRebuild()) {
		m_pInstrumentFileTree->BuildMenuTree(theApp.GetSettings()->InstrumentMenuPath);
	}

	UINT retValue = m_pInstrumentFileTree->GetMenu()->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON | TPM_RETURNCMD | TPM_NONOTIFY, rect.left, rect.bottom, this);

	if (retValue == CInstrumentFileTree::MENU_BASE) {
		// Open file
		OnLoadInstrument();
	}
	else if (retValue == CInstrumentFileTree::MENU_BASE + 1) {
		// Select dir
		SelectInstrumentFolder();
	}
	else if (retValue >= CInstrumentFileTree::MENU_BASE + 2) {
		// A file
		CFamiTrackerDoc *pDoc = static_cast<CFamiTrackerDoc*>(GetActiveDocument());

		int Index = pDoc->LoadInstrument(m_pInstrumentFileTree->GetFile(retValue));

		if (Index == -1)
			return;

		SelectInstrument(Index);
		m_pInstrumentList->InsertInstrument(Index);
	}
}

void CMainFrame::SelectInstrumentFolder()
{
	BROWSEINFOA Browse;
	LPITEMIDLIST lpID;
	char Path[MAX_PATH];
	CString Title;

	Title.LoadString(IDS_INSTRUMENT_FOLDER);
	Browse.lpszTitle	= Title;
	Browse.hwndOwner	= m_hWnd;
	Browse.pidlRoot		= NULL;
	Browse.lpfn			= NULL;
	Browse.ulFlags		= BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE;
	Browse.pszDisplayName = Path;

	lpID = SHBrowseForFolder(&Browse);

	if (lpID != NULL) {
		SHGetPathFromIDList(lpID, Path);
		theApp.GetSettings()->InstrumentMenuPath = Path;
		m_pInstrumentFileTree->Changed();
	}
}

BOOL CMainFrame::OnCopyData(CWnd* pWnd, COPYDATASTRUCT* pCopyDataStruct)
{
	switch (pCopyDataStruct->dwData) {
		case IPC_LOAD:
			// Load file
			if (_tcslen((LPCTSTR)pCopyDataStruct->lpData) > 0)
				theApp.OpenDocumentFile((LPCTSTR)pCopyDataStruct->lpData);
			return TRUE;
		case IPC_LOAD_PLAY:
			// Load file
			if (_tcslen((LPCTSTR)pCopyDataStruct->lpData) > 0)
				theApp.OpenDocumentFile((LPCTSTR)pCopyDataStruct->lpData);
			// and play
			if (CFamiTrackerDoc::GetDoc()->IsFileLoaded() &&
				!CFamiTrackerDoc::GetDoc()->HasLastLoadFailed())
				theApp.GetSoundGenerator()->StartPlayer(MODE_PLAY_START, 0);
			return TRUE;
	}

	return CFrameWnd::OnCopyData(pWnd, pCopyDataStruct);
}

/** \brief Performs pAction, saves "before" and "after" into pAction, and adds to history.
*/
bool CMainFrame::AddAction(Action *pAction)
{
	ASSERT(m_history != NULL);

	pAction->SaveUndoState(this);

	// Save state before operation. (Pasting also moves selection :( )
	bool saveSuccess = pAction->SaveState(this);
	if (!saveSuccess) {
		// Operation cancelled
		SAFE_RELEASE(pAction);
		return false;
	}

	// Perform action.
	try {
		pAction->Redo(this);
	} catch (std::runtime_error *e) {
		AfxMessageBox(e->what());
		pAction->Undo(this);
		SAFE_RELEASE(pAction);
		SAFE_RELEASE(e);
		return false;
	}

	// Save state after action.
	pAction->SaveRedoState(this);

	// Add action to history.
	CFamiTrackerDoc	*pDoc = (CFamiTrackerDoc*)GetActiveDocument();			// // //
	if (m_history->GetUndoLevel() == History::MAX_LEVELS)
		pDoc->SetExceededFlag();
	m_history->Push(pAction);

	return true;
}

Action *CMainFrame::GetLastAction(int Filter) const
{
	ASSERT(m_history != NULL);
	Action *pAction = m_history->GetLastAction();
	return (pAction == NULL || pAction->GetAction() != Filter) ? NULL : pAction;
}

void CMainFrame::ResetUndo()
{
	ASSERT(m_history != NULL);

	m_history->Clear();
}

void CMainFrame::OnEditUndo()
{
	ASSERT(m_history != NULL);

	if (Action *pAction = m_history->PopUndo()) {
		pAction->RestoreRedoState(this);		// // //
		pAction->Undo(this);
		pAction->RestoreUndoState(this);		// // //
	}

	CFamiTrackerDoc	*pDoc = (CFamiTrackerDoc*)GetActiveDocument();			// // //
	if (!m_history->CanUndo() && !pDoc->GetExceededFlag())
		pDoc->SetModifiedFlag(false);
}

void CMainFrame::OnEditRedo()
{
	ASSERT(m_history != NULL);

	if (Action *pAction = m_history->PopRedo()) {
		pAction->RestoreUndoState(this);		// // //
		pAction->Redo(this);
		pAction->RestoreRedoState(this);		// // //
	}
}

void CMainFrame::OnUpdateEditUndo(CCmdUI *pCmdUI)
{
	ASSERT(m_history != NULL);

	pCmdUI->Enable(m_history->CanUndo() ? 1 : 0);
}

void CMainFrame::OnUpdateEditRedo(CCmdUI *pCmdUI)
{
	ASSERT(m_history != NULL);

	pCmdUI->Enable(m_history->CanRedo() ? 1 : 0);
}

void CMainFrame::UpdateMenus()
{
	// Write new shortcuts to menus
	UpdateMenu(GetMenu());
}

void CMainFrame::UpdateMenu(CMenu *pMenu)
{
	CAccelerator *pAccel = theApp.GetAccelerator();

	for (UINT i = 0; i < static_cast<unsigned int>(pMenu->GetMenuItemCount()); ++i) {		// // //
		UINT state = pMenu->GetMenuState(i, MF_BYPOSITION);
		if (state & MF_POPUP) {
			// Update sub menu
			UpdateMenu(pMenu->GetSubMenu(i));
		}
		else if ((state & MF_SEPARATOR) == 0) {
			// Change menu name
			CString shortcut;
			UINT id = pMenu->GetMenuItemID(i);

			if (pAccel->GetShortcutString(id, shortcut)) {
				CString string;
				pMenu->GetMenuString(i, string, MF_BYPOSITION);

				int tab = string.Find(_T('\t'));

				if (tab != -1) {
					string = string.Left(tab);
				}

				string += shortcut;
				pMenu->ModifyMenu(i, MF_BYPOSITION, id, string);
			}
		}
	}
}

void CMainFrame::OnEditCut()
{
	if (GetFocus() == GetActiveView())
		static_cast<CFamiTrackerView*>(GetActiveView())->OnEditCut();
	else if (GetFocus() == GetFrameEditor())
		GetFrameEditor()->OnEditCut();
}

void CMainFrame::OnEditCopy()
{
	if (GetFocus() == GetActiveView())
		static_cast<CFamiTrackerView*>(GetActiveView())->OnEditCopy();
	else if (GetFocus() == GetFrameEditor())
		GetFrameEditor()->OnEditCopy();
}

void CMainFrame::OnEditCopyAsText()		// // //
{
	if (GetFocus() == GetActiveView())
		static_cast<CFamiTrackerView*>(GetActiveView())->OnEditCopyAsText();
	//else if (GetFocus() == GetFrameEditor())
	//	GetFrameEditor()->OnEditCopy();
}

void CMainFrame::OnEditCopyAsVolumeSequence()		// // //
{
	if (GetFocus() == GetActiveView())
		static_cast<CFamiTrackerView*>(GetActiveView())->OnEditCopyAsVolumeSequence();
}

void CMainFrame::OnEditCopyAsPPMCK()		// // //
{
	if (GetFocus() == GetActiveView())
		static_cast<CFamiTrackerView*>(GetActiveView())->OnEditCopyAsPPMCK();
}

void CMainFrame::OnEditPaste()
{
	if (GetFocus() == GetActiveView())
		static_cast<CFamiTrackerView*>(GetActiveView())->OnEditPaste();
	else if (GetFocus() == GetFrameEditor())
		GetFrameEditor()->OnEditPaste();
}

void CMainFrame::OnEditPasteOverwrite()		// // //
{
	if (GetFocus() == GetActiveView())
		static_cast<CFamiTrackerView*>(GetActiveView())->OnEditPasteOverwrite();
	else if (GetFocus() == GetFrameEditor())
		GetFrameEditor()->OnEditPasteOverwrite();
}

void CMainFrame::OnUpdateEditPasteOverwrite(CCmdUI *pCmdUI)		// // //
{
	if (GetFocus() == GetActiveView())
		static_cast<CFamiTrackerView*>(GetActiveView())->OnUpdateEditPaste(pCmdUI);
	else if (GetFocus() == GetFrameEditor())
		GetFrameEditor()->OnUpdateEditPasteOverwrite(pCmdUI);
}

void CMainFrame::OnEditDelete()
{
	if (GetFocus() == GetActiveView())
		static_cast<CFamiTrackerView*>(GetActiveView())->OnEditCut();
	else if (GetFocus() == GetFrameEditor())
		GetFrameEditor()->OnEditDelete();
}

void CMainFrame::OnEditSelectall()
{
	if (GetFocus() == GetActiveView())
		static_cast<CFamiTrackerView*>(GetActiveView())->OnEditSelectall();
	else if (GetFocus() == GetFrameEditor())		// // //
		GetFrameEditor()->OnEditSelectall();
}

void CMainFrame::OnEditSelectnone()		// // //
{
	if (GetFocus() == GetActiveView())
		static_cast<CFamiTrackerView*>(GetActiveView())->OnEditSelectnone();
	else if (GetFocus() == GetFrameEditor())
		GetFrameEditor()->CancelSelection();
}

void CMainFrame::OnEditSelectrow()
{
	if (GetFocus() == GetActiveView())
		static_cast<CFamiTrackerView*>(GetActiveView())->OnEditSelectrow();
}

void CMainFrame::OnEditSelectcolumn()
{
	if (GetFocus() == GetActiveView())
		static_cast<CFamiTrackerView*>(GetActiveView())->OnEditSelectcolumn();
}

void CMainFrame::OnEditSelectpattern()
{
	if (GetFocus() == GetActiveView())
		static_cast<CFamiTrackerView*>(GetActiveView())->OnEditSelectpattern();
	else if (GetFocus() == GetFrameEditor())
		GetFrameEditor()->OnEditSelectpattern();
}

void CMainFrame::OnEditSelectframe()
{
	if (GetFocus() == GetActiveView())
		static_cast<CFamiTrackerView*>(GetActiveView())->OnEditSelectframe();
	else if (GetFocus() == GetFrameEditor())
		GetFrameEditor()->OnEditSelectframe();
}

void CMainFrame::OnEditSelectchannel()
{
	if (GetFocus() == GetActiveView())
		static_cast<CFamiTrackerView*>(GetActiveView())->OnEditSelectchannel();
	else if (GetFocus() == GetFrameEditor())
		GetFrameEditor()->OnEditSelectchannel();
}

void CMainFrame::OnEditSelecttrack()
{
	if (GetFocus() == GetActiveView())
		static_cast<CFamiTrackerView*>(GetActiveView())->OnEditSelecttrack();
	else if (GetFocus() == GetFrameEditor())
		GetFrameEditor()->OnEditSelectall();
}

void CMainFrame::OnEditSelectother()		// // //
{
	auto pView = static_cast<CFamiTrackerView*>(GetActiveView());
	auto pEditor = pView->GetPatternEditor();
	auto pDoc = static_cast<const CFamiTrackerDoc*>(GetActiveDocument());

	if (GetFocus() == pView) {
		if (pEditor->IsSelecting()) {
			const CSelection Sel = pEditor->GetSelection().GetNormalized();
			pEditor->CancelSelection();

			CFrameSelection NewSel;
			int Frames = pDoc->GetFrameCount(m_iTrack);
			NewSel.m_cpStart.m_iFrame = Sel.m_cpStart.m_iFrame;
			NewSel.m_cpEnd.m_iFrame = Sel.m_cpEnd.m_iFrame;
			if (NewSel.m_cpStart.m_iFrame < 0) {
				NewSel.m_cpStart.m_iFrame += Frames;
				NewSel.m_cpEnd.m_iFrame += Frames;
			}
			NewSel.m_cpEnd.m_iFrame = std::min(NewSel.m_cpEnd.m_iFrame, Frames - 1);
			NewSel.m_cpStart.m_iChannel = Sel.m_cpStart.m_iChannel;
			NewSel.m_cpEnd.m_iChannel = Sel.m_cpEnd.m_iChannel;

			m_pFrameEditor->SetSelection(NewSel);
		}
		else
			m_pFrameEditor->CancelSelection();
		m_pFrameEditor->EnableInput();
	}
	else if (GetFocus() == m_pFrameEditor) {
		if (m_pFrameEditor->IsSelecting()) {
			const CFrameSelection Sel = m_pFrameEditor->GetSelection().GetNormalized();
			m_pFrameEditor->CancelSelection();

			CSelection NewSel;
			NewSel.m_cpStart.m_iFrame = Sel.m_cpStart.m_iFrame;
			NewSel.m_cpEnd.m_iFrame = Sel.m_cpEnd.m_iFrame;
			NewSel.m_cpStart.m_iRow = 0;
			NewSel.m_cpEnd.m_iRow = pDoc->GetCurrentPatternLength(m_iTrack, Sel.m_cpStart.m_iFrame) - 1;
			NewSel.m_cpStart.m_iChannel = Sel.m_cpStart.m_iChannel;
			NewSel.m_cpEnd.m_iChannel = Sel.m_cpEnd.m_iChannel;
			NewSel.m_cpStart.m_iColumn = C_NOTE;
			NewSel.m_cpEnd.m_iColumn = pEditor->GetChannelColumns(Sel.m_cpEnd.m_iChannel);

			pEditor->SetSelection(NewSel);
			pEditor->UpdateSelectionCondition();
		}
		else
			pEditor->CancelSelection();
		pView->SetFocus();
	}
}

void CMainFrame::OnDecayFast()
{
	theApp.GetSoundGenerator()->SetMeterDecayRate(theApp.GetSettings()->MeterDecayRate = DECAY_FAST);		// // // 050B
}

void CMainFrame::OnDecaySlow()
{
	theApp.GetSoundGenerator()->SetMeterDecayRate(theApp.GetSettings()->MeterDecayRate = DECAY_SLOW);		// // // 050B
}

void CMainFrame::OnUpdateDecayFast(CCmdUI *pCmdUI)		// // // 050B
{
	pCmdUI->SetCheck(theApp.GetSoundGenerator()->GetMeterDecayRate() == DECAY_FAST ? MF_CHECKED : MF_UNCHECKED);
}

void CMainFrame::OnUpdateDecaySlow(CCmdUI *pCmdUI)		// // // 050B
{
	pCmdUI->SetCheck(theApp.GetSoundGenerator()->GetMeterDecayRate() == DECAY_SLOW ? MF_CHECKED : MF_UNCHECKED);
}

void CMainFrame::OnEditExpandpatterns()
{
	if (GetFocus() == GetActiveView())		// // //
		static_cast<CFamiTrackerView*>(GetActiveView())->OnEditExpandPatterns();
}

void CMainFrame::OnEditShrinkpatterns()
{
	if (GetFocus() == GetActiveView())		// // //
		static_cast<CFamiTrackerView*>(GetActiveView())->OnEditShrinkPatterns();
}

void CMainFrame::OnEditStretchpatterns()		// // //
{
	if (GetFocus() == GetActiveView())
		static_cast<CFamiTrackerView*>(GetActiveView())->OnEditStretchPatterns();
}

void CMainFrame::OnEditTransposeCustom()		// // //
{
	CTransposeDlg TrspDlg;
	TrspDlg.SetTrack(GetSelectedTrack());
	if (TrspDlg.DoModal() == IDOK)
		ResetUndo();
}

void CMainFrame::OnEditClearPatterns()
{
	CFamiTrackerDoc *pDoc = static_cast<CFamiTrackerDoc*>(GetActiveDocument());
	int Track = GetSelectedTrack();

	if (AfxMessageBox(IDS_CLEARPATTERN, MB_YESNO | MB_ICONWARNING | MB_DEFBUTTON2) == IDNO)
		return;

	pDoc->ClearPatterns(Track);
	pDoc->UpdateAllViews(NULL, UPDATE_PATTERN);

	ResetUndo();
}

void CMainFrame::OnEditRemoveUnusedInstruments()
{
	// Removes unused instruments in the module

	CFamiTrackerDoc *pDoc = static_cast<CFamiTrackerDoc*>(GetActiveDocument());

	if (AfxMessageBox(IDS_REMOVE_INSTRUMENTS, MB_YESNO | MB_ICONWARNING | MB_DEFBUTTON2) == IDNO)
		return;

	// Current instrument might disappear
	CloseInstrumentEditor();

	pDoc->RemoveUnusedInstruments();

	// Update instrument list
	pDoc->UpdateAllViews(NULL, UPDATE_INSTRUMENT);
}

void CMainFrame::OnEditRemoveUnusedPatterns()
{
	// Removes unused patterns in the module

	CFamiTrackerDoc *pDoc = static_cast<CFamiTrackerDoc*>(GetActiveDocument());

	if (AfxMessageBox(IDS_REMOVE_PATTERNS, MB_YESNO | MB_ICONWARNING | MB_DEFBUTTON2) == IDNO)
		return;

	pDoc->RemoveUnusedPatterns();
	ResetUndo();		// // //
	pDoc->UpdateAllViews(NULL, UPDATE_PATTERN);
}

void CMainFrame::OnEditMergeDuplicatedPatterns()
{
	AddAction(new CFActionMergeDuplicated { });		// // //
}

void CMainFrame::OnUpdateSelectionEnabled(CCmdUI *pCmdUI)
{
	CFamiTrackerView *pView	= static_cast<CFamiTrackerView*>(GetActiveView());
	pCmdUI->Enable(pView->IsSelecting() ? 1 : 0);
}

void CMainFrame::OnUpdateCurrentSelectionEnabled(CCmdUI *pCmdUI)		// // //
{
	if (GetFocus() == GetActiveView())
		OnUpdateSelectionEnabled(pCmdUI);
	else if (GetFocus() == GetFrameEditor())
		pCmdUI->Enable(GetFrameEditor()->IsSelecting() ? 1 : 0);
}

void CMainFrame::SetFrameEditorPosition(int Position)
{
	// Change frame editor position
	m_iFrameEditorPos = Position;

	m_pFrameEditor->ShowWindow(SW_HIDE);

	switch (Position) {
		case FRAME_EDIT_POS_TOP:
			m_wndVerticalControlBar.ShowWindow(SW_HIDE);
			m_pFrameEditor->SetParent(&m_wndControlBar);
			m_wndFrameControls.SetParent(&m_wndControlBar);
			break;
		case FRAME_EDIT_POS_LEFT:
			m_wndVerticalControlBar.ShowWindow(SW_SHOW);
			m_pFrameEditor->SetParent(&m_wndVerticalControlBar);
			m_wndFrameControls.SetParent(&m_wndVerticalControlBar);
			break;
	}

	ResizeFrameWindow();

	m_pFrameEditor->ShowWindow(SW_SHOW);
	m_pFrameEditor->Invalidate();
	m_pFrameEditor->RedrawWindow();

	ResizeFrameWindow();	// This must be called twice or the editor disappears, I don't know why

	// Save to settings
	theApp.GetSettings()->FrameEditPos = Position;
}

void CMainFrame::SetControlPanelPosition(control_panel_pos_t Position)		// // // 050B
{
	m_iControlPanelPos = Position;
	if (m_iControlPanelPos)
		SetFrameEditorPosition(FRAME_EDIT_POS_LEFT);

	/*
	CRect Rect {193, 0, 193, 126};
	MapDialogRect(m_wndInstToolBarWnd, &Rect);

	switch (m_iControlPanelPos) {
	case CONTROL_PANEL_POS_TOP:
		m_wndToolBar.SetBarStyle(CBRS_ALIGN_TOP | CBRS_BORDER_BOTTOM | CBRS_TOOLTIPS | CBRS_FLYBY);
		m_wndToolBar.CalcFixedLayout(TRUE, FALSE);
		break;
	case CONTROL_PANEL_POS_LEFT:
		m_wndToolBar.SetBarStyle(0x1430);
		m_wndToolBar.CalcFixedLayout(TRUE, FALSE);
		break;
	case CONTROL_PANEL_POS_RIGHT:
		m_wndToolBar.SetBarStyle(0x4130);
		m_wndToolBar.CalcFixedLayout(TRUE, FALSE);
		break;
	}

	// 0x462575
	*/

	ResizeFrameWindow();
	ResizeFrameWindow();
}

void CMainFrame::OnFrameeditorTop()
{
	SetFrameEditorPosition(FRAME_EDIT_POS_TOP);
}

void CMainFrame::OnFrameeditorLeft()
{
	SetFrameEditorPosition(FRAME_EDIT_POS_LEFT);
}

void CMainFrame::OnUpdateFrameeditorTop(CCmdUI *pCmdUI)
{
	pCmdUI->SetCheck(m_iFrameEditorPos == FRAME_EDIT_POS_TOP);
}

void CMainFrame::OnUpdateFrameeditorLeft(CCmdUI *pCmdUI)
{
	pCmdUI->SetCheck(m_iFrameEditorPos == FRAME_EDIT_POS_LEFT);
}

void CMainFrame::OnToggleSpeed()
{
	CFamiTrackerDoc *pDoc = static_cast<CFamiTrackerDoc*>(GetActiveDocument());
	int Speed = pDoc->GetSpeedSplitPoint();

	if (Speed == DEFAULT_SPEED_SPLIT_POINT)
		Speed = OLD_SPEED_SPLIT_POINT;
	else
		Speed = DEFAULT_SPEED_SPLIT_POINT;

	pDoc->SetSpeedSplitPoint(Speed);
	theApp.GetSoundGenerator()->DocumentPropertiesChanged(pDoc);

	SetStatusText(_T("Speed/tempo split-point set to %i"), Speed);
}

void CMainFrame::OnUpdateFrameTitle(BOOL bAddToTitle)
{
	CFamiTrackerDoc *pDoc = static_cast<CFamiTrackerDoc*>(GetActiveDocument());

	CString title = pDoc->GetTitle();

	// Add a star (*) for unsaved documents
	if (pDoc->IsModified())
		title.Append(_T("*"));

	// Add name of subtune
	title.AppendFormat(_T(" [#%i %s]"), m_iTrack + 1, pDoc->GetTrackTitle(GetSelectedTrack()).GetString());

	title.AppendFormat(_T(" - %s"), _T(APP_NAME_VERSION));		// // //
	SetWindowText(title);
	// UpdateFrameTitleForDocument(title);
}

LRESULT CMainFrame::OnDisplayMessageString(WPARAM wParam, LPARAM lParam)
{
	AfxMessageBox((LPCTSTR)wParam, (UINT)lParam);
	return 0;
}

LRESULT CMainFrame::OnDisplayMessageID(WPARAM wParam, LPARAM lParam)
{
	AfxMessageBox((UINT)wParam, (UINT)lParam);
	return 0;
}

void CMainFrame::CheckAudioStatus()
{
	const DWORD TIMEOUT = 2000; // Display a message for 2 seconds

	// Monitor audio playback
	// TODO remove static variables
	static BOOL DisplayedError;
	static DWORD MessageTimeout;

	CSoundGen *pSoundGen = theApp.GetSoundGenerator();

	if (pSoundGen == NULL) {
		// Should really never be displayed (only during debugging)
		SetMessageText(_T("Audio is not working"));
		return;
	}

	// Wait for signals from the player thread
	if (pSoundGen->GetSoundTimeout()) {
		// No events from the audio pump
		SetMessageText(IDS_SOUND_FAIL);
		DisplayedError = TRUE;
		MessageTimeout = GetTickCount() + TIMEOUT;
	}
#ifndef _DEBUG
	else if (pSoundGen->IsBufferUnderrun()) {
		// Buffer underrun
		SetMessageText(IDS_UNDERRUN_MESSAGE);
		DisplayedError = TRUE;
		MessageTimeout = GetTickCount() + TIMEOUT;
	}
	else if (pSoundGen->IsAudioClipping()) {
		// Audio is clipping
		SetMessageText(IDS_CLIPPING_MESSAGE);
		DisplayedError = TRUE;
		MessageTimeout = GetTickCount() + TIMEOUT;
	}
#endif
	else {
		if (DisplayedError == TRUE && MessageTimeout < GetTickCount()) {
			// Restore message
			//SetMessageText(AFX_IDS_IDLEMESSAGE);
			DisplayedError = FALSE;
		}
	}
}

void CMainFrame::OnViewToolbar()
{
	BOOL Visible = m_wndToolBar.IsVisible();
	m_wndToolBar.ShowWindow(Visible ? SW_HIDE : SW_SHOW);
	m_wndToolBarReBar.ShowWindow(Visible ? SW_HIDE : SW_SHOW);
	RecalcLayout();
}

// // //

void CMainFrame::OnToggleMultiplexer()
{
	CSettings *pSettings = theApp.GetSettings();
	CSoundGen *pSoundGen = theApp.GetSoundGenerator();
	if (!pSettings->Emulation.bNamcoMixing){
		pSettings->Emulation.bNamcoMixing = true;
		pSoundGen->SetNamcoMixing(theApp.GetSettings()->Emulation.bNamcoMixing);
		SetStatusText(_T("Namco 163 multiplexer emulation disabled"));
	}
	else{
		pSettings->Emulation.bNamcoMixing = false;
		pSoundGen->SetNamcoMixing(theApp.GetSettings()->Emulation.bNamcoMixing);
		SetStatusText(_T("Namco 163 multiplexer emulation enabled"));
	}
}

void CMainFrame::OnToggleGroove()
{
	CFamiTrackerDoc *pDoc = (CFamiTrackerDoc*)GetActiveDocument();
	pDoc->SetSongGroove(m_iTrack, !pDoc->GetSongGroove(m_iTrack));
	GetActiveView()->SetFocus();
}

void CMainFrame::OnUpdateGrooveEdit(CCmdUI *pCmdUI)
{
	CFamiTrackerDoc *pDoc = (CFamiTrackerDoc*)GetActiveDocument();
	int Speed = pDoc->GetSongSpeed(m_iTrack);
	if (pDoc->GetSongGroove(m_iTrack)) {
		m_pButtonGroove->SetWindowText(_T("Groove"));
		if (Speed > MAX_GROOVE - 1) Speed = MAX_GROOVE - 1;
		if (Speed < 0) Speed = 0;
		pDoc->SetSongSpeed(m_iTrack, Speed);
	}
	else {
		m_pButtonGroove->SetWindowText(_T("Speed"));
		int MaxSpeed = pDoc->GetSongTempo(m_iTrack) ? pDoc->GetSpeedSplitPoint() - 1 : 0xFF;
		if (Speed > MaxSpeed) Speed = MaxSpeed;
		if (Speed < MIN_SPEED) Speed = MIN_SPEED;
		pDoc->SetSongSpeed(m_iTrack, Speed);
	}
}

void CMainFrame::OnToggleFixTempo()
{
	CFamiTrackerDoc *pDoc = (CFamiTrackerDoc*)GetActiveDocument();
	pDoc->SetSongTempo(m_iTrack, pDoc->GetSongTempo(m_iTrack) ? 0 : 150);
	GetActiveView()->SetFocus();
}

void CMainFrame::OnUpdateToggleFixTempo(CCmdUI *pCmdUI)
{
	CFamiTrackerDoc *pDoc = (CFamiTrackerDoc*)GetActiveDocument();
	int Tempo = pDoc->GetSongTempo(m_iTrack);
	if (Tempo) {
		m_pButtonFixTempo->SetWindowText(_T("Tempo"));
		m_pLockedEditTempo->EnableWindow(true);
		m_wndDialogBar.GetDlgItem(IDC_TEMPO_SPIN)->EnableWindow(true);
	}
	else {
		m_pButtonFixTempo->SetWindowText(_T("Fixed"));
		m_pLockedEditTempo->EnableWindow(false);
		m_wndDialogBar.GetDlgItem(IDC_TEMPO_SPIN)->EnableWindow(false);
		CString str;
		str.Format(_T("%.2f"), static_cast<float>(pDoc->GetFrameRate()) * 2.5);
		m_pLockedEditTempo->SetWindowText(str);
	}
}

void CMainFrame::OnEasterEggKraid1()			// Easter Egg
{
	if (m_iKraidCounter == 0) m_iKraidCounter++;
	else m_iKraidCounter = 0;
}

void CMainFrame::OnEasterEggKraid2()
{
	if (m_iKraidCounter == 1) m_iKraidCounter++;
	else m_iKraidCounter = 0;
}

void CMainFrame::OnEasterEggKraid3()
{
	if (m_iKraidCounter == 2) m_iKraidCounter++;
	else m_iKraidCounter = 0;
}

void CMainFrame::OnEasterEggKraid4()
{
	if (m_iKraidCounter == 3) m_iKraidCounter++;
	else m_iKraidCounter = 0;
}

void CMainFrame::OnEasterEggKraid5()
{
	if (m_iKraidCounter == 4) {
		if (AfxMessageBox(IDS_KRAID, MB_YESNO | MB_ICONWARNING | MB_DEFBUTTON2) == IDNO) {
			m_iKraidCounter = 0;
			return;}
		CFamiTrackerDoc *pDoc = (CFamiTrackerDoc*)GetActiveDocument();
		pDoc->MakeKraid();
		SelectTrack(0);
		SetSongInfo(_T(""), _T(""), _T(""));
		UpdateControls();
		UpdateInstrumentList();
		UpdateTrackBox();
		ResetUndo();
		ResizeFrameWindow();
		SetStatusText(_T("Famitracker - Metroid - Kraid's Lair (Uploaded on Jun 9, 2010 http://www.youtube.com/watch?v=9yzCLy-fZVs) The FTM straight from the tutorial. - 8BitDanooct1"));
	}
	m_iKraidCounter = 0;
}

void CMainFrame::OnFindNext()
{
	m_pFindDlg->OnBnClickedButtonFindNext();
}

void CMainFrame::OnFindPrevious()
{
	m_pFindDlg->OnBnClickedButtonFindPrevious();
}

void CMainFrame::OnEditFindToggle()
{
	if (m_wndFindControlBar.IsWindowVisible())
		m_wndFindControlBar.ShowWindow(SW_HIDE);
	else {
		m_wndFindControlBar.ShowWindow(SW_SHOW);
		m_wndFindControlBar.Invalidate();
		m_wndFindControlBar.RedrawWindow();
	}
	ResizeFrameWindow();
}

void CMainFrame::OnUpdateEditFindToggle(CCmdUI *pCmdUI)
{
	pCmdUI->SetCheck(m_wndFindControlBar.IsWindowVisible());
}

void CMainFrame::ResetFind()
{
	m_pFindDlg->Reset();
}

void CMainFrame::OnEditRemoveUnusedSamples()
{
	CFamiTrackerDoc *pDoc = static_cast<CFamiTrackerDoc*>(GetActiveDocument());

	if (AfxMessageBox(IDS_REMOVE_SAMPLES, MB_YESNO | MB_ICONWARNING | MB_DEFBUTTON2) == IDNO)
		return;

	CloseInstrumentEditor();
	pDoc->RemoveUnusedSamples();
	ResetUndo();		// // //
	pDoc->UpdateAllViews(NULL, UPDATE_PATTERN);
}

void CMainFrame::OnEditPopulateUniquePatterns()		// // //
{
	CFamiTrackerDoc *pDoc = static_cast<CFamiTrackerDoc*>(GetActiveDocument());

	if (AfxMessageBox(IDS_POPULATE_PATTERNS, MB_YESNO | MB_ICONWARNING | MB_DEFBUTTON2) == IDNO)
		return;

	pDoc->PopulateUniquePatterns(m_iTrack);
	ResetUndo();
	pDoc->UpdateAllViews(NULL, UPDATE_FRAME);
}

void CMainFrame::OnEditGoto()
{
	CGotoDlg gotoDlg;
	gotoDlg.DoModal();
}

void CMainFrame::OnEditSwapChannels()
{
	CSwapDlg swapDlg;
	swapDlg.SetTrack(GetSelectedTrack());
	if (swapDlg.DoModal() == IDOK)
		ResetUndo();
}

// // // Moved from CFamiTrackerView

void CMainFrame::OnTrackerPal()
{
	CFamiTrackerDoc *pDoc = static_cast<CFamiTrackerDoc*>(GetActiveDocument());
	ASSERT_VALID(pDoc);

	machine_t Machine = PAL;
	pDoc->SetMachine(Machine);
	theApp.GetSoundGenerator()->LoadMachineSettings();		// // //
	theApp.GetSoundGenerator()->DocumentPropertiesChanged(pDoc);		// // //
	m_wndInstEdit.SetRefreshRate(static_cast<float>(pDoc->GetFrameRate()));		// // //
}

void CMainFrame::OnTrackerNtsc()
{
	CFamiTrackerDoc *pDoc = static_cast<CFamiTrackerDoc*>(GetActiveDocument());
	ASSERT_VALID(pDoc);

	machine_t Machine = NTSC;
	pDoc->SetMachine(Machine);
	theApp.GetSoundGenerator()->LoadMachineSettings();		// // //
	theApp.GetSoundGenerator()->DocumentPropertiesChanged(pDoc);		// // //
	m_wndInstEdit.SetRefreshRate(static_cast<float>(pDoc->GetFrameRate()));		// // //
}

void CMainFrame::OnSpeedDefault()
{
	CFamiTrackerDoc *pDoc = static_cast<CFamiTrackerDoc*>(GetActiveDocument());
	ASSERT_VALID(pDoc);

	int Speed = 0;
	pDoc->SetEngineSpeed(Speed);
	theApp.GetSoundGenerator()->LoadMachineSettings();		// // //
	m_wndInstEdit.SetRefreshRate(static_cast<float>(pDoc->GetFrameRate()));		// // //
}

void CMainFrame::OnSpeedCustom()
{
	CFamiTrackerDoc *pDoc = static_cast<CFamiTrackerDoc*>(GetActiveDocument());
	ASSERT_VALID(pDoc);

	CSpeedDlg SpeedDlg;

	machine_t Machine = pDoc->GetMachine();
	int Speed = pDoc->GetEngineSpeed();
	if (Speed == 0)
		Speed = (Machine == NTSC) ? CAPU::FRAME_RATE_NTSC : CAPU::FRAME_RATE_PAL;
	Speed = SpeedDlg.GetSpeedFromDlg(Speed);

	if (Speed == 0)
		return;

	pDoc->SetEngineSpeed(Speed);
	theApp.GetSoundGenerator()->LoadMachineSettings();		// // //
	m_wndInstEdit.SetRefreshRate(static_cast<float>(pDoc->GetFrameRate()));		// // //
}

void CMainFrame::OnUpdateTrackerPal(CCmdUI *pCmdUI)
{
	CFamiTrackerDoc *pDoc = static_cast<CFamiTrackerDoc*>(GetActiveDocument());
	ASSERT_VALID(pDoc);

	pCmdUI->Enable(pDoc->GetExpansionChip() == SNDCHIP_NONE && !theApp.IsPlaying());		// // //
	UINT item = pDoc->GetMachine() == PAL ? ID_TRACKER_PAL : ID_TRACKER_NTSC;
	if (pCmdUI->m_pMenu != NULL)
		pCmdUI->m_pMenu->CheckMenuRadioItem(ID_TRACKER_NTSC, ID_TRACKER_PAL, item, MF_BYCOMMAND);
}

void CMainFrame::OnUpdateTrackerNtsc(CCmdUI *pCmdUI)
{
	CFamiTrackerDoc *pDoc = static_cast<CFamiTrackerDoc*>(GetActiveDocument());
	ASSERT_VALID(pDoc);

	pCmdUI->Enable(!theApp.IsPlaying());		// // //
	UINT item = pDoc->GetMachine() == NTSC ? ID_TRACKER_NTSC : ID_TRACKER_PAL;
	if (pCmdUI->m_pMenu != NULL)
		pCmdUI->m_pMenu->CheckMenuRadioItem(ID_TRACKER_NTSC, ID_TRACKER_PAL, item, MF_BYCOMMAND);
}

void CMainFrame::OnUpdateSpeedDefault(CCmdUI *pCmdUI)
{
	CFamiTrackerDoc *pDoc = static_cast<CFamiTrackerDoc*>(GetActiveDocument());
	ASSERT_VALID(pDoc);

	pCmdUI->Enable(!theApp.IsPlaying());		// // //
	pCmdUI->SetCheck(pDoc->GetEngineSpeed() == 0);
}

void CMainFrame::OnUpdateSpeedCustom(CCmdUI *pCmdUI)
{
	CFamiTrackerDoc *pDoc = static_cast<CFamiTrackerDoc*>(GetActiveDocument());
	ASSERT_VALID(pDoc);

	pCmdUI->Enable(!theApp.IsPlaying());		// // //
	pCmdUI->SetCheck(pDoc->GetEngineSpeed() != 0);
}
