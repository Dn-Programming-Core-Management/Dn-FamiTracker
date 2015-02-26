#if !defined COMPLEX_H
#define COMPLEX_H
//------------------------------------
//  complex.h
//  Complex number
//  (c) Reliable Software, 1996
//------------------------------------

#include <math.h>

class Complex
{
public:
    Complex () {}
    Complex (double re): _re(re), _im(0.0) {}
    Complex (double re, double im): _re(re), _im(im) {}
    double Re () const { return _re; }
    double Im () const { return _im; }
    void operator += (const Complex& c)
    {
        _re += c._re;
        _im += c._im;
    }
    void operator -= (const Complex& c)
    {
        _re -= c._re;
        _im -= c._im;
    }
    void operator *= (const Complex& c)
    {
        double reT = c._re * _re - c._im * _im;
        _im = c._re * _im + c._im * _re;
        _re = reT;
    }
    Complex operator- () 
    {
            return Complex (-_re, -_im);
    }
    double Mod () const { return sqrt (_re * _re + _im * _im); }
private:
    double _re;
    double _im;
};

#endif