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

    // A struct to store the coefficients for 4 separate items in AVX2 registers. This means the memory layout is:
    // a0, a1, a2, a3, b0, b1, b2, b3, c0, c1, c2, c3, d0, d1, d2, d3
    // which is the convenient way around to do vectorized operations on 0,1,2,3 in parallel
    struct alignas(64) CoeffsAVX2{
      CoeffsAVX2(__m128 _a, __m128 _b, __m128 _c, __m128 _d)
        : a(_a), b(_b), c(_c), d(_d) {}
      __m128 a, b, c, d;
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
        __m256d a=_mm256_cvtps_pd(f.a);
        __m256d b=_mm256_cvtps_pd(f.b);
        __m256d c=_mm256_cvtps_pd(f.c);
        __m256d d=_mm256_cvtps_pd(f.d);
        __m256d out=_mm256_mul_pd(a, x3);

        out=_mm256_add_pd(out, _mm256_mul_pd(b, x2));
        out=_mm256_add_pd(out, _mm256_mul_pd(c, x));
        out=_mm256_add_pd(out, d);

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
        const CoeffsAVX2& f = fits[n];
        __m256d a=_mm256_cvtps_pd(f.a);
        __m256d b=_mm256_cvtps_pd(f.b);
        __m256d c=_mm256_cvtps_pd(f.c);
        __m256d d=_mm256_cvtps_pd(f.d);

        // out  = f.a*x3
        // out += f.b*x2
        // out += f.c*x
        // out += f.d
        // out *= corr[n]
        // store corr

        __m256d out=_mm256_mul_pd(a, x3);

        // Straight adds
        // out=_mm256_add_pd(out, _mm256_mul_pd(f.b, x2));
        // out=_mm256_add_pd(out, _mm256_mul_pd(f.c, x));

        // Fused multiply-adds
        out=_mm256_fmadd_pd(b, x2, out);
        __m256d tmp=_mm256_fmadd_pd(c, x, d);
        out=_mm256_add_pd(out, tmp);

        out=_mm256_mul_pd(out, _mm256_loadu_pd(corr+4*n));
        _mm256_storeu_pd(corr+4*n, out);
        // corr[n] *= f.a*x3 + f.b*x2 + f.c*x + f.d;
      } // end for n
    }
  }
}
