#include "CAFAna/Prediction/PredictionInterpKernel.h"

// This trivial function has been split out into a separate file to make it
// easier to confirm it's being vectorized. Do something like
//
// g++ -S -fverbose-asm PredictionInterpKernel.cxx -Ofast -march=native -I../..
//
// and look at PredictionInterpKernel.s for xmm, ymm or zmm registers


