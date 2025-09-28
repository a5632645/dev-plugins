// #include "qwqdsp/fx/resample_iir.hpp"
// #include "elliptic_blep.hpp"

// namespace qwqdsp::fx {
// ResampleIIR::ResampleIIR() = default;

// ResampleIIR::~ResampleIIR() = default;

// void ResampleIIR::Init(float source_fs, float target_fs, size_t partial_step) {
//     blep_ = std::make_unique<signalsmith::blep::EllipticBlep<float>>(
//         true, source_fs, target_fs / 2.0f - (44100.0f / 2.0f - 20000.0f), partial_step
//     );
//     phase_inc_ = source_fs / target_fs;
// }

// std::vector<float> ResampleIIR::Process(std::span<float> x) {
//     std::vector<float> ret;

//     blep_->reset();

//     float phase{};
//     float max_one_{};
//     size_t rpos{};
//     blep_->add(x[0], 0);
//     while (rpos < x.size() - 1) {
//         float const frac = phase;
//         float const v = blep_->get(frac);
//         if (std::abs(v) > max_one_) {
//             max_one_ = std::abs(v);
//         }
//         ret.push_back(v);

//         phase += phase_inc_;
//         size_t new_rpos = rpos + static_cast<size_t>(std::floor(phase));
//         phase -= std::floor(phase);

//         new_rpos = std::min(new_rpos, x.size() - 1);
//         for (size_t i = rpos; i < new_rpos; ++i) {
//             blep_->step();
//             blep_->add(x[i + 1], 0);
//         }
//         rpos = new_rpos;
//     }

//     if (max_one_ == 0.0f) {
//         max_one_ = 1.0f;
//     }
//     else {
//         max_one_ = 0.9f / max_one_;
//     }
//     for (auto& s : ret) {
//         s *= max_one_;
//     }

//     return ret;
// }

// }