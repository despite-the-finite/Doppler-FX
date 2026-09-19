#pragma once

#include "FractionalDelay.h"
#include <juce_core/juce_core.h>
#include <array>

/*  Geometric doppler model.

    A virtual sound source travels a path around a listener whose two ears sit
    a settable distance apart. For each ear we work out how far away the source
    is, and turn that distance into

        - a propagation delay  (distance / speed of sound)  -> pitch shift,
          because a delay that shortens replays the signal faster,
        - a level              (inverse distance law)       -> the swell as it
          approaches and the fall as it leaves,
        - a high frequency roll off                         -> air absorption,

    which is the whole of the effect: we never shift pitch explicitly here, it
    falls out of the moving read pointer exactly as it does in the real world.
*/
class DopplerEngine
{
public:
    static constexpr float speedOfSound = 343.0f;   // m/s, dry air at 20 C
    static constexpr float maxPathMetres = 260.0f;  // sets the delay buffer size

    struct Settings
    {
        int   mode        = 0;      // MotionMode
        float distance    = 4.0f;   // closest approach / orbit radius, metres
        float pathLength  = 40.0f;  // metres travelled across a flyby
        float depth       = 1.0f;   // 0..2, scales the delay swing
        float spread      = 0.55f;  // 0..1, ear separation
        float proximity   = 0.6f;   // 0..1, how much inverse-distance level
        float manualPos   = 0.5f;   // 0..1, used by Manual mode
    };

    struct Frame
    {
        std::array<float, 2> delaySamples { { 0.0f, 0.0f } };
        std::array<float, 2> gain         { { 1.0f, 1.0f } };
        float sourceX = 0.0f;       // metres, +right
        float sourceY = 0.0f;       // metres, +in front
        float distance = 1.0f;      // metres, to the head centre
        float radialVelocity = 0.0f;// m/s, + means receding
    };

    void prepare (double sampleRate);
    void reset();

    void setSettings (const Settings& s) noexcept { settings = s; }

    /** Advances the source along its path by one sample and reports the result. */
    Frame tick (float phaseIncrement) noexcept;

    /** Reads the two delay lines; call after pushing this sample's input. */
    inline float read (int channel, float delaySamples) noexcept
    {
        return lines[(size_t) channel].read (delaySamples);
    }

    inline void push (int channel, float x) noexcept
    {
        lines[(size_t) channel].push (x);
    }

    float getPhase() const noexcept { return phase; }
    void  setPhase (float p) noexcept { phase = p - std::floor (p); }

    /** Where the source sits at a given phase. Shared with the UI, which draws
        the whole route from it. */
    static void positionAt (const Settings&, float phase, float& x, float& y) noexcept;

private:

    Settings settings;
    std::array<FractionalDelay, 2> lines;

    double sr        = 44100.0;
    float  phase     = 0.0f;
    // Slew-limited delay per ear, so the read pointer can never outrun the
    // write pointer no matter how violently the motion parameters are automated.
    std::array<float, 2> smoothedDelay { { 0.0f, 0.0f } };
    std::array<float, 2> smoothedGain  { { 1.0f, 1.0f } };
    float lastDistance = 1.0f;
    bool  primed       = false;
};
