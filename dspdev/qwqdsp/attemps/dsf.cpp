#include "qwqdsp/osciilor/dsf_correct.hpp"

/**
 * @ref https://ccrma.stanford.edu/~stilti/papers/blit.pdf
 */
int main() {
    static constexpr float w = 3.0f * std::numbers::pi_v<float> * 2.0f / 1024.0f;
    {
        // figure 15 上面那个
        float test[1024]{};
        qwqdsp::oscillor::DSFCorrect<> dsf;
        dsf.SetAmpFactor(0.99f);
        dsf.SetN(20);
        dsf.SetW0(0.0f);
        dsf.SetWSpace(w);
        for (auto& s : test) {
            s = dsf.Tick();
        }
        std::ignore = test; // 添加一个语句让调试器暂停查看图像
    }
    {
        // figure 15 下面那个
        float test[1024]{};
        qwqdsp::oscillor::DSFCorrect<> dsf;
        dsf.SetAmpFactor(-0.99f);
        dsf.SetN(20);
        dsf.SetW0(0.0f);
        dsf.SetWSpace(w);
        for (auto& s : test) {
            s = dsf.Tick();
        }
        std::ignore = test;
    }
    {
        // figure 16下面那个
        float test[1024]{};
        qwqdsp::oscillor::DSFCorrect<> dsf;
        dsf.SetAmpFactor(0.99f);
        dsf.SetN(20);
        dsf.SetW0(w);
        dsf.SetWSpace(w * 2.0f);
        for (auto& s : test) {
            s = dsf.Tick();
        }
        std::ignore = test;
    }
    {
        // figure 17上面那个
        float test[1024]{};
        qwqdsp::oscillor::DSFCorrectComplex<false> dsf;
        dsf.SetAmpFactor(0.99f, std::numbers::pi_v<float> / 4.0f);
        dsf.SetN(20);
        dsf.SetW0(0.0f);
        dsf.SetWSpace(w);
        for (auto& s : test) {
            s = dsf.Tick();
        }
        std::ignore = test;
    }
    {
        // figure 17 下面那个
        float test[1024]{};
        qwqdsp::oscillor::DSFCorrectComplex<true> dsf;
        dsf.SetAmpFactor(0.99f, std::numbers::pi_v<float> / 4.0f);
        dsf.SetN(20);
        dsf.SetW0(0.0f);
        dsf.SetWSpace(w);
        for (auto& s : test) {
            s = dsf.Tick();
        }
        std::ignore = test;
    }
}