#include "idsp.hpp"
#include "pluginshared/simd/simd.hpp"

#include <simd_detector.h>

namespace steep_flanger {

std::unique_ptr<Idsp> CreateDsp() {
    using IS = simd_detector::InstructionSet;

#if defined(__aarch64__) || defined(_M_ARM64)
    return CreateDspImpl<simd::Inst::NEON>();
#elif defined(__x86_64__) || defined(_M_X64)
    if (simd_detector::is_supported(IS::AVX2)) {
        if (simd_detector::is_supported(IS::FMA3)) {
            return CreateDspImpl<simd::Inst::FMA>();
        }
        else {
            return CreateDspImpl<simd::Inst::AVX2>();
        }
    }
    if (simd_detector::is_supported(IS::AVX)) {
        return CreateDspImpl<simd::Inst::AVX>();
    }
    if (simd_detector::is_supported(IS::SSE4_1)) {
        return CreateDspImpl<simd::Inst::SSE4>();
    }
    return CreateDspImpl<simd::Inst::SSE2>();
#else
#error "unsupport platform"
#endif
}

} // namespace steep_flanger
