#pragma once
#include <array>
#include <complex>

template<typename Sample>
struct EllipticBlepCoeffs {
    static constexpr size_t complexCount = 5;
    static constexpr int realCount = 1;
    static constexpr Sample passbandFreq = 20000;
    static constexpr Sample stopbandFreq = 24000;
    static constexpr std::array<std::complex<Sample>, complexCount> complexPoles{{
        {Sample(-5561.982558545558), Sample(7721.564144482831)},
        {Sample(-3936.7227375705806), Sample(13650.197801811304)},
        {Sample(-2348.139919173165), Sample(17360.271619482097)},
        {Sample(-1177.5927594523114), Sample(19350.80475283362)},
        {Sample(-351.83634005467775), Sample(20192.238514865294)}
    }};
    static constexpr std::array<Sample, realCount> realPoles{{
        Sample(-6297.9970566057245)
    }};
    // Coeffs for direct bandlimited synthesis of a polynomial-segment waveform
    static constexpr std::array<std::complex<Sample>, complexCount> complexCoeffsDirect{{
        {Sample(-16437.98665421065), Sample(-7224.765894589879)},
        {Sample(7790.757942544076), Sample(9526.642450543113)},
        {Sample(-840.5346697125525), Sample(-6786.810488747387)},
        {Sample(-1524.1587677384439), Sample(2562.99591951713)},
        {Sample(754.6036883901486), Sample(-311.82754418710147)}
    }};
    static constexpr std::array<Sample, realCount> realCoeffsDirect{{
        Sample(10260.028875848706)
    }};
    // Coeffs for cancelling the aliasing from discontinuities in an existing waveform
    static constexpr std::array<std::complex<Sample>, complexCount> complexCoeffsBlep{{
        {Sample(-16437.98665421065), Sample(-7224.765894589879)},
        {Sample(7790.757942544076), Sample(9526.642450543113)},
        {Sample(-840.5346697125525), Sample(-6786.810488747387)},
        {Sample(-1524.1587677384439), Sample(2562.99591951713)},
        {Sample(754.6036883901486), Sample(-311.82754418710147)}
    }};
    static constexpr std::array<Sample, realCount> realCoeffsBlep{{
        Sample(10300.028875848706)
    }};
    // Coeffs for adding an impulse in "residue" (not direct) mode
    static constexpr std::array<std::complex<Sample>, complexCount> complexCoeffsImpulse{{
        {Sample(-16437.98665421065), Sample(-7224.765894589879)},
        {Sample(7790.757942544076), Sample(9526.642450543113)},
        {Sample(-840.5346697125525), Sample(-6786.810488747387)},
        {Sample(-1524.1587677384439), Sample(2562.99591951713)},
        {Sample(754.6036883901486), Sample(-311.82754418710147)}
    }};
    static constexpr std::array<Sample, realCount> realCoeffsImpulse{{
        Sample(10260.028875848706)
    }};
    // Allpass to make the phase approximately linear
    static constexpr int allpassLinearDelay = 14;
    static constexpr int allpassOrder = 10;
    static constexpr std::array<Sample, allpassOrder> allpassCoeffs{{
        Sample(-1.1393338099049124),
        Sample(0.8686449142371376),
        Sample(-0.5359511474432548),
        Sample(0.27694149157086057),
        Sample(-0.1167070988511846),
        Sample(0.03940919592349818),
        Sample(-0.010823523622726386),
        Sample(0.002370885084537395),
        Sample(-0.0003407743471887175),
        Sample(2.269852418006042e-05)
    }};
};