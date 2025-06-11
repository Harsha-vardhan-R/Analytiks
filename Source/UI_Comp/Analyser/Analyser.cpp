#include "Analyser.h"

SpectrumAnalyserComponent::SpectrumAnalyserComponent(AudioProcessorValueTreeState& apvts_reference)
    : apvts_ref(apvts_reference)
{
}

void SpectrumAnalyserComponent::paint(Graphics& g)
{
    g.fillAll(juce::Colours::brown);
    g.setColour(juce::Colours::white);
    g.drawText("Spectrum", g.getClipBounds(), juce::Justification::centred);
}

void SpectrumAnalyserComponent::resized()
{
    
}