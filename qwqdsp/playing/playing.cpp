#include <raylib.h>
#include "qwqdsp/filter/analog_responce.hpp"
#include "qwqdsp/filter/match_biquad.hpp"
#include "qwqdsp/filter/biquad.hpp"
#include "qwqdsp/spectral/real_fft.hpp"
#include "qwqdsp/convert.hpp"
#include "slider.hpp"

static float Map(float x, float x_min, float x_max, float target_min, float target_max) noexcept {
    return (x - x_min) / (x_max - x_min) * (target_max - target_min) + target_min;
}

int main() {
    InitWindow(800, 600, "filter");
    SetTargetFPS(30);

    Knob w;
    w.set_bound(0,0,50,50);
    w.set_range(0.0f, 3.14f, 0.001f, 3.14f/2);
    w.set_bg_color(BLACK);
    w.set_fore_color(RAYWHITE);
    w.set_title("w");
    Knob Q;
    Q.set_bound(0,50,50,50);
    Q.set_range(0.1f, 10.0f, 0.1f, 0.707f);
    Q.set_bg_color(BLACK);
    Q.set_fore_color(RAYWHITE);
    Q.set_title("Q");
    Knob db;
    db.set_bound(0,100,50,50);
    db.set_range(-40.0f, 40.0f, 0.1f, 0.0f);
    db.set_bg_color(BLACK);
    db.set_fore_color(RAYWHITE);
    db.set_title("db");

    qwqdsp::spectral::RealFFT fft;
    fft.Init(2048);

    while (!WindowShouldClose()) {
        BeginDrawing();
        ClearBackground(BLACK);

        qwqdsp::filter::Biquad dsp;
        qwqdsp::filter::MatchBiquad design;
        auto coeff = design.Allpass(w.get_value(), Q.get_value());
        dsp = coeff;

        float ir[2048];
        ir[0] = dsp.Tick(1);
        for (size_t i = 1; i < 2048; ++i) {
            ir[i] = dsp.Tick(0);
        }

        float gains[1025];
        fft.FFTGainPhase(ir, gains);

        // draw gains as db in range -100db to 10db
        for (size_t i = 0; i < 1025; ++i) {
            // float gain = qwqdsp::convert::Gain2Db<-100.0f>(gains[i]);
            // float y = Map(gain, -100.0f, 10.0f, 500.0f, 100.0f);
            // float x = static_cast<float>(i) / 1024.0f * 800.0f;
            // DrawLine(x, 500, x, y, RED);
            float w = Map(i, 0, 1024, 0.0f, std::numbers::pi_v<float>);
            auto z = std::polar(1.0f, w);
            z = coeff.DigitalResonpoce(z);
            auto phase = std::arg(z);
            float y = Map(phase, -std::numbers::pi_v<float>, std::numbers::pi_v<float>, 500.0f, 100.0f);
            float x = static_cast<float>(i) / 1024.0f * 800.0f;
            DrawLine(x, 500, x, y, RED);
        }

        qwqdsp::filter::AnalogResponce analog;
        float lastx = 0;
        float lasty = 0;
        float A = std::pow(10.0f, db.get_value() / 40.0f);
        for (size_t i = 0; i < 1025; ++i) {
            float x = static_cast<float>(i) / 1024.0f * 800.0f;
            float wx = static_cast<float>(i) / 1024.0f * 3.14f;

            // float gain = std::abs(analog.AllpassOnepole(wx, w.get_value()));

            // gain = qwqdsp::convert::Gain2Db<-100.0f>(gain);
            // float y = Map(gain, -100.0f, 10.0f, 500.0f, 100.0f);
            float phase = std::arg(analog.Allpass(wx, w.get_value(), Q.get_value()));
            float y = Map(phase, -std::numbers::pi_v<float>, std::numbers::pi_v<float>, 500.0f, 100.0f);
            DrawLine(lastx, lasty, x, y, GREEN);
            lastx = x;
            lasty = y;
        }

        // // draw vertical line at x=w
        {
            float x = Map(w.get_value(), 0.0f, 3.14f, 0.0f, 800.0f);
            DrawLine(x, 600.0f, x, 0.0f, BLUE);
        }
        // x=w
        {
            // float y = Map(db.get_value()/2, -100.0f, 10.0f, 500.0f, 100.0f);
            // DrawLine(0, y, 800.0f, y, BLUE);
        }
        {
            // float y = Map(-db.get_value()/2, -100.0f, 10.0f, 500.0f, 100.0f);
            // DrawLine(0, y, 800.0f, y, BLUE);
        }
        // // x=0
        {
            // float y = Map(db.get_value(), -100.0f, 10.0f, 500.0f, 100.0f);
            // DrawLine(0, y, 800.0f, y, BLUE);
        }
        // x=pi
        // {
        //     float y = Map(0, -100.0f, 10.0f, 500.0f, 100.0f);
        //     DrawLine(0, y, 800.0f, y, BLUE);
        // }
        {
            float y = Map(-std::numbers::pi_v<float>/4, -std::numbers::pi_v<float>, std::numbers::pi_v<float>, 500.0f, 100.0f);
            DrawLine(0, y, 800.0f, y, BLUE);
        }

        Q.display();
        w.display();
        db.display();

        EndDrawing();
    }

    CloseWindow();
}