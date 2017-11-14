/*
** FamiTracker - NES/Famicom sound tracker
** Copyright (C) 2005-2014  Jonathan Liss
**
** 0CC-FamiTracker is (C) 2014-2017 HertzDevil
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

#include "FftComplex.hpp"
#include <algorithm>

namespace details {

template <typename T, std::size_t N>
constexpr std::array<T, N> make_hann_window() {
	std::array<T, N> window = { };
	T fraction = FFT::details::PI / (N - 1);
	for (int i = 0; i < N; ++i)
		window[i] = [] (T x) { return x * x; }(FFT::details::remez_sin(-i * fraction));
	return window;
}

} // namespace details

template <std::size_t N>
class FftBuffer {
public:
	void Reset() {
		samples_.fill({ });
		buffer_.fill({ });
	}

	void Transform() {
		FFT::transform_fwd(samples_, buffer_, window.cbegin());
	}

	template <typename InputIt>
	void CopyIn(InputIt Samples, std::size_t SampleCount) {
		if (SampleCount > std::size(samples_))
			return;
		std::copy(samples_.cbegin() + SampleCount, samples_.cend(), samples_.begin());
		std::transform(Samples, Samples + SampleCount, samples_.begin(), [] (auto x) {
			return std::complex<double>(x, 0);
		});
	}

	double GetIntensity(int i) const {
		const double sqrtpoints = 1 << (FFT::details::floor_log2(N) / 2);
		return std::abs(buffer_[i]) / sqrtpoints;
	}

private:
	std::array<std::complex<double>, N> samples_;
	std::array<std::complex<double>, N> buffer_;
	static constexpr auto window = details::make_hann_window<double, N>();
};
