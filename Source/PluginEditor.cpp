#include "PluginEditor.h"
#include "gui/Theme.h"

using namespace juce;

namespace
{
    /*  Lays components out across `area` in equal cells. Cells are capped in
        both directions and the whole row is centred vertically, so a control
        never stretches into a tall panel and leaves its label stranded.
    */
    void layoutRow (Rectangle<int> area, const std::vector<Component*>& items,
                    int maxWidth, int maxHeight, bool centreRow = false)
    {
        if (items.empty())
            return;

        const auto count  = (int) items.size();
        const auto cell   = jmin (maxWidth,  area.getWidth() / count);
        const auto height = jmin (maxHeight, area.getHeight());

        auto row = area.withHeight (height).withY (area.getY() + (area.getHeight() - height) / 2);

        if (centreRow)
            row = row.withWidth (cell * count).withX (area.getX() + (area.getWidth() - cell * count) / 2);

        for (auto* item : items)
            item->setBounds (row.removeFromLeft (cell).reduced (4, 0));
    }

    void setTip (Component& c, const String& tip)
    {
        if (auto* s = dynamic_cast<SettableTooltipClient*> (&c))
            s->setTooltip (tip);
    }
}

DopplerFXAudioProcessorEditor::DopplerFXAudioProcessorEditor (DopplerFXAudioProcessor& p)
    : AudioProcessorEditor (&p), processor (p), radar (p)
{
    setLookAndFeel (&lookAndFeel);

    for (auto* panel : { &motionPanel, &tonePanel, &gatePanel, &outputPanel })
        addAndMakeVisible (panel);

    addAndMakeVisible (radar);
    outputPanel.addAndMakeVisible (meter);

    // ---- Motion -------------------------------------------------------------
    motionMode = &addChoice (motionPanel, ParamID::motionMode, "Path");
    division   = &addChoice (motionPanel, ParamID::rateDiv,    "Division");
    sync       = &addSwitch (motionPanel, ParamID::tempoSync,  "Sync");

    rate       = &addKnob (motionPanel, ParamID::rateHz,     "Rate");
    position   = &addKnob (motionPanel, ParamID::manualPos,  "Position");
    distance   = &addKnob (motionPanel, ParamID::distance,   "Distance");
    path       = &addKnob (motionPanel, ParamID::pathLength, "Path Len");
    dopplerAmt = &addKnob (motionPanel, ParamID::dopplerAmt, "Doppler");
    spread     = &addKnob (motionPanel, ParamID::spread,     "Spread");
    proximity  = &addKnob (motionPanel, ParamID::proximity,  "Proximity");

    // ---- Tone ---------------------------------------------------------------
    filterType = &addChoice (tonePanel, ParamID::filterType, "Filter");
    pitch      = &addKnob (tonePanel, ParamID::pitchSemis,  "Pitch");
    cutoff     = &addKnob (tonePanel, ParamID::cutoff,      "Cutoff");
    resonance  = &addKnob (tonePanel, ParamID::resonance,   "Resonance");
    track      = &addKnob (tonePanel, ParamID::filterTrack, "Track");
    air        = &addKnob (tonePanel, ParamID::airDamp,     "Air");
    feedback   = &addKnob (tonePanel, ParamID::feedback,    "Feedback");

    // ---- Gate ---------------------------------------------------------------
    gateOn      = &addSwitch (gatePanel, ParamID::gateOn,      "Gate");
    gateThresh  = &addKnob (gatePanel, ParamID::gateThresh,  "Threshold");
    gateDepth   = &addKnob (gatePanel, ParamID::gateDepth,   "Depth");
    gateAttack  = &addKnob (gatePanel, ParamID::gateAttack,  "Attack");
    gateRelease = &addKnob (gatePanel, ParamID::gateRelease, "Release");
    gateDuck    = &addKnob (gatePanel, ParamID::gateDuck,    "Duck");

    // ---- Output -------------------------------------------------------------
    mix        = &addKnob (outputPanel, ParamID::mix,        "Mix");
    ceiling    = &addKnob (outputPanel, ParamID::ceiling,    "Ceiling");
    outputGain = &addKnob (outputPanel, ParamID::outputGain, "Output");
    safety     = &addSwitch (outputPanel, ParamID::safety,   "Safety");

    setTip (*dopplerAmt, "How far the propagation delay is allowed to swing. 0% freezes the source; "
                         "100% is physically correct; above that is deliberately exaggerated.");
    setTip (*proximity,  "Depth of the inverse-distance level change. The effect ducks as the source "
                         "recedes, and never rises above the dry level.");
    setTip (*feedback,   "Recirculates the wet signal. Soft-clipped and watched by an automatic guard, "
                         "so it settles instead of howling.");
    setTip (*resonance,  "Filter Q. The resonant peak is gain-compensated as it rises, so it sings "
                         "without eating the track's headroom.");
    setTip (*gateDuck,   "Ties level to distance: the further the source travels, the harder it ducks. "
                         "Turns the motion into a rhythmic chop.");
    setTip (*ceiling,    "Absolute output ceiling. Nothing leaves the plugin above this.");
    setTip (*safety,     "Look-ahead brickwall limiter on the output. Leave this on.");

    setResizable (true, true);
    getConstrainer()->setFixedAspectRatio (980.0 / 770.0);
    setResizeLimits (833, 654, 1470, 1155);
    setSize (980, 770);

    startTimerHz (30);
}

DopplerFXAudioProcessorEditor::~DopplerFXAudioProcessorEditor()
{
    setLookAndFeel (nullptr);
}

Knob& DopplerFXAudioProcessorEditor::addKnob (SectionPanel& panel, const char* paramID,
                                              const String& name)
{
    knobs.push_back (std::make_unique<Knob> (processor.getState(), paramID, name));
    panel.addAndMakeVisible (*knobs.back());
    return *knobs.back();
}

ChoiceBox& DopplerFXAudioProcessorEditor::addChoice (SectionPanel& panel, const char* paramID,
                                                     const String& name)
{
    choices.push_back (std::make_unique<ChoiceBox> (processor.getState(), paramID, name));
    panel.addAndMakeVisible (*choices.back());
    return *choices.back();
}

Switch& DopplerFXAudioProcessorEditor::addSwitch (SectionPanel& panel, const char* paramID,
                                                  const String& name)
{
    switches.push_back (std::make_unique<Switch> (processor.getState(), paramID, name));
    panel.addAndMakeVisible (*switches.back());
    return *switches.back();
}

void DopplerFXAudioProcessorEditor::timerCallback()
{
    meter.setLevels (processor.visuals.outLevel.load(),
                     processor.visuals.gainReduction.load());
}

void DopplerFXAudioProcessorEditor::paint (Graphics& g)
{
    auto bounds = getLocalBounds();

    g.setGradientFill (ColourGradient (Theme::backdropTop,    (float) bounds.getCentreX(), 0.0f,
                                       Theme::backdropBottom, (float) bounds.getCentreX(), (float) bounds.getBottom(),
                                       false));
    g.fillAll();

    // A very soft top-centre glow keeps the flat background from looking dead.
    g.setGradientFill (ColourGradient (Theme::accent.withAlpha (0.055f), (float) bounds.getCentreX(), 0.0f,
                                       Colours::transparentBlack,        (float) bounds.getCentreX(), 280.0f,
                                       false));
    g.fillRect (bounds.removeFromTop (280));

    drawHeader (g, getLocalBounds().removeFromTop (66));
}

void DopplerFXAudioProcessorEditor::drawHeader (Graphics& g, Rectangle<int> area)
{
    auto header = area.reduced (18, 0);

    auto titleArea = header.removeFromLeft (300).toFloat();

    g.setFont (Theme::display (27.0f, true));
    g.setColour (Theme::text);

    const auto titleWidth = GlyphArrangement::getStringWidth (Theme::display (27.0f, true), "DOPPLER ");
    g.drawText ("DOPPLER", titleArea.removeFromLeft (titleWidth + 2.0f),
                Justification::centredLeft, false);

    g.setColour (Theme::accent);
    g.drawText ("FX", titleArea, Justification::centredLeft, false);

    g.setColour (Theme::hairline);
    g.drawHorizontalLine (area.getBottom() - 1,
                          (float) area.getX() + 18.0f, (float) area.getRight() - 18.0f);

    // juce::String treats a bare const char* as ASCII, so the separator has to
    // be spelled out rather than pasted in as a UTF-8 literal.
    const auto dot = String::charToString ((juce_wchar) 0x00b7);
    g.setFont (Theme::label (10.0f, true));
    g.setColour (Theme::textFaint);
    g.drawText ("M O T I O N   " + dot + "   P I T C H   " + dot + "   R E S O N A N C E",
                header.removeFromLeft (340), Justification::centredLeft, false);
}

void DopplerFXAudioProcessorEditor::resized()
{
    auto bounds = getLocalBounds();
    bounds.removeFromTop (66);
    bounds.reduce (14, 0);
    bounds.removeFromBottom (14);

    // ---- Output strip along the bottom -------------------------------------
    outputPanel.setBounds (bounds.removeFromBottom (126));
    bounds.removeFromBottom (10);

    {
        auto content = outputPanel.getContentArea();

        layoutRow (content.removeFromLeft (350), { mix, ceiling, outputGain }, 116, 88);

        content.removeFromLeft (12);
        safety->setBounds (content.removeFromLeft (118).withSizeKeepingCentre (104, 22));

        content.removeFromLeft (12);
        meter.setBounds (content.withSizeKeepingCentre (content.getWidth() - 8, 30));
    }

    // ---- Two columns --------------------------------------------------------
    auto left  = bounds.removeFromLeft (jmax (330, bounds.getWidth() * 39 / 100));
    bounds.removeFromLeft (10);
    auto right = bounds;

    const auto topHeight = jmax (220, left.getHeight() * 49 / 100);

    radar.setBounds (left.removeFromTop (topHeight));
    left.removeFromTop (10);
    gatePanel.setBounds (left);

    motionPanel.setBounds (right.removeFromTop (topHeight));
    right.removeFromTop (10);
    tonePanel.setBounds (right);

    // ---- Motion contents ----------------------------------------------------
    {
        auto content = motionPanel.getContentArea();

        auto topRow = content.removeFromTop (40);
        motionMode->setBounds (topRow.removeFromLeft (150).reduced (2, 0));
        topRow.removeFromLeft (10);
        division->setBounds (topRow.removeFromLeft (120).reduced (2, 0));
        topRow.removeFromLeft (16);
        sync->setBounds (topRow.removeFromLeft (100).withSizeKeepingCentre (90, 22).translated (0, 7));

        const auto rowHeight = content.getHeight() / 2;
        layoutRow (content.removeFromTop (rowHeight), { rate, position, distance, path }, 132, 96);
        layoutRow (content, { dopplerAmt, spread, proximity }, 132, 96, true);
    }

    // ---- Tone contents ------------------------------------------------------
    {
        auto content = tonePanel.getContentArea();

        auto topRow = content.removeFromTop (38);
        filterType->setBounds (topRow.removeFromLeft (170).reduced (2, 0));

        const auto rowHeight = content.getHeight() / 2;
        layoutRow (content.removeFromTop (rowHeight), { pitch, cutoff, resonance }, 168, 96, true);
        layoutRow (content, { track, air, feedback }, 168, 96, true);
    }

    // ---- Gate contents ------------------------------------------------------
    {
        auto content = gatePanel.getContentArea();

        gateOn->setBounds (content.removeFromTop (30).removeFromLeft (110)
                                  .withSizeKeepingCentre (100, 22));

        const auto rowHeight = content.getHeight() / 2;
        layoutRow (content.removeFromTop (rowHeight), { gateThresh, gateDepth, gateAttack }, 128, 96, true);
        layoutRow (content, { gateRelease, gateDuck }, 128, 96, true);
    }
}
