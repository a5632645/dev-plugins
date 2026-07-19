#pragma once
#include "idsp.hpp"
#include "pluginshared/simd/simd.hpp"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wsign-conversion"

namespace empty {

template <simd::Inst inst, class SimdT>
class DspImpl : public Idsp {
public:
    ~DspImpl() override = default;

    void Init(float fs) override {}

    void Reset() override {}

    void Update(const DspParam& p) override {}

    void Process(float* left, float* right, int num_samples) override {}

    std::string_view InstName() override {
        return INST_NAME;
    }
private:
};

} // namespace empty

#pragma GCC diagnostic pop
