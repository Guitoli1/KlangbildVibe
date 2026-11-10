#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "ScaledEditor.h"

namespace
{
using APF = juce::AudioParameterFloat;
using Attr = juce::AudioParameterFloatAttributes;
Attr withText (std::function<juce::String (float)> f)
{
    return Attr().withStringFromValueFunction ([f] (float v, int) { return f (v); })
                 .withValueFromStringFunction ([] (const juce::String& s) { return s.getFloatValue(); });
}
}

juce::AudioProcessorValueTreeState::ParameterLayout VibeAudioProcessor::createLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout l;
    l.add (std::make_unique<juce::AudioParameterChoice> (juce::ParameterID { "mode", 1 }, "Modus",
        juce::StringArray { "Tremolo", "Harmonisch", "Vibe Chorus", "Vibe Vibrato" }, 0));
    l.add (std::make_unique<APF> (juce::ParameterID { "speed", 1 }, "Geschwindigkeit", juce::NormalisableRange<float> (0.3f, 12.0f, 0.01f, 0.5f), 5.0f,
        withText ([] (float v) { return juce::String (v, 2) + " Hz"; })));
    l.add (std::make_unique<juce::AudioParameterBool> (juce::ParameterID { "sync", 1 }, "Sync", false));
    l.add (std::make_unique<juce::AudioParameterChoice> (juce::ParameterID { "division", 1 }, "Notenwert",
        juce::StringArray { "1/2", "1/4", "1/4 punktiert", "1/8", "1/8 Triole", "1/16" }, 3));
    l.add (std::make_unique<APF> (juce::ParameterID { "depth", 1 }, "Tiefe", juce::NormalisableRange<float> (0.0f, 100.0f, 0.1f), 50.0f,
        withText ([] (float v) { return juce::String (juce::roundToInt (v)) + " %"; })));
    l.add (std::make_unique<APF> (juce::ParameterID { "shape", 1 }, "Form", juce::NormalisableRange<float> (0.0f, 10.0f, 0.01f), 2.0f,
        withText ([] (float v) { return juce::String (v, 1); })));
    l.add (std::make_unique<juce::AudioParameterBool> (juce::ParameterID { "stereo", 1 }, "Stereo", false));
    l.add (std::make_unique<APF> (juce::ParameterID { "out", 1 }, "Ausgang", juce::NormalisableRange<float> (-12.0f, 12.0f, 0.1f), 0.0f,
        withText ([] (float v) { return (v > 0 ? "+" : "") + juce::String (v, 1) + " dB"; })));
    return l;
}

const std::vector<VibeAudioProcessor::Preset>& VibeAudioProcessor::presets()
{
    static const std::vector<Preset> p = {
        { "Standard",                          { { "mode", 0 }, { "speed", 5.0f }, { "sync", 0 }, { "division", 3 }, { "depth", 50 }, { "shape", 2 }, { "stereo", 0 }, { "out", 0 } } },
        { "Amp-Tremolo \xe2\x80\x93 klassisch", { { "mode", 0 }, { "speed", 5.5f }, { "sync", 0 }, { "division", 3 }, { "depth", 60 }, { "shape", 3 }, { "stereo", 0 }, { "out", 0 } } },
        { "Surf \xe2\x80\x93 schnell & tief",   { { "mode", 0 }, { "speed", 7.5f }, { "sync", 0 }, { "division", 3 }, { "depth", 85 }, { "shape", 5 }, { "stereo", 0 }, { "out", 0 } } },
        { "Harmonisch \xe2\x80\x93 Brownface",  { { "mode", 1 }, { "speed", 4.0f }, { "sync", 0 }, { "division", 3 }, { "depth", 75 }, { "shape", 2 }, { "stereo", 0 }, { "out", 1.0f } } },
        { "Vibe \xe2\x80\x93 Chorus",           { { "mode", 2 }, { "speed", 3.0f }, { "sync", 0 }, { "division", 3 }, { "depth", 70 }, { "shape", 2 }, { "stereo", 0 }, { "out", 0 } } },
        { "Vibe \xe2\x80\x93 Vibrato",          { { "mode", 3 }, { "speed", 4.5f }, { "sync", 0 }, { "division", 3 }, { "depth", 55 }, { "shape", 2 }, { "stereo", 0 }, { "out", 0 } } },
        { "Soul \xe2\x80\x93 langsamer Vibe",   { { "mode", 2 }, { "speed", 1.2f }, { "sync", 0 }, { "division", 3 }, { "depth", 55 }, { "shape", 2 }, { "stereo", 1 }, { "out", 0 } } },
        { "Stereo-Pendel",                     { { "mode", 0 }, { "speed", 3.0f }, { "sync", 0 }, { "division", 3 }, { "depth", 100 }, { "shape", 1 }, { "stereo", 1 }, { "out", 0 } } },
        { "Im Takt \xe2\x80\x93 Achtel",        { { "mode", 0 }, { "speed", 5.0f }, { "sync", 1 }, { "division", 3 }, { "depth", 70 }, { "shape", 6 }, { "stereo", 0 }, { "out", 0 } } },
    };
    return p;
}

void VibeAudioProcessor::setCurrentProgram (int index)
{
    if (! juce::isPositiveAndBelow (index, (int) presets().size()))
        return;
    currentProgram = index;
    for (auto& [id, value] : presets()[(size_t) index].values)
        if (auto* param = apvts.getParameter (id))
            param->setValueNotifyingHost (param->convertTo0to1 (value));
}

const juce::String VibeAudioProcessor::getProgramName (int index)
{
    return juce::isPositiveAndBelow (index, (int) presets().size())
               ? juce::String (juce::CharPointer_UTF8 (presets()[(size_t) index].name)) : juce::String();
}

VibeAudioProcessor::VibeAudioProcessor()
    : AudioProcessor (BusesProperties().withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "KlangbildVibe", createLayout())
{
}

bool VibeAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto in = layouts.getMainInputChannelSet(), out = layouts.getMainOutputChannelSet();
    if (out != juce::AudioChannelSet::mono() && out != juce::AudioChannelSet::stereo())
        return false;
    return in == out || (in == juce::AudioChannelSet::mono() && out == juce::AudioChannelSet::stereo());
}

void VibeAudioProcessor::prepareToPlay (double sampleRate, int)
{
    sr = sampleRate;
    for (auto& v : vibes) v.prepare (sr);
    xoLp = eq::makeLowPass (sr, 650.0, 0.70710678);
    for (auto* a : { &xo1, &xo2 }) for (auto& s : *a) s.reset();
    lastMode = -1;
    for (auto* s : { &smDepth, &smShape, &smOut, &smStereo })
        s->reset (sr, 0.03);
    smDepth.setCurrentAndTargetValue (apvts.getRawParameterValue ("depth")->load() * 0.01f);
    smShape.setCurrentAndTargetValue (apvts.getRawParameterValue ("shape")->load() * 0.1f);
    smOut.setCurrentAndTargetValue (juce::Decibels::decibelsToGain (apvts.getRawParameterValue ("out")->load()));
    smStereo.setCurrentAndTargetValue (apvts.getRawParameterValue ("stereo")->load());
    vuCoef = (float) (1.0 - std::exp (-1.0 / (0.01 * sr)));
    vuState = 0;
    phase = 0;
}

void VibeAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    const int n = buffer.getNumSamples();
    const int inCh = getTotalNumInputChannels(), outCh = juce::jmin (2, getTotalNumOutputChannels());
    if (n == 0 || outCh == 0)
        return;
    if (inCh == 1 && outCh == 2)
        buffer.copyFrom (1, 0, buffer, 0, 0, n);   // Mono -> Stereo, damit Stereo-Pendeln wirkt

    const int mode = (int) apvts.getRawParameterValue ("mode")->load();
    if (mode != lastMode)
    {
        // Tremolo: Lampe glüht schnell auf; Vibe: träge Glühbirne
        const bool isVibe = mode >= 2;
        for (auto& c : cells) { c.prepare (sr, isVibe ? 9.0 : 2.5, isVibe ? 45.0 : 10.0); }
        lastMode = mode;
    }

    // Geschwindigkeit: frei oder im Songtempo (Phase folgt der Song-Position)
    const bool sync = apvts.getRawParameterValue ("sync")->load() > 0.5f;
    const double bpc = beatsPerCycle[(size_t) juce::jlimit (0, 5, (int) apvts.getRawParameterValue ("division")->load())];
    double rate = apvts.getRawParameterValue ("speed")->load();
    if (sync)
    {
        double bpm = 120.0;
        if (auto* ph = getPlayHead())
            if (auto pos = ph->getPosition())
            {
                if (auto b = pos->getBpm()) bpm = *b;
                if (pos->getIsPlaying())
                    if (auto ppq = pos->getPpqPosition())
                    {
                        const double target = std::fmod (*ppq / bpc, 1.0);
                        double diff = target - phase;
                        diff -= std::round (diff);
                        if (std::abs (diff) > 0.02) phase = target;          // Sprung (z. B. Positionswechsel)
                        else phase += diff * 0.1;                            // sonst sanft nachziehen
                    }
            }
        rate = bpm / 60.0 / bpc;
    }
    currentRateHz.store ((float) rate);
    const double inc = rate / sr;

    smDepth.setTargetValue (apvts.getRawParameterValue ("depth")->load() * 0.01f);
    smShape.setTargetValue (apvts.getRawParameterValue ("shape")->load() * 0.1f);
    smOut.setTargetValue (juce::Decibels::decibelsToGain (apvts.getRawParameterValue ("out")->load()));
    smStereo.setTargetValue (apvts.getRawParameterValue ("stereo")->load() > 0.5f ? 1.0f : 0.0f);

    float* ch[2] = { buffer.getWritePointer (0), outCh > 1 ? buffer.getWritePointer (1) : nullptr };
    float pk = 0, lamp = 0;
    for (int i = 0; i < n; ++i)
    {
        const float depth = smDepth.getNextValue(), shape = smShape.getNextValue(), og = smOut.getNextValue();
        const float st = smStereo.getNextValue();
        phase += inc;
        if (phase >= 1.0) phase -= 1.0;
        // Stereo: Tremolo pendelt gegenläufig, Vibe um eine Viertelperiode versetzt
        const double offR = st * (mode >= 2 ? 0.25 : 0.5);
        for (int c = 0; c < outCh; ++c)
        {
            if (ch[c] == nullptr) continue;
            const double ph = c == 0 ? phase : std::fmod (phase + offR, 1.0);
            const float light = cells[(size_t) c].process (vibe::lfoShape (ph, mode >= 2 ? 0.15f : shape));
            if (c == 0) lamp = light;
            const float x = ch[c][i];
            float y;
            switch (mode)
            {
                case 0:   // Tremolo, Pegel wird teilweise ausgeglichen
                    y = x * (1.0f - depth * (1.0f - light)) / (1.0f - 0.3f * depth);
                    break;
                case 1:   // Harmonisch: Tiefen und Höhen gegenläufig
                {
                    const float lo = xo2[(size_t) c].process (xoLp, xo1[(size_t) c].process (xoLp, x));
                    const float hi = x - lo;
                    y = (lo * (1.0f - depth * (1.0f - light)) + hi * (1.0f - depth * light)) / (1.0f - 0.3f * depth);
                    break;
                }
                case 2:   y = 0.5f * (x + vibes[(size_t) c].process (x, light, depth)) * 1.55f; break;   // gleicht Auslöschungen aus
                default:  y = vibes[(size_t) c].process (x, light, depth); break;
            }
            y *= og;
            ch[c][i] = y;
            pk = juce::jmax (pk, std::abs (y));
            if (c == 0) vuState += (std::abs (y) - vuState) * vuCoef;
        }
    }
    lampLevel.store (lamp);
    vuOut.store (vuState);
    peakOut.store (juce::jmax (pk, peakOut.load()));
}

void VibeAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    state.setProperty ("program", currentProgram, nullptr);
    if (auto xml = state.createXml())
        copyXmlToBinary (*xml, destData);
}

void VibeAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    auto xml = getXmlFromBinary (data, sizeInBytes);
    if (xml == nullptr || ! xml->hasTagName (apvts.state.getType()))
        return;
    auto tree = juce::ValueTree::fromXml (*xml);
    currentProgram = juce::jlimit (0, getNumPrograms() - 1, (int) tree.getProperty ("program", 0));
    apvts.replaceState (tree);
    for (auto* p : getParameters())
        if (auto* rp = dynamic_cast<juce::RangedAudioParameter*> (p))
            rp->setValueNotifyingHost (rp->convertTo0to1 (rp->convertFrom0to1 (rp->getValue())));
}

juce::AudioProcessorEditor* VibeAudioProcessor::createEditor() { return new kb::ScaledEditor (*this, new VibeAudioProcessorEditor (*this)); }

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new VibeAudioProcessor(); }
