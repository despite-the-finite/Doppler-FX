#pragma once

#include <juce_core/juce_core.h>
#include <array>

/*  Topology-preserving state variable filter (Zavalishin), plus the one-pole
    "air" stage that models high frequency loss over distance.

    Resonance is deliberately gain-compensated. A self-oscillating filter is
    the fastest way to turn a tidy mix into a clipped one, so the resonant
    peak is scaled back as Q rises: you still hear the peak, it just cannot
    run away with the track's headroom.
*/
class ResonantFilter
{
public:
    enum class Type { lowpass = 0, bandpass, highpass };

    void prepare (double sampleRate, int numChannels);
    void reset();

    /** Recompute coefficients. Call at control rate, not per sample. */
    void setCoefficients (float cutoffHz, float resonance01, Type type) noexcept;

    /** Air absorption cutoff in Hz; pass sampleRate/2 to disable. */
    void setAirCutoff (float cutoffHz) noexcept;

    float process (int channel, float x) noexcept;

    float getResonanceCompensation() const noexcept { return comp; }

private:
    struct State { float ic1 = 0.0f, ic2 = 0.0f, air = 0.0f; };

    std::array<State, 2> state;

    double sr   = 44100.0;
    float  g    = 0.1f;
    float  k    = 1.41f;
    float  a1   = 1.0f, a2 = 0.0f, a3 = 0.0f;
    float  comp = 1.0f;
    float  airCoeff = 0.0f;
    Type   type = Type::lowpass;
};
