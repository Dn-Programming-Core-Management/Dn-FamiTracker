/*
** FamiTracker - NES/Famicom sound tracker
** Copyright (C) 2005-2015 Jonathan Liss
**
** 0CC-FamiTracker is (C) 2014-2018 HertzDevil
**
** Dn-FamiTracker is (C) 2020-2021 D.P.C.M.
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

//
// The NSF driver binaries
//

// Change this for new versions
const char DRIVER_ID[] = "NSF-driver v2.11";

#pragma warning( disable : 4309 ) // disable warning 4309: 'initializing' : truncation of constant value
#pragma warning( disable : 4838 ) // 4838: conversion from 'int' to 'const char' requires a narrowing conversion

const unsigned short NSF_CALLER_SIZE = 128;	// bytes

// NES program for running a NSF
const char NSF_CALLER_BIN[] = {
	0x78,0xD8,0xAD,0x02,0x20,0x10,0xFB,0xAD,0x02,0x20,0x10,0xFB,0xA2,0x00,0x8A,0x9D,
	0x00,0x02,0xE8,0xD0,0xFA,0xA9,0x0F,0x8D,0x15,0x40,0xA9,0x0A,0x8D,0x10,0x40,0xA9,
	0x00,0xA2,0x00,0xA0,0x00,0x20,0x08,0x80,0xA9,0x80,0x8D,0x00,0x20,0xA9,0x00,0x8D,
	0x01,0x20,0x4C,0xB2,0xFF,0x20,0x0B,0x80,0x40,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xB5,0xFF,0x80,0xFF,0xB8,0xFF
};

const char NSF_CALLER_BIN_VRC6[] = {
	0x78,0xD8,0xAD,0x02,0x20,0x10,0xFB,0xAD,0x02,0x20,0x10,0xFB,0xA2,0x00,0x8A,0x9D,
	0x00,0x02,0xE8,0xD0,0xFA,0xA9,0x00,0x8D,0x00,0x80,0xA9,0x02,0x8D,0x00,0xC0,0xA9,
	0x0F,0x8D,0x15,0x40,0xA9,0x0A,0x8D,0x10,0x40,0xA9,0x00,0xA2,0x00,0xA0,0x00,0x20,
	0x00,0x80,0xA9,0x80,0x8D,0x00,0x20,0xA9,0x00,0x8D,0x01,0x20,0x4C,0xBC,0xFF,0x20,
	0x03,0x80,0x40,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xBF,0xFF,0x80,0xFF,0xC2,0xFF
};

#include "DetuneTable.h"		// // //
//
// The driver binaries
//
// created with a binary->c-array program
//
#include "drivers/drv_2a03.h"
#include "drivers/drv_vrc6.h"
#include "drivers/drv_vrc7.h"
#include "drivers/drv_mmc5.h"
#include "drivers/drv_fds.h"
#include "drivers/drv_n163.h"
#include "drivers/drv_s5b.h"		// // //
#include "drivers/drv_all.h"		// // //

struct driver_t {
	const unsigned char *driver;
	const unsigned int driver_size;
	const int *word_reloc;
	const size_t word_reloc_size;
	const int *freq_table;		// // //
	const size_t freq_table_size;	// // //
	const int *adr_reloc;		// // //
	const size_t adr_reloc_size;	// // //
};

const driver_t DRIVER_PACK_2A03 = { 
	DRIVER_2A03, 
	sizeof(DRIVER_2A03),
	DRIVER_RELOC_WORD_2A03, 
	sizeof(DRIVER_RELOC_WORD_2A03) / sizeof(int), 
	DRIVER_FREQ_TABLE_2A03,				// // //
	sizeof(DRIVER_FREQ_TABLE_2A03) / sizeof(int),
	DRIVER_RELOC_ADR_2A03,
	sizeof(DRIVER_RELOC_ADR_2A03) / sizeof(int),
};

const driver_t DRIVER_PACK_VRC6 = { 
	DRIVER_VRC6, 
	sizeof(DRIVER_VRC6),
	DRIVER_RELOC_WORD_VRC6, 
	sizeof(DRIVER_RELOC_WORD_VRC6) / sizeof(int), 
	DRIVER_FREQ_TABLE_VRC6,				// // //
	sizeof(DRIVER_FREQ_TABLE_VRC6) / sizeof(int),
	DRIVER_RELOC_ADR_VRC6,
	sizeof(DRIVER_RELOC_ADR_VRC6) / sizeof(int),
};

const driver_t DRIVER_PACK_VRC7 = { 
	DRIVER_VRC7, 
	sizeof(DRIVER_VRC7),
	DRIVER_RELOC_WORD_VRC7, 
	sizeof(DRIVER_RELOC_WORD_VRC7) / sizeof(int), 
	DRIVER_FREQ_TABLE_VRC7,				// // //
	sizeof(DRIVER_FREQ_TABLE_VRC7) / sizeof(int),
	DRIVER_RELOC_ADR_VRC7,
	sizeof(DRIVER_RELOC_ADR_VRC7) / sizeof(int),
};

const driver_t DRIVER_PACK_MMC5 = { 
	DRIVER_MMC5, 
	sizeof(DRIVER_MMC5),
	DRIVER_RELOC_WORD_MMC5, 
	sizeof(DRIVER_RELOC_WORD_MMC5) / sizeof(int), 
	DRIVER_FREQ_TABLE_MMC5,				// // //
	sizeof(DRIVER_FREQ_TABLE_MMC5) / sizeof(int),
	DRIVER_RELOC_ADR_MMC5,
	sizeof(DRIVER_RELOC_ADR_MMC5) / sizeof(int),
};

const driver_t DRIVER_PACK_FDS = { 
	DRIVER_FDS, 
	sizeof(DRIVER_FDS),
	DRIVER_RELOC_WORD_FDS, 
	sizeof(DRIVER_RELOC_WORD_FDS) / sizeof(int), 
	DRIVER_FREQ_TABLE_FDS,				// // //
	sizeof(DRIVER_FREQ_TABLE_FDS) / sizeof(int),
	DRIVER_RELOC_ADR_FDS,
	sizeof(DRIVER_RELOC_ADR_FDS) / sizeof(int),
};

const driver_t DRIVER_PACK_N163 = { 
	DRIVER_N163, 
	sizeof(DRIVER_N163),
	DRIVER_RELOC_WORD_N163, 
	sizeof(DRIVER_RELOC_WORD_N163) / sizeof(int),
	DRIVER_FREQ_TABLE_N163,				// // //
	sizeof(DRIVER_FREQ_TABLE_N163) / sizeof(int), 
	DRIVER_RELOC_ADR_N163,
	sizeof(DRIVER_RELOC_ADR_N163) / sizeof(int),
};

const driver_t DRIVER_PACK_S5B = {		// // //
	DRIVER_S5B, 
	sizeof(DRIVER_S5B),
	DRIVER_RELOC_WORD_S5B, 
	sizeof(DRIVER_RELOC_WORD_S5B) / sizeof(int),
	DRIVER_FREQ_TABLE_S5B,				// // //
	sizeof(DRIVER_FREQ_TABLE_S5B) / sizeof(int),
	DRIVER_RELOC_ADR_S5B,
	sizeof(DRIVER_RELOC_ADR_S5B) / sizeof(int), 
};

const driver_t DRIVER_PACK_ALL = {		// // //
	DRIVER_ALL, 
	sizeof(DRIVER_ALL),
	DRIVER_RELOC_WORD_ALL, 
	sizeof(DRIVER_RELOC_WORD_ALL) / sizeof(int), 
	DRIVER_FREQ_TABLE_ALL,				// // //
	sizeof(DRIVER_FREQ_TABLE_ALL) / sizeof(int),
	DRIVER_RELOC_ADR_ALL,
	sizeof(DRIVER_RELOC_ADR_ALL) / sizeof(int),
};
