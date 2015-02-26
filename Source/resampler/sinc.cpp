/**This program is free software. It comes without any warranty, to
 **the extent permitted by applicable law. You can redistribute it
 **and/or modify it under the terms of the Do What The Fuck You Want
 **To Public License, Version 2, as published by Sam Hocevar. See
 **http://sam.zoy.org/wtfpl/COPYING for more details. **/
//------------------------------------------------------------------------
#include "sinc.hpp"
//------------------------------------------------------------------------
#include <limits>
#include <cmath>
//------------------------------------------------------------------------

/** Implementation of the sinc class **/



/**Anonymous namespace, internal linking**/
namespace {

// Well, this constant is quite ubiquitous...
const float PI = 3.141592653589793238462643383279502884197169f;

const float eps = std::numeric_limits<float>::epsilon();

inline float fetchval(const std::vector<float> &v, size_t elem)
{
    return elem >= v.size() ? 0.f : v[elem];
}

} // anonymous namespace



namespace jarh
{



//------------------------------------------------------------------------
// ctor -
//------------------------------------------------------------------------
sinc::sinc(size_t sz, size_t firstnull, float gain)
{
    resize(sz, firstnull, gain);
}
//------------------------------------------------------------------------
// resize(size_t sz, size_t firstnull, float gain) -
//------------------------------------------------------------------------
void
sinc::resize(size_t sz, size_t firstnull, float gain)
{
    using std::sin;
    using std::cos;

    // free and minimize vector.
    std::vector<float>().swap(tbl_);
    tbl_.reserve(sz);

    // sanitize input
    sz        = std::max<size_t>(1,  sz);
    firstnull = std::min<size_t>(sz, firstnull);

    convfactor_ = (float)firstnull;

    size_t i;
    double sum = 0.;
    for (i = 0; i < sz; i++)
    {
        /*Evaluate Upper half of sinc(x) * Hamming window */
        const float f = eps + (PI*i) / firstnull;
        const float v = sin(f) / f * (.54f + .46f * cos(i*PI / sz));
        tbl_.push_back(v);
        sum += v;
    }

    const float adjust = gain * firstnull / (float)(2*sum - tbl_[0]);

    // adjust the 'gain' of the sinc
    for (i = 0; i < sz; i++)
    {
        tbl_[i] *= adjust;
    }
}
//------------------------------------------------------------------------
// operator()(float) -
//  evaluate sinc at x, using our table and linear interpolation.
//------------------------------------------------------------------------
float
sinc::operator()(float x) const
{
    using std::abs;
    using std::floor;

    x = abs(x*convfactor_);

    const size_t ix1 = (size_t)floor(x), ix2 = ix1 + 1;
    const float v1 = fetchval(tbl_,ix1), v2 = fetchval(tbl_,ix2);

    return v1 + (x - ix1)*(v2-v1);
}
//------------------------------------------------------------------------



} // namespace jarh



//------------------------------------------------------------------------
