#include <algorithm>
#include <array>
#include <cstddef>
#include <span>

#include "raylib.h"
#include "../playing/slider.hpp"

#include "qwqdsp/osciilor/polyblep.hpp"
#include "qwqdsp/convert.hpp"

static constexpr int kWidth = 500;
static constexpr int kHeight = 400;
static constexpr float kFs = 48000.0f;

static qwqdsp::oscillor::PolyBlep<float, false> dsp;

static void AudioInputCallback(void* _buffer, unsigned int frames) {
    struct T {
        float l;
        float r;
    };
    std::span buffer{reinterpret_cast<T*>(_buffer), frames};
    for (auto& s : buffer) {
        s.l = dsp.SawtoothSync();
        s.r = s.l;
    }
}

int main(void) {
    InitWindow(kWidth, kHeight, "blep");

    Knob w;
    w.on_value_change = [](float v) {
        dsp.SetFreq(v, kFs);
    };
    w.set_bound(0, 0, 100, 100);
    w.set_range(0.0f, 5000.0f, 1.0f, 700.0f);
    w.set_bg_color(BLACK);
    w.set_fore_color(RAYWHITE);
    w.set_title("w");

    Knob pwm;
    pwm.on_value_change = [](float v) {
        dsp.SetPWM(v);
    };
    pwm.set_bound(0, 100, 100, 100);
    pwm.set_range(0.01f, 0.99f, 0.01f, 0.5f);
    pwm.set_bg_color(BLACK);
    pwm.set_fore_color(RAYWHITE);
    pwm.set_title("pwm");

    Knob sync;
    sync.on_value_change = [](float v) {
        dsp.SetHardSync(v);
    };
    sync.set_bound(0, 200, 100, 100);
    sync.set_range(1.0f, 4.0f, 0.01f, 1.0f);
    sync.set_bg_color(BLACK);
    sync.set_fore_color(RAYWHITE);
    sync.set_title("sync");

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
            pwm.display();
            sync.display();
        }
        EndDrawing();
    }

    UnloadAudioStream(stream);
    CloseAudioDevice();
    CloseWindow();
}