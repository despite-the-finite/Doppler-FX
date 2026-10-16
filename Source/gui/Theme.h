#pragma once

#include <juce_graphics/juce_graphics.h>
#include "Brand.h"

/*  Entropic Labs house style: graphite chassis, acid accent, ember telltales.

    Two accents only. Acid is anything you can move or anything currently
    reading true; ember is the instrument telling you about itself — the
    listener, the route, the limiter biting, the pixel in the logo. Everything
    else is graphite and hairlines, so the acid means something when it appears.

    The accents come from Brand.h, which also draws the mark and the two
    textures (dot grid, dissolving rule) every Entropic Labs plugin shares.
*/
namespace Theme
{
    inline const juce::Colour backdropTop    { 0xff121418 };
    inline const juce::Colour backdropBottom { 0xff08090c };
    inline const juce::Colour panel          { 0xff191c21 };
    inline const juce::Colour panelRaised    { 0xff20242b };
    inline const juce::Colour hairline       { 0xff2e333c };
    inline const juce::Colour engrave        { 0xff0d0f12 };

    inline const juce::Colour accent         = Brand::acid;
    inline const juce::Colour accentSoft     = Brand::acid.withAlpha (0.33f);
    inline const juce::Colour ember          = Brand::ember;
    inline const juce::Colour emberSoft      = Brand::ember.withAlpha (0.33f);
    inline const juce::Colour warn           { 0xffff4a3d };

    inline const juce::Colour text           = Brand::paper;
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
}
