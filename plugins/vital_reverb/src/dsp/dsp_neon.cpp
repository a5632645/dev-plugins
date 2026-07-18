#define SIMD_USE_NEON
#define INST_NAME "NEON"

#include "dsp_impl.hpp"

namespace vital_reverb {

template <>
std::unique_ptr<Idsp> CreateDspImpl<simd::Inst::NEON>() {
    return std::make_unique<DspImpl<simd::Inst::NEON, simd::Float128>>();
}

} // namespace vital_reverb
