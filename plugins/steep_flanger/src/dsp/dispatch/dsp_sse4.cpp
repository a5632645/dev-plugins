#if defined(__x86_64__) || defined(_M_X64)

#define SIMD_USE_SSE4
#define INST_NAME "SSE4"

#include "../dsp_impl.hpp"

namespace steep_flanger {

template <>
std::unique_ptr<Idsp> CreateDspImpl<simd::Inst::SSE4>() {
    return std::make_unique<DspImpl<simd::Inst::SSE4, simd::Float128>>();
}

} // namespace steep_flanger

#endif
