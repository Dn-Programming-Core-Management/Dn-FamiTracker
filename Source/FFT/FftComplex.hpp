/* 
 * Free FFT and convolution (C++)
 * 
 * Copyright (c) 2017 Project Nayuki. (MIT License)
 * https://www.nayuki.io/page/free-small-fft-in-multiple-languages
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 * - The above copyright notice and this permission notice shall be included in
 *   all copies or substantial portions of the Software.
 * - The Software is provided "as is", without warranty of any kind, express or
 *   implied, including but not limited to the warranties of merchantability,
 *   fitness for a particular purpose and noninfringement. In no event shall the
 *   authors or copyright holders be liable for any claim, damages or other
 *   liability, whether in an action of contract, tort or otherwise, arising from,
 *   out of or in connection with the Software or the use or other dealings in the
 *   Software.
 */


#pragma once

#include <complex>
#include <array>		// // //

// // // only the main algorithm in Fft::transformRadix2 remains
// // // almost everything else is redone to improve performance

namespace FFT {

namespace details {

// lolremez --double -d 13 -r "-pi:0" "cos(x)"
constexpr double remez_cos(double x) {
	double u = 1.5365548664876748e-10;
	u = u * x + 3.1376911390031669e-9;
	u = u * x + 4.5453499571450849e-9;
	u = u * x + -2.6210698302209483e-7;
	u = u * x + 2.8246867130932693e-8;
	u = u * x + 2.4844063461110785e-5;
	u = u * x + 4.569044107298047e-8;
	u = u * x + -1.3888542639595254e-3;
	u = u * x + 1.7937254541327703e-8;
	u = u * x + 4.1666672703855374e-2;
	u = u * x + 1.2114885190547519e-9;
	u = u * x + -4.9999999987659928e-1;
	u = u * x + 4.2050066293474362e-12;
	return u * x + 9.9999999999996078e-1;
}

// lolremez --double -d 13 -r "-pi:0" "sin(x)"
constexpr double remez_sin(double x) {
    double u = -4.3650632368660362e-140;
    u = u * x + -1.9907856852657762e-9;
    u = u * x + -3.7525426102216109e-8;
    u = u * x + -4.8949444257135462e-8;
    u = u * x + 2.6260807748916021e-6;
    u = u * x + -2.3895018459923412e-7;
    u = u * x + -1.9872150093904687e-4;
    u = u * x + -2.7733943536051726e-7;
    u = u * x + 8.3331645621466347e-3;
    u = u * x + -6.6459142501439606e-8;
    u = u * x + -1.6666668233124603e-1;
    u = u * x + -1.9257056775099166e-9;
    u = u * x + 9.9999999990678278e-1;
    return u * x + -7.481698606943585e-13;
}

constexpr std::size_t reverseBits(std::size_t x, int n) {
	std::size_t result = 0;
	for (int i = 0; i < n; i++, x >>= 1)
		result = (result << 1) | (x & 1U);
	return result;
}

constexpr std::size_t floor_log2(std::size_t x) {
	std::size_t levels = 0;  // Compute levels = floor(log2(n))
	while (x > 1U) {
		x >>= 1;
		++levels;
	}
	return levels;
}

constexpr double PI = 3.14159265358979323846; // M_PI;

template <typename T, std::size_t N>
constexpr std::array<T, N / 2> make_exp_table() {
	std::array<T, N / 2> exp_table = { };
	for (size_t i = 0; i < N / 2; ++i) {
		typename T::value_type angle = -2 * PI * i / N;
		exp_table[i] = T {remez_cos(angle), remez_sin(angle)};
	}
	return exp_table;
}

template <typename T, std::size_t Levels>
class Radix2Transformer {
	using element_type = std::complex<T>;

	static constexpr auto Points = static_cast<std::size_t>(1) << Levels;
	static constexpr auto exp_table = details::make_exp_table<element_type, Points>();

public:
	template <typename InputIt, typename RandomIt, typename InputIt2>
	void operator()(InputIt first, RandomIt d_first, InputIt2 window) {
		// Bit-reversed addressing permutation
		for (std::size_t i = 0; i < Points; ++i)
			d_first[details::reverseBits(i, Levels)] = (*first++) * (*window++);
	
		// Cooley-Tukey decimation-in-time radix-2 FFT
		std::size_t size = 2;
		while (true) {
			std::size_t halfsize = size / 2;
			std::size_t tablestep = Points / size;
			for (std::size_t i = 0; i < Points; i += size) {
				for (std::size_t j = i, k = 0; j < i + halfsize; j++, k += tablestep) {
					element_type temp = d_first[j + halfsize] * exp_table[k];
					d_first[j + halfsize] = d_first[j] - temp;
					d_first[j] += temp;
				}
			}
			if (size == Points)  // Prevent overflow in 'size *= 2'
				break;
			size <<= 1;
		}
	}
};

} // namespace details

/* 
 * Computes the discrete Fourier transform (DFT) of the given complex vector, storing the result into `out`.
 * The vector's length must be a power of 2. Uses the Cooley-Tukey decimation-in-time radix-2 algorithm.
 */
template <typename T, std::size_t N, typename InputIt>
void transform_fwd(const std::array<std::complex<T>, N> &arr, std::array<std::complex<T>, N> &out, InputIt window) {
	if constexpr (N && !(N & (N - 1)))
		details::Radix2Transformer<T, details::floor_log2(N)>()(arr.cbegin(), out.begin(), window);
}

} // namespace FFT
