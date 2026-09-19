#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include "Theme.h"

using APVTS = juce::AudioProcessorValueTreeState;

/** A titled panel with an engraved caption; the visual grouping of the UI. */
class SectionPanel final : public juce::Component
{
public:
    explicit SectionPanel (juce::String sectionTitle) : title (std::move (sectionTitle)) {}
    void paint (juce::Graphics&) override;

    /** Content area, inside the frame and below the caption. */
    juce::Rectangle<int> getContentArea() const;

private:
    juce::String title;
};

/** Rotary control with its name above and its live value below. */
class Knob final : public juce::Component
{
public:
    Knob (APVTS& state, const juce::String& paramID, juce::String displayName);

    void resized() override;
    void paint (juce::Graphics&) override;

    juce::Slider& getSlider() noexcept { return slider; }

private:
    juce::Slider slider;
    juce::Label  nameLabel, valueLabel;
    juce::String name;
    std::unique_ptr<APVTS::SliderAttachment> attachment;
};

/** Labelled drop-down. */
class ChoiceBox final : public juce::Component
{
public:
    ChoiceBox (APVTS& state, const juce::String& paramID, juce::String displayName);

    void resized() override;
    void paint (juce::Graphics&) override;

private:
    juce::ComboBox box;
    juce::String   name;
    std::unique_ptr<APVTS::ComboBoxAttachment> attachment;
};

/** Pill switch. */
class Switch final : public juce::Component
{
public:
    Switch (APVTS& state, const juce::String& paramID, const juce::String& displayName);

    void resized() override;

private:
    juce::ToggleButton button;
    std::unique_ptr<APVTS::ButtonAttachment> attachment;
};

/** Output meter: signal level as a bar, limiter reduction biting down from the right. */
class LevelMeter final : public juce::Component
{
public:
    void setLevels (float outLevelLinear, float gainReductionLinear);
    void paint (juce::Graphics&) override;

private:
    float level = 0.0f;         // smoothed, linear
    float reduction = 0.0f;     // dB of reduction, positive
};
