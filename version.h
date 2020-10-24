/*
** FamiTracker - NES/Famicom sound tracker
** Copyright (C) 2005-2014  Jonathan Liss
**
** 0CC-FamiTracker is (C) 2014-2016 HertzDevil
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
#include "name.h"


#define STR_HELPER(x) #x
#define STR(x) STR_HELPER(x)


// Define this for beta builds
#define WIP


#define VERSION_API  0
#define VERSION_MAJ  2
#define VERSION_MIN  1
#define VERSION_REV  0

#define VERSION VERSION_API,VERSION_MAJ,VERSION_MIN,VERSION_REV

#define VERSION_STR \
		STR(VERSION_API) "." \
		STR(VERSION_MAJ) "." \
		STR(VERSION_MIN) "." \
		STR(VERSION_REV)


#define APP_NAME_VERSION	APP_NAME " " VERSION_STR
