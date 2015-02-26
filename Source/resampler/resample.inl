/**This program is free software. It comes without any warranty, to
 **the extent permitted by applicable law. You can redistribute it
 **and/or modify it under the terms of the Do What The Fuck You Want
 **To Public License, Version 2, as published by Sam Hocevar. See
 **http://sam.zoy.org/wtfpl/COPYING for more details. **/

/**Author: Jarhmander **/

/**Description:
    implementation of template resample methods
**/

#ifndef RESAMPLE_INL
#define RESAMPLE_INL
//------------------------------------------------------------------------
/*
#include <numeric>
#include <algorithm>
#include <cmath>
*/
//------------------------------------------------------------------------



namespace jarh
{



//------------------------------------------------------------------------
// init(float theratio, float thecutoff) -
//
//------------------------------------------------------------------------
template <typename Base>
 void resample<Base>::init(float theratio, float thecutoff)
{
    clear();
    if (theratio != ratio() || thecutoff != cutoff())
    {
        cutoff(thecutoff);
        ratio(theratio);

        remainsamples_ = std::numeric_limits<float>::infinity();
        notend_ = true;
        idx_ = 0;
        const float range = -sinc_.range() / sincstep_;
        subidx_ = range - std::floor(range);
    }
    bool res = static_cast<Base *>(this)->initstream();
    if (res)
    {
        const size_t offset = (size_t)std::ceil((float)(buf_.size()/2)) - 1;
        float *start = &*buf_.begin() + offset;

        std::fill(&*buf_.begin(), start, 0.f);
//        fillcheck(&* buf_.begin(), start, &*buf_.end());
        fillcheck(	&buf_[0], start, &buf_[0] + buf_.size());


    }
    else
    {
        // problem!
        setstate(badbit);
    }
}
//------------------------------------------------------------------------
// get(OutputIterator begin, OutputIterator end) -
//
//------------------------------------------------------------------------
template <typename Base>
 template <typename OutputIterator>
 OutputIterator resample<Base>::get(OutputIterator begin,
                                    OutputIterator end)
{
    for (;
          begin != end && this->get(*begin);
        ++begin)
    {}
    return begin;
}
//------------------------------------------------------------------------

//------------------------------------------------------------------------
template <typename Base>
 void resample<Base>::update_buffer()
{
    const size_t offset = updateidx();
    float *start = &* buf_.begin();
    if (offset)
    {
        if (offset < buf_.size())
            start = &*std::copy(buf_.begin()+offset, buf_.end(), buf_.begin());



		fillcheck(&buf_[0], start, &buf_[0] + buf_.size());
    }

}
//------------------------------------------------------------------------
// fillcheck(float *first, float *mid, float *last) -
//
//------------------------------------------------------------------------
template <typename Base>
 void resample<Base>::fillcheck(float *first, float *mid, float *last)
{
    if (notend_)
    {

        float *const res= static_cast<Base *>(this)->fill(mid, last);

        if (!res)
        {
            setstate(badbit);
            notend_ = false;
        }
        // Non-NULL pointer.
        else if (res != last)
        {
            // stream will end soon or later
            notend_ = false;

            // copy not complete, EOF etc. encountered.
            // first fill remaining by zeros...
            std::fill(res, last, 0.f);
            // ... then evaluate the number of samples remaining
            // from mid-buffer to last acquired sample
            remainsamples_ = float(res + 1 -
                        (first + ((int)(std::distance(first,last))/2)));

            if (remainsamples_ < 0)
            {
                setstate(failbit | eofbit);
            }
        }
        //else ok!
    }
    else
    {
        std::fill(mid, last, 0.f);
    }
}
//------------------------------------------------------------------------



} // namespace jarh



//------------------------------------------------------------------------
#endif // RESAMPLE_INL
