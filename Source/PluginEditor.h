#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"
#include "VintageUI.h"

class VibeAudioProcessorEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit VibeAudioProcessorEditor (VibeAudioProcessor&);
    ~VibeAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;

    // Sichtfenster: Glühbirne mit zwei Fotowiderständen, pulsiert mit der Modulation
    struct LampWindow : public juce::Component
    {
        float light = 0.0f;
        void paint (juce::Graphics&) override;
    };

    struct Knob { juce::Slider slider; juce::String name; bool big = false; std::unique_ptr<SliderAttachment> attachment; };
    Knob& addKnob (const juce::String& paramID, const juce::String& name, bool big);
    void setupSwitch (juce::ToggleButton&, const juce::String& paramID, const juce::String& onText,
                      const juce::String& offText, juce::Colour lampColour);
    void timerCallback() override;
    void renderTextures();
    void drawEngraved (juce::Graphics&, const juce::String&, juce::Rectangle<float>, juce::Font, juce::Justification);

    VibeAudioProcessor& proc;
    VintageLookAndFeel lnf;
    std::vector<std::unique_ptr<Knob>> knobs;
    Knob* speedKnob = nullptr; Knob* depthKnob = nullptr;
    Knob* modeKnob = nullptr;  Knob* shapeKnob = nullptr; Knob* divKnob = nullptr; Knob* outKnob = nullptr;
    juce::ToggleButton syncSwitch, stereoSwitch;
    std::vector<std::unique_ptr<ButtonAttachment>> buttonAttachments;
    juce::ComboBox presetBox;
    LampWindow lamp;
    juce::Label rateLabel;

    juce::Rectangle<int> faceplate, presetArea, lampBezel, pilotArea, divider;
    std::array<juce::Rectangle<int>, 2> switchLabels;
    juce::Image tolexImage, faceplateImage;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VibeAudioProcessorEditor)
};
