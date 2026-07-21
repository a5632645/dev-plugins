#pragma once
#include <algorithm>
#include <cassert>
#include <numbers>
#include "com/iirn_filter.hpp"
#include "com/xiir_delay.hpp"
#include "global.hpp"
#include "idsp.hpp"
#include "pluginshared/dsp/delay_line_1ch_4time.hpp"
#include "pluginshared/dsp/one_pole_tpt.hpp"
#include "pluginshared/dsp/stereo_iir_hilbert_cpx.hpp"
#include "pluginshared/dsp/stereo_iir_hilbert_cpx2.hpp"
#include "pluginshared/simd/simd.hpp"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wsign-conversion"

namespace steep_flanger {

template <simd::Inst inst, class SimdT>
struct DspStateN {
    // ----------------------------------------
    // fir part
    // ----------------------------------------
    pluginshared::dsp::DelayLineSingleChannelMultiTime<inst, SimdT> delay_left_;
    pluginshared::dsp::DelayLineSingleChannelMultiTime<inst, SimdT> delay_right_;

    // ----------------------------------------
    // iir part
    // ----------------------------------------
    com::IirNFilter<SimdT> iir_[global::kIirMaxNumFilters / simd::LaneSize<SimdT>];

    // -------------------- hilbert filter --------------------
    pluginshared::dsp::StereoIIRHilbertCpx hilbert_complex_;
};

template <simd::Inst inst, class SimdT>
class DspImpl : public Idsp {
public:
    ~DspImpl() override = default;

    void Init(float fs) override {
    }

    void Reset() override {
    }

    void Update(const DspParam& p) override {
        
    }

    void Process(float* left, float* right, int num_samples) override {
        
    }

    std::string_view InstName() override {
        return INST_NAME;
    }
private:
    
};

} // namespace steep_flanger

#pragma GCC diagnostic pop
