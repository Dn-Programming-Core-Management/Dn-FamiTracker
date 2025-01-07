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

#include "FamiTrackerTypes.h"
#include "APU/Types.h"
#include <array>

constexpr auto Effects(){
	std::array<Effect, EF_COUNT> effects;
	for (size_t i = 0; i < EF_COUNT; i++) {
		int initial = 0;
		int uiDefault = 0;

		if (i == EF_PITCH || i == EF_FDS_MOD_BIAS) {
			initial = uiDefault = 0x80;
		} else if (i == EF_N163_WAVE_BUFFER) {
			initial = 0x7F;
		} else if (i == EF_HARMONIC) {
			initial = uiDefault = 1;
		}
		effects[i] = { EFF_CHAR[i], initial, uiDefault };
	}
	return effects;
}
const std::array<Effect, EF_COUNT> effects = Effects();

// TODO: Define std::unordered_map<char, effect_t> for all effects, plus each expansion.
// Faster, but produces duplicate global consts.
effect_t GetEffectFromChar(char ch, int Chip, bool *bValid)		// // //
{
	bool dummy;
	if (bValid == nullptr) bValid = &dummy;

	*bValid = true;

	switch (Chip) {
	case SNDCHIP_FDS:
		for (const auto &x : FDS_EFFECTS)
			if (ch == EFF_CHAR[x])
				return x;
		break;
	case SNDCHIP_N163:
		for (const auto &x : N163_EFFECTS)
			if (ch == EFF_CHAR[x])
				return x;
		break;
	case SNDCHIP_S5B:
		for (const auto &x : S5B_EFFECTS)
			if (ch == EFF_CHAR[x])
				return x;
		break;
	case SNDCHIP_VRC7:
		for (const auto &x : VRC7_EFFECTS)
			if (ch == EFF_CHAR[x])
				return x;
		break;
	}

	for (effect_t eff = EF_NONE; eff != EF_COUNT; eff = static_cast<effect_t>(eff + 1)) {
		if (EFF_CHAR[eff] == ch) {
			return eff;
		}
	}

	*bValid = false;
	return EF_NONE;
}
