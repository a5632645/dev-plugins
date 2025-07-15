#pragma once
#include <array>

namespace id {

// --------------------------------------------------------------------------------
// params
// --------------------------------------------------------------------------------
static constexpr auto kMainGain = "main_gain";
static constexpr auto kSideGain = "side_gain";
static constexpr auto kOutputgain = "output_gain";
static constexpr auto kMainChannelConfig = "main_ch_config";
static constexpr auto kSideChannelConfig = "side_ch_config";

static constexpr auto kVocoderType = "vocoder_type";

static constexpr auto kEmphasisPitch = "em_pitch";
static constexpr auto kEmphasisGain = "em_gain";
static constexpr auto kEmphasisS = "em_s";

static constexpr auto kShiftPitch = "shift_pitch";
static constexpr auto kEnableShifter = "shift_enable";

static constexpr auto kForgetRate = "lpc_forget";
static constexpr auto kLPCSmooth = "lpc_smooth";
static constexpr auto kLPCOrder = "lpc_order";
static constexpr auto kRLSLPCOrder = "lpc_order2";
static constexpr auto kLPCGainAttack = "lpc_attack";
static constexpr auto kLPCGainRelease = "lpc_release";
static constexpr auto kLPCDicimate = "lpc_dicimate";

static constexpr auto kStftWindowWidth = "stft_bandwidth";
static constexpr auto kStftAttack = "stft_attack";
static constexpr auto kStftRelease = "stft_release";
static constexpr auto kStftBlend = "stft_blend";

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

// --------------------------------------------------------------------------------
// gui others
// --------------------------------------------------------------------------------
static constexpr auto kFilterTitle = "filter_title";
static constexpr auto kEnsembleTitle = "ensemble_title";
static constexpr auto kRLSTitle = "rls_title";
static constexpr auto kSliderMenuEnterValue = "sm_enter_value";
namespace combbox {
static constexpr auto kVocoderNames = kVocoderType;
static constexpr std::array kVocoderNameIds {
    "vn_burg",
    "vn_rls",
    "vn_stft",
    "vn_channel"
};
static constexpr auto kEnsembleModeNames = kEnsembleMode;
static constexpr std::array kEnsembleModeNameIds {
    "emn_sine",
    "emn_noise"
};
}

}