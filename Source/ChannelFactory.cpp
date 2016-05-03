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

#include "stdafx.h"
#include "Instrument.h" // TODO: remove
#include "ChannelHandler.h"
#include "ChannelFactory.h"

#include "Channels2A03.h"
#include "ChannelsVRC6.h"
#include "ChannelsVRC7.h"
#include "ChannelsFDS.h"
#include "ChannelsMMC5.h"
#include "ChannelsN163.h"
#include "ChannelsS5B.h"

// // // Default implementation for channel factory

CChannelFactory::CChannelFactory() : CFactory()
{
	FuncType Func;

	Func = MakeCtor<C2A03Square>();
	m_pMakeFunc[CHANID_SQUARE1] = Func;
	m_pMakeFunc[CHANID_SQUARE2] = Func;
	AddProduct<CTriangleChan>(CHANID_TRIANGLE);
	AddProduct<CNoiseChan>(CHANID_NOISE);
	AddProduct<CDPCMChan>(CHANID_DPCM);
	
	Func = MakeCtor<CVRC6Square>();
	m_pMakeFunc[CHANID_VRC6_PULSE1] = Func;
	m_pMakeFunc[CHANID_VRC6_PULSE2] = Func;
	AddProduct<CVRC6Sawtooth>(CHANID_VRC6_SAWTOOTH);

	Func = MakeCtor<CVRC7Channel>();
	m_pMakeFunc[CHANID_VRC7_CH1] = Func;
	m_pMakeFunc[CHANID_VRC7_CH2] = Func;
	m_pMakeFunc[CHANID_VRC7_CH3] = Func;
	m_pMakeFunc[CHANID_VRC7_CH4] = Func;
	m_pMakeFunc[CHANID_VRC7_CH5] = Func;
	m_pMakeFunc[CHANID_VRC7_CH6] = Func;

	AddProduct<CChannelHandlerFDS>(CHANID_FDS);
	
	Func = MakeCtor<CChannelHandlerMMC5>();
	m_pMakeFunc[CHANID_MMC5_SQUARE1] = Func;
	m_pMakeFunc[CHANID_MMC5_SQUARE2] = Func;
	
	Func = MakeCtor<CChannelHandlerN163>();
	m_pMakeFunc[CHANID_N163_CH1] = Func;
	m_pMakeFunc[CHANID_N163_CH2] = Func;
	m_pMakeFunc[CHANID_N163_CH3] = Func;
	m_pMakeFunc[CHANID_N163_CH4] = Func;
	m_pMakeFunc[CHANID_N163_CH5] = Func;
	m_pMakeFunc[CHANID_N163_CH6] = Func;
	m_pMakeFunc[CHANID_N163_CH7] = Func;
	m_pMakeFunc[CHANID_N163_CH8] = Func;
	
	Func = MakeCtor<CChannelHandlerS5B>();
	m_pMakeFunc[CHANID_S5B_CH1] = Func;
	m_pMakeFunc[CHANID_S5B_CH2] = Func;
	m_pMakeFunc[CHANID_S5B_CH3] = Func;
}
