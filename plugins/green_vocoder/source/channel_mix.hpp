#include "juce_audio_basics/juce_audio_basics.h"
#include <array>
#include <cstddef>
#include <span>

static inline void AddInto(std::span<float> block, std::span<float> other) {
    for (size_t i = 0; i < block.size(); ++i) {
        block[i] += other[i];
    }
}

static inline void MixInto(std::span<float> block, const float* ptr1, const float* ptr2, size_t num) {
    for (size_t i = 0; i < num; ++i) {
        block[i] = ptr1[i] + ptr2[i];
    }
}

static inline std::array<std::span<float>, 2> Mix(int modu, int carry, juce::AudioBuffer<float>& buffer) {
    size_t num_samples = static_cast<size_t>(buffer.getNumSamples());
    std::array<std::span<float>, 2> res{
        std::span<float>{buffer.getWritePointer(0), num_samples},
        std::span<float>{buffer.getWritePointer(1), num_samples},
    };
    switch (modu) {
    case 0:
        switch (carry) {
        case 0:
            std::copy(res[0].begin(), res[0].end(), res[1].begin());
            break;
        case 1:
        case 6:
            break;
        case 2:
            AddInto(res[1], res[0]);
            break;
        case 3:
            std::copy_n(buffer.getReadPointer(2), num_samples, res[1].begin());
            break;
        case 4:
            std::copy_n(buffer.getReadPointer(3), num_samples, res[1].begin());
            break;
        case 5:
            MixInto(res[1], buffer.getReadPointer(2), buffer.getReadPointer(3), num_samples);
            break;
        default:
            break;
        }
        break;
    case 1:
        switch (carry) {
        case 0:
            for (size_t i = 0; i < num_samples; ++i) {
                std::swap(res[0][i], res[1][i]);
            }
            break;
        case 1:
        case 6:
            std::copy(res[1].begin(), res[1].end(), res[0].begin());
            break;
            case 2:
            for (size_t i = 0; i < num_samples; ++i) {
                float t = res[0][i];
                res[0][i] = res[1][i];
                res[1][i] += t;
            }
            break;
        case 3:
            std::copy(res[1].begin(), res[1].end(), res[0].begin());
            std::copy_n(buffer.getReadPointer(2), num_samples, res[1].begin());
            break;
        case 4:
            std::copy(res[1].begin(), res[1].end(), res[0].begin());
            std::copy_n(buffer.getReadPointer(3), num_samples, res[1].begin());
            break;
        case 5:
            std::copy(res[1].begin(), res[1].end(), res[0].begin());
            MixInto(res[1], buffer.getReadPointer(2), buffer.getReadPointer(3), num_samples);
            break;
        default:
            break;
        }
        break;
    case 2:
        switch (carry) {
        case 0:
            for (size_t i = 0; i < num_samples; ++i) {
                float t = res[0][i];
                res[0][i] += res[1][i];
                res[1][i] = t;
            }
            break;
        case 1:
        case 6:
            AddInto(res[0], res[1]);
            break;
        case 2:
            for (size_t i = 0; i < num_samples; ++i) {
                float t = res[0][i] + res[1][i];
                res[0][i] = t;
                res[1][i] = t;
            }
            break;
        case 3:
            AddInto(res[0], res[1]);
            std::copy_n(buffer.getReadPointer(2), num_samples, res[1].begin());
            break;
        case 4:
            AddInto(res[0], res[1]);
            std::copy_n(buffer.getReadPointer(3), num_samples, res[1].begin());
            break;
        case 5:
            AddInto(res[0], res[1]);
            MixInto(res[1], buffer.getReadPointer(2), buffer.getReadPointer(3), num_samples);
            break;
        default:
            break;
        }
        break;
    case 3:
        switch (carry) {
        case 0:
            std::copy(res[0].begin(), res[0].end(), res[1].begin());
            std::copy_n(buffer.getReadPointer(2), num_samples, res[0].begin());
            break;
        case 1:
        case 6:
            std::copy_n(buffer.getReadPointer(2), num_samples, res[0].begin());
            break;
        case 2:
            AddInto(res[1], res[0]);
            std::copy_n(buffer.getReadPointer(2), num_samples, res[0].begin());
            break;
        case 3:
            std::copy_n(buffer.getReadPointer(2), num_samples, res[0].begin());
            std::copy_n(buffer.getReadPointer(2), num_samples, res[1].begin());
            break;
        case 4:
            std::copy_n(buffer.getReadPointer(2), num_samples, res[0].begin());
            std::copy_n(buffer.getReadPointer(3), num_samples, res[1].begin());
            break;
        case 5:
            std::copy_n(buffer.getReadPointer(2), num_samples, res[0].begin());
            MixInto(res[1], buffer.getReadPointer(2), buffer.getReadPointer(3), num_samples);
            break;
        default:
            break;
        }
        break;
    case 4:
        switch (carry) {
        case 0:
            std::copy(res[0].begin(), res[0].end(), res[1].begin());
            std::copy_n(buffer.getReadPointer(3), num_samples, res[0].begin());
            break;
        case 1:
        case 6:
            std::copy_n(buffer.getReadPointer(3), num_samples, res[0].begin());
            break;
        case 2:
            AddInto(res[1], res[0]);
            std::copy_n(buffer.getReadPointer(3), num_samples, res[0].begin());
            break;
        case 3:
            std::copy_n(buffer.getReadPointer(2), num_samples, res[1].begin());
            std::copy_n(buffer.getReadPointer(3), num_samples, res[0].begin());
            break;
        case 4:
            std::copy_n(buffer.getReadPointer(3), num_samples, res[1].begin());
            std::copy_n(buffer.getReadPointer(3), num_samples, res[0].begin());
            break;
        case 5:
            MixInto(res[1], buffer.getReadPointer(2), buffer.getReadPointer(3), num_samples);
            std::copy_n(buffer.getReadPointer(3), num_samples, res[0].begin());
            break;
        default:
            break;
        }
        break;
    case 5:
        switch (carry) {
        case 0:
            std::copy(res[0].begin(), res[0].end(), res[1].begin());
            MixInto(res[0], buffer.getReadPointer(2), buffer.getReadPointer(3), num_samples);
            break;
        case 1:
        case 6:
            MixInto(res[0], buffer.getReadPointer(2), buffer.getReadPointer(3), num_samples);
            break;
        case 2:
            AddInto(res[1], res[0]);
            MixInto(res[0], buffer.getReadPointer(2), buffer.getReadPointer(3), num_samples);
            break;
        case 3:
            std::copy_n(buffer.getReadPointer(2), num_samples, res[1].begin());
            MixInto(res[0], buffer.getReadPointer(2), buffer.getReadPointer(3), num_samples);
            break;
        case 4:
            std::copy_n(buffer.getReadPointer(3), num_samples, res[1].begin());
            MixInto(res[0], buffer.getReadPointer(2), buffer.getReadPointer(3), num_samples);
            break;
        case 5:
            MixInto(res[1], buffer.getReadPointer(2), buffer.getReadPointer(3), num_samples);
            std::copy(res[1].begin(), res[1].end(), res[0].begin());
            break;
        default:
            break;
        }
        break;
    }
    return res;
}
