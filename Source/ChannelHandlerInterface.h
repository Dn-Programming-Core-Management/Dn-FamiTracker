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


/*!
	\brief A pure virtual interface for channel handlers.
	\details This class resembles the part of CSequenceHandler of the official build that is
	unrelated to instruments and sequences handling.
*/
class CChannelHandlerInterface		// // //
{
public:
	virtual int		TriggerNote(int) = 0;

	virtual void	SetVolume(int) = 0;
	virtual void	SetPeriod(int) = 0;
	virtual void	SetNote(int) = 0;
	virtual void	SetDutyPeriod(int) = 0;

	virtual int		GetChannelVolume() const = 0;
	virtual int		GetVolume() const = 0;
	virtual int		GetPeriod() const = 0;
	virtual int		GetNote() const = 0;
	virtual int		GetDutyPeriod() const = 0;

	virtual unsigned char GetArpParam() const = 0;

	virtual bool	IsActive() const = 0;
	virtual bool	IsReleasing() const = 0;
};

class CDSample;
class CChannelHandlerInterfaceDPCM
{
public:
	virtual void	WriteDCOffset(unsigned char) = 0;
	virtual void	SetLoopOffset(unsigned char) = 0;
	virtual void	PlaySample(const CDSample*, int) = 0;
};

class CChannelHandlerInterfaceVRC7
{
public:
	virtual void	SetPatch(unsigned char) = 0;
	virtual void	SetCustomReg(size_t, unsigned char) = 0;
};

class CChannelHandlerInterfaceFDS
{
public:
	virtual void	SetFMSpeed(int) = 0;
	virtual void	SetFMDepth(int) = 0;
	virtual void	SetFMDelay(int) = 0;
	virtual void	FillWaveRAM(const char*) = 0;
	virtual void	FillModulationTable(const char*) = 0;
};

class CChannelHandlerInterfaceN163
{
public:
	virtual void	SetWaveLength(int) = 0;
	virtual void	SetWavePosition(int) = 0;
	virtual void	SetWaveCount(int) = 0;
	virtual void	FillWaveRAM(const char*, int) = 0;
};

class CChannelHandlerInterfaceS5B
{
public:
	virtual void	SetNoiseFreq(int) = 0;
};
