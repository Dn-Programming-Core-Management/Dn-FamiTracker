/*
** FamiTracker - NES/Famicom sound tracker
** Copyright (C) 2005-2014  Jonathan Liss
**
** 0CC-FamiTracker is (C) 2014-2016 HertzDevil
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

private:
	void StoreSequence(CDocumentFile *pDocFile, const CSequence *pSeq);		// // //
	CSequence *LoadSequence(CDocumentFile *pDocFile) const;
	void StoreInstSequence(CInstrumentFile *pDocFile, const CSequence *pSeq);
	CSequence *LoadInstSequence(CInstrumentFile *pFile) const;
	void DoubleVolume() const;		// // //

public:
	static const int WAVE_SIZE = 64;
	static const int MOD_SIZE = 32;
	static const int SEQUENCE_COUNT = 3;		// // //

private:
	// Instrument data
	std::vector<std::unique_ptr<CSequence>> m_pSequence;
	unsigned char m_iSamples[64];
	unsigned char m_iModulation[32];
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
