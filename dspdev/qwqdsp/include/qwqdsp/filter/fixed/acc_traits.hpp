#pragma once
#include <cstdint>

namespace qwqdsp::filter::fixed {
template<class T>
struct AccTypeTrait {
    using Type = void;
};
template<>
struct AccTypeTrait<int8_t> {
    using Type = int16_t;
};
template<>
struct AccTypeTrait<int16_t> {
    using Type = int32_t;
};
template<>
struct AccTypeTrait<int32_t> {
    using Type = int64_t;
};
template<class T>
using AccType = AccTypeTrait<T>::Type;
}