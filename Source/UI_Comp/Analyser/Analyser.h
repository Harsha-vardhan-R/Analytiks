// The spectrum analyser.

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>

using namespace juce;

class SpectrumAnalyserComponent : public Component
{
public:
    
    SpectrumAnalyserComponent(AudioProcessorValueTreeState& apvts_reference);

    void paint(Graphics& g) override;
    void resized() override;

private:
    AudioProcessorValueTreeState& apvts_ref;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SpectrumAnalyserComponent)
};