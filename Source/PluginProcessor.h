#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "VibeDsp.h"

class VibeAudioProcessor : public juce::AudioProcessor
{
public:
    VibeAudioProcessor();
    ~VibeAudioProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override  { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    struct Preset { const char* name; std::vector<std::pair<const char*, float>> values; };
    static const std::vector<Preset>& presets();
    static constexpr std::array<double, 6> beatsPerCycle { 2.0, 1.0, 1.5, 0.5, 1.0 / 3.0, 0.25 };
    int getNumPrograms() override { return (int) presets().size(); }
    int getCurrentProgram() override { return currentProgram; }
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState apvts;
    std::atomic<float> lampLevel { 0 };            // Helligkeit der Lampe für die Anzeige (0..1)
    std::atomic<float> vuOut { 0 }, peakOut { 0 };
    std::atomic<float> currentRateHz { 5 };        // tatsächliche Geschwindigkeit (auch bei Sync)

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();

    std::array<vibe::PhotoCell, 2> cells;
    std::array<vibe::UniVibe, 2> vibes;
    std::array<eq::BiquadState, 2> xo1, xo2;
    eq::Coeffs xoLp;
    double phase = 0.0;
    int lastMode = -1;
    juce::SmoothedValue<float> smDepth, smShape, smOut, smStereo;
    float vuState = 0, vuCoef = 0.001f;
    int currentProgram = 0;
    double sr = 48000.0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VibeAudioProcessor)
};
