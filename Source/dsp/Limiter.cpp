#include "Limiter.h"
#include <cmath>
#include <algorithm>

using juce::jlimit;
using juce::jmax;

void Limiter::prepare (double sampleRate, int /*numChannels*/, float lookAheadMs)
{
    sr        = sampleRate;
    lookAhead = jmax (8, (int) std::ceil (sampleRate * lookAheadMs * 0.001));

    delayL.assign ((size_t) lookAhead + 1, 0.0f);
    delayR.assign ((size_t) lookAhead + 1, 0.0f);
    minValues.assign ((size_t) lookAhead + 2, 1.0f);
    minIndices.assign ((size_t) lookAhead + 2, 0);

    // Reach the target reduction within the look-ahead window.
    attackCoeff = (float) std::exp (-1.0 / (jmax (1.0, lookAhead * 0.35)));
    setReleaseMs (120.0f);
    setCeilingDb (-0.5f);
    reset();
}

void Limiter::reset()
{
    std::fill (delayL.begin(), delayL.end(), 0.0f);
    std::fill (delayR.begin(), delayR.end(), 0.0f);
    std::fill (minValues.begin(), minValues.end(), 1.0f);

    writePos = 0;
    qHead = qTail = qCount = 0;
    sampleCounter = 0;
    gain = 1.0f;
    minGainSinceRead = 1.0f;
}

void Limiter::setCeilingDb (float dB) noexcept
{
    ceilingLin = juce::Decibels::decibelsToGain (jlimit (-24.0f, 0.0f, dB));
}

void Limiter::setReleaseMs (float ms) noexcept
{
    const auto t = jmax (5.0f, ms) * 0.001f;
    releaseCoeff = (float) std::exp (-1.0 / ((double) t * sr));
}

float Limiter::runningMin (float g) noexcept
{
    const auto capacity = (int) minValues.size();

    // Drop anything at the back that the new value already beats.
    while (qCount > 0)
    {
        const auto backIdx = (qTail - 1 + capacity) % capacity;
        if (minValues[(size_t) backIdx] >= g)
        {
            qTail = backIdx;
            --qCount;
        }
        else break;
    }

    minValues[(size_t) qTail]  = g;
    minIndices[(size_t) qTail] = sampleCounter;
    qTail = (qTail + 1) % capacity;
    ++qCount;

    // Retire anything that has fallen out of the look-ahead window.
    while (qCount > 0 && minIndices[(size_t) qHead] <= sampleCounter - lookAhead)
    {
        qHead = (qHead + 1) % capacity;
        --qCount;
    }

    ++sampleCounter;
    return minValues[(size_t) qHead];
}

void Limiter::processFrame (float& left, float& right) noexcept
{
    // Always run the delay line so that latency stays constant whether or not
    // the limiter is switched on — a changing latency mid-song is far worse
    // than a few samples of it.
    const auto readPos = (writePos + 1) % (int) delayL.size();

    delayL[(size_t) writePos] = left;
    delayR[(size_t) writePos] = right;

    auto outL = delayL[(size_t) readPos];
    auto outR = delayR[(size_t) readPos];

    writePos = readPos;

    if (! enabled)
    {
        // Still a hard backstop: nothing leaves this plugin above 0 dBFS.
        left  = jlimit (-1.0f, 1.0f, outL);
        right = jlimit (-1.0f, 1.0f, outR);
        return;
    }

    const auto peak    = jmax (std::abs (left), std::abs (right));
    const auto desired = peak > ceilingLin ? ceilingLin / peak : 1.0f;
    const auto target  = runningMin (desired);

    const auto coeff = target < gain ? attackCoeff : releaseCoeff;
    gain = target + (gain - target) * coeff;
    gain = jlimit (0.0f, 1.0f, gain);

    minGainSinceRead = std::min (minGainSinceRead, gain);

    outL *= gain;
    outR *= gain;

    // Layer three: clip away the fraction of a dB the smoother leaves behind.
    left  = jlimit (-ceilingLin, ceilingLin, outL);
    right = jlimit (-ceilingLin, ceilingLin, outR);
}
