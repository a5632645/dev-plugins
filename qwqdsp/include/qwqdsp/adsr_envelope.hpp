#pragma once
#include <algorithm>
#include <span>
#include <cmath>

namespace qwqdsp {
class AdsrEnvelope {
public:
    struct Parameter {
        float attack_ms{1.0f};
        float decay_ms{1000.0f};
        float release_ms{100.0f};
        float fs{};
        float sustain_level{1.0f};
    };

    void Reset() noexcept {
        state_ = State::Init;
        phase_ = 0;
        last_out_ = 0;
    }

    void Process(std::span<float> block) noexcept {
        int samples = static_cast<int>(block.size());
        auto it = block.begin();
        while (samples != 0) {
            switch (state_) {
                case State::Init: {
                    std::fill(it, block.end(), 0.0f);
                    samples = 0;
                    phase_ = 0;
                    last_out_ = 0;
                    break;
                }
                case State::Attack: {
                    int can_do = attack_samples_ - phase_;
                    can_do = std::clamp(can_do, 0, samples);

                    float begin_val = last_out_;
                    float inc_val = (1.0f - begin_val) / static_cast<float>(attack_samples_ - phase_);
                    for (int i = 0; i < can_do; ++i) {
                        *it = begin_val;
                        ++it;
                        begin_val += inc_val;
                    }
                    samples -= can_do;
                    phase_ += can_do;
                    last_out_ = begin_val;

                    if (phase_ >= attack_samples_) {
                        state_ = State::Decay;
                        phase_ = 0;
                        last_out_ = 1.0f;
                    }
                    break;
                }
                case State::Decay: {
                    int can_do = decay_samples_ - phase_;
                    can_do = std::clamp(can_do, 0, samples);

                    float begin_val = last_out_;
                    float dec_val = (sustain_level_ - begin_val) / static_cast<float>(decay_samples_ - phase_);
                    for (int i = 0; i < can_do; ++i) {
                        *it = begin_val;
                        ++it;
                        begin_val += dec_val;
                    }
                    phase_ += can_do;
                    samples -= can_do;
                    last_out_ = begin_val;

                    if (phase_ >= decay_samples_) {
                        state_ = State::Sustain;
                        phase_ = 0;
                        last_out_ = sustain_level_;
                    }
                    break;
                }
                case State::Sustain: {
                    float begin_val = last_out_;
                    float inc_val = (sustain_level_ - begin_val) / static_cast<float>(samples);
                    for (int i = 0; i < samples; ++i) {
                        *it = begin_val;
                        ++it;
                        begin_val += inc_val;
                    }
                    samples = 0;
                    last_out_ = begin_val;
                    phase_ = 0;
                    break;
                }
                case State::Release: {
                    int can_do = release_samples_ - phase_;
                    can_do = std::clamp(can_do, 0, samples);

                    float begin_val = last_out_;
                    float dec_val = (0.0f - begin_val) / static_cast<float>(release_samples_ - phase_);
                    for (int i = 0; i < can_do; ++i) {
                        *it = begin_val;
                        ++it;
                        begin_val += dec_val;
                    }
                    samples -= can_do;
                    phase_ += can_do;
                    last_out_ = begin_val;

                    if (phase_ >= release_samples_) {
                        state_ = State::Init;
                        phase_ = 0;
                        last_out_ = 0.0f;
                    }
                    break;
                }
            }
        }
    }

    void ProcessExp(std::span<float> block) noexcept {
        int samples = static_cast<int>(block.size());
        auto it = block.begin();
        while (samples != 0) {
            switch (state_) {
                case State::Init: {
                    std::fill(it, block.end(), 0.0f);
                    samples = 0;
                    phase_ = 0;
                    last_out_ = 0;
                    break;
                }
                case State::Attack: {
                    float factor = ComputeSmoothFactor(static_cast<float>(attack_samples_));
                    float target = 1.0f;
                    float u_cap = last_out_;
                    while (u_cap < 1.0f - 1e-3f && samples != 0) {
                        *it = u_cap;
                        u_cap = target + factor * (u_cap - target);
                        ++it;
                        --samples;
                    }
                    last_out_ = u_cap;
                    if (u_cap >= 1.0f - 1e-3f) {
                        state_ = State::Decay;
                    }
                    break;
                }
                case State::Decay: {
                    float factor = ComputeSmoothFactor(static_cast<float>(decay_samples_));
                    float target = sustain_level_;
                    float u_cap = last_out_;
                    while (u_cap > target && samples != 0) {
                        *it = u_cap;
                        u_cap = target + factor * (u_cap - target);
                        ++it;
                        --samples;
                    }
                    last_out_ = u_cap;
                    if (u_cap <= target) {
                        state_ = State::Sustain;
                    }
                    break;
                }
                case State::Sustain: {
                    float begin_val = last_out_;
                    float inc_val = (sustain_level_ - begin_val) / static_cast<float>(samples);
                    for (int i = 0; i < samples; ++i) {
                        *it = begin_val;
                        ++it;
                        begin_val += inc_val;
                    }
                    samples = 0;
                    last_out_ = begin_val;
                    phase_ = 0;
                    break;
                }
                case State::Release: {
                    float factor = ComputeSmoothFactor(static_cast<float>(release_samples_));
                    float target = 0.0f;
                    float u_cap = last_out_;
                    while (u_cap > 1e-3f && samples != 0) {
                        *it = u_cap;
                        u_cap = target + factor * (u_cap - target);
                        ++it;
                        --samples;
                    }
                    last_out_ = u_cap;
                    if (u_cap <= 1e-3f) {
                        state_ = State::Init;
                    }
                    break;
                }
            }
        }
    }

    /**
     * @param reset_to_attack 是否强制跳转到0音量，这会导致click
     */
    void NoteOn(bool reset_to_attack) noexcept {
        state_ = State::Attack;
        phase_ = 0;
        if (reset_to_attack) {
            last_out_ = 0;
        }
    }

    /**
     * @param reset_to_release 是否强制跳转到sustain音量，这会导致click
     */
    void Noteoff(bool reset_to_release) noexcept {
        state_ = State::Release;
        phase_ = 0;
        if (reset_to_release) {
            last_out_ = sustain_level_;
        }
    }

    void Update(Parameter const& p) noexcept {
        attack_samples_ = static_cast<int>(p.attack_ms * p.fs / 1000.0f);
        decay_samples_ = static_cast<int>(p.decay_ms * p.fs / 1000.0f);
        release_samples_ = static_cast<int>(p.release_ms * p.fs / 1000.0f);
        sustain_level_ = p.sustain_level;
    }
private:
    static float ComputeSmoothFactor(float samples, float close_ratio = 3) noexcept {
        if (samples < 1.0f) {
            return 0.0f;
        }
        else {
            return std::pow(10.0f, -close_ratio / samples);
        }
    }

    enum class State {
        Init,
        Attack,
        Decay,
        Sustain,
        Release
    };
    State state_{State::Init};
    int phase_{};
    int attack_samples_{};
    int decay_samples_{};
    int release_samples_{};
    float sustain_level_{};
    float last_out_{};
};
}