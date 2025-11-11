#include <span>
#include <map>
#include <array>

#include "raylib.h"
#include "../playing/slider.hpp"

#include "qwqdsp/osciilor/polyblep.hpp"
#include "qwqdsp/osciilor/polyblep_sync.hpp"
#include "qwqdsp/convert.hpp"
#include "qwqdsp/adsr_envelope.hpp"

static constexpr int kWidth = 1000;
static constexpr int kHeight = 400;
static constexpr float kFs = 48000.0f;

class KeyboardSynth {
public:
    using NoteCallback = std::function<void(int midiNote)>;

    KeyboardSynth(NoteCallback onNoteOn, NoteCallback onNoteOff)
        : onNoteOn_(onNoteOn), onNoteOff_(onNoteOff), currentOctave_(4) {
        
        keyMap_ = {
            {KEY_A, 0},
            {KEY_W, 1},
            {KEY_S, 2},
            {KEY_E, 3},
            {KEY_D, 4},
            {KEY_F, 5},
            {KEY_T, 6},
            {KEY_G, 7},
            {KEY_Y, 8},
            {KEY_H, 9},
            {KEY_U, 10},
            {KEY_J, 11},
            
            {KEY_K, 12},
            {KEY_O, 13},
            {KEY_L, 14},
        };
    }

    void Update() {
        HandleNoteKeys();
        HandleOctaveKeys();
    }
    
    int getBaseNote() const {
        return currentOctave_ * 12; 
    }

private:
    NoteCallback onNoteOn_;
    NoteCallback onNoteOff_;
    std::map<int, int> keyMap_;
    int currentOctave_;
    
    void HandleNoteKeys() {
        for (const auto& pair : keyMap_) {
            int key = pair.first;
            int relativeNote = pair.second;

            int midiNote = getBaseNote() + relativeNote;

            if (IsKeyPressed(key)) {
                if (midiNote <= 127) {
                    onNoteOn_(midiNote);
                }
            }

            if (IsKeyReleased(key)) {
                if (midiNote <= 127) {
                    onNoteOff_(midiNote);
                }
            }
        }
    }

    void HandleOctaveKeys() {
        if (IsKeyPressed(KEY_Z)) {
            if (currentOctave_ > 0) {
                currentOctave_--;
                TraceLog(LOG_INFO, TextFormat("Octave Down. Current Octave: C%d", currentOctave_));
            }
        }

        if (IsKeyPressed(KEY_X)) {
            if (currentOctave_ < 8) {
                currentOctave_++;
                TraceLog(LOG_INFO, TextFormat("Octave Up. Current Octave: C%d", currentOctave_));
            }
        }
    }
};

// -------------------- handle notes --------------------
class NoteQueue {
public:
    int NoteOn(int note) noexcept {
        notes_[num_notes_] = note;
        ++num_notes_;
        return note;
    }

    int NoteOff(int note) noexcept {
        if (num_notes_ == 0) return -1;
        [[maybe_unused]] auto it = std::remove(notes_.begin(), notes_.end(), note);
        --num_notes_;
        if (num_notes_ == 0) return -1;
        return notes_[num_notes_ - 1];
    }
private:
    std::array<int, 128> notes_{};
    size_t num_notes_{};
};

// -------------------- envelope --------------------
class EnvelopeGui {
public:
    EnvelopeGui() {
        attack_.set_range(0.0f, 10000.0f, 1.0f, 1.0f);
        attack_.set_bg_color(BLACK);
        attack_.set_fore_color(RAYWHITE);
        attack_.set_title("attack");

        decay_.set_range(0.0f, 10000.0f, 1.0f, 1000.0f);
        decay_.set_bg_color(BLACK);
        decay_.set_fore_color(RAYWHITE);
        decay_.set_title("decay");

        sustain_.set_range(0.0f, 1.0f, 0.01f, 1.0f);
        sustain_.set_bg_color(BLACK);
        sustain_.set_fore_color(RAYWHITE);
        sustain_.set_title("sustain");

        release_.set_range(0.0f, 10000.0f, 1.0f, 100.0f);
        release_.set_bg_color(BLACK);
        release_.set_fore_color(RAYWHITE);
        release_.set_title("release");

        envelope_param.fs = kFs;
    }

    void SetBounds(int x, int y, int w, int h) noexcept {
        w /= 4;
        attack_.set_bound(x + 0, y, w, h);
        decay_.set_bound(x + w, y, w, h);
        sustain_.set_bound(x + w * 2, y, w, h);
        release_.set_bound(x + w * 3, y, w, h);
    }

    void Update() noexcept {
        attack_.display();
        decay_.display();
        sustain_.display();
        release_.display();

        envelope_param.attack_ms = attack_.get_value();
        envelope_param.decay_ms = decay_.get_value();
        envelope_param.release_ms = release_.get_value();
        envelope_param.sustain_level = sustain_.get_value();
    }

    qwqdsp::AdsrEnvelope envelope;
    qwqdsp::AdsrEnvelope::Parameter envelope_param;
private:
    Knob attack_;
    Knob decay_;
    Knob sustain_;
    Knob release_;
};

enum Waveform {
    Sawtooth = 0,
    Square,
    PwmClassic,
    PwmNoDC,
    Triangle,
    SyncSawtooth,
    SyncPWM,
    Sine,
    SyncTriangle,
    NumWaveforms
};
static constexpr const char* kWaveformNames[]{
    "saw",
    "square",
    "pwm classic",
    "pwm noDC",
    "triangle",
    "sync saw",
    "sync pwm",
    "sine",
    "sync tri"
};

static qwqdsp::oscillor::PolyBlep<qwqdsp::oscillor::blep_coeff::BlackmanNutallApprox> dsp;
static qwqdsp::oscillor::PolyBlepSync<qwqdsp::oscillor::blep_coeff::BlackmanNutallApprox> dsp2;

static EnvelopeGui envelope_gui;
static NoteQueue note_queue_;

static Waveform waveform = Waveform::Sawtooth;
static float master_phase_{};
static float master_phase_inc_{0.00001f};

static float envelope_buffer[1024]{};
static void AudioInputCallback(void* _buffer, unsigned int frames) {
    envelope_gui.envelope.Update(envelope_gui.envelope_param);
    envelope_gui.envelope.Process({envelope_buffer, frames});

    struct T {
        float l;
        float r;
    };
    std::span buffer{reinterpret_cast<T*>(_buffer), frames};
    switch (waveform) {
        case Sawtooth:
            for (auto& s : buffer) {
                s.l = dsp.Sawtooth() * 0.5f;
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
                master_phase_ += master_phase_inc_;
                bool reset = master_phase_ > 1.0f;
                master_phase_ -= std::floor(master_phase_);
                s.l = dsp2.PWM(reset, master_phase_ / master_phase_inc_) * 0.5f;
                s.r = s.l;
            }
            break;
        case Sine:
            for (auto& s : buffer) {
                master_phase_ += master_phase_inc_;
                bool reset = master_phase_ > 1.0f;
                master_phase_ -= std::floor(master_phase_);
                s.l = dsp2.Sine(reset, master_phase_ / master_phase_inc_) * 0.5f;
                s.r = s.l;
            }
            break;
        case SyncTriangle:
            for (auto& s : buffer) {
                master_phase_ += master_phase_inc_;
                bool reset = master_phase_ > 1.0f;
                master_phase_ -= std::floor(master_phase_);
                s.l = dsp2.Triangle(reset, master_phase_ / master_phase_inc_) * 0.5f;
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

    for (unsigned int i = 0; i < frames; ++i) {
        buffer[i].l *= envelope_buffer[i];
        buffer[i].r = buffer[i].l;
    }
}

static void NoteOnCallback(int note) noexcept {
    note = note_queue_.NoteOn(note);
    envelope_gui.envelope.NoteOn(true);
    dsp.SetFreq(qwqdsp::convert::Pitch2Freq(note), kFs);
    dsp2.SetFreq(qwqdsp::convert::Pitch2Freq(note), kFs);
}

static void NoteOffCallback(int note) noexcept {
    note = note_queue_.NoteOff(note);
    if (note != -1) {
        envelope_gui.envelope.NoteOn(true);
        dsp.SetFreq(qwqdsp::convert::Pitch2Freq(note), kFs);
        dsp2.SetFreq(qwqdsp::convert::Pitch2Freq(note), kFs);
    }
    else {
        envelope_gui.envelope.Noteoff(true);
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

    dsf_bound.y += waveform_bound.height;
    auto envelope_bound = dsf_bound;
    envelope_bound.width = 200;
    envelope_bound.height = 50;
    envelope_gui.SetBounds(envelope_bound.x, envelope_bound.y, envelope_bound.width, envelope_bound.height);

    KeyboardSynth midi_keyboard{NoteOnCallback, NoteOffCallback};
    
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

            midi_keyboard.Update();
            envelope_gui.Update();
        }
        EndDrawing();
    }

    UnloadAudioStream(stream);
    CloseAudioDevice();
    CloseWindow();
}
