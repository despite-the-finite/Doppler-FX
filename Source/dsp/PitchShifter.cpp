#include "PitchShifter.h"
#include <juce_core/juce_core.h>
#include <cmath>

void PitchShifter::prepare (double sampleRate, int /*numChannels*/)
{
    sr = sampleRate;
    // ~55 ms of grain. Long enough to keep low notes intact, short enough that
    // the doubling on transients stays subtle.
    window = (float) juce::jmax (256.0, sampleRate * 0.055);

    for (auto& line : lines)
        line.prepare ((int) window + 64);

    reset();
}

void PitchShifter::reset()
{
    for (auto& line : lines)
        line.reset();

    phase = { { 0.0f, 0.0f } };
}

void PitchShifter::setSemitones (float semitones) noexcept
{
    const auto s = juce::jlimit (-24.0f, 24.0f, semitones);
    active = std::abs (s) > 0.01f;

    const auto ratio = std::pow (2.0f, s / 12.0f);
    increment = 1.0f - ratio;   // >0 shifts down, <0 shifts up
}

float PitchShifter::process (int channel, float input) noexcept
{
    auto& line = lines[(size_t) channel];
    line.push (input);

    if (! active)
        return input;

    auto& p = phase[(size_t) channel];
    p += increment;

    // Keep the primary pointer inside one window length.
    while (p >= window) p -= window;
    while (p < 0.0f)    p += window;

    const auto pB = p + window * 0.5f >= window ? p + window * 0.5f - window
                                                : p + window * 0.5f;

    const auto twoPi = juce::MathConstants<float>::twoPi;
    const auto gA = 0.5f - 0.5f * std::cos (twoPi * p  / window);
    const auto gB = 0.5f - 0.5f * std::cos (twoPi * pB / window);

    const auto a = line.read (2.0f + p);
    const auto b = line.read (2.0f + pB);

    return a * gA + b * gB;
}
