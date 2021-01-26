#pragma once
#include "stdafx.h"
#include <algorithm>
#include "APU.h"
#include "BaseFdsChannel.h"
#include "ModChannel.h"

class FdsAudio
{
private:
	static constexpr uint32_t WaveVolumeTable[4] = { 36, 24, 17, 14 };

	//Register values
	uint8_t _waveTable[64] = { 0 };
	bool _waveWriteEnabled = false;

	BaseFdsChannel _carrier;
	ModChannel _mod;

	bool _disableEnvelopes = false;
	bool _haltWaveform = false;

	uint8_t _masterVolume = 0;

	//Internal values
	uint16_t _waveOverflowCounter = 0;
	int32_t _wavePitch = 0;
	uint8_t _wavePosition = 0;

public:
	uint8_t ClockAudio()
	{
		// Unsigned 12 bits, bounded by [0..0xfff).
		int32_t frequency = _carrier.GetFrequency();
		if(!_haltWaveform && !_disableEnvelopes) {
			_carrier.TickEnvelope();
			if(_mod.TickEnvelope()) {
				_mod.UpdateOutput(frequency);
			}
		}

		if(_mod.TickModulator()) {
			//Modulator was ticked, update wave pitch
			_mod.UpdateOutput(frequency);
		}
	
		if(_haltWaveform) {
			_wavePosition = 0;
			return UpdateOutput();
		} else {
			auto out = UpdateOutput();

			// Unsure how many bits. Hopefully it's 16 bits or less.
			int32_t modFrequency = frequency + _mod.GetOutput();

			if(modFrequency > 0 && !_waveWriteEnabled) {
				// u16 wrapping addition.
				_waveOverflowCounter += modFrequency;

				// If output is less than both inputs, then addition overflowed.
				if(_waveOverflowCounter < modFrequency) {
					_wavePosition = (_wavePosition + 1) & 0x3F;
				}
			}

			return out;
		}
	}

private:
	uint8_t UpdateOutput()
	{
		uint32_t level = std::min((int)_carrier.GetGain(), 32) * WaveVolumeTable[_masterVolume];

		// `_waveTable[_wavePosition]` is bounded within [0..63].
		// `level` is bounded within [0..1152] because `_carrier.GetGain()` is clamped to ≤32
		// and `WaveVolumeTable[_masterVolume]` ≤ 36.
		// As a result, `(_waveTable[_wavePosition] * level) / 1152` is bounded within [0..63].
		uint8_t outputLevel = (_waveTable[_wavePosition] * level) / 1152;
		return outputLevel;
	}

public:
	void Reset() {
		*this = {};
	}

	uint8_t ReadRegister(uint16_t addr)
	{
		uint8_t value = 0;
		if(addr <= 0x407F) {
			value &= 0xC0;
			value |= _waveTable[addr & 0x3F];
		} else if(addr == 0x4090) {
			value &= 0xC0;
			value |= _carrier.GetGain();
		} else if(addr == 0x4092) {
			value &= 0xC0;
			value |= _mod.GetGain();
		}

		return value;
	}

	void WriteRegister(uint16_t addr, uint8_t value)
	{
		if(addr <= 0x407F) {
			if(_waveWriteEnabled) {
				_waveTable[addr & 0x3F] = value & 0x3F;
			}
		} else {
			switch(addr) {
				case 0x4080:
				case 0x4082:
					_carrier.WriteReg(addr, value);
					break;

				case 0x4083:
					_disableEnvelopes = (value & 0x40) == 0x40;
					_haltWaveform = (value & 0x80) == 0x80;
					if(_disableEnvelopes) {
						_carrier.ResetTimer();
						_mod.ResetTimer();
					}
					_carrier.WriteReg(addr, value);
					break;

				case 0x4084:
				case 0x4085:
				case 0x4086:
				case 0x4087:
					_mod.WriteReg(addr, value);
					break;

				case 0x4088:
					_mod.WriteModTable(value);
					break;

				case 0x4089:
					_masterVolume = value & 0x03;
					_waveWriteEnabled = (value & 0x80) == 0x80;
					break;

				case 0x408A:
					_carrier.SetMasterEnvelopeSpeed(value);
					_mod.SetMasterEnvelopeSpeed(value);
					break;
			}
		}
	}
};