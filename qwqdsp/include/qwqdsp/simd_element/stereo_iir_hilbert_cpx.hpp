#pragma once
#include "qwqdsp/filter/iir_cpx_hilbert_stereo_simd.hpp"

namespace qwqdsp::simd_element {
template<class SIMD_TYPE>
using StereoIIRHilbertCpx = qwqdsp::filter::StereoIIRHilbertCpx<SIMD_TYPE>;

template<class SIMD_TYPE>
using StereoIIRHilbertDeeperCpx = qwqdsp::filter::StereoIIRHilbertDeeperCpx<SIMD_TYPE>;
}