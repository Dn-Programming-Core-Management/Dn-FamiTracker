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


#include <vector>
#include <memory>

class CInstrumentFDS : public CSeqInstrument {
public:
	CInstrumentFDS();
	CInstrument* Clone() const;
	void	Setup();
	void	Store(CDocumentFile *pDocFile);
	bool	Load(CDocumentFile *pDocFile);
	void	SaveFile(CInstrumentFile *pFile);
	bool	LoadFile(CInstrumentFile *pFile, int iVersion);
	int		Compile(CChunk *pChunk, int Index);
	bool	CanRelease() const;

public:
	unsigned char GetSample(int Index) const;
	void	SetSample(int Index, int Sample);
	int		GetModulationSpeed() const;
	void	SetModulationSpeed(int Speed);
	int		GetModulation(int Index) const;
	void	SetModulation(int Index, int Value);
	int		GetModulationDepth() const;
	void	SetModulationDepth(int Depth);
	int		GetModulationDelay() const;
	void	SetModulationDelay(int Delay);
	bool	GetModulationEnable() const;
	void	SetModulationEnable(bool Enable);

protected:
	virtual void	CloneFrom(const CInstrument *pInst);		// // //

private:
	void StoreSequence(CDocumentFile *pDocFile, const CSequence *pSeq);		// // //
	CSequence *LoadSequence(CDocumentFile *pDocFile) const;
	void StoreInstSequence(CInstrumentFile *pDocFile, const CSequence *pSeq);
	CSequence *LoadInstSequence(CInstrumentFile *pFile) const;
	void DoubleVolume() const;		// // //

public:
	static const int WAVE_SIZE = 64;
	static const int MOD_SIZE = 32;
	static const int MOD_Y = 8;
	static const int SEQUENCE_COUNT = 3;		// // //
	static LPCTSTR SEQUENCE_NAME[];
	LPCTSTR	GetSequenceName(int Index) const { return SEQUENCE_NAME[Index]; }		// // //

private:
	// Instrument data
	std::vector<std::unique_ptr<CSequence>> m_pSequence;
	unsigned char m_iSamples[WAVE_SIZE];
	unsigned char m_iModulation[MOD_SIZE];
	int			  m_iModulationSpeed;
	int			  m_iModulationDepth;
	int			  m_iModulationDelay;
	bool		  m_bModulationEnable;
	
public: // // // porting CSeqInstrument
	virtual int		GetSeqEnable(int Index) const;
	virtual int		GetSeqIndex(int Index) const;
	virtual void	SetSeqIndex(int Index, int Value);
	CSequence		*GetSequence(int SeqType) const;		// // //
	virtual void	SetSequence(int SeqType, CSequence *pSeq);		// // //
};
