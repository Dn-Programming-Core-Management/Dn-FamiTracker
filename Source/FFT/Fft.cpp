//------------------------------------
//  fft.cpp
//  The implementation of the 
//  Fast Fourier Transform algorithm
//  (c) Reliable Software, 1996
//------------------------------------
#include <memory>
#include "fft.h"
//#include "recorder.h"

// log (1) = 0, log(2) = 1, log(3) = 2, log(4) = 2 ...

#define PI (2.0 * asin(1.0))

// Points must be a power of 2

Fft::Fft (int Points, long sampleRate)
: m_Points (Points), m_sampleRate (sampleRate)
{
    m_aTape = new double [m_Points];
#if 0
    // 1 kHz calibration wave
    for (int i = 0; i < _Points; i++)
        _aTape[i] = 1600 * sin (2 * PI * 1000. * i / _sampleRate);
#else
    for (int i = 0; i < m_Points; i++)
        m_aTape[i] = 0;
#endif
    m_sqrtPoints = sqrt((double)m_Points);
    // calculate binary log
    m_logPoints = 0;
    Points--;
    while (Points != 0)
    {
        Points >>= 1;
        m_logPoints++;
    }

    m_aBitRev = new int [m_Points];
    m_X = new Complex[m_Points];
    m_W = new Complex* [m_logPoints+1];
    // Precompute complex exponentials
    int v_2_l = 2;
    for (int l = 1; l <= m_logPoints; l++)
    {
        m_W[l] = new Complex [m_Points];

        for ( int i = 0; i < m_Points; i++ )
        {
            double re =  cos (2. * PI * i / v_2_l);
            double im = -sin (2. * PI * i / v_2_l);
            m_W[l][i] = Complex (re, im);
        }
        v_2_l *= 2;
    }

    // set up bit reverse mapping
    int rev = 0;
    int halfPoints = m_Points/2;
    for (int i = 0; i < m_Points - 1; i++)
    {
        m_aBitRev[i] = rev;
        int mask = halfPoints;
        // add 1 backwards
        while (rev >= mask)
        {
            rev -= mask; // turn off this bit
            mask >>= 1;
        }
        rev += mask;
    }
    m_aBitRev [m_Points-1] = m_Points-1;
}

Fft::~Fft()
{
    delete []m_aTape;
    delete []m_aBitRev;
    for (int l = 1; l <= m_logPoints; l++)
    {
        delete []m_W[l];
    }
    delete []m_W;
    delete []m_X;
}

void Fft::CopyIn (/*SampleIter& iter*/ int SampleCount, short *Samples)
{
//    int cSample = iter.Count();
	int cSample = SampleCount;
    if (cSample > m_Points)
        return;

    // make space for cSample samples at the end of tape
    // shifting previous samples towards the beginning
    memmove (m_aTape, &m_aTape[cSample], 
              (m_Points - cSample) * sizeof(double));
    // copy samples from iterator to tail end of tape
    int iTail  = m_Points - cSample;
    for (int i = 0; i < cSample; i++/*, iter.Advance()*/)
    {
        m_aTape [i + iTail] = (double) /*iter.GetSample()*/ Samples[i];
    }
    // Initialize the FFT buffer
    for (int i = 0; i < m_Points; i++)
        PutAt (i, m_aTape[i]);
}

//
//               0   1   2   3   4   5   6   7
//  level   1
//  step    1                                     0
//  increm  2                                   W 
//  j = 0        <--->   <--->   <--->   <--->   1
//  level   2
//  step    2
//  increm  4                                     0
//  j = 0        <------->       <------->      W      1
//  j = 1            <------->       <------->   2   W
//  level   3                                         2
//  step    4
//  increm  8                                     0
//  j = 0        <--------------->              W      1
//  j = 1            <--------------->           3   W      2
//  j = 2                <--------------->            3   W      3
//  j = 3                    <--------------->             3   W
//                                                              3
//

void Fft::Transform ()
{
    // step = 2 ^ (level-1)
    // increm = 2 ^ level;
    int step = 1;
    for (int level = 1; level <= m_logPoints; level++)
    {
        int increm = step * 2;
        for (int j = 0; j < step; j++)
        {
            // U = exp ( - 2 PI j / 2 ^ level )
            Complex U = m_W [level][j];
            for (int i = j; i < m_Points; i += increm)
            {
                // butterfly
                Complex T = U;
                T *= m_X [i+step];
                m_X [i+step] = m_X[i];
                m_X [i+step] -= T;
                m_X [i] += T;
            }
        }
        step *= 2;
    }
}

