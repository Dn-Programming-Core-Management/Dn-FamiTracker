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
//  - Create new interface for CFamiTrackerView with thread-safe functions
//  - Same for CFamiTrackerDoc
//  - Perhaps this should be a worker thread and not GUI thread?
//

/*/ // //
CSoundGen depends on CFamiTrackerView for:
 - MakeSilent
 - GetSelectedPos
 - GetSelectedChannel
 - IsChannelMuted
 - PlayerPlayNote
// // /*/

#include "SoundGen.h"
#include "FamiTracker.h"
#include "FTMComponentInterface.h"		// // //
#include "SongState.h"		// // //
#include "FamiTrackerDoc.h"
#include "FamiTrackerView.h"
#include "VisualizerWnd.h"
#include "MainFrm.h"
#include "DirectSound.h"
#include "WaveFile.h"		// // //
#include "APU/APU.h"
#include "ChannelHandler.h" // TODO: remove (hex)
#include "DSample.h"		// // //
#include "InstrumentRecorder.h"		// // //
#include "Settings.h"
#include "MIDI.h"
#include "Arpeggiator.h"		// // //
#include "TempoCounter.h"		// // //
#include "TempoDisplay.h"		// // // 050B
#include "AudioDriver.h"		// // //
#include "WaveRenderer.h"		// // //
#include "SoundDriver.h"		// // //

// // // Log VGM output (port from sn7t when necessary)
//#define WRITE_VGM



namespace {

const std::size_t DEFAULT_AVERAGE_BPM_SIZE = 24;

} // namespace

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
	m_pSoundDriver(std::make_unique<CSoundDriver>(this)),		// // //
	m_pAPU(std::make_unique<CAPU>()),		// // //
	m_bHaltRequest(false),
	m_pInstRecorder(std::make_unique<CInstrumentRecorder>(this)),		// // //
	m_bWaveChanged(0),
	m_iMachineType(NTSC),
	m_bRunning(false),
	m_hInterruptEvent(NULL),
	m_pArpeggiator(std::make_unique<CArpeggiator>()),		// // //
	m_pSequencePlayPos(NULL),
	m_iSequencePlayPos(0),
	m_iSequenceTimeout(0)
{
	TRACE("SoundGen: Object created\n");

	// Create all kinds of channels
	m_pSoundDriver->SetupTracks();		// // //
}

CSoundGen::~CSoundGen()
{
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
	m_pTempoCounter = std::make_shared<CTempoCounter>(*m_pDocument);		// // //

	m_pSoundDriver->LoadDocument(*pDoc, *m_pAPU, *this);		// // //
	m_pSoundDriver->SetTempoCounter(m_pTempoCounter);		// // //
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

	ASSERT(pDoc == m_pDocument);
	m_pSoundDriver->RegisterTracks(*pDoc);		// // //
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

void CSoundGen::DocumentPropertiesChanged(CFamiTrackerDoc *pDocument)
{
	ASSERT(pDocument == m_pDocument);		// // //
	m_pSoundDriver->ConfigureDocument();
}

//
// Interface functions
//

void CSoundGen::StartPlayer(std::unique_ptr<CPlayerCursor> Pos)		// // //
{
	if (!m_hThread)
		return;

	PostThreadMessage(WM_USER_PLAY, reinterpret_cast<uintptr_t>(Pos.release()), 0);
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

	auto pCur = std::make_unique<CPlayerCursor>(*m_pDocument, Track);		// // //
	PostThreadMessage(WM_USER_RESET, reinterpret_cast<uintptr_t>(pCur.release()), 0);
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

int CSoundGen::ReadVibratoTable(int index) const
{
	return m_pSoundDriver->ReadVibratoTable(index);		// // //
}

int CSoundGen::ReadPeriodTable(int Index, int Table) const		// // //
{
	return m_pSoundDriver->ReadPeriodTable(Index, Table);		// // //
}

void CSoundGen::BeginPlayer(std::unique_ptr<CPlayerCursor> Pos)		// // //
{
	// Called from player thread
	ASSERT(GetCurrentThreadId() == m_nThreadID);
	ASSERT(m_pDocument != NULL);
	ASSERT(m_pTrackerView != NULL);

	if (!m_pDocument || !m_pAudioDriver->IsAudioDeviceOpen() || !m_pDocument->IsFileLoaded())		// // //
		return;

	auto &cur = *Pos;		// // //
	m_pSoundDriver->StartPlayer(std::move(Pos));		// // //

	m_bHaltRequest		= false;
	m_iLastTrack		= cur.GetCurrentSong();		// // //

#ifdef WRITE_VGM		// // //
	m_pVGMWriter = std::make_unique<CVGMWriter>();
#endif

	if (theApp.GetSettings()->Display.bAverageBPM)		// // // 050B
		m_pTempoDisplay = std::make_unique<CTempoDisplay>(*m_pTempoCounter, DEFAULT_AVERAGE_BPM_SIZE);

	ResetTempo();
	ResetAPU();

	MakeSilent();

	if (theApp.GetSettings()->General.bRetrieveChanState)		// // //
		ApplyGlobalState();

	if (m_pInstRecorder->GetRecordChannel() != -1)		// // //
		m_pInstRecorder->StartRecording();
}

void CSoundGen::ApplyGlobalState()		// // //
{
	auto [Frame, Row] = IsPlaying() ? GetPlayerPos() : m_pTrackerView->GetSelectedPos();		// // //

	CSongState state;
	state.Retrieve(*m_pDocument, GetPlayerTrack(), Frame, Row);

	m_pSoundDriver->LoadSoundState(state);

	m_iLastHighlight = m_pDocument->GetHighlightAt(GetPlayerTrack(), Frame, Row).First;
}

// // //
void CSoundGen::OnTick() {
	if (IsRendering())
		m_pWaveRenderer->Tick();
	if (m_pTempoDisplay)		// // // 050B
		m_pTempoDisplay->Tick();
	if (theApp.GetSettings()->Midi.bMidiArpeggio && m_pArpeggiator)		// // //
		m_pArpeggiator->Tick(m_pTrackerView->GetSelectedChannel());
}

void CSoundGen::OnStepRow() {
	if (m_pTempoDisplay)		// // // 050B
		m_pTempoDisplay->StepRow();
	if (IsRendering())
		m_pWaveRenderer->StepRow();		// // //
}

void CSoundGen::OnPlayNote(int chan, const stChanNote &note) {
	m_pTrackerView->PlayerPlayNote(chan, note);
	if (!m_pTrackerView->IsChannelMuted(chan))
		theApp.GetMIDI()->WriteNote(chan, note.Note, note.Octave, note.Vol);
}

void CSoundGen::OnUpdateRow(int frame, int row) {
	auto pMark = m_pDocument->GetBookmarkAt(m_iLastTrack, frame, row);
	if (pMark && pMark->m_Highlight.First != -1)		// // //
		m_iLastHighlight = pMark->m_Highlight.First;
	if (!IsBackgroundTask() && m_pTrackerView)		// // //
		m_pTrackerView->PostMessage(WM_USER_PLAYER, frame, row);
}

bool CSoundGen::IsChannelMuted(int chan) const {
	return m_pTrackerView->IsChannelMuted(chan);
}

bool CSoundGen::ShouldStopPlayer() const {
	return IsRendering() && m_pWaveRenderer->ShouldStopPlayer();
}

int CSoundGen::GetArpNote(int chan) const {
	if (theApp.GetSettings()->Midi.bMidiArpeggio && m_pArpeggiator)		// // //
		return m_pArpeggiator->GetNextNote(chan);
	return -1;
}

std::string CSoundGen::RecallChannelState(int Channel) const		// // //
{
	if (IsPlaying())
		return m_pSoundDriver->GetChannelStateString(Channel);

	auto [Frame, Row] = m_pTrackerView->GetSelectedPos();
	CSongState state;
	state.Retrieve(*m_pDocument, GetPlayerTrack(), Frame, Row);
	return state.GetChannelStateString(*m_pDocument, Channel);
}

void CSoundGen::HaltPlayer() {
	// Called from player thread
	ASSERT(GetCurrentThreadId() == m_nThreadID);

	// Move player to non-playing state

	MakeSilent();
	m_pAPU->ClearSample();		// // //

	// Signal that playback has stopped
	if (m_pTrackerView)
		m_pInstRecorder->StopRecording(m_pTrackerView);		// // //

	m_pSoundDriver->StopPlayer();		// // //
	m_bHaltRequest = false;
	m_pTempoDisplay.reset();		// // //

#ifdef WRITE_VGM		// // //
	if (m_pVGMWriter) {
		m_pVGMWriter->SaveVGMFile();
		m_pVGMWriter.reset();
	}
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

	if (m_pTrackerView)		// // //
		m_pTrackerView->MakeSilent();
	*m_pArpeggiator = CArpeggiator { };

	m_pAPU->Reset();
	m_pAPU->ClearSample();		// // //
	m_pSoundDriver->ResetTracks();		// // //
}

void CSoundGen::ResetState()
{
	// Called when a new module is loaded
	m_iLastTrack = 0;		// // //
}

// Get tempo values from the document
void CSoundGen::ResetTempo()
{
	ASSERT(m_pDocument != NULL);

	if (!m_pDocument)
		return;

	m_pTempoCounter->LoadTempo(m_iLastTrack);		// // //
	m_iLastHighlight = m_pDocument->GetHighlight().First;		// // //
}

void CSoundGen::SetHighlightRows(int Rows)		// // //
{
	m_iLastHighlight = Rows;
}

// Return current tempo setting in BPM
double CSoundGen::GetAverageBPM() const		// // // 050B
{
	return m_pTempoDisplay ? m_pTempoDisplay->GetAverageBPM() : m_pTempoCounter->GetTempo();
}

float CSoundGen::GetCurrentBPM() const		// // //
{
	double Max = m_pDocument->GetFrameRate() * 15.;
	double BPM = GetAverageBPM();		// // // 050B
	return static_cast<float>((BPM > Max ? Max : BPM) * 4. / (m_iLastHighlight ? m_iLastHighlight : 4));
}

// // //

bool CSoundGen::IsPlaying() const {
	return m_pSoundDriver && m_pSoundDriver->IsPlaying();		// // //
}

CArpeggiator &CSoundGen::GetArpeggiator() {		// // //
	return *m_pArpeggiator;
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
}

stDPCMState CSoundGen::GetDPCMState() const
{
	return m_pAPU ? stDPCMState {m_pAPU->GetSamplePos(), m_pAPU->GetDeltaCounter()} : stDPCMState {0, 0};		// // //
}

int CSoundGen::GetChannelVolume(int Channel) const
{
	return m_pSoundDriver ? m_pSoundDriver->GetChannelVolume(Channel) : 0;
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

	StopPlayer();
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

	m_pWaveRenderer.reset();		// // //
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

	if (!ResetAudioDevice()) {
		TRACE("SoundGen: Failed to reset audio device!\n");
		if (m_pVisualizerWnd != NULL)
			m_pVisualizerWnd->ReportAudioProblem();
	}

	ResetAPU();

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

	// Rendering
	if (m_pWaveRenderer)		// // //
		if (m_pWaveRenderer->ShouldStopRender())
			StopRendering();
		else if (m_pWaveRenderer->ShouldStartPlayer())
			StartPlayer(std::make_unique<CPlayerCursor>(*m_pDocument, m_pWaveRenderer->GetRenderTrack()));

	// Update APU registers
	UpdateAPU();

	if (IsPlaying()) {		// // //
		int Channel = m_pInstRecorder->GetRecordChannel();
		if (Channel != -1)		// // //
			m_pInstRecorder->RecordInstrument(GetPlayerTicks(), m_pTrackerView);
	}

	if (m_bHaltRequest) {
		// Halt has been requested, abort playback here
		HaltPlayer();
	}

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

	m_pSoundDriver->Tick();		// // //
	m_pSoundDriver->UpdateChannels();		// // //

	if (m_pSoundDriver->ShouldHalt())
		m_bHaltRequest = true;
}

void CSoundGen::UpdateAPU()
{
	// Write to APU registers

	m_iConsumedCycles = 0;

	// Copy wave changed flag
	m_bInternalWaveChanged = m_bWaveChanged;
	m_bWaveChanged = false;
	
	if (CSingleLock l(&m_csAPULock); l.Lock()) {
		// Update APU channel registers
		m_pSoundDriver->UpdateAPU(m_iUpdateCycles);

#ifdef WRITE_VGM		// // //
		if (m_pVGMWriter)
			m_pVGMWriter->Tick();		// // //
#endif
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
	BeginPlayer(std::unique_ptr<CPlayerCursor> {reinterpret_cast<CPlayerCursor *>(wParam)});
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

	auto pCur = std::unique_ptr<CPlayerCursor> {reinterpret_cast<CPlayerCursor *>(wParam)};		// // //
	m_iLastTrack = pCur->GetCurrentSong();
	if (IsPlaying())
		BeginPlayer(std::move(pCur));		// // //
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

void CSoundGen::QueueNote(int Channel, stChanNote &NoteData, note_prio_t Priority) const
{
	// Queue a note for play
	m_pSoundDriver->QueueNote(Channel, NoteData, Priority);
	theApp.GetMIDI()->WriteNote(Channel, NoteData.Note, NoteData.Octave, NoteData.Vol);
}

void CSoundGen::ForceReloadInstrument(int Channel)		// // //
{
	m_pSoundDriver->ForceReloadInstrument(Channel);
}

std::pair<unsigned, unsigned> CSoundGen::GetPlayerPos() const {		// // //
	return m_pSoundDriver->GetPlayerPos();
}

int CSoundGen::GetPlayerTrack() const
{
	return m_iLastTrack;
}

int CSoundGen::GetPlayerTicks() const
{
	return m_pSoundDriver->GetPlayerTicks();
}

void CSoundGen::MoveToFrame(int Frame)
{
	// Todo: synchronize
	m_pSoundDriver->SetPlayerPos(Frame, 0);		// // //
}

void CSoundGen::SetQueueFrame(unsigned Frame)
{
	m_pSoundDriver->EnqueueFrame(Frame);		// // //
}

unsigned CSoundGen::GetQueueFrame() const
{
	return m_pSoundDriver->GetQueuedFrame();		// // //
}

// Verification

void CSoundGen::WriteRegister(uint16_t Reg, uint8_t Value)
{
#ifdef WRITE_VGM		// // //
	uint8_t Port = 0;
	m_pVGMWriter->WriteRegister(Reg, Value, Port);
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
	return m_pInstRecorder->GetRecordInstrument(GetPlayerTicks());
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
