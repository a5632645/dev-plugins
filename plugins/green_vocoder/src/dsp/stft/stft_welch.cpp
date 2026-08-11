#include "stft_welch.hpp"

#include <algorithm>
#include <cmath>

#include <qwqdsp/convert.hpp>
#include "qwqdsp/interpolation.hpp"

namespace {

// 增益上限（dB）：防止非法/极端值经 Db2Gain 放大为 INF
constexpr float kMaxGainDb = 40.0f;

} // namespace

namespace green_vocoder::dsp {

void STFTWelch::Init(STFT& self) {
    SetParam(Params{}, self);
}

void STFTWelch::SetParam(const Params& p, STFT& self) {
    int const fft_size = self.fft_size_;
    int const num_bins = fft_size / 2 + 1;
    int const welch_frames = std::max(1, p.welch_frames);

    bool const size_changed = num_bins != num_bins_;
    if (size_changed) {
        num_bins_ = num_bins;
        for (auto& lg : log_gains_)
            lg.assign(static_cast<size_t>(num_bins_) + 1, p.floor_db);

        // hann 分析窗重建增益（2 / sum(hann) ≈ 4 / N）
        double sum = 0.0;
        for (float const w : self.hann_window_)
            sum += static_cast<double>(w);
        window_gain_ = 2.0f / static_cast<float>(sum);
    }

    // Welch 帧数或尺寸变化时重建环缓冲（分配开销大）
    if (size_changed || welch_frames != welch_frames_) {
        welch_frames_ = welch_frames;
        welch_buf_.assign(static_cast<size_t>(2) * static_cast<size_t>(welch_frames_) * static_cast<size_t>(num_bins_),
                          0.0f);
        welch_sum_.assign(static_cast<size_t>(2) * static_cast<size_t>(num_bins_), 0.0f);
        welch_pos_ = {};
        welch_count_ = {};
    }

    floor_db_ = p.floor_db;
}

void STFTWelch::operator()(STFT& self, std::span<const float> real_in, std::span<const float> imag_in,
                           std::span<float> real_out, std::span<float> imag_out, int channel) {
    auto& gains = channel == 0 ? self.gains_ : self.gains2_;
    auto& log_gains = log_gains_[static_cast<size_t>(channel)];
    int const num_bins = num_bins_;
    size_t const ch = static_cast<size_t>(channel);

    // Welch 多帧平均：写入当前帧功率谱并增量更新环和
    float* sum = welch_sum_.data() + ch * static_cast<size_t>(num_bins);
    float* slot = welch_buf_.data()
                  + (ch * static_cast<size_t>(welch_frames_) + static_cast<size_t>(welch_pos_[ch]))
                        * static_cast<size_t>(num_bins);
    for (int i = 0; i < num_bins; ++i) {
        float power = real_in[static_cast<size_t>(i)] * real_in[static_cast<size_t>(i)]
                    + imag_in[static_cast<size_t>(i)] * imag_in[static_cast<size_t>(i)];
        if (!std::isfinite(power))
            power = 0.0f; // NaN/INF 按 0 处理，防止污染 Welch 环
        sum[static_cast<size_t>(i)] += power - slot[static_cast<size_t>(i)];
        slot[static_cast<size_t>(i)] = power;
    }
    welch_pos_[ch] = (welch_pos_[ch] + 1) % welch_frames_;
    if (welch_count_[ch] < welch_frames_)
        ++welch_count_[ch];
    float const inv = 1.0f / static_cast<float>(welch_count_[ch]);

    // log 域攻击/释放平滑 + 谱下限（含 NaN/INF 防护）
    for (int i = 0; i < num_bins; ++i) {
        float const p_avg = sum[static_cast<size_t>(i)] * inv;
        float gain = std::sqrt(std::max(p_avg, 0.0f)) * window_gain_;
        gain = self.Blend(gain);

        // 非法值（NaN/INF/非正）一律按谱下限处理，防止污染平滑状态
        float target_db = floor_db_;
        if (std::isfinite(gain) && gain > 0.0f)
            target_db = std::clamp(20.0f * std::log10(gain), floor_db_, kMaxGainDb);

        if (target_db > log_gains[static_cast<size_t>(i)])
            log_gains[static_cast<size_t>(i)] =
                self.attack_factor_ * log_gains[static_cast<size_t>(i)] + (1.0f - self.attack_factor_) * target_db;
        else
            log_gains[static_cast<size_t>(i)] =
                self.decay_ * log_gains[static_cast<size_t>(i)] + (1.0f - self.decay_) * target_db;

        gains[static_cast<size_t>(i)] = qwqdsp::convert::Db2Gain(log_gains[static_cast<size_t>(i)]);
    }
    gains[static_cast<size_t>(num_bins)] = gains[0];
    // 共振峰搬移
    for (int i = 0; i < num_bins; ++i) {
        float idx = static_cast<float>(i) * self.formant_mul_;
        float frac = idx - std::floor(idx);
        int iidx = static_cast<int>(idx);

        float g = 0;
        if (iidx < num_bins) {
            g = qwqdsp::Interpolation::Linear(gains[static_cast<size_t>(iidx)], gains[static_cast<size_t>(iidx) + 1],
                                              frac);
        }

        real_out[static_cast<size_t>(i)] *= g;
        imag_out[static_cast<size_t>(i)] *= g;
    }
}

} // namespace green_vocoder::dsp
