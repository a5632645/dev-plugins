#include <span>

#include "raylib.h"
#include "../playing/slider.hpp"

#include "qwqdsp/osciilor/polyblep.hpp"
#include "qwqdsp/osciilor/polyblep_sync.hpp"
#include "qwqdsp/convert.hpp"

static constexpr int kWidth = 800;
static constexpr int kHeight = 400;
static constexpr float kFs = 48000.0f;

enum Waveform {
    Sawtooth = 0,
    Square,
    PwmClassic,
    PwmNoDC,
    Triangle,
    SyncSawtooth,
    SyncSquare,
    SyncPWM,
    NumWaveforms
};
static constexpr const char* kWaveformNames[]{
    "saw",
    "square",
    "pwm classic",
    "pwm noDC",
    "triangle",
    "sync saw",
    "sync square",
    "sync pwm"
};

static qwqdsp::oscillor::PolyBlep<qwqdsp::oscillor::blep_coeff::BlackmanNutall> dsp;
static qwqdsp::oscillor::PolyBlepSync<qwqdsp::oscillor::blep_coeff::BlackmanNutall> dsp2;
static Waveform waveform = Waveform::Sawtooth;
static float master_phase_{};
static float master_phase_inc_{0.00001f};

static void AudioInputCallback(void* _buffer, unsigned int frames) {
    struct T {
        float l;
        float r;
    };
    std::span buffer{reinterpret_cast<T*>(_buffer), frames};
    switch (waveform) {
        case Sawtooth:
            for (auto& s : buffer) {
                s.l = dsp.Sawtooth() * 0.5f;
                // s.l = dsp2.Sawtooth(false, 0) * 0.5f;
                s.r = s.l;
            }
            break;
        case Square:
            for (auto& s : buffer) {
                s.l = dsp.Sqaure() * 0.5f;
                s.r = s.l;
            }
            break;
        case PwmClassic:
            for (auto& s : buffer) {
                s.l = dsp.PWM_Classic() * 0.5f;
                s.r = s.l;
            }
            break;
        case PwmNoDC:
            for (auto& s : buffer) {
                s.l = dsp.PWM_NoDC() * 0.5f;
                s.r = s.l;
            }
            break;
        case Triangle:
            for (auto& s : buffer) {
                s.l = dsp.Triangle() * 0.5f;
                s.r = s.l;
            }
            break;
        case SyncSawtooth:
            for (auto& s : buffer) {
                master_phase_ += master_phase_inc_;
                bool reset = master_phase_ > 1.0f;
                master_phase_ -= std::floor(master_phase_);
                s.l = dsp2.Sawtooth(reset, master_phase_ / master_phase_inc_) * 0.5f;
                s.r = s.l;
            }
            break;
        case SyncPWM:
            for (auto& s : buffer) {
                s.l = dsp.PWMSync() * 0.5f;
                s.r = s.l;
            }
            break;
        case SyncSquare:
            for (auto& s : buffer) {
                s.l = dsp.SqaureSync() * 0.5f;
                s.r = s.l;
            }
            break;
        default:
            for (auto& s : buffer) {
                s.l = 0;
                s.r = s.l;
            }
            break;
    }
}

int main(void) {
    InitWindow(kWidth, kHeight, "polyblep");

    InitAudioDevice();
    SetAudioStreamBufferSizeDefault(512);
    AudioStream stream = LoadAudioStream(48000, 32, 2);
    SetAudioStreamCallback(stream, AudioInputCallback);
    PlayAudioStream(stream);
    
    Rectangle dsf_bound;
    dsf_bound.x = 0;
    dsf_bound.y = 0;
    dsf_bound.width = 50;
    dsf_bound.height = 50;

    Knob pitch;
    pitch.on_value_change = [](float pitch) {
        float f = qwqdsp::convert::Pitch2Freq(pitch);
        dsp.SetFreq(f, kFs);
        master_phase_inc_ = f / kFs;
    };
    pitch.set_bound(dsf_bound);
    pitch.set_range(0.0f, 127.0f, 0.1f, 0.0f);
    pitch.set_bg_color(BLACK);
    pitch.set_fore_color(RAYWHITE);
    pitch.set_title("pitch");

    Knob slave_pitch;
    slave_pitch.on_value_change = [](float pitch) {
        dsp2.SetFreq(qwqdsp::convert::Pitch2Freq(pitch), kFs);
    };
    dsf_bound.y += dsf_bound.height;
    slave_pitch.set_bound(dsf_bound);
    slave_pitch.set_range(0.0f, 127.0f, 0.1f, 0.0f);
    slave_pitch.set_bg_color(BLACK);
    slave_pitch.set_fore_color(RAYWHITE);
    slave_pitch.set_title("slave_pitch");

    Knob pwm;
    pwm.on_value_change = [](float width) {
        dsp.SetPWM(width);
        dsp2.SetPWM(width);
    };
    dsf_bound.y += dsf_bound.height;
    pwm.set_bound(dsf_bound);
    pwm.set_range(0.01f, 0.99f, 0.01f, 0.5f);
    pwm.set_bg_color(BLACK);
    pwm.set_fore_color(RAYWHITE);
    pwm.set_title("pwm");

    Knob sync;
    sync.on_value_change = [](float width) {
        dsp.SetHardSync(width);
    };
    dsf_bound.y += dsf_bound.height;
    sync.set_bound(dsf_bound);
    sync.set_range(1.0f, 8.0f, 0.1f, 2.2f);
    sync.set_bg_color(BLACK);
    sync.set_fore_color(RAYWHITE);
    sync.set_title("sync");

    dsf_bound.y += dsf_bound.height;
    auto waveform_bound = dsf_bound;
    waveform_bound.width = GetScreenWidth();
    waveform_bound.height = 30;
    float each_width = waveform_bound.width / static_cast<float>(NumWaveforms);
    size_t num_waveforms = static_cast<size_t>(NumWaveforms);
    
    SetTargetFPS(30);
    while (!WindowShouldClose()) {
        BeginDrawing();
        {
            ClearBackground(BLACK);

            pitch.display();
            pwm.display();
            sync.display();
            slave_pitch.display();

            auto mouse_pos = GetMousePosition();
            for (size_t i = 0; i < num_waveforms; ++i) {
                float x = i * each_width;
                float y = waveform_bound.y;
                float w = each_width - 2;
                float h = waveform_bound.height;
                if (CheckCollisionPointRec(mouse_pos, Rectangle{x,y,w,h}) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                    waveform = static_cast<Waveform>(i);
                }

                if (i != waveform) {
                    DrawRectangleLines(x, y, w, h, WHITE);
                    DrawText(kWaveformNames[i], x, y, 12, WHITE);
                }
                else {
                    DrawRectangle(x, y, w, h, WHITE);
                    DrawText(kWaveformNames[i], x, y, 12, BLACK);
                }
            }
        }
        EndDrawing();
    }

    UnloadAudioStream(stream);
    CloseAudioDevice();
    CloseWindow();
}
