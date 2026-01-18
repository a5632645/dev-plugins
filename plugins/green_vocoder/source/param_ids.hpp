#pragma once
#include <juce_core/juce_core.h>

enum eVocoderType {
    eVocoderType_LeakyBurgLPC = 0,
    eVocoderType_BlockBurgLPC,
    eVocoderType_STFTVocoder,
    eVocoderType_MFCCVocoder,
    eVocoderType_ChannelVocoder,
    eVocoderType_NumVocoderTypes
};

enum eChannelVocoderMap {
    eChannelVocoderMap_Linear = 0,
    eChannelVocoderMap_Mel,
    eChannelVocoderMap_Log,
    eChannelVocoderMap_NumEnums
};

static const juce::StringArray kVocoderNames{
    "Leaky Burg LPC",
    "Block Burg LPC",
    "STFT Vocoder",
    "MFCC Vocoder",
    "Channel Vocoder",
};

static const juce::StringArray kChannelVocoderMapNames{
    "linear",
    "mel",
    "log"
};

namespace id {

// --------------------------------------------------------------------------------
// params
// --------------------------------------------------------------------------------
static constexpr auto kOutputgain = "output_gain";
static constexpr auto kMainChannelConfig = "main_ch_config";
static constexpr auto kSideChannelConfig = "side_ch_config";

static constexpr auto kVocoderType = "vocoder_type";

static constexpr auto kPreTilt = "pre_tilt";

static constexpr auto kShiftPitch = "shift_pitch";

static constexpr auto kForgetRate = "lpc_forget";
static constexpr auto kLPCSmooth = "lpc_smooth";
static constexpr auto kLPCOrder = "lpc_order";
static constexpr auto kLPCGainAttack = "lpc_attack";
static constexpr auto kLPCGainHold = "lpc_hold";
static constexpr auto kLPCGainRelease = "lpc_release";
static constexpr auto kLPCDicimate = "lpc_dicimate";

static constexpr auto kStftWindowWidth = "stft_bandwidth";
static constexpr auto kMfccNumBands = "mfcc_nbands";
static constexpr auto kStftAttack = "stft_attack";
static constexpr auto kStftRelease = "stft_release";
static constexpr auto kStftBlend = "stft_blend";
static constexpr auto kStftSize = "stft_size";
static constexpr auto kStftVocoderV2 = "stft_v2";
static constexpr auto kStftDetail = "stft_detail";

static constexpr auto kEnsembleDetune = "ens_detune";
static constexpr auto kEnsembleRate = "ens_rate";
static constexpr auto kEnsembleSpread = "ens_spread";
static constexpr auto kEnsembleMix = "ens_mix";
static constexpr auto kEnsembleNumVoices = "ens_num_voices";
static constexpr auto kEnsembleMode = "ens_mode";

static constexpr auto kChannelVocoderNBands = "cv_nbands";
static constexpr auto kChannelVocoderFreqBegin = "cv_fbegin";
static constexpr auto kChannelVocoderFreqEnd = "cv_fend";
static constexpr auto kChannelVocoderAttack = "cv_attack";
static constexpr auto kChannelVocoderRelease = "cv_release";
static constexpr auto kChannelVocoderScale = "cv_scale";
static constexpr auto kChannelVocoderCarryScale = "cv_carry_scale";
static constexpr auto kChannelVocoderMap = "cv_map";
static constexpr auto kChannelVocoderFilterBankMode = "cv_fbank_mode";
static constexpr auto kChannelVocoderGate = "cv_gate";

static constexpr auto kTrackingLow = "track_low";
static constexpr auto kTrackingHigh = "track_high";
static constexpr auto kTrackingPitch = "track_pitch";
static constexpr auto kTrackingWaveform = "track_waveform";
static constexpr auto kTrackingPwm = "track_pwm";
static constexpr auto kTrackingNoise = "track_noise";
static constexpr auto kTrackingGlide = "track_glide";

} // id
