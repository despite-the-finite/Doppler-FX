#pragma once

#include <juce_graphics/juce_graphics.h>

/*  One place for the palette and the type, so the whole panel reads as a
    single object rather than a pile of widgets. Deep graphite, warm champagne
    for anything you can move, cool steel for anything the plugin is telling
    you about itself.
*/
namespace Theme
{
    inline const juce::Colour backdropTop    { 0xff17191e };
    inline const juce::Colour backdropBottom { 0xff0a0b0e };
    inline const juce::Colour panel          { 0xff1b1e24 };
    inline const juce::Colour panelRaised    { 0xff222630 };
    inline const juce::Colour hairline       { 0xff2e333c };
    inline const juce::Colour engrave        { 0xff101216 };

    inline const juce::Colour accent         { 0xffc9a96a };   // champagne
    inline const juce::Colour accentSoft     { 0x55c9a96a };
    inline const juce::Colour cool           { 0xff7fa6bd };   // steel blue
    inline const juce::Colour coolSoft       { 0x557fa6bd };
    inline const juce::Colour warn           { 0xffc86a5a };

    inline const juce::Colour text           { 0xffdad6cd };
    inline const juce::Colour textDim        { 0xff8a9099 };
    inline const juce::Colour textFaint      { 0xff5b6068 };

    inline juce::Font display (float height, bool bold = false)
    {
        return juce::Font (juce::FontOptions()
                             .withName (juce::Font::getDefaultSerifFontName())
                             .withHeight (height)
                             .withStyle (bold ? "Bold" : "Regular"));
    }

    inline juce::Font label (float height, bool bold = false)
    {
        return juce::Font (juce::FontOptions()
                             .withName (juce::Font::getDefaultSansSerifFontName())
                             .withHeight (height)
                             .withStyle (bold ? "Bold" : "Regular"));
    }
}
