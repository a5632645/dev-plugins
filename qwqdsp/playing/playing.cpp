#include "qwqdsp/filter/fast_set_iir_paralle.hpp"

int main() {
    qwqdsp::filter::FastSetIirParalle<qwqdsp::filter::fastset_coeff::Order2_1e2> filter;
    filter.Make(100);

    float ir[1024]{};
    ir[0] = filter.Tick(1);
    for (size_t i = 1; i < 1024; ++i) {
        ir[i] = filter.Tick(1);
    }
}