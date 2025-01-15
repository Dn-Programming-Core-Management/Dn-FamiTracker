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
#include "name.h"


#define STR_HELPER(x) #x
#define STR(x) STR_HELPER(x)


// Define this for beta builds
#define WIP


#define VERSION_API  0
#define VERSION_MAJ  5
#define VERSION_MIN  1
#define VERSION_REV  1

#define VERSION VERSION_API,VERSION_MAJ,VERSION_MIN,VERSION_REV

#define VERSION_STR \
		STR(VERSION_API) "." \
		STR(VERSION_MAJ) "." \
		STR(VERSION_MIN) "." \
		STR(VERSION_REV)


#define APP_NAME_VERSION	APP_NAME " " VERSION_STR
