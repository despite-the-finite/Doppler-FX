#pragma once

#include "PluginProcessor.h"
#include "gui/LookAndFeel.h"
#include "gui/Controls.h"
#include "gui/RadarDisplay.h"

#include <memory>
#include <vector>

class DopplerFXAudioProcessorEditor final : public juce::AudioProcessorEditor,
                                            private juce::Timer
{
public:
    explicit DopplerFXAudioProcessorEditor (DopplerFXAudioProcessor&);
    ~DopplerFXAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    void drawHeader (juce::Graphics&, juce::Rectangle<int>);

    Knob&      addKnob (SectionPanel&, const char* paramID, const juce::String& name);
    ChoiceBox& addChoice (SectionPanel&, const char* paramID, const juce::String& name);
    Switch&    addSwitch (SectionPanel&, const char* paramID, const juce::String& name);

    DopplerFXAudioProcessor& processor;
    DopplerLookAndFeel       lookAndFeel;

    SectionPanel motionPanel { "Motion" };
    SectionPanel tonePanel   { "Tone" };
    SectionPanel gatePanel   { "Gate" };
    SectionPanel outputPanel { "Output" };

    RadarDisplay radar;
    LevelMeter   meter;

    juce::ComboBox   presetBox;
    juce::TextButton prevPreset { "<" }, nextPreset { ">" };
    int              shownProgram = -1;

    void buildPresetSelector();
    void stepPreset (int delta);
    void refreshPresetSelector();

    std::vector<std::unique_ptr<Knob>>      knobs;
    std::vector<std::unique_ptr<ChoiceBox>> choices;
    std::vector<std::unique_ptr<Switch>>    switches;

    // Named handles into the vectors above, for layout.
    Knob *rate = nullptr, *position = nullptr, *distance = nullptr, *path = nullptr;
    Knob *dopplerAmt = nullptr, *spread = nullptr, *proximity = nullptr;
    Knob *pitch = nullptr, *cutoff = nullptr, *resonance = nullptr;
    Knob *track = nullptr, *air = nullptr, *feedback = nullptr;
    Knob *gateThresh = nullptr, *gateDepth = nullptr, *gateAttack = nullptr;
    Knob *gateRelease = nullptr, *gateDuck = nullptr;
    Knob *mix = nullptr, *ceiling = nullptr, *outputGain = nullptr;

    ChoiceBox *motionMode = nullptr, *division = nullptr, *filterType = nullptr;
    Switch    *sync = nullptr, *gateOn = nullptr, *safety = nullptr;

    juce::TooltipWindow tooltips { this, 600 };

    static constexpr int headerHeight = 76;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DopplerFXAudioProcessorEditor)
};
