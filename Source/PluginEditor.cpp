/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
AnalytiksAudioProcessorEditor::AnalytiksAudioProcessorEditor(
    AnalytiksAudioProcessor& p,
    juce::AudioProcessorValueTreeState& apvts_ref
)
    : 
    AudioProcessorEditor(p),
    audioProcessor(p),
    mainUIComponent(apvts_ref, freeze_button_callback, settings_button_callback),
    settings_page_component(apvts_ref)
{
    setOpaque(true);
    
    setResizable(true, false);
    setResizeLimits(
        600,
        600,
        4000,
        3000
    );

    setSize (
        audioProcessor.apvts.getRawParameterValue("ui_width")->load(),
        audioProcessor.apvts.getRawParameterValue("ui_height")->load());

    addAndMakeVisible(mainUIComponent);
    addAndMakeVisible(settings_page_component);
}

AnalytiksAudioProcessorEditor::~AnalytiksAudioProcessorEditor()
{
}

//==============================================================================
void AnalytiksAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll(juce::Colours::white);
}

void AnalytiksAudioProcessorEditor::resized()
{

    auto bounds = getLocalBounds();

    auto setting_page_width= std::max<int>(settings_width_min, bounds.getWidth() * 0.3);

    if (mainUIComponent.settings_toggle_button.state)
    {
        settings_page_component.setBounds(bounds.removeFromLeft(setting_page_width));
    }
    else 
    {
        settings_page_component.setBounds(bounds.removeFromLeft(0));
    }

    mainUIComponent.setBounds(bounds);
}
