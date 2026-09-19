#pragma once

#include "FractionalDelay.h"
#include <array>

/*  Fixed-interval pitch shifter on top of the doppler idea.

    Two read pointers walk through a delay line at the rate that produces the
    wanted interval; because a pointer cannot walk forever, each one is faded
    out and restarted while the other carries the signal. The crossfade window
    is cosine shaped so the two gains always sum to one.

    Completely bypassed at unity, so leaving Pitch at 0 costs nothing and adds
    no smearing.
*/
class PitchShifter
{
public:
    void prepare (double sampleRate, int numChannels);
    void reset();

    /** -12 .. +12 semitones. */
    void setSemitones (float semitones) noexcept;

    bool isActive() const noexcept { return active; }

    float process (int channel, float input) noexcept;

private:
    std::array<FractionalDelay, 2> lines;
    std::array<float, 2>           phase { { 0.0f, 0.0f } };

    double sr        = 44100.0;
    float  window    = 2048.0f;   // crossfade window, samples
    float  increment = 0.0f;      // delay change per sample
    bool   active    = false;
};
