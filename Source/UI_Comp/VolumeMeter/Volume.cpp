#include "Volume.h"

VolumeMeterComponent::VolumeMeterComponent(AudioProcessorValueTreeState& apvts_reference)
    : apvts_ref(apvts_reference)
{
}

void VolumeMeterComponent::paint(Graphics& g)
{
    g.fillAll(juce::Colours::brown);
    g.setColour(juce::Colours::white);
    g.drawText("Volume", g.getClipBounds(), juce::Justification::centred);
}

void VolumeMeterComponent::resized()
{

}