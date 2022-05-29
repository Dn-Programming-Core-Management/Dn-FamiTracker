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
#include "InstrumentManager.h"
#include "FamiTrackerDoc.h"
#include "FamiTrackerView.h"
#include "TrackerChannel.h"
#include "FamiTrackerViewMessage.h"
#include "SoundGen.h"
#include "InstrumentRecorder.h"
#include "SeqInstrument.h"
#include "InstrumentFDS.h"
#include "InstrumentN163.h"
#include "InstrumentFactory.h"
#include "DetuneTable.h"

CInstrumentRecorder::CInstrumentRecorder(CSoundGen *pSG) :
	m_pSoundGen(pSG),
	m_iRecordChannel(-1),
	m_iDumpCount(0),
	m_iRecordWaveCache(nullptr)
{
	m_stRecordSetting.Interval = MAX_SEQUENCE_ITEMS;
	m_stRecordSetting.InstCount = 1;
	m_stRecordSetting.Reset = true;
	for (int i = 0; i < SEQ_COUNT; i++)
		m_pSequenceCache[i] = new CSequence();
	memset(m_pDumpCache, 0, sizeof(CInstrument*) * MAX_INSTRUMENTS);
	ResetRecordCache();
}

CInstrumentRecorder::~CInstrumentRecorder()
{
	SAFE_RELEASE(*m_pDumpInstrument);
	for (int i = 0; i < SEQ_COUNT; i++)
		SAFE_RELEASE(m_pSequenceCache[i]);
	for (int i = 0; i < MAX_INSTRUMENTS; i++)
		SAFE_RELEASE(m_pDumpCache[i]);
	SAFE_RELEASE_ARRAY(m_iRecordWaveCache);
}

void CInstrumentRecorder::StartRecording()
{
	m_iDumpCount = m_stRecordSetting.InstCount;
	ResetRecordCache();
	InitRecordInstrument();
}

void CInstrumentRecorder::StopRecording(CFamiTrackerView *pView)
{
	if (*m_pDumpInstrument != nullptr && pView != nullptr)
		pView->PostAudioMessage(AM_DUMP_INST);
	--m_iDumpCount;
}

void CInstrumentRecorder::RecordInstrument(const unsigned Tick, CFamiTrackerView *pView)		// // //
{
	unsigned int Intv = static_cast<unsigned>(m_stRecordSetting.Interval);
	if (m_iRecordChannel == -1 || Tick > Intv * m_stRecordSetting.InstCount + 1) return;
	if (Tick % Intv == 1 && Tick > Intv) {
		if (*m_pDumpInstrument != nullptr && pView != nullptr) {
			pView->PostAudioMessage(AM_DUMP_INST);
			m_pDumpInstrument++;
		}
		--m_iDumpCount;
	}
	bool Temp = *m_pDumpInstrument == nullptr;
	int Pos = (Tick - 1) % Intv;

	signed char Val = 0;

	int PitchReg = 0;
	int Detune = 0x7FFFFFFF;
	int ID = m_iRecordChannel;

	char Chip = m_pDocument->GetChannel(m_pDocument->GetChannelIndex(m_iRecordChannel))->GetChip();
	const auto REG = [&] (int x) { return m_pSoundGen->GetReg(Chip, x); };

	switch (Chip) {
	case SNDCHIP_NONE:
		ID -= CHANID_SQUARE1;
		PitchReg = m_iRecordChannel == CHANID_NOISE ? (0x0F & REG(0x400E)) :
					(REG(0x4002 | (ID << 2)) | (0x07 & REG(0x4003 | (ID << 2))) << 8); break;
	case SNDCHIP_VRC6:
		ID -= CHANID_VRC6_PULSE1;
		PitchReg = REG(0x9001 + (ID << 12)) | (0x0F & REG(0x9002 | (ID << 12))) << 8; break;
	case SNDCHIP_FDS:
		ID -= CHANID_FDS; // ID = 0;
		PitchReg = REG(0x4082) | (0x0F & REG(0x4083)) << 8; break;
	case SNDCHIP_MMC5:
		ID -= CHANID_MMC5_SQUARE1;
		PitchReg = (REG(0x5002 | (ID << 2)) | (0x07 & REG(0x5003 | (ID << 2))) << 8); break;
	case SNDCHIP_N163:
		ID -= CHANID_N163_CH1;
		PitchReg = (REG(0x78 - (ID << 3))
					| (REG(0x7A - (ID << 3))) << 8
					| (0x03 & REG(0x7C - (ID << 3))) << 16) >> 2; // N163_PITCH_SLIDE_SHIFT;
		break;
	case SNDCHIP_S5B:
		ID -= CHANID_S5B_CH1;
		PitchReg = (REG(ID << 1) | (0x0F & REG(1 + (ID << 1))) << 8); break;
	}

	CDetuneTable::type_t Table;
	switch (Chip) {
	case SNDCHIP_NONE: Table = m_pDocument->GetMachine() == PAL ? CDetuneTable::DETUNE_PAL : CDetuneTable::DETUNE_NTSC; break;
	case SNDCHIP_VRC6: Table = m_iRecordChannel == CHANID_VRC6_SAWTOOTH ? CDetuneTable::DETUNE_SAW : CDetuneTable::DETUNE_NTSC; break;
	case SNDCHIP_VRC7: Table = CDetuneTable::DETUNE_VRC7; break;
	case SNDCHIP_FDS:  Table = CDetuneTable::DETUNE_FDS; break;
	case SNDCHIP_MMC5: Table = CDetuneTable::DETUNE_NTSC; break;
	case SNDCHIP_N163: Table = CDetuneTable::DETUNE_N163; break;
	case SNDCHIP_S5B:  Table = CDetuneTable::DETUNE_S5B; break;
	}
	int Note = 0;
	if (m_iRecordChannel == CHANID_NOISE) {
		Note = PitchReg ^ 0xF; Detune = 0;
	}
	else for (int i = 0; i < NOTE_COUNT; i++) {
		int diff = PitchReg - m_pSoundGen->ReadPeriodTable(i, Table);
		if (std::abs(diff) < std::abs(Detune)) {
			Note = i; Detune = diff;
		}
	}

	inst_type_t InstType = INST_NONE; // optimize this
	switch (Chip) {
	case SNDCHIP_NONE: case SNDCHIP_MMC5: InstType = INST_2A03; break;
	case SNDCHIP_VRC6: InstType = INST_VRC6; break;
	// case SNDCHIP_VRC7: Type = INST_VRC7; break;
	case SNDCHIP_FDS:  InstType = INST_FDS; break;
	case SNDCHIP_N163: InstType = INST_N163; break;
	case SNDCHIP_S5B:  InstType = INST_S5B; break;
	}

	switch (InstType) {
	case INST_2A03: case INST_VRC6: case INST_N163: case INST_S5B:
		for (int i = 0; i < SEQ_COUNT; i++) {
			sequence_t s = static_cast<sequence_t>(i);
			switch (s) {
			case SEQ_VOLUME:
				switch (Chip) {
				case SNDCHIP_NONE:
					Val = m_iRecordChannel == CHANID_TRIANGLE ? ((0x7F & REG(0x4008)) ? 15 : 0) : (0x0F & REG(0x4000 | (ID << 2))); break;
				case SNDCHIP_VRC6:
					Val = m_iRecordChannel == CHANID_VRC6_SAWTOOTH ? (0x0F & REG(0xB000) >> 1) : (0x0F & REG(0x9000 | (ID << 12))); break;
				case SNDCHIP_MMC5:
					Val = 0x0F & REG(0x5000 | (ID << 2)); break;
				case SNDCHIP_N163:
					Val = 0x0F & REG(0x7F - (ID << 3)); break;
				case SNDCHIP_S5B:
					Val = 0x0F & REG(0x08 + ID); break;
				}
				break;
			case SEQ_ARPEGGIO: Val = static_cast<char>(Note); break;
			case SEQ_PITCH: Val = static_cast<char>(Detune % 16); break;
			case SEQ_HIPITCH: Val = static_cast<char>(Detune / 16); break;
			case SEQ_DUTYCYCLE:
				switch (Chip) {
				case SNDCHIP_NONE:
					Val = m_iRecordChannel == CHANID_TRIANGLE ? 0 :
						m_iRecordChannel == CHANID_NOISE ? (0x01 & REG(0x400E) >> 7) : (0x03 & REG(0x4000 | (ID << 2)) >> 6); break;
				case SNDCHIP_VRC6:
					Val = m_iRecordChannel == CHANID_VRC6_SAWTOOTH ? (0x01 & REG(0xB000) >> 5) : (0x07 & REG(0x9000 | (ID << 12)) >> 4); break;
				case SNDCHIP_MMC5:
					Val = 0x03 & REG(0x5000 | (ID << 2)) >> 6; break;
				case SNDCHIP_N163:
					Val = 0;
					if (m_iRecordWaveCache == NULL) {
						int Size = 0x100 - (0xFC & REG(0x7C - (ID << 3)));
						if (Size <= CInstrumentN163::MAX_WAVE_SIZE) {
							m_iRecordWaveSize = Size;
							m_iRecordWaveCache = new char[m_iRecordWaveSize * CInstrumentN163::MAX_WAVE_COUNT]();
							m_iRecordWaveCount = 0;
						}
					}
					if (m_iRecordWaveCache != nullptr) {
						int Count = 0x100 - (0xFC & REG(0x7C - (ID << 3)));
						if (Count == m_iRecordWaveSize) {
							int pos = REG(0x7E - (ID << 3));
							char *Wave = new char[Count];
							for (int j = 0; j < Count; j++)
								Wave[j] = 0x0F & REG((pos + j) >> 1) >> ((j & 0x01) ? 4 : 0);
							for (int j = 1; j <= m_iRecordWaveCount; j++) {
								if (!memcmp(Wave, m_iRecordWaveCache + j * Count, Count)) {
									Val = j; goto outer;
								}
							}
							if (m_iRecordWaveCount < CInstrumentN163::MAX_WAVE_COUNT - 1) {
								Val = ++m_iRecordWaveCount;
								memcpy(m_iRecordWaveCache + Val * Count, Wave, Count);
							}
							else
								Val = m_pSequenceCache[i]->GetItem(Pos - 1);
						outer:
							SAFE_RELEASE_ARRAY(Wave);
						}
					}
					break;
				case SNDCHIP_S5B:
					//0x1F accounts for inverted noise period value
					Val = 0x1F - (0x1F & REG(0x06)) | (0x10 & REG(0x08 + ID)) << 1
						| (0x01 << ID & ~REG(0x07)) << (6 - ID) | (0x08 << ID & ~REG(0x07)) << (4 - ID); break;
				}
				break;
			}
			m_pSequenceCache[i]->SetItemCount(Pos + 1);
			m_pSequenceCache[i]->SetItem(Pos, Val);
		}
		break;
	case INST_FDS:
		for (int k = 0; k <= 2; k++) {
			switch (k) {
			case 0: Val = 0x3F & REG(0x4080); if (Val > 0x20) Val = 0x20; break;
			case 1: Val = static_cast<char>(Note); break;
			case 2: Val = static_cast<char>(Detune); break;
			}
			m_pSequenceCache[k]->SetItemCount(Pos + 1);
			m_pSequenceCache[k]->SetItem(Pos, Val);
		}
	}

	if (!(Tick % Intv))
		FinalizeRecordInstrument();
}

CInstrument* CInstrumentRecorder::GetRecordInstrument(unsigned Tick) const
{
	return m_pDumpCache[Tick / m_stRecordSetting.Interval - (m_pSoundGen->IsPlaying() ? 1 : 0)];
}

int CInstrumentRecorder::GetRecordChannel() const
{
	return m_iRecordChannel;
}

void CInstrumentRecorder::SetRecordChannel(int Channel)
{
	m_iRecordChannel = Channel;
}

stRecordSetting *CInstrumentRecorder::GetRecordSetting() const
{
	return new stRecordSetting(m_stRecordSetting);
}

void CInstrumentRecorder::SetRecordSetting(stRecordSetting *Setting)
{
	m_stRecordSetting = *Setting; delete Setting;
}

void CInstrumentRecorder::ResetDumpInstrument()
{
	if (m_iDumpCount < 0) return;
	if (m_pSoundGen->IsPlaying()) {
		--m_pDumpInstrument;
		if (m_pDumpInstrument >= m_pDumpCache)
			ReleaseCurrent();
		++m_pDumpInstrument;
	}
	if (m_iDumpCount && *m_pDumpInstrument == nullptr) {
		InitRecordInstrument();
	}
	else {
		if (*m_pDumpInstrument != nullptr)
			FinalizeRecordInstrument();
		if (!m_iDumpCount || !m_pSoundGen->IsPlaying()) {
			m_iRecordChannel = -1;
			if (m_stRecordSetting.Reset) {
				m_stRecordSetting.Interval = MAX_SEQUENCE_ITEMS;
				m_stRecordSetting.InstCount = 1;
			}
			ReleaseCurrent();
		}
	}
}

void CInstrumentRecorder::ResetRecordCache()
{
	memset(m_pDumpCache, 0, sizeof(CInstrument*) * MAX_INSTRUMENTS);
	m_pDumpInstrument = &m_pDumpCache[0];
	for (int i = 0; i < SEQ_COUNT; i++)
		m_pSequenceCache[i]->Clear();
}

void CInstrumentRecorder::ReleaseCurrent()
{
	if (*m_pDumpInstrument != nullptr) {
		//(*m_pDumpInstrument)->Release();
		*m_pDumpInstrument = nullptr;
	}
}

void CInstrumentRecorder::InitRecordInstrument()
{
	CTrackerChannel *pChan = m_pDocument->GetChannel(m_pDocument->GetChannelIndex(m_iRecordChannel));
	if (m_pDocument->GetInstrumentCount() >= MAX_INSTRUMENTS) {
		m_iDumpCount = 0; m_iRecordChannel = -1; return;
	}
	inst_type_t Type = INST_NONE; // optimize this
	switch (pChan->GetChip()) {
	case SNDCHIP_NONE: case SNDCHIP_MMC5: Type = INST_2A03; break;
	case SNDCHIP_VRC6: Type = INST_VRC6; break;
	// case SNDCHIP_VRC7: Type = INST_VRC7; break;
	case SNDCHIP_FDS:  Type = INST_FDS; break;
	case SNDCHIP_N163: Type = INST_N163; break;
	case SNDCHIP_S5B:  Type = INST_S5B; break;
	}
	*m_pDumpInstrument = CInstrumentFactory::CreateNew(Type);		// // //
	if (!*m_pDumpInstrument) return;

	CString str;
	str.Format(_T("from %s"), pChan->GetChannelName());
	(*m_pDumpInstrument)->SetName(str);

	if (Type == INST_FDS) {
		m_pSequenceCache[SEQ_ARPEGGIO]->SetSetting(SETTING_ARP_FIXED);
		return;
	}
	switch (Type) {
	case INST_2A03: case INST_VRC6: case INST_N163: case INST_S5B:
		CSeqInstrument *Inst = dynamic_cast<CSeqInstrument*>(*m_pDumpInstrument);
		ASSERT(Inst != NULL);
		for (int i = 0; i < SEQ_COUNT; i++) {
			Inst->SetSeqEnable(i, 1);
			Inst->SetSeqIndex(i, m_pDocument->GetFreeSequence(Type, i));
		}
		m_pSequenceCache[SEQ_ARPEGGIO]->SetSetting(SETTING_ARP_FIXED);
		// m_pSequenceCache[SEQ_PITCH]->SetSetting(SETTING_PITCH_ABSOLUTE);
		// m_pSequenceCache[SEQ_HIPITCH]->SetSetting(SETTING_PITCH_ABSOLUTE);
		// VRC6 sawtooth 64-step volume
		if (m_iRecordChannel == CHANID_TRIANGLE)
			Inst->SetSeqEnable(SEQ_DUTYCYCLE, 0);
		break;
	}
	m_iRecordWaveSize = 32; // DEFAULT_WAVE_SIZE
	m_iRecordWaveCount = 0;
}

void CInstrumentRecorder::FinalizeRecordInstrument()
{
	if (*m_pDumpInstrument == NULL) return;
	inst_type_t InstType = (*m_pDumpInstrument)->GetType();
	CSeqInstrument *Inst = dynamic_cast<CSeqInstrument*>(*m_pDumpInstrument);
	CInstrumentFDS *FDSInst = dynamic_cast<CInstrumentFDS*>(*m_pDumpInstrument);
	CInstrumentN163 *N163Inst = dynamic_cast<CInstrumentN163*>(*m_pDumpInstrument);
	if (Inst != NULL) {
		(*m_pDumpInstrument)->RegisterManager(m_pDocument->GetInstrumentManager());
		for (int i = 0; i < SEQ_COUNT; i++) {
			if (Inst->GetSeqEnable(i) != 0) {
				m_pSequenceCache[i]->SetLoopPoint(m_pSequenceCache[i]->GetItemCount() - 1);
				Inst->SetSequence(i, m_pSequenceCache[i]);
			}
			m_pSequenceCache[i] = new CSequence();
		}
	}
	switch (InstType) {
	case INST_FDS:
		ASSERT(FDSInst != NULL);
		/*
		for (int i = 0; i < CInstrumentFDS::SEQUENCE_COUNT; i++) {
			const CSequence *Seq = FDSInst->GetSequence(i);
			ASSERT(Seq != NULL);
			m_pSequenceCache[i]->SetLoopPoint(m_pSequenceCache[i]->GetItemCount() - 1);
			FDSInst->SetSequence(i, m_pSequenceCache[i]);
			m_pSequenceCache[i] = new CSequence();
		}
		*/
		for (int i = 0; i < CInstrumentFDS::WAVE_SIZE; i++)
			FDSInst->SetSample(i, 0x3F & m_pSoundGen->GetReg(SNDCHIP_FDS, 0x4040 + i));
		FDSInst->SetModulationDepth(0x3F & m_pSoundGen->GetReg(SNDCHIP_FDS, 0x4084));
		FDSInst->SetModulationSpeed(m_pSoundGen->GetReg(SNDCHIP_FDS, 0x4086) | (0x0F & m_pSoundGen->GetReg(SNDCHIP_FDS, 0x4087)) << 8);
		break;
	case INST_N163:
		ASSERT(N163Inst != NULL);
		int offs = (m_iRecordChannel - CHANID_N163_CH1) << 3;
		int pos = m_pSoundGen->GetReg(SNDCHIP_N163, 0x7E - offs);
		N163Inst->SetWavePos(pos);
		N163Inst->SetWaveSize(m_iRecordWaveSize);
		N163Inst->SetWaveCount(m_iRecordWaveCount + 1);
		if (m_iRecordWaveCache != NULL) {
			for (int i = 0; i <= m_iRecordWaveCount; i++) for (int j = 0; j < m_iRecordWaveSize; j++)
				N163Inst->SetSample(i, j, m_iRecordWaveCache[m_iRecordWaveSize * i + j]);
			SAFE_RELEASE(m_iRecordWaveCache);
		}
		else for (int j = 0; j < m_iRecordWaveSize; j++) // fallback for blank recording
			N163Inst->SetSample(0, j, 0);
		break;
	}
}
