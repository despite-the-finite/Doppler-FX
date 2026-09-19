#include "ResonantFilter.h"
#include <cmath>

using juce::jlimit;

void ResonantFilter::prepare (double sampleRate, int /*numChannels*/)
{
    sr = sampleRate;
    setCoefficients (12000.0f, 0.2f, Type::lowpass);
    setAirCutoff ((float) sampleRate * 0.5f);
    reset();
}

void ResonantFilter::reset()
{
    for (auto& s : state)
        s = {};
}

void ResonantFilter::setCoefficients (float cutoffHz, float resonance01, Type t) noexcept
{
    type = t;

    const auto nyquist = (float) sr * 0.5f;
    const auto fc      = jlimit (15.0f, nyquist * 0.98f, cutoffHz);

    g = std::tan (juce::MathConstants<float>::pi * fc / (float) sr);

    // Q from a gentle 0.6 up to a singing 18, mapped so the top of the knob
    // is where the character is, not where the trouble is.
    const auto q = 0.6f + jlimit (0.0f, 1.0f, resonance01) * 17.4f;
    k = 1.0f / q;

    a1 = 1.0f / (1.0f + g * (g + k));
    a2 = g * a1;
    a3 = g * a2;

    // Pull the output down as the peak grows: peak gain ~ Q, so sqrt(1/Q)
    // leaves an audible but bounded resonance instead of a runaway one.
    comp = jlimit (0.15f, 1.0f, std::sqrt (0.707f / q));
}

void ResonantFilter::setAirCutoff (float cutoffHz) noexcept
{
    const auto nyquist = (float) sr * 0.5f;
    const auto fc      = jlimit (200.0f, nyquist * 0.98f, cutoffHz);
    airCoeff = 1.0f - std::exp (-juce::MathConstants<float>::twoPi * fc / (float) sr);
    airCoeff = jlimit (0.0f, 1.0f, airCoeff);
}

float ResonantFilter::process (int channel, float x) noexcept
{
    auto& s = state[(size_t) channel];

    const auto v3 = x - s.ic2;
    const auto v1 = a1 * s.ic1 + a2 * v3;
    const auto v2 = s.ic2 + a2 * s.ic1 + a3 * v3;

    s.ic1 = 2.0f * v1 - s.ic1;
    s.ic2 = 2.0f * v2 - s.ic2;

    float out = 0.0f;
    switch (type)
    {
        case Type::bandpass: out = v1 * comp;                        break;
        case Type::highpass: out = (x - k * v1 - v2) * comp;         break;
        case Type::lowpass:
        default:             out = v2 * comp;                        break;
    }

    // Air absorption: a simple one pole, opened up or closed down by distance.
    s.air += airCoeff * (out - s.air);

    // The only recursive state in the wet path, so this is where a stray NaN
    // would otherwise live forever.
    if (! std::isfinite (s.air) || ! std::isfinite (s.ic1) || ! std::isfinite (s.ic2))
    {
        s = {};
        return 0.0f;
    }

    return s.air;
}
