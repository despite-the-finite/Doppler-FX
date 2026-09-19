#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "Theme.h"
#include "../PluginProcessor.h"

/*  Plan view of the virtual space: you at the bottom, the source moving
    through the field above you, a comet trail behind it, and the pitch shift
    the geometry is currently producing printed underneath.

    It exists to make the controls legible — Distance, Path and Motion are
    abstract as numbers and obvious as a picture.
*/
class RadarDisplay final : public juce::Component,
                           private juce::Timer
{
public:
    explicit RadarDisplay (DopplerFXAudioProcessor&);

    void paint (juce::Graphics&) override;

private:
    void timerCallback() override;

    DopplerFXAudioProcessor& processor;

    static constexpr int trailLength = 110;
    std::array<juce::Point<float>, trailLength> trail {};
    int   trailHead   = 0;
    int   trailFilled = 0;

    float smoothedExtent   = 20.0f;
    float smoothedVelocity = 0.0f;
    float smoothedSpread   = 0.55f;

    DopplerEngine::Settings pathSettings;
};
