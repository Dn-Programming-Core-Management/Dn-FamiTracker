/*
** FamiTracker - NES/Famicom sound tracker
** Copyright (C) 2005-2012  Jonathan Liss
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
#include "FamiTracker.h"
#include "ConfigWindow.h"
#include "ConfigGeneral.h"
#include "ConfigAppearance.h"
#include "ConfigSound.h"
#include "Settings.h"


// CConfigWindow

IMPLEMENT_DYNAMIC(CConfigWindow, CPropertySheet)
CConfigWindow::CConfigWindow(UINT nIDCaption, CWnd* pParentWnd, UINT iSelectPage)
	:CPropertySheet(nIDCaption, pParentWnd, iSelectPage)
{
}

CConfigWindow::CConfigWindow(LPCTSTR pszCaption, CWnd* pParentWnd, UINT iSelectPage)
	:CPropertySheet(pszCaption, pParentWnd, iSelectPage, NULL, NULL)
{
}

CConfigWindow::~CConfigWindow()
{
}


BEGIN_MESSAGE_MAP(CConfigWindow, CPropertySheet)
END_MESSAGE_MAP()


// CConfigWindow message handlers

void CConfigWindow::SetupPages(void)
{
}
