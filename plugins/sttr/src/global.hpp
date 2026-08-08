#pragma once

namespace global {

// ------------------------------------------------------------------
// STTR processor constants
// ------------------------------------------------------------------

/** Maximum hop time in milliseconds. */
static constexpr float kMaxHopMs = 500.0f;

/** Number of grains, derived from the window-length multiplier (max 4). */
static constexpr int kMaxGrains = 4;

/** Formant shift range in semitones (stretch = 2^(formant/12)). */
static constexpr float kMaxFormantShift = 12.0f;

/** Maximum stretch ratio, i.e. 2^(kMaxFormantShift/12) = 2.0 at +12 st. */
static constexpr float kMaxStretch = 2.0f;

// Worst-case read depth: the reverse grain sweeps the read delay up to
// (1 + stretch_max) * hop_max * mul_max. The delay line is sized dynamically
// in Prepare().
static constexpr float kMaxDelayMs = (1.0f + kMaxStretch) * kMaxHopMs * static_cast<float>(kMaxGrains);

// One-pole slew (dryDelay / mix) time constants in milliseconds.
static constexpr float kDryDelaySlewMs = 130.0f;
static constexpr float kMixSlewMs = 0.15f;

} // namespace global
