/*
** FamiTracker - NES/Famicom sound tracker
** Copyright (C) 2005-2014  Jonathan Liss
**
** 0CC-FamiTracker is (C) 2014-2015 HertzDevil
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

#include "../Common.h"

const uint8 SNDCHIP_NONE  = 0;
const uint8 SNDCHIP_VRC6  = 1;			// Konami VRCVI
const uint8 SNDCHIP_VRC7  = 2;			// Konami VRCVII
const uint8 SNDCHIP_FDS	  = 4;			// Famicom Disk Sound
const uint8 SNDCHIP_MMC5  = 8;			// Nintendo MMC5
const uint8 SNDCHIP_N163  = 16;			// Namco N-106
const uint8 SNDCHIP_S5B	  = 32;			// Sunsoft 5B
const uint8 SNDCHIP_2A07  = 128;		// // // compatibility

enum chan_id_t {
	CHANID_SQUARE1,
	CHANID_SQUARE2,
	CHANID_TRIANGLE,
	CHANID_NOISE,
	CHANID_DPCM,

	CHANID_VRC6_PULSE1,
	CHANID_VRC6_PULSE2,
	CHANID_VRC6_SAWTOOTH,

	CHANID_MMC5_SQUARE1,
	CHANID_MMC5_SQUARE2,
	CHANID_MMC5_VOICE,

	CHANID_N163_CH1,		// // //
	CHANID_N163_CH2,
	CHANID_N163_CH3,
	CHANID_N163_CH4,
	CHANID_N163_CH5,
	CHANID_N163_CH6,
	CHANID_N163_CH7,
	CHANID_N163_CH8,

	CHANID_FDS,

	CHANID_VRC7_CH1,
	CHANID_VRC7_CH2,
	CHANID_VRC7_CH3,
	CHANID_VRC7_CH4,
	CHANID_VRC7_CH5,
	CHANID_VRC7_CH6,

	CHANID_S5B_CH1,
	CHANID_S5B_CH2,
	CHANID_S5B_CH3,

	CHANNELS		/* Total number of channels */
};

enum apu_machine_t {
	MACHINE_NTSC, 
	MACHINE_PAL
};
