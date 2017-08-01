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

#include "stdafx.h"		// // //
#include "resource.h"		// // //

// CConfigMIDI dialog

class CConfigMIDI : public CPropertyPage
{
	DECLARE_DYNAMIC(CConfigMIDI)

public:
	CConfigMIDI();
	virtual ~CConfigMIDI();

// Dialog Data
	enum { IDD = IDD_CONFIG_MIDI };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	virtual BOOL OnInitDialog();
	virtual BOOL OnApply();
	afx_msg void OnCbnSelchangeDevices();
	afx_msg void OnBnClickedMasterSync();
	afx_msg void OnBnClickedKeyRelease();
	afx_msg void OnBnClickedChanmap();
	afx_msg void OnBnClickedVelocity();
	afx_msg void OnBnClickedArpeggiate();
	afx_msg void OnCbnSelchangeOutdevices();
};
