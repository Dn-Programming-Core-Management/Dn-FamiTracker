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

#pragma once


// Application version information

// Define this for beta builds
//#define WIP

// Version info
#define VERSION_MAJ  0
#define VERSION_MIN  3
#define VERSION_REV  9

#define VERSION_WIP  0

#ifdef RELEASE_BUILD

#include "config.h"
#define VERSION VERSION_MAJ,VERSION_MIN,VERSION_REV,SVN_VERSION

#else

#define VERSION VERSION_MAJ,VERSION_MIN,VERSION_REV,VERSION_WIP

#endif /* RELEASE_BUILD */

