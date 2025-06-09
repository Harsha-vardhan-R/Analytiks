#pragma once

#include <JuceHeader.h>

constexpr auto MIN_WIDTH      = 600;
constexpr auto MIN_HEIGHT     = 600;
constexpr auto MAX_WIDTH      = 4000;
constexpr auto MAX_HEIGHT     = 3000;
constexpr auto DEFAULT_WIDTH  = 1200;
constexpr auto DEFAULT_HEIGHT = 800;

// Any parameter that cntrols more than one of the component(out of the 4)
// is initiated and taken care of here, a refernce will be sent to the required component.
class AnalytiksAudioProcessor  : public juce::AudioProcessor
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
    const std::unordered_map<juce::String, int> channelMode {
        { "L+R", 0 },
        { "L",   1 },
        { "R",   2 },
        { "S",   3 },
        { "M",   4 }
    };

    // Named from matplotlib colour maps for easy reference.
    const std::unordered_map<juce::String, int> colourMaps {
        { "Analytics",       0 },        
        { "Seismic",         1 },
        { "Cyberpunk",       2 }, // own
        { "Plasma",          3 },
        { "Viridis",         4 },
        { "Blues",           5 },
        { "Greens",          6 },
        { "Oranges",         7 },
        { "Greys",           8 },
        // NOTE : would be confusing, cause it is cyclic.(use a part of it ?)
        { "HSV",             9 }
    };
    
    const std::unordered_map<juce::String, int> viewMode {
        { "Scroll",     0 },
        { "Overview",   1 },
    };

    const std::unordered_map<juce::String, int> viewOrientation {
        { "Normal",     0 },
        { "Rotated",    1 },
    };

    juce::AudioProcessorValueTreeState apvts;
    juce::AudioProcessorValueTreeState::ParameterLayout create_parameter_layout();

   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
   #endif

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

private:

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AnalytiksAudioProcessor)
};
