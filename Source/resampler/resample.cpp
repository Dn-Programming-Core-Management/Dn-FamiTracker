/**This program is free software. It comes without any warranty, to
 **the extent permitted by applicable law. You can redistribute it
 **and/or modify it under the terms of the Do What The Fuck You Want
 **To Public License, Version 2, as published by Sam Hocevar. See
 **http://sam.zoy.org/wtfpl/COPYING for more details. **/
//------------------------------------------------------------------------

// Get rid of visual studio specific warnings
#define _SCL_SECURE_NO_WARNINGS

#include "resample.hpp"
//------------------------------------------------------------------------
#include <limits>
#include <cmath>
#include <numeric>
#include <algorithm>
//------------------------------------------------------------------------

/** Below is for debug purpose only; normally commented-out**/
/*

#define inner_product inner_product2_2eA6jpo51vPqr9ww

namespace std {
template <typename T, typename InIter1, typename InIter2>
 T inner_product2_2eA6jpo51vPqr9ww(InIter1 first1, InIter1 last1, InIter2 first2, T init)
{
    for (;first1 != last1; ++first1, ++first2)
    {
        T temp;
        T t1 = *first1;
        T t2 = *first2;
        temp = t1 * t2;
        init += temp;
    }
    return init;
}
}
//*/

/** small utility class to change a function object into an iterator **/

template <typename func>
 class func_iterator : public std::iterator<std::forward_iterator_tag, float>
{
    func f_;
    float x_;
    float step_;
public:
    func_iterator(func f, float initval, float step=1.f)
     : f_(f), x_(initval), step_(step) {}

    func_iterator &operator++() { x_ += step_; return *this; }
    float operator*() { return f_(x_); }
};

/** small utility function to create a func_iterator **/

template <typename func>
 func_iterator<func> make_func_iterator(func f, float init, float step)
{
    return func_iterator<func>(f, init, step);
}

//------------------------------------------------------------------------

//------------------------------------------------------------------------



/** Implementation of the interp_base class **/



namespace jarh
{



//------------------------------------------------------------------------
// resample_base(const sinc &s) -
//
//------------------------------------------------------------------------
resample_base::resample_base(const sinc &s)
 : flags_(goodbit), sinc_(s)
{
}
//------------------------------------------------------------------------
// cutoff(float thecutoff) -
//
//------------------------------------------------------------------------
void resample_base::cutoff(float thecutoff)
{
    // limit cutoff_ in the range ]0-1]
    cutoff_ = (std::min)(
                       1.f,
                       std::max(
                                std::numeric_limits<float>::epsilon(),
                                thecutoff
                                )
                       );
}
//------------------------------------------------------------------------
// ratio(float theratio) -
//
//------------------------------------------------------------------------
void resample_base::ratio(float theratio)
{
    // ratio_ can't be 0 or below.
    ratio_   = (std::max)(std::numeric_limits<float>::epsilon(), theratio);
    invratio_= 1.f/ratio_;

    sincstep_= (std::min)(1.f, ratio_) * cutoff_;

    buf_.resize(1 +
                static_cast<size_t>(std::floor(
                          2*sinc_.range() / sincstep_
                          ))
            );
}
//------------------------------------------------------------------------

//------------------------------------------------------------------------
float resample_base::conv() const
{
    return std::inner_product(buf_.begin(),
                              buf_.end(),
                              make_func_iterator(sinc_,
                                                -sinc_.range() + sincstep_ - subidx_ * sincstep_,
                                                 sincstep_
                                            ),
                                0.f);
}
//------------------------------------------------------------------------

//------------------------------------------------------------------------
size_t resample_base::updateidx()
{
    subidx_         += invratio_;
    const size_t inc = static_cast<size_t>(subidx_);
    subidx_         -= inc;

    remainsamples_ -= invratio_;
    if (remainsamples_ < 0)
    {
        setstate(eofbit | failbit);
    }
    return inc;
}
//------------------------------------------------------------------------



} // namespace jarh



//------------------------------------------------------------------------
