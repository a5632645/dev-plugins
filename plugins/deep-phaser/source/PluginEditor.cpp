#include "PluginEditor.h"
#include "PluginProcessor.h"

#include "qwqdsp/convert.hpp"
#include "qwqdsp/osciilor/mcf_sine_osc.hpp"

// ---------------------------------------- time prev ----------------------------------------

void TimeView::UpdateGui() {
    std::copy_n(p_.coeffs_.begin(), p_.coeff_len_, coeff_buffer_.begin());

    if (p_.coeff_len_ != 0) {
        coeff_buffer_[p_.coeff_len_] = coeff_buffer_[p_.coeff_len_ - 1];
    }
    repaint();
}

void TimeView::paint(juce::Graphics& g) {
    g.fillAll(ui::green_bg);

    // 获取图表bound
    auto b = getLocalBounds();
    b.removeFromTop(title_.getHeight());
    b.reduce(4, 4);
    auto bf = b.toFloat();
    g.setColour(ui::black_bg);
    g.fillRect(b);
    
    g.reduceClipRegion(b);
    // 绘制实际使用的，如果是mouseUp后就是实际的
    g.setColour(ui::line_fore);
    float lasty = juce::jmap(coeff_buffer_[0], -1.0f, 1.0f, bf.getBottom(), bf.getY());
    float lastx = bf.getX();
    float const fcoeff_len = static_cast<float>(p_.coeff_len_);
    for (int x = 0; x < b.getWidth(); ++x) {
        size_t const idx = static_cast<size_t>(static_cast<float>(x) * fcoeff_len / static_cast<float>(b.getWidth()));
        float const val = coeff_buffer_[idx];
        float const y = juce::jmap(val, -1.0f, 1.0f, bf.getBottom(), bf.getY());
        float const xx = static_cast<float>(x) + bf.getX();
        g.drawLine(lastx, lasty, xx, y);
        lastx = xx;
        lasty = y;
    }

    if (display_custom_.getToggleState()) {
        // 绘制自定义波形
        g.setColour(ui::active_bg);
        lasty = juce::jmap(p_.custom_coeffs_[0], -1.0f, 1.0f, bf.getBottom(), bf.getY());
        lastx = bf.getX();
        for (int x = 0; x < b.getWidth(); ++x) {
            size_t const idx = static_cast<size_t>(static_cast<float>(x) * fcoeff_len / static_cast<float>(b.getWidth()));
            float const val = p_.custom_coeffs_[idx];
            float const y = juce::jmap(val, -1.0f, 1.0f, bf.getBottom(), bf.getY());
            float const xx = static_cast<float>(x) + bf.getX();
            g.drawLine(lastx, lasty, xx, y);
            lastx = xx;
            lasty = y;
        }
    }
}

void TimeView::mouseDrag(const juce::MouseEvent& e) {
    // 获取图表bound
    auto b = getLocalBounds();
    b.removeFromTop(title_.getHeight());
    b.reduce(4, 4);

    auto pos = e.getPosition();
    pos.x = std::clamp(pos.x, b.getX(), b.getRight());
    pos.y = std::clamp(pos.y, b.getY(), b.getBottom());

    float const fcoeff_len = static_cast<float>(p_.coeff_len_);
    auto bf = b.toFloat();
    size_t idx = static_cast<size_t>((static_cast<float>(pos.getX()) - bf.getX()) * fcoeff_len / static_cast<float>(bf.getWidth()));
    idx = std::clamp(idx, 0ull, p_.coeff_len_ - 1);

    float val = juce::jmap(static_cast<float>(pos.y), bf.getY(), bf.getBottom(), 1.0f, -1.0f);
    if (e.mods.isRightButtonDown()) {
        val = 0;
    }
    
    coeff_buffer_[idx] = val;
    p_.custom_coeffs_[idx] = val;

    repaint();
    if (auto* parent = getParentComponent(); parent != nullptr) {
        static_cast<DeepPhaserAudioProcessorEditor*>(parent)->UpdateGuiFromTimeView();
    }
}

void TimeView::RepaintTimeAndSpectralView() {
    repaint();
    if (auto* parent = getParentComponent(); parent != nullptr) {
        static_cast<DeepPhaserAudioProcessorEditor*>(parent)->repaint();
    }
}

void TimeView::mouseUp(const juce::MouseEvent& e) {
    std::ignore = e;
    SendCoeffs();
}

void TimeView::SendCoeffs() {
    p_.is_using_custom_ = true;
    p_.should_update_fir_ = true;
}

void TimeView::CopyCoeffesToCustom() {
    {
        juce::ScopedLock _{p_.getCallbackLock()};
        std::copy_n(p_.coeffs_.begin(), p_.coeff_len_, p_.custom_coeffs_.begin());
    }
    std::copy(p_.custom_coeffs_.begin(), p_.custom_coeffs_.end(), coeff_buffer_.begin());
    repaint();
}

void TimeView::ClearCustomCoeffs() {
    std::fill(p_.custom_coeffs_.begin(), p_.custom_coeffs_.end(), float{});
    repaint();
}

// ---------------------------------------- spectral view ----------------------------------------

void SpectralView::paint(juce::Graphics& g) {
    g.fillAll(ui::green_bg);

    // 获取图表bound
    auto b = getLocalBounds();
    b.removeFromTop(title_.getHeight());
    b.reduce(2, 8);
    auto text_bound = b.removeFromLeft(36).toFloat();
    auto bf = b.toFloat();
    g.setColour(ui::black_bg);
    g.fillRect(b);
    
    // 绘制频谱音量数字
    float const fcoeff_len = static_cast<float>(time_.p_.coeff_len_);
    constexpr size_t kNumLines = 5;
    float const centerx = text_bound.getCentreX();
    g.setColour(juce::Colours::white);
    g.setFont(juce::Font{juce::FontOptions{}.withHeight(12)});
    for (size_t i = 0; i < kNumLines; ++i) {
        float const centery = text_bound.getY() + static_cast<float>(i) * static_cast<float>(text_bound.getHeight()) / (kNumLines - 1.0f);
        juce::Rectangle<float> text{0.0, 0.0, text_bound.getWidth(), 12.0f};
        text = text.withCentre({centerx, centery});
        float const val = max_db_ - static_cast<float>(i) * (max_db_ - min_db_) / (kNumLines - 1.0f);
        g.drawText(juce::String{val, 1}, text, juce::Justification::right);
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
    if (time_.display_custom_.getToggleState()) {
        g.setColour(ui::active_bg);
        lasty = juce::jmap(time_.p_.custom_spectral_gains[0], bf.getBottom(), bf.getY());
        lastx = bf.getX();
        for (int x = 0; x < b.getWidth(); ++x) {
            size_t const idx = static_cast<size_t>(static_cast<float>(static_cast<float>(x) * fcoeff_len) / static_cast<float>(b.getWidth()));
            float const val = time_.p_.custom_spectral_gains[idx];
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
    std::copy_n(time_.coeff_buffer_.begin(), time_.p_.coeff_len_, fft_buffer.begin());
    fft_.FFTGainPhase(fft_buffer, gains_);

    for (auto& x : gains_) {
        x = qwqdsp::convert::Gain2Db<-100.0f>(x);
    }

    auto[pmin, pmax] = std::minmax_element(gains_.begin(), gains_.end());
    float const min = *pmin;
    float const max = *pmax;
    float const scale = 1.0f / (max - min + 1e-6f);
    for (auto& x : gains_) {
        x = (x - min) * scale;
    }
    max_db_ = max;
    min_db_ = min;

    repaint();
}

void SpectralView::mouseDrag(const juce::MouseEvent& e) {
    // 获取图表bound
    auto b = getLocalBounds();
    b.removeFromTop(title_.getHeight());
    b.reduce(2, 8);
    b.removeFromLeft(36).toFloat();
    auto bf = b.toFloat();

    auto pos = e.getPosition();
    pos.x = std::clamp(pos.x, b.getX(), b.getRight());
    pos.y = std::clamp(pos.y, b.getY(), b.getBottom());

    size_t const coeff_len = time_.p_.coeff_len_;
    float const fcoeff_len = static_cast<float>(coeff_len);
    size_t idx = static_cast<size_t>((static_cast<float>(pos.getX()) - bf.getX()) * fcoeff_len / bf.getWidth());
    idx = std::clamp(idx, 0ull, coeff_len - 1);

    float val = juce::jmap(static_cast<float>(pos.y), bf.getY(), bf.getBottom(), 1.0f, 0.0f);
    if (e.mods.isRightButtonDown()) {
        val = 0;
    }
    
    time_.p_.custom_spectral_gains[idx] = val;

    // 加法合成
    std::array<qwqdsp::oscillor::MCFSineOsc, kMaxCoeffLen> oscs;
    std::array<float, kMaxCoeffLen> true_gains;
    for (size_t i = 0; i < coeff_len; ++i) {
        oscs[i].Reset(static_cast<float>(i) * std::numbers::pi_v<float> / fcoeff_len, 0.0f);
        float const db = std::lerp(-101.0f, 0.0f, time_.p_.custom_spectral_gains[i]);
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
        time_.p_.custom_coeffs_[tidx] = sum;
    }

    UpdateGui();
    time_.repaint();
}

void SpectralView::mouseUp(const juce::MouseEvent& e) {
    std::ignore = e;
    time_.SendCoeffs();
}

// ---------------------------------------- editor ----------------------------------------

DeepPhaserAudioProcessorEditor::DeepPhaserAudioProcessorEditor (DeepPhaserAudioProcessor& p)
    : AudioProcessorEditor (&p)
    , p_(p)
    , preset_panel_(*p.preset_manager_)
    , timeview_(p)
    , spectralview_(timeview_)
{
    auto& apvts = *p.value_tree_;

    addAndMakeVisible(preset_panel_);

    addAndMakeVisible(allpass_title_);
    allpass_blend_.BindParam(p.param_allpass_blend_);
    addAndMakeVisible(allpass_blend_);
    state_.BindParam(p.param_state_);
    addAndMakeVisible(state_);
    drywet_.BindParam(p.param_drywet_);
    addAndMakeVisible(drywet_);
    fb_style_.BindParam(p.param_feedback_style_);
    addAndMakeVisible(fb_style_);
    feedback_style_label_.setJustificationType(juce::Justification::right);
    addAndMakeVisible(feedback_style_label_);

    cutoff_.BindParam(apvts, "cutoff");
    addAndMakeVisible(cutoff_);
    coeff_len_.BindParam(apvts, "coeff_len");
    addAndMakeVisible(coeff_len_);
    side_lobe_.BindParam(apvts, "side_lobe");
    addAndMakeVisible(side_lobe_);
    minum_phase_.BindParam(apvts, "minum_phase");
    addAndMakeVisible(minum_phase_);
    addAndMakeVisible(fir_title_);
    highpass_.BindParam(apvts, "highpass");
    addAndMakeVisible(highpass_);
    custom_.onStateChange = [this] {
        if (custom_.getToggleState()) {
            setSize(600, 264 + 200 + 50);
            timeview_.setVisible(true);
            spectralview_.setVisible(true);
        }
        else {
            setSize(600, 264 + 50);
            timeview_.setVisible(false);
            spectralview_.setVisible(false);
        }
    };
    addAndMakeVisible(custom_);

    fb_value_.BindParam(apvts, "fb_value");
    addAndMakeVisible(fb_value_);
    panic_.setButtonText("panic");
    panic_.onClick = [&p] {
        p.Panic();
    };
    addAndMakeVisible(panic_);
    fb_damp_.BindParam(apvts, "fb_damp");
    addAndMakeVisible(fb_damp_);

    addAndMakeVisible(barber_title_);
    barber_phase_.BindParam(apvts, "barber_phase");
    addAndMakeVisible(barber_phase_);
    barber_speed_.BindParam(p.barber_lfo_state_);
    addAndMakeVisible(barber_speed_);
    barber_enable_.BindParam(apvts, "barber_enable");
    addAndMakeVisible(barber_enable_);
    barber_reset_phase_.setButtonText("reset phase");
    barber_reset_phase_.onClick = [this] {
        juce::ScopedLock _{p_.getCallbackLock()};
        p_.barber_oscillator_.Reset();
    };
    addAndMakeVisible(barber_reset_phase_);
    barber_stereo_.BindParam(p.param_barber_stereo_);
    addAndMakeVisible(barber_stereo_);

    addAndMakeVisible(blend_lfo_title_);
    addAndMakeVisible(blend_lfo_reset_phase_);
    blend_lfo_reset_phase_.onClick = [this] {
        juce::ScopedLock _{p_.getCallbackLock()};
        p_.blend_lfo_phase_ = 0;
    };
    blend_phase_.BindParam(p.param_blend_phase_);
    addAndMakeVisible(blend_phase_);
    blend_range_.BindParam(p.param_blend_range_);
    addAndMakeVisible(blend_range_);
    blend_speed_.BindParam(p.blend_lfo_state_);
    addAndMakeVisible(blend_speed_);

    addAndMakeVisible(timeview_);
    addAndMakeVisible(spectralview_);

    setSize(600, 264 + 50);

    custom_.setToggleState(p.is_using_custom_, juce::sendNotificationSync);

    startTimerHz(30);
}

DeepPhaserAudioProcessorEditor::~DeepPhaserAudioProcessorEditor() {
}

//==============================================================================
void DeepPhaserAudioProcessorEditor::paint (juce::Graphics& g) {
    g.fillAll(ui::black_bg);

    g.setColour(ui::green_bg);
    auto b = getLocalBounds();
    g.fillRect(b.removeFromTop(50 - 2));
    b.removeFromTop(2);
    g.fillRect(basic_bound_);
    g.fillRect(fir_bound_);
    g.fillRect(barber_bound_);
    g.fillRect(blend_lfo_bound_);
}

void DeepPhaserAudioProcessorEditor::resized() {
    auto b = getLocalBounds();
    preset_panel_.setBounds(b.removeFromTop(50));
    {
        auto topblock = b.removeFromTop(125);
        {
            auto basic_bound = topblock.removeFromLeft(80 * 4);
            basic_bound_ = basic_bound;
            auto basic_bound_top = basic_bound.removeFromTop(25);
            panic_.setBounds(basic_bound_top.removeFromRight(80).reduced(2));
            fb_style_.setBounds(basic_bound_top.removeFromRight(50).reduced(2));
            feedback_style_label_.setBounds(basic_bound_top.removeFromRight(100));
            allpass_title_.setBounds(basic_bound_top);
            state_.setBounds(basic_bound.removeFromLeft(64));;
            allpass_blend_.setBounds(basic_bound.removeFromLeft(64));
            fb_value_.setBounds(basic_bound.removeFromLeft(64));
            fb_damp_.setBounds(basic_bound.removeFromLeft(64));
            drywet_.setBounds(basic_bound.removeFromLeft(64));
        }
        topblock.removeFromLeft(8);
        {
            auto fir_block = topblock.removeFromLeft(80 * 4);
            fir_bound_ = fir_block;
            {
                auto fir_title = fir_block.removeFromTop(25);
                minum_phase_.setBounds(fir_title.removeFromRight(100).reduced(2, 0));
                highpass_.setBounds(fir_title.removeFromRight(70).reduced(2, 0));
                custom_.setBounds(fir_title.removeFromRight(60).reduced(2, 0));
                fir_title_.setBounds(fir_title);
            }
            cutoff_.setBounds(fir_block.removeFromLeft(80));;
            coeff_len_.setBounds(fir_block.removeFromLeft(80));;
            side_lobe_.setBounds(fir_block.removeFromLeft(80));;
        }
    }
    b.removeFromTop(8);
    {
        auto bottom_block = b.removeFromTop(125);
        {
            barber_bound_ = bottom_block.removeFromLeft(260);
            auto barber_block = barber_bound_;
            auto barber_title_bound = barber_block.removeFromTop(25);
            barber_reset_phase_.setBounds(barber_title_bound.removeFromRight(100).reduced(2));
            barber_enable_.setBounds(barber_title_bound.removeFromRight(80).reduced(2));
            barber_title_.setBounds(barber_title_bound);
            barber_phase_.setBounds(barber_block.removeFromLeft(80));
            barber_speed_.setBounds(barber_block.removeFromLeft(80));
            barber_stereo_.setBounds(barber_block);
        }
        bottom_block.removeFromLeft(8);
        {
            blend_lfo_bound_ = bottom_block.removeFromLeft(240);
            auto blend_lfo_block = blend_lfo_bound_;
            auto blend_lfo_top_block = blend_lfo_block.removeFromTop(25);
            blend_lfo_reset_phase_.setBounds(blend_lfo_top_block.removeFromRight(100).reduced(2));
            blend_lfo_title_.setBounds(blend_lfo_top_block);
            blend_speed_.setBounds(blend_lfo_block.removeFromLeft(80));
            blend_range_.setBounds(blend_lfo_block.removeFromLeft(80));
            blend_phase_.setBounds(blend_lfo_block.removeFromLeft(80));
        }
    }
    b.removeFromTop(8);
    if (custom_.getToggleState()) {
        auto graphic_block = b.removeFromTop(200);
        auto time_block = graphic_block.removeFromLeft(graphic_block.getWidth() / 2);
        time_block.removeFromRight(4);
        timeview_.setBounds(time_block);
        auto spectral_block = graphic_block;
        spectral_block.removeFromRight(4);
        spectralview_.setBounds(spectral_block);
    }
}

void DeepPhaserAudioProcessorEditor::timerCallback() {
    if (p_.have_new_coeff_.exchange(false)) {
        UpdateGui();
    }
}
