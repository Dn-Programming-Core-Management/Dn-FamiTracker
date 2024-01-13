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

// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently,
// but are changed infrequently

#pragma once

#define _CRTDBG_MAPALLOC
#define NOMINMAX

// Get rid of warnings in VS 2005
#define _CRT_SECURE_NO_DEPRECATE
#define _CRT_SECURE_NO_WARNINGS

#ifndef VC_EXTRALEAN
#define VC_EXTRALEAN		// Exclude rarely-used stuff from Windows headers
#endif

// Modify the following defines if you have to target a platform prior to the ones specified below.
// Refer to MSDN for the latest info on corresponding values for different platforms.
#ifndef WINVER
#define WINVER _WIN32_WINNT_VISTA
#endif

#ifndef _WIN32_WINNT
#define _WIN32_WINNT _WIN32_WINNT_VISTA
#endif

#ifndef _WIN32_WINDOWS
#define _WIN32_WINDOWS _WIN32_WINNT_VISTA
#endif

#ifndef _WIN32_IE
#define _WIN32_IE _WIN32_WINNT_VISTA
#endif

#define _ATL_CSTRING_EXPLICIT_CONSTRUCTORS	// some CString constructors will be explicit

// turns off MFC's hiding of some common and often safely ignored warning messages
#define _AFX_ALL_WARNINGS

#define NO_WARN_MBCS_MFC_DEPRECATION		// // // MBCS

#include <afx.h>
#include <afxwin.h>         // MFC core and standard components
#include <afxext.h>         // MFC extensions

#include <afxdtctl.h>		// MFC support for Internet Explorer 4 Common Controls
#ifndef _AFX_NO_AFXCMN_SUPPORT
#include <afxcmn.h>			// MFC support for Windows Common Controls
#endif // _AFX_NO_AFXCMN_SUPPORT
#include <afxadv.h>
#include <afxdhtml.h>
#include <afxdlgs.h>

#include <afxole.h>        // MFC OLE support

// Releasing pointers
#define SAFE_RELEASE(p) \
	if (p != NULL) { \
		delete p;	\
		p = NULL;	\
	}	\

#define SAFE_RELEASE_ARRAY(p) \
	if (p != NULL) { \
		delete [] p;	\
		p = NULL;	\
	}	\

// Calling member function
#define CALL_MEMBER_FN(obj, ptr) ((obj)->*(ptr))

#ifdef TRACE
#undef TRACE
#endif

#ifdef _DEBUG
template <typename... T>
bool _trace(TCHAR *format, T... args)
{
	TCHAR buffer[1000];
	_sntprintf_s(buffer, sizeof(buffer), _TRUNCATE, format, args...);
	OutputDebugString(buffer);

	return true;
}
#define TRACE _trace
#else
#define TRACE __noop
#endif


#include "name.h"
