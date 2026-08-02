#include "spectral_view.hpp"
#include "qwqdsp/oscillator/mcf_sine_osc.hpp"
#include "time_view.hpp"
#include "../PluginProcessor.h"
#include "qwqdsp/convert.hpp"
#include <cmath>

void SpectralView::paint(juce::Graphics& g) {
    g.fillAll(ui::green_bg);

    // 获取图表bound
    auto b = getLocalBounds();
    b.reduce(2, 8);
    auto text_bound = b.removeFromLeft(24).toFloat();
    auto bf = b.toFloat();
    g.setColour(ui::black_bg);
    g.fillRect(b);
    
    // 绘制频谱音量数字
    float const fcoeff_len = static_cast<float>(time_.p_.param_fir_coeff_len_->get());
    constexpr int kNumLines = static_cast<int>((kDbCeil - kDbFloor) / kDbStep) + 1;
    float const centerx = text_bound.getCentreX();
    g.setColour(ui::white_fore);
    g.setFont(juce::Font{juce::FontOptions{}.withHeight(12)});
    for (int i = 0; i < kNumLines; ++i) {
        float const centery = text_bound.getY() + static_cast<float>(i) * static_cast<float>(text_bound.getHeight()) / (kNumLines - 1.0f);
        juce::Rectangle<float> text{0.0, 0.0, text_bound.getWidth(), 12.0f};
        text = text.withCentre({centerx, centery});
        float const val = kDbCeil - kDbStep * static_cast<float>(i);
        g.drawText(juce::String{static_cast<int>(val)}, text, juce::Justification::right);
    }

    if (iir_) {
        DrawIir(g);
        return;
    }

    // 绘制超采样频谱
    g.setColour(ui::line_fore);
    float lasty = juce::jmap(gains_[0], bf.getBottom(), bf.getY());
    float lastx = bf.getX();
    for (int x = 0; x < b.getWidth(); ++x) {
        size_t idx = static_cast<size_t>(static_cast<float>(static_cast<size_t>(x) * gains_.size()) / static_cast<float>(b.getWidth()));
        idx = std::min(idx, gains_.size() - 1);
        float const val = gains_[idx];
        float const y = juce::jmap(val, bf.getBottom(), bf.getY());
        float const xx = static_cast<float>(x) + bf.getX();
        g.drawLine(lastx, lasty, xx, y);
        lastx = xx;
        lasty = y;
    }

    // 绘制自定义频谱
    if (time_.display_waveform_) {
        std::array<float, global::kMaxCoeffLen> custom_spectral_snapshot{};
        {
            juce::SpinLock::ScopedLockType lock(time_.p_.dsp_state_.param.custom_coeffs_lock_);
            std::copy_n(time_.p_.dsp_state_.param.custom_spectral_gains.begin(),
                        global::kMaxCoeffLen,
                        custom_spectral_snapshot.begin());
        }

        g.setColour(ui::active_bg);
        lasty = juce::jmap(custom_spectral_snapshot[0], bf.getBottom(), bf.getY());
        lastx = bf.getX();
        for (int x = 0; x < b.getWidth(); ++x) {
            size_t const idx = static_cast<size_t>(static_cast<float>(static_cast<float>(x) * fcoeff_len) / static_cast<float>(b.getWidth()));
            float const val = custom_spectral_snapshot[idx];
            float const y = juce::jmap(val, bf.getBottom(), bf.getY());
            float const xx = static_cast<float>(x) + bf.getX();
            g.drawLine(lastx, lasty, xx, y);
            lastx = xx;
            lasty = y;
        }
    }
}

void SpectralView::UpdateGui() {
    std::array<float, kGainFFTSize> fft_buffer{};
    std::copy_n(time_.coeff_buffer_.begin(), time_.p_.param_fir_coeff_len_->get(), fft_buffer.begin());
    fft_.FFTGainPhase(fft_buffer, gains_);

    for (auto& x : gains_) {
        x = qwqdsp::convert::Gain2Db<kDbFloor>(x);
    }

    for (auto& x : gains_) {
        x = std::clamp((x - kDbFloor) / (kDbCeil - kDbFloor), 0.0f, 1.0f);
    }

    repaint();
}

void SpectralView::mouseDrag(const juce::MouseEvent& e) {
    if (!time_.display_waveform_) return;
    // 获取图表bound

    auto b = getLocalBounds();
    b.reduce(2, 8);
    b.removeFromLeft(24).toFloat();
    auto bf = b.toFloat();

    auto pos = e.getPosition();
    pos.x = std::clamp(pos.x, b.getX(), b.getRight());
    pos.y = std::clamp(pos.y, b.getY(), b.getBottom());

    size_t const coeff_len = time_.p_.param_fir_coeff_len_->get();
    float const fcoeff_len = static_cast<float>(coeff_len);
    size_t idx = static_cast<size_t>((static_cast<float>(pos.getX()) - bf.getX()) * fcoeff_len / bf.getWidth());
    idx = std::clamp<size_t>(idx, 0, coeff_len - 1);

    float val = juce::jmap(static_cast<float>(pos.y), bf.getY(), bf.getBottom(), 1.0f, 0.0f);
    if (e.mods.isRightButtonDown()) {
        val = 0;
    }
    
    std::array<float, global::kMaxCoeffLen> custom_spectral_gains_snapshot{};
    {
        juce::SpinLock::ScopedLockType lock(time_.p_.dsp_state_.param.custom_coeffs_lock_);
        std::copy_n(time_.p_.dsp_state_.param.custom_spectral_gains.begin(),
                    global::kMaxCoeffLen,
                    custom_spectral_gains_snapshot.begin());
        custom_spectral_gains_snapshot[idx] = val;
    }

    // 加法合成
    std::array<qwqdsp_oscillator::MCFSineOsc, global::kMaxCoeffLen> oscs;
    std::array<float, global::kMaxCoeffLen> true_gains;
    std::array<float, global::kMaxCoeffLen> custom_coeffs_snapshot{};
    for (size_t i = 0; i < coeff_len; ++i) {
        oscs[i].Reset(static_cast<float>(i) * std::numbers::pi_v<float> / fcoeff_len, 0.0f);
        float const db = std::lerp(-101.0f, 0.0f, custom_spectral_gains_snapshot[i]);
        if (db < -100.0f) {
            true_gains[i] = 0;
        }
        else {
            true_gains[i] = qwqdsp::convert::Db2Gain(db);
        }
    }
    
    for (size_t tidx = 0; tidx < coeff_len; ++tidx) {
        float sum{};
        for (size_t fidx = 0; fidx < coeff_len; ++fidx) {
            sum += true_gains[fidx] * oscs[fidx].Tick();
        }
        time_.coeff_buffer_[tidx] = sum;
        custom_coeffs_snapshot[tidx] = sum;
    }

    {
        juce::SpinLock::ScopedLockType lock(time_.p_.dsp_state_.param.custom_coeffs_lock_);
        std::copy_n(custom_spectral_gains_snapshot.begin(),
                    global::kMaxCoeffLen,
                    time_.p_.dsp_state_.param.custom_spectral_gains.begin());
        std::copy_n(custom_coeffs_snapshot.begin(),
                    coeff_len,
                    time_.p_.dsp_state_.param.custom_coeffs_.begin());
    }

    UpdateGui();
    time_.repaint();
}

void SpectralView::mouseUp(const juce::MouseEvent& e) {
    if (!time_.display_waveform_) return;
    std::ignore = e;
    time_.SendCoeffs();
}

void SpectralView::mouseDown(const juce::MouseEvent& e) {
    if (!time_.display_waveform_) return;
    mouseDrag(e);
}

void SpectralView::DrawIir(juce::Graphics& g) {
    int nfilter_ = p_.param_iir_filter_num_->get();
    float w_ = p_.param_fir_cutoff_->get();
    float ripple_ = p_.param_iir_ripple_->get();
    bool highpass = p_.param_fir_highpass_->get();

    if (nfilter_ <= 0) {
        return;
    }

    auto b = getLocalBounds();
    b.reduce(2, 8);
    b.removeFromLeft(24);
    auto bf = b.toFloat();
    if (bf.getWidth() <= 1.0f || bf.getHeight() <= 1.0f) {
        return;
    }

    constexpr float kWMax = std::numbers::pi_v<float> - 0.1f;
    constexpr float kMinGain = 1.0e-8f;
    float cutoff_w = w_;
    if (highpass) {
        cutoff_w = std::numbers::pi_v<float> - cutoff_w;
    }
    float const wc = std::tan(std::clamp(cutoff_w, 1.0e-4f, std::numbers::pi_v<float> - 1.0e-4f) * 0.5f);
    float const ripple_db = std::max(ripple_, 0.001f);
    float const epsilon = std::sqrt(std::pow(10.0f, ripple_db * 0.1f) - 1.0f);
    float const g_mul = std::pow(10.0f, ripple_db * 0.05f * 0.5f);
    int const order = std::max(1, nfilter_ * 2);

    g.setColour(ui::line_fore);

    float lastx = bf.getX();
    float lasty = bf.getBottom();
    for (int px = 0; px < b.getWidth(); ++px) {
        float const t = static_cast<float>(px) / std::max(1.0f, static_cast<float>(b.getWidth() - 1));
        float const w = t * kWMax;
        float const tw = std::max(std::tan(w * 0.5f), 1.0e-8f);
        float const x = highpass ? (wc / tw) : (tw / wc);

        float tn{};
        if (x <= 1.0f) {
            tn = std::cos(static_cast<float>(order) * std::acos(std::clamp(x, -1.0f, 1.0f)));
        } else {
            tn = std::cosh(static_cast<float>(order) * std::acosh(x));
        }

        float const gain = g_mul / std::sqrt(1.0f + epsilon * epsilon * tn * tn);
        float const db = juce::jlimit(kDbFloor, kDbCeil, 20.0f * std::log10(std::max(gain, kMinGain)));
        float const norm = std::clamp((db - kDbFloor) / (kDbCeil - kDbFloor), 0.0f, 1.0f);
        float const yy = juce::jmap(norm, bf.getBottom(), bf.getY());
        float const xx = bf.getX() + static_cast<float>(px);

        if (px > 0) {
            g.drawLine(lastx, lasty, xx, yy, 1.5f);
        }
        lastx = xx;
        lasty = yy;
    }
}
