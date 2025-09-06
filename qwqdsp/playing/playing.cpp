#include <algorithm>
#include <array>
#include <cstddef>
#include <span>

#include "raylib.h"
#include "../playing/slider.hpp"

#include "qwqdsp/osciilor/elliptic_sine_osc.hpp"
#include "qwqdsp/osciilor/vic_sine_osc.hpp"
#include "qwqdsp/convert.hpp"

static constexpr int kWidth = 500;
static constexpr int kHeight = 400;
static constexpr float kFs = 48000.0f;

qwqdsp::oscillor::EllipticSineOsc dsp;

static void AudioInputCallback(void* _buffer, unsigned int frames) {
    struct T {
        float l;
        float r;
    };
    std::span buffer{reinterpret_cast<T*>(_buffer), frames};
    for (auto& s : buffer) {
        // s.l = dsp.Tick();
        dsp.Tick();
        s.l = dsp.PCosine();
        s.r = dsp.Sine();
    }
}

int main(void) {
    InitWindow(kWidth, kHeight, "formant");

    // dsp.SetFreq(1000, kFs);
    dsp.Reset();
    
    Knob w;
    w.on_value_change = [](float v) {
        // dsp.SetPWM(v);
        dsp.SetFreq(v);
    };
    w.set_bound(0, 0, 100, 100);
    w.set_range(0.0f, 1.00f, 0.01f, 0.5f);
    w.set_bg_color(BLACK);
    w.set_fore_color(RAYWHITE);
    w.set_title("w");

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
            w.display();
        }
        EndDrawing();
    }

    UnloadAudioStream(stream);
    CloseAudioDevice();
    CloseWindow();
}
