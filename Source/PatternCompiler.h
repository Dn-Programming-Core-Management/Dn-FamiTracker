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

#pragma once

class CFamiTrackerDoc;
class CCompilerLog;

typedef unsigned char DPCM_List_t[MAX_INSTRUMENTS][OCTAVE_RANGE][NOTE_RANGE];

class CPatternCompiler
{
public:
	CPatternCompiler(CFamiTrackerDoc *pDoc, unsigned int *pInstList, DPCM_List_t *pDPCMList, CCompilerLog *pLogger);
	~CPatternCompiler();

	void			CompileData(int Track, int Pattern, int Channel);
	
	unsigned int	GetHash() const;
	bool			CompareData(const std::vector<char> &data) const;

	const std::vector<char> &GetData() const;
	const std::vector<char> &GetCompressedData() const;

	unsigned int	GetDataSize() const;
	unsigned int	GetCompressedDataSize() const;

private:	
	struct stSpacingInfo {
		int SpaceCount;
		int SpaceSize;
	};

private:
	unsigned int	FindInstrument(int Instrument) const;
	unsigned int	FindSample(int Instrument, int Octave, int Key) const;

	unsigned char	Command(int cmd) const;

	void			WriteData(unsigned char Value);
	void			WriteDuration();
	void			AccumulateDuration();
	void			OptimizeString();
	int				GetBlockSize(int Position);
	void			ScanNoteLengths(stSpacingInfo &Info, int Track, unsigned int StartRow, int Pattern, int Channel);

	// Debugging
	void			Print(LPCTSTR text) const;

private:
	std::vector<char> m_vData;
	std::vector<char> m_vCompressedData;

	unsigned int	m_iDuration;
	unsigned int	m_iCurrentDefaultDuration;
	bool			m_bDSamplesAccessed[OCTAVE_RANGE * NOTE_RANGE]; // <- check the range, its not optimal right now
	unsigned int	m_iHash;
	unsigned int	*m_pInstrumentList;

	DPCM_List_t		*m_pDPCMList;

	CFamiTrackerDoc *m_pDocument;
	CCompilerLog	*m_pLogger;
};
