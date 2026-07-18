#define SIMD_USE_SSE2
#define INST_NAME "SSE2"

#include "dsp_impl.hpp"

namespace vital_reverb {

template <>
std::unique_ptr<Idsp> CreateDspImpl<simd::Inst::SSE2>() {
    return std::make_unique<DspImpl<simd::Inst::SSE2, simd::Float128>>();
}

} // namespace vital_reverb
