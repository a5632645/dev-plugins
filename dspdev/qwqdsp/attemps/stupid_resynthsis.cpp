#include <algorithm>
#include <cmath>
#include <cstddef>
#include <array>
#include <format>
#include <numbers>
#include <numeric>
#include "../playing/AudioFile.h"
#include "qwqdsp/convert.hpp"
#include "qwqdsp/interpolation/linear.hpp"
#include "qwqdsp/interpolation/sppchip.hpp"
#include "qwqdsp/spectral/complex_fft.hpp"
#include "qwqdsp/spectral/real_fft.hpp"
#include "qwqdsp/spectral/reassignment.hpp"
#include "qwqdsp/segement/analyze_auto.hpp"
#include "qwqdsp/window/blackman.hpp"
#include "qwqdsp/interpolation/makima.hpp"
#include "qwqdsp/osciilor/table_sine_osc.hpp"

static constexpr size_t kFFTSize = 512;
static constexpr size_t kNumData = qwqdsp::spectral::RealFFT::NumBins(kFFTSize);
struct Frame {
    std::array<float, kNumData> gains;
    std::array<float, kNumData> gain_dbs;
    std::array<float, kNumData> freqs;
    std::array<float, kNumData> formants;
};

static std::vector<Frame> AnalyzeAudio(std::span<const float> x) {
    std::vector<Frame> frames;
    qwqdsp::segement::AnalyzeAuto<true> segement;
    segement.SetSize(kFFTSize);
    segement.SetHop(kFFTSize / 4);
    frames.resize(segement.GetMinFrameSize(x.size()));

    qwqdsp::spectral::Reassignment reass;
    reass.Init(kFFTSize);
    reass.ChangeWindow(
        [](std::span<float> win) {
            qwqdsp::window::Blackman::Window(win, true);
        }
    );
    
    size_t frame_idx = 0;
    segement.Process(x, [&frame_idx, &frames, &reass](std::span<const float> block) {
        reass.Process(block);
        reass.GetFrequency(frames[frame_idx].freqs);
        reass.GetGain(frames[frame_idx].gains);

        std::array<float, kNumData> logs;
        for (size_t i = 0; i < kNumData; ++i) {
            logs[i] = qwqdsp::convert::Gain2Db(frames[frame_idx].gains[i]);
            frames[frame_idx].gain_dbs[i] = logs[i];
        }
        float const max_db = *std::max_element(logs.begin(), logs.end());
        
        if (max_db < qwqdsp::window::Blackman::kSidelobe) {
            std::fill(frames[frame_idx].formants.begin(), frames[frame_idx].formants.end(), -360.0f);
        }
        else {
            std::array<float, kNumData> peaks_pos{};
            std::array<float, kNumData> peaks_db{};
            size_t num_peaks = 0;
            for (size_t i = 1; i < kNumData - 1; ++i) {
                float const prev = logs[i - 1] / max_db;
                float const now = logs[i] / max_db;
                float const next = logs[i + 1] / max_db;
                if (now > prev && now > next) {
                    if (now > qwqdsp::window::Blackman::kSidelobe) {
                        peaks_db[num_peaks] = logs[i];
                        peaks_pos[num_peaks] = i;
                        ++num_peaks;
                    }
                }
            }

            std::array<float, kNumData> peaks_pos2{};
            std::array<float, kNumData> peaks_db2{};
            size_t num_peaks2 = 0;
            for (size_t i = 1; i < num_peaks - 1; ++i) {
                float const prev = peaks_db[i - 1];
                float const now = peaks_db[i];
                float const next = peaks_db[i + 1];
                if (now < prev && now < next) {
                    // 丢弃
                }
                else {
                    peaks_pos2[num_peaks2] = peaks_pos[i];
                    peaks_db2[num_peaks2] = peaks_db[i];
                    ++num_peaks2;
                }
            }
    
            qwqdsp::interpolation::Makima makima;
            makima.Reset({peaks_pos2.data(), num_peaks2}, {peaks_db2.data(), num_peaks2});
            for (size_t i = 0; i < kNumData; ++i) {
                frames[frame_idx].formants[i] = makima.Next(i);
            }
        }


        ++frame_idx;
    });
    return frames;
}

int main() {
    constexpr auto kPath = R"(C:\Users\Kawai\Music\gunge_slice.wav)";
    AudioFile<float> infile;
    infile.load(kPath);
    auto frames = AnalyzeAudio(infile.samples.front());

    AudioFile<float>::AudioBuffer output;
    output.resize(1);
    auto& channel = output.front();
    size_t const num_samples_per_frame = kFFTSize / 4;
    qwqdsp::oscillor::TableSineOsc<> oscs[kNumData];
    for (size_t i = 0; i < frames.size(); ++i) {
        auto& fr = frames[i];
        for (size_t j = 0; j < kNumData; ++j) {
            oscs[j].SetFreq(fr.freqs[j] * std::numbers::pi_v<float> * 2);
            // oscs[j].SetFreq(j * std::numbers::pi_v<float> / kNumData);
        }
        for (size_t j = 0; j < num_samples_per_frame; ++j) {
            float sum = 0.0f;
            for (size_t k = 0; k < kNumData; ++k) {
                sum += oscs[k].Tick() * fr.gains[k];
            }
            channel.push_back(sum * 0.5f);
        }

        std::cout << std::format("frame: {} / {}\n", i, frames.size());
    }

    AudioFile<float> outfile;
    outfile.setAudioBuffer(output);
    outfile.setSampleRate(infile.getSampleRate());
    outfile.setBitDepth(32);
    outfile.save(R"(C:\Users\Kawai\Music\gunge_slice-synth.wav)");
}