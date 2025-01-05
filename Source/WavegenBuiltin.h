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


#include "WaveformGenerator.h"

/**
	\brief Partial implementation of the waveform generator.
*/
class CWavegenImp : public CWaveformGenerator {
public:
	CWavegenImp(const char *Name);
	virtual bool CreateWaves(float *const Dest, unsigned int Size, unsigned int Index);
	const char *GetGeneratorName() const;
	virtual const char *GetStatus() const;

protected:
	virtual const char *CreateWavesInternal(float *const Dest, unsigned int Size, unsigned int Index) const = 0;

protected:
	bool m_bSuccess;
	const char *m_pName;
	const char **m_pError;

private:
	static const char *DEFAULT_ERROR;
};

class CWavegenSingle : public CWavegenImp {
public:
	CWavegenSingle(const char *Name);
	virtual bool CreateWaves(float *const Dest, unsigned int Size, unsigned int Index);
	unsigned int GetCount() const;
private:
	static const char *COUNT_ERROR;
};

class CWavegenSine : public CWavegenSingle {
public:
	CWavegenSine();
	CWavegenParam *GetParameter(unsigned int Index) const;
protected:
	const char *CreateWavesInternal(float *const Dest, unsigned int Size, unsigned int Index) const;
};

class CWavegenSawtooth : public CWavegenSingle {
public:
	CWavegenSawtooth();
	CWavegenParam *GetParameter(unsigned int Index) const;
protected:
	const char *CreateWavesInternal(float *const Dest, unsigned int Size, unsigned int Index) const;
};

class CWavegenTriangle : public CWavegenSingle {
public:
	CWavegenTriangle();
	CWavegenParam *GetParameter(unsigned int Index) const;
protected:
	const char *CreateWavesInternal(float *const Dest, unsigned int Size, unsigned int Index) const;
};

class CWavegenPulse : public CWavegenSingle {
public:
	CWavegenPulse();
	virtual ~CWavegenPulse();
	CWavegenParam *GetParameter(unsigned int Index) const;
protected:
	const char *CreateWavesInternal(float *const Dest, unsigned int Size, unsigned int Index) const;
private:
	CWavegenParamFloat *m_pPulseWidth;
	static const char *PULSE_WIDTH_ERROR;
};
