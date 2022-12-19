/*
** FamiTracker - NES/Famicom sound tracker
** Copyright (C) 2005-2020 Jonathan Liss
**
** 0CC-FamiTracker is (C) 2014-2018 HertzDevil
**
** Dn-FamiTracker is (C) 2020-2022 D.P.C.M.
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
