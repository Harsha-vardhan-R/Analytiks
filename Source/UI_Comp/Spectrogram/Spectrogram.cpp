#include "Spectrogram.h"

SpectrogramComponent::SpectrogramComponent(
    AudioProcessorValueTreeState& apvts_reference)
    : apvts_ref(apvts_reference)
{
}

void SpectrogramComponent::newDataBatch(std::array<std::vector<float>, 32> &data, int valid, int numBins)
{
    numValidBins = numBins;
}

void SpectrogramComponent::paint(Graphics& g)
{
    g.fillAll(juce::Colours::black);
    g.setColour(juce::Colours::white);
    g.drawText("Spectrogram", g.getClipBounds(), juce::Justification::centred);
}

void SpectrogramComponent::resized()
{

}