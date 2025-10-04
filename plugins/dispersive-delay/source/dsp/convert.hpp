#pragma once
#include <cmath>

inline static float SimpleLerp(float a, float b, float t) {
    return a + (b - a) * t;
}

inline static float Semitone2Hz(float semitone) {
    return 8.176f * std::pow(2.0f, semitone / 12.0f);
}

inline static float Hz2Semitone(float hz) {
    return 12.0f * std::log2(hz / 8.176f);
}

inline static const auto s_st_begin = Hz2Semitone(20.0f);
inline static const auto s_st_end = Hz2Semitone(20000.0f);

inline static float SemitoneMap(float nor_x) {
    auto st = s_st_begin + nor_x * (s_st_end - s_st_begin);
    return Semitone2Hz(st);
}

inline static float SemitoneNor(float nor) {
    return s_st_begin + nor * (s_st_end - s_st_begin);
}

// Ã·¶ûÆµÂÊÓ³Éä
inline static float Hz2Mel(float hz) {
    return 1127.0f * std::log(1.0f + hz / 700.0f);
}

inline static float Mel2Hz(float mel) {
    return 700.0f * (std::exp(mel / 1127.0f) - 1.0f);
}

inline static const auto kMinMel = Hz2Mel(20.0f);
inline static const auto kMaxMel = Hz2Mel(20000.0f);

inline static float MelMap(float nor) {
    return Mel2Hz(SimpleLerp(kMinMel, kMaxMel, nor));
}