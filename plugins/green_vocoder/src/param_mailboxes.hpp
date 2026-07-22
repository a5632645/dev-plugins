#pragma once
#include <atomic>

// --------------------------------------------------------------------------------
// Param mailboxes — written by param listener (message thread),
// read + cleared by audio thread, then passed to Engine::Update*()
// --------------------------------------------------------------------------------

struct TiltFilterMailbox {
    std::atomic<bool> dirty{false};
    float pre_tilt_db{10.0f};
};

struct LeakyLpcMailbox {
    std::atomic<bool> dirty{false};
    float forget_rate{10.0f};
    float smooth{1.0f};
    float gain_attack{10.0f};
    float gain_hold{10.0f};
    float gain_release{20.0f};
    float order{36.0f};
    float formant_shift{0.0f};
};

struct BlockLpcMailbox {
    std::atomic<bool> dirty{false};
    float smear{1.0f};
    float attack{10.0f};
    float poles{36.0f};
    float formant_shift{0.0f};
    int block_size{1024};
};

struct STFTVocoderMailbox {
    std::atomic<bool> dirty{false};
    float bandwidth{2.0f};
    int num_mfcc{20};
    float attack{1.0f};
    float release{100.0f};
    float blend{0.2f};
    float detail{0.3f};
    int fft_size{1024};
    int mode{1}; // 0=Standard, 1=Cepstrum, 2=MFCC
    float formant_shift{0.0f};
};

struct ChannelVocoderMailbox {
    std::atomic<bool> dirty{false};
    int filter_bank_mode{1};
    float attack{10.0f};
    float gate{-100.0f};
    float release{150.0f};
    float freq_begin{40.0f};
    float freq_end{12000.0f};
    int nbands{20};
    float scale{1.0f};
    float ripple{1.0f};
    float carry_scale{1.0f};
    int map{1}; // eChannelVocoderMap_Mel
    float formant_shift{0.0f};
};

struct PitchOscMailbox {
    std::atomic<bool> dirty{false};
    float min_pitch{80.0f};
    float max_pitch{500.0f};
    float pitch_shift{0.0f};
    float pwm{0.5f};
    float noise_gain{0.5f};
    int waveform{0};
    float glide{1.0f};
};
