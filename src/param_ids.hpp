#pragma once

namespace id {

static constexpr auto kHighpassPitch = "hp_pitch";

static constexpr auto kEmphasisPitch = "em_pitch";
static constexpr auto kEmphasisGain = "em_gain";
static constexpr auto kEmphasisS = "em_s";

static constexpr auto kShiftPitch = "shift_pitch";

static constexpr auto kLearnRate = "lpc_leran";
static constexpr auto kForgetRate = "lpc_forget";
static constexpr auto kLPCSmooth = "lpc_smooth";
static constexpr auto kLPCOrder = "lpc_order";
static constexpr auto kLPCGainAttack = "lpc_attack";
static constexpr auto kLPCGainRelease = "lpc_release";
static constexpr auto kLPCDicimate = "lpc_dicimate";

static constexpr auto kStftWindowWidth = "stft_bandwidth";

}