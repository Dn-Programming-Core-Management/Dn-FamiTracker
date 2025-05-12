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

class CFamiTrackerDoc;
class CCompilerLog;

typedef unsigned char DPCM_List_t[MAX_INSTRUMENTS][OCTAVE_RANGE][NOTE_RANGE];

class CPatternCompiler
{
public:
	CPatternCompiler(CFamiTrackerDoc *pDoc, unsigned int *pInstList, DPCM_List_t *pDPCMList, CCompilerLog *pLogger);
	~CPatternCompiler();

	void			CompileData(int Track, int Pattern, int Channel, bool bUseAllExp = true);
	
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
	template <typename... T>
	void	Print(std::string_view text, T... args) const;		// !! !!

private:
	std::vector<char> m_vData;
	std::vector<char> m_vCompressedData;

	unsigned int	m_iDuration;
	unsigned int	m_iCurrentDefaultDuration;
	bool			m_bDSamplesAccessed[OCTAVE_RANGE * NOTE_RANGE]; // <- check the range, its not optimal right now
	bool			m_bUseAllChips;		// !! !! we store a local copy to accomodate both NSF and .asm/.bin export
	unsigned int	m_iHash;
	unsigned int	*m_pInstrumentList;

	DPCM_List_t		*m_pDPCMList;

	CFamiTrackerDoc *m_pDocument;
	CCompilerLog	*m_pLogger;
};
