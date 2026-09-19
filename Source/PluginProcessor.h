#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>

#include "Parameters.h"
#include "Presets.h"
#include "dsp/DopplerEngine.h"
#include "dsp/PitchShifter.h"
#include "dsp/ResonantFilter.h"
#include "dsp/Gate.h"
#include "dsp/Limiter.h"

/*  Signal flow, per sample:

        in ──┬─────────────────────────── dry ───────────────┐
             │                                               │
             └─ gate ─┬─ (+ feedback) ─ doppler delay line ───┤
                      │        (pitch shift, moving level)    │
                      │                                       │
                      └──────── resonant filter ── air ───────┤
                                    │                         │
                                    └──── feedback ───────┐   │
                                                          │   │
                                              duck ◄──────┘   │
                                                              │
                                    mix ◄─────────────────────┘
                                     │
                                  output ── DC block ── limiter ── out

    Everything that can build up energy — the feedback path, the resonant
    filter — is bounded on its own terms before the limiter ever sees it.
*/
class DopplerFXAudioProcessor final : public juce::AudioProcessor
{
public:
    DopplerFXAudioProcessor();
    ~DopplerFXAudioProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using juce::AudioProcessor::processBlock;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override                          { return true; }

    const juce::String getName() const override              { return "Doppler FX"; }
    bool acceptsMidi() const override                        { return false; }
    bool producesMidi() const override                       { return false; }
    bool isMidiEffect() const override                       { return false; }
    double getTailLengthSeconds() const override             { return 2.0; }

    // Factory presets, exposed as host programs so they appear in the DAW's
    // own preset menu as well as in the plugin header.
    int getNumPrograms() override                            { return (int) Presets::factory().size(); }
    int getCurrentProgram() override                         { return currentProgram; }
    void setCurrentProgram (int) override;
    const juce::String getProgramName (int) override;
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

    juce::AudioProcessorValueTreeState& getState() noexcept  { return apvts; }

    // ---- Values the editor polls; written on the audio thread ---------------
    struct Visuals
    {
        std::atomic<float> sourceX      { 0.0f };   // metres
        std::atomic<float> sourceY      { 4.0f };   // metres
        std::atomic<float> extent       { 20.0f };  // metres, for scaling the display
        std::atomic<float> distance     { 4.0f };   // metres
        std::atomic<float> radialVelocity { 0.0f }; // m/s, + is receding
        std::atomic<float> gainReduction{ 1.0f };   // linear, 1 == none
        std::atomic<float> gateGain     { 1.0f };
        std::atomic<float> inLevel      { 0.0f };
        std::atomic<float> outLevel     { 0.0f };
        std::atomic<float> rateDisplay  { 0.5f };   // Hz actually in use
    };

    Visuals visuals;

private:
    void updateParameters (double bpm);
    void syncToHost();

    juce::AudioProcessorValueTreeState apvts;

    DopplerEngine  doppler;
    PitchShifter   pitchShifter;
    ResonantFilter filter;
    Gate           gate;
    Limiter        limiter;

    // Smoothed, so that host automation and knob drags never click.
    juce::SmoothedValue<float> smoothedMix, smoothedOut, smoothedFeedback, smoothedDuck;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Multiplicative> smoothedCutoff;

    struct DcBlock { float x1 = 0.0f, y1 = 0.0f; };
    std::array<DcBlock, 2> dcBlockers;

    std::array<float, 2> feedbackState { { 0.0f, 0.0f } };
    float feedbackEnv   = 0.0f;
    float feedbackGuard = 1.0f;

    double currentSampleRate = 44100.0;
    float  phaseIncrement    = 0.0f;
    float  maxExtent         = 20.0f;
    float  resonanceValue    = 0.18f;
    float  airAmount         = 0.45f;
    float  trackAmount       = 0.35f;
    float  baseCutoff        = 12000.0f;
    ResonantFilter::Type filterType = ResonantFilter::Type::lowpass;

    int currentProgram = 0;

    int controlCounter = 0;
    static constexpr int controlInterval = 32;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DopplerFXAudioProcessor)
};
