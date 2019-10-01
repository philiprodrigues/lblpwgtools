#pragma once

#include <immintrin.h>

namespace ana
{
  namespace PredIntKern
  {

    struct Coeffs{
      Coeffs(double _a, double _b, double _c, double _d)
        : a(_a), b(_b), c(_c), d(_d) {}
      double a, b, c, d;
    };

    struct alignas(64) CoeffsAVX2{
      CoeffsAVX2(__m256d _a, __m256d _b, __m256d _c, __m256d _d)
        : a(_a), b(_b), c(_c), d(_d) {}
      __m256d a, b, c, d;
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

    inline void ShiftSpectrumKernelAVX2(const CoeffsAVX2* __restrict__ fits,
                                        unsigned int N,
                                        double x, double x2, double x3,
                                        double* __restrict__ corr) __attribute__((always_inline));

    inline void ShiftSpectrumKernelAVX2(const CoeffsAVX2* __restrict__ fits,
                                        unsigned int N,
                                        double x_, double x2_, double x3_,
                                        double* __restrict__  corr)
    {
      // broadcast x3, x2, x into __m256d registers
      __m256d x=_mm256_set1_pd(x_);
      __m256d x2=_mm256_set1_pd(x2_);
      __m256d x3=_mm256_set1_pd(x3_);
      for(unsigned int n = 0; n < N; ++n){
        // out  = f.a*x3
        // out += f.b*x2
        // out += f.c*x
        // out += f.d
        // out *= corr[n]
        // store corr
        const CoeffsAVX2& f = fits[n];
        __m256d out=_mm256_mul_pd(f.a, x3);

        out=_mm256_add_pd(out, _mm256_mul_pd(f.b, x2));
        out=_mm256_add_pd(out, _mm256_mul_pd(f.c, x));
        out=_mm256_add_pd(out, f.d);

        out=_mm256_mul_pd(out, _mm256_loadu_pd(corr+4*n));
        _mm256_storeu_pd(corr+4*n, out);
        // corr[n] *= f.a*x3 + f.b*x2 + f.c*x + f.d;
      } // end for n
    }

    inline void ShiftSpectrumKernelAVX2FMA(const CoeffsAVX2* __restrict__ fits,
                                        unsigned int N,
                                        double x, double x2, double x3,
                                        double* __restrict__ corr) __attribute__((always_inline));

    inline void ShiftSpectrumKernelAVX2FMA(const CoeffsAVX2* __restrict__ fits,
                                        unsigned int N,
                                        double x_, double x2_, double x3_,
                                        double* __restrict__  corr)
    {
      // broadcast x3, x2, x into __m256d registers
      __m256d x=_mm256_set1_pd(x_);
      __m256d x2=_mm256_set1_pd(x2_);
      __m256d x3=_mm256_set1_pd(x3_);
      for(unsigned int n = 0; n < N; ++n){
        // out  = f.a*x3
        // out += f.b*x2
        // out += f.c*x
        // out += f.d
        // out *= corr[n]
        // store corr
        const CoeffsAVX2& f = fits[n];
        __m256d out=_mm256_mul_pd(f.a, x3);

        // Straight adds
        // out=_mm256_add_pd(out, _mm256_mul_pd(f.b, x2));
        // out=_mm256_add_pd(out, _mm256_mul_pd(f.c, x));

        // Fused multiply-adds
        out=_mm256_fmadd_pd(f.b, x2, out);
        __m256d tmp=_mm256_fmadd_pd(f.c, x, f.d);
        out=_mm256_add_pd(out, tmp);

        out=_mm256_mul_pd(out, _mm256_loadu_pd(corr+4*n));
        _mm256_storeu_pd(corr+4*n, out);
        // corr[n] *= f.a*x3 + f.b*x2 + f.c*x + f.d;
      } // end for n
    }
  }
}
