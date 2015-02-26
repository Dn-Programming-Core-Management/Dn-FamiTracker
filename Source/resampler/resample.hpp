/**This program is free software. It comes without any warranty, to
 **the extent permitted by applicable law. You can redistribute it
 **and/or modify it under the terms of the Do What The Fuck You Want
 **To Public License, Version 2, as published by Sam Hocevar. See
 **http://sam.zoy.org/wtfpl/COPYING for more details. **/

/**Author: Jarhmander **/

/**Description:
    A resampler that implement a polyphase FIR filter. It uses a
    sinc object as its impulse response, and as such the quality
    of the resampling is related to the parameters given to the
    resize method of the sinc object used.

    For better quality (at the expense of computational time),
    'sz' should be big, 'firstnull' should be somewhat big and
    the ratio sz/firstnull should yield a big value.

    Cutoff control is provided to help reducing aliasing, but
    must be used with caution, because with low cutoff values
    the memory usage will be ridiculously high.

    'resample' class implement the resampler; to use it, define
    a new class and use the CRTP idiom like this:

        class myclass : public resampler<myclass> {...}

    ... and then implement the methods 'initstream' and 'fill'
    in your newly defined class (as if they were virtual methods).
**/

#ifndef RESAMPLE_HPP
#define RESAMPLE_HPP
//------------------------------------------------------------------------
#include "sinc.hpp"
//------------------------------------------------------------------------
#include <limits>
#include <iterator>
#include <vector>
//------------------------------------------------------------------------

//------------------------------------------------------------------------



namespace jarh
{



// `forward declarations'
template <typename Base>
 class resample;

class resample_base
{
    template <typename Base> friend class resample;
public:
    resample_base(const sinc &s);
    ~resample_base() {}

    typedef int iostate;
    enum
    {
        goodbit = 0,
        failbit = 1,
        badbit  = (failbit << 1),
        eofbit  = (badbit << 1),
    };
public:
    // ratio
    void    ratio(float theratio);
protected:
    float   ratio() const { return ratio_; }


public:
    // cutoff
    float   cutoff() const { return cutoff_; }
protected:
    void    cutoff(float cutoff);

public:
    // state query (a blatant reproduction of part of ios functionnality)
    iostate rdstate() const { return flags_; }
    bool    fail() const { return (flags_ & (failbit | badbit)) != 0; }
    bool    bad()  const { return (flags_ & badbit) != 0; }
    bool    eof()  const { return (flags_ & badbit) != 0; }
    bool    good() const { return (*this) != 0; }
    bool operator!() const { return fail(); }
    operator const void *() const { return fail() ? 0 : this; }

    // state control
    void clear(iostate b= goodbit)
    {
        flags_ = b;
    }
    void setstate(iostate b)
    {
        clear(rdstate() | b );
    }

protected:
    float conv() const;
private:
    iostate flags_;
    const sinc &sinc_;
    std::vector<float> buf_;
    float cutoff_;
    float ratio_;
    float invratio_;
    float sincstep_;

    size_t idx_;
    float  subidx_;
    float  remainsamples_;
    bool   notend_;
private:
    size_t updateidx();
};
//------------------------------------------------------------------------
template <typename Base>
 class resample : public resample_base
{
public:
    resample(const sinc &s)
    : resample_base(s)
    {}

    void  init(float ratio, float thecutoff);

    void  reset() { init(ratio(), cutoff()); }
    void  init()  { reset(); }

    float get()
    {
        const float v = conv() * sincstep_;
        update_buffer();
        return v;
    }

    template <typename T>
     resample &get(T &v)
    {
        v = get();
        return *this;
    }

    template <typename OutputIterator>
     OutputIterator get(OutputIterator begin, OutputIterator end);

protected:
    // should be implemented in derived classes
    bool        initstream() { return true; }
    float       *fill(float *begin, float *end);
private:
    void update_buffer();
    void fillcheck(float *first, float *mid, float *last);
};
//------------------------------------------------------------------------



} // namespace jarh



//------------------------------------------------------------------------
#endif
