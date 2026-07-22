#pragma once
#include <pluginshared/wrap_parameters.hpp>

// --------------------------------------------------------------------------------
// enums
// --------------------------------------------------------------------------------
enum eVocoderType {
    eVocoderType_LeakyBurgLPC = 0,
    eVocoderType_BlockBurgLPC,
    eVocoderType_STFTVocoder,
    eVocoderType_ChannelVocoder,
    eVocoderType_NumVocoderTypes
};

static const juce::StringArray kVocoderNames{
    "Leaky LPC",
    "Block LPC",
    "STFT",
    "Bandpass",
};

static const juce::StringArray kChannelVocoderMapNames{"linear", "mel", "log"};

// --------------------------------------------------------------------------------
// params class — all plugin parameters defined here
// --------------------------------------------------------------------------------
class Params {
public:
    // pre fx
    pluginshared::FloatParam pre_tilt{
        "pre_tilt", {0.0f, 20.0f, 1.0f},
         10.0f
    };
    pluginshared::BoolParam channel_swap{"ch_swap", false};
    pluginshared::ChoiceParam pitch_channel{
        "pitch_ch", juce::StringArray{"Off", "Main L", "Main R", "Side L", "Side R"},
         "Off"
    };

    // vocoder type
    pluginshared::ChoiceParam vocoder_type{"vocoder_type", kVocoderNames, "STFT"};

    // pitch shifter
    pluginshared::FloatParam shift_pitch{
        "shift_pitch", {-24.0f, 24.0f, 1.0f},
         0.0f
    };

    // channel vocoder
    pluginshared::ChoiceParam cv_filter_bank_mode{
        "cv_fbank_mode",
        juce::StringArray{"bandpass 12", "stack butterworth 24", "stack butterworth 36", "flat butterworth 24",
                          "flat butterworth 36", "chebyshev 24", "chebyshev 36", "Elliptic 24", "Elliptic 36"},
        "stack butterworth 24"
    };
    pluginshared::FloatParam cv_attack{
        "cv_attack", {1.0f, 1000.0f, 1.0f},
         10.0f
    };
    pluginshared::FloatParam cv_gate{
        "cv_gate", {-100.0f, 0.0f, 1.0f},
         -100.0f
    };
    pluginshared::FloatParam cv_release{
        "cv_release", {10.0f, 32000.0f, 1.0f, 0.4f},
         150.0f
    };
    pluginshared::FloatParam cv_freq_begin{
        "cv_fbegin", {20.0f, 2000.0f, 1.0f},
         40.0f
    };
    pluginshared::FloatParam cv_freq_end{
        "cv_fend", {4000.0f, 20000.0f, 1.0f},
         12000.0f
    };
    pluginshared::FloatParam cv_nbands{
        "cv_nbands", {4.0f, 100.0f, 4.0f},
         20.0f
    };
    pluginshared::FloatParam cv_scale{
        "cv_scale", {0.1f, 2.0f, 0.1f},
         1.0f
    };
    pluginshared::FloatParam cv_ripple{
        "cv_ripple", {0.1f, 10.0f, 0.1f},
         1.0f
    };
    pluginshared::FloatParam cv_carry_scale{
        "cv_carry_scale", {0.1f, 2.0f, 0.1f},
         1.0f
    };
    pluginshared::ChoiceParam cv_map{"cv_map", kChannelVocoderMapNames, "mel"};

    // lpc
    pluginshared::FloatParam lpc_forget{
        "lpc_forget", {5.0f, 200.0f, 1.0f, 0.4f},
         10.0f
    };
    pluginshared::FloatParam lpc_smooth{
        "lpc_smooth", {0.0f, 50.0f, 1.0f, 0.4f},
         1.0f
    };
    pluginshared::FloatParam lpc_gain_attack{
        "lpc_attack", {10.0f, 100.0f, 1.0f, 0.4f},
         10.0f
    };
    pluginshared::FloatParam lpc_gain_hold{
        "lpc_hold", {1.0f, 100.0f, 1.0f, 0.4f},
         10.0f
    };
    pluginshared::FloatParam lpc_gain_release{
        "lpc_release", {5.0f, 200.0f, 1.0f, 0.4f},
         20.0f
    };
    pluginshared::FloatParam lpc_order{
        "lpc_order", {4.0f, 80.0f, 4.0f},
         36.0f
    };

    // stft
    pluginshared::FloatParam stft_bandwidth{
        "stft_bandwidth", {0.0f, 5.0f, 0.1f},
         2.0f
    };
    pluginshared::FloatParam mfcc_nbands{
        "mfcc_nbands", {8.0f, 80.0f, 4.0f},
         20.0f
    };
    pluginshared::FloatParam stft_release{
        "stft_release", {1.0f, 1000.0f, 1.0f, 0.4f},
         100.0f
    };
    pluginshared::FloatParam stft_attack{
        "stft_attack", {1.0f, 1000.0f, 1.0f, 0.4f},
         1.0f
    };
    pluginshared::FloatParam stft_blend{
        "stft_blend", {0.0f, 0.99f, 0.01f},
         0.2f
    };
    pluginshared::ChoiceParam stft_size{
        "stft_size", juce::StringArray{"256", "512", "1024", "2048", "4096"},
         "1024"
    };
    pluginshared::FloatParam stft_detail{
        "stft_detail", {0.01f, 1.0f, 0.01f},
         0.3f
    };
    pluginshared::ChoiceParam stft_type{
        "stft_type", juce::StringArray{"Standard", "Cepstrum", "MFCC"},
         "Cepstrum"
    };

    // pitch tracking
    pluginshared::FloatParam track_low{
        "track_low", {20.0f, 300.0f, 1.0f},
         80.0f
    };
    pluginshared::FloatParam track_high{
        "track_high", {300.0f, 800.0f, 1.0f},
         500.0f
    };
    pluginshared::FloatParam track_pitch{
        "track_pitch", {-36.0f, 36.0f, 1.0f},
         0.0f
    };
    pluginshared::FloatParam track_pwm{
        "track_pwm", {0.01f, 0.99f, 0.01f},
         0.5f
    };
    pluginshared::FloatParam track_noise{
        "track_noise", {0.0f, 1.0f, 0.01f},
         0.5f
    };
    pluginshared::ChoiceParam track_waveform{
        "track_waveform", juce::StringArray{"saw", "pwm"},
         "saw"
    };
    pluginshared::FloatParam track_glide{
        "track_glide", {1.0f, 1000.0f, 1.0f, 0.4f},
         1.0f
    };
};
