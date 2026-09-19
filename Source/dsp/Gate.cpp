#include "Gate.h"
#include <cmath>

using juce::jlimit;

namespace
{
    inline float coeffFor (float milliseconds, double sr)
    {
        const auto t = juce::jmax (0.02f, milliseconds) * 0.001f;
        return (float) std::exp (-1.0 / (t * sr));
    }
}

void Gate::prepare (double sampleRate)
{
    sr = sampleRate;
    setAttack (2.0f);
    setRelease (160.0f);
    reset();
}

void Gate::reset()
{
    env  = 0.0f;
    gain = 1.0f;
}

void Gate::setAttack (float ms) noexcept   { attackCoeff  = coeffFor (ms, sr); }
void Gate::setRelease (float ms) noexcept  { releaseCoeff = coeffFor (ms, sr); }

float Gate::processGain (float keyL, float keyR) noexcept
{
    if (! enabled)
    {
        // Glide back to unity rather than jumping when the gate is switched off.
        gain += (1.0f - gain) * 0.001f;
        return gain > 0.9995f ? (gain = 1.0f) : gain;
    }

    const auto key = juce::jmax (std::abs (keyL), std::abs (keyR));

    // Peak follower. The detector attack is deliberately fixed and fast: if it
    // used the Attack knob, a slow attack would stop the gate ever noticing a
    // short transient in the first place.
    const auto detectorAttack = coeffFor (0.3f, sr);
    if (key > env) env = key + (env - key) * detectorAttack;
    else           env = key + (env - key) * releaseCoeff;

    const auto envDb  = juce::Decibels::gainToDecibels (env, -100.0f);
    const auto knee   = 6.0f;                       // dB, soft knee width
    const auto over   = envDb - thresholdDb;

    float openness;                                 // 0 = shut, 1 = open
    if (over >= knee * 0.5f)        openness = 1.0f;
    else if (over <= -knee * 0.5f)  openness = 0.0f;
    else                            openness = (over + knee * 0.5f) / knee;

    const auto target = 1.0f - depth * (1.0f - openness);

    const auto coeff = target < gain ? releaseCoeff : attackCoeff;
    gain = target + (gain - target) * coeff;

    return jlimit (0.0f, 1.0f, gain);
}

float Gate::duckGain (float normalisedDistance, float amount01) noexcept
{
    const auto d = jlimit (0.0f, 1.0f, normalisedDistance);
    const auto a = jlimit (0.0f, 1.0f, amount01);

    // Squared falloff reads as a sharper chop than a straight line.
    const auto duck = 1.0f - d * d;
    return 1.0f + (duck - 1.0f) * a;
}
