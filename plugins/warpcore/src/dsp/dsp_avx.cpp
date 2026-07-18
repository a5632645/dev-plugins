#define SIMD_USE_AVX
#define INST_NAME "AVX"

#include "dsp_impl.hpp"

namespace warpcore {

template <>
std::unique_ptr<Idsp> CreateDspImpl<simd::Inst::AVX>() {
    return std::make_unique<DspImpl<simd::Inst::AVX, simd::Float256>>();
}

} // namespace warpcore
