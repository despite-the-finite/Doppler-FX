#include "PluginEditor.h"
#include "gui/Theme.h"
#include "Presets.h"

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

    buildPresetSelector();

    setResizable (true, true);
    getConstrainer()->setFixedAspectRatio (980.0 / 780.0);
    setResizeLimits (833, 663, 1470, 1170);
    setSize (980, 780);

    startTimerHz (30);
}

DopplerFXAudioProcessorEditor::~DopplerFXAudioProcessorEditor()
{
    setLookAndFeel (nullptr);
}

void DopplerFXAudioProcessorEditor::buildPresetSelector()
{
    const auto& bank = Presets::factory();

    for (int i = 0; i < (int) bank.size(); ++i)
        presetBox.addItem (bank[(size_t) i].name, i + 1);

    presetBox.setTextWhenNothingSelected ("PRESET");
    presetBox.onChange = [this]
    {
        const auto index = presetBox.getSelectedId() - 1;
        if (index >= 0 && index != processor.getCurrentProgram())
            processor.setCurrentProgram (index);

        if (const auto& bank2 = Presets::factory(); isPositiveAndBelow (index, (int) bank2.size()))
            presetBox.setTooltip (bank2[(size_t) index].blurb);
    };
    addAndMakeVisible (presetBox);

    for (auto* button : { &prevPreset, &nextPreset })
    {
        button->setTooltip ("Step through the factory presets");
        addAndMakeVisible (button);
    }

    prevPreset.onClick = [this] { stepPreset (-1); };
    nextPreset.onClick = [this] { stepPreset (1); };

    refreshPresetSelector();
}

void DopplerFXAudioProcessorEditor::stepPreset (int delta)
{
    const auto count = (int) Presets::factory().size();
    if (count <= 0)
        return;

    // Wrap, so holding one button walks the whole bank.
    const auto next = (processor.getCurrentProgram() + delta + count) % count;
    processor.setCurrentProgram (next);
    refreshPresetSelector();
}

void DopplerFXAudioProcessorEditor::refreshPresetSelector()
{
    const auto program = processor.getCurrentProgram();
    if (program == shownProgram)
        return;

    shownProgram = program;
    presetBox.setSelectedId (program + 1, dontSendNotification);

    if (const auto& bank = Presets::factory(); isPositiveAndBelow (program, (int) bank.size()))
        presetBox.setTooltip (bank[(size_t) program].blurb);
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

    refreshPresetSelector();
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

    drawHeader (g, getLocalBounds().removeFromTop (headerHeight));
}

void DopplerFXAudioProcessorEditor::drawHeader (Graphics& g, Rectangle<int> area)
{
    // Hazard trim along the bottom edge of the header. Faint: it is trim, not
    // a warning, and it has to sit under twenty knobs without shouting.
    auto stripe = area.removeFromBottom (6).toFloat();
    Theme::hazard (g, stripe, Theme::accent, 0.14f);
    g.setColour (Theme::hairline);
    g.drawHorizontalLine (area.getBottom(), (float) area.getX(), (float) area.getRight());

    auto header = area.reduced (18, 0);
    auto titleArea = header.removeFromLeft (330);

    g.setFont (Theme::stencil (9.0f));
    g.setColour (Theme::accent.withAlpha (0.85f));
    g.drawText (Theme::spaced ("Entropic Labs"),
                titleArea.removeFromTop (32).withTrimmedTop (14),
                Justification::topLeft, false);

    auto nameArea = titleArea.toFloat();
    const auto titleFont = Theme::stencil (25.0f);
    g.setFont (titleFont);

    const auto titleWidth = GlyphArrangement::getStringWidth (titleFont, "DOPPLER ");
    g.setColour (Theme::text);
    g.drawText ("DOPPLER", nameArea.removeFromLeft (titleWidth + 2.0f),
                Justification::topLeft, false);

    g.setColour (Theme::accent);
    g.drawText ("FX", nameArea, Justification::topLeft, false);

    // Unit plate, right of the wordmark and left of the preset bar.
    auto plate = header.removeFromLeft (150).toFloat().reduced (0.0f, 22.0f);
    if (plate.getWidth() > 40.0f)
    {
        g.setColour (Theme::hairline);
        g.drawRoundedRectangle (plate, 3.0f, 1.0f);
        g.setFont (Theme::label (9.0f, true));
        g.setColour (Theme::textFaint);
        g.drawText (Theme::spaced ("Mod 01 / Rev A"), plate, Justification::centred, false);
    }
}

void DopplerFXAudioProcessorEditor::resized()
{
    auto bounds = getLocalBounds();

    // ---- Preset bar, right-hand end of the header ---------------------------
    {
        auto header = bounds.removeFromTop (headerHeight).reduced (18, 0);
        header.removeFromBottom (6);                       // the hazard stripe

        auto bar = header.removeFromRight (jmin (300, header.getWidth() / 2))
                         .withSizeKeepingCentre (jmin (300, header.getWidth() / 2), 26);

        nextPreset.setBounds (bar.removeFromRight (28));
        bar.removeFromRight (4);
        prevPreset.setBounds (bar.removeFromRight (28));
        bar.removeFromRight (8);
        presetBox.setBounds (bar);
    }

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
