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

#include "SoundDriver.h"
#include "SoundGenBase.h"
#include "FamiTrackerDoc.h"
#include "TempoCounter.h"
#include "ChannelHandler.h"
#include "ChannelFactory.h"		// // // test
#include "TrackerChannel.h"
#include "PlayerCursor.h"
#include "DetuneTable.h"
#include "SongState.h"
#include "APU/APU.h"
#include "ChannelsN163.h"



namespace {

const double NEW_VIBRATO_DEPTH[] = {
	1.0, 1.5, 2.5, 4.0, 5.0, 7.0, 10.0, 12.0, 14.0, 17.0, 22.0, 30.0, 44.0, 64.0, 96.0, 128.0
};

const double OLD_VIBRATO_DEPTH[] = {
	1.0, 1.0, 2.0, 3.0, 4.0, 7.0, 8.0, 15.0, 16.0, 31.0, 32.0, 63.0, 64.0, 127.0, 128.0, 255.0
};

} // namespace



CSoundDriver::CSoundDriver(CSoundGenBase *parent) : parent_(parent) {
}

CSoundDriver::~CSoundDriver() {
}

void CSoundDriver::SetupTracks() {
	// Only called once!

	// Clear all channels
	tracks_.clear();		// // /.

	// 2A03/2A07
	// // // Short header names
	AssignTrack(std::make_unique<CTrackerChannel>(_T("Pulse 1"), _T("PU1"), SNDCHIP_NONE, CHANID_SQUARE1));
	AssignTrack(std::make_unique<CTrackerChannel>(_T("Pulse 2"), _T("PU2"), SNDCHIP_NONE, CHANID_SQUARE2));
	AssignTrack(std::make_unique<CTrackerChannel>(_T("Triangle"), _T("TRI"), SNDCHIP_NONE, CHANID_TRIANGLE));
	AssignTrack(std::make_unique<CTrackerChannel>(_T("Noise"), _T("NOI"), SNDCHIP_NONE, CHANID_NOISE));
	AssignTrack(std::make_unique<CTrackerChannel>(_T("DPCM"), _T("DMC"), SNDCHIP_NONE, CHANID_DPCM));

	// Konami VRC6
	AssignTrack(std::make_unique<CTrackerChannel>(_T("VRC6 Pulse 1"), _T("V1"), SNDCHIP_VRC6, CHANID_VRC6_PULSE1));
	AssignTrack(std::make_unique<CTrackerChannel>(_T("VRC6 Pulse 2"), _T("V2"), SNDCHIP_VRC6, CHANID_VRC6_PULSE2));
	AssignTrack(std::make_unique<CTrackerChannel>(_T("Sawtooth"), _T("SAW"), SNDCHIP_VRC6, CHANID_VRC6_SAWTOOTH));

	// // // Nintendo MMC5
	AssignTrack(std::make_unique<CTrackerChannel>(_T("MMC5 Pulse 1"), _T("PU3"), SNDCHIP_MMC5, CHANID_MMC5_SQUARE1));
	AssignTrack(std::make_unique<CTrackerChannel>(_T("MMC5 Pulse 2"), _T("PU4"), SNDCHIP_MMC5, CHANID_MMC5_SQUARE2));
	AssignTrack(std::make_unique<CTrackerChannel>(_T("MMC5 PCM"), _T("PCM"), SNDCHIP_MMC5, CHANID_MMC5_VOICE)); // null channel handler

	// Namco N163
	AssignTrack(std::make_unique<CTrackerChannel>(_T("Namco 1"), _T("N1"), SNDCHIP_N163, CHANID_N163_CH1));
	AssignTrack(std::make_unique<CTrackerChannel>(_T("Namco 2"), _T("N2"), SNDCHIP_N163, CHANID_N163_CH2));
	AssignTrack(std::make_unique<CTrackerChannel>(_T("Namco 3"), _T("N3"), SNDCHIP_N163, CHANID_N163_CH3));
	AssignTrack(std::make_unique<CTrackerChannel>(_T("Namco 4"), _T("N4"), SNDCHIP_N163, CHANID_N163_CH4));
	AssignTrack(std::make_unique<CTrackerChannel>(_T("Namco 5"), _T("N5"), SNDCHIP_N163, CHANID_N163_CH5));
	AssignTrack(std::make_unique<CTrackerChannel>(_T("Namco 6"), _T("N6"), SNDCHIP_N163, CHANID_N163_CH6));
	AssignTrack(std::make_unique<CTrackerChannel>(_T("Namco 7"), _T("N7"), SNDCHIP_N163, CHANID_N163_CH7));
	AssignTrack(std::make_unique<CTrackerChannel>(_T("Namco 8"), _T("N8"), SNDCHIP_N163, CHANID_N163_CH8));

	// Nintendo FDS
	AssignTrack(std::make_unique<CTrackerChannel>(_T("FDS"), _T("FDS"), SNDCHIP_FDS, CHANID_FDS));

	// Konami VRC7
	AssignTrack(std::make_unique<CTrackerChannel>(_T("FM Channel 1"), _T("FM1"), SNDCHIP_VRC7, CHANID_VRC7_CH1));
	AssignTrack(std::make_unique<CTrackerChannel>(_T("FM Channel 2"), _T("FM2"), SNDCHIP_VRC7, CHANID_VRC7_CH2));
	AssignTrack(std::make_unique<CTrackerChannel>(_T("FM Channel 3"), _T("FM3"), SNDCHIP_VRC7, CHANID_VRC7_CH3));
	AssignTrack(std::make_unique<CTrackerChannel>(_T("FM Channel 4"), _T("FM4"), SNDCHIP_VRC7, CHANID_VRC7_CH4));
	AssignTrack(std::make_unique<CTrackerChannel>(_T("FM Channel 5"), _T("FM5"), SNDCHIP_VRC7, CHANID_VRC7_CH5));
	AssignTrack(std::make_unique<CTrackerChannel>(_T("FM Channel 6"), _T("FM6"), SNDCHIP_VRC7, CHANID_VRC7_CH6));

	// // // Sunsoft 5B
	AssignTrack(std::make_unique<CTrackerChannel>(_T("5B Square 1"), _T("5B1"), SNDCHIP_S5B, CHANID_S5B_CH1));
	AssignTrack(std::make_unique<CTrackerChannel>(_T("5B Square 2"), _T("5B2"), SNDCHIP_S5B, CHANID_S5B_CH2));
	AssignTrack(std::make_unique<CTrackerChannel>(_T("5B Square 3"), _T("5B3"), SNDCHIP_S5B, CHANID_S5B_CH3));
}

void CSoundDriver::LoadDocument(const CFamiTrackerDoc &doc, CAPU &apu, CSoundGen &sound) {
	doc_ = &doc;
	apu_ = &apu;

	// Setup all channels
	for (auto &x : tracks_)		// // //
		if (auto &ch = x.first)
			ch->InitChannel(&apu, m_iVibratoTable, /*this*/ &sound);
}

void CSoundDriver::ConfigureDocument() {
	SetupVibrato();
	SetupPeriodTables();

	for (auto &x : tracks_)
		if (auto &ch = x.first) {
			ch->SetVibratoStyle(doc_->GetVibratoStyle());		// // //
			ch->SetLinearPitch(doc_->GetLinearPitch());
			if (auto pChan = dynamic_cast<CChannelHandlerN163 *>(ch.get()))
				pChan->SetChannelCount(doc_->GetNamcoChannels());
		}
}

void CSoundDriver::RegisterTracks(CFamiTrackerDoc &doc) {
	ASSERT(&doc == doc_); // TODO: use std::unique_ptr<CChannelMap>

	// This affects the sound channel interface so it must be synchronized
	doc.LockDocument();

	// Clear all registered channels
	doc.ResetChannels();
	const auto Chip = doc.GetExpansionChip();

	// Register the channels in the document
	// Expansion & internal channels
	int i = 0;		// // //
	for (auto &x : tracks_) {
		int ID = x.second->GetID();		// // //
		if (x.first && ((x.second->GetChip() & Chip) || (i <= CHANID_DPCM))			// // //
					&& (i >= CHANID_FDS || i < CHANID_N163_CH1 + doc_->GetNamcoChannels())) {
			doc.RegisterChannel(x.second.get(), ID, x.second->GetChip());
		}
		++i;
	}

	doc.UnlockDocument();
}

void CSoundDriver::StartPlayer(std::unique_ptr<CPlayerCursor> cur) {
	m_pPlayerCursor = std::move(cur);		// // //
	m_bPlaying = true;
	m_bHaltRequest = false;

	m_iJumpToPattern = -1;
	m_iSkipToRow = -1;
	m_bDoHalt = false;		// // //
}

void CSoundDriver::StopPlayer() {
	m_bPlaying = false;
	m_bDoHalt = false;
	m_bHaltRequest = false;
}

void CSoundDriver::ResetTracks() {
	for (auto &x : tracks_) {
		if (auto &ch = x.first)
			ch->ResetChannel();
		if (auto &track = x.second)
			track->Reset();
	}
}

void CSoundDriver::LoadSoundState(const CSongState &state) {
	m_pTempoCounter->LoadSoundState(state);
	for (int i = 0, n = doc_->GetChannelCount(); i < n; ++i) {
		for (auto &x : tracks_)		// // // pick this out later
			if (x.first && x.second->GetID() == state.State[i].ChannelIndex) {
				x.first->ApplyChannelState(state.State[i]); break;
			}
	}
}

void CSoundDriver::SetTempoCounter(const std::shared_ptr<CTempoCounter> &tempo) {
	m_pTempoCounter = tempo;
	m_pTempoCounter->AssignDocument(*doc_);
}

void CSoundDriver::Tick() {
	// Access the document object, skip if access wasn't granted to avoid gaps in audio playback
	if (doc_ && doc_->LockDocument(0)) {
		if (IsPlaying())
			PlayerTick();
		UpdateChannels();
		doc_->UnlockDocument();
	}
}

void CSoundDriver::StepRow() {
	// ???
}

void CSoundDriver::PlayerTick() {
	m_pPlayerCursor->Tick();
	if (parent_)
		parent_->OnTick();

	int SteppedRows = 0;		// // //

	// Fetch next row
//	while (m_pTempoCounter->CanStepRow()) {
	if (m_pTempoCounter->CanStepRow()) {
		if (m_bDoHalt)
			m_bHaltRequest = true;
		else
			++SteppedRows;
		m_pTempoCounter->StepRow();		// // //

		for (int i = 0, Channels = doc_->GetChannelCount(); i < Channels; ++i) {		// // //
			stChanNote NoteData;
			doc_->GetNoteData(m_pPlayerCursor->GetCurrentSong(), m_pPlayerCursor->GetCurrentFrame(),
				i, m_pPlayerCursor->GetCurrentRow(), &NoteData);
			HandleGlobalEffects(NoteData, doc_->GetEffColumns(m_pPlayerCursor->GetCurrentSong(), i) + 1);
			if (!parent_ || !parent_->IsChannelMuted(i))
				QueueNote(i, NoteData, NOTE_PRIO_1);
			// Let view know what is about to play
			if (parent_)
				parent_->OnPlayNote(i, NoteData);
		}

		if (parent_)
			parent_->OnStepRow();
	}
	m_pTempoCounter->Tick();		// // //
	if (parent_)
		parent_->OnTick();

	// Update player
	if (parent_ && parent_->ShouldStopPlayer())		// // //
		m_bHaltRequest = true;
	else if (SteppedRows > 0 && !m_bDoHalt) {
		// Jump
		if (m_iJumpToPattern != -1)
			m_pPlayerCursor->DoBxx(m_iJumpToPattern);
		// Skip
		else if (m_iSkipToRow != -1)
			m_pPlayerCursor->DoDxx(m_iSkipToRow);
		// or just move on
		else
			while (SteppedRows--)
				m_pPlayerCursor->StepRow();

		m_iJumpToPattern = -1;
		m_iSkipToRow = -1;

		if (parent_)
			parent_->OnUpdateRow(m_pPlayerCursor->GetCurrentFrame(), m_pPlayerCursor->GetCurrentRow());
	}
}

void CSoundDriver::UpdateChannels() {
	for (auto &x : tracks_) {		// // //
		// workaround to permutate channel indices
		int Index = x.second->GetID();
		int Channel = doc_->GetChannelIndex(tracks_[Index].second->GetID());
		if (Channel == -1)
			continue;
		auto &pChan = tracks_[Index].first;
		auto &pTrackerChan = tracks_[Index].second;
		
		// Run auto-arpeggio, if enabled
		if (int Arpeggio = parent_ ? parent_->GetArpNote(Channel) : -1; Arpeggio > 0)		// // //
			pChan->Arpeggiate(Arpeggio);

		// Check if new note data has been queued for playing
		if (pTrackerChan->NewNoteData()) {
			stChanNote Note = pTrackerChan->GetNote();
			pChan->PlayNote(&Note, doc_->GetEffColumns(m_pPlayerCursor->GetCurrentSong(), Channel) + 1);
		}

		// Pitch wheel
		pChan->SetPitch(pTrackerChan->GetPitch());

		// Channel updates (instruments, effects etc)
		m_bHaltRequest ? pChan->ResetChannel() : pChan->ProcessChannel();
		pChan->RefreshChannel();
		pChan->FinishTick();		// // //

		// Update volume meters
		pTrackerChan->SetVolumeMeter(apu_->GetVol(pTrackerChan->GetID()));		// // //
	}
}

void CSoundDriver::UpdateAPU(int cycles) {
	unsigned int LastChip = SNDCHIP_NONE;		// // // 050B

	for (auto &x : tracks_) {
		if (auto &ch = x.first) {
			unsigned int Chip = x.second->GetChip();
			if (doc_->ExpansionEnabled(Chip)) {
				int Delay = (Chip == LastChip) ? 150 : 250;
				if (Delay < cycles) {
					// Add APU cycles
					cycles -= Delay;
					apu_->AddTime(Delay);
				}
				LastChip = Chip;
			}
			apu_->Process();
		}
	}

	// Finish the audio frame
	apu_->AddTime(cycles);
	apu_->Process();
}

void CSoundDriver::QueueNote(int chan, stChanNote &note, note_prio_t priority) {
	doc_->GetChannel(chan)->SetNote(note, NOTE_PRIO_1);
}

void CSoundDriver::SetPlayerPos(int Frame, int Row) {
	if (m_pPlayerCursor)
		m_pPlayerCursor->SetPosition(Frame, Row);
}

void CSoundDriver::EnqueueFrame(int Frame) {
	m_pPlayerCursor->QueueFrame(Frame);
}

void CSoundDriver::ForceReloadInstrument(int chan) {
	if (doc_)
		tracks_[doc_->GetChannel(chan)->GetID()].first->ForceReloadInstrument();
}

bool CSoundDriver::IsPlaying() const {
	return m_bPlaying;
}

bool CSoundDriver::ShouldHalt() const {
	return m_bHaltRequest;
}

int CSoundDriver::GetCurrentSong() const {
	return m_pPlayerCursor->GetCurrentSong();
}

std::pair<unsigned, unsigned> CSoundDriver::GetPlayerPos() const {
	return std::make_pair(
		m_pPlayerCursor ? m_pPlayerCursor->GetCurrentFrame() : 0,
		m_pPlayerCursor ? m_pPlayerCursor->GetCurrentRow() : 0);
}

unsigned CSoundDriver::GetPlayerTicks() const {
	return m_pPlayerCursor ? m_pPlayerCursor->GetTotalTicks() : 0;
}

unsigned CSoundDriver::GetQueuedFrame() const {
	return m_pPlayerCursor ? m_pPlayerCursor->GetQueuedFrame().value_or(-1) : -1;
}

CChannelHandler *CSoundDriver::GetChannelHandler(int Index) const {
	return tracks_[Index].first.get();
}

int CSoundDriver::GetChannelVolume(int chan) const {
	const auto &ch = GetChannelHandler(chan);
	return ch ? ch->GetChannelVolume() : 0;
}

std::string CSoundDriver::GetChannelStateString(int chan) const {
	const auto &ch = GetChannelHandler(chan);
	return ch ? ch->GetStateString() : "";
}

int CSoundDriver::ReadPeriodTable(int Index, int Table) const {
	switch (Table) {
	case CDetuneTable::DETUNE_NTSC: return m_iNoteLookupTableNTSC[Index]; break;
	case CDetuneTable::DETUNE_PAL:  return m_iNoteLookupTablePAL[Index]; break;
	case CDetuneTable::DETUNE_SAW:  return m_iNoteLookupTableSaw[Index]; break;
	case CDetuneTable::DETUNE_VRC7: return m_iNoteLookupTableVRC7[Index]; break;
	case CDetuneTable::DETUNE_FDS:  return m_iNoteLookupTableFDS[Index]; break;
	case CDetuneTable::DETUNE_N163: return m_iNoteLookupTableN163[Index]; break;
	case CDetuneTable::DETUNE_S5B:  return m_iNoteLookupTableNTSC[Index] + 1; break;
	}
	__debugbreak();
	return m_iNoteLookupTableNTSC[Index];
}

int CSoundDriver::ReadVibratoTable(int index) const {
	return m_iVibratoTable[index];
}

void CSoundDriver::AssignTrack(std::unique_ptr<CTrackerChannel> track) {
	static CChannelFactory F {}; // test
	chan_id_t ID = track->GetID();

	CChannelHandler *ch = F.Produce(ID);
	if (ch)
		ch->SetChannelID(ID);

	tracks_.emplace_back(std::unique_ptr<CChannelHandler>(ch), std::move(track));		// // //
}

void CSoundDriver::SetupVibrato() {
	const vibrato_t style = doc_->GetVibratoStyle();

	for (int i = 0; i < 16; ++i) {	// depth 
		for (int j = 0; j < 16; ++j) {	// phase
			int value = 0;
			double angle = (double(j) / 16.0) * (3.1415 / 2.0);

			if (style == VIBRATO_NEW)
				value = int(sin(angle) * NEW_VIBRATO_DEPTH[i] /*+ 0.5f*/);
			else {
				value = (int)((double(j * OLD_VIBRATO_DEPTH[i]) / 16.0) + 1);
			}

			m_iVibratoTable[i * 16 + j] = value;
		}
	}
}

void CSoundDriver::SetupPeriodTables() {

	machine_t Machine = doc_->GetMachine();
	const double A440_NOTE = 45. - doc_->GetTuningSemitone() - doc_->GetTuningCent() / 100.;
	double clock_ntsc = CAPU::BASE_FREQ_NTSC / 16.0;
	double clock_pal = CAPU::BASE_FREQ_PAL / 16.0;

	for (int i = 0; i < NOTE_COUNT; ++i) {
		// Frequency (in Hz)
		double Freq = 440. * pow(2.0, double(i - A440_NOTE) / 12.);
		double Pitch;

		// 2A07
		Pitch = (clock_pal / Freq) - 0.5;
		m_iNoteLookupTablePAL[i] = (unsigned int)(Pitch - doc_->GetDetuneOffset(1, i));		// // //

																							// 2A03 / MMC5 / VRC6
		Pitch = (clock_ntsc / Freq) - 0.5;
		m_iNoteLookupTableNTSC[i] = (unsigned int)(Pitch - doc_->GetDetuneOffset(0, i));		// // //
		m_iNoteLookupTableS5B[i] = m_iNoteLookupTableNTSC[i] + 1;		// correction

																		// VRC6 Saw
		Pitch = ((clock_ntsc * 16.0) / (Freq * 14.0)) - 0.5;
		m_iNoteLookupTableSaw[i] = (unsigned int)(Pitch - doc_->GetDetuneOffset(2, i));		// // //

																							// FDS
#ifdef TRANSPOSE_FDS
		Pitch = (Freq * 65536.0) / (clock_ntsc / 1.0) + 0.5;
#else
		Pitch = (Freq * 65536.0) / (clock_ntsc / 4.0) + 0.5;
#endif
		m_iNoteLookupTableFDS[i] = (unsigned int)(Pitch + doc_->GetDetuneOffset(4, i));		// // //

																							// N163
		Pitch = ((Freq * doc_->GetNamcoChannels() * 983040.0) / clock_ntsc + 0.5) / 4;		// // //
		m_iNoteLookupTableN163[i] = (unsigned int)(Pitch + doc_->GetDetuneOffset(5, i));		// // //

		if (m_iNoteLookupTableN163[i] > 0xFFFF)	// 0x3FFFF
			m_iNoteLookupTableN163[i] = 0xFFFF;	// 0x3FFFF

												// // // Sunsoft 5B uses NTSC table

												// // // VRC7
		if (i < NOTE_RANGE) {
			Pitch = Freq * 262144.0 / 49716.0 + 0.5;
			m_iNoteLookupTableVRC7[i] = (unsigned int)(Pitch + doc_->GetDetuneOffset(3, i));		// // //
		}
	}

	// // // Setup note tables
	for (auto &x : tracks_) {
		if (!x.first)
			continue;
		if (auto Table = [&] () -> const unsigned * {
			switch (x.second->GetID()) {
			case CHANID_SQUARE1: case CHANID_SQUARE2: case CHANID_TRIANGLE:
				return Machine == PAL ? m_iNoteLookupTablePAL : m_iNoteLookupTableNTSC;
			case CHANID_VRC6_PULSE1: case CHANID_VRC6_PULSE2:
			case CHANID_MMC5_SQUARE1: case CHANID_MMC5_SQUARE2:
				return m_iNoteLookupTableNTSC;
			case CHANID_VRC6_SAWTOOTH:
				return m_iNoteLookupTableSaw;
			case CHANID_VRC7_CH1: case CHANID_VRC7_CH2: case CHANID_VRC7_CH3:
			case CHANID_VRC7_CH4: case CHANID_VRC7_CH5: case CHANID_VRC7_CH6:
				return m_iNoteLookupTableVRC7;
			case CHANID_FDS:
				return m_iNoteLookupTableFDS;
			case CHANID_N163_CH1: case CHANID_N163_CH2: case CHANID_N163_CH3: case CHANID_N163_CH4:
			case CHANID_N163_CH5: case CHANID_N163_CH6: case CHANID_N163_CH7: case CHANID_N163_CH8:
				return m_iNoteLookupTableN163;
			case CHANID_S5B_CH1: case CHANID_S5B_CH2: case CHANID_S5B_CH3:
				return m_iNoteLookupTableS5B;
			}
			return nullptr;
		}())
			x.first->SetNoteTable(Table);
	}
}

void CSoundDriver::HandleGlobalEffects(stChanNote &note, int fxCols) {
	for (int i = 0; i < fxCols; ++i) {
		unsigned char EffParam = note.EffParam[i];
		switch (note.EffNumber[i]) {
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
				m_iJumpToPattern = EffParam;
				break;

			// Dxx: Skip to next track and start at row xx
			case EF_SKIP:
				m_iSkipToRow = EffParam;
				break;

			// Cxx: Halt playback
			case EF_HALT:
				m_bDoHalt = true;		// // //
				m_pPlayerCursor->DoCxx();		// // //
				break;

			default: continue;		// // //
		}

		note.EffNumber[i] = EF_NONE;
	}
}
