#include "qwqdsp/misc/ampd_peak.hpp"
#include "qwqdsp/osciilor/noise.hpp"
#include <cmath>
#include <numbers>

int main() {
    qwqdsp::misc::AMPDPeakFinding ampd;
    qwqdsp::oscillor::WhiteNoise noise;

    float test[1000]{};
    for (size_t i = 0; i < 1000; ++i) {
        test[i] = 0;
    }

    ampd.Init(1000);
    auto& v = ampd.Process<float>(test);
}