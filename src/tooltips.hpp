#pragma once

namespace tooltip {

static constexpr auto kMainGain = "control the main gain";
static constexpr auto kSideGain = "control the side gain";
static constexpr auto kOutputgain = "control the output gain";

static constexpr auto kVocoderType = "change the vocoder type";

static constexpr auto kHighpassPitch = "set main high pass filter cutoff pitch";

static constexpr auto kEmphasisPitch = "set main high shelf filter cutoff pitch";
static constexpr auto kEmphasisGain = "set main high shelf filter gain";
static constexpr auto kEmphasisS = "set main high shelf filter resonance";

static constexpr auto kShiftPitch = "set main pitch shifter, this cause formant shift like effects";

static constexpr auto kForgetRate = "lpc vocoder's forget rate";
static constexpr auto kLPCSmooth = "lpc's lattice coeffient smooth, this cause smear responce";
static constexpr auto kLPCOrder = "burg lpc order, higher order take higher cpu";
static constexpr auto kRLSLPCOrder = "rls lpc order, higher order take higher cpu";
static constexpr auto kLPCGainAttack = "lpc vocoder gain attack";
static constexpr auto kLPCGainRelease = "lpc vocoder gain release";
static constexpr auto kLPCDicimate = "lpc vocoder dicimate, higher value make a more robotic sound and clearity";

static constexpr auto kStftWindowWidth = "expand energy in frequency axis by spectral leak caused by sinc window";
static constexpr auto kStftRelease = "bin energy release in time axis";
static constexpr auto kStftBlend = "bin energy remap, higher value cause more noisy sound while lower cause crystal sound";

static constexpr auto kCepstrumOmega = "cepstrum filter cutoff";

}