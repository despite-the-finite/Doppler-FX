#include "LookAndFeel.h"

using namespace juce;

DopplerLookAndFeel::DopplerLookAndFeel()
{
    setColour (ResizableWindow::backgroundColourId, Theme::backdropBottom);
    setColour (Slider::rotarySliderFillColourId,    Theme::accent);
    setColour (Slider::rotarySliderOutlineColourId, Theme::engrave);
    setColour (Slider::textBoxTextColourId,         Theme::text);
    setColour (Slider::textBoxOutlineColourId,      Colours::transparentBlack);
    setColour (Slider::textBoxBackgroundColourId,   Colours::transparentBlack);
    setColour (Label::textColourId,                 Theme::text);
    setColour (ComboBox::backgroundColourId,        Theme::panelRaised);
    setColour (ComboBox::textColourId,              Theme::text);
    setColour (ComboBox::outlineColourId,           Theme::hairline);
    setColour (ComboBox::arrowColourId,             Theme::accent);
    setColour (PopupMenu::backgroundColourId,       Theme::panelRaised);
    setColour (PopupMenu::textColourId,             Theme::text);
    setColour (PopupMenu::highlightedBackgroundColourId, Theme::accent.withAlpha (0.22f));
    setColour (PopupMenu::highlightedTextColourId,  Theme::text);
    setColour (TooltipWindow::backgroundColourId,   Theme::panelRaised);
    setColour (TooltipWindow::textColourId,         Theme::text);
    setColour (TooltipWindow::outlineColourId,      Theme::hairline);
}

void DopplerLookAndFeel::drawRotarySlider (Graphics& g, int x, int y, int width, int height,
                                           float sliderPos, float rotaryStartAngle,
                                           float rotaryEndAngle, Slider& slider)
{
    const auto bounds = Rectangle<int> (x, y, width, height).toFloat().reduced (3.0f);
    const auto radius = jmin (bounds.getWidth(), bounds.getHeight()) * 0.5f;
    const auto centre = bounds.getCentre();
    const auto angle  = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);

    const auto arcRadius = radius - 3.0f;
    const auto thickness = jmax (2.5f, radius * 0.11f);

    // Engraved track.
    Path track;
    track.addCentredArc (centre.x, centre.y, arcRadius, arcRadius, 0.0f,
                         rotaryStartAngle, rotaryEndAngle, true);
    g.setColour (Theme::engrave);
    g.strokePath (track, PathStrokeType (thickness, PathStrokeType::curved, PathStrokeType::rounded));

    // Value arc. Bipolar controls fill outward from the centre detent.
    const auto isBipolar = slider.getMinimum() < -0.0001 && slider.getMaximum() > 0.0001
                            && std::abs (slider.getMinimum() + slider.getMaximum()) < 0.0001;
    const auto originAngle = isBipolar ? (rotaryStartAngle + rotaryEndAngle) * 0.5f : rotaryStartAngle;

    if (std::abs (angle - originAngle) > 0.001f)
    {
        Path value;
        value.addCentredArc (centre.x, centre.y, arcRadius, arcRadius, 0.0f,
                             jmin (originAngle, angle), jmax (originAngle, angle), true);

        const auto fill = slider.isEnabled() ? Theme::accent : Theme::textFaint;
        g.setColour (fill.withAlpha (slider.isMouseOverOrDragging() ? 1.0f : 0.88f));
        g.strokePath (value, PathStrokeType (thickness, PathStrokeType::curved, PathStrokeType::rounded));
    }

    // Body: a brushed disc with a soft top light.
    const auto bodyRadius = arcRadius - thickness - 2.5f;
    if (bodyRadius > 2.0f)
    {
        const auto body = Rectangle<float> (bodyRadius * 2.0f, bodyRadius * 2.0f).withCentre (centre);

        g.setGradientFill (ColourGradient (Theme::panelRaised.brighter (0.16f), body.getCentreX(), body.getY(),
                                           Theme::panel.darker (0.35f),         body.getCentreX(), body.getBottom(), false));
        g.fillEllipse (body);

        g.setColour (Theme::hairline);
        g.drawEllipse (body.reduced (0.5f), 1.0f);

        // Pointer.
        const auto tip  = centre.getPointOnCircumference (bodyRadius * 0.82f, angle);
        const auto tail = centre.getPointOnCircumference (bodyRadius * 0.28f, angle);
        g.setColour (Theme::text.withAlpha (slider.isEnabled() ? 0.92f : 0.4f));
        g.drawLine (Line<float> (tail, tip), jmax (1.4f, bodyRadius * 0.09f));

        g.setColour (Theme::accent.withAlpha (0.55f));
        g.fillEllipse (Rectangle<float> (3.0f, 3.0f).withCentre (centre));
    }
}

void DopplerLookAndFeel::drawComboBox (Graphics& g, int width, int height, bool,
                                       int, int, int, int, ComboBox& box)
{
    const auto bounds = Rectangle<float> (0.0f, 0.0f, (float) width, (float) height).reduced (0.5f);
    const auto corner = 4.0f;

    g.setGradientFill (ColourGradient (Theme::panelRaised, 0.0f, 0.0f,
                                       Theme::panel,       0.0f, (float) height, false));
    g.fillRoundedRectangle (bounds, corner);

    g.setColour (box.isMouseOver() ? Theme::accent.withAlpha (0.55f) : Theme::hairline);
    g.drawRoundedRectangle (bounds, corner, 1.0f);

    Path arrow;
    const auto cx = bounds.getRight() - 13.0f;
    const auto cy = bounds.getCentreY();
    arrow.startNewSubPath (cx - 4.0f, cy - 2.0f);
    arrow.lineTo (cx, cy + 2.5f);
    arrow.lineTo (cx + 4.0f, cy - 2.0f);

    g.setColour (Theme::accent.withAlpha (0.9f));
    g.strokePath (arrow, PathStrokeType (1.4f, PathStrokeType::curved, PathStrokeType::rounded));
}

void DopplerLookAndFeel::positionComboBoxText (ComboBox& box, Label& label)
{
    label.setBounds (9, 1, box.getWidth() - 26, box.getHeight() - 2);
    label.setFont (getComboBoxFont (box));
    label.setJustificationType (Justification::centredLeft);
}

void DopplerLookAndFeel::drawPopupMenuBackground (Graphics& g, int width, int height)
{
    const auto bounds = Rectangle<float> (0.0f, 0.0f, (float) width, (float) height);
    g.setColour (Theme::panelRaised);
    g.fillRoundedRectangle (bounds.reduced (0.5f), 4.0f);
    g.setColour (Theme::hairline);
    g.drawRoundedRectangle (bounds.reduced (0.5f), 4.0f, 1.0f);
}

void DopplerLookAndFeel::drawToggleButton (Graphics& g, ToggleButton& button,
                                           bool isHighlighted, bool)
{
    auto bounds = button.getLocalBounds().toFloat();

    const auto trackWidth  = jmin (38.0f, bounds.getWidth());
    const auto trackHeight = jmin (18.0f, bounds.getHeight());
    auto track = Rectangle<float> (trackWidth, trackHeight)
                    .withPosition (bounds.getX(), bounds.getCentreY() - trackHeight * 0.5f);

    const auto on = button.getToggleState();

    g.setColour (on ? Theme::accent.withAlpha (0.28f) : Theme::engrave);
    g.fillRoundedRectangle (track, trackHeight * 0.5f);

    g.setColour (on ? Theme::accent.withAlpha (0.8f)
                    : (isHighlighted ? Theme::hairline.brighter (0.3f) : Theme::hairline));
    g.drawRoundedRectangle (track.reduced (0.5f), trackHeight * 0.5f, 1.0f);

    const auto knobSize = trackHeight - 5.0f;
    const auto knobX    = on ? track.getRight() - knobSize - 2.5f : track.getX() + 2.5f;
    const auto knob     = Rectangle<float> (knobSize, knobSize)
                            .withPosition (knobX, track.getCentreY() - knobSize * 0.5f);

    g.setColour (on ? Theme::accent : Theme::textDim);
    g.fillEllipse (knob);

    const auto textArea = bounds.withTrimmedLeft (trackWidth + 9.0f);
    if (textArea.getWidth() > 8.0f && button.getButtonText().isNotEmpty())
    {
        g.setColour (on ? Theme::text : Theme::textDim);
        g.setFont (Theme::label (11.5f, true));
        g.drawText (button.getButtonText().toUpperCase(), textArea,
                    Justification::centredLeft, true);
    }
}
