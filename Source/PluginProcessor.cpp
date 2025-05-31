/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
AnalytiksAudioProcessor::AnalytiksAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       ),
#endif
        apvts(*this, nullptr, "PARAMETERS", create_parameter_layout())
{
}

AnalytiksAudioProcessor::~AnalytiksAudioProcessor()
{
}

//==============================================================================
const juce::String AnalytiksAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool AnalytiksAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool AnalytiksAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool AnalytiksAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double AnalytiksAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int AnalytiksAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int AnalytiksAudioProcessor::getCurrentProgram()
{
    return 0;
}

void AnalytiksAudioProcessor::setCurrentProgram (int index)
{
}

const juce::String AnalytiksAudioProcessor::getProgramName (int index)
{
    return {};
}

void AnalytiksAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

//==============================================================================
void AnalytiksAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    // Use this method as the place to do any pre-playback
    // initialisation that you need..
}

void AnalytiksAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

juce::AudioProcessorValueTreeState::ParameterLayout AnalytiksAudioProcessor::create_parameter_layout()
{
    auto layout = juce::AudioProcessorValueTreeState::ParameterLayout();

    layout.add(std::make_unique<juce::AudioParameterFloat>( "ui_sep_x",   "UI Seperator X",  0.0, 1.0,    0.75 ));
    layout.add(std::make_unique<juce::AudioParameterFloat>( "ui_sep_y",   "UI Seperator Y",  0.0, 1.0,    0.6 ));

    layout.add(std::make_unique<juce::AudioParameterFloat>( "ui_width",   "Plugin Width",    150, 4000.0, 1920.0 ));
    layout.add(std::make_unique<juce::AudioParameterFloat>( "ui_height",  "Plugin Height",   150, 3000.0, 1130.0));
    
    // UI Accent Hue colour.
    layout.add(std::make_unique<juce::AudioParameterFloat>( "ui_acc_hue", "UI Accent Hue",   0.0, 1.0,    0.75));

    return layout;
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool AnalytiksAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    // Some plugin hosts, such as certain GarageBand versions, will only
    // load plugins that support stereo bus layouts.
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // This checks if the input layout matches the output layout
   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}
#endif

void AnalytiksAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    // In case we have more outputs than inputs, this code clears any output
    // channels that didn't contain input data, (because these aren't
    // guaranteed to be empty - they may contain garbage).
    // This is here to avoid people getting screaming feedback
    // when they first compile a plugin, but obviously you don't need to keep
    // this code if your algorithm always overwrites all the output channels.
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    // This is the place where you'd normally do the guts of your plugin's
    // audio processing...
    // Make sure to reset the state if your inner loop is processing
    // the samples and the outer loop is handling the channels.
    // Alternatively, you can process the samples with the channels
    // interleaved by keeping the same state.
    for (int channel = 0; channel < totalNumInputChannels; ++channel)
    {
        auto* channelData = buffer.getWritePointer (channel);

        // ..do something to the data...
    }
}

//==============================================================================
bool AnalytiksAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* AnalytiksAudioProcessor::createEditor()
{
    return new AnalytiksAudioProcessorEditor (*this);
}

//==============================================================================
void AnalytiksAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto state = apvts.copyState().createXml())
        copyXmlToBinary(*state, destData);
}

void AnalytiksAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xmlState = getXmlFromBinary(data, sizeInBytes))
        apvts.replaceState(juce::ValueTree::fromXml(*xmlState));
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new AnalytiksAudioProcessor();
}
