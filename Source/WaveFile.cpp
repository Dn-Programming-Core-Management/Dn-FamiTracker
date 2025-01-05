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

#include "WaveFile.h"

bool CWaveFile::OpenFile(LPTSTR Filename, int SampleRate, int SampleSize, int Channels)
{
	// Open a wave file for streaming
	//

	int nError;

	WaveFormat.wf.wFormatTag	  = WAVE_FORMAT_PCM;
	WaveFormat.wf.nChannels		  = Channels;
	WaveFormat.wf.nSamplesPerSec  = SampleRate;
	WaveFormat.wf.nBlockAlign	  = (SampleSize / 8) * Channels;
	WaveFormat.wf.nAvgBytesPerSec = SampleRate * (SampleSize / 8) * Channels;
	WaveFormat.wBitsPerSample	  = SampleSize;

	hmmioOut = mmioOpen(Filename, NULL, MMIO_ALLOCBUF | MMIO_READWRITE | MMIO_CREATE);

	ckOutRIFF.fccType = mmioFOURCC('W', 'A', 'V', 'E');
	ckOutRIFF.cksize  = 0;

	nError = mmioCreateChunk(hmmioOut, &ckOutRIFF, MMIO_CREATERIFF);

	if (nError != MMSYSERR_NOERROR)
		return false;

	ckOut.ckid	 = mmioFOURCC('f', 'm', 't', ' ');     
	ckOut.cksize = sizeof(PCMWAVEFORMAT);

	nError = mmioCreateChunk(hmmioOut, &ckOut, 0);

	if (nError != MMSYSERR_NOERROR)
		return false;

	mmioWrite(hmmioOut, (HPSTR)&WaveFormat, sizeof(PCMWAVEFORMAT));
	mmioAscend(hmmioOut, &ckOut, 0);

	ckOut.ckid	 = mmioFOURCC('d', 'a', 't', 'a');
	ckOut.cksize = 0;

	nError = mmioCreateChunk(hmmioOut, &ckOut, 0);

	if (nError != MMSYSERR_NOERROR)
		return false;

	mmioGetInfo(hmmioOut, &mmioinfoOut, 0);	

	return true;
}

void CWaveFile::CloseFile()
{
	// Close the file
	//

	mmioinfoOut.dwFlags |= MMIO_DIRTY;
	mmioSetInfo(hmmioOut, &mmioinfoOut, 0);

	mmioAscend(hmmioOut, &ckOut, 0);
	mmioAscend(hmmioOut, &ckOutRIFF, 0);

	mmioSeek(hmmioOut, 0, SEEK_SET); 
	mmioDescend(hmmioOut, &ckOutRIFF, NULL, 0);

	mmioClose(hmmioOut, 0);
}

void CWaveFile::WriteWave(char *Data, int Size)
{
	// Save data to the file
	//

	int cT;
	
	for (cT = 0; cT < Size; cT++) {
		if (mmioinfoOut.pchNext == mmioinfoOut.pchEndWrite) { 
			mmioinfoOut.dwFlags |= MMIO_DIRTY; 
			mmioAdvance(hmmioOut, &mmioinfoOut, MMIO_WRITE);
		}

		*((BYTE*)mmioinfoOut.pchNext) = *((BYTE*)Data + cT); 
		mmioinfoOut.pchNext++;
	}
}

