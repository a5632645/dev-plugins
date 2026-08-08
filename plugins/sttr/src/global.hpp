#pragma once

namespace global {

// ------------------------------------------------------------------
// STTR processor constants
// ------------------------------------------------------------------

/** Maximum hop time in milliseconds. */
static constexpr float kMaxHopMs = 500.0f;

/** Number of grains, derived from the window-length multiplier (max 4). */
static constexpr int kMaxGrains = 4;

static constexpr float kMaxFormantShift = 10.0f;

// Worst-case read depth: 2 * stretch_max(2^(10/12)) * hop_max * mul_max,
// in milliseconds. The delay line is sized dynamically in Prepare().
static constexpr float kMaxDelayMs = 2.0f * 1.7818f * kMaxHopMs * static_cast<float>(kMaxGrains);

} // namespace global
