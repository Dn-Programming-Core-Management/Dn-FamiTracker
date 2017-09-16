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

#include <memory>
#include <cstdint>
#include <string>

class CWaveFile;

class CWaveRenderer {
public:
	virtual ~CWaveRenderer();

	void SetOutputFile(std::unique_ptr<CWaveFile> pWave);
	void CloseOutputFile();
	void FlushBuffer(char *pBuf, unsigned Size) const;

	void Start();
	virtual void Tick() { }
	virtual void StepRow() { }

	bool ShouldStartPlayer();
	bool ShouldStopPlayer() const;		// // //
	bool ShouldStopRender();

	bool Started() const;
	bool Finished() const;

	void SetRenderTrack(int Track);
	int GetRenderTrack() const;
	virtual std::string GetProgressString() const = 0;
	virtual int GetProgressPercent() const = 0;

protected:
	void FinishRender();

private:
	std::unique_ptr<CWaveFile> m_pWaveFile;
	bool m_bStarted = false;
	bool m_bFinished = false;

	bool m_bRequestRenderStop = false;
	bool m_bStoppingRender = false;		// // //
	int m_iDelayedStart = 5;
	int m_iDelayedEnd = 5;
	int m_iRenderTrack;
	unsigned int m_iRenderRowCount = 0;
};

class CWaveRendererTick : public CWaveRenderer {
public:
	explicit CWaveRendererTick(unsigned Ticks, double Rate);

private:
	void Tick() override;
	std::string GetProgressString() const override;
	int GetProgressPercent() const override;

private:
	unsigned m_iTicksToRender;
	unsigned m_iRenderTick = 0;
	double m_fFrameRate;
};

class CWaveRendererRow : public CWaveRenderer {
public:
	explicit CWaveRendererRow(unsigned Rows);

private:
	void StepRow() override;
	std::string GetProgressString() const override;
	int GetProgressPercent() const override;

private:
	unsigned m_iRowsToRender;
	unsigned m_iRenderRow = 0;
};
