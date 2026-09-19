#include "Presets.h"
#include "Parameters.h"

using namespace juce;

/*  Tempo divisions, by index into Params::syncDivisions():
    0: 8 bars  1: 4 bars  2: 2 bars  3: 1 bar  4: 1/2   5: 1/2T
    6: 1/4     7: 1/4T    8: 1/8     9: 1/8T  10: 1/16 11: 1/16T  12: 1/32
*/
const std::vector<Preset>& Presets::factory()
{
    static const std::vector<Preset> bank
    {
        {
            "Calibration", "Everything at default. The reference point.",
            { }
        },
        {
            "Drive-By", "A source passing you at speed, close and wide. The literal version of the effect.",
            { { ParamID::motionMode,  0.0f }, { ParamID::rateHz,      0.30f },
              { ParamID::distance,    3.0f }, { ParamID::pathLength,  80.0f },
              { ParamID::dopplerAmt,100.0f }, { ParamID::spread,      70.0f },
              { ParamID::proximity,  80.0f }, { ParamID::airDamp,     60.0f },
              { ParamID::cutoff,   9000.0f }, { ParamID::resonance,   10.0f },
              { ParamID::filterTrack,40.0f }, { ParamID::feedback,     5.0f },
              { ParamID::mix,       100.0f } }
        },
        {
            "Rotary Chamber", "Tight synced orbit. Rotary-cabinet territory on keys and guitars.",
            { { ParamID::motionMode,  1.0f }, { ParamID::tempoSync,    1.0f },
              { ParamID::rateDiv,     6.0f }, { ParamID::distance,     1.5f },
              { ParamID::pathLength, 12.0f }, { ParamID::dopplerAmt, 130.0f },
              { ParamID::spread,     45.0f }, { ParamID::proximity,   55.0f },
              { ParamID::cutoff,   7000.0f }, { ParamID::resonance,   25.0f },
              { ParamID::filterTrack,30.0f }, { ParamID::airDamp,     35.0f },
              { ParamID::feedback,   10.0f }, { ParamID::mix,         60.0f } }
        },
        {
            "Heat Death", "Very slow pendulum over a long path. A riser that takes its time falling apart.",
            { { ParamID::motionMode,  2.0f }, { ParamID::rateHz,      0.15f },
              { ParamID::distance,    1.0f }, { ParamID::pathLength, 150.0f },
              { ParamID::dopplerAmt,200.0f }, { ParamID::spread,      60.0f },
              { ParamID::proximity,  70.0f }, { ParamID::cutoff,    4000.0f },
              { ParamID::resonance,  55.0f }, { ParamID::filterTrack,-60.0f },
              { ParamID::airDamp,    55.0f }, { ParamID::feedback,    45.0f },
              { ParamID::mix,       100.0f }, { ParamID::ceiling,     -1.0f } }
        },
        {
            "Half-Life", "Distance duck on eighths. The motion becomes the rhythm.",
            { { ParamID::motionMode,  0.0f }, { ParamID::tempoSync,    1.0f },
              { ParamID::rateDiv,     8.0f }, { ParamID::gateDuck,    85.0f },
              { ParamID::dopplerAmt, 40.0f }, { ParamID::spread,      90.0f },
              { ParamID::proximity,  30.0f }, { ParamID::distance,     3.0f },
              { ParamID::pathLength, 50.0f }, { ParamID::cutoff,   14000.0f },
              { ParamID::resonance,  20.0f }, { ParamID::filterTrack, 20.0f },
              { ParamID::airDamp,    25.0f }, { ParamID::feedback,     8.0f },
              { ParamID::mix,       100.0f } }
        },
        {
            "Brownian Width", "Barely there. A slow drift that widens a source without announcing itself.",
            { { ParamID::motionMode,  1.0f }, { ParamID::rateHz,      0.10f },
              { ParamID::distance,    8.0f }, { ParamID::pathLength,   6.0f },
              { ParamID::dopplerAmt, 25.0f }, { ParamID::spread,      75.0f },
              { ParamID::proximity,  20.0f }, { ParamID::cutoff,   16000.0f },
              { ParamID::resonance,   8.0f }, { ParamID::filterTrack, 10.0f },
              { ParamID::airDamp,    20.0f }, { ParamID::feedback,     0.0f },
              { ParamID::mix,        25.0f } }
        },
        {
            "Centrifuge", "Sixteenth-note orbit, close in and hard. Violent, and still bounded.",
            { { ParamID::motionMode,  1.0f }, { ParamID::tempoSync,    1.0f },
              { ParamID::rateDiv,    10.0f }, { ParamID::distance,     1.0f },
              { ParamID::pathLength,  8.0f }, { ParamID::dopplerAmt, 180.0f },
              { ParamID::spread,     80.0f }, { ParamID::proximity,   75.0f },
              { ParamID::cutoff,   6000.0f }, { ParamID::resonance,   45.0f },
              { ParamID::filterTrack,50.0f }, { ParamID::airDamp,     40.0f },
              { ParamID::feedback,   30.0f }, { ParamID::mix,         85.0f },
              { ParamID::ceiling,    -1.0f } }
        },
        {
            "Decay Chamber", "Heavy feedback with the gate holding the tail back. A room that will not let go.",
            { { ParamID::motionMode,  0.0f }, { ParamID::rateHz,      0.20f },
              { ParamID::distance,    6.0f }, { ParamID::pathLength,  50.0f },
              { ParamID::dopplerAmt, 90.0f }, { ParamID::spread,      50.0f },
              { ParamID::proximity,  60.0f }, { ParamID::cutoff,    5000.0f },
              { ParamID::resonance,  35.0f }, { ParamID::filterTrack,-25.0f },
              { ParamID::airDamp,    70.0f }, { ParamID::feedback,    75.0f },
              { ParamID::gateOn,      1.0f }, { ParamID::gateThresh, -48.0f },
              { ParamID::gateDepth,  60.0f }, { ParamID::gateAttack,   5.0f },
              { ParamID::gateRelease,400.0f}, { ParamID::mix,         70.0f },
              { ParamID::ceiling,    -1.0f } }
        },
        {
            "Red Shift", "Pitched down and receding, dark and far off.",
            { { ParamID::motionMode,  0.0f }, { ParamID::rateHz,      0.12f },
              { ParamID::pitchSemis, -7.0f }, { ParamID::dopplerAmt,  70.0f },
              { ParamID::distance,   12.0f }, { ParamID::pathLength, 100.0f },
              { ParamID::spread,     40.0f }, { ParamID::proximity,   70.0f },
              { ParamID::cutoff,   3500.0f }, { ParamID::resonance,   30.0f },
              { ParamID::filterTrack,-40.0f}, { ParamID::airDamp,     80.0f },
              { ParamID::feedback,   25.0f }, { ParamID::mix,        100.0f } }
        },
        {
            "Blue Shift", "Pitched up and arriving, bright and close.",
            { { ParamID::motionMode,  0.0f }, { ParamID::rateHz,      0.12f },
              { ParamID::pitchSemis,  7.0f }, { ParamID::dopplerAmt,  70.0f },
              { ParamID::distance,    3.0f }, { ParamID::pathLength,  90.0f },
              { ParamID::spread,     65.0f }, { ParamID::proximity,   70.0f },
              { ParamID::cutoff,  15000.0f }, { ParamID::resonance,   22.0f },
              { ParamID::filterTrack,55.0f }, { ParamID::airDamp,     20.0f },
              { ParamID::feedback,   12.0f }, { ParamID::mix,        100.0f } }
        },
        {
            "Resonance Cascade", "Band pass at high Q, tracking the motion, fed back on itself. The guard earns its keep here.",
            { { ParamID::motionMode,  0.0f }, { ParamID::tempoSync,    1.0f },
              { ParamID::rateDiv,     4.0f }, { ParamID::dopplerAmt, 120.0f },
              { ParamID::distance,    2.0f }, { ParamID::pathLength,  60.0f },
              { ParamID::spread,     70.0f }, { ParamID::proximity,   65.0f },
              { ParamID::filterType,  1.0f }, { ParamID::cutoff,     900.0f },
              { ParamID::resonance,  85.0f }, { ParamID::filterTrack, 70.0f },
              { ParamID::airDamp,    30.0f }, { ParamID::feedback,    55.0f },
              { ParamID::mix,        90.0f }, { ParamID::ceiling,     -1.5f } }
        },
        {
            "Vacuum Drift", "Far away, barely moving, almost all high end gone. Background weather.",
            { { ParamID::motionMode,  1.0f }, { ParamID::rateHz,      0.04f },
              { ParamID::distance,   30.0f }, { ParamID::pathLength, 120.0f },
              { ParamID::dopplerAmt, 80.0f }, { ParamID::spread,      35.0f },
              { ParamID::proximity,  90.0f }, { ParamID::cutoff,    2200.0f },
              { ParamID::resonance,  12.0f }, { ParamID::filterTrack,-30.0f },
              { ParamID::airDamp,    90.0f }, { ParamID::feedback,    20.0f },
              { ParamID::mix,        45.0f } }
        }
    };

    return bank;
}

void Presets::apply (AudioProcessorValueTreeState& state, int index)
{
    const auto& bank = factory();
    if (! isPositiveAndBelow (index, (int) bank.size()))
        return;

    // Start from defaults so a preset never inherits whatever was set before it.
    for (auto* param : state.processor.getParameters())
        if (auto* ranged = dynamic_cast<RangedAudioParameter*> (param))
            ranged->setValueNotifyingHost (ranged->getDefaultValue());

    for (const auto& [id, value] : bank[(size_t) index].values)
        if (auto* param = state.getParameter (id))
            param->setValueNotifyingHost (param->convertTo0to1 (value));
}
