#pragma once

namespace ana
{
  namespace PredIntKern
  {

    struct Coeffs{
      Coeffs(float _a, float _b, float _c, float _d)
        : a(_a), b(_b), c(_c), d(_d) {}
      alignas(64) float a;
      float b, c, d;
    };

    inline void ShiftSpectrumKernel(const Coeffs* __restrict__ fits,
                                    unsigned int N,
                                    double x, double x2, double x3,
                                    double* __restrict__ corr) __attribute__((always_inline));

    inline void ShiftSpectrumKernel(const Coeffs* __restrict__ fits,
                                    unsigned int N,
                                    double x, double x2, double x3,
                                    double* __restrict__  corr)
    {
      for(unsigned int n = 0; n < N; ++n){
        const Coeffs& f = fits[n];
        corr[n] *= f.a*x3 + f.b*x2 + f.c*x + f.d;
      } // end for n
    }
  }
}
