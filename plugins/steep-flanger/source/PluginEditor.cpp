#include "PluginEditor.h"
#include "PluginProcessor.h"

#include "qwqdsp/convert.hpp"
#include "qwqdsp/osciilor/mcf_sine_osc.hpp"

// ---------------------------------------- time prev ----------------------------------------

void TimeView::UpdateGui() {
    {
        juce::ScopedLock _{p_.getCallbackLock()};
        std::copy_n(p_.coeffs_.begin(), p_.coeff_len_, coeff_buffer_.begin());
    }
    if (p_.coeff_len_ != 0) {
        coeff_buffer_[p_.coeff_len_] = coeff_buffer_[p_.coeff_len_ - 1];
    }
    repaint();
}

void TimeView::paint(juce::Graphics& g) {
    g.fillAll(green_bg);

    // 获取图表bound
    auto b = getLocalBounds();
    b.removeFromTop(title_.getHeight());
    b.reduce(4, 4);
    auto bf = b.toFloat();
    g.setColour(black_bg);
    g.fillRect(b);
    
    g.reduceClipRegion(b);
    // 绘制实际使用的，如果是mouseUp后就是实际的
    g.setColour(line_fore);
    float lasty = juce::jmap(coeff_buffer_[0], -1.0f, 1.0f, bf.getBottom(), bf.getY());
    float lastx = bf.getX();
    for (int x = 0; x < b.getWidth(); ++x) {
        size_t const idx = static_cast<size_t>(static_cast<float>(x * p_.coeff_len_) / static_cast<float>(b.getWidth()));
        float const val = coeff_buffer_[idx];
        float const y = juce::jmap(val, -1.0f, 1.0f, bf.getBottom(), bf.getY());
        float const xx = x + bf.getX();
        g.drawLine(lastx, lasty, xx, y);
        lastx = xx;
        lasty = y;
    }

    if (display_custom_.getToggleState()) {
        // 绘制自定义波形
        g.setColour(active_bg);
        lasty = juce::jmap(p_.custom_coeffs_[0], -1.0f, 1.0f, bf.getBottom(), bf.getY());
        lastx = bf.getX();
        for (int x = 0; x < b.getWidth(); ++x) {
            size_t const idx = static_cast<size_t>(static_cast<float>(x * p_.coeff_len_) / static_cast<float>(b.getWidth()));
            float const val = p_.custom_coeffs_[idx];
            float const y = juce::jmap(val, -1.0f, 1.0f, bf.getBottom(), bf.getY());
            float const xx = x + bf.getX();
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

    auto bf = b.toFloat();
    size_t idx = (pos.getX() - bf.getX()) * p_.coeff_len_ / bf.getWidth();
    idx = std::min(idx, p_.coeff_len_ - 1);

    float val = juce::jmap(static_cast<float>(pos.y), bf.getY(), bf.getBottom(), 1.0f, -1.0f);
    if (e.mods.isRightButtonDown()) {
        val = 0;
    }
    
    coeff_buffer_[idx] = val;
    p_.custom_coeffs_[idx] = val;

    repaint();
    if (auto* parent = getParentComponent(); parent != nullptr) {
        static_cast<SteepFlangerAudioProcessorEditor*>(parent)->UpdateGuiFromTimeView();
    }
}

void TimeView::RepaintTimeAndSpectralView() {
    repaint();
    if (auto* parent = getParentComponent(); parent != nullptr) {
        static_cast<SteepFlangerAudioProcessorEditor*>(parent)->repaint();
    }
}

void TimeView::mouseUp(const juce::MouseEvent& e) {
    SendCoeffs();
}

void TimeView::SendCoeffs() {
    {
        juce::ScopedLock _{p_.getCallbackLock()};
        p_.is_using_custom_ = true;
        p_.UpdateCoeff();
    }
    p_.editor_update_.UpdateGui();
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
    g.fillAll(green_bg);

    // 获取图表bound
    auto b = getLocalBounds();
    b.removeFromTop(title_.getHeight());
    b.reduce(2, 8);
    auto text_bound = b.removeFromLeft(36).toFloat();
    auto bf = b.toFloat();
    g.setColour(black_bg);
    g.fillRect(b);
    
    // 绘制频谱音量数字
    constexpr size_t kNumLines = 5;
    float const centerx = text_bound.getCentreX();
    g.setColour(juce::Colours::white);
    g.setFont(juce::Font{12});
    for (size_t i = 0; i < kNumLines; ++i) {
        float const centery = text_bound.getY() + i * text_bound.getHeight() / (kNumLines - 1.0f);
        juce::Rectangle<float> text{0.0, 0.0, text_bound.getWidth(), 12.0f};
        text = text.withCentre({centerx, centery});
        float const val = max_db_ - i * (max_db_ - min_db_) / (kNumLines - 1.0f);
        g.drawText(juce::String{val, 1}, text, juce::Justification::right);
    }

    // 绘制超采样频谱
    g.setColour(line_fore);
    float lasty = juce::jmap(gains_[0], bf.getBottom(), bf.getY());
    float lastx = bf.getX();
    for (int x = 0; x < b.getWidth(); ++x) {
        size_t idx = static_cast<size_t>(static_cast<float>(x * gains_.size()) / static_cast<float>(b.getWidth()));
        idx = std::min(idx, gains_.size() - 1);
        float const val = gains_[idx];
        float const y = juce::jmap(val, bf.getBottom(), bf.getY());
        float const xx = x + bf.getX();
        g.drawLine(lastx, lasty, xx, y);
        lastx = xx;
        lasty = y;
    }

    // 绘制自定义频谱
    if (time_.display_custom_.getToggleState()) {
        g.setColour(active_bg);
        lasty = juce::jmap(time_.p_.custom_spectral_gains[0], bf.getBottom(), bf.getY());
        lastx = bf.getX();
        for (int x = 0; x < b.getWidth(); ++x) {
            size_t const idx = static_cast<size_t>(static_cast<float>(x * time_.p_.coeff_len_) / static_cast<float>(b.getWidth()));
            float const val = time_.p_.custom_spectral_gains[idx];
            float const y = juce::jmap(val, bf.getBottom(), bf.getY());
            float const xx = x + bf.getX();
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
        x = qwqdsp::convert::Gain2Db(x);
        x = std::max(x, -100.0f);
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
    size_t idx = (pos.getX() - bf.getX()) * coeff_len / bf.getWidth();
    idx = std::min(idx, coeff_len - 1);

    float val = juce::jmap(static_cast<float>(pos.y), bf.getY(), bf.getBottom(), 1.0f, 0.0f);
    if (e.mods.isRightButtonDown()) {
        val = 0;
    }
    
    time_.p_.custom_spectral_gains[idx] = val;

    // 加法合成
    std::array<qwqdsp::oscillor::MCFSineOsc, kMaxCoeffLen> oscs;
    std::array<float, kMaxCoeffLen> true_gains;
    for (size_t i = 0; i < coeff_len; ++i) {
        oscs[i].Reset(i * std::numbers::pi_v<float> / coeff_len, static_cast<float>(rand()) / static_cast<float>(RAND_MAX));
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
    time_.SendCoeffs();
}

// ---------------------------------------- editor ----------------------------------------

SteepFlangerAudioProcessorEditor::SteepFlangerAudioProcessorEditor (SteepFlangerAudioProcessor& p)
    : AudioProcessorEditor (&p)
    , p_(p)
    , timeview_(p)
    , spectralview_(timeview_)
{
    auto& apvts = *p.value_tree_;

    plugin_title_.setText("SteepFlanger Prototype by Mana", juce::dontSendNotification);
    plugin_title_.setFont(juce::Font{28});
    addAndMakeVisible(plugin_title_);

    addAndMakeVisible(lfo_title_);
    delay_.BindParam(apvts, "delay");
    addAndMakeVisible(delay_);
    depth_.BindParam(apvts, "depth");
    addAndMakeVisible(depth_);
    speed_.BindParam(apvts, "speed");
    addAndMakeVisible(speed_);
    phase_.BindParam(apvts, "phase");
    addAndMakeVisible(phase_);
    lfo_reset_phase_.setButtonText("reset phase");
    lfo_reset_phase_.onClick = [this] {
        juce::ScopedLock _{p_.getCallbackLock()};
        p_.phase_ = 0;
    };
    addAndMakeVisible(lfo_reset_phase_);

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
            setSize(600, 294 + 200);
            timeview_.setVisible(true);
            spectralview_.setVisible(true);
        }
        else {
            setSize(600, 294);
            timeview_.setVisible(false);
            spectralview_.setVisible(false);
        }
    };
    addAndMakeVisible(custom_);

    fb_enable_.BindParam(apvts, "fb_enable");
    addAndMakeVisible(fb_enable_);
    fb_value_.BindParam(apvts, "fb_value");
    addAndMakeVisible(fb_value_);
    panic_.setButtonText("panic");
    panic_.onClick = [this, &p] {
        fb_enable_.setToggleState(false, juce::NotificationType::sendNotificationSync);
        p.Panic();
    };
    addAndMakeVisible(panic_);
    fb_damp_.BindParam(apvts, "fb_damp");
    addAndMakeVisible(fb_damp_);
    addAndMakeVisible(feedback_title_);

    addAndMakeVisible(barber_title_);
    barber_phase_.BindParam(apvts, "barber_phase");
    addAndMakeVisible(barber_phase_);
    barber_speed_.BindParam(apvts, "barber_speed");
    addAndMakeVisible(barber_speed_);
    barber_enable_.BindParam(apvts, "barber_enable");
    addAndMakeVisible(barber_enable_);
    barber_reset_phase_.setButtonText("reset phase");
    barber_reset_phase_.onClick = [this] {
        juce::ScopedLock _{p_.getCallbackLock()};
        p_.barber_oscillator_.Reset();
    };
    addAndMakeVisible(barber_reset_phase_);

    addAndMakeVisible(timeview_);
    addAndMakeVisible(spectralview_);

    setSize(600, 296);

    custom_.setToggleState(p.is_using_custom_, juce::sendNotificationSync);
    p.editor_update_.OnEditorCreate(this);

    startTimerHz(10);
}

SteepFlangerAudioProcessorEditor::~SteepFlangerAudioProcessorEditor() {
    p_.editor_update_.OnEditorDestory();
}

//==============================================================================
void SteepFlangerAudioProcessorEditor::paint (juce::Graphics& g) {
    g.fillAll(juce::Colour{22,27,32});

    auto b = getLocalBounds();
    {
        auto title_block = b.removeFromTop(plugin_title_.getFont().getHeight());
        g.setColour(juce::Colour{193,193,166});
        g.fillRect(title_block);
    }
    g.setColour(green_bg);
    {
        auto topblock = b.removeFromTop(125);
        {
            auto lfo_block = topblock.removeFromLeft(80 * 4);
            g.fillRect(lfo_block);
        }
        topblock.removeFromLeft(8);
        {
            auto fir_block = topblock.removeFromLeft(80 * 4);
            g.fillRect(fir_block);
        }
    }
    b.removeFromTop(8);
    {
        auto bottom_block = b.removeFromTop(125);
        {
            auto feedback_block = bottom_block.removeFromLeft(80 * 3);
            g.fillRect(feedback_block);
        }
        bottom_block.removeFromLeft(8);
        {
            g.fillRect(bottom_block);
        }
    }
}

void SteepFlangerAudioProcessorEditor::resized() {
    auto b = getLocalBounds();
    {
        plugin_title_.setBounds(b.removeFromTop(plugin_title_.getFont().getHeight()));
    }
    {
        auto topblock = b.removeFromTop(125);
        {
            auto lfo_block = topblock.removeFromLeft(80 * 4);
            auto lfo_block_top = lfo_block.removeFromTop(25);
            lfo_reset_phase_.setBounds(lfo_block_top.removeFromRight(100).reduced(1, 1));
            lfo_title_.setBounds(lfo_block_top);
            delay_.setBounds(lfo_block.removeFromLeft(80));;
            depth_.setBounds(lfo_block.removeFromLeft(80));
            speed_.setBounds(lfo_block.removeFromLeft(80));
            phase_.setBounds(lfo_block.removeFromLeft(80));
        }
        topblock.removeFromLeft(8);
        {
            auto fir_block = topblock.removeFromLeft(80 * 4);
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
            auto feedback_block = bottom_block.removeFromLeft(80 * 3);
            feedback_title_.setBounds(feedback_block.removeFromTop(25));
            {
                auto button_block = feedback_block.removeFromLeft(80);
                button_block.removeFromTop(8);
                fb_enable_.setBounds(button_block.removeFromTop(30));
                button_block.removeFromBottom(8);
                panic_.setBounds(button_block.removeFromBottom(30));
            }
            fb_value_.setBounds(feedback_block.removeFromLeft(80));
            fb_damp_.setBounds(feedback_block.removeFromLeft(80));
        }
        bottom_block.removeFromLeft(8);
        {
            barber_title_.setBounds(bottom_block.removeFromTop(25));
            barber_phase_.setBounds(bottom_block.removeFromLeft(80));
            barber_speed_.setBounds(bottom_block.removeFromLeft(80));
            auto e = bottom_block.removeFromLeft(100);
            auto barber_enable_block = e.removeFromTop(e.getHeight() / 2);
            barber_enable_.setBounds(barber_enable_block.withSizeKeepingCentre(e.getWidth(), 25));
            barber_reset_phase_.setBounds(e.withSizeKeepingCentre(e.getWidth(), 25));
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

void SteepFlangerAudioProcessorEditor::timerCallback() {
    static double a = 0;

    auto cpu_percent = p_.measurer.getLoadAsPercentage();
    a = a * 0.9f + 0.1f * cpu_percent;
    juce::String s;
    s << "[cpu]: " << a << "%";
    juce::Logger::getCurrentLogger()->writeToLog(s);
}