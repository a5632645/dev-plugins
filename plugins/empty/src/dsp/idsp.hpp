#pragma once
#include <juce_core/juce_core.h>
#include <array>
#include <memory>
#include <string_view>
#include "pluginshared/simd/inst.hpp"

namespace empty {

struct DspParam {};

class Idsp {
public:
    virtual ~Idsp() = default;

    virtual void Init(float fs) = 0;
    virtual void Reset() = 0;
    virtual void Update(const DspParam& p) = 0;
    virtual void Process(float* left, float* right, int num_samples) = 0;
    virtual std::string_view InstName() = 0;
};

using DspHanle = std::unique_ptr<Idsp>;

template <simd::Inst inst>
std::unique_ptr<Idsp> CreateDspImpl();

std::unique_ptr<Idsp> CreateDsp();

} // namespace empty
