#pragma once
#include "qwqdsp/fx/delay_line_simd.hpp"

namespace qwqdsp_simd_element {
using DelayLineInterp = qwqdsp_fx::DelayLineInterpSIMD;

template<class SIMD_TYPE, DelayLineInterp kInterpType>
using DelayLine = qwqdsp_fx::DelayLineSIMD<SIMD_TYPE, kInterpType>;
}
