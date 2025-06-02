/*
  ==============================================================================

    settingsPage.h
    Created: 2 Jun 2025 9:45:45am
    Author:  iamde

  ==============================================================================
*/

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>

class settingsPage : public juce::Component
{
public:

    settingsPage(juce::AudioProcessorValueTreeState& apvts_ref)
    {
    }

    void paint(juce::Graphics& g) override
    {
        g.fillAll(juce::Colours::black);
    }

    void resized() override
    {

    }

private:

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(settingsPage)
};