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

    // NOTE : juce::AudioProcessorParameter::otherMeter makes them non automatable.
    juce::AudioParameterFloatAttributes float_param_attributes = 
        juce::AudioParameterFloatAttributes().withAutomatable(false);
    juce::AudioParameterIntAttributes int_param_attributes = 
        juce::AudioParameterIntAttributes().withAutomatable(false);
    juce::AudioParameterBoolAttributes bool_param_attributes = 
        juce::AudioParameterBoolAttributes().withAutomatable(false);

    ////////////////////////////////////////////////////
    ////////////////////////////////////////////////////
    layout.add(std::make_unique<juce::AudioParameterFloat>( 
        "ui_sep_x",   
        "UI Seperator X", 
        juce::NormalisableRange<float>(0.0, 1.0), 
        0.75, 
        float_param_attributes));
    layout.add(std::make_unique<juce::AudioParameterFloat>( 
        "ui_sep_y",   
        "UI Seperator Y", 
        juce::NormalisableRange<float>(0.0, 1.0), 
        0.6,  
        float_param_attributes));
    layout.add(std::make_unique<juce::AudioParameterFloat>( 
        "ui_width",   
        "Plugin Width",   
        juce::NormalisableRange<float>(MIN_WIDTH,  MAX_WIDTH),  
        DEFAULT_WIDTH, 
        float_param_attributes));
    layout.add(std::make_unique<juce::AudioParameterFloat>( 
        "ui_height",  
        "Plugin Height",  
        juce::NormalisableRange<float>(MIN_HEIGHT, MAX_HEIGHT), 
        DEFAULT_HEIGHT, 
        float_param_attributes));
    // UI Accent Hue colour.
    layout.add(std::make_unique<juce::AudioParameterFloat>( 
        "ui_acc_hue",   
        "UI Accent Hue", 
        juce::NormalisableRange<float>(0.0, 1.0),    
        0.9, 
        float_param_attributes));


    ////////////////////////////////////////////////////
    ////////////////////////////////////////////////////
    // gobal parameters, parameters that affect more than one of the 4 components.
    layout.add(std::make_unique<juce::AudioParameterInt>( 
        "gb_clrmap",  
        "Colourmap",        
        0, 
        colourMaps.size() - 1,      
        colourMaps.at("Analytics"), 
        int_param_attributes));
    layout.add(std::make_unique<juce::AudioParameterInt>( 
        "gb_chnl",    
        "Channel",          
        0, 
        channelMode.size() - 1,     
        channelMode.at("L+R"), 
        int_param_attributes));
    layout.add(std::make_unique<juce::AudioParameterInt>( 
        "gb_vw_mde",  
        "View Mode",        
        0, 
        viewMode.size() - 1,        
        viewMode.at("Scroll"), 
        int_param_attributes));
    layout.add(std::make_unique<juce::AudioParameterInt>( 
        "gb_vw_ortn", 
        "View Orientation", 
        0, 
        viewOrientation.size() - 1, 
        viewOrientation.at("Normal"), 
        int_param_attributes));
    // based on the channel selection, output is written.
    layout.add(std::make_unique<juce::AudioParameterBool>(
        "gb_listen", 
        "Listen", 
        false, 
        bool_param_attributes));

    ////////////////////////////////////////////////////
    ////////////////////////////////////////////////////
    // SPECTRUM PARAMETERS.
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "sp_rng_min", 
        "Spectrum Frequncy range Min[Hz]", 
        juce::NormalisableRange<float>(10.0, 20000.0), 
        10.0, 
        float_param_attributes));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "sp_rng_max", 
        "Spectrum Frequncy range Max[Hz]", 
        juce::NormalisableRange<float>(10.0, 20000.0), 
        20000.0, 
        float_param_attributes));

    juce::AudioParameterIntAttributes int_param_attributes_num_bars = int_param_attributes.withStringFromValueFunction(
        [](int value, int max_str_length) -> juce::String {
            if (value > 255) return juce::String("Line");
            else return juce::String(value);
        });
    // number of bars to be shown, at exactly 256 we consider this a line graph, so max 255 bars.
    layout.add(std::make_unique<juce::AudioParameterInt>(
        "sp_num_brs", 
        "Number of Bars", 
        3, 
        256, 
        128, 
        int_param_attributes_num_bars
    ));

    // the bars(or points on the line for a line graph) are slowed
    // with a one pole filter so that they do not jitter, making them hard to see.
    // use that 1-e^(-1/SR*time) one pole thingy.
    // both of the below params in ms.
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "sp_bar_spd", 
        "Spectrum Bar Speed[ms]", 
        juce::NormalisableRange<float>(1.0, 2000.0), 
        100.0, 
        float_param_attributes));
    // when less than 0.5 ms the peak hold line will not be visible.
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "sp_pek_hld", 
        "Spectrum peak hold[ms]", 
        juce::NormalisableRange<float>(0.0, 2000.0), 
        100.0, 
        float_param_attributes));
    // want highlighting ?
    layout.add(std::make_unique<juce::AudioParameterBool>(
        "sp_high", 
        "Spectrum highlighting", 
        true, 
        bool_param_attributes));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "sp_high_tm", 
        "Spectrum highlighting time[ms]", 
        juce::NormalisableRange<float>(0.0, 2000.0), 
        100.0, 
        float_param_attributes));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "sp_high_gt", 
        "Spectrum highlighting gate[dB]", 
        juce::NormalisableRange<float>(-60.0, 0.0), 
        -35.0, 
        float_param_attributes));

    ////////////////////////////////////////////////////
    ////////////////////////////////////////////////////
    // spectrogram parameters
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "sg_cm_bias", 
        "Spectrum colourmap bias", 
        juce::NormalisableRange<float>(-1.0, 1.0), 
        0.0, 
        float_param_attributes));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "sg_cm_curv", 
        "Spectrum colourmap curve", 
        juce::NormalisableRange<float>(-1.0, 1.0), 
        0.0, 
        float_param_attributes));
    layout.add(std::make_unique<juce::AudioParameterBool>(
        "sg_high_res", 
        "Non Blurred Spectrogram", 
        false, 
        bool_param_attributes));

    ////////////////////////////////////////////////////
    ////////////////////////////////////////////////////
    // oscilloscope, phase correlation stuff.
    // higlighting happens based on the volume calculated by the 
    layout.add(std::make_unique<juce::AudioParameterBool>(
        "os_high", 
        "Oscilloscope highlighting", 
        true, 
        bool_param_attributes));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "v_rms_time", 
        "RMS window length[ms]", 
        juce::NormalisableRange<float>(0.1, 500.0), 
        45.0, 
        float_param_attributes));

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
    return new AnalytiksAudioProcessorEditor (*this, apvts);
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
