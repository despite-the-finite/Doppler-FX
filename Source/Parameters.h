#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <vector>

/*  Central definition of every automatable parameter.

    IDs are string constants so that the processor, the editor and any saved
    session state all agree on them. Never change an existing ID: doing so
    silently breaks recall in projects that already use the plugin.
*/
namespace ParamID
{
    // Motion -----------------------------------------------------------------
    inline constexpr const char* motionMode = "motionMode";
    inline constexpr const char* tempoSync  = "tempoSync";
    inline constexpr const char* rateHz     = "rateHz";
    inline constexpr const char* rateDiv    = "rateDiv";
    inline constexpr const char* manualPos  = "manualPos";
    inline constexpr const char* distance   = "distance";
    inline constexpr const char* pathLength = "pathLength";
    inline constexpr const char* dopplerAmt = "dopplerAmt";
    inline constexpr const char* spread     = "spread";
    inline constexpr const char* proximity  = "proximity";

    // Tone -------------------------------------------------------------------
    inline constexpr const char* pitchSemis = "pitchSemis";
    inline constexpr const char* filterType = "filterType";
    inline constexpr const char* cutoff     = "cutoff";
    inline constexpr const char* resonance  = "resonance";
    inline constexpr const char* filterTrack= "filterTrack";
    inline constexpr const char* airDamp    = "airDamp";
    inline constexpr const char* feedback   = "feedback";

    // Gate -------------------------------------------------------------------
    inline constexpr const char* gateOn     = "gateOn";
    inline constexpr const char* gateThresh = "gateThresh";
    inline constexpr const char* gateDepth  = "gateDepth";
    inline constexpr const char* gateAttack = "gateAttack";
    inline constexpr const char* gateRelease= "gateRelease";
    inline constexpr const char* gateDuck   = "gateDuck";

    // Output -----------------------------------------------------------------
    inline constexpr const char* mix        = "mix";
    inline constexpr const char* safety     = "safety";
    inline constexpr const char* ceiling    = "ceiling";
    inline constexpr const char* outputGain = "outputGain";
}

enum class MotionMode { flyby = 0, orbit, pendulum, manual };
enum class FilterType { lowpass = 0, bandpass, highpass };

/*  Tempo-sync divisions, expressed as a fraction of a whole note.
    A value of 1.0 means the full motion cycle lasts one bar of 4/4.
*/
struct SyncDivision
{
    const char* name;
    double      wholeNotes;
};

namespace Params
{
    juce::AudioProcessorValueTreeState::ParameterLayout createLayout();

    const std::vector<SyncDivision>& syncDivisions();

    /** Motion cycles per second for the given division at the given tempo. */
    double divisionToHz (int divisionIndex, double bpm);
}
