#pragma once

// DIY min/max compiles faster than including all of <algorithm> for std::min/max
#include "utils/variadic_minmax.h"

/// Used for CSoundChip2::GetChannelLevel().
template<typename T>
class ChannelLevelState {
	T m_CurrLevel = 0;
	T m_MinLevel = 0;
	T m_MaxLevel = 0;

public:
	void update(T level)
	{
		m_CurrLevel = level;
		m_MinLevel = min(m_MinLevel, m_CurrLevel);
		m_MaxLevel = max(m_MaxLevel, m_CurrLevel);
	}

	T getLevel()
	{
		T out = m_MaxLevel - m_MinLevel;
		m_MinLevel = m_MaxLevel = m_CurrLevel;
		return out;
	}
};
