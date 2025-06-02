/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"
#include "MainPage.h"

#include "UI_Comp/widgets/settingsPage.h"

//==============================================================================
/**
*/
class AnalytiksAudioProcessorEditor  : public juce::AudioProcessorEditor
{
public:
    AnalytiksAudioProcessorEditor (
        AnalytiksAudioProcessor&,
        juce::AudioProcessorValueTreeState& apvts_ref);
    ~AnalytiksAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    AnalytiksAudioProcessor& audioProcessor;

    int settings_width_min = 200;

    std::function<void(bool)> freeze_button_callback = [](bool button_state) {
        
    };

    std::function<void(bool)> settings_button_callback = [this](bool button_state) {
        resized();
    };

    MainPage mainUIComponent;
    settingsPage settings_page_component;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AnalytiksAudioProcessorEditor)
};
