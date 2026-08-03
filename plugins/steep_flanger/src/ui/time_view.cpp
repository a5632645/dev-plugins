#include "time_view.hpp"
#include "../PluginProcessor.h"
#include "plugin_ui.hpp"

void TimeView::UpdateGui() {
    p_.dsp_->GetCoeffs(coeff_buffer_.data(), static_cast<int>(global::kSIMDMaxCoeffLen));
    int curr_coeff_len = static_cast<int>(p_.params_.fir_coeff_len.Get());
    if (curr_coeff_len != 0) {
        coeff_buffer_[curr_coeff_len] = coeff_buffer_[curr_coeff_len - 1];
    }
    repaint();
}

void TimeView::paint(juce::Graphics& g) {
    g.fillAll(ui::green_bg);

    // 获取图表bound
    auto b = getLocalBounds();
    b.reduce(2, 2);
    auto bf = b.toFloat();
    g.setColour(ui::black_bg);
    g.fillRect(b);

    g.reduceClipRegion(b);
    // 绘制实际使用的，如果是mouseUp后就是实际的
    g.setColour(ui::line_fore);
    float lasty = juce::jmap(coeff_buffer_[0], -1.0f, 1.0f, bf.getBottom(), bf.getY());
    float lastx = bf.getX();
    float const fcoeff_len = p_.params_.fir_coeff_len.Get();
    for (int x = 0; x < b.getWidth(); ++x) {
        size_t const idx = static_cast<size_t>(static_cast<float>(x) * fcoeff_len / static_cast<float>(b.getWidth()));
        float const val = coeff_buffer_[idx];
        float const y = juce::jmap(val, -1.0f, 1.0f, bf.getBottom(), bf.getY());
        float const xx = static_cast<float>(x) + bf.getX();
        g.drawLine(lastx, lasty, xx, y);
        lastx = xx;
        lasty = y;
    }

    if (display_waveform_) {
        const juce::SpinLock::ScopedLockType lock(p_.params_.control_.custom_coeffs_lock_);
        // 绘制自定义波形
        g.setColour(ui::active_bg);
        lasty = juce::jmap(p_.params_.control_.custom_coeffs_[0], -1.0f, 1.0f, bf.getBottom(), bf.getY());
        lastx = bf.getX();
        for (int x = 0; x < b.getWidth(); ++x) {
            size_t const idx =
                static_cast<size_t>(static_cast<float>(x) * fcoeff_len / static_cast<float>(b.getWidth()));
            float const val = p_.params_.control_.custom_coeffs_[idx];
            float const y = juce::jmap(val, -1.0f, 1.0f, bf.getBottom(), bf.getY());
            float const xx = static_cast<float>(x) + bf.getX();
            g.drawLine(lastx, lasty, xx, y);
            lastx = xx;
            lasty = y;
        }
    }
}

void TimeView::mouseDrag(const juce::MouseEvent& e) {
    if (!display_waveform_) return;

    // 获取图表bound
    auto b = getLocalBounds();
    b.reduce(2, 2);

    auto pos = e.getPosition();
    pos.x = std::clamp(pos.x, b.getX(), b.getRight());
    pos.y = std::clamp(pos.y, b.getY(), b.getBottom());

    float const fcoeff_len = p_.params_.fir_coeff_len.Get();
    auto bf = b.toFloat();
    size_t idx = static_cast<size_t>((static_cast<float>(pos.getX()) - bf.getX()) * fcoeff_len
                                     / static_cast<float>(bf.getWidth()));
    idx = std::clamp<size_t>(idx, 0, static_cast<size_t>(p_.params_.fir_coeff_len.Get()) - 1);

    float val = juce::jmap(static_cast<float>(pos.y), bf.getY(), bf.getBottom(), 1.0f, -1.0f);
    if (e.mods.isRightButtonDown()) {
        val = 0;
    }

    coeff_buffer_[idx] = val;
    {
        const juce::SpinLock::ScopedLockType lock(p_.params_.control_.custom_coeffs_lock_);
        p_.params_.control_.custom_coeffs_[idx] = val;
    }

    repaint();
    if (auto* parent = findParentComponentOfClass<PluginUi>(); parent != nullptr) {
        parent->UpdateGuiFromTimeView();
    }
}

void TimeView::RepaintTimeAndSpectralView() {
    repaint();
    if (auto* parent = findParentComponentOfClass<PluginUi>(); parent != nullptr) {
        parent->repaint();
    }
}

void TimeView::mouseUp(const juce::MouseEvent& e) {
    if (!display_waveform_) return;

    std::ignore = e;
    SendCoeffs();
}

void TimeView::SendCoeffs() {
    p_.params_.control_.fir_source = steep_flanger::DspParam::FirSource::kTimeCoeff;
    p_.params_.control_.should_update_fir_ = true;
}

void TimeView::CopyCoeffesToCustom() {
    std::array<float, global::kMaxCoeffLen> snapshot{};
    p_.dsp_->GetCoeffs(snapshot.data(), static_cast<int>(snapshot.size()));
    const juce::SpinLock::ScopedLockType custom_lock(p_.params_.control_.custom_coeffs_lock_);
    std::ranges::copy(snapshot, p_.params_.control_.custom_coeffs_.begin());
    std::ranges::copy(snapshot, coeff_buffer_.begin());
    repaint();
}

void TimeView::ClearCustomCoeffs() {
    const juce::SpinLock::ScopedLockType lock(p_.params_.control_.custom_coeffs_lock_);
    std::ranges::fill(p_.params_.control_.custom_coeffs_, float{});
    repaint();
}
