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

#include "CustomExporterInterfaces.h"

// Instrument types
enum inst_type_t {
	INST_NONE = 0,
	INST_2A03 = 1,
	INST_VRC6,
	INST_VRC7,
	INST_FDS,
	INST_N163,
	INST_S5B
};

// External classes
class CCompiler;
class CDocumentFile;
class CSequence;
class CFamiTrackerDoc;

class CChunk;

class CRefCounter {
public:
	CRefCounter();
	virtual ~CRefCounter();
	void Retain();
	void Release();
private:
	volatile int m_iRefCounter;
};

// Instrument file load/store
class CInstrumentFile : public CFile
{
public:
	CInstrumentFile(LPCTSTR lpszFileName, UINT nOpenFlags) : CFile(lpszFileName, nOpenFlags) {};
	void WriteInt(unsigned int Value);
	void WriteChar(unsigned char Value);
	unsigned int ReadInt();
	unsigned char ReadChar();
};

// Instrument base class
class CInstrument : public CRefCounter {
public:
	CInstrument();
	virtual ~CInstrument();
	void SetName(const char *Name);
	void GetName(char *Name) const;
	const char* GetName() const;
public:
	virtual inst_type_t GetType() const = 0;										// Returns instrument type
	virtual CInstrument* CreateNew() const = 0;										// Creates a new object
	virtual CInstrument* Clone() const = 0;											// Creates a copy
	virtual void Setup() = 0;														// Setup some initial values
	virtual void Store(CDocumentFile *pDocFile) = 0;								// Saves the instrument to the module
	virtual bool Load(CDocumentFile *pDocFile) = 0;									// Loads the instrument from a module
	virtual void SaveFile(CInstrumentFile *pFile, const CFamiTrackerDoc *pDoc) = 0;	// Saves to an FTI file
	virtual bool LoadFile(CInstrumentFile *pFile, int iVersion, CFamiTrackerDoc *pDoc) = 0;	// Loads from an FTI file
	virtual int Compile(CFamiTrackerDoc *pDoc, CChunk *pChunk, int Index) = 0;		// Compiles the instrument for NSF generation
	virtual bool CanRelease() const = 0;
protected:
	void InstrumentChanged() const;
public:
	static const int INST_NAME_MAX = 128;
private:
	char m_cName[INST_NAME_MAX];
	int	 m_iType;
};

class CSeqInstrument : public CInstrument		// // //
{
public:
	CSeqInstrument();
	virtual int		GetSeqEnable(int Index) const;
	virtual int		GetSeqIndex(int Index) const;
	virtual void	SetSeqIndex(int Index, int Value);
	virtual void	SetSeqEnable(int Index, int Value);

public:
	static const int SEQUENCE_TYPES[];

protected:
	int		m_iSeqEnable[SEQ_COUNT];		// // //
	int		m_iSeqIndex[SEQ_COUNT];
};

class CInstrument2A03 : public CSeqInstrument/*, public CInstrument2A03Interface*/ {
public:
	CInstrument2A03();
	virtual inst_type_t	GetType() const { return INST_2A03; };
	virtual CInstrument* CreateNew() const { return new CInstrument2A03(); };
	virtual CInstrument* Clone() const;
	virtual void Setup();
	virtual void Store(CDocumentFile *pFile);
	virtual bool Load(CDocumentFile *pDocFile);
	virtual void SaveFile(CInstrumentFile *pFile, const CFamiTrackerDoc *pDoc);
	virtual bool LoadFile(CInstrumentFile *pFile, int iVersion, CFamiTrackerDoc *pDoc);
	virtual int Compile(CFamiTrackerDoc *pDoc, CChunk *pChunk, int Index);
	virtual bool CanRelease() const;

public:
	// Samples
	char	GetSample(int Octave, int Note) const;
	char	GetSamplePitch(int Octave, int Note) const;
	bool	GetSampleLoop(int Octave, int Note) const;
	char	GetSampleLoopOffset(int Octave, int Note) const;
	char	GetSampleDeltaValue(int Octave, int Note) const;
	void	SetSample(int Octave, int Note, char Sample);
	void	SetSamplePitch(int Octave, int Note, char Pitch);
	void	SetSampleLoop(int Octave, int Note, bool Loop);
	void	SetSampleLoopOffset(int Octave, int Note, char Offset);
	void	SetSampleDeltaValue(int Octave, int Note, char Offset);

	bool	AssignedSamples() const;

public:
	static const int SEQUENCE_TYPES[];

private:
	char	m_cSamples[OCTAVE_RANGE][12];				// Samples
	char	m_cSamplePitch[OCTAVE_RANGE][12];			// Play pitch/loop
	char	m_cSampleLoopOffset[OCTAVE_RANGE][12];		// Loop offset
	char	m_cSampleDelta[OCTAVE_RANGE][12];			// Delta setting

};

class CInstrumentVRC6 : public CSeqInstrument {
public:
	CInstrumentVRC6();
	virtual inst_type_t	GetType() const { return INST_VRC6; };
	virtual CInstrument* CreateNew() const { return new CInstrumentVRC6(); };
	virtual CInstrument* Clone() const;
	virtual void Setup();
	virtual void Store(CDocumentFile *pDocFile);
	virtual bool Load(CDocumentFile *pDocFile);
	virtual void SaveFile(CInstrumentFile *pFile, const CFamiTrackerDoc *pDoc);
	virtual bool LoadFile(CInstrumentFile *pFile, int iVersion, CFamiTrackerDoc *pDoc);
	virtual int Compile(CFamiTrackerDoc *pDoc, CChunk *pChunk, int Index);
	virtual bool CanRelease() const;

public:
	static const int SEQUENCE_TYPES[];
};

class CInstrumentVRC7 : public CInstrument {
public:
	CInstrumentVRC7();
	virtual inst_type_t	GetType() const { return INST_VRC7; };
	virtual CInstrument* CreateNew() const { return new CInstrumentVRC7(); };
	virtual CInstrument* Clone() const;
	virtual void Setup();
	virtual void Store(CDocumentFile *pDocFile);
	virtual bool Load(CDocumentFile *pDocFile);
	virtual void SaveFile(CInstrumentFile *pFile, const CFamiTrackerDoc *pDoc);
	virtual bool LoadFile(CInstrumentFile *pFile, int iVersion, CFamiTrackerDoc *pDoc);
	virtual int Compile(CFamiTrackerDoc *pDoc, CChunk *pChunk, int Index);
	virtual bool CanRelease() const;

public:
	void		 SetPatch(unsigned int Patch);
	unsigned int GetPatch() const;
	void		 SetCustomReg(int Reg, unsigned char Value);		// // //
	unsigned char GetCustomReg(int Reg) const;		// // //

private:
	unsigned int m_iPatch;
	unsigned char m_iRegs[8];		// // // Custom patch settings
};

class CInstrumentFDS : public CInstrument {
public:
	CInstrumentFDS();
	virtual ~CInstrumentFDS();
	virtual inst_type_t GetType() const { return INST_FDS; };
	virtual CInstrument* CreateNew() const { return new CInstrumentFDS(); };
	virtual CInstrument* Clone() const;
	virtual void Setup();
	virtual void Store(CDocumentFile *pDocFile);
	virtual bool Load(CDocumentFile *pDocFile);
	virtual void SaveFile(CInstrumentFile *pFile, const CFamiTrackerDoc *pDoc);
	virtual bool LoadFile(CInstrumentFile *pFile, int iVersion, CFamiTrackerDoc *pDoc);
	virtual int Compile(CFamiTrackerDoc *pDoc, CChunk *pChunk, int Index);
	virtual bool CanRelease() const;

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
	CSequence* GetVolumeSeq() const;
	CSequence* GetArpSeq() const;
	CSequence* GetPitchSeq() const;

private:
	void StoreSequence(CDocumentFile *pDocFile, CSequence *pSeq);
	bool LoadSequence(CDocumentFile *pDocFile, CSequence *pSeq);
	void StoreInstSequence(CInstrumentFile *pDocFile, CSequence *pSeq);
	bool LoadInstSequence(CInstrumentFile *pFile, CSequence *pSeq);

public:
	static const int WAVE_SIZE = 64;
	static const int MOD_SIZE = 32;

private:
	// Instrument data
	unsigned char m_iSamples[64];
	unsigned char m_iModulation[32];
	int			  m_iModulationSpeed;
	int			  m_iModulationDepth;
	int			  m_iModulationDelay;
	bool		  m_bModulationEnable;

	CSequence*	  m_pVolume;
	CSequence*	  m_pArpeggio;
	CSequence*	  m_pPitch;
};

class CInstrumentN163 : public CSeqInstrument {
public:
	CInstrumentN163();
	virtual inst_type_t GetType() const { return INST_N163; };
	virtual CInstrument* CreateNew() const { return new CInstrumentN163(); };
	virtual CInstrument* Clone() const;
	virtual void Setup();
	virtual void Store(CDocumentFile *pDocFile);
	virtual bool Load(CDocumentFile *pDocFile);
	virtual void SaveFile(CInstrumentFile *pFile, const CFamiTrackerDoc *pDoc);
	virtual bool LoadFile(CInstrumentFile *pFile, int iVersion, CFamiTrackerDoc *pDoc);
	virtual int Compile(CFamiTrackerDoc *pDoc, CChunk *pChunk, int Index);
	virtual bool CanRelease() const;

public:
	int		GetWaveSize() const;
	void	SetWaveSize(int size);
	int		GetWavePos() const;
	void	SetWavePos(int pos);
	int		GetSample(int wave, int pos) const;
	void	SetSample(int wave, int pos, int sample);
	/*
	void	SetAutoWavePos(bool Enable);
	bool	GetAutoWavePos() const;
	*/
	void	SetWaveCount(int count);
	int		GetWaveCount() const;

	int		StoreWave(CChunk *pChunk) const;
	bool	IsWaveEqual(CInstrumentN163 *pInstrument);
	
	bool	InsertNewWave(int Index);		// // //
	bool	RemoveWave(int Index);		// // //

public:
	static const int SEQUENCE_TYPES[];
	static const int MAX_WAVE_SIZE = 240;		// Wave size (240 samples)		// // //
	static const int MAX_WAVE_COUNT = 16;		// Number of waves

private:
	int		m_iSamples[MAX_WAVE_COUNT][MAX_WAVE_SIZE];
	int		m_iWaveSize;
	int		m_iWavePos;
//	bool	m_bAutoWavePos;
	int		m_iWaveCount;
};

class CInstrumentS5B : public CSeqInstrument {
public:
	CInstrumentS5B();
	virtual inst_type_t GetType() const { return INST_S5B; };
	virtual CInstrument* CreateNew() const { return new CInstrumentS5B(); };
	virtual CInstrument* Clone() const;
	virtual void Setup();
	virtual void Store(CDocumentFile *pDocFile);
	virtual bool Load(CDocumentFile *pDocFile);
	virtual void SaveFile(CInstrumentFile *pFile, const CFamiTrackerDoc *pDoc);
	virtual bool LoadFile(CInstrumentFile *pFile, int iVersion, CFamiTrackerDoc *pDoc);
	virtual int Compile(CFamiTrackerDoc *pDoc, CChunk *pChunk, int Index);
	virtual bool CanRelease() const;

public:
	static const int SEQUENCE_TYPES[];
};

// This takes care of reference counting
// TODO replace this with boost shared_ptr
template <class T>
class CInstrumentContainer {
public:
	CInstrumentContainer(CFamiTrackerDoc *pDoc, int Index) {
		ASSERT(Index < MAX_INSTRUMENTS);
		m_pInstrument = pDoc->GetInstrument(Index);
	}
	~CInstrumentContainer() {
		if (m_pInstrument != NULL)
			m_pInstrument->Release();
	}
	T* operator()() const {
		return dynamic_cast<T*>(m_pInstrument);
	}
private:
	CInstrument *m_pInstrument;
};
