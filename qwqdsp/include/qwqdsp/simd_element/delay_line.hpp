#pragma once
#include "qwqdsp/fx/delay_line_simd.hpp"

namespace qwqdsp::simd_element {
using DelayLineInterp = qwqdsp::fx::DelayLineInterpSIMD;

template<class SIMD_TYPE, DelayLineInterp kInterpType>
using DelayLine = qwqdsp::fx::DelayLineSIMD<SIMD_TYPE, kInterpType>;
}