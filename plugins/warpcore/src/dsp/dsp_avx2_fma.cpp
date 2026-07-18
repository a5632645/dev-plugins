#define SIMD_USE_FMA
#define INST_NAME "FMA"

#include "dsp_impl.hpp"

namespace warpcore {

template <>
std::unique_ptr<Idsp> CreateDspImpl<simd::Inst::FMA>() {
    return std::make_unique<DspImpl<simd::Inst::FMA, simd::Float256>>();
}

} // namespace warpcore
