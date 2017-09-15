/*
** FamiTracker - NES/Famicom sound tracker
** Copyright (C) 2005-2014  Jonathan Liss
**
** 0CC-FamiTracker is (C) 2014-2017 HertzDevil
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

//
// This file takes care of the NES sound playback
//
// TODO: 
//  - Break out actual player functions to a new class CPlayer
//  - Create new interface for CFamiTrackerView with thread-safe functions
//  - Same for CFamiTrackerDoc
//  - Perhaps this should be a worker thread and not GUI thread?
//

#include "SoundGen.h"
#include "FamiTracker.h"
#include "FTMComponentInterface.h"		// // //
#include "ChannelState.h"		// // //
#include "FamiTrackerDoc.h"
#include "FamiTrackerView.h"
#include "VisualizerWnd.h"
#include "MainFrm.h"
#include "DirectSound.h"
#include "WaveFile.h"		// // //
#include "APU/APU.h"
#include "ChannelHandler.h"
#include "ChannelsN163.h" // N163 channel count
#include "DSample.h"		// // //
#include "InstrumentRecorder.h"		// // //
#include "Settings.h"
#include "TrackerChannel.h"
#include "MIDI.h"
#include "ChannelFactory.h"		// // // test
#include "DetuneTable.h"		// // //
#include "Arpeggiator.h"		// // //
#include "TempoCounter.h"		// // //
#include "AudioDriver.h"		// // //
#include "WaveRenderer.h"		// // //

// Write period tables to files
//#define WRITE_PERIOD_FILES

// // // Write vibrato table to file
//#define WRITE_VIBRATO_FILE

// Write a file with the volume table
//#define WRITE_VOLUME_FILE

// // // Log VGM output (experimental)
//#define WRITE_VGM



// The depth of each vibrato level
const double CSoundGen::NEW_VIBRATO_DEPTH[] = {
	1.0, 1.5, 2.5, 4.0, 5.0, 7.0, 10.0, 12.0, 14.0, 17.0, 22.0, 30.0, 44.0, 64.0, 96.0, 128.0
};

const double CSoundGen::OLD_VIBRATO_DEPTH[] = {
	1.0, 1.0, 2.0, 3.0, 4.0, 7.0, 8.0, 15.0, 16.0, 31.0, 32.0, 63.0, 64.0, 127.0, 128.0, 255.0
};

IMPLEMENT_DYNCREATE(CSoundGen, CWinThread)

BEGIN_MESSAGE_MAP(CSoundGen, CWinThread)
	ON_THREAD_MESSAGE(WM_USER_SILENT_ALL, OnSilentAll)
	ON_THREAD_MESSAGE(WM_USER_LOAD_SETTINGS, OnLoadSettings)
	ON_THREAD_MESSAGE(WM_USER_PLAY, OnStartPlayer)
	ON_THREAD_MESSAGE(WM_USER_STOP, OnStopPlayer)
	ON_THREAD_MESSAGE(WM_USER_RESET, OnResetPlayer)
	ON_THREAD_MESSAGE(WM_USER_START_RENDER, OnStartRender)
	ON_THREAD_MESSAGE(WM_USER_STOP_RENDER, OnStopRender)
	ON_THREAD_MESSAGE(WM_USER_PREVIEW_SAMPLE, OnPreviewSample)
	ON_THREAD_MESSAGE(WM_USER_WRITE_APU, OnWriteAPU)
	ON_THREAD_MESSAGE(WM_USER_CLOSE_SOUND, OnCloseSound)
	ON_THREAD_MESSAGE(WM_USER_SET_CHIP, OnSetChip)
	ON_THREAD_MESSAGE(WM_USER_REMOVE_DOCUMENT, OnRemoveDocument)
END_MESSAGE_MAP()



// CSoundGen

CSoundGen::CSoundGen() :
	m_pAPU(std::make_unique<CAPU>()),		// // //
	m_pDocument(NULL),
	m_pTrackerView(NULL),
	m_bPlaying(false),
	m_bHaltRequest(false),
	m_bDoHalt(false),		// // //
	m_pInstRecorder(std::make_unique<CInstrumentRecorder>(this)),		// // //
	m_bWaveChanged(0),
	m_iMachineType(NTSC),
	m_bRunning(false),
	m_hInterruptEvent(NULL),
	m_iQueuedFrame(-1),
	m_iPlayFrame(0),
	m_iPlayRow(0),
	m_iPlayTrack(0),
	m_iTicksPlayed(0),
	m_iRegisterStream(),		// // //
	m_pSequencePlayPos(NULL),
	m_iSequencePlayPos(0),
	m_iSequenceTimeout(0),
	m_iBPMCachePosition(0)		// // //
{
	TRACE("SoundGen: Object created\n");

	// Create all kinds of channels
	CreateChannels();
}

CSoundGen::~CSoundGen()
{
}

//
// Object initialization, local
//

void CSoundGen::CreateChannels()
{
	// Only called once!

	// Clear all channels
	m_pChannels.clear();		// // //
	m_pTrackerChannels.clear();

	// 2A03/2A07
	// // // Short header names
	AssignChannel(std::make_unique<CTrackerChannel>(_T("Pulse 1"), _T("PU1"), SNDCHIP_NONE, CHANID_SQUARE1));
	AssignChannel(std::make_unique<CTrackerChannel>(_T("Pulse 2"), _T("PU2"), SNDCHIP_NONE, CHANID_SQUARE2));
	AssignChannel(std::make_unique<CTrackerChannel>(_T("Triangle"), _T("TRI"), SNDCHIP_NONE, CHANID_TRIANGLE));
	AssignChannel(std::make_unique<CTrackerChannel>(_T("Noise"), _T("NOI"), SNDCHIP_NONE, CHANID_NOISE));
	AssignChannel(std::make_unique<CTrackerChannel>(_T("DPCM"), _T("DMC"), SNDCHIP_NONE, CHANID_DPCM));

	// Konami VRC6
	AssignChannel(std::make_unique<CTrackerChannel>(_T("VRC6 Pulse 1"), _T("V1"), SNDCHIP_VRC6, CHANID_VRC6_PULSE1));
	AssignChannel(std::make_unique<CTrackerChannel>(_T("VRC6 Pulse 2"), _T("V2"), SNDCHIP_VRC6, CHANID_VRC6_PULSE2));
	AssignChannel(std::make_unique<CTrackerChannel>(_T("Sawtooth"), _T("SAW"), SNDCHIP_VRC6, CHANID_VRC6_SAWTOOTH));

	// // // Nintendo MMC5
	AssignChannel(std::make_unique<CTrackerChannel>(_T("MMC5 Pulse 1"), _T("PU3"), SNDCHIP_MMC5, CHANID_MMC5_SQUARE1));
	AssignChannel(std::make_unique<CTrackerChannel>(_T("MMC5 Pulse 2"), _T("PU4"), SNDCHIP_MMC5, CHANID_MMC5_SQUARE2));
	AssignChannel(std::make_unique<CTrackerChannel>(_T("MMC5 PCM"), _T("PCM"), SNDCHIP_MMC5, CHANID_MMC5_VOICE)); // null channel handler

	// Namco N163
	AssignChannel(std::make_unique<CTrackerChannel>(_T("Namco 1"), _T("N1"), SNDCHIP_N163, CHANID_N163_CH1));
	AssignChannel(std::make_unique<CTrackerChannel>(_T("Namco 2"), _T("N2"), SNDCHIP_N163, CHANID_N163_CH2));
	AssignChannel(std::make_unique<CTrackerChannel>(_T("Namco 3"), _T("N3"), SNDCHIP_N163, CHANID_N163_CH3));
	AssignChannel(std::make_unique<CTrackerChannel>(_T("Namco 4"), _T("N4"), SNDCHIP_N163, CHANID_N163_CH4));
	AssignChannel(std::make_unique<CTrackerChannel>(_T("Namco 5"), _T("N5"), SNDCHIP_N163, CHANID_N163_CH5));
	AssignChannel(std::make_unique<CTrackerChannel>(_T("Namco 6"), _T("N6"), SNDCHIP_N163, CHANID_N163_CH6));
	AssignChannel(std::make_unique<CTrackerChannel>(_T("Namco 7"), _T("N7"), SNDCHIP_N163, CHANID_N163_CH7));
	AssignChannel(std::make_unique<CTrackerChannel>(_T("Namco 8"), _T("N8"), SNDCHIP_N163, CHANID_N163_CH8));

	// Nintendo FDS
	AssignChannel(std::make_unique<CTrackerChannel>(_T("FDS"), _T("FDS"), SNDCHIP_FDS, CHANID_FDS));

	// Konami VRC7
	AssignChannel(std::make_unique<CTrackerChannel>(_T("FM Channel 1"), _T("FM1"), SNDCHIP_VRC7, CHANID_VRC7_CH1));
	AssignChannel(std::make_unique<CTrackerChannel>(_T("FM Channel 2"), _T("FM2"), SNDCHIP_VRC7, CHANID_VRC7_CH2));
	AssignChannel(std::make_unique<CTrackerChannel>(_T("FM Channel 3"), _T("FM3"), SNDCHIP_VRC7, CHANID_VRC7_CH3));
	AssignChannel(std::make_unique<CTrackerChannel>(_T("FM Channel 4"), _T("FM4"), SNDCHIP_VRC7, CHANID_VRC7_CH4));
	AssignChannel(std::make_unique<CTrackerChannel>(_T("FM Channel 5"), _T("FM5"), SNDCHIP_VRC7, CHANID_VRC7_CH5));
	AssignChannel(std::make_unique<CTrackerChannel>(_T("FM Channel 6"), _T("FM6"), SNDCHIP_VRC7, CHANID_VRC7_CH6));

	// // // Sunsoft 5B
	AssignChannel(std::make_unique<CTrackerChannel>(_T("5B Square 1"), _T("5B1"), SNDCHIP_S5B, CHANID_S5B_CH1));
	AssignChannel(std::make_unique<CTrackerChannel>(_T("5B Square 2"), _T("5B2"), SNDCHIP_S5B, CHANID_S5B_CH2));
	AssignChannel(std::make_unique<CTrackerChannel>(_T("5B Square 3"), _T("5B3"), SNDCHIP_S5B, CHANID_S5B_CH3));
}

void CSoundGen::AssignChannel(std::unique_ptr<CTrackerChannel> pTrackerChannel)		// // //
{
	static CChannelFactory F {}; // test
	chan_id_t ID = pTrackerChannel->GetID();

	CChannelHandler *pRenderer = F.Produce(ID);
	if (pRenderer)
		pRenderer->SetChannelID(ID);

	m_pTrackerChannels.push_back(std::move(pTrackerChannel));		// // //
	m_pChannels.emplace_back(pRenderer);
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
	m_pTempoCounter = std::make_unique<CTempoCounter>(*pDoc);		// // //

	// Setup all channels
	for (auto &ptr : m_pChannels)		// // //
		if (ptr)
			ptr->InitChannel(m_pAPU.get(), m_iVibratoTable, this);
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
	m_Arpeggiator = &m_pTrackerView->GetArpeggiator();		// // //
}

void CSoundGen::RemoveDocument()
{
	// Removes both the document and view from this object

	// Called from main thread
	ASSERT(GetCurrentThreadId() == theApp.m_nThreadID);
	ASSERT(m_pDocument != NULL);
	ASSERT(m_hThread != NULL);

	// Player cannot play when removing the document
	StopPlayer();
	WaitForStop();

	PostThreadMessage(WM_USER_REMOVE_DOCUMENT, 0, 0);

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

	m_csVisualizerWndLock.Lock();
	m_pVisualizerWnd = pWnd;
	m_csVisualizerWndLock.Unlock();
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
	int i = 0;		// // //
	for (auto &x : m_pTrackerChannels) {
		int ID = x->GetID();		// // //
		if (m_pChannels[i] && ((x->GetChip() & Chip) || (i <= CHANID_DPCM))			// // //
						   && (i >= CHANID_FDS || i < CHANID_N163_CH1 + pDoc->GetNamcoChannels())) {
			pDoc->RegisterChannel(x.get(), ID, x->GetChip());
		}
		++i;
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

	PostThreadMessage(WM_USER_SET_CHIP, Chip, 0);
}

CChannelHandler *CSoundGen::GetChannel(int Index) const
{
	return m_pChannels[Index].get();
}

void CSoundGen::DocumentPropertiesChanged(CFamiTrackerDoc *pDocument)
{
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
	for (size_t i = 0; i < m_pChannels.size(); ++i) {
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

	for (auto &ch : m_pChannels) if (ch) {
		ch->SetVibratoStyle(pDocument->GetVibratoStyle());		// // //
		ch->SetLinearPitch(pDocument->GetLinearPitch());
		if (auto pChan = dynamic_cast<CChannelHandlerN163 *>(ch.get()))
			pChan->SetChannelCount(pDocument->GetNamcoChannels());
	}
}

//
// Interface functions
//

void CSoundGen::StartPlayer(play_mode_t Mode, int Track)
{
	if (!m_hThread)
		return;

	PostThreadMessage(WM_USER_PLAY, Mode, Track);
}

void CSoundGen::StopPlayer()
{
	if (!m_hThread)
		return;

	PostThreadMessage(WM_USER_STOP, 0, 0);
}

void CSoundGen::ResetPlayer(int Track)
{
	if (!m_hThread)
		return;

	PostThreadMessage(WM_USER_RESET, Track, 0);
}

void CSoundGen::LoadSettings()
{
	if (!m_hThread)
		return;

	PostThreadMessage(WM_USER_LOAD_SETTINGS, 0, 0);
}

void CSoundGen::SilentAll()
{
	if (!m_hThread)
		return;

	PostThreadMessage(WM_USER_SILENT_ALL, 0, 0);
}

void CSoundGen::WriteAPU(int Address, char Value)
{
	if (!m_hThread)
		return;

	// Direct APU interface
	PostThreadMessage(WM_USER_WRITE_APU, (WPARAM)Address, (LPARAM)Value);
}

void CSoundGen::PreviewSample(const CDSample *pSample, int Offset, int Pitch)		// // //
{
	if (!m_hThread)
		return;

	// Preview a DPCM sample. If the name of sample is null, 
	// the sample will be removed after played
	PostThreadMessage(WM_USER_PREVIEW_SAMPLE, (WPARAM)pSample, MAKELPARAM(Offset, Pitch));
}

void CSoundGen::CancelPreviewSample()
{
	// Remove references to selected sample.
	// This must be done if a sample is about to be deleted!
	m_pAPU->ClearSample();		// // //
}

bool CSoundGen::IsRunning() const
{
	return (m_hThread != NULL) && m_bRunning;
}

//// Sound buffer handling /////////////////////////////////////////////////////////////////////////////////

bool CSoundGen::InitializeSound(HWND hWnd)
{
	// Initialize sound, this is only called once!
	// Start with NTSC by default

	// Called from main thread
	ASSERT(GetCurrentThread() == theApp.m_hThread);
	ASSERT(!m_pDSound);

	// Event used to interrupt the sound buffer synchronization
	m_hInterruptEvent = ::CreateEvent(NULL, FALSE, FALSE, NULL);

	// Create DirectSound object
	m_pDSound = std::make_unique<CDSound>(hWnd, m_hInterruptEvent);		// // //

	// Out of memory
	if (!m_pDSound)
		return false;

	m_pDSound->EnumerateDevices();

	// Start thread when audio is done
	ResumeThread();

	return true;
}

void CSoundGen::Interrupt() const
{
	if (m_hInterruptEvent != NULL)
		::SetEvent(m_hInterruptEvent);
}

bool CSoundGen::ResetAudioDevice()
{
	// Setup sound, return false if failed
	//
	// The application must be able to continue even if this fails
	//

	// Called from player thread
	ASSERT(GetCurrentThreadId() == m_nThreadID);
	ASSERT(m_pDSound != NULL);

	CSettings *pSettings = theApp.GetSettings();

	unsigned int SampleSize = pSettings->Sound.iSampleSize;
	unsigned int SampleRate = pSettings->Sound.iSampleRate;
	unsigned int BufferLen	= pSettings->Sound.iBufferLength;
	unsigned int Device		= pSettings->Sound.iDevice;

	if (m_pAudioDriver)
		m_pAudioDriver->CloseAudioDevice();		// // //

	if (Device >= m_pDSound->GetDeviceCount()) {
		// Invalid device detected, reset to 0
		Device = 0;
		pSettings->Sound.iDevice = 0;
	}

	// Reinitialize direct sound
	if (!m_pDSound->SetupDevice(Device)) {
		AfxMessageBox(IDS_DSOUND_ERROR, MB_ICONERROR);
		return false;
	}

	int iBlocks = 2;	// default = 2

	// Create more blocks if a bigger buffer than 100ms is used to reduce lag
	if (BufferLen > 100)
		iBlocks += (BufferLen / 66);

	m_pAudioDriver = std::make_unique<CAudioDriver>(*this,
		std::move(m_pDSound->OpenChannel(SampleRate, SampleSize, 1, BufferLen, iBlocks)), SampleSize);

	// Channel failed
	if (!m_pAudioDriver || !m_pAudioDriver->IsAudioDeviceOpen()) {
		AfxMessageBox(IDS_DSOUND_BUFFER_ERROR, MB_ICONERROR);
		return false;
	}

	// Sample graph rate
	m_csVisualizerWndLock.Lock();
	if (m_pVisualizerWnd)
		m_pVisualizerWnd->SetSampleRate(SampleRate);
	m_csVisualizerWndLock.Unlock();

	m_pAPU->SetCallback(*m_pAudioDriver);
	if (!m_pAPU->SetupSound(SampleRate, 1, (m_iMachineType == NTSC) ? MACHINE_NTSC : MACHINE_PAL))
		return false;

	m_pAPU->SetChipLevel(CHIP_LEVEL_APU1, float(pSettings->ChipLevels.iLevelAPU1 / 10.0f));
	m_pAPU->SetChipLevel(CHIP_LEVEL_APU2, float(pSettings->ChipLevels.iLevelAPU2 / 10.0f));
	m_pAPU->SetChipLevel(CHIP_LEVEL_VRC6, float(pSettings->ChipLevels.iLevelVRC6 / 10.0f));
	m_pAPU->SetChipLevel(CHIP_LEVEL_VRC7, float(pSettings->ChipLevels.iLevelVRC7 / 10.0f));
	m_pAPU->SetChipLevel(CHIP_LEVEL_MMC5, float(pSettings->ChipLevels.iLevelMMC5 / 10.0f));
	m_pAPU->SetChipLevel(CHIP_LEVEL_FDS, float(pSettings->ChipLevels.iLevelFDS / 10.0f));
	m_pAPU->SetChipLevel(CHIP_LEVEL_N163, float(pSettings->ChipLevels.iLevelN163 / 10.0f));
	m_pAPU->SetChipLevel(CHIP_LEVEL_S5B, float(pSettings->ChipLevels.iLevelS5B / 10.0f));
/*
	m_pAPU->SetChipLevel(SNDCHIP_NONE, 0);//pSettings->ChipLevels.iLevel2A03);
	m_pAPU->SetChipLevel(SNDCHIP_VRC6, 0);//pSettings->ChipLevels.iLevelVRC6);
	m_pAPU->SetChipLevel(SNDCHIP_VRC7, 0);//pSettings->ChipLevels.iLevelVRC7);
	m_pAPU->SetChipLevel(SNDCHIP_MMC5, 0);//pSettings->ChipLevels.iLevelMMC5);
	m_pAPU->SetChipLevel(SNDCHIP_FDS, 0);//pSettings->ChipLevels.iLevelFDS);
//	m_pAPU->SetChipLevel(SNDCHIP_N163, pSettings->ChipLevels.iLevelN163);
//	m_pAPU->SetChipLevel(SNDCHIP_S5B, pSettings->ChipLevels.iLevelS5B);
*/
	// Update blip-buffer filtering 
	m_pAPU->SetupMixer(pSettings->Sound.iBassFilter, pSettings->Sound.iTrebleFilter,
					   pSettings->Sound.iTrebleDamping, pSettings->Sound.iMixVolume);

	TRACE("SoundGen: Created sound channel with params: %i Hz, %i bits, %i ms (%i blocks)\n", SampleRate, SampleSize, BufferLen, iBlocks);

	return true;
}

void CSoundGen::CloseAudio()
{
	// Called from player thread
	ASSERT(GetCurrentThreadId() == m_nThreadID);

	m_pAudioDriver->CloseAudioDevice();		// // //

	if (m_pDSound) {
		m_pDSound->CloseDevice();
		m_pDSound.reset();		// // //
	}

	if (m_hInterruptEvent) {
		::CloseHandle(m_hInterruptEvent);
		m_hInterruptEvent = NULL;
	}
}

void CSoundGen::ResetBuffer()
{
	// Called from player thread
	ASSERT(GetCurrentThreadId() == m_nThreadID);

	m_pAudioDriver->Reset();		// // //
	m_pAPU->Reset();
}

void CSoundGen::FlushBuffer(int16_t *pBuffer, uint32_t Size)
{
	// Callback method from emulation

	// May only be called from sound player thread
	ASSERT(GetCurrentThreadId() == m_nThreadID);

	m_pAudioDriver->FlushBuffer(pBuffer, Size);		// // //
}

// // //
CDSound *CSoundGen::GetSoundInterface() const {
	return m_pDSound.get();
}

CAudioDriver *CSoundGen::GetAudioDriver() const {
	return m_pAudioDriver.get();
}

bool CSoundGen::PlayBuffer()
{
	ASSERT(GetCurrentThreadId() == m_nThreadID);

	if (IsRendering()) {
		auto [pBuf, size] = m_pAudioDriver->ReleaseSoundBuffer();		// // //
		m_pWaveRenderer->FlushBuffer(pBuf, size);		// // //
		return true;
	}

	if (!m_pAudioDriver->DoPlayBuffer())
		return false;

	// // // Draw graph
	if (!IsBackgroundTask()) {
		auto [pBuf, size] = m_pAudioDriver->ReleaseGraphBuffer();
		m_csVisualizerWndLock.Lock();
		if (m_pVisualizerWnd)
			m_pVisualizerWnd->FlushSamples(pBuf, size);
		m_csVisualizerWndLock.Unlock();
	}

	return true;
}

unsigned int CSoundGen::GetFrameRate()
{
	return std::exchange(m_iFrameCounter, 0);		// // //
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
	ASSERT(GetCurrentThreadId() == m_nThreadID);
	ASSERT(m_pDocument != NULL);
	ASSERT(m_pTrackerView != NULL);

	if (!m_pDocument || !m_pAudioDriver->IsAudioDeviceOpen() || !m_pDocument->IsFileLoaded())		// // //
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
	m_iTicksPlayed		= 0;
	m_iFramesPlayed		= 0;
	m_iRowsPlayed		= 0;		// // //
	m_iJumpToPattern	= -1;
	m_iSkipToRow		= -1;
	m_iPlayMode			= Mode;
	m_iPlayTrack		= Track;

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
		m_pTempoCounter->LoadSoundState(*State);
		m_iLastHighlight = m_pDocument->GetHighlightAt(GetPlayerTrack(), Frame, Row).First;
		for (int i = 0, n = m_pDocument->GetChannelCount(); i < n; ++i) {
			for (size_t j = 0; j < m_pTrackerChannels.size(); ++j)		// // // pick this out later
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
static std::string GetStateString(const stChannelState &State)
{
	std::string log("Inst.: ");
	if (State.Instrument == MAX_INSTRUMENTS)
		log += "None";
	else
		log += {hex(State.Instrument >> 4), hex(State.Instrument)};
	log += "        Vol.: ";
	log += hex(State.Volume >= MAX_VOLUME ? 0xF : State.Volume);
	log += "        Active effects:";

	std::string effStr;

	const effect_t SLIDE_EFFECT = State.Effect[EF_ARPEGGIO] >= 0 ? EF_ARPEGGIO :
								  State.Effect[EF_PORTA_UP] >= 0 ? EF_PORTA_UP :
								  State.Effect[EF_PORTA_DOWN] >= 0 ? EF_PORTA_DOWN :
								  EF_PORTAMENTO;
	for (const auto &x : {SLIDE_EFFECT, EF_VIBRATO, EF_TREMOLO, EF_VOLUME_SLIDE, EF_PITCH, EF_DUTY_CYCLE}) {
		int p = State.Effect[x];
		if (p < 0) continue;
		if (p == 0 && x != EF_PITCH) continue;
		if (p == 0x80 && x == EF_PITCH) continue;
		effStr += MakeCommandString(x, p);
	}

	if ((State.ChannelIndex >= CHANID_SQUARE1 && State.ChannelIndex <= CHANID_SQUARE2) ||
			State.ChannelIndex == CHANID_NOISE ||
		(State.ChannelIndex >= CHANID_MMC5_SQUARE1 && State.ChannelIndex <= CHANID_MMC5_SQUARE2))
		for (const auto &x : {EF_VOLUME}) {
			int p = State.Effect[x];
			if (p < 0) continue;
			effStr += MakeCommandString(x, p);
		}
	else if (State.ChannelIndex == CHANID_TRIANGLE)
		for (const auto &x : {EF_VOLUME, EF_NOTE_CUT}) {
			int p = State.Effect[x];
			if (p < 0) continue;
			effStr += MakeCommandString(x, p);
		}
	else if (State.ChannelIndex == CHANID_DPCM)
		for (const auto &x : {EF_SAMPLE_OFFSET, /*EF_DPCM_PITCH*/}) {
			int p = State.Effect[x];
			if (p <= 0) continue;
			effStr += MakeCommandString(x, p);
		}
	else if (State.ChannelIndex >= CHANID_VRC7_CH1 && State.ChannelIndex <= CHANID_VRC7_CH6)
		for (const auto &x : VRC7_EFFECTS) {
			int p = State.Effect[x];
			if (p < 0) continue;
			effStr += MakeCommandString(x, p);
		}
	else if (State.ChannelIndex == CHANID_FDS)
		for (const auto &x : FDS_EFFECTS) {
			int p = State.Effect[x];
			if (p < 0 || (x == EF_FDS_MOD_BIAS && p == 0x80)) continue;
			effStr += MakeCommandString(x, p);
		}
	else if (State.ChannelIndex >= CHANID_S5B_CH1 && State.ChannelIndex <= CHANID_S5B_CH3)
		for (const auto &x : S5B_EFFECTS) {
			int p = State.Effect[x];
			if (p < 0) continue;
			effStr += MakeCommandString(x, p);
		}
	else if (State.ChannelIndex >= CHANID_N163_CH1 && State.ChannelIndex <= CHANID_N163_CH8)
		for (const auto &x : N163_EFFECTS) {
			int p = State.Effect[x];
			if (p < 0 || (x == EF_N163_WAVE_BUFFER && p == 0x7F)) continue;
			effStr += MakeCommandString(x, p);
		}
	if (State.Effect_LengthCounter >= 0)
		effStr += MakeCommandString(EF_VOLUME, State.Effect_LengthCounter);
	if (State.Effect_AutoFMMult >= 0)
		effStr += MakeCommandString(EF_FDS_MOD_DEPTH, State.Effect_AutoFMMult);

	if (!effStr.size()) effStr = " None";
	log += effStr;
	return log;
}

std::string CSoundGen::RecallChannelState(int Channel) const		// // //
{
	if (IsPlaying())
		return m_pChannels[Channel]->GetStateString();
	int Frame = m_pTrackerView->GetSelectedFrame();
	int Row = m_pTrackerView->GetSelectedRow();
	std::string str;

	if (stFullState *State = m_pDocument->RetrieveSoundState(GetPlayerTrack(), Frame, Row, Channel)) {
		str = GetStateString(State->State[m_pDocument->GetChannelIndex(Channel)]);
		if (State->Tempo >= 0)
			str += "        Tempo: " + std::to_string(State->Tempo);
		if (State->Speed >= 0) {
			if (State->GroovePos >= 0) {
				str += "        Groove: ";
				str += {hex(State->Speed >> 4), hex(State->Speed)};
				CGroove *Groove = m_pDocument->GetGroove(State->Speed);
				const unsigned char Size = Groove->GetSize();
				for (unsigned char i = 0; i < Size; i++)
					str += ' ' + std::to_string(Groove->GetEntry((i + State->GroovePos) % Size));
			}
			else
				str += "        Speed: " + std::to_string(State->Speed);
		}
		delete State;
	}

	return str;
}

void CSoundGen::HaltPlayer()
{
	// Called from player thread
	ASSERT(GetCurrentThreadId() == m_nThreadID);

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
	if (m_pTrackerView) {
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
	ASSERT(GetCurrentThreadId() == m_nThreadID);

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

void CSoundGen::AddCycles(int Count)
{
	// Called from player thread
	ASSERT(GetCurrentThreadId() == m_nThreadID);

	// Add APU cycles
	m_iConsumedCycles += Count;
	m_pAPU->AddTime(Count);
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

void CSoundGen::MakeSilent()
{
	// Called from player thread
	ASSERT(GetCurrentThreadId() == m_nThreadID);

	m_pAPU->Reset();
	m_pAPU->ClearSample();		// // //

	for (auto &ch : m_pChannels)
		if (ch)
			ch->ResetChannel();
	for (auto &ch : m_pTrackerChannels)
		if (ch)
			ch->Reset();
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

	m_pTempoCounter->LoadTempo(m_iPlayTrack);		// // //
	m_iLastHighlight = m_pDocument->GetHighlight().First;		// // //
}

void CSoundGen::SetHighlightRows(int Rows)		// // //
{
	m_iLastHighlight = Rows;
}

// Return current tempo setting in BPM
float CSoundGen::GetTempo() const
{
	return static_cast<float>(m_pTempoCounter->GetTempo());		// // //
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

// // //

bool CSoundGen::IsPlaying() const {
	return m_bPlaying;
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
		CSingleLock l(&m_csAPULock, TRUE);		// // //
		m_pAPU->ChangeMachineRate(m_iMachineType == NTSC ? MACHINE_NTSC : MACHINE_PAL, Rate);		// // //
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
		unsigned char EffParam = NoteData->EffParam[i];
		switch (NoteData->EffNumber[i]) {
			// Fxx: Sets speed to xx
			case EF_SPEED:
				m_pTempoCounter->DoFxx(EffParam ? EffParam : 1);		// // //
				break;
				
			// Oxx: Sets groove to xx
			case EF_GROOVE:		// // //
				m_pTempoCounter->DoOxx(EffParam % MAX_GROOVE);		// // //
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
				if (IsRendering()) {
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

bool CSoundGen::RenderToFile(LPTSTR pFile, const std::shared_ptr<CWaveRenderer> &pRender)		// // //
{
	// Called from main thread
	ASSERT(GetCurrentThreadId() == theApp.m_nThreadID);
	ASSERT(m_pDocument != NULL);

	if (IsPlaying()) {
		//HaltPlayer();
		m_bHaltRequest = true;
		WaitForStop();
	}

	m_pWaveRenderer = pRender;		// // //
	ASSERT(m_pWaveRenderer);

	if (auto pWave = std::make_unique<CWaveFile>(); pWave &&		// // //
		pWave->OpenFile(pFile, theApp.GetSettings()->Sound.iSampleRate, theApp.GetSettings()->Sound.iSampleSize, 1)) {
		m_pWaveRenderer->SetOutputFile(std::move(pWave));
		PostThreadMessage(WM_USER_START_RENDER, 0, 0);
		return true;
	}

	AfxMessageBox(IDS_FILE_OPEN_ERROR);
	return false;
}

// // //
void CSoundGen::StartRendering() {
	ResetBuffer();
	m_pWaveRenderer->Start();
}

void CSoundGen::StopRendering()
{
	// Called from player thread
	ASSERT(GetCurrentThreadId() == m_nThreadID);
	ASSERT(IsRendering());

	if (!IsRendering())
		return;

	m_bPlaying = false;
	m_pWaveRenderer.reset();		// // //
	m_iPlayFrame = 0;
	m_iPlayRow = 0;

	ResetBuffer();
	HaltPlayer();		// // //
	ResetAPU();		// // //
}

bool CSoundGen::IsRendering() const
{
	return m_pWaveRenderer && m_pWaveRenderer->Started() && !m_pWaveRenderer->Finished();		// // //
}

bool CSoundGen::IsBackgroundTask() const
{
	return IsRendering();
}

// DPCM handling

void CSoundGen::PlaySample(const CDSample *pSample, int Offset, int Pitch)
{
	m_pPreviewSample.reset();		// // //

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
		m_pPreviewSample.reset(pSample);		// // //
}

bool CSoundGen::PreviewDone() const
{
	return (m_pAPU->DPCMPlaying() == false);
}

bool CSoundGen::WaitForStop() const
{
	// Wait for player to stop, timeout = 4s
	// The player must have received the stop command or this will fail

	ASSERT(GetCurrentThreadId() != m_nThreadID);

	//return ::WaitForSingleObject(m_hIsPlaying, 4000) == WAIT_OBJECT_0;

	for (int i = 0; i < 40 && IsPlaying(); ++i)
		Sleep(100);

	return !IsPlaying();	// return false if still playing
}

//
// Overloaded functions
//

BOOL CSoundGen::InitInstance()
{
	//
	// Setup the sound player object, called when thread is started
	//

	ASSERT(m_pDocument != NULL);
	ASSERT(m_pTrackerView != NULL);

	// First check if thread creation should be cancelled
	// This will occur when no sound object is available
	
	if (m_pDSound == NULL)
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
	m_pTempoCounter = std::make_unique<CTempoCounter>(*m_pDocument);		// // //

	TRACE("SoundGen: Created thread (0x%04x)\n", m_nThreadID);

	SetThreadPriority(THREAD_PRIORITY_TIME_CRITICAL);

	m_iFrameCounter = 0;

//	SetupChannels();

	return TRUE;
}

int CSoundGen::ExitInstance()
{
	// Shutdown the thread

	TRACE("SoundGen: Closing thread (0x%04x)\n", m_nThreadID);

	// Make sure sound interface is shut down
	CloseAudio();

	theApp.RemoveSoundGenerator();

	m_bRunning = false;

	return CWinThread::ExitInstance();
}

BOOL CSoundGen::OnIdle(LONG lCount)
{
	//
	// Main loop for audio playback thread
	//

	if (CWinThread::OnIdle(lCount))
		return TRUE;

	if (!m_pDocument || !m_pAudioDriver->IsAudioDeviceOpen() || !m_pDocument->IsFileLoaded())		// // //
		return TRUE;

	++m_iFrameCounter;

	// Access the document object, skip if access wasn't granted to avoid gaps in audio playback
	if (m_pDocument->LockDocument(0)) {
		DocumentHandleTick();		// // //
		m_pDocument->UnlockDocument();
	}

	// Update APU registers
	UpdateAPU();

	if (IsPlaying()) {		// // //
		int Channel = m_pInstRecorder->GetRecordChannel();
		if (Channel != -1 && m_pChannels[Channel])		// // //
			m_pInstRecorder->RecordInstrument(m_iTicksPlayed, m_pTrackerView);
	}

	if (m_bHaltRequest) {
		// Halt has been requested, abort playback here
		HaltPlayer();
	}

	// Rendering
	if (m_pWaveRenderer)		// // //
		if (m_pWaveRenderer->ShouldStopRender())
			StopRendering();
		else if (m_pWaveRenderer->ShouldStartPlayer())
			PostThreadMessage(WM_USER_PLAY, MODE_PLAY_START, m_pWaveRenderer->GetRenderTrack());

	// Check if a previewed sample should be removed
	if (m_pPreviewSample && PreviewDone())
		m_pPreviewSample.reset();		// // //

	return TRUE;
}

// // //
void CSoundGen::DocumentHandleTick() {
	// Called from player thread
	ASSERT(GetCurrentThreadId() == m_nThreadID);
	ASSERT(m_pDocument != NULL);
	ASSERT(m_pTrackerView != NULL);

	if (IsPlaying()) {
		if (IsRendering())
			m_pWaveRenderer->Tick();

		++m_iTicksPlayed;		// // //
		++m_iRowTickCount;		// // // 050B
		int SteppedRows = 0;		// // //

		// Fetch next row
		if (m_pTempoCounter->CanStepRow()) {
			// Enable this to skip rows on high tempos
//			while (m_pTempoCounter->CanStepRow())  {
			++SteppedRows;
			m_pTempoCounter->StepRow();		// // //
			if (IsRendering())
				m_pWaveRenderer->StepRow();		// // //
//			}
			ReadPatternRow(); // global commands should be processed here

			if (auto pMark = m_pDocument->GetBookmarkAt(m_iPlayTrack, m_iPlayFrame, m_iPlayRow))		// // //
				if (pMark->m_Highlight.First != -1)
					m_iLastHighlight = pMark->m_Highlight.First;

			// // // 050B
			m_fBPMCacheValue[m_iBPMCachePosition] = GetTempo();		// // // 050B
			m_iBPMCacheTicks[m_iBPMCachePosition] = m_iRowTickCount;
			m_iRowTickCount = 0;
			++m_iBPMCachePosition %= AVERAGE_BPM_SIZE;
		}
		m_pTempoCounter->Tick();		// // //

		if (IsRendering() && m_pWaveRenderer->ShouldStopPlayer())		// // //
			m_bHaltRequest = true;

		// Update player
		if (SteppedRows > 0 && !m_bHaltRequest) {
			if (m_bDoHalt)		// // //
				++m_iRowsPlayed;
			else {
				// Jump
				if (m_iJumpToPattern != -1)
					PlayerJumpTo(m_bPlayLooping ? m_iPlayFrame : m_iJumpToPattern);		// // //
				// Skip
				else if (m_iSkipToRow != -1)
					PlayerSkipTo(m_bPlayLooping ? m_iPlayFrame : m_iSkipToRow);
				// or just move on
				else
					while (SteppedRows--)
						PlayerStepRow();
			}

			m_iJumpToPattern = -1;
			m_iSkipToRow = -1;

			if (!m_bDoHalt && !IsBackgroundTask() && m_pTrackerView)		// // //
				m_pTrackerView->PostMessage(WM_USER_PLAYER, m_iPlayFrame, m_iPlayRow);
		}
	}

	// View callback
	if (theApp.GetSettings()->Midi.bMidiArpeggio && m_Arpeggiator)		// // //
		m_Arpeggiator->Tick(m_pTrackerView->GetSelectedChannel());

	// Play queued notes
	for (auto &x : m_pTrackerChannels) {		// // //
		// workaround to permutate channel indices
		int Index = x->GetID();
		int Channel = m_pDocument->GetChannelIndex(m_pTrackerChannels[Index]->GetID());
		if (Channel == -1)
			continue;
		auto &pChan = m_pChannels[Index];
		auto &pTrackerChan = m_pTrackerChannels[Index];
		
		// Run auto-arpeggio, if enabled
		if (theApp.GetSettings()->Midi.bMidiArpeggio && m_Arpeggiator)		// // //
			if (int Arpeggio = m_Arpeggiator->GetNextNote(Channel); Arpeggio > 0)
				pChan->Arpeggiate(Arpeggio);

		// Check if new note data has been queued for playing
		if (pTrackerChan->NewNoteData()) {
			stChanNote Note = pTrackerChan->GetNote();
			pChan->PlayNote(&Note, m_pDocument->GetEffColumns(m_iPlayTrack, Channel) + 1);
		}

		// Pitch wheel
		pChan->SetPitch(pTrackerChan->GetPitch());

		// Update volume meters
		pTrackerChan->SetVolumeMeter(m_pAPU->GetVol(pTrackerChan->GetID()));

		// Channel updates (instruments, effects etc)
		m_bHaltRequest ? pChan->ResetChannel() : pChan->ProcessChannel();
	}
}

void CSoundGen::UpdateAPU()
{
	// Write to APU registers

	m_iConsumedCycles = 0;

	// Copy wave changed flag
	m_bInternalWaveChanged = m_bWaveChanged;
	m_bWaveChanged = false;
	
	{
		CSingleLock l(&m_csAPULock);		// // //
		if (l.Lock()) {
			// Update APU channel registers
			unsigned int LastChip = SNDCHIP_NONE;		// // // 050B
			size_t i = 0;		// // //
			for (auto &ch : m_pChannels) {
				if (ch) {
					ch->RefreshChannel();
					ch->FinishTick();		// // //
					unsigned int Chip = m_pTrackerChannels[i]->GetChip();
					if (m_pDocument->ExpansionEnabled(Chip)) {
						int Delay = (Chip == LastChip) ? 150 : 250;
						if (m_iConsumedCycles + Delay < m_iUpdateCycles)
							AddCycles(Delay);
						LastChip = Chip;
					}
					m_pAPU->Process();
				}
				++i;
			}
		#ifdef WRITE_VGM		// // //
			if (m_bPlaying)
				m_iRegisterStream.push(0x62);		// // //
		#endif

			// Finish the audio frame
			m_pAPU->AddTime(m_iUpdateCycles - m_iConsumedCycles);
			m_pAPU->Process();
			l.Unlock();
		}
	}

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
	StartRendering();		// // //
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

	m_pAPU->SetExternalSound(Chip);

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
	m_pAPU->SetNamcoMixing(theApp.GetSettings()->m_bNamcoMixing);
}

// Player state functions

void CSoundGen::ReadPatternRow()
{
	stChanNote NoteData;

	for (int i = 0, Channels = m_pDocument->GetChannelCount(); i < Channels; ++i)
		if (PlayerGetNote(i, NoteData))		// // //
			QueueNote(i, NoteData, NOTE_PRIO_1);

	if (m_bDoHalt)		// // //
		m_bHaltRequest = true;
}

// // //
bool CSoundGen::PlayerGetNote(int Channel, stChanNote &NoteData) {
	ASSERT(m_pTrackerView != NULL);

	m_pDocument->GetNoteData(m_iPlayTrack, m_iPlayFrame, Channel, m_iPlayRow, &NoteData);

	// // // evaluate global commands as soon as possible
	EvaluateGlobalEffects(&NoteData, m_pDocument->GetEffColumns(m_iPlayTrack, Channel) + 1);
	
	// Let view know what is about to play
	m_pTrackerView->PlayerPlayNote(Channel, NoteData);		// // //

	return !m_pTrackerView->IsChannelMuted(Channel);		// // //
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
}

void CSoundGen::PlayerStepFrame()
{
	const int Frames = m_pDocument->GetFrameCount(m_iPlayTrack);

	if (m_iQueuedFrame == -1) {
		if (++m_iPlayFrame >= Frames)
			m_iPlayFrame = 0;
	}
	else {
		m_iPlayFrame = m_iQueuedFrame;
		m_iQueuedFrame = -1;
	}

	++m_iFramesPlayed;
}

void CSoundGen::PlayerJumpTo(int Frame)
{
	const int Frames = m_pDocument->GetFrameCount(m_iPlayTrack);

	m_iPlayFrame = Frame;

	if (m_iPlayFrame >= Frames)
		m_iPlayFrame = Frames - 1;

	m_iPlayRow = 0;

	++m_iFramesPlayed;
	++m_iRowsPlayed;		// // //
}

void CSoundGen::PlayerSkipTo(int Row)
{
	const int Frames = m_pDocument->GetFrameCount(m_iPlayTrack);
	const int Rows = m_pDocument->GetPatternLength(m_iPlayTrack);

	if (++m_iPlayFrame >= Frames)
		m_iPlayFrame = 0;

	m_iPlayRow = Row;

	if (m_iPlayRow >= Rows)
		m_iPlayRow = Rows - 1;
	
	++m_iFramesPlayed;
	++m_iRowsPlayed;		// // //
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
	return m_iTicksPlayed;
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
	return m_pInstRecorder->GetRecordInstrument(m_iTicksPlayed);
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
