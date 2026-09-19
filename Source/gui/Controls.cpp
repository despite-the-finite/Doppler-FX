#include "Controls.h"

using namespace juce;

// ---------------------------------------------------------------------------
// SectionPanel
// ---------------------------------------------------------------------------
void SectionPanel::paint (Graphics& g)
{
    auto bounds = getLocalBounds().toFloat().reduced (0.5f);

    g.setGradientFill (ColourGradient (Theme::panel.brighter (0.05f), bounds.getCentreX(), bounds.getY(),
                                       Theme::panel.darker (0.22f),   bounds.getCentreX(), bounds.getBottom(), false));
    g.fillRoundedRectangle (bounds, 6.0f);

    g.setColour (Theme::hairline);
    g.drawRoundedRectangle (bounds, 6.0f, 1.0f);

    // Caption, letter-spaced by hand for the engraved look.
    auto captionArea = bounds.removeFromTop (26.0f).reduced (12.0f, 0.0f);

    String spaced;
    for (auto c : title.toUpperCase())
        spaced << c << (char) ' ';

    g.setFont (Theme::label (10.5f, true));
    g.setColour (Theme::accent.withAlpha (0.75f));
    g.drawText (spaced.trim(), captionArea, Justification::centredLeft, true);

    g.setColour (Theme::hairline.withAlpha (0.8f));
    g.drawHorizontalLine ((int) captionArea.getBottom(), bounds.getX() + 10.0f, bounds.getRight() - 10.0f);
}

Rectangle<int> SectionPanel::getContentArea() const
{
    return getLocalBounds().withTrimmedTop (30).reduced (8, 6);
}

// ---------------------------------------------------------------------------
// Knob
// ---------------------------------------------------------------------------
Knob::Knob (APVTS& state, const String& paramID, String displayName)
    : name (std::move (displayName))
{
    slider.setSliderStyle (Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle (Slider::NoTextBox, false, 0, 0);
    slider.setRotaryParameters (MathConstants<float>::pi * 1.2f,
                                MathConstants<float>::pi * 2.8f, true);
    slider.setDoubleClickReturnValue (true, slider.getDoubleClickReturnValue());
    addAndMakeVisible (slider);

    valueLabel.setJustificationType (Justification::centred);
    valueLabel.setColour (Label::textColourId, Theme::textDim);
    valueLabel.setFont (Theme::label (11.0f));
    valueLabel.setInterceptsMouseClicks (false, false);
    addAndMakeVisible (valueLabel);

    attachment = std::make_unique<APVTS::SliderAttachment> (state, paramID, slider);

    // Mirror the parameter's own text formatting, so the readout always matches
    // what the host shows for the same parameter.
    if (auto* param = state.getParameter (paramID))
    {
        auto update = [this, param]
        {
            valueLabel.setText (param->getCurrentValueAsText(), dontSendNotification);
        };
        slider.onValueChange = update;
        update();
    }
}

void Knob::resized()
{
    auto b = getLocalBounds();
    b.removeFromTop (15);                       // room for the name
    valueLabel.setBounds (b.removeFromBottom (14));
    slider.setBounds (b.reduced (2));
}

void Knob::paint (Graphics& g)
{
    g.setFont (Theme::label (11.0f, true));
    g.setColour (Theme::textDim);
    g.drawText (name.toUpperCase(), getLocalBounds().removeFromTop (15),
                Justification::centred, true);
}

// ---------------------------------------------------------------------------
// ChoiceBox
// ---------------------------------------------------------------------------
ChoiceBox::ChoiceBox (APVTS& state, const String& paramID, String displayName)
    : name (std::move (displayName))
{
    // The attachment only syncs the selection; filling the list is ours to do.
    if (auto* param = state.getParameter (paramID))
        box.addItemList (param->getAllValueStrings(), 1);

    addAndMakeVisible (box);
    attachment = std::make_unique<APVTS::ComboBoxAttachment> (state, paramID, box);
}

void ChoiceBox::resized()
{
    box.setBounds (getLocalBounds().withTrimmedTop (15).reduced (0, 1));
}

void ChoiceBox::paint (Graphics& g)
{
    g.setFont (Theme::label (11.0f, true));
    g.setColour (Theme::textDim);
    g.drawText (name.toUpperCase(), getLocalBounds().removeFromTop (15),
                Justification::centredLeft, true);
}

// ---------------------------------------------------------------------------
// Switch
// ---------------------------------------------------------------------------
Switch::Switch (APVTS& state, const String& paramID, const String& displayName)
{
    button.setButtonText (displayName);
    addAndMakeVisible (button);
    attachment = std::make_unique<APVTS::ButtonAttachment> (state, paramID, button);
}

void Switch::resized()
{
    button.setBounds (getLocalBounds());
}

// ---------------------------------------------------------------------------
// LevelMeter
// ---------------------------------------------------------------------------
void LevelMeter::setLevels (float outLevelLinear, float gainReductionLinear)
{
    // Fast rise, slow fall, so peaks stay readable at 30 fps.
    const auto target = jlimit (0.0f, 1.2f, outLevelLinear);
    level = target > level ? target : level + (target - level) * 0.25f;

    const auto grDb = -Decibels::gainToDecibels (jlimit (0.0001f, 1.0f, gainReductionLinear));
    reduction = grDb > reduction ? grDb : reduction + (grDb - reduction) * 0.2f;

    repaint();
}

void LevelMeter::paint (Graphics& g)
{
    auto area = getLocalBounds().toFloat();

    auto captions = area.removeFromTop (12.0f);
    g.setFont (Theme::label (9.0f, true));
    g.setColour (Theme::textFaint);
    g.drawText ("OUTPUT", captions, Justification::centredLeft, false);
    g.setColour (reduction > 0.05f ? Theme::warn : Theme::textFaint);
    g.drawText (reduction > 0.05f ? "LIMITING  -" + String (reduction, 1) + " dB" : "LIMITER",
                captions, Justification::centredRight, false);

    auto bounds = area.removeFromTop (14.0f);

    g.setColour (Theme::engrave);
    g.fillRoundedRectangle (bounds, 3.0f);

    // Signal, on a -48 dB scale.
    const auto db   = Decibels::gainToDecibels (jmax (0.0001f, level));
    const auto norm = jlimit (0.0f, 1.0f, (db + 48.0f) / 48.0f);

    auto fill = bounds.reduced (1.5f);
    fill = fill.withWidth (fill.getWidth() * norm);

    g.setGradientFill (ColourGradient (Theme::cool,            bounds.getX(),     0.0f,
                                       Theme::accent,          bounds.getRight(), 0.0f, false));
    g.fillRoundedRectangle (fill, 2.0f);

    // Limiter reduction, biting in from the right on a 12 dB scale.
    if (reduction > 0.05f)
    {
        const auto grNorm = jlimit (0.0f, 1.0f, reduction / 12.0f);
        auto grArea = bounds.reduced (1.5f);
        grArea = grArea.withLeft (grArea.getRight() - grArea.getWidth() * grNorm);

        g.setColour (Theme::warn.withAlpha (0.8f));
        g.fillRoundedRectangle (grArea, 2.0f);
    }

    g.setColour (Theme::hairline);
    g.drawRoundedRectangle (bounds.reduced (0.5f), 3.0f, 1.0f);
}
