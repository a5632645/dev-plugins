#include <algorithm>
#include <array>
#include <cstddef>
#include <span>

#include "raylib.h"
#include "../playing/slider.hpp"

#include "qwqdsp/osciilor/noise.hpp"

static constexpr int kWidth = 500;
static constexpr int kHeight = 400;
static constexpr float kFs = 48000.0f;

static qwqdsp::oscillor::BrownNoise dsp;

static void AudioInputCallback(void* _buffer, unsigned int frames) {
    struct T {
        float l;
        float r;
    };
    std::span buffer{reinterpret_cast<T*>(_buffer), frames};
    for (auto& s : buffer) {
        s.l = dsp.Next();
        s.r = s.l;
    }
}

int main(void) {
    InitWindow(kWidth, kHeight, "formant");

    InitAudioDevice();
    SetAudioStreamBufferSizeDefault(512);
    AudioStream stream = LoadAudioStream(48000, 32, 2);
    SetAudioStreamCallback(stream, AudioInputCallback);
    PlayAudioStream(stream);
    
    SetTargetFPS(30);
    while (!WindowShouldClose()) {
        BeginDrawing();
        {
            ClearBackground(BLACK);
        }
        EndDrawing();
    }

    UnloadAudioStream(stream);
    CloseAudioDevice();
    CloseWindow();
}
