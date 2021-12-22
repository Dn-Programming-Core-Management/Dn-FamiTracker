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

