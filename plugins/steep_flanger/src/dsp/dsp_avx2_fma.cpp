#if defined(__x86_64__) || defined(_M_X64)

#define SIMD_USE_FMA
#define INST_NAME "FMA"

#include "dsp_impl.hpp"

namespace steep_flanger {

template <>
std::unique_ptr<Idsp> CreateDspImpl<simd::Inst::FMA>() {
    return std::make_unique<DspImpl<simd::Inst::FMA, simd::Float256>>();
}

} // namespace steep_flanger

#endif
