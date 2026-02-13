#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <unordered_map>

constexpr auto MIN_WIDTH      = 800;
constexpr auto MIN_HEIGHT     = 800;
constexpr auto MAX_WIDTH      = 4000;
constexpr auto MAX_HEIGHT     = 3000;
constexpr auto DEFAULT_WIDTH  = 1200;
constexpr auto DEFAULT_HEIGHT = 1000;

#include "UI_Comp/DFT/DFT.h"

#include "UI_Comp/Oscilloscope/Oscilloscope.h"
#include "UI_Comp/Correlation/Correlation.h"

using namespace juce;

// Any parameter that cntrols more than one of the component(out of the 4)
// is initiated and taken care of here, a refernce will be sent to the required component.
class AnalytiksAudioProcessor  : public AudioProcessor
{
public:
    //==============================================================================
    AnalytiksAudioProcessor();
    ~AnalytiksAudioProcessor() override;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    // supported modes.
    // LR -> Left and right.
    // MS -> Mid and side.
    const std::unordered_map<String, int> channelMode {
        { "L+R", 0 }, // same as mid.
        { "L",   1 },
        { "R",   2 },
        { "S",   3 }
    };

    // Named from matplotlib colour maps for easy reference.
    const std::unordered_map<String, int> colourMaps {
        { "Inferno",         0 },
        { "Plasma",          1 },
        { "Viridis",         2 },
        { "Coolwarm",        3 },
        { "Rainbow",         4 },
        { "Hot",             5 },
        { "Copper",          6 },
        { "Pink",            7 },
        { "Greys",           8 },
        { "gnuplot2",        9 }
    };
    
    const std::unordered_map<String, int> viewMode {
        { "Scroll",     0 },
        { "Overview",   1 },
    };

    const std::unordered_map<String, int> viewOrientation {
        { "Normal",     0 },
        { "Rotated",    1 },
    };

    AudioProcessorValueTreeState apvts;
    AudioProcessorValueTreeState::ParameterLayout create_parameter_layout();

   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
   #endif

    void processBlock (AudioBuffer<float>&, MidiBuffer&) override;

    //==============================================================================
    AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const String getProgramName (int index) override;
    void changeProgramName (int index, const String& newName) override;

    //==============================================================================
    void getStateInformation (MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    //==============================================================================
    std::array<juce::Component*, 4> getComponentArray();
    void setFreeze(bool);

    // resets the components indexes to the start so at every play from the daw, you
    // start seeing the data from the start of the buffer, instead of somewhere in the middle.
    void play();
    // presently does nothing on pause.
    void pause();

private:

    std::vector<float> temp_buffer;

    bool isLastPlaying = false;

    std::unique_ptr<PFFFT> fft_engine;
    float SR = 44100.0f;
    
    std::unique_ptr<OscilloscopeComponent> oscilloscope_component;
    std::unique_ptr<PhaseCorrelationAnalyserComponent> phase_correlation_component;

    std::function<void(float*, int)> new_fft_frame_callback;

    std::atomic<bool> freeze = false;

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AnalytiksAudioProcessor)
};
