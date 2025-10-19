#include "Spectrogram.h"

SpectrogramComponent::SpectrogramComponent(
    AudioProcessorValueTreeState& apvts_reference
    , linkDS& lnk_reference)
    : linker_ref(lnk_reference),
    apvts_ref(apvts_reference)
{
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