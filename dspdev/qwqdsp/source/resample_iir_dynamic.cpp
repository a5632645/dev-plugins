// #include "qwqdsp/fx/resample_iir_dynamic.hpp"
// #include "elliptic_blep.hpp"

// namespace qwqdsp::fx {
// ResampleIIRDynamic::ResampleIIRDynamic() = default;
// ResampleIIRDynamic::~ResampleIIRDynamic() = default;

// void ResampleIIRDynamic::Init(float source_fs) {
//     source_fs_ = source_fs;
//     blep_ = std::make_unique<signalsmith::blep::EllipticBlep<float>>(
//         true, source_fs, 20000.0f, 127
//     );
//     Reset();
// }

// void ResampleIIRDynamic::Set(float source_fs, float target_fs) noexcept {
//     SetRatio(source_fs / target_fs);
// }

// void ResampleIIRDynamic::SetRatio(float ratio) noexcept {
//     if (!first_init_) {
//         float const last_phase = static_cast<float>(need_) + phase_ - phase_inc_;
//         float new_phase = last_phase + ratio;
//         need_ = static_cast<size_t>(std::floor(new_phase));
//         phase_ = new_phase - std::floor(new_phase);
//     }
//     phase_inc_ = ratio;

//     if (ratio > 1.0f) {
//         blep_->SetCutoff(std::min(20000.0f, source_fs_ * 0.5f / ratio));
//     }
//     else {
//         blep_->SetCutoff(std::min(20000.0f, source_fs_ * 0.5f));
//     }
// }

// void ResampleIIRDynamic::SetPitchShift(float shift) noexcept {
//     SetRatio(std::exp2(shift / 12.0f));
// }

// void ResampleIIRDynamic::Reset() noexcept {
//     first_init_ = true;
//     need_ = 1;
//     wpos_ = 0;
//     rpos_ = 0;
//     phase_ = 0;
//     buffer_size_ = 0;
//     blep_->reset();
// }

// void ResampleIIRDynamic::Push(float x) noexcept {
//     buffer_[wpos_++] = x;
//     wpos_ &= 63;
//     ++buffer_size_;
// }

// float ResampleIIRDynamic::Read() noexcept {
//     first_init_ = false;

//     for (size_t i = 0; i < need_; ++i) {
//         blep_->step();
//         blep_->add(buffer_[rpos_++], 0);
//         rpos_ &= 63;
//     }
//     buffer_size_ -= need_;

//     float const frac = phase_;
//     phase_ += phase_inc_;
//     need_ = static_cast<size_t>(std::floor(phase_));
//     phase_ -= std::floor(phase_);

//     return blep_->get(frac);
// }

// bool ResampleIIRDynamic::IsReady() const noexcept {
//     return buffer_size_ >= need_;
// }
// }