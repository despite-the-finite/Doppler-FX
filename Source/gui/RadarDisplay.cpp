#include "RadarDisplay.h"
#include "../Parameters.h"

using namespace juce;

RadarDisplay::RadarDisplay (DopplerFXAudioProcessor& p)
    : processor (p)
{
    setOpaque (false);
    startTimerHz (30);
}

void RadarDisplay::timerCallback()
{
    const auto extent = jmax (1.0f, processor.visuals.extent.load());
    smoothedExtent += (extent - smoothedExtent) * 0.15f;

    const auto vel = processor.visuals.radialVelocity.load();
    smoothedVelocity += (vel - smoothedVelocity) * 0.25f;

    auto& state = processor.getState();
    auto value = [&state] (const char* id)
    {
        auto* p = state.getRawParameterValue (id);
        return p != nullptr ? p->load() : 0.0f;
    };

    smoothedSpread += (value (ParamID::spread) * 0.01f - smoothedSpread) * 0.2f;

    pathSettings.mode       = (int) value (ParamID::motionMode);
    pathSettings.distance   = value (ParamID::distance);
    pathSettings.pathLength = value (ParamID::pathLength);
    pathSettings.manualPos  = value (ParamID::manualPos);

    trail[(size_t) trailHead] = { processor.visuals.sourceX.load(),
                                  processor.visuals.sourceY.load() };
    trailHead = (trailHead + 1) % trailLength;
    trailFilled = jmin (trailFilled + 1, trailLength);

    repaint();
}

void RadarDisplay::paint (Graphics& g)
{
    auto bounds = getLocalBounds().toFloat().reduced (0.5f);

    g.setGradientFill (ColourGradient (Theme::panel.brighter (0.04f), bounds.getCentreX(), bounds.getY(),
                                       Theme::engrave,                bounds.getCentreX(), bounds.getBottom(), false));
    g.fillRoundedRectangle (bounds, 6.0f);

    const auto readoutHeight = 30.0f;
    auto field = bounds.reduced (10.0f).withTrimmedBottom (readoutHeight);

    // World -> screen. The listener sits low and centred, looking "up" the panel.
    const auto listener = Point<float> (field.getCentreX(), field.getBottom() - 16.0f);
    const auto extent   = jmax (1.0f, smoothedExtent);
    const auto scale    = jmin (field.getWidth() * 0.5f, field.getHeight() - 24.0f) / extent;

    auto toScreen = [&] (float wx, float wy)
    {
        return Point<float> (listener.x + wx * scale, listener.y - wy * scale);
    };

    Graphics::ScopedSaveState saved (g);
    {
        Path clip;
        clip.addRoundedRectangle (bounds, 6.0f);
        g.reduceClipRegion (clip);
    }

    // ---- Range rings --------------------------------------------------------
    g.setFont (Theme::label (9.0f));
    for (int i = 1; i <= 4; ++i)
    {
        const auto ringMetres = extent * (float) i / 4.0f;
        const auto r = ringMetres * scale;

        g.setColour (Theme::hairline.withAlpha (i == 4 ? 0.55f : 0.35f));
        g.drawEllipse (Rectangle<float> (r * 2.0f, r * 2.0f).withCentre (listener), 1.0f);

        g.setColour (Theme::textFaint.withAlpha (0.7f));
        g.drawText (String (ringMetres, ringMetres < 10.0f ? 1 : 0) + " m",
                    Rectangle<float> (listener.x + 4.0f, listener.y - r - 11.0f, 46.0f, 11.0f),
                    Justification::centredLeft, false);
    }

    // Centre cross-hairs.
    g.setColour (Theme::hairline.withAlpha (0.3f));
    g.drawVerticalLine ((int) listener.x, field.getY(), listener.y);
    g.drawHorizontalLine ((int) listener.y, field.getX(), field.getRight());

    // ---- Listener -----------------------------------------------------------
    const auto earHalf = (0.02f + smoothedSpread * 1.4f) * scale;
    g.setColour (Theme::ember.withAlpha (0.35f));
    g.drawLine (listener.x - earHalf, listener.y, listener.x + earHalf, listener.y, 1.2f);

    for (auto side : { -1.0f, 1.0f })
    {
        const auto ear = Point<float> (listener.x + side * earHalf, listener.y);
        g.setColour (Theme::ember.withAlpha (0.85f));
        g.fillEllipse (Rectangle<float> (4.0f, 4.0f).withCentre (ear));
    }

    g.setColour (Theme::ember);
    g.drawEllipse (Rectangle<float> (11.0f, 11.0f).withCentre (listener), 1.4f);

    // ---- The route the source will take -------------------------------------
    {
        Path route;
        constexpr int steps = 96;

        for (int i = 0; i <= steps; ++i)
        {
            float wx = 0.0f, wy = 0.0f;
            DopplerEngine::positionAt (pathSettings, (float) i / (float) steps, wx, wy);

            const auto pt = toScreen (wx, wy);
            if (i == 0) route.startNewSubPath (pt);
            else        route.lineTo (pt);
        }

        // Manual mode has no route to speak of: the knob is the position.
        if (pathSettings.mode != 3)
        {
            g.setColour (Theme::ember.withAlpha (0.30f));
            g.strokePath (route, PathStrokeType (1.0f));
        }
    }

    // ---- Comet trail --------------------------------------------------------
    for (int i = 0; i < trailFilled; ++i)
    {
        const auto idx = (trailHead - 1 - i + trailLength * 2) % trailLength;
        const auto age = (float) i / (float) trailLength;
        const auto pt  = toScreen (trail[(size_t) idx].x, trail[(size_t) idx].y);

        const auto alpha = (1.0f - age) * (1.0f - age) * 0.55f;
        const auto size  = 1.5f + (1.0f - age) * 3.0f;

        g.setColour (Theme::accent.withAlpha (alpha));
        g.fillEllipse (Rectangle<float> (size, size).withCentre (pt));
    }

    // ---- Source -------------------------------------------------------------
    const auto source = toScreen (processor.visuals.sourceX.load(),
                                  processor.visuals.sourceY.load());

    g.setGradientFill (ColourGradient (Theme::accent.withAlpha (0.42f), source.x, source.y,
                                       Theme::accent.withAlpha (0.0f),  source.x + 16.0f, source.y, true));
    g.fillEllipse (Rectangle<float> (32.0f, 32.0f).withCentre (source));

    g.setColour (Theme::accent);
    g.fillEllipse (Rectangle<float> (8.0f, 8.0f).withCentre (source));
    g.setColour (Theme::text.withAlpha (0.9f));
    g.fillEllipse (Rectangle<float> (3.0f, 3.0f).withCentre (source));

    // ---- Readout ------------------------------------------------------------
    // Pitch the geometry is producing right now: f' = f * c / (c + v_radial).
    const auto c     = DopplerEngine::speedOfSound;
    const auto ratio = c / jmax (1.0f, c + jlimit (-300.0f, 300.0f, smoothedVelocity));
    const auto semis = 12.0f * std::log2 (jmax (0.05f, ratio));

    auto readout = bounds.reduced (12.0f, 0.0f).removeFromBottom (readoutHeight);

    auto cell = [&] (Rectangle<float> area, const String& caption, const String& value, Colour colour)
    {
        g.setFont (Theme::label (9.0f, true));
        g.setColour (Theme::textFaint);
        g.drawText (caption, area.removeFromTop (11.0f), Justification::centredLeft, false);

        g.setFont (Theme::label (13.0f));
        g.setColour (colour);
        g.drawText (value, area, Justification::centredLeft, false);
    };

    const auto cellWidth = readout.getWidth() / 3.0f;

    cell (readout.removeFromLeft (cellWidth), "DISTANCE",
          String (processor.visuals.distance.load(), 1) + " m", Theme::text);

    cell (readout.removeFromLeft (cellWidth), "SHIFT",
          (semis >= 0.0f ? "+" : "") + String (semis, 2) + " st",
          std::abs (semis) > 0.05f ? Theme::accent : Theme::textDim);

    cell (readout, "RATE",
          String (processor.visuals.rateDisplay.load(), 2) + " Hz", Theme::text);

    // ---- Frame --------------------------------------------------------------
    g.setColour (Theme::hairline);
    g.drawRoundedRectangle (bounds, 6.0f, 1.0f);
}
