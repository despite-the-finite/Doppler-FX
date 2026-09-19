#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "Theme.h"

class DopplerLookAndFeel final : public juce::LookAndFeel_V4
{
public:
    DopplerLookAndFeel();

    void drawRotarySlider (juce::Graphics&, int x, int y, int width, int height,
                           float sliderPos, float rotaryStartAngle, float rotaryEndAngle,
                           juce::Slider&) override;

    void drawComboBox (juce::Graphics&, int width, int height, bool isButtonDown,
                       int buttonX, int buttonY, int buttonW, int buttonH,
                       juce::ComboBox&) override;

    void positionComboBoxText (juce::ComboBox&, juce::Label&) override;

    void drawPopupMenuBackground (juce::Graphics&, int width, int height) override;

    void drawToggleButton (juce::Graphics&, juce::ToggleButton&,
                           bool shouldDrawButtonAsHighlighted,
                           bool shouldDrawButtonAsDown) override;

    void drawButtonBackground (juce::Graphics&, juce::Button&,
                               const juce::Colour& backgroundColour,
                               bool shouldDrawButtonAsHighlighted,
                               bool shouldDrawButtonAsDown) override;

    juce::Font getTextButtonFont (juce::TextButton&, int) override { return Theme::label (13.0f, true); }

    juce::Font getComboBoxFont (juce::ComboBox&) override        { return Theme::label (13.0f); }
    juce::Font getPopupMenuFont() override                        { return Theme::label (13.0f); }
    juce::Font getLabelFont (juce::Label&) override               { return Theme::label (12.0f); }
};
