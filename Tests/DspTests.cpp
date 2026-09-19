/*  Headless checks for the parts of Doppler FX that are easy to get wrong and
    hard to hear: the output never leaving the ceiling, the feedback path
    settling instead of running away, the geometry actually producing a pitch
    shift, and nothing in the chain latching to NaN.

    Build and run:
        cmake --build build --target DopplerFXTests
        ./build/DopplerFXTests_artefacts/Release/DopplerFXTests
*/

#include "../Source/PluginProcessor.h"
#include "../Source/Presets.h"

#include <cmath>
#include <cstdio>
#include <random>
#include <string>

namespace
{
    int failures = 0;
    int checks   = 0;

    void check (bool condition, const std::string& what, const std::string& detail = {})
    {
        ++checks;
        if (condition)
        {
            std::printf ("  pass  %s\n", what.c_str());
        }
        else
        {
            ++failures;
            std::printf ("  FAIL  %s%s%s\n", what.c_str(),
                         detail.empty() ? "" : "  -- ", detail.c_str());
        }
    }

    void setParam (DopplerFXAudioProcessor& p, const char* id, float valueInRealUnits)
    {
        auto* param = p.getState().getParameter (id);
        jassert (param != nullptr);
        param->setValueNotifyingHost (param->convertTo0to1 (valueInRealUnits));
    }

    constexpr double sampleRate = 48000.0;
    constexpr int    blockSize  = 256;

    struct RunResult
    {
        float maxAbs  = 0.0f;
        bool  finite  = true;
        float rmsTail = 0.0f;     // RMS over the final second
    };

    /** Runs `seconds` of the given generator through the processor. */
    template <typename Generator>
    RunResult run (DopplerFXAudioProcessor& p, double seconds, Generator&& gen)
    {
        const auto totalSamples = (int) (seconds * sampleRate);
        const auto tailStart    = totalSamples - (int) sampleRate;

        juce::AudioBuffer<float> buffer (2, blockSize);
        juce::MidiBuffer midi;

        RunResult result;
        double tailSum = 0.0;
        int    tailCount = 0;
        int    n = 0;

        while (n < totalSamples)
        {
            const auto num = juce::jmin (blockSize, totalSamples - n);
            buffer.setSize (2, num, false, false, true);

            for (int i = 0; i < num; ++i)
            {
                const auto s = gen (n + i);
                buffer.setSample (0, i, s);
                buffer.setSample (1, i, s);
            }

            p.processBlock (buffer, midi);

            for (int ch = 0; ch < 2; ++ch)
                for (int i = 0; i < num; ++i)
                {
                    const auto s = buffer.getSample (ch, i);
                    if (! std::isfinite (s))
                        result.finite = false;
                    else
                        result.maxAbs = juce::jmax (result.maxAbs, std::abs (s));

                    if (n + i >= tailStart && std::isfinite (s))
                    {
                        tailSum += (double) s * s;
                        ++tailCount;
                    }
                }

            n += num;
        }

        result.rmsTail = tailCount > 0 ? (float) std::sqrt (tailSum / tailCount) : 0.0f;
        return result;
    }

    /** Crude but adequate pitch estimate: zero crossings of a single sine. */
    float estimateFrequency (const std::vector<float>& x)
    {
        int crossings = 0;
        for (size_t i = 1; i < x.size(); ++i)
            if ((x[i - 1] < 0.0f) != (x[i] < 0.0f))
                ++crossings;

        const auto seconds = (double) x.size() / sampleRate;
        return (float) (crossings / (2.0 * seconds));
    }

    /** Captures the processor's output over a window, feeding it a steady sine. */
    std::vector<float> capture (DopplerFXAudioProcessor& p, double freq,
                                double startSeconds, double windowSeconds)
    {
        const auto total = (int) ((startSeconds + windowSeconds) * sampleRate);
        const auto from  = (int) (startSeconds * sampleRate);

        juce::AudioBuffer<float> buffer (2, blockSize);
        juce::MidiBuffer midi;
        std::vector<float> out;

        for (int n = 0; n < total; )
        {
            const auto num = juce::jmin (blockSize, total - n);
            buffer.setSize (2, num, false, false, true);

            for (int i = 0; i < num; ++i)
            {
                const auto s = (float) std::sin (juce::MathConstants<double>::twoPi
                                                   * freq * (n + i) / sampleRate);
                buffer.setSample (0, i, s);
                buffer.setSample (1, i, s);
            }

            p.processBlock (buffer, midi);

            for (int i = 0; i < num; ++i)
                if (n + i >= from)
                    out.push_back (buffer.getSample (0, i));

            n += num;
        }

        return out;
    }

    /** A clean starting point: wet only, no filtering, no feedback, no gate. */
    void neutralPreset (DopplerFXAudioProcessor& p)
    {
        setParam (p, ParamID::mix,         100.0f);
        setParam (p, ParamID::outputGain,    0.0f);
        setParam (p, ParamID::ceiling,      -0.5f);
        setParam (p, ParamID::safety,        1.0f);
        setParam (p, ParamID::feedback,      0.0f);
        setParam (p, ParamID::resonance,     0.0f);
        setParam (p, ParamID::cutoff,    20000.0f);
        setParam (p, ParamID::filterTrack,   0.0f);
        setParam (p, ParamID::airDamp,       0.0f);
        setParam (p, ParamID::proximity,     0.0f);
        setParam (p, ParamID::gateOn,        0.0f);
        setParam (p, ParamID::gateDuck,      0.0f);
        setParam (p, ParamID::pitchSemis,    0.0f);
        setParam (p, ParamID::tempoSync,     0.0f);
        setParam (p, ParamID::spread,        0.0f);
    }
}

/*  Renders the editor straight into an image, with no window and no display
    server, so the interface can be eyeballed on a build machine.
*/
static int writeScreenshot (const juce::String& path)
{
    juce::ScopedJuceInitialiser_GUI juceInit;

    DopplerFXAudioProcessor processor;
    processor.prepareToPlay (48000.0, 512);

    // A loaded preset makes for a more representative shot than defaults.
    processor.setCurrentProgram (10);   // Resonance Cascade

    std::unique_ptr<juce::AudioProcessorEditor> editor (processor.createEditor());
    if (editor == nullptr)
        return 1;

    // A little motion so the display has something to show.
    processor.visuals.sourceX.store (-6.0f);
    processor.visuals.sourceY.store (7.5f);
    processor.visuals.distance.store (9.6f);
    processor.visuals.radialVelocity.store (-11.0f);
    processor.visuals.extent.store (22.0f);

    editor->setSize (980, 780);

    juce::Image image (juce::Image::ARGB, editor->getWidth(), editor->getHeight(), true);
    {
        juce::Graphics g (image);
        editor->paintEntireComponent (g, true);
    }

    juce::File out (path);
    out.deleteFile();
    juce::FileOutputStream stream (out);
    if (! stream.openedOk())
        return 1;

    juce::PNGImageFormat png;
    return png.writeImageToStream (image, stream) ? 0 : 1;
}

int main (int argc, char** argv)
{
    if (argc > 2 && juce::String (argv[1]) == "--screenshot")
        return writeScreenshot (juce::String (juce::CharPointer_UTF8 (argv[2])));

    juce::ScopedJuceInitialiser_GUI juceInit;

    std::printf ("Doppler FX - DSP checks @ %.0f Hz\n\n", sampleRate);

    const auto ceilingLin = juce::Decibels::decibelsToGain (-0.5f);
    const auto tolerance  = 1.0e-4f;    // allow for float rounding in the clip stage

    // -----------------------------------------------------------------------
    std::printf ("Output ceiling is never exceeded\n");
    {
        DopplerFXAudioProcessor p;
        p.prepareToPlay (sampleRate, blockSize);
        neutralPreset (p);

        // Everything that can add level, turned up.
        setParam (p, ParamID::feedback,     95.0f);
        setParam (p, ParamID::resonance,   100.0f);
        setParam (p, ParamID::dopplerAmt,  200.0f);
        setParam (p, ParamID::proximity,   100.0f);
        setParam (p, ParamID::outputGain,   12.0f);
        setParam (p, ParamID::cutoff,      400.0f);
        setParam (p, ParamID::rateHz,        8.0f);
        setParam (p, ParamID::distance,      0.5f);
        setParam (p, ParamID::pathLength,  200.0f);

        std::mt19937 rng (1234);
        std::uniform_real_distribution<float> noise (-1.0f, 1.0f);

        const auto r = run (p, 12.0, [&] (int) { return noise (rng); });

        check (r.finite, "output stays finite under full-scale noise and maximum feedback");
        check (r.maxAbs <= ceilingLin + tolerance,
               "peak stays at or below the -0.5 dB ceiling",
               "peak was " + std::to_string (juce::Decibels::gainToDecibels (r.maxAbs)) + " dB");
    }

    // -----------------------------------------------------------------------
    std::printf ("\nHard backstop with the safety limiter switched off\n");
    {
        DopplerFXAudioProcessor p;
        p.prepareToPlay (sampleRate, blockSize);
        neutralPreset (p);

        setParam (p, ParamID::safety,       0.0f);
        setParam (p, ParamID::feedback,    95.0f);
        setParam (p, ParamID::resonance,  100.0f);
        setParam (p, ParamID::outputGain,  12.0f);
        setParam (p, ParamID::cutoff,     300.0f);

        std::mt19937 rng (99);
        std::uniform_real_distribution<float> noise (-1.0f, 1.0f);

        const auto r = run (p, 8.0, [&] (int) { return noise (rng); });

        check (r.finite, "output stays finite with the limiter bypassed");
        check (r.maxAbs <= 1.0f + tolerance, "nothing leaves the plugin above 0 dBFS even bypassed",
               "peak was " + std::to_string (r.maxAbs));
    }

    // -----------------------------------------------------------------------
    std::printf ("\nFeedback settles instead of running away\n");
    {
        DopplerFXAudioProcessor p;
        p.prepareToPlay (sampleRate, blockSize);
        neutralPreset (p);

        setParam (p, ParamID::feedback,    95.0f);
        setParam (p, ParamID::resonance,  100.0f);
        setParam (p, ParamID::cutoff,     500.0f);
        setParam (p, ParamID::rateHz,       2.0f);

        // A single second of signal, then eleven seconds of silence.
        const auto r = run (p, 12.0, [] (int n)
        {
            return n < (int) sampleRate
                     ? (float) std::sin (juce::MathConstants<double>::twoPi * 110.0 * n / sampleRate)
                     : 0.0f;
        });

        check (r.finite, "feedback path stays finite");
        check (r.rmsTail < 0.2f, "tail decays rather than self-oscillating",
               "tail RMS was " + std::to_string (r.rmsTail));
    }

    // -----------------------------------------------------------------------
    std::printf ("\nMotion produces a real pitch shift\n");
    {
        DopplerFXAudioProcessor p;
        p.prepareToPlay (sampleRate, blockSize);
        neutralPreset (p);

        setParam (p, ParamID::motionMode,   0.0f);   // Flyby
        setParam (p, ParamID::dopplerAmt, 100.0f);
        setParam (p, ParamID::rateHz,       0.5f);   // one pass every 2 s
        setParam (p, ParamID::distance,     5.0f);
        setParam (p, ParamID::pathLength,  60.0f);

        // Approach: phase 0.15..0.25. Recede: phase 0.75..0.85.
        const auto approaching = estimateFrequency (capture (p, 1000.0, 0.30, 0.20));

        DopplerFXAudioProcessor q;
        q.prepareToPlay (sampleRate, blockSize);
        neutralPreset (q);
        setParam (q, ParamID::motionMode,   0.0f);
        setParam (q, ParamID::dopplerAmt, 100.0f);
        setParam (q, ParamID::rateHz,       0.5f);
        setParam (q, ParamID::distance,     5.0f);
        setParam (q, ParamID::pathLength,  60.0f);

        const auto receding = estimateFrequency (capture (q, 1000.0, 1.50, 0.20));

        std::printf ("        approaching %.1f Hz, receding %.1f Hz (source 1000 Hz)\n",
                     approaching, receding);

        check (approaching > 1005.0f, "pitch rises as the source approaches");
        check (receding   <  995.0f, "pitch falls as the source recedes");
    }

    // -----------------------------------------------------------------------
    std::printf ("\nPitch control shifts by the interval asked for\n");
    {
        DopplerFXAudioProcessor p;
        p.prepareToPlay (sampleRate, blockSize);
        neutralPreset (p);

        setParam (p, ParamID::dopplerAmt,   0.0f);   // hold the source still
        setParam (p, ParamID::pitchSemis,  12.0f);   // one octave up

        const auto f = estimateFrequency (capture (p, 220.0, 0.5, 0.5));
        std::printf ("        220 Hz shifted up an octave reads %.1f Hz\n", f);

        check (std::abs (f - 440.0f) < 30.0f, "+12 semitones roughly doubles the frequency",
               "measured " + std::to_string (f) + " Hz");
    }

    // -----------------------------------------------------------------------
    std::printf ("\nSilence in, silence out\n");
    {
        DopplerFXAudioProcessor p;
        p.prepareToPlay (sampleRate, blockSize);
        neutralPreset (p);
        setParam (p, ParamID::feedback,   90.0f);
        setParam (p, ParamID::resonance, 100.0f);

        const auto r = run (p, 3.0, [] (int) { return 0.0f; });
        check (r.maxAbs < 1.0e-6f, "no self noise with no input",
               "peak was " + std::to_string (r.maxAbs));
    }

    // -----------------------------------------------------------------------
    std::printf ("\nBad input does not latch the plugin into NaN\n");
    {
        DopplerFXAudioProcessor p;
        p.prepareToPlay (sampleRate, blockSize);
        neutralPreset (p);
        setParam (p, ParamID::feedback, 80.0f);

        juce::AudioBuffer<float> buffer (2, blockSize);
        juce::MidiBuffer midi;

        // One poisoned block.
        for (int ch = 0; ch < 2; ++ch)
            for (int i = 0; i < blockSize; ++i)
                buffer.setSample (ch, i, i % 3 == 0 ? std::numeric_limits<float>::quiet_NaN()
                                                    : std::numeric_limits<float>::infinity());
        p.processBlock (buffer, midi);

        // Then a second of ordinary signal.
        const auto r = run (p, 1.0, [] (int n)
        {
            return 0.5f * (float) std::sin (juce::MathConstants<double>::twoPi * 440.0 * n / sampleRate);
        });

        check (r.finite, "recovers to finite output after NaN and Inf input");
        check (r.maxAbs > 0.01f, "still passes audio afterwards",
               "peak was " + std::to_string (r.maxAbs));
    }

    // -----------------------------------------------------------------------
    std::printf ("\nEvery factory preset is real, and safe\n");
    {
        const auto& bank = Presets::factory();
        check (! bank.empty(), "the factory bank is not empty");

        // A typo in a preset's parameter ID would silently do nothing, so make
        // every ID resolve against the real parameter list.
        {
            DopplerFXAudioProcessor p;
            p.prepareToPlay (sampleRate, blockSize);

            juce::StringArray unknown;
            for (const auto& preset : bank)
                for (const auto& [id, value] : preset.values)
                    if (p.getState().getParameter (id) == nullptr)
                        unknown.add (juce::String (preset.name) + "/" + id);

            check (unknown.isEmpty(), "every preset parameter ID resolves",
                   unknown.joinIntoString (", ").toStdString());
        }

        // Each preset, driven with full-scale noise, must stay inside its own
        // ceiling. These are the settings people will actually reach for.
        std::mt19937 rng (7);
        std::uniform_real_distribution<float> noise (-1.0f, 1.0f);

        bool allBounded = true, allFinite = true;
        std::string worst;

        for (int i = 0; i < (int) bank.size(); ++i)
        {
            DopplerFXAudioProcessor p;
            p.prepareToPlay (sampleRate, blockSize);
            p.setCurrentProgram (i);

            const auto ceilDb = p.getState().getRawParameterValue (ParamID::ceiling)->load();
            const auto limit  = juce::Decibels::decibelsToGain (ceilDb) + tolerance;

            const auto r = run (p, 4.0, [&] (int) { return noise (rng); });

            if (! r.finite) { allFinite = false; worst = bank[(size_t) i].name; }
            if (r.maxAbs > limit)
            {
                allBounded = false;
                worst = std::string (bank[(size_t) i].name) + " hit "
                          + std::to_string (juce::Decibels::gainToDecibels (r.maxAbs)) + " dB";
            }
        }

        check (allFinite,  "no preset produces non-finite output", worst);
        check (allBounded, "no preset exceeds its own ceiling under full-scale noise", worst);

        // Loading a preset must reset what the previous one changed, or presets
        // quietly inherit each other and stop being reproducible.
        {
            DopplerFXAudioProcessor viaOther;
            viaOther.prepareToPlay (sampleRate, blockSize);
            viaOther.setCurrentProgram ((int) bank.size() - 1);   // something maximal
            viaOther.setCurrentProgram (1);

            DopplerFXAudioProcessor fresh;
            fresh.prepareToPlay (sampleRate, blockSize);
            fresh.setCurrentProgram (1);

            juce::StringArray drifted;
            for (auto* param : fresh.getParameters())
                if (auto* ranged = dynamic_cast<juce::RangedAudioParameter*> (param))
                {
                    auto* other = viaOther.getState().getParameter (ranged->paramID);
                    if (other != nullptr && std::abs (other->getValue() - ranged->getValue()) > 1.0e-6f)
                        drifted.add (ranged->paramID);
                }

            check (drifted.isEmpty(), "a preset lands the same whatever was loaded before it",
                   drifted.joinIntoString (", ").toStdString());
        }

        // The host program interface is what the DAW's own preset menu drives.
        {
            DopplerFXAudioProcessor p;
            p.prepareToPlay (sampleRate, blockSize);
            p.setCurrentProgram (3);

            check (p.getNumPrograms() == (int) bank.size()
                     && p.getCurrentProgram() == 3
                     && p.getProgramName (3) == juce::String (bank[3].name),
                   "programs report back to the host correctly");

            // A saved session must not re-apply the preset over restored values.
            auto* cutoff = p.getState().getParameter (ParamID::cutoff);
            cutoff->setValueNotifyingHost (cutoff->convertTo0to1 (777.0f));

            juce::MemoryBlock state;
            p.getStateInformation (state);

            DopplerFXAudioProcessor q;
            q.prepareToPlay (sampleRate, blockSize);
            q.setStateInformation (state.getData(), (int) state.getSize());

            const auto restored = q.getState().getRawParameterValue (ParamID::cutoff)->load();
            check (q.getCurrentProgram() == 3 && std::abs (restored - 777.0f) < 1.0f,
                   "restoring a session keeps edits made on top of a preset",
                   "cutoff came back as " + std::to_string (restored));
        }
    }

    // -----------------------------------------------------------------------
    std::printf ("\nHousekeeping\n");
    {
        DopplerFXAudioProcessor p;
        p.prepareToPlay (sampleRate, blockSize);

        check (p.getLatencySamples() > 0, "reports its look-ahead latency to the host");

        setParam (p, ParamID::cutoff, 3210.0f);
        setParam (p, ParamID::spread,   77.0f);

        juce::MemoryBlock state;
        p.getStateInformation (state);

        DopplerFXAudioProcessor q;
        q.prepareToPlay (sampleRate, blockSize);
        q.setStateInformation (state.getData(), (int) state.getSize());

        const auto cutoffBack = q.getState().getRawParameterValue (ParamID::cutoff)->load();
        const auto spreadBack = q.getState().getRawParameterValue (ParamID::spread)->load();

        check (std::abs (cutoffBack - 3210.0f) < 1.0f && std::abs (spreadBack - 77.0f) < 0.5f,
               "saves and restores its state");

        // Sample rates a host might actually hand us.
        bool allRatesFine = true;
        for (double sr : { 44100.0, 48000.0, 88200.0, 96000.0, 192000.0 })
        {
            DopplerFXAudioProcessor r;
            r.prepareToPlay (sr, 64);
            neutralPreset (r);
            setParam (r, ParamID::feedback, 70.0f);

            juce::AudioBuffer<float> buffer (2, 64);
            juce::MidiBuffer midi;
            for (int block = 0; block < 200; ++block)
            {
                for (int ch = 0; ch < 2; ++ch)
                    for (int i = 0; i < 64; ++i)
                        buffer.setSample (ch, i, 0.7f * (float) std::sin (0.05 * (block * 64 + i)));

                r.processBlock (buffer, midi);

                for (int ch = 0; ch < 2; ++ch)
                    for (int i = 0; i < 64; ++i)
                        if (! std::isfinite (buffer.getSample (ch, i)))
                            allRatesFine = false;
            }
        }
        check (allRatesFine, "runs clean from 44.1 kHz to 192 kHz");
    }

    std::printf ("\n%d checks, %d failures\n", checks, failures);
    return failures == 0 ? 0 : 1;
}
