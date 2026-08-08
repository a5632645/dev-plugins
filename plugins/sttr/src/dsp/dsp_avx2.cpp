#if defined(__x86_64__) || defined(_M_X64)

#define SIMD_USE_AVX2
#define INST_NAME "AVX2"

#include "dsp_impl.hpp"

namespace sttr {

template <>
std::unique_ptr<Idsp> CreateDspImpl<simd::Inst::AVX2>() {
    return std::make_unique<DspImpl<simd::Inst::AVX2, simd::Float256>>();
}

} // namespace sttr

#endif
