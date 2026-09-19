#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <vector>

/*  Look-ahead brickwall limiter — the safety net on the output.

    The whole point of this plugin is a resonant, feeding-back delay whose
    level swings with a moving source, so the output stage has to be the thing
    you never have to think about. Three layers:

      1. a look-ahead running minimum of the required gain, so reduction starts
         before the peak arrives instead of clipping the front of it;
      2. smoothing, with a slow musical release, so it breathes rather than
         pumps;
      3. a final hard clip exactly at the ceiling, which only ever sees the
         sub-decibel overshoot the smoother leaves behind.

    Latency equals the look-ahead and is reported to the host, so the plugin
    stays sample aligned with the rest of the project.
*/
class Limiter
{
public:
    void prepare (double sampleRate, int numChannels, float lookAheadMs = 4.0f);
    void reset();

    void setCeilingDb (float dB) noexcept;
    void setReleaseMs (float ms) noexcept;
    void setEnabled (bool shouldBeEnabled) noexcept { enabled = shouldBeEnabled; }

    /** Processes one stereo frame in place. */
    void processFrame (float& left, float& right) noexcept;

    int   getLatencySamples() const noexcept { return lookAhead; }

    /** Lowest linear gain applied since the last call; for metering. */
    float readGainReduction() noexcept
    {
        const auto gr = minGainSinceRead;
        minGainSinceRead = 1.0f;
        return gr;
    }

private:
    float runningMin (float g) noexcept;

    std::vector<float> delayL, delayR;
    std::vector<float> minValues;       // monotonic queue, values
    std::vector<int>   minIndices;      // monotonic queue, arrival index
    int                qHead = 0, qTail = 0, qCount = 0;
    int                sampleCounter = 0;

    double sr        = 44100.0;
    int   writePos   = 0;
    int   lookAhead  = 192;
    float ceilingLin = 0.944f;
    float attackCoeff  = 0.0f;
    float releaseCoeff = 0.0f;
    float gain         = 1.0f;
    float minGainSinceRead = 1.0f;
    bool  enabled = true;
};
