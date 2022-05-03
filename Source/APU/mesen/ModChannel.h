#pragma once
#include "stdafx.h"
#include "BaseFdsChannel.h"
#include <assert.h>
#include <algorithm>  // std::min

class ModChannel : public BaseFdsChannel
{
private:
	static constexpr int32_t ModReset = 0xFF;
	static constexpr int32_t _modLut[8] = { 0,1,2,4,ModReset,-4,-2,-1 };

	int8_t _counter = 0;
	bool _modulationDisabled = false;

	uint8_t _modTable[64] = { 0 };
	uint8_t _modTablePosition = 0;
	uint16_t _overflowCounter = 0;
	int32_t _output = 0;

public:
	virtual void WriteReg(uint16_t addr, uint8_t value) override
	{
		switch(addr) {
			case 0x4084:
			case 0x4086:
				BaseFdsChannel::WriteReg(addr, value);
				break;
			case 0x4085:
				UpdateCounter(value & 0x7F);
				break;
			case 0x4087:
				BaseFdsChannel::WriteReg(addr, value);
				_modulationDisabled = (value & 0x80) == 0x80;
				if(_modulationDisabled) {
					_overflowCounter = 0;
				}
				break;
		}
	}

	void WriteModTable(uint8_t value)
	{
		//"This register has no effect unless the mod unit is disabled via the high bit of $4087."
		if(_modulationDisabled) {
			_modTable[_modTablePosition & 0x3F] = value & 0x07;
			_modTable[(_modTablePosition + 1) & 0x3F] = value & 0x07;
			_modTablePosition = (_modTablePosition + 2) & 0x3F;
		}
	}

	void UpdateCounter(int8_t value)
	{
		_counter = value;
		if(_counter >= 64) {
			_counter -= 128;
		} else if(_counter < -64) {
			_counter += 128;
		}
	}

	bool IsEnabled() const
	{
		return !_modulationDisabled && _frequency > 0;
	}

	bool TickModulator()
	{
		if(IsEnabled()) {
			_overflowCounter += _frequency;

			if(_overflowCounter < _frequency) {
				//Overflowed, tick the modulator
				int32_t offset = _modLut[_modTable[_modTablePosition]];
				UpdateCounter(offset == ModReset ? 0 : _counter + offset);

				_modTablePosition = (_modTablePosition + 1) & 0x3F;

				return true;
			}
		}
		return false;
	}

	/// Compute how many calls to TickModulator() can be skipped without changing output.
	uint32_t TickModulatorMaxSkip() const
	{
		// Copied from TickModulator().
		if(IsEnabled()) {  // if true, then _frequency > 0.
			uint32_t phaseWithoutOverflow = UINT16_MAX - _overflowCounter;
			uint32_t clocksWithoutOverflow = phaseWithoutOverflow / _frequency;
			return clocksWithoutOverflow;
		}

		// If chip is in an idle state, return a very large number as a placeholder.
		return 1 << 24;
	}

	/// Simulates the effects of multiple calls to TickModulator().
	/// The modulator level must not change in the process.
	void SkipTickModulator(uint32_t clocks)
	{
		if(IsEnabled()) {
			uint32_t newOverflowCounter = _overflowCounter + _frequency * clocks;
			_overflowCounter = newOverflowCounter;

			// Ensure that cycle-skipping does not overflow the counter and trigger a step.
			assert(_overflowCounter == newOverflowCounter);
		}
	}

	void UpdateOutput(uint16_t volumePitch)
	{
		//Code from NesDev Wiki

		// pitch   = $4082/4083 (12-bit unsigned pitch value)
		// counter = $4085 (7-bit signed mod counter)
		// gain    = $4084 (6-bit unsigned mod gain)

		// 1. multiply counter by gain, lose lowest 4 bits of result but "round" in a strange way
		int32_t temp = _counter * _gain;
		int32_t remainder = temp & 0xF;
		temp >>= 4;
		if(remainder > 0 && (temp & 0x80) == 0) {
			temp += _counter < 0 ? -1 : 2;
		}

		// 2. wrap if a certain range is exceeded
		if(temp >= 192) {
			temp -= 256;
		} else if(temp < -64) {
			temp += 256;
		}

		// 3. multiply result by pitch, then round to nearest while dropping 6 bits
		temp = volumePitch * temp;
		remainder = temp & 0x3F;
		temp >>= 6;
		if(remainder >= 32) {
			temp += 1;
		}

		// final mod result is in temp
		_output = temp;
	}

	int32_t GetOutput() const
	{
		return IsEnabled() ? _output : 0;
	}

	int8_t GetCounter() const
	{
		return _counter;
	}
};
