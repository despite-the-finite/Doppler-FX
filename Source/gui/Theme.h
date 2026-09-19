#pragma once

#include <juce_graphics/juce_graphics.h>

/*  Entropic Labs house style: graphite chassis, acid hazard accent.

    Two accents only. Acid is anything you can move or anything currently
    reading true; ember is the instrument telling you about itself — the
    listener, the route, the limiter biting. Everything else is graphite and
    hairlines, so the acid means something when it appears.
*/
namespace Theme
{
    inline const juce::Colour backdropTop    { 0xff121418 };
    inline const juce::Colour backdropBottom { 0xff08090c };
    inline const juce::Colour panel          { 0xff191c21 };
    inline const juce::Colour panelRaised    { 0xff20242b };
    inline const juce::Colour hairline       { 0xff2e333c };
    inline const juce::Colour engrave        { 0xff0d0f12 };

    inline const juce::Colour accent         { 0xffd9ff3d };   // acid
    inline const juce::Colour accentSoft     { 0x55d9ff3d };
    inline const juce::Colour ember          { 0xffff6b3d };   // ember
    inline const juce::Colour emberSoft      { 0x55ff6b3d };
    inline const juce::Colour warn           { 0xffff4a3d };

    inline const juce::Colour text           { 0xffd5d8d0 };
    inline const juce::Colour textDim        { 0xff868d92 };
    inline const juce::Colour textFaint      { 0xff585e66 };

    /** Stencil-style face: heavy sans, only ever used in capitals. */
    inline juce::Font stencil (float height)
    {
        return juce::Font (juce::FontOptions()
                             .withName (juce::Font::getDefaultSansSerifFontName())
                             .withHeight (height)
                             .withStyle ("Bold"));
    }

    inline juce::Font label (float height, bool bold = false)
    {
        return juce::Font (juce::FontOptions()
                             .withName (juce::Font::getDefaultSansSerifFontName())
                             .withHeight (height)
                             .withStyle (bold ? "Bold" : "Regular"));
    }

    /** Upper-cases and letter-spaces, which is what sells the stencil look at
        the small sizes the panel captions use. */
    inline juce::String spaced (juce::StringRef textToSpace)
    {
        juce::String out;
        for (auto c : juce::String (textToSpace).toUpperCase())
            out << c << ' ';
        return out.trimEnd();
    }

    /** Diagonal hazard hatching, clipped to `area`. Kept faint: it is trim,
        not a warning anyone needs to read. */
    inline void hazard (juce::Graphics& g, juce::Rectangle<float> area,
                        juce::Colour colour, float alpha = 0.16f, float spacing = 11.0f)
    {
        if (area.isEmpty())
            return;

        juce::Graphics::ScopedSaveState saved (g);
        g.reduceClipRegion (area.toNearestInt());
        g.setColour (colour.withAlpha (alpha));

        const auto h = area.getHeight();
        for (auto x = area.getX() - h; x < area.getRight() + h; x += spacing)
            g.drawLine (x, area.getBottom(), x + h, area.getY(), spacing * 0.34f);
    }
}
