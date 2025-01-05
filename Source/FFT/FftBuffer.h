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
	static constexpr std::size_t GetPoints() noexcept {
		return N;
	}

	void Reset() {
		samples_.fill({ });
		buffer_.fill({ });
	}

	void Transform() {
		FFT::transform_fwd(samples_, buffer_, window.cbegin());
	}

	template <typename InputIt>
	void CopyIn(InputIt Samples, std::size_t SampleCount) {
		if (SampleCount > N) {
			std::advance(Samples, SampleCount - N);
			SampleCount = N;
		}
		std::copy(samples_.cbegin() + SampleCount, samples_.cend(), samples_.begin());
		std::transform(Samples, Samples + SampleCount, samples_.end() - SampleCount, [] (auto x) {
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
