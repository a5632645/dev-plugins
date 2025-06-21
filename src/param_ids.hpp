#pragma once

namespace id {

static constexpr auto kMainGain = "main_gain";
static constexpr auto kSideGain = "side_gain";
static constexpr auto kOutputgain = "output_gain";

static constexpr auto kVocoderType = "vocoder_type";

static constexpr auto kHighpassPitch = "hp_pitch";

static constexpr auto kEmphasisPitch = "em_pitch";
static constexpr auto kEmphasisGain = "em_gain";
static constexpr auto kEmphasisS = "em_s";

static constexpr auto kShiftPitch = "shift_pitch";

static constexpr auto kForgetRate = "lpc_forget";
static constexpr auto kLPCSmooth = "lpc_smooth";
static constexpr auto kLPCOrder = "lpc_order";
static constexpr auto kRLSLPCOrder = "lpc_order2";
static constexpr auto kLPCGainAttack = "lpc_attack";
static constexpr auto kLPCGainRelease = "lpc_release";
static constexpr auto kLPCDicimate = "lpc_dicimate";

static constexpr auto kStftWindowWidth = "stft_bandwidth";
static constexpr auto kStftRelease = "stft_release";
static constexpr auto kStftBlend = "stft_blend";

static constexpr auto kCepstrumOmega = "cepstrum_omega";

}