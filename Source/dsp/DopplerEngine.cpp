#include "DopplerEngine.h"
#include <cmath>

using juce::jlimit;
using juce::jmax;
using juce::jmin;

namespace
{
    // Never let the read pointer approach the write pointer faster than this.
    // 0.75 samples per sample caps the pitch swing at roughly +/- 2 octaves and
    // guarantees the interpolator always has valid history to work with.
    constexpr float maxDelaySlew = 0.75f;
    constexpr float minDistance  = 0.15f;   // metres; stops 1/d blowing up
}

void DopplerEngine::prepare (double sampleRate)
{
    sr = sampleRate;

    const auto maxDelay = (int) std::ceil (maxPathMetres / speedOfSound * sampleRate) + 8;
    for (auto& line : lines)
        line.prepare (maxDelay);

    reset();
}

void DopplerEngine::reset()
{
    for (auto& line : lines)
        line.reset();

    phase  = 0.0f;
    primed = false;
    smoothedDelay = { { 0.0f, 0.0f } };
    smoothedGain  = { { 1.0f, 1.0f } };
    lastDistance  = jmax (minDistance, settings.distance);
}

void DopplerEngine::positionAt (const Settings& settings, float p, float& x, float& y) noexcept
{
    const auto dist = jmax (minDistance, settings.distance);
    const auto path = jmax (0.5f, settings.pathLength);

    switch (settings.mode)
    {
        case 1:     // Orbit: a circle whose nearest point is `distance` away.
        {
            const auto radius = jlimit (0.1f, maxPathMetres * 0.4f, path * 0.125f);
            const auto theta  = juce::MathConstants<float>::twoPi * p;
            x = radius * std::sin (theta);
            y = dist + radius - radius * std::cos (theta);
            break;
        }

        case 2:     // Pendulum: sinusoidal sweep, so it eases at the turnarounds.
            x = std::sin (juce::MathConstants<float>::twoPi * p) * path * 0.5f;
            y = dist;
            break;

        case 3:     // Manual: position is driven by the host / user.
            x = (jlimit (0.0f, 1.0f, settings.manualPos) - 0.5f) * path;
            y = dist;
            break;

        case 0:     // Flyby: straight line, constant speed, left to right.
        default:
            x = (p - 0.5f) * path;
            y = dist;
            break;
    }
}

DopplerEngine::Frame DopplerEngine::tick (float phaseIncrement) noexcept
{
    Frame frame;

    if (settings.mode != 3)                 // Manual mode ignores the rate
    {
        phase += phaseIncrement;
        if (phase >= 1.0f) phase -= std::floor (phase);
        if (phase < 0.0f)  phase += 1.0f - std::floor (phase);
    }

    float x = 0.0f, y = 0.0f;
    positionAt (settings, settings.mode == 3 ? settings.manualPos : phase, x, y);

    frame.sourceX = x;
    frame.sourceY = y;

    const auto earHalf   = 0.02f + jlimit (0.0f, 1.0f, settings.spread) * 1.4f;
    const auto refDist   = jmax (minDistance, settings.distance);
    const auto baseDelay = (float) (refDist / speedOfSound * sr);

    const auto centreDist = jmax (minDistance, std::sqrt (x * x + y * y));
    frame.distance = centreDist;
    frame.radialVelocity = (centreDist - lastDistance) * (float) sr;
    lastDistance = centreDist;

    for (int ch = 0; ch < 2; ++ch)
    {
        const auto ex   = x + (ch == 0 ? earHalf : -earHalf);   // left ear sits at -earHalf
        const auto dist = jmax (minDistance, std::sqrt (ex * ex + y * y));

        // Propagation delay, scaled around the resting delay by the Doppler amount.
        const auto rawDelay = (float) (dist / speedOfSound * sr);
        auto target = baseDelay + (rawDelay - baseDelay) * settings.depth;
        target = jlimit (1.0f, (float) (lines[(size_t) ch].getSize() - 8), target);

        // Inverse distance law, normalised so the closest approach is unity:
        // the effect can duck below the dry signal but never boosts above it.
        const auto rawGain = jlimit (0.02f, 1.0f, refDist / dist);
        const auto gTarget = 1.0f + (rawGain - 1.0f) * jlimit (0.0f, 1.0f, settings.proximity);

        if (! primed)
        {
            smoothedDelay[(size_t) ch] = target;
            smoothedGain[(size_t) ch]  = gTarget;
        }
        else
        {
            const auto diff = target - smoothedDelay[(size_t) ch];
            smoothedDelay[(size_t) ch] += jlimit (-maxDelaySlew, maxDelaySlew, diff);

            // ~5 ms one pole on level, enough to keep fast automation click free.
            const auto gCoeff = (float) std::exp (-1.0 / (0.005 * sr));
            smoothedGain[(size_t) ch] = gTarget + (smoothedGain[(size_t) ch] - gTarget) * gCoeff;
        }

        frame.delaySamples[(size_t) ch] = smoothedDelay[(size_t) ch];
        frame.gain[(size_t) ch]         = smoothedGain[(size_t) ch];
    }

    primed = true;
    return frame;
}
