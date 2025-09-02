#pragma once
#include <chrono>

namespace qwqdsp {

class Performance {
public:
    struct PerformanceObj {
        int& target;
        std::chrono::time_point<std::chrono::high_resolution_clock> begin;

        PerformanceObj(int& v) noexcept
            : target(v)
            , begin(std::chrono::high_resolution_clock::now())
        {}

        ~PerformanceObj() noexcept {
            auto end = std::chrono::high_resolution_clock::now();
            target = std::chrono::duration_cast<std::chrono::microseconds>(end - begin).count();
        }
    };

    [[nodiscard]]
    PerformanceObj Count() noexcept { return PerformanceObj(process_us_); }

    int GetProcessUs() const noexcept { return process_us_; }
private:
    int process_us_{};
};

}