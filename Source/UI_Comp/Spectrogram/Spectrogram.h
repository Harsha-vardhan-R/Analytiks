// The Spectrogram.

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>

#include "../../ds/dataStructure.h"


using namespace juce;

class SpectrogramComponent : public Component
{
public:

    SpectrogramComponent(AudioProcessorValueTreeState& apvts_reference, linkDS& lnk_reference);

    void paint(Graphics& g) override;
    void resized() override;

private:
    linkDS& linker_ref;
    AudioProcessorValueTreeState& apvts_ref;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SpectrogramComponent)
};