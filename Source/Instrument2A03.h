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


#pragma once

#include "SeqInstrument.h"		// // //

class CDSample;

class CInstrument2A03 : public CSeqInstrument {
public:
	CInstrument2A03();
	CInstrument* Clone() const;
	void	Store(CDocumentFile *pFile) const override;
	bool	Load(CDocumentFile *pDocFile) override;
	void	SaveFile(CSimpleFile *pFile) const override;
	bool	LoadFile(CSimpleFile *pFile, int iVersion) override;

private:
	char	GetSample(int Octave, int Note) const { return GetSampleIndex(Octave, Note); };		// // //
	int		GetSampleCount() const;		// // // 050B

public:
	// Samples
	char	GetSampleIndex(int Octave, int Note) const;
	char	GetSamplePitch(int Octave, int Note) const;
	bool	GetSampleLoop(int Octave, int Note) const;
	char	GetSampleLoopOffset(int Octave, int Note) const;
	char	GetSampleDeltaValue(int Octave, int Note) const;
	void	SetSampleIndex(int Octave, int Note, char Sample);
	void	SetSamplePitch(int Octave, int Note, char Pitch);
	void	SetSampleLoop(int Octave, int Note, bool Loop);
	void	SetSampleLoopOffset(int Octave, int Note, char Offset);
	void	SetSampleDeltaValue(int Octave, int Note, char Offset);

	bool	AssignedSamples() const;
	const CDSample *GetDSample(int Octave, int Note) const;		// // //

protected:
	void	CloneFrom(const CInstrument *pInst) override;		// // //

public:
	static const char *SEQUENCE_NAME[];
	const char *GetSequenceName(int Index) const override { return SEQUENCE_NAME[Index]; }		// // //

private:
	char	m_cSamples[OCTAVE_RANGE][NOTE_RANGE];				// Samples
	char	m_cSamplePitch[OCTAVE_RANGE][NOTE_RANGE];			// Play pitch/loop
	char	m_cSampleLoopOffset[OCTAVE_RANGE][NOTE_RANGE];		// Loop offset
	char	m_cSampleDelta[OCTAVE_RANGE][NOTE_RANGE];			// Delta setting
};
