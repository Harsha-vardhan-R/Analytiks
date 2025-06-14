#include "Oscilloscope.h"

OscilloscopeComponent::OscilloscopeComponent(AudioProcessorValueTreeState& apvts_reference)
    : apvts_ref(apvts_reference)
{
}

void OscilloscopeComponent::paint(Graphics& g)
{
    g.fillAll(juce::Colours::black);
    g.setColour(juce::Colours::white);
    g.drawText("Oscilloscope", g.getClipBounds(), juce::Justification::centred);
}

void OscilloscopeComponent::resized()
{

}