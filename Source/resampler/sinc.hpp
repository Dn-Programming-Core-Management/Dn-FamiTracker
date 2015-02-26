/**This program is free software. It comes without any warranty, to
 **the extent permitted by applicable law. You can redistribute it
 **and/or modify it under the terms of the Do What The Fuck You Want
 **To Public License, Version 2, as published by Sam Hocevar. See
 **http://sam.zoy.org/wtfpl/COPYING for more details. **/

/**Author: Jarhmander **/

/**Description:
    This class has the sole purpose of calculating a point of a
    normalised sinc [ sin(pi * x) / (pi * x) ] using a combination of
    look-up table and linear interpolation. The table itself contains
    only half of the sinc values, as it uses the symetric property of
    the sinc function.
    Its domain is limited and the step size (resolution) of the sinc
    is configurable.
    A Hamming window is applied to the sinc to minimise spectral
    leakage and gain control is provided so when used as an impulse
    response of a filter, the DC gain can be controlled.

    Mainly intended for signal processing purposes.
**/

#ifndef SINC_HPP
#define SINC_HPP

//------------------------------------------------------------------------
#include <vector>
#include <cstddef>
//------------------------------------------------------------------------
#include <xutility>

namespace jarh
{



using std::size_t;



class sinc
{
public:
    /** ctor:
      *   calls resize.
      */
    sinc(size_t sz, size_t firstnull, float gain=1.f);

    /** resize:
      *   resize internal table and recompute values in it
      *     sz        = size of the vector
      *     firstnull = steps before the first null of sinc.
      *     gain      = the integral of the sinc will be this value.
      *
      */
    void resize(size_t sz, size_t firstnull, float gain=1.f);

    /** operator():
      *   'compute' the value sinc(x).
      *     x = evaluate sinc at this value
      */
    float operator()(float x) const;

    /** range:
      *   upper range of sinc. Anything outside [-range..range] yields 0.
      */
    float range() const { return tbl_.size() / convfactor_; };

private:
    std::vector<float> tbl_;
    float convfactor_;

};
//------------------------------------------------------------------------



} // namespace jarh



//------------------------------------------------------------------------
#endif
