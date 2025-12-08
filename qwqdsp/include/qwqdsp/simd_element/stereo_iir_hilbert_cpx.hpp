#pragma once
#include "qwqdsp/filter/iir_cpx_hilbert_stereo_simd.hpp"

namespace qwqdsp_simd_element {
template<class SIMD_TYPE>
using StereoIIRHilbertCpx = qwqdsp_filter::StereoIIRHilbertCpx<SIMD_TYPE>;

template<class SIMD_TYPE>
using StereoIIRHilbertDeeperCpx = qwqdsp_filter::StereoIIRHilbertDeeperCpx<SIMD_TYPE>;
}
