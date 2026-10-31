#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"
#include "NovaLook.h"

namespace nova::ui { class Content; }

//==============================================================================
class NovaAudioProcessorEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    static constexpr int designW = 1240, designH = 800;

    explicit NovaAudioProcessorEditor (NovaAudioProcessor&);
    ~NovaAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    void applyTransform();

    NovaAudioProcessor& proc;
    nova::ui::KnobLook look;
    std::unique_ptr<nova::ui::Content> content;
    double lastTime = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (NovaAudioProcessorEditor)
};
