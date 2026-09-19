#pragma once

#include <juce_core/juce_core.h>
#include <vector>
#include <cmath>

/*  Circular delay line with 4-point 3rd-order Hermite interpolation.

    The doppler engine sweeps the read position continuously, so the
    interpolator matters: linear interpolation acts as a moving low pass and
    audibly dulls the effect as the delay modulates. Hermite is the usual
    sweet spot between cost and artefacts for this job.
*/
class FractionalDelay
{
public:
    void prepare (int maxDelaySamples)
    {
        // +4 guard samples for the interpolator's neighbourhood.
        size = juce::jmax (8, maxDelaySamples + 4);
        buffer.assign ((size_t) size, 0.0f);
        writeIndex = 0;
    }

    void reset() noexcept
    {
        std::fill (buffer.begin(), buffer.end(), 0.0f);
        writeIndex = 0;
    }

    /** Writes one sample and advances the line by one sample. */
    inline void push (float x) noexcept
    {
        buffer[(size_t) writeIndex] = x;
        if (++writeIndex >= size)
            writeIndex = 0;
    }

    /** Reads `delaySamples` back from the most recent write. */
    inline float read (float delaySamples) const noexcept
    {
        const auto maxDelay = (float) (size - 4);
        const auto d        = juce::jlimit (1.0f, maxDelay, delaySamples);

        // Most recent sample sits at writeIndex - 1.
        float readPos = (float) writeIndex - 1.0f - d;
        while (readPos < 0.0f)
            readPos += (float) size;

        const auto  i0 = (int) readPos;
        const auto  f  = readPos - (float) i0;

        const auto sm1 = at (i0 - 1);
        const auto s0  = at (i0);
        const auto s1  = at (i0 + 1);
        const auto s2  = at (i0 + 2);

        // Hermite / Catmull-Rom form.
        const auto c0 = s0;
        const auto c1 = 0.5f * (s1 - sm1);
        const auto c2 = sm1 - 2.5f * s0 + 2.0f * s1 - 0.5f * s2;
        const auto c3 = 0.5f * (s2 - sm1) + 1.5f * (s0 - s1);

        return ((c3 * f + c2) * f + c1) * f + c0;
    }

    int getSize() const noexcept { return size; }

private:
    inline float at (int index) const noexcept
    {
        index %= size;
        if (index < 0)
            index += size;
        return buffer[(size_t) index];
    }

    std::vector<float> buffer;
    int                size       = 0;
    int                writeIndex = 0;
};
