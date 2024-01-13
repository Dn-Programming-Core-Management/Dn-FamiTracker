/*
** FamiTracker - NES/Famicom sound tracker
** Copyright (C) 2005-2020 Jonathan Liss
**
** 0CC-FamiTracker is (C) 2014-2018 HertzDevil
**
** Dn-FamiTracker is (C) 2020-2024 D.P.C.M.
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

// // // 050B
// common base class for pattern editor components

enum class pattern_component_t : int
{
	HEADER = 1,
	EDITOR = 2,
	EFFECT = 3,
};

/*!
	\brief A class which represents an independent component of the pattern editor. This class
	handles several mouse events while the drawing takes place in the new CPatternEditor class in
	0.5.0 beta.
*/
class CPatternComponent
{
public:
	/*!	Constructor of the pattern component.
		*/
	CPatternComponent(int Arg);
	/*!	Destructor of the pattern component. */
	virtual ~CPatternComponent();

	/*!	\brief Checks whether the component contains a given point.
		\return True if the point lies within the rectangle defined by this component.
		\param point A point struct. */
	virtual bool ContainsPoint(const CPoint &point) const;
	
	/*!	\brief Called when the cursor clicks on the component.
		\param point The cursor position. */
	virtual void OnMouseDown(const CPoint &point) = 0;
	/*!	\brief Called when the cursor releases on the component.
		\param point The cursor position. */
	virtual void OnMouseUp(const CPoint &point) = 0;
	/*!	\brief Called when the cursor drags over the component.
		\param point The cursor position. */
	virtual void OnMouseMove(const CPoint &point) = 0;
	/*!	\brief Called when the cursor double-clicks on the component.
		\param point The cursor position. */
	virtual void OnMouseDblClk(const CPoint &point) = 0;

private:
	int m_i0004;
	int m_i0008;
	CRect m_000C;
	CRect m_001C;
	int m_i002C;
	bool m_b0030;
};
