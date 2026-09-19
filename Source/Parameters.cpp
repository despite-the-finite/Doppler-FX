#include "Parameters.h"

using namespace juce;

namespace
{
    // A gentle skew so that the useful low end of a range gets more of the knob.
    NormalisableRange<float> skewed (float lo, float hi, float centre, float step = 0.0f)
    {
        NormalisableRange<float> r { lo, hi, step };
        r.setSkewForCentre (centre);
        return r;
    }

    String pctText   (float v, int)  { return String (roundToInt (v)) + " %"; }
    String msText    (float v, int)  { return (v < 10.0f ? String (v, 2) : String (v, 1)) + " ms"; }
    String dbText    (float v, int)  { return String (v, 1) + " dB"; }
    String metreText (float v, int)  { return (v < 10.0f ? String (v, 2) : String (v, 1)) + " m"; }
    String hzText    (float v, int)  { return v >= 1000.0f ? String (v / 1000.0f, 2) + " kHz"
                                                           : String (roundToInt (v)) + " Hz"; }
    String rateText  (float v, int)  { return v < 1.0f ? String (v, 3) + " Hz" : String (v, 2) + " Hz"; }
    String semiText  (float v, int)
    {
        const auto s = String (v, 2);
        return (v > 0.0f ? "+" + s : s) + " st";
    }
}

const std::vector<SyncDivision>& Params::syncDivisions()
{
    // Durations of one full motion cycle, in whole notes (1.0 == one 4/4 bar).
    static const std::vector<SyncDivision> divs
    {
        { "8 bars",  8.0     }, { "4 bars",  4.0     }, { "2 bars",  2.0     },
        { "1 bar",   1.0     }, { "1/2",     0.5     }, { "1/2T",    1.0 / 3.0 },
        { "1/4",     0.25    }, { "1/4T",    1.0 / 6.0 }, { "1/8",   0.125   },
        { "1/8T",    1.0 / 12.0 }, { "1/16",  0.0625 }, { "1/16T", 1.0 / 24.0 },
        { "1/32",    0.03125 }
    };
    return divs;
}

double Params::divisionToHz (int divisionIndex, double bpm)
{
    const auto& divs = syncDivisions();
    const auto  idx  = (size_t) jlimit (0, (int) divs.size() - 1, divisionIndex);
    const auto  beatsPerCycle = divs[idx].wholeNotes * 4.0;      // 4 beats per whole note
    const auto  secondsPerCycle = beatsPerCycle * 60.0 / jmax (1.0, bpm);
    return 1.0 / jmax (1.0e-4, secondsPerCycle);
}

AudioProcessorValueTreeState::ParameterLayout Params::createLayout()
{
    AudioProcessorValueTreeState::ParameterLayout layout;

    auto choiceNames = [] (const std::vector<SyncDivision>& d)
    {
        StringArray s;
        for (const auto& div : d)
            s.add (div.name);
        return s;
    };

    auto fparam = [&layout] (const char* id, const char* name,
                             NormalisableRange<float> range, float def,
                             std::function<String (float, int)> toText,
                             const char* label = "")
    {
        layout.add (std::make_unique<AudioParameterFloat> (
            ParameterID { id, 1 }, name, range, def,
            AudioParameterFloatAttributes().withStringFromValueFunction (std::move (toText))
                                           .withLabel (label)));
    };

    // ---- Motion ------------------------------------------------------------
    layout.add (std::make_unique<AudioParameterChoice> (
        ParameterID { ParamID::motionMode, 1 }, "Motion",
        StringArray { "Flyby", "Orbit", "Pendulum", "Manual" }, 0));

    layout.add (std::make_unique<AudioParameterBool> (
        ParameterID { ParamID::tempoSync, 1 }, "Tempo Sync", false));

    fparam (ParamID::rateHz, "Rate", skewed (0.01f, 20.0f, 1.0f), 0.5f, rateText, "Hz");

    layout.add (std::make_unique<AudioParameterChoice> (
        ParameterID { ParamID::rateDiv, 1 }, "Division",
        choiceNames (syncDivisions()), 3));

    fparam (ParamID::manualPos,  "Position",   { 0.0f, 1.0f },                  0.5f,  [] (float v, int) { return String (v, 3); });
    fparam (ParamID::distance,   "Distance",   skewed (0.20f, 60.0f, 6.0f),     4.0f,  metreText, "m");
    fparam (ParamID::pathLength, "Path",       skewed (1.0f, 200.0f, 40.0f),    40.0f, metreText, "m");
    fparam (ParamID::dopplerAmt, "Doppler",    { 0.0f, 200.0f, 0.1f },          100.0f, pctText,  "%");
    fparam (ParamID::spread,     "Spread",     { 0.0f, 100.0f, 0.1f },          55.0f, pctText,   "%");
    fparam (ParamID::proximity,  "Proximity",  { 0.0f, 100.0f, 0.1f },          60.0f, pctText,   "%");

    // ---- Tone --------------------------------------------------------------
    fparam (ParamID::pitchSemis, "Pitch", { -12.0f, 12.0f, 0.01f }, 0.0f, semiText, "st");

    layout.add (std::make_unique<AudioParameterChoice> (
        ParameterID { ParamID::filterType, 1 }, "Filter",
        StringArray { "Low Pass", "Band Pass", "High Pass" }, 0));

    fparam (ParamID::cutoff,      "Cutoff",    skewed (20.0f, 20000.0f, 1000.0f), 12000.0f, hzText, "Hz");
    fparam (ParamID::resonance,   "Resonance", { 0.0f, 100.0f, 0.1f },            18.0f,  pctText, "%");
    fparam (ParamID::filterTrack, "Track",     { -100.0f, 100.0f, 0.1f },         35.0f,  pctText, "%");
    fparam (ParamID::airDamp,     "Air",       { 0.0f, 100.0f, 0.1f },            45.0f,  pctText, "%");
    fparam (ParamID::feedback,    "Feedback",  { 0.0f, 95.0f, 0.1f },             15.0f,  pctText, "%");

    // ---- Gate --------------------------------------------------------------
    layout.add (std::make_unique<AudioParameterBool> (
        ParameterID { ParamID::gateOn, 1 }, "Gate", false));

    fparam (ParamID::gateThresh,  "Threshold", { -72.0f, 0.0f, 0.1f },           -42.0f, dbText, "dB");
    fparam (ParamID::gateDepth,   "Gate Depth",{ 0.0f, 100.0f, 0.1f },            100.0f, pctText, "%");
    fparam (ParamID::gateAttack,  "Attack",    skewed (0.1f, 200.0f, 10.0f),      2.0f,  msText, "ms");
    fparam (ParamID::gateRelease, "Release",   skewed (5.0f, 2000.0f, 200.0f),    160.0f, msText, "ms");
    fparam (ParamID::gateDuck,    "Duck",      { 0.0f, 100.0f, 0.1f },            0.0f,  pctText, "%");

    // ---- Output ------------------------------------------------------------
    fparam (ParamID::mix,        "Mix",     { 0.0f, 100.0f, 0.1f },  50.0f, pctText, "%");

    layout.add (std::make_unique<AudioParameterBool> (
        ParameterID { ParamID::safety, 1 }, "Safety Limiter", true));

    fparam (ParamID::ceiling,    "Ceiling", { -12.0f, 0.0f, 0.1f },  -0.5f, dbText, "dB");
    fparam (ParamID::outputGain, "Output",  { -24.0f, 12.0f, 0.1f },  0.0f, dbText, "dB");

    return layout;
}
