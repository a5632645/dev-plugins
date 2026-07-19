#if defined(__aarch64__) || defined(_M_ARM64)

#define SIMD_USE_NEON
#define INST_NAME "NEON"

#include "dsp_impl.hpp"

namespace warpcore {

template <>
std::unique_ptr<Idsp> CreateDspImpl<simd::Inst::NEON>() {
    return std::make_unique<DspImpl<simd::Inst::NEON, simd::Float128>>();
}

} // namespace warpcore

#endif
