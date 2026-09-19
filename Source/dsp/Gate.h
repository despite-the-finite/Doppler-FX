#pragma once

#include <juce_audio_basics/juce_audio_basics.h>

/*  Stereo-linked gate with a soft knee, plus a distance duck.

    Two jobs in one:
      - a conventional gate, so the long doppler tails do not drag room tone
        and hiss up with them;
      - "duck", which ties gain to how far away the virtual source is, turning
        the motion into a rhythmic chop as the source sweeps past.
*/
class Gate
{
public:
    void prepare (double sampleRate);
    void reset();

    void setEnabled (bool shouldBeEnabled) noexcept   { enabled = shouldBeEnabled; }
    void setThreshold (float dB) noexcept             { thresholdDb = dB; }
    void setDepth (float depth01) noexcept            { depth = juce::jlimit (0.0f, 1.0f, depth01); }
    void setAttack (float ms) noexcept;
    void setRelease (float ms) noexcept;

    /** Computes this sample's gain from the stereo-linked key signal. */
    float processGain (float keyL, float keyR) noexcept;

    /** 0 = source at its closest, 1 = as far as the path goes. */
    static float duckGain (float normalisedDistance, float amount01) noexcept;

    float getCurrentGain() const noexcept { return gain; }

private:
    double sr           = 44100.0;
    bool   enabled      = false;
    float  thresholdDb  = -42.0f;
    float  depth        = 1.0f;
    float  attackCoeff  = 0.5f;
    float  releaseCoeff = 0.999f;
    float  env          = 0.0f;
    float  gain         = 1.0f;
};
