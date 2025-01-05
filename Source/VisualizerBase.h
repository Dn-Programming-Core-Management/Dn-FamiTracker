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

#include "stdafx.h"
#include <memory>		// // //

class CVisualizerBase {		// // //
public:
	virtual ~CVisualizerBase() = default;

	// Create the visualizer
	virtual void Create(int Width, int Height);
	// Set rate of samples
	virtual void SetSampleRate(int SampleRate) = 0;

	// Set new sample data (block-aligned)
	virtual bool SetScopeData(short const* iSamples, unsigned int iCount);

	// Set new sample data (latest)
	virtual bool SetSpectrumData(short const* iSamples, unsigned int iCount);

	// Render an image from the sample data
	virtual void Draw() = 0;
	// Display the image
	virtual void Display(CDC *pDC, bool bPaintMsg);		// // //

protected:
	BITMAPINFO m_bmi;
	std::unique_ptr<COLORREF[]> m_pBlitBuffer;		// // //

	int m_iWidth = 0;
	int m_iHeight = 0;

	unsigned int m_iSampleCount = 0;
	short const* m_pSamples = nullptr;
};
