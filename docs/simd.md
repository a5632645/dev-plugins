将DspProcessor制成接口，运行时创建算法以及状态  
`idsp.hpp`
```cpp
class Idsp {
public:
    virtual float Tick(float x) = 0;
}
```

`inst.hpp`
```cpp
enum class INST {
    SSE2,
    SSE4
};
```

创建DspProcessor的东西  
`dispatch.cpp`
```cpp
template <INST inst>
Idsp* CreateDsp();

Idsp* Create() {
    if (simd::IsSSE4()) {
        return CreateDsp<INST::SSE4>();
    }
    else {
        return CreateDsp<INST::SSE2>();
    }
}
```

---

`dsp_impl.hpp`
```cpp

#ifdef INST_USE_FLOAT128
#include "simd/simd128.hpp"
#endif

#ifdef INST_USE_SSE2
#include <xxx.h>
#endif

template <INST inst, class SIMDT>
class DspImpl : public Idsp {
public:
    float Tick(float x) override {
        return dl_.Tick(x);
    }
private:
    DelayLine<inst, SIMDT> dl_;
};
```

在对应指令集的文件里可以隔开不同类的布局和具体实现  
`sse2.cpp`
```cpp

#define INST_USE_FLOAT128
#define INST_USE_SSE2

#include "dsp_impl.hpp"

template <>
Idsp* CreateDsp<INST::SSE2>() {
    return new DspImpl<INST::SSE2, simd::Float128>;
}
```
