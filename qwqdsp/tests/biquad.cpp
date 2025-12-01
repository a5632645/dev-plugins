#include <qwqdsp/filter/rbj.hpp>
#include <qwqdsp/filter/biquad.hpp>
#include <qwqdsp/filter/lattice_biquad.hpp>

int main() {
    constexpr size_t kIrLen = 256;
    float ir_biquad[kIrLen] {1.0f};
    float ir_lattice[kIrLen] {1.0f};

    qwqdsp::filter::RBJ design;
    design.Lowpass(2.0f, 10.0f);
    auto coeff = design.ToBiquadCoeff();

    qwqdsp::filter::Biquad biquad;
    qwqdsp::filter::LatticeBiquad lattice;
    biquad.Set(coeff);
    lattice.Set(coeff);

    for (size_t i = 0; i < kIrLen; ++i) {
        ir_biquad[i] = biquad.Tick(ir_biquad[i]);
        ir_lattice[i] = lattice.Tick(ir_lattice[i]);
    }
}