#include "PluginProcessor.h"
#include "PluginEditor.h"

#include <cmath>

using namespace juce;

namespace
{
    inline float softClip (float x) noexcept
    {
        // Cheap tanh-alike: bounded, odd, and flat enough through zero that it
        // stays clean until the feedback path is genuinely being pushed.
        x = jlimit (-3.0f, 3.0f, x);
        return x * (27.0f + x * x) / (27.0f + 9.0f * x * x);
    }

    inline float sanitise (float x) noexcept
    {
        return std::isfinite (x) ? x : 0.0f;
    }

    template <typename T>
    inline T paramValue (AudioProcessorValueTreeState& s, const char* id)
    {
        if (auto* p = s.getRawParameterValue (id))
            return (T) p->load();
        jassertfalse;
        return T();
    }
}

DopplerFXAudioProcessor::DopplerFXAudioProcessor()
    : AudioProcessor (BusesProperties()
                        .withInput  ("Input",  AudioChannelSet::stereo(), true)
                        .withOutput ("Output", AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "DopplerFX", Params::createLayout())
{
}

bool DopplerFXAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto& out = layouts.getMainOutputChannelSet();
    const auto& in  = layouts.getMainInputChannelSet();

    if (out != AudioChannelSet::mono() && out != AudioChannelSet::stereo())
        return false;

    return in == out;
}

void DopplerFXAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;

    doppler.prepare (sampleRate);
    pitchShifter.prepare (sampleRate, 2);
    filter.prepare (sampleRate, 2);
    gate.prepare (sampleRate);
    limiter.prepare (sampleRate, 2, 4.0f);

    const auto ramp = 0.02;   // 20 ms on everything that is not a filter cutoff
    smoothedMix.reset (sampleRate, ramp);
    smoothedOut.reset (sampleRate, ramp);
    smoothedFeedback.reset (sampleRate, ramp);
    smoothedDuck.reset (sampleRate, ramp);
    smoothedCutoff.reset (sampleRate, 0.03);

    smoothedMix.setCurrentAndTargetValue (paramValue<float> (apvts, ParamID::mix) * 0.01f);
    smoothedOut.setCurrentAndTargetValue (Decibels::decibelsToGain (paramValue<float> (apvts, ParamID::outputGain)));
    smoothedFeedback.setCurrentAndTargetValue (paramValue<float> (apvts, ParamID::feedback) * 0.01f);
    smoothedDuck.setCurrentAndTargetValue (paramValue<float> (apvts, ParamID::gateDuck) * 0.01f);
    smoothedCutoff.setCurrentAndTargetValue (jmax (20.0f, paramValue<float> (apvts, ParamID::cutoff)));

    for (auto& d : dcBlockers) d = {};
    feedbackState = { { 0.0f, 0.0f } };
    feedbackEnv = 0.0f;
    feedbackGuard = 1.0f;
    controlCounter = 0;

    setLatencySamples (limiter.getLatencySamples());
    ignoreUnused (samplesPerBlock);
}

void DopplerFXAudioProcessor::releaseResources()
{
    doppler.reset();
    pitchShifter.reset();
    filter.reset();
    gate.reset();
    limiter.reset();
}

void DopplerFXAudioProcessor::updateParameters (double bpm)
{
    // ---- Motion ------------------------------------------------------------
    DopplerEngine::Settings s;
    s.mode       = (int) paramValue<int> (apvts, ParamID::motionMode);
    s.distance   = paramValue<float> (apvts, ParamID::distance);
    s.pathLength = paramValue<float> (apvts, ParamID::pathLength);
    s.depth      = paramValue<float> (apvts, ParamID::dopplerAmt) * 0.01f;
    s.spread     = paramValue<float> (apvts, ParamID::spread)     * 0.01f;
    s.proximity  = paramValue<float> (apvts, ParamID::proximity)  * 0.01f;
    s.manualPos  = paramValue<float> (apvts, ParamID::manualPos);
    doppler.setSettings (s);

    const auto synced = paramValue<float> (apvts, ParamID::tempoSync) > 0.5f;
    const auto rate   = synced ? (float) Params::divisionToHz ((int) paramValue<int> (apvts, ParamID::rateDiv), bpm)
                               : paramValue<float> (apvts, ParamID::rateHz);

    phaseIncrement = (float) (rate / currentSampleRate);
    visuals.rateDisplay.store (rate);

    // How far the source can ever get, used for scaling the display, the
    // filter tracking and the distance duck.
    const auto radius = jlimit (0.1f, 100.0f, s.pathLength * 0.125f);
    maxExtent = s.mode == 1 ? s.distance + 2.0f * radius
                            : std::sqrt (std::pow (s.pathLength * 0.5f, 2.0f) + s.distance * s.distance);
    maxExtent = jmax (s.distance + 0.5f, maxExtent);
    visuals.extent.store (maxExtent);

    // ---- Tone --------------------------------------------------------------
    pitchShifter.setSemitones (paramValue<float> (apvts, ParamID::pitchSemis));

    baseCutoff     = paramValue<float> (apvts, ParamID::cutoff);
    resonanceValue = paramValue<float> (apvts, ParamID::resonance) * 0.01f;
    trackAmount    = paramValue<float> (apvts, ParamID::filterTrack) * 0.01f;
    airAmount      = paramValue<float> (apvts, ParamID::airDamp) * 0.01f;
    filterType     = (ResonantFilter::Type) paramValue<int> (apvts, ParamID::filterType);

    smoothedCutoff.setTargetValue (jmax (20.0f, baseCutoff));
    smoothedFeedback.setTargetValue (paramValue<float> (apvts, ParamID::feedback) * 0.01f);

    // ---- Gate --------------------------------------------------------------
    gate.setEnabled  (paramValue<float> (apvts, ParamID::gateOn) > 0.5f);
    gate.setThreshold (paramValue<float> (apvts, ParamID::gateThresh));
    gate.setDepth     (paramValue<float> (apvts, ParamID::gateDepth) * 0.01f);
    gate.setAttack    (paramValue<float> (apvts, ParamID::gateAttack));
    gate.setRelease   (paramValue<float> (apvts, ParamID::gateRelease));
    smoothedDuck.setTargetValue (paramValue<float> (apvts, ParamID::gateDuck) * 0.01f);

    // ---- Output ------------------------------------------------------------
    smoothedMix.setTargetValue (paramValue<float> (apvts, ParamID::mix) * 0.01f);
    smoothedOut.setTargetValue (Decibels::decibelsToGain (paramValue<float> (apvts, ParamID::outputGain)));
    limiter.setEnabled  (paramValue<float> (apvts, ParamID::safety) > 0.5f);
    limiter.setCeilingDb (paramValue<float> (apvts, ParamID::ceiling));
}

void DopplerFXAudioProcessor::syncToHost()
{
    if (paramValue<float> (apvts, ParamID::tempoSync) < 0.5f)
        return;

    auto* ph = getPlayHead();
    if (ph == nullptr)
        return;

    const auto pos = ph->getPosition();
    if (! pos.hasValue() || ! pos->getIsPlaying())
        return;

    const auto ppq = pos->getPpqPosition();
    if (! ppq.hasValue())
        return;

    const auto& divs = Params::syncDivisions();
    const auto  idx  = (size_t) jlimit (0, (int) divs.size() - 1,
                                        (int) paramValue<int> (apvts, ParamID::rateDiv));
    const auto  beatsPerCycle = divs[idx].wholeNotes * 4.0;

    if (beatsPerCycle > 1.0e-6)
    {
        auto p = *ppq / beatsPerCycle;
        p -= std::floor (p);
        doppler.setPhase ((float) p);
    }
}

void DopplerFXAudioProcessor::processBlock (AudioBuffer<float>& buffer, MidiBuffer&)
{
    ScopedNoDenormals noDenormals;

    const auto numSamples  = buffer.getNumSamples();
    const auto numIn       = getTotalNumInputChannels();
    const auto numOut      = getTotalNumOutputChannels();

    for (int ch = numIn; ch < numOut; ++ch)
        buffer.clear (ch, 0, numSamples);

    if (numIn == 0 || numOut == 0 || numSamples == 0)
        return;

    double bpm = 120.0;
    if (auto* ph = getPlayHead())
        if (const auto pos = ph->getPosition())
            if (const auto hostBpm = pos->getBpm())
                bpm = *hostBpm;

    updateParameters (bpm);
    syncToHost();

    const auto stereoIn  = numIn  > 1;
    const auto stereoOut = numOut > 1;

    auto* inL  = buffer.getReadPointer (0);
    auto* inR  = stereoIn ? buffer.getReadPointer (1) : inL;
    auto* outL = buffer.getWritePointer (0);
    auto* outR = stereoOut ? buffer.getWritePointer (1) : nullptr;

    // Hoisted out of the loop: these are block-rate values, and an atomic load
    // per sample per parameter is real CPU for no benefit.
    const auto closestDistance = jmax (0.2f, paramValue<float> (apvts, ParamID::distance));
    const auto distanceSpan    = jmax (0.001f, maxExtent - closestDistance);

    float peakIn = 0.0f, peakOut = 0.0f;
    DopplerEngine::Frame frame;
    frame.distance = closestDistance;

    for (int n = 0; n < numSamples; ++n)
    {
        // Sanitise at the door. Hosts should never hand us NaN, but an
        // upstream plugin misbehaving once should not leave this one dead for
        // the rest of the session.
        const auto dryL = sanitise (inL[n]);
        const auto dryR = sanitise (inR[n]);

        peakIn = jmax (peakIn, std::abs (dryL), std::abs (dryR));

        // ---- Motion for this sample ---------------------------------------
        frame = doppler.tick (phaseIncrement);

        const auto normDistance = jlimit (0.0f, 1.0f,
                                          (frame.distance - closestDistance) / distanceSpan);

        // ---- Control rate coefficient updates ------------------------------
        if (controlCounter-- <= 0)
        {
            controlCounter = controlInterval;

            // Cutoff follows the source: bright as it arrives, dark as it goes
            // (or the other way round, with a negative Track value).
            const auto trackOctaves = trackAmount * (0.5f - normDistance) * 4.0f;
            const auto tracked = smoothedCutoff.getCurrentValue() * std::pow (2.0f, trackOctaves);
            filter.setCoefficients (jlimit (20.0f, (float) currentSampleRate * 0.49f, tracked),
                                    resonanceValue, filterType);

            // Air absorption: high frequencies thin out with distance.
            const auto airHz = 20000.0f * std::exp (-airAmount * frame.distance / 8.0f);
            filter.setAirCutoff (jmax (300.0f, airHz));
        }
        smoothedCutoff.getNextValue();

        // ---- Gate ----------------------------------------------------------
        const auto gateG = gate.processGain (dryL, dryR);
        const auto duckG = Gate::duckGain (normDistance, smoothedDuck.getNextValue());

        // ---- Feedback, bounded before it can become a problem --------------
        const auto fbAmount = smoothedFeedback.getNextValue() * feedbackGuard;

        const auto srcL = dryL * gateG + softClip (feedbackState[0] * fbAmount);
        const auto srcR = dryR * gateG + softClip (feedbackState[1] * fbAmount);

        doppler.push (0, srcL);
        doppler.push (1, srcR);

        auto wetL = doppler.read (0, frame.delaySamples[0]) * frame.gain[0];
        auto wetR = doppler.read (1, frame.delaySamples[1]) * frame.gain[1];

        // ---- Pitch, filter, air --------------------------------------------
        wetL = pitchShifter.process (0, wetL);
        wetR = pitchShifter.process (1, wetR);

        wetL = filter.process (0, wetL);
        wetR = filter.process (1, wetR);

        wetL *= duckG;
        wetR *= duckG;

        feedbackState[0] = sanitise (wetL);
        feedbackState[1] = sanitise (wetR);

        // Resonance guard: if the recirculating signal starts to build, ease
        // the feedback back down until it settles. Recovery is slow enough not
        // to be heard as pumping, clamp-down fast enough to stop a howl.
        const auto fbPeak = jmax (std::abs (feedbackState[0]), std::abs (feedbackState[1]));
        feedbackEnv += (fbPeak - feedbackEnv) * (fbPeak > feedbackEnv ? 0.02f : 0.0002f);

        if (feedbackEnv > 0.9f)
            feedbackGuard = jmax (0.15f, feedbackGuard - 0.0006f);
        else if (feedbackEnv < 0.7f)
            feedbackGuard = jmin (1.0f, feedbackGuard + 0.00002f);

        // ---- Mix and output -------------------------------------------------
        const auto mix = smoothedMix.getNextValue();
        const auto out = smoothedOut.getNextValue();

        auto mixedL = (dryL * (1.0f - mix) + wetL * mix) * out;
        auto mixedR = (dryR * (1.0f - mix) + wetR * mix) * out;

        // DC blocker: a slow moving delay line can leave an offset behind, and
        // an offset is headroom you paid for and never hear.
        for (int ch = 0; ch < 2; ++ch)
        {
            auto& d = dcBlockers[(size_t) ch];
            auto& x = ch == 0 ? mixedL : mixedR;
            const auto y = x - d.x1 + 0.9995f * d.y1;
            d.x1 = x;
            d.y1 = y;
            x = sanitise (y);
        }

        limiter.processFrame (mixedL, mixedR);

        peakOut = jmax (peakOut, std::abs (mixedL), std::abs (mixedR));

        if (outR != nullptr)
        {
            outL[n] = mixedL;
            outR[n] = mixedR;
        }
        else
        {
            outL[n] = 0.5f * (mixedL + mixedR);   // fold down for a mono track
        }
    }

    // ---- Feed the display ---------------------------------------------------
    visuals.sourceX.store (frame.sourceX);
    visuals.sourceY.store (frame.sourceY);
    visuals.distance.store (frame.distance);
    visuals.radialVelocity.store (frame.radialVelocity);
    visuals.gateGain.store (gate.getCurrentGain());
    visuals.gainReduction.store (limiter.readGainReduction());
    visuals.inLevel.store (peakIn);
    visuals.outLevel.store (peakOut);
}

void DopplerFXAudioProcessor::setCurrentProgram (int index)
{
    const auto& bank = Presets::factory();
    if (! isPositiveAndBelow (index, (int) bank.size()))
        return;

    currentProgram = index;
    Presets::apply (apvts, index);
    updateHostDisplay();
}

const String DopplerFXAudioProcessor::getProgramName (int index)
{
    const auto& bank = Presets::factory();
    return isPositiveAndBelow (index, (int) bank.size()) ? String (bank[(size_t) index].name)
                                                         : String();
}

AudioProcessorEditor* DopplerFXAudioProcessor::createEditor()
{
    return new DopplerFXAudioProcessorEditor (*this);
}

void DopplerFXAudioProcessor::getStateInformation (MemoryBlock& destData)
{
    auto state = apvts.copyState();
    state.setProperty ("currentProgram", currentProgram, nullptr);

    if (auto xml = state.createXml())
        copyXmlToBinary (*xml, destData);
}

void DopplerFXAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
    {
        if (xml->hasTagName (apvts.state.getType()))
        {
            const auto state = ValueTree::fromXml (*xml);
            apvts.replaceState (state);

            // Only a label: the parameter values themselves were just restored,
            // so this must not re-apply the preset over the top of them.
            currentProgram = jlimit (0, (int) Presets::factory().size() - 1,
                                     (int) state.getProperty ("currentProgram", 0));
        }
    }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new DopplerFXAudioProcessor();
}
