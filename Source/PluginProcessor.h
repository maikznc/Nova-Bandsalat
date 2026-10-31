#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <map>
#include "TapeEngine.h"

//==============================================================================
class NovaAudioProcessor : public juce::AudioProcessor
{
public:
    using ValueMap = std::map<juce::String, float>;

    NovaAudioProcessor();
    ~NovaAudioProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "NOVA Bandsalat"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 2.6; }

    int getNumPrograms() override;
    int getCurrentProgram() override { return currentProgram; }
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    //==============================================================================
    juce::AudioProcessorValueTreeState apvts;
    nova::TapeEngine engine;

    std::array<std::atomic<float>, 8> wear;
    void resetWear (int tapeIndex) { if (tapeIndex >= 0 && tapeIndex < 8) wear[(size_t) tapeIndex].store (0.0f); }

    /** Every sound-relevant parameter (not MONITOR, RANDOM or BRAKE). Used by presets and A/B. */
    static const juce::StringArray& presetParameterIds();
    ValueMap captureValues() const;
    void applyValues (const ValueMap& values);
    ValueMap factoryValues (int index) const;

    // user presets on disk
    juce::File getUserPresetFolder() const;
    juce::Array<juce::File> getUserPresets() const;
    bool saveUserPreset (const juce::String& name);
    bool loadUserPreset (const juce::File& file);

    juce::String currentPresetName;
    ValueMap lastLoadedValues;

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    int currentProgram = 0;

    std::atomic<float>* p[32] {};
    juce::RangedAudioParameter* paramTape = nullptr;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (NovaAudioProcessor)
};
