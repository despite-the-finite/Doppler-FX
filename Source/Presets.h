#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <vector>
#include <utility>

/*  Factory presets, exposed through the host's program interface so they show
    up in the DAW's own preset menu as well as in the plugin's header.

    A preset lists only what it changes. Loading one first returns every
    parameter to its default, so a preset always sounds the same however the
    plugin was set when you reached for it.
*/
struct Preset
{
    const char* name;
    const char* blurb;      // shown as the tooltip on the preset selector
    std::vector<std::pair<const char*, float>> values;   // parameter ID -> value in real units
};

namespace Presets
{
    const std::vector<Preset>& factory();

    /** Resets everything to default, then applies preset `index`. */
    void apply (juce::AudioProcessorValueTreeState&, int index);
}
