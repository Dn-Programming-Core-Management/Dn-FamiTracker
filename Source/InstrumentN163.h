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


class CInstrumentN163 : public CSeqInstrument {
public:
	CInstrumentN163();
	CInstrument* Clone() const;
	void	Store(CDocumentFile *pDocFile);
	bool	Load(CDocumentFile *pDocFile);
	void	SaveFile(CInstrumentFile *pFile);
	bool	LoadFile(CInstrumentFile *pFile, int iVersion);
	int		Compile(CChunk *pChunk, int Index);

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

protected:
	virtual void	CloneFrom(const CInstrument *pInst);		// // //

public:
	static const int MAX_WAVE_SIZE = 240;		// Wave size (240 samples)		// // //
	static const int MAX_WAVE_COUNT = 64;		// Number of waves

public:
	static LPCTSTR SEQUENCE_NAME[];
	LPCTSTR	GetSequenceName(int Index) const { return SEQUENCE_NAME[Index]; }		// // //

private:
	int		m_iSamples[MAX_WAVE_COUNT][MAX_WAVE_SIZE];
	int		m_iWaveSize;
	int		m_iWavePos;
	bool	m_bAutoWavePos;		// // // 050B
	int		m_iWaveCount;
};
