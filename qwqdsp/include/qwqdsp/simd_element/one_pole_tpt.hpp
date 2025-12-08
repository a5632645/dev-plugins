#pragma once
#include "qwqdsp/filter/one_pole_tpt_simd.hpp"

namespace qwqdsp_simd_element {
template<class SIMD_TYPE>
using OnepoleTPT = qwqdsp_filter::OnePoleTPTSimd<SIMD_TYPE>;
}
