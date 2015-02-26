#if !defined FFT_H
#define FFT_H
//------------------------------------
//  fft.h
//  Fast Fourier Transform
//  (c) Reliable Software, 1996
//------------------------------------

#include "complex.h"
//#include "assert.h"

class SampleIter;

class Fft
{
public:
    Fft (int Points, long sampleRate);
    ~Fft ();
    int     Points () const { return m_Points; }
    void    Transform ();
    void    CopyIn (/*SampleIter& iter*/int SampleCount, short *Samples);

    double  GetIntensity (int i) const
    { 
  //      Assert (i < _Points);
        return m_X[i].Mod()/m_sqrtPoints; 
    }

    int     GetFrequency (int point) const
    {
        // return frequency in Hz of a given point
//        Assert (point < _Points);
        long x =m_sampleRate * point;
        return x / m_Points;
    }

    int     HzToPoint (int freq) const 
    { 
        return (long)m_Points * freq / m_sampleRate; 
    }

    int     MaxFreq() const { return m_sampleRate; }

    int     Tape (int i) const
    {
//        Assert (i < _Points);
        return (int) m_aTape[i];
    }

private:

    void PutAt ( int i, double val )
    {
        m_X [m_aBitRev[i]] = Complex (val);
    }

    int			m_Points;
    long		m_sampleRate;
    int			m_logPoints;
    double		m_sqrtPoints;
    int		   *m_aBitRev;       // bit reverse vector
    Complex	   *m_X;             // in-place fft array
    Complex	  **m_W;             // exponentials
    double     *m_aTape;         // recording tape
};

#endif
