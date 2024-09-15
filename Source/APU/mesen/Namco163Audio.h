#pragma once
#include "stdafx.h"
#include "../APU.h"

class Namco163Audio
{
public:
	static constexpr uint32_t AudioRamSize = 0x80;

	int16_t _channelOutput[8];

private:
	uint8_t _ramPosition;
	bool _autoIncrement;
	uint8_t _updateCounter;
	int8_t _currentChannel;
	int16_t _lastOutput;
	bool _disableSound;
	bool _mixLinear;

	enum SoundReg
	{
		FrequencyLow = 0x00,
		PhaseLow = 0x01,
		FrequencyMid = 0x02,
		PhaseMid = 0x03,
		FrequencyHigh = 0x04,
		WaveLength = 0x04,
		PhaseHigh = 0x05,
		WaveAddress = 0x06,
		Volume = 0x07
	};

public:

	uint8_t _internalRam[Namco163Audio::AudioRamSize];

	double GetChannelFrequency(int channel, int cpu_clock) const
	{
		auto period = GetFrequency(channel);
		auto wavelength = GetWaveLength(channel);
		auto channelcount = GetNumberOfChannels() + 1;
		if (wavelength > 0)
			return cpu_clock * (double)period / 983040 / (double)wavelength / (double)channelcount;
		else
			return 0.0;
	}

	uint32_t GetFrequency(int channel) const
	{
		uint8_t baseAddr = 0x40 + channel * 0x08;
		return ((_internalRam[baseAddr + SoundReg::FrequencyHigh] & 0x03) << 16) | (_internalRam[baseAddr + SoundReg::FrequencyMid] << 8) | _internalRam[baseAddr + SoundReg::FrequencyLow];
	}

	uint32_t GetPhase(int channel)
	{
		uint8_t baseAddr = 0x40 + channel * 0x08;
		return (_internalRam[baseAddr + SoundReg::PhaseHigh] << 16) | (_internalRam[baseAddr + SoundReg::PhaseMid] << 8) | _internalRam[baseAddr + SoundReg::PhaseLow];
	}

	void SetPhase(int channel, uint32_t phase)
	{
		uint8_t baseAddr = 0x40 + channel * 0x08;
		_internalRam[baseAddr + SoundReg::PhaseHigh] = (phase >> 16) & 0xFF;
		_internalRam[baseAddr + SoundReg::PhaseMid] = (phase >> 8) & 0xFF;
		_internalRam[baseAddr + SoundReg::PhaseLow] = phase & 0xFF;
	}

	uint8_t GetWaveAddress(int channel)
	{
		uint8_t baseAddr = 0x40 + channel * 0x08;
		return _internalRam[baseAddr + SoundReg::WaveAddress];
	}

	uint8_t GetWaveLength(int channel) const
	{
		uint8_t baseAddr = 0x40 + channel * 0x08;
		return 256 - (_internalRam[baseAddr + SoundReg::WaveLength] & 0xFC);
	}

	uint8_t GetVolume(int channel)
	{
		uint8_t baseAddr = 0x40 + channel * 0x08;
		return _internalRam[baseAddr + SoundReg::Volume] & 0x0F;
	}
	
	uint8_t GetNumberOfChannels() const
	{
		return (_internalRam[0x7F] >> 4) & 0x07;
	}

	uint8_t GetActiveChannel()
	{
		return _currentChannel;
	}

	void SetMixing(bool mixLinear)
	{
		_mixLinear = mixLinear;
	}

	void UpdateChannel(int channel)
	{
		uint32_t phase = GetPhase(channel);
		uint32_t freq = GetFrequency(channel);
		uint8_t length = GetWaveLength(channel);
		uint8_t offset = GetWaveAddress(channel);
		uint8_t volume = GetVolume(channel);

		if(length == 0) {
			phase = 0;
		} else {
			phase = (phase + freq) % (length << 16);
		}
		
		uint8_t samplePosition = ((phase >> 16) + offset) & 0xFF;
		int8_t sample;
		if((samplePosition & 0x01)) {
			sample = _internalRam[samplePosition / 2] >> 4;
		} else {
			sample = _internalRam[samplePosition / 2] & 0x0F;
		}

		_channelOutput[channel] = (sample - 8) * volume;
		SetPhase(channel, phase);
	}

	int16_t UpdateOutputLevel()
	{
		int16_t summedOutput = 0;
		for (int i = 7, min = 7 - GetNumberOfChannels(); i >= min; i--) {
			summedOutput += _channelOutput[i];
		}
		summedOutput /= (GetNumberOfChannels() + 1);
		return (_mixLinear ? summedOutput : _channelOutput[_currentChannel]);
	}

	int16_t ClockAudio()
	{
		if(!_disableSound) {
			_updateCounter++;
			if(_updateCounter == 15) {
				UpdateChannel(_currentChannel);

				_updateCounter = 0;
				_currentChannel--;
				if(_currentChannel < 7 - GetNumberOfChannels()) {
					_currentChannel = 7;
				}
			}
		}
		auto out = UpdateOutputLevel();
		return out;
	}

	/// Compute how many clock cycles Namco163Audio can skip ahead in time
	/// without changing output.
	uint32_t ClockAudioMaxSkip() const
	{
		// off-by-one due to the counter incrementing upon clocking
		return 14 - _updateCounter;
	}

	void SkipClockAudio(uint32_t clocks)
	{
		_updateCounter += clocks;
	}

	Namco163Audio()
		: _channelOutput{}
		, _internalRam{}
	{
		_ramPosition = 0;
		_autoIncrement = false;
		_updateCounter = 0;
		_currentChannel = 7;
		_lastOutput = 0;
		_disableSound = false;
		_mixLinear = false;
	}

	void Reset() {
		*this = {};
	}

	uint8_t* GetInternalRam()
	{
		return _internalRam;
	}

	void WriteRegister(uint16_t addr, uint8_t value)
	{
		switch(addr & 0xF800) {
			case 0x4800:
				_internalRam[_ramPosition] = value;
				if(_autoIncrement) {
					_ramPosition = (_ramPosition + 1) & 0x7F;
				}
				break;
			case 0xE000:
				_disableSound = (value & 0x40) == 0x40;
				break;
			case 0xF800:
				_ramPosition = value & 0x7F;
				_autoIncrement = (value & 0x80) == 0x80;
				break;

		}
	}

	uint8_t ReadRegister(uint16_t addr)
	{
		uint8_t value = 0;
		switch(addr & 0xF800) {
			case 0x4800: {
				value = _internalRam[_ramPosition];
				if(_autoIncrement) {
					_ramPosition = (_ramPosition + 1) & 0x7F;
				}
				break;
			}
		}

		return value;
	}
};