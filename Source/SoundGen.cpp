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

//
// This file takes care of the NES sound playback
//
// TODO:
//  - Break out actual player functions to a new class CPlayer
//  - Create new interface for CFamiTrackerView with thread-safe functions
//  - Same for CFamiTrackerDoc
//  - Perhaps this should be a worker thread and not GUI thread?
//

#include "stdafx.h"
#include "FamiTracker.h"
#include "FamiTrackerTypes.h"
#include "FTMComponentInterface.h"		// // //
#include "ChannelState.h"		// // //
#include "FamiTrackerDoc.h"
#include "FamiTrackerView.h"
#include "VisualizerWnd.h"
#include "MainFrm.h"
#include "SoundInterface.h"
#include "WaveFile.h"		// // //
#include "APU/APU.h"
#include "ChannelHandler.h"
#include "ChannelsN163.h" // N163 channel count
#include "DSample.h"		// // //
#include "SoundGen.h"
#include "InstrumentRecorder.h"		// // //
#include "Settings.h"
#include "TrackerChannel.h"
#include "MIDI.h"
#include "ChannelFactory.h"		// // // test
#include "DetuneTable.h"		// // //
#include <iostream>

// 1kHz test tone
//#define AUDIO_TEST

// Write period tables to files
//#define WRITE_PERIOD_FILES

// // // Write vibrato table to file
//#define WRITE_VIBRATO_FILE

// Write a file with the volume table
//#define WRITE_VOLUME_FILE

// // // Log VGM output (experimental)
//#define WRITE_VGM

// Enable audio dithering
//#define DITHERING

// The depth of each vibrato level
const double CSoundGen::NEW_VIBRATO_DEPTH[] = {
	1.0, 1.5, 2.5, 4.0, 5.0, 7.0, 10.0, 12.0, 14.0, 17.0, 22.0, 30.0, 44.0, 64.0, 96.0, 128.0
};

const double CSoundGen::OLD_VIBRATO_DEPTH[] = {
	1.0, 1.0, 2.0, 3.0, 4.0, 7.0, 8.0, 15.0, 16.0, 31.0, 32.0, 63.0, 64.0, 127.0, 128.0, 255.0
};

bool CSoundGen::DispatchGuiMessage(GuiMessage msg) {
	switch (msg.message) {
	#define ON_SPSC_MESSAGE(message, memberFxn) \
		case message: memberFxn(msg.wParam, msg.lParam); return true;

	ON_SPSC_MESSAGE(WM_USER_SILENT_ALL, OnSilentAll)
	ON_SPSC_MESSAGE(WM_USER_LOAD_SETTINGS, OnLoadSettings)
	ON_SPSC_MESSAGE(WM_USER_PLAY, OnStartPlayer)
	ON_SPSC_MESSAGE(WM_USER_STOP, OnStopPlayer)
	ON_SPSC_MESSAGE(WM_USER_RESET, OnResetPlayer)
	ON_SPSC_MESSAGE(WM_USER_START_RENDER, OnStartRender)
	ON_SPSC_MESSAGE(WM_USER_STOP_RENDER, OnStopRender)
	ON_SPSC_MESSAGE(WM_USER_PREVIEW_SAMPLE, OnPreviewSample)
	ON_SPSC_MESSAGE(WM_USER_WRITE_APU, OnWriteAPU)
	ON_SPSC_MESSAGE(WM_USER_CLOSE_SOUND, OnCloseSound)
	ON_SPSC_MESSAGE(WM_USER_SET_CHIP, OnSetChip)
	ON_SPSC_MESSAGE(WM_USER_REMOVE_DOCUMENT, OnRemoveDocument)

	default:
		TRACE("Error: unrecognized SPSC message %d sent to audio thread", msg.message);
		ASSERT(false);
		return true;
	}
}

#ifdef DITHERING
int dither(long size);
#endif

// CSoundGen

// According to https://docs.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-postthreadmessagea,
// the default window message limit is 10000. Let's use 8192 for our replacement queue.
static constexpr size_t MESSAGE_QUEUE_SIZE = 8192;

CSoundGen::CSoundGen() :
	m_MessageQueue(MESSAGE_QUEUE_SIZE),
	m_pAPU(NULL),
	m_pSoundInterface(NULL),
	m_pSoundStream(NULL),
	m_pAccumBuffer(NULL),
	m_iGraphBuffer(NULL),
	m_pDocument(NULL),
	m_pTrackerView(NULL),
	m_bRequestRenderStart(false),
	m_bRendering(false),
	m_bPlaying(false),
	m_bHaltRequest(false),
	m_bDoHalt(false),		// // //
	m_pPreviewSample(NULL),
	m_pVisualizerWnd(NULL),
	m_iSpeed(0),
	m_iTempo(0),
	m_iGrooveIndex(-1),		// // //
	m_iGroovePosition(0),		// // //
	m_pInstRecorder(new CInstrumentRecorder(this)),		// // //
	m_bWaveChanged(0),
	m_iMachineType(NTSC),
	m_CoInitialized(false),
	m_bRunning(false),
	m_hInterruptEvent(NULL),
	m_bBufferTimeout(false),
	m_bDirty(false),
	m_iQueuedFrame(-1),
	m_iPlayFrame(0),
	m_iPlayRow(0),
	m_iPlayTrack(0),
	m_iPlayTicks(0),
	m_iConsumedCycles(0),
	m_iRegisterStream(),		// // //
	m_bBufferUnderrun(false),
	m_bAudioClipping(false),
	m_iClipCounter(0),
	m_pSequencePlayPos(NULL),
	m_iSequencePlayPos(0),
	m_iSequenceTimeout(0),
	m_iBPMCachePosition(0),		// // //
	currN163LevelOffset(0)
{
	TRACE("SoundGen: Object created\n");

	// Create APU
	m_pAPU = new CAPU(this);		// // //

	// Create all kinds of channels
	CreateChannels();
}

CSoundGen::~CSoundGen()
{
	// Delete APU
	SAFE_RELEASE(m_pAPU);

	// Remove channels
	for (int i = 0; i < CHANNELS; ++i) {
		SAFE_RELEASE(m_pChannels[i]);
		SAFE_RELEASE(m_pTrackerChannels[i]);
	}

	SAFE_RELEASE(m_pInstRecorder);		// // //
}

//
// Object initialization, local
//

void CSoundGen::CreateChannels()
{
	// Only called once!

	// Clear all channels
	for (int i = 0; i < CHANNELS; ++i) {
		m_pChannels[i] = NULL;
		m_pTrackerChannels[i] = NULL;
	}

	// 2A03/2A07
	// // // Short header names
#ifdef _DUAL_CH		// // //
	CSquare1Chan *PU1 = new C2A03Square();
	AssignChannel(new CTrackerChannel(_T("Pulse 1"), _T("PU1"), SNDCHIP_NONE, CHANID_SQUARE1));
	AssignChannel(new CTrackerChannel(_T("Pulse 1 SFX"), _T("PU1*"), SNDCHIP_NONE, CHANID_SQUARE1));
#else
	AssignChannel(new CTrackerChannel(_T("Pulse 1"), _T("PU1"), SNDCHIP_NONE, CHANID_SQUARE1));
	AssignChannel(new CTrackerChannel(_T("Pulse 2"), _T("PU2"), SNDCHIP_NONE, CHANID_SQUARE2));
#endif
	AssignChannel(new CTrackerChannel(_T("Triangle"), _T("TRI"), SNDCHIP_NONE, CHANID_TRIANGLE));
	AssignChannel(new CTrackerChannel(_T("Noise"), _T("NOI"), SNDCHIP_NONE, CHANID_NOISE));
	AssignChannel(new CTrackerChannel(_T("DPCM"), _T("DMC"), SNDCHIP_NONE, CHANID_DPCM));

	// Konami VRC6
	AssignChannel(new CTrackerChannel(_T("VRC6 Pulse 1"), _T("V1"), SNDCHIP_VRC6, CHANID_VRC6_PULSE1));
	AssignChannel(new CTrackerChannel(_T("VRC6 Pulse 2"), _T("V2"), SNDCHIP_VRC6, CHANID_VRC6_PULSE2));
	AssignChannel(new CTrackerChannel(_T("Sawtooth"), _T("SAW"), SNDCHIP_VRC6, CHANID_VRC6_SAWTOOTH));

	// // // Nintendo MMC5
	AssignChannel(new CTrackerChannel(_T("MMC5 Pulse 1"), _T("PU3"), SNDCHIP_MMC5, CHANID_MMC5_SQUARE1));
	AssignChannel(new CTrackerChannel(_T("MMC5 Pulse 2"), _T("PU4"), SNDCHIP_MMC5, CHANID_MMC5_SQUARE2));
	AssignChannel(new CTrackerChannel(_T("MMC5 PCM"), _T("PCM"), SNDCHIP_MMC5, CHANID_MMC5_VOICE)); // null channel handler

	// Namco N163
	AssignChannel(new CTrackerChannel(_T("Namco 1"), _T("N1"), SNDCHIP_N163, CHANID_N163_CH1));
	AssignChannel(new CTrackerChannel(_T("Namco 2"), _T("N2"), SNDCHIP_N163, CHANID_N163_CH2));
	AssignChannel(new CTrackerChannel(_T("Namco 3"), _T("N3"), SNDCHIP_N163, CHANID_N163_CH3));
	AssignChannel(new CTrackerChannel(_T("Namco 4"), _T("N4"), SNDCHIP_N163, CHANID_N163_CH4));
	AssignChannel(new CTrackerChannel(_T("Namco 5"), _T("N5"), SNDCHIP_N163, CHANID_N163_CH5));
	AssignChannel(new CTrackerChannel(_T("Namco 6"), _T("N6"), SNDCHIP_N163, CHANID_N163_CH6));
	AssignChannel(new CTrackerChannel(_T("Namco 7"), _T("N7"), SNDCHIP_N163, CHANID_N163_CH7));
	AssignChannel(new CTrackerChannel(_T("Namco 8"), _T("N8"), SNDCHIP_N163, CHANID_N163_CH8));

	// Nintendo FDS
	AssignChannel(new CTrackerChannel(_T("FDS"), _T("FDS"), SNDCHIP_FDS, CHANID_FDS));

	// Konami VRC7
	AssignChannel(new CTrackerChannel(_T("FM Channel 1"), _T("FM1"), SNDCHIP_VRC7, CHANID_VRC7_CH1));
	AssignChannel(new CTrackerChannel(_T("FM Channel 2"), _T("FM2"), SNDCHIP_VRC7, CHANID_VRC7_CH2));
	AssignChannel(new CTrackerChannel(_T("FM Channel 3"), _T("FM3"), SNDCHIP_VRC7, CHANID_VRC7_CH3));
	AssignChannel(new CTrackerChannel(_T("FM Channel 4"), _T("FM4"), SNDCHIP_VRC7, CHANID_VRC7_CH4));
	AssignChannel(new CTrackerChannel(_T("FM Channel 5"), _T("FM5"), SNDCHIP_VRC7, CHANID_VRC7_CH5));
	AssignChannel(new CTrackerChannel(_T("FM Channel 6"), _T("FM6"), SNDCHIP_VRC7, CHANID_VRC7_CH6));

	// // // Sunsoft 5B
	AssignChannel(new CTrackerChannel(_T("5B Square 1"), _T("5B1"), SNDCHIP_S5B, CHANID_S5B_CH1));
	AssignChannel(new CTrackerChannel(_T("5B Square 2"), _T("5B2"), SNDCHIP_S5B, CHANID_S5B_CH2));
	AssignChannel(new CTrackerChannel(_T("5B Square 3"), _T("5B3"), SNDCHIP_S5B, CHANID_S5B_CH3));
}

void CSoundGen::AssignChannel(CTrackerChannel *pTrackerChannel)		// // //
{
	static CChannelFactory F {}; // test
	chan_id_t ID = pTrackerChannel->GetID();

	CChannelHandler *pRenderer = F.Produce(ID);
	if (pRenderer)
		pRenderer->SetChannelID(ID);

	static size_t Pos = 0;		// // // test
	m_pTrackerChannels[Pos] = pTrackerChannel;
	m_pChannels[Pos++] = pRenderer;
}

//
// Object initialization, global
//

void CSoundGen::AssignDocument(CFamiTrackerDoc *pDoc)
{
	// Called from main thread
	ASSERT(GetCurrentThreadId() == theApp.m_nThreadID);

	// Ignore all but the first document (as new documents are used to import files)
	if (m_pDocument != NULL)
		return;

	// Assigns a document to this object
	m_pDocument = pDoc;
	m_pInstRecorder->m_pDocument = pDoc;		// // //

	// Setup all channels
	for (int i = 0; i < CHANNELS; ++i) {
		if (m_pChannels[i])
			m_pChannels[i]->InitChannel(m_pAPU, m_iVibratoTable, this);
	}
	DocumentPropertiesChanged(pDoc);		// // //
}

void CSoundGen::AssignView(CFamiTrackerView *pView)
{
	// Called from main thread
	ASSERT(GetCurrentThreadId() == theApp.m_nThreadID);

	if (m_pTrackerView != NULL)
		return;

	// Assigns the tracker view to this object
	m_pTrackerView = pView;
}

void CSoundGen::RemoveDocument()
{
	// Removes both the document and view from this object

	// Called from main thread
	ASSERT(GetCurrentThreadId() == theApp.m_nThreadID);
	ASSERT(m_pDocument != NULL);
	ASSERT(m_audioThread.joinable());

	// Player cannot play when removing the document
	StopPlayer();
	WaitForStop();

	PostGuiMessage(WM_USER_REMOVE_DOCUMENT, 0, 0);

	// Wait 5s for thread to clear the pointer
	for (int i = 0; i < 50 && m_pDocument != NULL; ++i)
		Sleep(100);

	if (m_pDocument != NULL) {
		// Thread stuck
		TRACE("SoundGen: Could not remove document pointer!\n");
	}
}

void CSoundGen::SetVisualizerWindow(CVisualizerWnd *pWnd)
{
	// Called from main thread
	ASSERT(GetCurrentThreadId() == theApp.m_nThreadID);

	{
		std::unique_lock<std::mutex> lock(m_csVisualizerWndLock);
		m_pVisualizerWnd = pWnd;
	}
}

void CSoundGen::RegisterChannels(int Chip, CFamiTrackerDoc *pDoc)
{
	// This method will add channels to the document object, depending on the expansion chip used.
	// Called from the document object (from the main thread)

	// Called from main thread
	ASSERT(GetCurrentThreadId() == theApp.m_nThreadID);

	// This affects the sound channel interface so it must be synchronized
	pDoc->LockDocument();

	// Clear all registered channels
	pDoc->ResetChannels();

	// Register the channels in the document
	// Expansion & internal channels
	for (int i = 0; i < sizeof(m_pTrackerChannels) / sizeof(CTrackerChannel*); ++i) {
		int ID = m_pTrackerChannels[i]->GetID();		// // //
		if (m_pChannels[i] && ((m_pTrackerChannels[i]->GetChip() & Chip) || (i < 5))			// // //
						   && (i >= CHANID_FDS || i < CHANID_N163_CH1 + pDoc->GetNamcoChannels())) {
			pDoc->RegisterChannel(m_pTrackerChannels[i], ID, m_pTrackerChannels[i]->GetChip());
		}
	}

	pDoc->UnlockDocument();
}

void CSoundGen::SelectChip(int Chip)
{
	if (IsPlaying()) {
		StopPlayer();
	}

	if (!WaitForStop()) {
		TRACE("CSoundGen: Could not stop player!");
		return;
	}

	PostGuiMessage(WM_USER_SET_CHIP, Chip, 0);
}

CChannelHandler *CSoundGen::GetChannel(int Index) const
{
	return m_pChannels[Index];
}

/*
INVARIANT: called whenever any document is created or changes.
	CREATION:
	CreateEmpty() calls DocumentPropertiesChanged.
	OpenDocument() calls DocumentPropertiesChanged.

PRECONDITION: pDocument is not null.
PROPERTY: if pDocument is main document, linear pitch is synced.
RESULT: linear pitch is correct.
*/
void CSoundGen::DocumentPropertiesChanged(CFamiTrackerDoc *pDocument)
{
	if (pDocument != m_pDocument)
		return;
	ASSERT(pDocument != NULL);

	SetupVibratoTable(pDocument->GetVibratoStyle());		// // //

	machine_t Machine = pDocument->GetMachine();
	const double A440_NOTE = 45. - pDocument->GetTuningSemitone() - pDocument->GetTuningCent() / 100.;
	double clock_ntsc = CAPU::BASE_FREQ_NTSC / 16.0;
	double clock_pal = CAPU::BASE_FREQ_PAL / 16.0;

	for (int i = 0; i < NOTE_COUNT; ++i) {
		// Frequency (in Hz)
		double Freq = 440. * pow(2.0, double(i - A440_NOTE) / 12.);
		double Pitch;

		// 2A07
		Pitch = (clock_pal / Freq) - 0.5;
		m_iNoteLookupTablePAL[i] = (unsigned int)(Pitch - pDocument->GetDetuneOffset(1, i));		// // //

		// 2A03 / MMC5 / VRC6
		Pitch = (clock_ntsc / Freq) - 0.5;
		m_iNoteLookupTableNTSC[i] = (unsigned int)(Pitch - pDocument->GetDetuneOffset(0, i));		// // //
		m_iNoteLookupTableS5B[i] = m_iNoteLookupTableNTSC[i] + 1;		// correction

		// VRC6 Saw
		Pitch = ((clock_ntsc * 16.0) / (Freq * 14.0)) - 0.5;
		m_iNoteLookupTableSaw[i] = (unsigned int)(Pitch - pDocument->GetDetuneOffset(2, i));		// // //

		// FDS
#ifdef TRANSPOSE_FDS
		Pitch = (Freq * 65536.0) / (clock_ntsc / 1.0) + 0.5;
#else
		Pitch = (Freq * 65536.0) / (clock_ntsc / 4.0) + 0.5;
#endif
		m_iNoteLookupTableFDS[i] = (unsigned int)(Pitch + pDocument->GetDetuneOffset(4, i));		// // //

		// N163
		Pitch = ((Freq * pDocument->GetNamcoChannels() * 983040.0) / clock_ntsc + 0.5) / 4;		// // //
		m_iNoteLookupTableN163[i] = (unsigned int)(Pitch + pDocument->GetDetuneOffset(5, i));		// // //

		if (m_iNoteLookupTableN163[i] > 0xFFFF)	// 0x3FFFF
			m_iNoteLookupTableN163[i] = 0xFFFF;	// 0x3FFFF

		// // // Sunsoft 5B uses NTSC table

		// // // VRC7
		if (i < NOTE_RANGE) {
			Pitch = Freq * 262144.0 / 49716.0 + 0.5;
			m_iNoteLookupTableVRC7[i] = (unsigned int)(Pitch + pDocument->GetDetuneOffset(3, i));		// // //
		}
	}

	// // // Setup note tables
	for (int i = 0; i < CHANNELS; ++i) {
		if (!m_pChannels[i]) continue;
		const unsigned int *Table = nullptr;
		switch (m_pTrackerChannels[i]->GetID()) {
		case CHANID_SQUARE1: case CHANID_SQUARE2: case CHANID_TRIANGLE:
			Table = Machine == PAL ? m_iNoteLookupTablePAL : m_iNoteLookupTableNTSC; break;
		case CHANID_VRC6_PULSE1: case CHANID_VRC6_PULSE2:
		case CHANID_MMC5_SQUARE1: case CHANID_MMC5_SQUARE2:
			Table = m_iNoteLookupTableNTSC; break;
		case CHANID_VRC6_SAWTOOTH:
			Table = m_iNoteLookupTableSaw; break;
		case CHANID_VRC7_CH1: case CHANID_VRC7_CH2: case CHANID_VRC7_CH3:
		case CHANID_VRC7_CH4: case CHANID_VRC7_CH5: case CHANID_VRC7_CH6:
			Table = m_iNoteLookupTableVRC7; break;
		case CHANID_FDS:
			Table = m_iNoteLookupTableFDS; break;
		case CHANID_N163_CH1: case CHANID_N163_CH2: case CHANID_N163_CH3: case CHANID_N163_CH4:
		case CHANID_N163_CH5: case CHANID_N163_CH6: case CHANID_N163_CH7: case CHANID_N163_CH8:
			Table = m_iNoteLookupTableN163; break;
		case CHANID_S5B_CH1: case CHANID_S5B_CH2: case CHANID_S5B_CH3:
			Table = m_iNoteLookupTableS5B; break;
		default: continue;
		}
		m_pChannels[i]->SetNoteTable(Table);
	}

#ifdef WRITE_PERIOD_FILES

	// Write periods to a single file with assembly formatting
	CStdioFile period_file("..\\nsf driver\\periods.s", CStdioFile::modeWrite | CStdioFile::modeCreate);

	const auto DumpFunc = [&period_file] (const unsigned int *Table) {
		for (int i = 0; i < NOTE_COUNT; ++i) {
			unsigned int Val = Table[i] & 0xFFFF;
			CString str;
			if (i < NOTE_COUNT - 1) {
				if (i % NOTE_RANGE < NOTE_RANGE - 1)
					str.Format("$%04X, ", Val);
				else
					str.Format("$%04X\n\t.word\t", Val);
			}
			else
				str.Format("$%04X\n", Val);
			period_file.WriteString(str);
		}
	};

	// One possible optimization is to store the PAL table as the difference from the NTSC table

	period_file.WriteString("; 2A03 NTSC\n");
	period_file.WriteString(".if .defined(NTSC_PERIOD_TABLE)\n");
	period_file.WriteString("ft_periods_ntsc: ;; Patch\n\t.word\t");
	DumpFunc(m_iNoteLookupTableNTSC);

	period_file.WriteString(".endif\n\n");
	period_file.WriteString("; 2A03 PAL\n");
	period_file.WriteString(".if .defined(PAL_PERIOD_TABLE)\n");
	period_file.WriteString("ft_periods_pal: ;; Patch\n\t.word\t");
	DumpFunc(m_iNoteLookupTablePAL);

	period_file.WriteString(".endif\n\n");
	period_file.WriteString("; VRC6 Sawtooth\n");
	period_file.WriteString(".if .defined(USE_VRC6)\n");
	period_file.WriteString("ft_periods_sawtooth: ;; Patch\n\t.word\t");
	DumpFunc(m_iNoteLookupTableSaw);

	period_file.WriteString(".endif\n\n");
	period_file.WriteString("; FDS\n");
	period_file.WriteString(".if .defined(USE_FDS)\n");
	period_file.WriteString("ft_periods_fds: ;; Patch\n\t.word\t");
	DumpFunc(m_iNoteLookupTableFDS);

	period_file.WriteString(".endif\n\n");
	period_file.WriteString("; N163\n");
	period_file.WriteString(".if .defined(USE_N163)\n");
	period_file.WriteString("ft_periods_n163: ;; Patch\n\t.word\t");
	DumpFunc(m_iNoteLookupTableN163);

	period_file.WriteString(".endif\n\n");
	period_file.WriteString("; VRC7\n");
	period_file.WriteString(".if .defined(USE_VRC7)\n");
	period_file.WriteString("; Fnum table, multiplied by 4 for higher resolution\n");
	period_file.WriteString(".define ft_vrc7_table ");

	for (int i = 0; i <= NOTE_RANGE; ++i) {		// // // include last item for linear pitch code optimization
		CString str;
		if (i == NOTE_RANGE)
			str.Format("$%04X\n\n", m_iNoteLookupTableVRC7[0] << 3);
		else
			str.Format("$%04X, ", m_iNoteLookupTableVRC7[i] << 2);
		period_file.WriteString(str);
	}

	period_file.WriteString("ft_note_table_vrc7_l: ;; Patch\n");
	period_file.WriteString("\t.lobytes ft_vrc7_table\n");
	period_file.WriteString("ft_note_table_vrc7_h:\n");
	period_file.WriteString("\t.hibytes ft_vrc7_table\n");
	period_file.WriteString(".endif\n");

	period_file.Close();

#endif

	for (int i = 0; i < CHANNELS; ++i) {
		if (m_pChannels[i]) {
			m_pChannels[i]->SetVibratoStyle(pDocument->GetVibratoStyle());		// // //
			m_pChannels[i]->SetLinearPitch(pDocument->GetLinearPitch());
		}
		if (auto pChan = dynamic_cast<CChannelHandlerN163*>(m_pChannels[i]))
			pChan->SetChannelCount(pDocument->GetNamcoChannels());
	}

	m_iSpeedSplitPoint = pDocument->GetSpeedSplitPoint();

	if (currN163LevelOffset != pDocument->GetN163LevelOffset()) {
		// Player thread calls OnLoadSettings() which calls ResetAudioDevice()
		// Why are GetCurrentThreadId and GetCurrentThread used interchangably?
		LoadSettings();
	}
}

//
// Interface functions
//

void CSoundGen::StartPlayer(play_mode_t Mode, int Track)
{
	if (!m_audioThread.joinable())
		return;

	PostGuiMessage(WM_USER_PLAY, Mode, Track);
}

void CSoundGen::StopPlayer()
{
	if (!m_audioThread.joinable())
		return;

	PostGuiMessage(WM_USER_STOP, 0, 0);
}

void CSoundGen::ResetPlayer(int Track)
{
	if (!m_audioThread.joinable())
		return;

	PostGuiMessage(WM_USER_RESET, Track, 0);
}

void CSoundGen::LoadSettings()
{
	if (!m_audioThread.joinable())
		return;

	PostGuiMessage(WM_USER_LOAD_SETTINGS, 0, 0);
}

void CSoundGen::SilentAll()
{
	if (!m_audioThread.joinable())
		return;

	PostGuiMessage(WM_USER_SILENT_ALL, 0, 0);
}

void CSoundGen::WriteAPU(int Address, char Value)
{
	if (!m_audioThread.joinable())
		return;

	// Direct APU interface
	PostGuiMessage(WM_USER_WRITE_APU, (WPARAM)Address, (LPARAM)Value);
}

void CSoundGen::PreviewSample(const CDSample *pSample, int Offset, int Pitch)		// // //
{
	if (!m_audioThread.joinable())
		return;

	// Preview a DPCM sample. If the name of sample is null,
	// the sample will be removed after played
	PostGuiMessage(WM_USER_PREVIEW_SAMPLE, (WPARAM)pSample, MAKELPARAM(Offset, Pitch));
}

void CSoundGen::CancelPreviewSample()
{
	// Remove references to selected sample.
	// This must be done if a sample is about to be deleted!
	m_pAPU->ClearSample();		// // //
}

bool CSoundGen::IsRunning() const
{
	return (m_audioThread.joinable()) && m_bRunning;
}

//// Sound buffer handling /////////////////////////////////////////////////////////////////////////////////

bool CSoundGen::BeginThread()
{
	// Initialize sound, this is only called once!
	// Start with NTSC by default

	// Called from main thread
	ASSERT(GetCurrentThread() == theApp.m_hThread);
	ASSERT(m_pSoundInterface == NULL);

	// Make sure we only start one thread
	ASSERT(!m_audioThread.joinable());

	// Event used to interrupt the sound buffer synchronization
	m_hInterruptEvent = ::CreateEvent(NULL, FALSE, FALSE, NULL);

	// Create sound interface object
	m_pSoundInterface = new CSoundInterface(m_hInterruptEvent);

	// Out of memory
	if (!m_pSoundInterface)
		return false;

	m_pSoundInterface->EnumerateDevices();

	// Start thread when audio is done
	m_audioThread = std::thread([this]() {
		ThreadEntry();
	});

	return true;
}

void CSoundGen::ThreadEntry()
{
	m_audioThreadID = std::this_thread::get_id();

	if (!InitInstance()) {
		ExitInstance();
		return;
	}
	while (true) {
		while (auto pMessage = m_MessageQueue.front()) {
			m_MessageQueue.pop();
			if (pMessage->message == WM_QUIT) {
				goto end_while;
			}
			if (!DispatchGuiMessage(*pMessage)) {
				goto end_while;
			}
		}

		// Blocks on audio output.
		OnIdle();
	}
	end_while:

	ExitInstance();
	// lolmfc
	delete this;
}


bool CSoundGen::PostGuiMessage(GuiMessageId message, WPARAM wParam, LPARAM lParam)
{
	return m_MessageQueue.try_push(GuiMessage{
		message,
		wParam,
		lParam,
	});
}

void CSoundGen::Interrupt() const
{
	if (m_hInterruptEvent != NULL)
		::SetEvent(m_hInterruptEvent);
}

bool CSoundGen::GetSoundTimeout() const
{
	// Read without reset
	return m_bBufferTimeout;
}

bool CSoundGen::IsBufferUnderrun()
{
	// Read and reset flag
	bool ret(m_bBufferUnderrun);
	m_bBufferUnderrun = false;
	return ret;
}

bool CSoundGen::IsAudioClipping()
{
	// Read and reset flag
	bool ret(m_bAudioClipping);
	m_bAudioClipping = false;
	return ret;
}

bool CSoundGen::ResetAudioDevice()
{
	// Setup sound, return false if failed
	//
	// The application must be able to continue even if this fails
	//

	// Called from player thread
	ASSERT(std::this_thread::get_id() == m_audioThreadID);
	ASSERT(m_pSoundInterface != NULL);

	CSettings *pSettings = theApp.GetSettings();

	unsigned int SampleSize = pSettings->Sound.iSampleSize;
	unsigned int SampleRate = pSettings->Sound.iSampleRate;
	unsigned int BufferLen	= pSettings->Sound.iBufferLength;
	unsigned int Device		= pSettings->Sound.iDevice;

	auto l = Lock();

	m_iSampleSize = SampleSize;
	m_iAudioUnderruns = 0;
	m_iBufferPtr = 0;

	// Close the old sound channel
	CloseAudioDevice();

	if (Device >= m_pSoundInterface->GetDeviceCount()) {
		// Invalid device detected, reset to 0
		Device = 0;
		pSettings->Sound.iDevice = 0;
	}

	// Reinitialize sound interface
	if (!m_pSoundInterface->SetupDevice(Device)) {
		m_pTrackerView->PostMessage(WM_USER_ERROR, IDS_SOUND_ERROR, MB_ICONERROR);
		return false;
	}

	int iBlocks = 2;	// default = 2

	// Create more blocks if a bigger buffer than 100ms is used to reduce lag
	if (BufferLen > 100)
		iBlocks += (BufferLen / 66);

	// Create channel
	m_pSoundStream = m_pSoundInterface->OpenChannel(SampleRate, SampleSize, 1, BufferLen, iBlocks);

	// Channel failed
	if (m_pSoundStream == NULL) {
		m_pTrackerView->PostMessage(WM_USER_ERROR, IDS_SOUND_BUFFER_ERROR, MB_ICONERROR);
		return false;
	}

	// Create a buffer
	m_iBufSizeBytes	  = m_pSoundStream->TotalBufferSizeBytes();
	m_iBufSizeSamples = m_pSoundStream->TotalBufferSizeFrames();

	// Temp. audio buffer
	SAFE_RELEASE_ARRAY(m_pAccumBuffer);
	m_pAccumBuffer = new char[m_iBufSizeBytes];

	// Sample graph buffer
	SAFE_RELEASE_ARRAY(m_iGraphBuffer);
	m_iGraphBuffer = new short[m_iBufSizeSamples];

	// Sample graph rate
	{
		std::unique_lock<std::mutex> lock(m_csVisualizerWndLock);
		if (m_pVisualizerWnd)
			m_pVisualizerWnd->SetSampleRate(SampleRate);
	}

	if (!m_pAPU->SetupSound(SampleRate, 1, (m_iMachineType == NTSC) ? MACHINE_NTSC : MACHINE_PAL))
		return false;

	currN163LevelOffset = m_pDocument->GetN163LevelOffset();

	{
		auto config = CAPUConfig(m_pAPU);

		config.SetChipLevel(CHIP_LEVEL_APU1, float(pSettings->ChipLevels.iLevelAPU1 / 10.0f));
		config.SetChipLevel(CHIP_LEVEL_APU2, float(pSettings->ChipLevels.iLevelAPU2 / 10.0f));
		config.SetChipLevel(CHIP_LEVEL_VRC6, float(pSettings->ChipLevels.iLevelVRC6 / 10.0f));
		config.SetChipLevel(CHIP_LEVEL_VRC7, float(pSettings->ChipLevels.iLevelVRC7 / 10.0f));
		config.SetChipLevel(CHIP_LEVEL_MMC5, float(pSettings->ChipLevels.iLevelMMC5 / 10.0f));
		config.SetChipLevel(CHIP_LEVEL_FDS, float(pSettings->ChipLevels.iLevelFDS / 10.0f));
		config.SetChipLevel(CHIP_LEVEL_N163, float(
			(pSettings->ChipLevels.iLevelN163 + currN163LevelOffset) / 10.0f));
		config.SetChipLevel(CHIP_LEVEL_S5B, float(pSettings->ChipLevels.iLevelS5B / 10.0f));

		// Update blip-buffer filtering
		config.SetupMixer(
			pSettings->Sound.iBassFilter,
			pSettings->Sound.iTrebleFilter,
			pSettings->Sound.iTrebleDamping,
			pSettings->Sound.iMixVolume,
			pSettings->Emulation.iFDSLowpass,
			pSettings->Emulation.iVRC7Patch);
	}

	m_bAudioClipping = false;
	m_bBufferUnderrun = false;
	m_bBufferTimeout = false;
	m_iClipCounter = 0;

	TRACE("SoundGen: Created sound channel with params: %i Hz, %i bits, %i ms (%i blocks)\n", SampleRate, SampleSize, BufferLen, iBlocks);

	return true;
}

void CSoundGen::CloseAudioDevice()
{
	// Kill sound stream
	if (m_pSoundStream) {
		m_pSoundStream->Stop();
		m_pSoundInterface->CloseChannel(m_pSoundStream);
		m_pSoundStream = NULL;
	}
}

void CSoundGen::CloseAudio()
{
	// Called from player thread
	ASSERT(std::this_thread::get_id() == m_audioThreadID);

	CloseAudioDevice();

	if (m_pSoundInterface) {
		m_pSoundInterface->CloseDevice();
		delete m_pSoundInterface;
		m_pSoundInterface = NULL;
	}

	if (m_hInterruptEvent) {
		::CloseHandle(m_hInterruptEvent);
		m_hInterruptEvent = NULL;
	}
}

void CSoundGen::ResetBuffer()
{
	// Called from player thread
	ASSERT(std::this_thread::get_id() == m_audioThreadID);

	m_iBufferPtr = 0;

	if (m_pSoundStream)
		m_pSoundStream->ClearBuffer();

	m_pAPU->Reset();
}

bool CSoundGen::TryWaitForWritable(uint32_t& framesWritable, uint32_t& bytesWritable) {
	// Waits for sound stream, even in WAV export mode. Not a big deal (doesn't
	// slow down export), and redesigning to avoid this is hard.
	while (true) {
		WaitResult result = m_pSoundStream->WaitForReady(AUDIO_TIMEOUT);
		// TRACE("WaitResult %d\n", result);
		switch (result) {
		case WaitResult::Ready:
			goto endWhile;

		case WaitResult::InternalError:
		case WaitResult::Timeout:
			// Show in the UI, "It appears the current sound settings aren't working,
			// change settings and try again."
			m_bBufferTimeout = true;
			[[fallthrough]];

			// Custom event (requested quit)
		case WaitResult::Interrupted:
			// Forget queued audio and quit.
			m_iBufferPtr = 0;
			return false;

		case WaitResult::OutOfSync:
			// Buffer underrun detected (unreachable since underruns are not reported)
			m_iAudioUnderruns++;
			m_bBufferUnderrun = true;
			continue;
		}
	}
endWhile:

	framesWritable = GetBufferFramesWritable();
	bytesWritable = m_pSoundStream->FramesToPubBytes(framesWritable);
	return true;
}

unsigned int CSoundGen::GetBufferFramesWritable() const {
	if (m_bRendering) {
		return m_iBufSizeSamples;
	}
	else {
		return m_pSoundStream->BufferFramesWritable();
	}
}

void CSoundGen::FlushBuffer(int16_t const * pBuffer, uint32_t Size)
{
	// Callback method from emulation

	// May only be called from sound player thread
	ASSERT(std::this_thread::get_id() == m_audioThreadID);

	if (!m_pSoundStream)
		return;

	if (m_iSampleSize == 8)
		FillBuffer<uint8_t, 8>(pBuffer, Size);
	else
		FillBuffer<int16_t, 0>(pBuffer, Size);

	if (m_iClipCounter > 50) {
		// Ignore some clipping to allow the HP-filter adjust itself
		m_iClipCounter = 0;
		m_bAudioClipping = true;
	}
	else if (m_iClipCounter > 0)
		--m_iClipCounter;
}

template <class T, int SHIFT>
void CSoundGen::FillBuffer(int16_t const * pBuffer, uint32_t Size)
{
	// Called when the APU audio buffer is full and
	// ready for playing

	const int SAMPLE_MAX = 32768;

	T *pConversionBuffer = (T*)m_pAccumBuffer;

	unsigned int framesWritable, bytesWritable;
	if (!TryWaitForWritable(framesWritable, bytesWritable)) {
		return;
	}

	for (uint32_t i = 0; i < Size; ++i) {
		int16_t Sample = pBuffer[i];

		// 1000 Hz test tone
#ifdef AUDIO_TEST
		static double sine_phase = 0;
		Sample = int32_t(sin(sine_phase) * 10000.0);

		static double freq = 1000;
		// Sweep
		//freq+=0.1;
		if (freq > 20000)
			freq = 20;

		sine_phase += freq / (double(m_pSoundStream->GetSampleRate()) / 6.283184);
		if (sine_phase > 6.283184)
			sine_phase -= 6.283184;
#endif /* AUDIO_TEST */

		// Clip detection
		if (Sample == (SAMPLE_MAX - 1) || Sample == -SAMPLE_MAX) {
			++m_iClipCounter;
		}

		ASSERT(m_iBufferPtr < m_iBufSizeSamples);

		// Visualizer
		m_iGraphBuffer[m_iBufferPtr] = (short)Sample;

		// Convert sample and store in temp buffer
#ifdef DITHERING
		if (SHIFT > 0)
			Sample = (Sample + dither(1 << SHIFT)) >> SHIFT;
#else
		Sample >>= SHIFT;
#endif

		if (SHIFT == 8)
			Sample ^= 0x80;

		pConversionBuffer[m_iBufferPtr++] = (T)Sample;

		// If buffer is filled, throw it to sound interface
		// TODO if we add stereo support, ensure m_iBufferPtr is frames not samples
		// (otherwise this code breaks).
		if (m_iBufferPtr >= framesWritable) {
			if (!PlayBuffer(framesWritable, bytesWritable))
				return;

			if (!TryWaitForWritable(framesWritable, bytesWritable)) {
				return;
			}
		}
	}

	/*
	Write the entire remaining buffer to WASAPI. This is not necessary to prevent
	stuttering, but it's harmless to keep. How do we know it's legal to do so?

	Before the loop, we call TryWaitForWritable(), which calls WaitForReady() before
	updating framesWritable.

	On every loop iteration, we check if m_iBufferPtr >= framesWritable.
	If so, we call CSoundGen::PlayBuffer() (which sets m_iBufferPtr to 0), then call
	TryWaitForWritable() again. So after each loop iteration, we've called
	TryWaitForWritable() at least once since last calling CSoundGen::PlayBuffer(),
	and either m_iBufferPtr == 0 or m_iBufferPtr < framesWritable.

	Therefore once the loop finishes, if m_iBufferPtr > 0, it's legal to write the
	entire remaining buffer to WASAPI.
	*/
	if (m_iBufferPtr > 0) {
		ASSERT(m_iBufferPtr < framesWritable);
		if (!PlayBuffer(m_iBufferPtr, m_pSoundStream->FramesToPubBytes(m_iBufferPtr)))
			return;
	}
}

bool CSoundGen::PlayBuffer(unsigned int framesToWrite, unsigned int bytesToWrite)
{
	if (m_bRendering) {
		// Output to file
		ASSERT(m_pWaveFile);		// // //
		m_pWaveFile->WriteWave(m_pAccumBuffer, bytesToWrite);
		m_iBufferPtr = 0;
	}
	else {
		// Output audio
		// Write audio to buffer
		m_pSoundStream->WriteBuffer(m_pAccumBuffer, bytesToWrite);

		// Draw graph
		{
			std::unique_lock<std::mutex> lock(m_csVisualizerWndLock);
			if (m_pVisualizerWnd)
				m_pVisualizerWnd->FlushSamples(gsl::span(m_iGraphBuffer, framesToWrite));
		}

		// Reset buffer position
		m_iBufferPtr = 0;
		m_bBufferTimeout = false;
	}

	return true;
}

unsigned int CSoundGen::GetUnderruns() const
{
	return m_iAudioUnderruns;
}

unsigned int CSoundGen::GetFrameRate()
{
	int FrameRate = m_iFrameCounter;
	m_iFrameCounter = 0;
	return FrameRate;
}

//// Tracker playing routines //////////////////////////////////////////////////////////////////////////////

void CSoundGen::GenerateVibratoTable(vibrato_t Type)
{
	for (int i = 0; i < 16; ++i) {	// depth
		for (int j = 0; j < 16; ++j) {	// phase
			int value = 0;
			double angle = (double(j) / 16.0) * (3.1415 / 2.0);

			if (Type == VIBRATO_NEW)
				value = int(sin(angle) * NEW_VIBRATO_DEPTH[i] /*+ 0.5f*/);
			else {
				value = (int)((double(j * OLD_VIBRATO_DEPTH[i]) / 16.0) + 1);
			}

			m_iVibratoTable[i * 16 + j] = value;
		}
	}

#ifdef WRITE_VIBRATO_FILE
	CStdioFile a("..\\nsf driver\\vibrato.s", CFile::modeWrite | CFile::modeCreate);
	a.WriteString("; Vibrato table (256 bytes)\n"
				  "ft_vibrato_table: ;; Patch\n");
	for (int i = 0; i < 16; i++) {	// depth
		a.WriteString("\t.byte ");
		for (int j = 0; j < 16; j++) {	// phase
			CString b;
			b.Format("$%02X%s", m_iVibratoTable[i * 16 + j], j < 15 ? ", " : "");
			a.WriteString(b);
		}
		a.WriteString("\n");
	}
	a.Close();
#endif
}

void CSoundGen::SetupVibratoTable(vibrato_t Type)
{
	GenerateVibratoTable(Type);
}

int CSoundGen::ReadVibratoTable(int index) const
{
	return m_iVibratoTable[index];
}

int CSoundGen::ReadPeriodTable(int Index, int Table) const		// // //
{
	switch (Table) {
	case CDetuneTable::DETUNE_NTSC: return m_iNoteLookupTableNTSC[Index]; break;
	case CDetuneTable::DETUNE_PAL:  return m_iNoteLookupTablePAL[Index]; break;
	case CDetuneTable::DETUNE_SAW:  return m_iNoteLookupTableSaw[Index]; break;
	case CDetuneTable::DETUNE_VRC7: return m_iNoteLookupTableVRC7[Index]; break;
	case CDetuneTable::DETUNE_FDS:  return m_iNoteLookupTableFDS[Index]; break;
	case CDetuneTable::DETUNE_N163: return m_iNoteLookupTableN163[Index]; break;
	case CDetuneTable::DETUNE_S5B:  return m_iNoteLookupTableNTSC[Index] + 1; break;
	default:
		AfxDebugBreak(); return m_iNoteLookupTableNTSC[Index];
	}
}

void CSoundGen::BeginPlayer(play_mode_t Mode, int Track)
{
	// Called from player thread
	ASSERT(std::this_thread::get_id() == m_audioThreadID);
	ASSERT(m_pDocument != NULL);
	ASSERT(m_pTrackerView != NULL);

	if (!m_pDocument || !m_pSoundStream || !m_pDocument->IsFileLoaded())
		return;

	switch (Mode) {
		// Play from top of pattern
		case MODE_PLAY:
			m_bPlayLooping = false;
			m_iPlayFrame = m_pTrackerView->GetSelectedFrame();
			m_iPlayRow = 0;
			break;
		// Repeat pattern
		case MODE_PLAY_REPEAT:
			m_bPlayLooping = true;
			m_iPlayFrame = m_pTrackerView->GetSelectedFrame();
			m_iPlayRow = 0;
			break;
		// Start of song
		case MODE_PLAY_START:
			m_bPlayLooping = false;
			m_iPlayFrame = 0;
			m_iPlayRow = 0;
			break;
		// From cursor
		case MODE_PLAY_CURSOR:
			m_bPlayLooping = false;
			m_iPlayFrame = m_pTrackerView->GetSelectedFrame();
			m_iPlayRow = m_pTrackerView->GetSelectedRow();
			break;
		// From row marker (bookmark)
		case MODE_PLAY_MARKER:		// // // 050B
			m_bPlayLooping = false;
			m_iPlayFrame = m_pTrackerView->GetMarkerFrame();
			m_iPlayRow = m_pTrackerView->GetMarkerRow();
			break;
	}

	m_bPlaying			= true;
	m_bHaltRequest      = false;
	m_bDoHalt			= false;		// // //
	m_iPlayTicks		= 0;
	m_iFramesPlayed		= 0;
	m_iRowsPlayed		= 0;		// // //
	m_iJumpToPattern	= -1;
	m_iSkipToRow		= -1;
	m_bUpdateRow		= true;
	m_iPlayMode			= Mode;
	m_bDirty			= true;
	m_iPlayTrack		= Track;

	memset(m_bFramePlayed, false, sizeof(bool) * MAX_FRAMES);

#ifdef WRITE_VGM		// // //
	std::queue<int>().swap(m_iRegisterStream);
#endif

	{		// // // 050B
		m_iRowTickCount = 0;

		float Tempo = GetTempo();
		for (auto &x : m_fBPMCacheValue)
			x = Tempo;
		for (auto &x : m_iBPMCacheTicks)
			x = 1;
	}

	ResetTempo();
	ResetAPU();

	MakeSilent();

	m_pTrackerView->MakeSilent();

	if (theApp.GetSettings()->General.bRetrieveChanState)		// // //
		ApplyGlobalState();

	if (m_pInstRecorder->GetRecordChannel() != -1)		// // //
		m_pInstRecorder->StartRecording();
}

void CSoundGen::ApplyGlobalState()		// // //
{
	int Frame = IsPlaying() ? GetPlayerFrame() : m_pTrackerView->GetSelectedFrame();
	int Row = IsPlaying() ? GetPlayerRow() : m_pTrackerView->GetSelectedRow();
	if (stFullState *State = m_pDocument->RetrieveSoundState(GetPlayerTrack(), Frame, Row, -1)) {
		if (State->Tempo != -1)
			m_iTempo = State->Tempo;
		if (State->GroovePos >= 0) {
			m_iGroovePosition = State->GroovePos;
			if (State->Speed >= 0)
				m_iGrooveIndex = State->Speed;
			if (m_pDocument->GetGroove(m_iGrooveIndex) != NULL)
				m_iSpeed = m_pDocument->GetGroove(m_iGrooveIndex)->GetEntry(m_iGroovePosition);
		}
		else {
			if (State->Speed >= 0)
				m_iSpeed = State->Speed;
			m_iGrooveIndex = -1;
		}
		m_iLastHighlight = m_pDocument->GetHighlightAt(GetPlayerTrack(), Frame, Row).First;
		SetupSpeed();
		for (int i = 0; i < m_pDocument->GetChannelCount(); i++) {
			for (int j = 0; j < sizeof(m_pTrackerChannels) / sizeof(CTrackerChannel*); ++j)		// // // pick this out later
				if (m_pChannels[j] && m_pTrackerChannels[j]->GetID() == State->State[i].ChannelIndex) {
					m_pChannels[j]->ApplyChannelState(&State->State[i]); break;
				}
		}
		delete State;
	}
}

/*!	\brief Obtains a human-readable form of a channel state object.
	\warning The output of this method is neither guaranteed nor required to match that of
	CChannelHandler::GetStateString.
	\param State A reference to the channel state object.
	\return A string representing the channel's state.
	\relates CChannelHandler
*/
static CString GetStateString(const stChannelState &State)
{
	CString log = _T("");
	log.Format(_T("Inst.: "));
	if (State.Instrument == MAX_INSTRUMENTS)
		log.Append("None");
	else
		log.AppendFormat(_T("%02X"), State.Instrument);
	log.AppendFormat(_T("        Vol.: %X        Active effects:"), State.Volume >= MAX_VOLUME ? 0xF : State.Volume);

	CString effStr = _T("");

	const effect_t SLIDE_EFFECT = State.Effect[EF_ARPEGGIO] >= 0 ? EF_ARPEGGIO :
								  State.Effect[EF_PORTA_UP] >= 0 ? EF_PORTA_UP :
								  State.Effect[EF_PORTA_DOWN] >= 0 ? EF_PORTA_DOWN :
								  EF_PORTAMENTO;
	for (const auto &x : {SLIDE_EFFECT, EF_VIBRATO, EF_TREMOLO, EF_VOLUME_SLIDE, EF_PITCH, EF_DUTY_CYCLE, EF_HARMONIC}) {
		int p = State.Effect[x];
		if (p < 0) continue;
		if (p == effects[x].initial) continue;
		effStr.AppendFormat(_T(" %c%02X"), EFF_CHAR[x], p);
	}

	if ((State.ChannelIndex >= CHANID_SQUARE1 && State.ChannelIndex <= CHANID_SQUARE2) ||
			State.ChannelIndex == CHANID_NOISE ||
		(State.ChannelIndex >= CHANID_MMC5_SQUARE1 && State.ChannelIndex <= CHANID_MMC5_SQUARE2))
		for (const auto &x : {EF_VOLUME}) {
			int p = State.Effect[x];
			if (p < 0) continue;
			effStr.AppendFormat(_T(" %c%02X"), EFF_CHAR[x], p);
		}
	else if (State.ChannelIndex == CHANID_TRIANGLE)
		for (const auto &x : {EF_VOLUME, EF_NOTE_CUT}) {
			int p = State.Effect[x];
			if (p < 0) continue;
			effStr.AppendFormat(_T(" %c%02X"), EFF_CHAR[x], p);
		}
	else if (State.ChannelIndex == CHANID_DPCM)
		for (const auto &x : {EF_SAMPLE_OFFSET, /*EF_DPCM_PITCH*/}) {
			int p = State.Effect[x];
			if (p <= 0) continue;
			effStr.AppendFormat(_T(" %c%02X"), EFF_CHAR[x], p);
		}
	else if (State.ChannelIndex >= CHANID_VRC7_CH1 && State.ChannelIndex <= CHANID_VRC7_CH6)
		for (const auto &x : VRC7_EFFECTS) {
			int p = State.Effect[x];
			if (p < 0) continue;
			effStr.AppendFormat(_T(" %c%02X"), EFF_CHAR[x], p);
		}
	else if (State.ChannelIndex == CHANID_FDS)
		for (const auto &x : FDS_EFFECTS) {
			// CFamiTrackerDoc::RetrieveSoundState() does not retrieve I0x and Jxx effects (only Ixy),
			// so those will never be displayed.
			int p = State.Effect[x];
			if (p < 0) continue;
			if (p == effects[x].initial) continue;
			effStr.AppendFormat(_T(" %c%02X"), EFF_CHAR[x], p);
		}
	else if (State.ChannelIndex >= CHANID_S5B_CH1 && State.ChannelIndex <= CHANID_S5B_CH3)
		for (const auto &x : S5B_EFFECTS) {
			int p = State.Effect[x];
			if (p < 0) continue;
			effStr.AppendFormat(_T(" %c%02X"), EFF_CHAR[x], p);
		}
	else if (State.ChannelIndex >= CHANID_N163_CH1 && State.ChannelIndex <= CHANID_N163_CH8)
		for (const auto &x : N163_EFFECTS) {
			int p = State.Effect[x];
			if (p < 0) continue;
			if (p == effects[x].initial) continue;
			effStr.AppendFormat(_T(" %c%02X"), EFF_CHAR[x], p);
		}
	if (State.Effect_LengthCounter >= 0)
		effStr.AppendFormat(_T(" %c%02X"), EFF_CHAR[EF_VOLUME], State.Effect_LengthCounter);
	if (State.Effect_AutoFMMult >= 0)
		effStr.AppendFormat(_T(" %c%02X"), EFF_CHAR[EF_FDS_MOD_DEPTH], State.Effect_AutoFMMult);

	if (effStr.IsEmpty()) effStr = _T(" None");
	log.Append(effStr);
	return log;
}

CString CSoundGen::RecallChannelState(int Channel) const		// // //
{
	if (IsPlaying()) return m_pChannels[Channel]->GetStateString();
	int Frame = m_pTrackerView->GetSelectedFrame();
	int Row = m_pTrackerView->GetSelectedRow();
	CString str = _T("");
	if (stFullState *State = m_pDocument->RetrieveSoundState(GetPlayerTrack(), Frame, Row, Channel)) {
		str = GetStateString(State->State[m_pDocument->GetChannelIndex(Channel)]);
		if (State->Tempo >= 0)
			str.AppendFormat(_T("        Tempo: %d"), State->Tempo);
		if (State->Speed >= 0) {
			if (State->GroovePos >= 0) {
				str.AppendFormat(_T("        Groove: %02X <-"), State->Speed);
				CGroove *Groove = m_pDocument->GetGroove(State->Speed);
				const unsigned char Size = Groove->GetSize();
				for (unsigned char i = 0; i < Size; i++)
					str.AppendFormat(_T(" %d"), Groove->GetEntry((i + State->GroovePos) % Size));
			}
			else
				str.AppendFormat(_T("        Speed: %d"), State->Speed);
		}
		delete State;
	}
	return str;
}

void CSoundGen::HaltPlayer()
{
	// Called from player thread
	ASSERT(std::this_thread::get_id() == m_audioThreadID);

	// Move player to non-playing state
	m_bPlaying = false;
	m_bHaltRequest = false;
	m_bDoHalt = false;		// // //

	MakeSilent();

	m_pAPU->ClearSample();		// // //
/*
	for (int i = 0; i < CHANNELS; ++i) {
		if (m_pChannels[i] != NULL) {
			m_pChannels[i]->ResetChannel();
		}
	}
*/

	// Signal that playback has stopped
	if (m_pTrackerView != NULL) {
		m_pTrackerView->PostMessage(WM_USER_PLAYER, m_iPlayFrame, m_iPlayRow);
		m_pInstRecorder->StopRecording(m_pTrackerView);		// // //
	}

#ifdef WRITE_VGM		// // //
	CFile vgm("test.vgm", CFile::modeCreate | CFile::modeWrite | CFile::typeBinary);
	int c = 0;
	for (int i = 0; i < 0x100; ++i)
		vgm.Write(&c, 1);
	int Delay = 0;
	int DelayTotal = 0;
	const long long VGM_SAMPLE_RATE = 44100;

	while (!m_iRegisterStream.empty()) { // TODO: move this to a separate thread
		int Val = m_iRegisterStream.front();
		m_iRegisterStream.pop();
		if (Val == 0x62) {
			++Delay;
			++DelayTotal;
		}
		else {
			if (Delay) {
				int Samples = static_cast<int>(VGM_SAMPLE_RATE * Delay / m_iFrameRate);
				while (Samples > 0xFFFF) {
					Samples -= 0xFFFF;
					c = 0xFFFF61;
					vgm.Write(&c, 3);
				}
				if (Samples == static_cast<int>(VGM_SAMPLE_RATE / CAPU::FRAME_RATE_NTSC)) { // 735
					c = 0x62;
					vgm.Write(&c, 1);
				}
				else if (Samples == static_cast<int>(VGM_SAMPLE_RATE / CAPU::FRAME_RATE_PAL)) { // 882
					c = 0x63;
					vgm.Write(&c, 1);
				}
				else {
					c = 0x61 | (Samples << 8);
					vgm.Write(&c, 3);
				}
				Delay = 0;
			}
			vgm.Write(&Val, 1);
			Val = m_iRegisterStream.front();
			m_iRegisterStream.pop();
			vgm.Write(&Val, 1);
			Val = m_iRegisterStream.front();
			m_iRegisterStream.pop();
			vgm.Write(&Val, 1);
		}
	}
	c = 0x66;
	vgm.Write(&c, 1);

	vgm.Seek(0, CFile::begin);
	char Header[256] = {'V', 'g', 'm', ' '};
	*reinterpret_cast<int*>(Header + 0x04) = static_cast<int>(vgm.GetLength()) - 4;
	*reinterpret_cast<int*>(Header + 0x08) = 0x161;
	*reinterpret_cast<int*>(Header + 0x18) = static_cast<int>(VGM_SAMPLE_RATE * DelayTotal / m_iFrameRate);
	*reinterpret_cast<int*>(Header + 0x24) = m_iFrameRate;
	*reinterpret_cast<int*>(Header + 0x34) = 0xCC;
	*reinterpret_cast<int*>(Header + 0x84) = CAPU::BASE_FREQ_NTSC;
	if (m_pDocument) { // TODO: do this in BeginPlayer
		if (m_pDocument->ExpansionEnabled(SNDCHIP_FDS))
			*reinterpret_cast<int*>(Header + 0x84) |= 0x80000000;
		if (m_pDocument->ExpansionEnabled(SNDCHIP_S5B)) {
			*reinterpret_cast<int*>(Header + 0x74) = CAPU::BASE_FREQ_NTSC / 2;
			*reinterpret_cast<int*>(Header + 0x78) = 0x0110;
		}
	}

	vgm.Write(Header, 256);
	vgm.Close();
#endif
}

void CSoundGen::ResetAPU()
{
	// Called from player thread
	ASSERT(std::this_thread::get_id() == m_audioThreadID);

	// Reset the APU
	m_pAPU->Reset();

	// Enable all channels
	m_pAPU->Write(0x4015, 0x0F);
	m_pAPU->Write(0x4017, 0x00);

	// // // for VGM
	WriteRegister(0x4015, 0x0F);
	WriteRegister(0x4017, 0x00);
	WriteRegister(0x4023, 0x02); // FDS enable

	// MMC5
	m_pAPU->Write(0x5015, 0x03);

	m_pAPU->ClearSample();		// // //
}

void CSoundGen::AddCyclesUnlessEndOfFrame(int Count)
{
	// Called from player thread
	ASSERT(std::this_thread::get_id() == m_audioThreadID);

	// Add APU cycles
	Count = std::min(Count, m_iUpdateCycles - m_iConsumedCycles);
	m_iConsumedCycles += Count;
	m_pAPU->AddCycles(Count);
}

uint8_t CSoundGen::GetReg(int Chip, int Reg) const
{
	return m_pAPU->GetReg(Chip, Reg);
}

CRegisterState *CSoundGen::GetRegState(unsigned Chip, unsigned Reg) const		// // //
{
	return m_pAPU->GetRegState(Chip, Reg);
}

double CSoundGen::GetChannelFrequency(unsigned Chip, int Channel) const		// // //
{
	return m_pAPU->GetFreq(Chip, Channel);
}

int CSoundGen::GetFDSModCounter() const
{
	return m_pAPU->GetFDSModCounter();
}

void CSoundGen::MakeSilent()
{
	// Called from player thread
	ASSERT(std::this_thread::get_id() == m_audioThreadID);

	m_pAPU->Reset();
	m_pAPU->ClearSample();		// // //

	for (int i = 0; i < CHANNELS; ++i) {
		if (m_pChannels[i])
			m_pChannels[i]->ResetChannel();
		if (m_pTrackerChannels[i])
			m_pTrackerChannels[i]->Reset();
	}
}

void CSoundGen::ResetState()
{
	// Called when a new module is loaded
	m_iPlayTrack = 0;
}

// Get tempo values from the document
void CSoundGen::ResetTempo()
{
	ASSERT(m_pDocument != NULL);

	if (!m_pDocument)
		return;

	m_iSpeed = m_pDocument->GetSongSpeed(m_iPlayTrack);
	m_iTempo = m_pDocument->GetSongTempo(m_iPlayTrack);
	m_iLastHighlight = m_pDocument->GetHighlight().First;		// // //

	m_iTempoAccum = 0;

	if (m_pDocument->GetSongGroove(m_iPlayTrack) && m_pDocument->GetGroove(m_iSpeed) != NULL) {		// // //
		m_iGrooveIndex = m_iSpeed;
		m_iGroovePosition = 0;
		if (m_pDocument->GetGroove(m_iGrooveIndex) != NULL)
			m_iSpeed = m_pDocument->GetGroove(m_iGrooveIndex)->GetEntry(m_iGroovePosition);
	}
	else {
		m_iGrooveIndex = -1;
		if (m_pDocument->GetSongGroove(m_iPlayTrack))
			m_iSpeed = DEFAULT_SPEED;
	}
	SetupSpeed();

	m_bUpdateRow = false;
}

void CSoundGen::SetHighlightRows(int Rows)		// // //
{
	m_iLastHighlight = Rows;
}

void CSoundGen::SetupSpeed()
{
	if (m_iTempo) {		// // //
		m_iTempoDecrement = (m_iTempo * 24) / m_iSpeed;
		m_iTempoRemainder = (m_iTempo * 24) % m_iSpeed;
	}
	else {
		m_iTempoDecrement = 1;
		m_iTempoRemainder = 0;
	}
}

// Return current tempo setting in BPM
float CSoundGen::GetTempo() const
{
	float Tempo = static_cast<float>(m_iTempo);		// // //
	if (!m_iTempo) Tempo = static_cast<float>(2.5 * m_iFrameRate);

	float Speed;
	if (m_iGrooveIndex != -1) {
		if (m_pDocument->GetGroove(m_iGrooveIndex) == NULL)
			Speed = DEFAULT_SPEED;
		else Speed = m_pDocument->GetGroove(m_iGrooveIndex)->GetAverage();
	}
	else Speed = static_cast<float>(m_iSpeed);

	return !m_iSpeed ? 0 : float(Tempo * 6) / Speed;
}

float CSoundGen::GetAverageBPM() const		// // // 050B
{
	float BPMtot = 0.f;
	float TickTot = 0.f;
	for (int i = 0; i < AVERAGE_BPM_SIZE; ++i)
		BPMtot += m_fBPMCacheValue[i] * m_iBPMCacheTicks[i];
	for (const auto &x : m_iBPMCacheTicks)
		TickTot += x;
	return static_cast<float>(BPMtot / TickTot);
}

float CSoundGen::GetCurrentBPM() const		// // //
{
	float EngineSpeed = static_cast<float>(m_pDocument->GetFrameRate());
	float BPM = std::min(IsPlaying() && theApp.GetSettings()->Display.bAverageBPM ? GetAverageBPM() : GetTempo(),
						 EngineSpeed * 15);		// // // 050B
	return static_cast<float>(BPM * 4. / (m_iLastHighlight ? m_iLastHighlight : 4));
}

void CSoundGen::RunFrame()
{
	// Called from player thread
	ASSERT(std::this_thread::get_id() == m_audioThreadID);
	ASSERT(m_pDocument != NULL);
	ASSERT(m_pTrackerView != NULL);

	// View callback
	m_pTrackerView->PlayerTick();

	if (IsPlaying()) {

		++m_iPlayTicks;

		if (m_bRendering) {
			if (m_iRenderEndWhen == SONG_TIME_LIMIT) {
				if (m_iPlayTicks > (unsigned int)m_iRenderEndParam)
					m_bRequestRenderStop = m_bHaltRequest = true;		// // //
			}
			else if (m_iRenderEndWhen == SONG_LOOP_LIMIT) {
				//if (m_iFramesPlayed >= m_iRenderEndParam)
				if (m_iRowsPlayed >= m_iRenderEndParam && m_iTempoAccum <= 0)		// // //
					m_bRequestRenderStop = m_bHaltRequest = true;
			}
		}

		++m_iRowTickCount;		// // // 050B
		m_iStepRows = 0;

		// Fetch next row
		if (m_iTempoAccum <= 0) {
			// Enable this to skip rows on high tempos
//			while (m_iTempoAccum <= 0)  {
			if (m_iGrooveIndex != -1 && m_pDocument->GetGroove(m_iGrooveIndex) != NULL) {		// // //
				m_iSpeed = m_pDocument->GetGroove(m_iGrooveIndex)->GetEntry(m_iGroovePosition);
				SetupSpeed();
				m_iGroovePosition++;
			}
				m_iStepRows++;
//			}
			m_bUpdateRow = true;
			ReadPatternRow();
			++m_iRenderRow;

			if (auto pMark = m_pDocument->GetBookmarkAt(m_iPlayTrack, m_iPlayFrame, m_iPlayRow))		// // //
				if (pMark->m_Highlight.First != -1)
					m_iLastHighlight = pMark->m_Highlight.First;

			// // // 050B
			m_fBPMCacheValue[m_iBPMCachePosition] = GetTempo();		// // // 050B
			m_iBPMCacheTicks[m_iBPMCachePosition] = m_iRowTickCount;
			m_iRowTickCount = 0;
			++m_iBPMCachePosition %= AVERAGE_BPM_SIZE;
		}
		else {
			m_bUpdateRow = false;
		}
	}
}

void CSoundGen::CheckControl()
{
	// This function takes care of jumping and skipping
	ASSERT(m_pTrackerView != NULL);

	if (IsPlaying()) {
		if (m_bDoHalt) {		// // //
			++m_iRowsPlayed;
		}
		else if (m_bPlayLooping) {
			if (m_iJumpToPattern != -1 || m_iSkipToRow != -1)
				m_iPlayRow = 0;
			else
				while (m_iStepRows--)
					PlayerStepRow();
		}
		else {
			// Jump
			if (m_iJumpToPattern != -1)
				PlayerJumpTo(m_iJumpToPattern);
			// Skip
			else if (m_iSkipToRow != -1)
				PlayerSkipTo(m_iSkipToRow);
			// or just move on
			else
				while (m_iStepRows--)
					PlayerStepRow();
		}

		m_iJumpToPattern = -1;
		m_iSkipToRow = -1;
	}

	if (m_bDirty) {
		m_bDirty = false;
		if (!m_bRendering)
			m_pTrackerView->PostMessage(WM_USER_PLAYER, m_iPlayFrame, m_iPlayRow);
	}
}

void CSoundGen::LoadMachineSettings()		// // //
{
	// Setup machine-type and speed
	//
	// Machine = NTSC or PAL
	//
	// Rate = frame rate (0 means machine default)
	//

	ASSERT(m_pAPU != NULL);

	m_iMachineType = m_pDocument->GetMachine();		// // // 050B

	int BaseFreq	= (m_iMachineType == NTSC) ? CAPU::BASE_FREQ_NTSC  : CAPU::BASE_FREQ_PAL;
	int DefaultRate = (m_iMachineType == NTSC) ? CAPU::FRAME_RATE_NTSC : CAPU::FRAME_RATE_PAL;

	// Choose a default rate if not predefined
	int Rate = m_pDocument->GetEngineSpeed();		// // //
	if (Rate == 0)
		Rate = DefaultRate;

	// Number of cycles between each APU update
	m_iUpdateCycles = BaseFreq / Rate;

	{
		auto l = Lock();
		m_pAPU->ChangeMachineRate(m_iMachineType == NTSC ? MACHINE_NTSC : MACHINE_PAL, Rate);		// // //
		m_pAPU->Reset();
	}

#if WRITE_VOLUME_FILE
	CFile file("vol.txt", CFile::modeWrite | CFile::modeCreate);

	for (int i = 0; i < 16; i++) {
		for (int j = /*(i / 2)*/0; j < 8; j++) {
			int a = (i*(j*2)) / 15;
			int b = (i*(j*2+1)) / 15;
			if (i > 0 && j > 0 && a == 0) a = 1;
			if (j > 0 && i > 0 && b == 0) b = 1;
			unsigned char c = (a<<4) | b;
			CString st;
			st.Format("$%02X, ", c);
			file.Write(st, st.GetLength());
		}
		file.Write("\n", 1);
	}

	file.Close();
#endif /* WRITE_VOLUME_FILE */
}

stDPCMState CSoundGen::GetDPCMState() const
{
	stDPCMState State;

	if (m_pAPU == NULL) {
		State.DeltaCntr = 0;
		State.SamplePos = 0;
	}
	else {
		State.DeltaCntr = m_pAPU->GetDeltaCounter();
		State.SamplePos = m_pAPU->GetSamplePos();
	}

	return State;
}

int CSoundGen::GetChannelVolume(int Channel) const
{
	if (!m_pChannels[Channel])
		return 0;
	return m_pChannels[Channel]->GetChannelVolume();
}

void CSoundGen::PlayNote(int Channel, stChanNote *NoteData, int EffColumns)
{
	if (!NoteData)
		return;

	// Update the individual channel
	m_pChannels[Channel]->PlayNote(NoteData, EffColumns);
}

void CSoundGen::SetSkipRow(int Row)
{
	m_iSkipToRow = Row;
}

void CSoundGen::SetJumpPattern(int Pattern)
{
	m_iJumpToPattern = Pattern;
}

void CSoundGen::EvaluateGlobalEffects(stChanNote *NoteData, int EffColumns)
{
	// Handle global effects (effects that affects all channels)
	for (int i = 0; i < EffColumns; ++i) {

		unsigned char EffNum   = NoteData->EffNumber[i];
		unsigned char EffParam = NoteData->EffParam[i];

		switch (EffNum) {
			// Fxx: Sets speed to xx
			case EF_SPEED:
				if (!EffParam)
					++EffParam;
				if (m_iTempo && EffParam >= m_iSpeedSplitPoint)		// // //
					m_iTempo = EffParam;
				else {		// // //
					m_iSpeed = EffParam;
					m_iGrooveIndex = -1;
				}
				SetupSpeed();
				break;

			// Oxx: Sets groove to xx
			// currently does not support starting at arbitrary index of a groove
			case EF_GROOVE:		// // //
				if (m_pDocument->GetGroove(EffParam % MAX_GROOVE) == NULL) break;
				m_iGrooveIndex = EffParam % MAX_GROOVE;
				m_iSpeed = m_pDocument->GetGroove(m_iGrooveIndex)->GetEntry(0);
				m_iGroovePosition = 1;
				SetupSpeed();
				break;

			// Bxx: Jump to pattern xx
			case EF_JUMP:
				SetJumpPattern(EffParam);
				break;

			// Dxx: Skip to next track and start at row xx
			case EF_SKIP:
				SetSkipRow(EffParam);
				break;

			// Cxx: Halt playback
			case EF_HALT:
				m_bDoHalt = true;		// // //
				if (m_bRendering) {
					// Unconditional stop
					++m_iFramesPlayed;
				}
				break;

			default: continue;		// // //
		}

		NoteData->EffNumber[i] = EF_NONE;
		NoteData->EffParam[i] = 0;
	}
}

// File rendering functions

bool CSoundGen::RenderToFile(LPTSTR pFile, render_end_t SongEndType, int SongEndParam, int Track)
{
	// Called from main thread
	ASSERT(GetCurrentThreadId() == theApp.m_nThreadID);
	ASSERT(m_pDocument != NULL);

	if (IsPlaying()) {
		//HaltPlayer();
		m_bHaltRequest = true;
		WaitForStop();
	}

	auto l = Lock();

	m_iRenderEndWhen = SongEndType;
	m_iRenderEndParam = SongEndParam;
	m_iRenderTrack = Track;
	m_iRenderRowCount = 0;
	m_iRenderRow = 0;

	if (m_iRenderEndWhen == SONG_TIME_LIMIT) {
		// This variable is stored in seconds, convert to frames
		m_iRenderEndParam *= m_pDocument->GetFrameRate();
	}
	else if (m_iRenderEndWhen == SONG_LOOP_LIMIT) {
		m_iRenderEndParam = m_pDocument->ScanActualLength(Track, m_iRenderEndParam);		// // //
		m_iRenderRowCount = m_iRenderEndParam;
	}

	ASSERT(!m_bRendering);
	ASSERT(m_pWaveFile == nullptr);
	m_pWaveFile = std::make_unique<CWaveFile>();
	// Unfortunately, destructor doesn't cleanup object. Only CloseFile() does.
	if (!m_pWaveFile ||
		!m_pWaveFile->OpenFile(pFile, theApp.GetSettings()->Sound.iSampleRate, theApp.GetSettings()->Sound.iSampleSize, 1)) {
		m_pTrackerView->PostMessage(WM_USER_ERROR, IDS_FILE_OPEN_ERROR);
		return false;
	}
	else {
		m_bRequestRenderStart = true;
		PostGuiMessage(WM_USER_START_RENDER, 0, 0);
	}

	return true;
}

void CSoundGen::StopRendering()
{
	// Called from player thread
	ASSERT(std::this_thread::get_id() == m_audioThreadID);
	ASSERT(m_bRendering);

	auto l = Lock();

	if (!IsRendering())
		return;

	m_bPlaying = false;
	m_bRendering = false;
	m_bStoppingRender = false;		// // //
	m_bRequestRenderStop = false;		// // //
	m_iPlayFrame = 0;
	m_iPlayRow = 0;
	m_pWaveFile->CloseFile();		// // //
	m_pWaveFile.reset();

	ResetBuffer();
	ResetAPU();		// // //
	HaltPlayer();
}

void CSoundGen::GetRenderStat(int &Frame, int &Time, bool &Done, int &FramesToRender, int &Row, int &RowCount) const
{
	Frame = m_iFramesPlayed;
	Time = m_iPlayTicks / m_pDocument->GetFrameRate();
	Done = m_bRendering;
	FramesToRender = m_iRenderEndParam;
	RowCount = m_iRenderRowCount;
	Row = m_iRenderRow;
}

bool CSoundGen::IsRendering() const
{
	return m_bRequestRenderStart || m_bRendering;
}

bool CSoundGen::IsBackgroundTask() const
{
	return m_bRendering;
}

// DPCM handling

void CSoundGen::PlaySample(const CDSample *pSample, int Offset, int Pitch)
{
	SAFE_RELEASE(m_pPreviewSample);

	// Sample may not be removed when used by the sample memory class!
	m_pAPU->WriteSample(pSample->GetData(), pSample->GetSize());		// // //

	int Loop = 0;
	int Length = ((pSample->GetSize() - 1) >> 4) - (Offset << 2);

	m_pAPU->Write(0x4010, Pitch | Loop);
	m_pAPU->Write(0x4012, Offset);			// load address, start at $C000
	m_pAPU->Write(0x4013, Length);			// length
	m_pAPU->Write(0x4015, 0x0F);
	m_pAPU->Write(0x4015, 0x1F);			// fire sample

	// Auto-delete samples with no name
	if (*pSample->GetName() == 0)
		m_pPreviewSample = pSample;
}

bool CSoundGen::PreviewDone() const
{
	return (m_pAPU->DPCMPlaying() == false);
}

bool CSoundGen::WaitForStop() const
{
	// Wait for player to stop, timeout = 4s
	// The player must have received the stop command or this will fail

	ASSERT(GetCurrentThreadId() == theApp.m_nThreadID);

	//return ::WaitForSingleObject(m_hIsPlaying, 4000) == WAIT_OBJECT_0;

	for (int i = 0; i < 40 && IsPlaying(); ++i)
		Sleep(100);

	return !IsPlaying();	// return false if still playing
}

//
// Overloaded functions
//

bool CSoundGen::InitInstance()
{
	//
	// Setup the sound player object, called when thread is started
	//

	HRESULT hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
	if (FAILED(hr)) {
		TRACE("SoundGen: Failed to CoInitializeEx COM!\n");
	}
	if (!FAILED(hr)) {
		// Call CoUninitialize() on shutdown.
		m_CoInitialized = true;
	}

	ASSERT(m_pDocument != NULL);
	ASSERT(m_pTrackerView != NULL);

	// First check if thread creation should be cancelled
	// This will occur when no sound object is available

	if (m_pSoundInterface == NULL)
		return FALSE;

	// Set running flag
	m_bRunning = true;

	// Generate default vibrato table
	GenerateVibratoTable(VIBRATO_NEW);

	if (!ResetAudioDevice()) {
		TRACE("SoundGen: Failed to reset audio device!\n");
		if (m_pVisualizerWnd != NULL)
			m_pVisualizerWnd->ReportAudioProblem();
	}

	ResetAPU();

	// Default tempo & speed
	m_iSpeed = DEFAULT_SPEED;
	m_iTempo = (DEFAULT_MACHINE_TYPE == NTSC) ? DEFAULT_TEMPO_NTSC : DEFAULT_TEMPO_PAL;

	TRACE("SoundGen: Created thread (0x%04x)\n", GetCurrentThreadId());

	// SetThreadPriority may or may not be unimplemented on Wine (I don't see a fixme
	// on Mac or Linux).
	auto success = SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL);
	TRACE("SoundGen: SetThreadPriority success %d\n", success);

	m_iDelayedStart = 0;
	m_iFrameCounter = 0;

//	SetupChannels();

	// We need to initialize state before playback begins.
	//
	// Normally, CSoundStream comes up and the GUI sends a WM_USER_STOP message,
	// calling CSoundGen::OnStopPlayer() -> CSoundGen::HaltPlayer() which initializes
	// state.
	//
	// However, if CSoundStream fails to initialize (for example due to WASAPI
	// sampling rate not matching system rate), WM_USER_STOP never arrives
	// and CSoundGen::HaltPlayer() is never called to initialize state.
	// Once you fix the sampling rate and properly initialize CSoundStream,
	// FT crashes in CChannelHandler::GetVibrato() because
	// CChannelHandler::m_iVibratoDepth is uninitialized.
	//
	// To avoid the crash, we need to initialize state on startup.
	HaltPlayer();

	return TRUE;
}

void CSoundGen::ExitInstance()
{
	// Shutdown the thread

	TRACE("SoundGen: Closing thread (0x%04x)\n", GetCurrentThreadId());

	// Free allocated memory
	SAFE_RELEASE_ARRAY(m_iGraphBuffer);
	SAFE_RELEASE_ARRAY(m_pAccumBuffer);

	// Make sure sound interface is shut down
	CloseAudio();

	theApp.RemoveSoundGenerator();

	m_bRunning = false;

	if (m_CoInitialized) {
		CoUninitialize();
	}
}

void CSoundGen::OnIdle()
{
	//
	// Main loop for audio playback thread
	//

	if (!m_pDocument || !m_pSoundStream || !m_pDocument->IsFileLoaded())
		return;

	++m_iFrameCounter;

	// Access the document object, skip if access wasn't granted to avoid gaps in audio playback
	if (m_pDocument->LockDocument(0)) {

		// Read module framerate
		m_iFrameRate = m_pDocument->GetFrameRate();

		RunFrame();

		// Play queued notes
		PlayChannelNotes();

		// Update player
		UpdatePlayer();

		// Channel updates (instruments, effects etc)
		UpdateChannels();

		// Unlock document
		m_pDocument->UnlockDocument();
	}

	// Update APU registers
	UpdateAPU();

	if (IsPlaying()) {		// // //
		int Channel = m_pInstRecorder->GetRecordChannel();
		if (Channel != -1 && m_pChannels[Channel] != nullptr)		// // //
			m_pInstRecorder->RecordInstrument(m_iPlayTicks, m_pTrackerView);
	}

	if (m_bHaltRequest) {
		// Halt has been requested, abort playback here
		auto l = Lock();
		HaltPlayer();
	}

	// Rendering
	if (m_bRendering && m_bRequestRenderStop)
		m_bStoppingRender = true;
	if (m_bStoppingRender) {
		if (!m_iDelayedEnd)
			StopRendering();
		else
			--m_iDelayedEnd;
	}

	if (m_iDelayedStart > 0) {
		--m_iDelayedStart;
		if (!m_iDelayedStart) {
			PostGuiMessage(WM_USER_PLAY, MODE_PLAY_START, m_iRenderTrack);
		}
	}

	// Check if a previewed sample should be removed
	if (m_pPreviewSample && PreviewDone()) {
		delete m_pPreviewSample;
		m_pPreviewSample = NULL;
	}
}

void CSoundGen::PlayChannelNotes()
{
	// Read notes
	for (int i = 0; i < CHANNELS; ++i) {		// // //
		int Index = m_pTrackerChannels[i]->GetID();
		int Channel = m_pDocument->GetChannelIndex(m_pTrackerChannels[Index]->GetID());
		if (Channel == -1) continue;

		// Run auto-arpeggio, if enabled
		int Arpeggio = m_pTrackerView->GetAutoArpeggio(Channel);
		if (Arpeggio > 0) {
			m_pChannels[Index]->Arpeggiate(Arpeggio);
		}

		// Check if new note data has been queued for playing
		if (m_pTrackerChannels[Index]->NewNoteData()) {
			stChanNote Note = m_pTrackerChannels[Index]->GetNote();
			PlayNote(Index, &Note, m_pDocument->GetEffColumns(m_iPlayTrack, Channel) + 1);
		}

		// Pitch wheel
		int Pitch = m_pTrackerChannels[Index]->GetPitch();
		m_pChannels[Index]->SetPitch(Pitch);

		// Update volume meters
		m_pTrackerChannels[Index]->SetVolumeMeter(m_pAPU->GetVol(m_pTrackerChannels[Index]->GetID()));
	}

	// Instrument sequence visualization
	// // //
}

void CSoundGen::UpdatePlayer()
{
	// Update player state

	if (m_bUpdateRow && !m_bHaltRequest)
		CheckControl();

	if (m_bPlaying) {
		if (m_iTempoAccum <= 0) {
			int TicksPerSec = m_pDocument->GetFrameRate();
			m_iTempoAccum += (m_iTempo ? 60 * TicksPerSec : m_iSpeed) - m_iTempoRemainder;		// // //
		}
		m_iTempoAccum -= m_iTempoDecrement;
	}
}

void CSoundGen::UpdateChannels()
{
	// Update channels
	for (int i = 0; i < CHANNELS; ++i) {
		if (m_pChannels[i] != NULL) {
			if (m_bHaltRequest)
				m_pChannels[i]->ResetChannel();
			else
				m_pChannels[i]->ProcessChannel();
		}
	}
}

void CSoundGen::UpdateAPU()
{
	// Write to APU registers
	// Copy wave changed flag
	m_bInternalWaveChanged = m_bWaveChanged;
	m_bWaveChanged = false;

	{
		auto l = DeferLock();
		if (l.try_lock()) {
			// Update APU channel registers
			unsigned int PrevChip = SNDCHIP_NONE;		// // // 050B
			for (int i = 0; i < CHANNELS; ++i) {
				if (m_pChannels[i] != NULL) {
					m_pChannels[i]->RefreshChannel();
					m_pChannels[i]->FinishTick();		// // //
					unsigned int Chip = m_pTrackerChannels[i]->GetChip();
					if (m_pDocument->ExpansionEnabled(Chip)) {
						int Delay = (Chip == PrevChip) ? 150 : 250;

						AddCyclesUnlessEndOfFrame(Delay);
						m_pAPU->Process();

						PrevChip = Chip;
					}
				}
			}
		#ifdef WRITE_VGM		// // //
			if (m_bPlaying)
				m_iRegisterStream.push(0x62);		// // //
		#endif

			// Finish the audio frame
			if (m_iConsumedCycles > m_iUpdateCycles) {
				throw std::runtime_error("overflowed vblank!");
			}

			m_pAPU->AddCycles(m_iUpdateCycles - m_iConsumedCycles);
			m_pAPU->Process();

			l.unlock();
		}
	}

	m_iConsumedCycles = 0;

#ifdef LOGGING
	if (m_bPlaying)
		m_pAPU->Log();
#endif
}

// End of overloaded functions

// Thread message handler

void CSoundGen::OnStartPlayer(WPARAM wParam, LPARAM lParam)
{
	BeginPlayer((play_mode_t)wParam, lParam);
}

void CSoundGen::OnSilentAll(WPARAM wParam, LPARAM lParam)
{
	MakeSilent();
}

void CSoundGen::OnLoadSettings(WPARAM wParam, LPARAM lParam)
{
	if (!ResetAudioDevice()) {
		TRACE("SoundGen: Failed to reset audio device!\n");
		if (m_pVisualizerWnd != NULL)
			m_pVisualizerWnd->ReportAudioProblem();
	}
}

void CSoundGen::OnStopPlayer(WPARAM wParam, LPARAM lParam)
{
	auto l = Lock();
	HaltPlayer();
}

void CSoundGen::OnResetPlayer(WPARAM wParam, LPARAM lParam)
{
	// Called when the selected song has changed

	if (IsPlaying())
		BeginPlayer(MODE_PLAY_START, wParam);		// // //

	m_iPlayTrack = wParam;
}

void CSoundGen::OnStartRender(WPARAM wParam, LPARAM lParam)
{
	auto l = Lock();
	ResetBuffer();
	m_bRequestRenderStart = false;
	m_bRequestRenderStop = false;
	m_bStoppingRender = false;		// // //
	m_bRendering = true;
	m_iDelayedStart = 5;	// Wait 5 frames until player starts
	m_iDelayedEnd = 5;
}

void CSoundGen::OnStopRender(WPARAM wParam, LPARAM lParam)
{
	StopRendering();
}

void CSoundGen::OnPreviewSample(WPARAM wParam, LPARAM lParam)
{
	PlaySample(reinterpret_cast<CDSample*>(wParam), LOWORD(lParam), HIWORD(lParam));
}

void CSoundGen::OnWriteAPU(WPARAM wParam, LPARAM lParam)
{
	m_pAPU->Write((uint16_t)wParam, (uint8_t)lParam);
}

void CSoundGen::OnCloseSound(WPARAM wParam, LPARAM lParam)
{
	CloseAudio();

	// Notification
	CEvent *pEvent = (CEvent*)wParam;
	if (pEvent != NULL && pEvent->IsKindOf(RUNTIME_CLASS(CEvent)))
		pEvent->SetEvent();
}

void CSoundGen::OnSetChip(WPARAM wParam, LPARAM lParam)
{
	int Chip = wParam;

	auto l = Lock();

	{
		auto config = CAPUConfig(m_pAPU);
		config.SetExternalSound(Chip);
	}

	// Enable internal channels after reset
	m_pAPU->Write(0x4015, 0x0F);
	m_pAPU->Write(0x4017, 0x00);

	// MMC5
	if (Chip & SNDCHIP_MMC5)
		m_pAPU->Write(0x5015, 0x03);
}

void CSoundGen::OnRemoveDocument(WPARAM wParam, LPARAM lParam)
{
	// Remove document and view pointers
	m_pDocument = NULL;
	m_pTrackerView = NULL;
	m_pInstRecorder->SetDumpCount(0);		// // //
	m_pInstRecorder->ReleaseCurrent();
	// m_pInstRecorder->ResetDumpInstrument();
	//if (*m_pDumpInstrument)		// // //
	//	(*m_pDumpInstrument)->Release();
	m_pInstRecorder->ResetRecordCache();
	TRACE("SoundGen: Document removed\n");
}

void CSoundGen::RegisterKeyState(int Channel, int Note)
{
	if (m_pTrackerView != NULL)
		m_pTrackerView->PostMessage(WM_USER_NOTE_EVENT, Channel, Note);
}

// FDS & N163

void CSoundGen::WaveChanged()
{
	// Call when FDS or N163 wave is altered from the instrument editor
	m_bWaveChanged = true;
}

bool CSoundGen::HasWaveChanged() const
{
	return m_bInternalWaveChanged;
}

void CSoundGen::SetNamcoMixing(bool bLinear)		// // //
{
	m_pAPU->SetNamcoMixing(theApp.GetSettings()->Emulation.bNamcoMixing);
}

// Player state functions

void CSoundGen::ReadPatternRow()
{
	const int Channels = m_pDocument->GetChannelCount();
	stChanNote NoteData;

	for (int i = 0; i < Channels; ++i) {
		if (m_pTrackerView->PlayerGetNote(m_iPlayTrack, m_iPlayFrame, i, m_iPlayRow, NoteData))
			QueueNote(i, NoteData, NOTE_PRIO_1);
	}
	if (m_bDoHalt) {		// // //
		m_bHaltRequest = true;
	}
}

void CSoundGen::PlayerStepRow()
{
	const int PatternLen = m_pDocument->GetPatternLength(m_iPlayTrack);

	if (++m_iPlayRow >= PatternLen) {
		m_iPlayRow = 0;
		if (!m_bPlayLooping)
			PlayerStepFrame();
	}

	++m_iRowsPlayed;		// // //

	m_bDirty = true;
}

void CSoundGen::PlayerStepFrame()
{
	const int Frames = m_pDocument->GetFrameCount(m_iPlayTrack);

	m_bFramePlayed[m_iPlayFrame] = true;

	if (m_iQueuedFrame == -1) {
		if (++m_iPlayFrame >= Frames)
			m_iPlayFrame = 0;
	}
	else {
		m_iPlayFrame = m_iQueuedFrame;
		m_iQueuedFrame = -1;
	}

	++m_iFramesPlayed;

	m_bDirty = true;
}

void CSoundGen::PlayerJumpTo(int Frame)
{
	const int Frames = m_pDocument->GetFrameCount(m_iPlayTrack);

	m_bFramePlayed[m_iPlayFrame] = true;

	m_iPlayFrame = Frame;

	if (m_iPlayFrame >= Frames)
		m_iPlayFrame = Frames - 1;

	m_iPlayRow = 0;

	++m_iFramesPlayed;
	++m_iRowsPlayed;		// // //

	m_bDirty = true;
}

void CSoundGen::PlayerSkipTo(int Row)
{
	const int Frames = m_pDocument->GetFrameCount(m_iPlayTrack);
	const int Rows = m_pDocument->GetPatternLength(m_iPlayTrack);

	m_bFramePlayed[m_iPlayFrame] = true;

	if (++m_iPlayFrame >= Frames)
		m_iPlayFrame = 0;

	m_iPlayRow = Row;

	if (m_iPlayRow >= Rows)
		m_iPlayRow = Rows - 1;

	++m_iFramesPlayed;
	++m_iRowsPlayed;		// // //

	m_bDirty = true;
}

void CSoundGen::QueueNote(int Channel, stChanNote &NoteData, note_prio_t Priority) const
{
	if (m_pDocument == NULL)
		return;

	// Queue a note for play
	m_pDocument->GetChannel(Channel)->SetNote(NoteData, Priority);
	theApp.GetMIDI()->WriteNote(Channel, NoteData.Note, NoteData.Octave, NoteData.Vol);
}

void CSoundGen::ForceReloadInstrument(int Channel)		// // //
{
	if (m_pDocument == NULL)
		return;
	m_pChannels[m_pDocument->GetChannel(Channel)->GetID()]->ForceReloadInstrument();
}

int	CSoundGen::GetPlayerRow() const
{
	return m_iPlayRow;
}

int CSoundGen::GetPlayerFrame() const
{
	return m_iPlayFrame;
}

int CSoundGen::GetPlayerTrack() const
{
	return m_iPlayTrack;
}

int CSoundGen::GetPlayerTicks() const
{
	return m_iPlayTicks;
}

void CSoundGen::MoveToFrame(int Frame)
{
	// Todo: synchronize
	m_iPlayFrame = Frame;
	m_iPlayRow = 0;
}

void CSoundGen::SetQueueFrame(int Frame)
{
	m_iQueuedFrame = Frame;
}

int CSoundGen::GetQueueFrame() const
{
	return m_iQueuedFrame;
}

// Verification

void CSoundGen::WriteRegister(uint16_t Reg, uint8_t Value)
{
#ifdef WRITE_VGM		// // //
	static int S5B_Port = 0; // TODO: elevate to full object status
	if (Reg >= 0x4000U && Reg <= 0x401FU) {
		m_iRegisterStream.push(0xB4);
		m_iRegisterStream.push(Reg & 0x1F);
		m_iRegisterStream.push(Value);
	}

	else if (Reg >= 0x4040U && Reg <= 0x407FU) {
		m_iRegisterStream.push(0xB4);
		m_iRegisterStream.push(Reg & 0x7F);
		m_iRegisterStream.push(Value);
	}
	else if (Reg >= 0x4080U && Reg <= 0x409EU) {
		m_iRegisterStream.push(0xB4);
		m_iRegisterStream.push((Reg & 0x1F) | 0x20);
		m_iRegisterStream.push(Value);
	}
	else if (Reg == 0x4023U) {
		m_iRegisterStream.push(0xB4);
		m_iRegisterStream.push(0x3F);
		m_iRegisterStream.push(Value);
	}

	else if (Reg == 0xC000)
		S5B_Port = Value;
	else if (Reg == 0xE000) {
		m_iRegisterStream.push(0xA0);
		m_iRegisterStream.push(S5B_Port);
		m_iRegisterStream.push(Value);
	}
#endif
}

CFTMComponentInterface *CSoundGen::GetDocumentInterface() const
{
	return static_cast<CFTMComponentInterface*>(m_pDocument);
}

void CSoundGen::SetSequencePlayPos(const CSequence *pSequence, int Pos)
{
	if (pSequence == m_pSequencePlayPos) {
		m_iSequencePlayPos = Pos;
		m_iSequenceTimeout = 5;
	}
}

int CSoundGen::GetSequencePlayPos(const CSequence *pSequence)
{
	if (m_pSequencePlayPos != pSequence)
		m_iSequencePlayPos = -1;

	if (m_iSequenceTimeout == 0)
		m_iSequencePlayPos = -1;
	else
		--m_iSequenceTimeout;

	int Ret = m_iSequencePlayPos;
	m_pSequencePlayPos = pSequence;
	return Ret;
}

void CSoundGen::SetMeterDecayRate(int Type) const		// // // 050B
{
	m_pAPU->SetMeterDecayRate(Type);
}

int CSoundGen::GetMeterDecayRate() const
{
	return m_pAPU->GetMeterDecayRate();
}

int CSoundGen::GetDefaultInstrument() const
{
	return ((CMainFrame*)theApp.m_pMainWnd)->GetSelectedInstrument();
}

// // // instrument recorder

CInstrument* CSoundGen::GetRecordInstrument() const
{
	return m_pInstRecorder->GetRecordInstrument(m_iPlayTicks);
}

void CSoundGen::ResetDumpInstrument()
{
	m_pInstRecorder->ResetDumpInstrument();
}

int CSoundGen::GetRecordChannel() const
{
	return m_pInstRecorder->GetRecordChannel();
}

void CSoundGen::SetRecordChannel(int Channel)
{
	m_pInstRecorder->SetRecordChannel(Channel);
}

stRecordSetting *CSoundGen::GetRecordSetting() const
{
	return m_pInstRecorder->GetRecordSetting();
}

void CSoundGen::SetRecordSetting(stRecordSetting *Setting)
{
	m_pInstRecorder->SetRecordSetting(Setting);
}
