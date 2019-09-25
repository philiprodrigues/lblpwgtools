#pragma once

namespace ana
{
  namespace PredIntKern
  {

    struct Coeffs{
      Coeffs(double _a, double _b, double _c, double _d)
        : a(_a), b(_b), c(_c), d(_d) {}
      double a, b, c, d;
    };

    inline void ShiftSpectrumKernel(const Coeffs* fits,
                             unsigned int N,
                             double x, double x2, double x3,
                             double* corr)
    {
      for(unsigned int n = 0; n < N; ++n){
        const Coeffs& f = fits[n];
        corr[n] *= f.a*x3 + f.b*x2 + f.c*x + f.d;
      } // end for n
    }
  }
}
