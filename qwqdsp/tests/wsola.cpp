#include <complex>
#include "AudioFile.h"
#include <qwqdsp/segement/mono_reader.hpp>
#include <qwqdsp/window/hann.hpp>
#include <qwqdsp/window/helper.hpp>
#include <qwqdsp/spectral/ipp_real_fft.hpp>

class OlaBuffer {
public:
    void Init(size_t size) {
        size_ = size;
        mask_ = size - 1;
        buffer_.resize(size);
    }

    void Add(float* buffer, float* window, size_t count, size_t hop) {
        size_t pos = ola_add_pos_;
        for (size_t i = 0; i < count; ++i) {
            buffer_[pos++] += buffer[i] * window[i];
            pos &= mask_;
        }
        ola_add_end_ += hop;
        ola_add_pos_ += hop;
        size_ += hop;
        ola_add_end_ &= mask_;
        ola_add_pos_ &= mask_;
    }

    void Read(float* buffer, size_t count) {
        size_t read = std::min(size_, count);
        for (size_t i = 0; i < read; ++i) {
            buffer[i] = buffer_[rpos_];
            buffer_[rpos_] = 0;
            rpos_ = (rpos_ + 1) & mask_;
        }
        size_ -= read;
        std::fill_n(buffer + read, count - read, 0.0f);
    }

    void ReadPushback(std::vector<float>& buffer) {
        for (size_t i = 0; i < size_; ++i) {
            buffer.push_back(buffer_[rpos_]);
            buffer_[rpos_] = 0;
            rpos_ = (rpos_ + 1) & mask_;
        }
        size_ = 0;
    }
private:
    std::vector<float> buffer_;
    size_t ola_add_pos_{};
    size_t ola_add_end_{};
    size_t rpos_{};
    size_t size_{};
    size_t mask_{};
};

int main() {
    AudioFile<float> infile;
    infile.load("../../o.wav");
    // infile.load(R"(C:\Users\Kawai\AppData\Local\Packages\36699Atelier39.3338947B849CD_rsj1xbqhgdnb8\LocalCache\BilibiliDownload\1250357\1\o.wav)");

    auto& data = infile.samples.front();
    constexpr size_t synthsis_block = 1024;
    constexpr size_t overlap_length = 256;
    constexpr size_t synthsis_hop = synthsis_block - overlap_length;
    constexpr float stretch_ratio = 2.0f;
    constexpr size_t analyze_hop = synthsis_hop / stretch_ratio;
    constexpr int search_range = overlap_length * 2;

    std::array<float, synthsis_block> hann_window;
    hann_window.fill(1.0f);
    for (size_t i = 0; i < overlap_length; ++i) {
        float x = static_cast<float>(i) / static_cast<float>(overlap_length);
        hann_window[i] = std::sin(x * std::numbers::pi_v<float> * 0.5f);
        hann_window[i + (synthsis_block - overlap_length)] = std::cos(x * std::numbers::pi_v<float> * 0.5f);
    }

    std::vector<float> output;
    qwqdsp_segement::MonoReader slice{data};
    OlaBuffer ola;
    ola.Init(synthsis_block * 4);

    // init
    std::array<float, synthsis_block> fill_block;
    slice.Read(synthsis_block, fill_block);
    slice.Step(analyze_hop);
    ola.Add(fill_block.data(), hann_window.data(), synthsis_block, synthsis_hop);
    ola.ReadPushback(output);

    while (!slice.IsEnd()) {
        // step
        size_t this_rpos = slice.GetReadpos();

        // find match block
        constexpr size_t search_block_size = search_range + overlap_length;
        std::array<float, search_block_size> search_block;
        int search_begin = this_rpos - search_range / 2;
        if (search_begin < 0) search_begin = 0;
        slice.ReadAbsolute(search_begin, search_block_size, search_block.data());
        std::array<float, overlap_length> match_block;
        std::copy_n(fill_block.data() + (synthsis_block - overlap_length), overlap_length, match_block.data());
        
        // correlation
        float max_corr = 99999999999.0f;
        int best_offset = 0;
        for (size_t i = 0; i < search_range; ++i) {
            float sum = 0;
            for (size_t j = 0; j < overlap_length; ++j) {
                float a = match_block[j];
                float b = search_block[j + i];
                sum += (a - b) * (a - b);
            }
            if (sum < max_corr) {
                max_corr = sum;
                best_offset = i;
            }
        }
        // constexpr size_t search_block_size = search_range + synthsis_block;
        // std::array<float, search_block_size> search_block;
        // int search_begin = this_rpos - search_range / 2;
        // if (search_begin < 0) search_begin = 0;
        // slice.ReadAbsolute(search_begin, search_block_size, search_block.data());
        
        // // correlation
        // float max_corr = 99999999999.0f;
        // int best_offset = 0;
        // for (size_t i = 0; i < synthsis_block; ++i) {
        //     float sum = 0;
        //     for (size_t j = 0; j < overlap_length; ++j) {
        //         float a = fill_block[j];
        //         float b = search_block[j + i];
        //         sum += (a - b) * (a - b);
        //     }
        //     if (sum < max_corr) {
        //         max_corr = sum;
        //         best_offset = i;
        //     }
        // }

        // get
        slice.ReadAbsolute(search_begin + best_offset, synthsis_block, fill_block.data());
        // slice.Read(synthsis_block, fill_block);
        ola.Add(fill_block.data(), hann_window.data(), synthsis_block, synthsis_hop);
        ola.ReadPushback(output);
        slice.Step(analyze_hop);
    }

    AudioFile<float>::AudioBuffer buf;
    buf.push_back(std::move(output));
    infile.setAudioBuffer(buf);
    infile.save("../../test.wav");
}
