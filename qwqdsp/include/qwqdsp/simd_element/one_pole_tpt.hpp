#pragma once
#include "qwqdsp/filter/one_pole_tpt_simd.hpp"

namespace qwqdsp::simd_element {
template<class SIMD_TYPE>
using OnepoleTPT = qwqdsp::filter::OnePoleTPTSimd<SIMD_TYPE>;
}