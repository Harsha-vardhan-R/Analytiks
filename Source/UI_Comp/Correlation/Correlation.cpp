#include "Correlation.h"

PhaseCorrelationAnalyserComponent::PhaseCorrelationAnalyserComponent(AudioProcessorValueTreeState& apvts_reference)
    : apvts_ref(apvts_reference)
{
}

void PhaseCorrelationAnalyserComponent::paint(Graphics& g)
{
    g.fillAll(juce::Colours::orange);
    g.setColour(juce::Colours::white);
    g.drawText("Phase Correlation", g.getClipBounds(), juce::Justification::centred);
}

void PhaseCorrelationAnalyserComponent::resized()
{

}