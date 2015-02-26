/*
** FamiTracker - NES/Famicom sound tracker
** Copyright (C) 2005-2014  Jonathan Liss
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

#include "stdafx.h"
#include "VisualizerWnd.h"
#include "VisualizerStatic.h"
#include "resource.h"

/*
 * Display a static image
 *
 */

CVisualizerStatic::CVisualizerStatic()
{
}

CVisualizerStatic::~CVisualizerStatic()
{
	if (m_dcImage.m_hDC != NULL) {
		m_dcImage.SelectObject(m_pOldBmp);
	}
}

void CVisualizerStatic::Create(int Width, int Height)
{
	CVisualizerBase::Create(Width, Height);
}

void CVisualizerStatic::SetSampleRate(int SampleRate)
{
}

void CVisualizerStatic::Draw()
{
}

void CVisualizerStatic::Display(CDC *pDC, bool bPaintMsg)
{
	if (!bPaintMsg)
		return;

	if (m_dcImage.m_hDC == NULL) {
		m_bmpImage.LoadBitmap(IDB_VISUALIZER);
		m_dcImage.CreateCompatibleDC(pDC);
		m_pOldBmp = m_dcImage.SelectObject(&m_bmpImage);
	}

	pDC->BitBlt(0, 0, m_iWidth, m_iHeight, &m_dcImage, 0, 0, SRCCOPY);
}
