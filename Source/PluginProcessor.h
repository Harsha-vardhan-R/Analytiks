
#pragma once

#include <JuceHeader.h>

constexpr auto MIN_WIDTH = 600;
constexpr auto  MIN_HEIGHT = 600;
constexpr auto  MAX_WIDTH = 4000;
constexpr auto  MAX_HEIGHT = 3000;
constexpr auto  DEFAULT_WIDTH = 1200;
constexpr auto  DEFAULT_HEIGHT = 800;

class AnalytiksAudioProcessor  : public juce::AudioProcessor
{
public:
    //==============================================================================
    AnalytiksAudioProcessor();
    ~AnalytiksAudioProcessor() override;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

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
