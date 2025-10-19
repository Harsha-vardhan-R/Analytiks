#include "PluginProcessor.h"
#include "PluginEditor.h"


AnalytiksAudioProcessor::AnalytiksAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", AudioChannelSet::stereo(), true)
                     #endif
                       ),
#endif
        apvts(*this, nullptr, "PARAMETERS", create_parameter_layout())
{
    // max supported block size.
    temp_buffer.resize(8192 * 2);
    
    oscilloscope_component = std::make_unique<OscilloscopeComponent>(apvts);
    phase_correlation_component = std::make_unique<PhaseCorrelationAnalyserComponent>(apvts);
    fft_engine = std::make_unique<PFFFT>(apvts);

}

AnalytiksAudioProcessor::~AnalytiksAudioProcessor()
{
}

//==============================================================================
const String AnalytiksAudioProcessor::getName() const
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

const String AnalytiksAudioProcessor::getProgramName (int index)
{
    return {};
}

void AnalytiksAudioProcessor::changeProgramName (int index, const String& newName)
{
}

//==============================================================================
void AnalytiksAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    /*spectral_analyser_component->prepareToPlay(sampleRate, samplesPerBlock);
    spectrogram_component->prepareToPlay(sampleRate, samplesPerBlock);
    oscilloscope_component->prepareToPlay(sampleRate, samplesPerBlock);*/
    phase_correlation_component->prepareToPlay(sampleRate, samplesPerBlock);
}

void AnalytiksAudioProcessor::releaseResources()
{
    phase_correlation_component->zeroOutMeters();
}

AudioProcessorValueTreeState::ParameterLayout AnalytiksAudioProcessor::create_parameter_layout()
{
    auto layout = AudioProcessorValueTreeState::ParameterLayout();

    // NOTE : AudioProcessorParameter::otherMeter makes them non automatable.
    AudioParameterFloatAttributes float_param_attributes = 
        AudioParameterFloatAttributes().withAutomatable(false);
    AudioParameterIntAttributes int_param_attributes = 
        AudioParameterIntAttributes().withAutomatable(false);
    AudioParameterBoolAttributes bool_param_attributes = 
        AudioParameterBoolAttributes().withAutomatable(false);
    AudioParameterChoiceAttributes choice_param_attributes =
        AudioParameterChoiceAttributes().withAutomatable(false);

    ////////////////////////////////////////////////////
    ////////////////////////////////////////////////////
    layout.add(std::make_unique<AudioParameterFloat>( 
        "ui_sep_x",   
        "UI Seperator X", 
        NormalisableRange<float>(0.0, 1.0), 
        0.75, 
        float_param_attributes));
    layout.add(std::make_unique<AudioParameterFloat>( 
        "ui_sep_y",   
        "UI Seperator Y", 
        NormalisableRange<float>(0.0, 1.0), 
        0.6,  
        float_param_attributes));
    layout.add(std::make_unique<AudioParameterFloat>( 
        "ui_width",   
        "Plugin Width",   
        NormalisableRange<float>(MIN_WIDTH,  MAX_WIDTH),  
        DEFAULT_WIDTH, 
        float_param_attributes));
    layout.add(std::make_unique<AudioParameterFloat>( 
        "ui_height",  
        "Plugin Height",  
        NormalisableRange<float>(MIN_HEIGHT, MAX_HEIGHT), 
        DEFAULT_HEIGHT, 
        float_param_attributes));
    // UI Accent Hue colour.
    layout.add(std::make_unique<AudioParameterFloat>( 
        "ui_acc_hue",   
        "UI Accent Hue", 
        NormalisableRange<float>(0.0, 1.0),    
        0.9, 
        float_param_attributes));

    ////////////////////////////////////////////////////
    ////////////////////////////////////////////////////
    // gobal parameters, parameters that affect more than one of the 4 components.
    layout.add(std::make_unique<AudioParameterChoice>( 
        "gb_clrmap",  
        "Colourmap",
        StringArray(
            "Inferno",
            "Plasma",
            "Viridis",
            "Coolwarm",
            "Rainbow",
            "Hot",
            "Copper",
            "Pink",
            "Greys",
            "gnuplot2"
        ),
        colourMaps.at("Inferno"),
        choice_param_attributes));
    layout.add(std::make_unique<AudioParameterChoice>(
        "gb_chnl",    
        "Channel",          
        StringArray(
            "L+R",
            "L",
            "R",
            "S"
        ),
        channelMode.at("L+R"),
        choice_param_attributes));
    layout.add(std::make_unique<AudioParameterChoice>(
        "gb_vw_mde",
        "View Mode",
        StringArray(
            "Scroll",
            "Overview"
        ),
        viewMode.at("Scroll"),
        choice_param_attributes));
    layout.add(std::make_unique<AudioParameterChoice>(
        "gb_vw_ortn", 
        "View Orientation",
        StringArray(
            "Normal",
            "Rotated"
        ),
        viewOrientation.at("Normal"),
        choice_param_attributes));
    layout.add(std::make_unique<AudioParameterChoice>(
        "gb_fft_ord",
        "FFT Order",
        StringArray(
            "9 - 512",
            "10 - 1024",
            "11 - 2048",
            "12 - 4098",
            "13 - 8192"
        ),
        2,
        choice_param_attributes));
    // based on the channel selection, output is written.
    layout.add(std::make_unique<AudioParameterBool>(
        "gb_listen", 
        "Listen", 
        true, 
        bool_param_attributes));

    ////////////////////////////////////////////////////
    // SPECTRUM PARAMETERS.
    layout.add(std::make_unique<AudioParameterFloat>(
        "sp_rng_min",
        "Spectrum Frequncy range Min[Hz]",
        NormalisableRange<float>(10.0, 20000.0),
        10.0,
        float_param_attributes));
    layout.add(std::make_unique<AudioParameterFloat>(
        "sp_rng",
        "Spectrum Frequncy Range[Hz]",
        NormalisableRange<float>(0.1, 1.0), 
        1.0, 
        float_param_attributes));
    // add an accent colour to the spectrum analyser.
    layout.add(std::make_unique<AudioParameterBool>(
        "sp_accent",
        "Colourise Spectrum",
        false,
        bool_param_attributes));


    AudioParameterIntAttributes int_param_attributes_num_bars = int_param_attributes.withStringFromValueFunction(
        [](int value, int max_str_length) -> String {
            if (value > 255) return String("Line");
            else return String(value);
        });
    // number of bars to be shown, at exactly 256 we consider this a line graph, so max 255 bars.
    layout.add(std::make_unique<AudioParameterInt>(
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
    layout.add(std::make_unique<AudioParameterFloat>(
        "sp_bar_spd", 
        "Spectrum Bar Speed[ms]", 
        NormalisableRange<float>(1.0, 2000.0), 
        100.0, 
        float_param_attributes));
    // when less than 0.5 ms the peak hold line will not be visible.
    layout.add(std::make_unique<AudioParameterFloat>(
        "sp_pek_hld", 
        "Spectrum peak hold[ms]", 
        NormalisableRange<float>(0.0, 2000.0), 
        100.0, 
        float_param_attributes));
    // want highlighting ?
    layout.add(std::make_unique<AudioParameterBool>(
        "sp_high", 
        "Spectrum highlighting", 
        true, 
        bool_param_attributes));
    layout.add(std::make_unique<AudioParameterFloat>(
        "sp_high_tm", 
        "Spectrum highlighting time[ms]", 
        NormalisableRange<float>(0.0, 2000.0), 
        100.0, 
        float_param_attributes));
    layout.add(std::make_unique<AudioParameterFloat>(
        "sp_high_gt", 
        "Spectrum highlighting gate[dB]", 
        NormalisableRange<float>(-60.0, 0.0), 
        -35.0, 
        float_param_attributes));

    ////////////////////////////////////////////////////
    ////////////////////////////////////////////////////
    // spectrogram parameters
    layout.add(std::make_unique<AudioParameterFloat>(
        "sg_cm_bias", 
        "Spectrum colourmap bias", 
        NormalisableRange<float>(0.0, 1.0), 
        0.0, 
        float_param_attributes));
    layout.add(std::make_unique<AudioParameterFloat>(
        "sg_cm_curv", 
        "Spectrum colourmap curve", 
        NormalisableRange<float>(-1.0, 1.0), 
        0.0, 
        float_param_attributes));
    layout.add(std::make_unique<AudioParameterBool>(
        "sg_high_res", 
        "Non Blurred Spectrogram", 
        false, 
        bool_param_attributes));
    layout.add(std::make_unique<AudioParameterChoice>(
            "sp_measure",
            "History Window Bars",
            StringArray(
                "1/32",
                "1/16",
                "1/8",
                "1/4",
                "1/2",
                "1",
                "2",
                "4"
            ),
            3,
            choice_param_attributes));
        layout.add(std::make_unique<AudioParameterInt>(
            "sp_multiple",
            "History Window Multiple",
            1,
            64,
            16,
            int_param_attributes));

    ////////////////////////////////////////////////////
    ////////////////////////////////////////////////////
    // oscilloscope, phase correlation stuff.
    // higlighting happens based on the volume calculated by the 
    layout.add(std::make_unique<AudioParameterBool>(
        "os_high", 
        "Oscilloscope highlighting", 
        true, 
        bool_param_attributes));
    layout.add(std::make_unique<AudioParameterFloat>(
        "v_rms_time", 
        "RMS window length[ms]", 
        NormalisableRange<float>(0.1, 500.0), 
        45.0, 
        float_param_attributes));

    return layout;
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool AnalytiksAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    ignoreUnused (layouts);
    return true;
  #else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    // Some plugin hosts, such as certain GarageBand versions, will only
    // load plugins that support stereo bus layouts.
    if (layouts.getMainOutputChannelSet() != AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != AudioChannelSet::stereo())
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

void AnalytiksAudioProcessor::processBlock (AudioBuffer<float>& buffer, MidiBuffer& midiMessages)
{
    ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    int present_channel = apvts.getRawParameterValue("gb_chnl")->load();

    const float* left_channel_data = buffer.getReadPointer(0);
    const float* right_channel_data = buffer.getReadPointer(1);

    int number_of_samples = buffer.getNumSamples();

    // send data to the respective components, if they are not freezed.
    if (!freeze.load())
    {
        if (present_channel == 0)
        {
            for (int sample = 0; sample < number_of_samples; ++sample)
            {
                temp_buffer[sample] = (left_channel_data[sample] + right_channel_data[sample]) * 0.5;
            }
        }
        else if (present_channel == 1)
        {
            for (int sample = 0; sample < number_of_samples; ++sample)
            {
                temp_buffer[sample] = left_channel_data[sample];
            }
        }
        else if (present_channel == 2)
        {
            for (int sample = 0; sample < number_of_samples; ++sample)
            {
                temp_buffer[sample] = right_channel_data[sample];
            }
        }
        else
        {
            for (int sample = 0; sample < number_of_samples; ++sample)
            {
                temp_buffer[sample] = (left_channel_data[sample] - right_channel_data[sample]) * 0.5;
            }
        }
    
        // the callbacks from this will update both the analyser and the spectrogram.
        fft_engine->processBlock(temp_buffer.data(), number_of_samples);

        // update the correlation meter and the oscilloscope here.
        phase_correlation_component->processBlock(buffer);
    }

    if (!apvts.getRawParameterValue("gb_listen")->load())
    {
        buffer.setNotClear();
        for (auto i = 0; i < totalNumOutputChannels; ++i)
            buffer.clear(i, 0, buffer.getNumSamples());
    }
}

//==============================================================================
bool AnalytiksAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

AudioProcessorEditor* AnalytiksAudioProcessor::createEditor()
{
    return new AnalytiksAudioProcessorEditor (*this, apvts);
}

//==============================================================================
void AnalytiksAudioProcessor::getStateInformation (MemoryBlock& destData)
{
    if (auto state = apvts.copyState().createXml())
        copyXmlToBinary(*state, destData);
}

void AnalytiksAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xmlState = getXmlFromBinary(data, sizeInBytes))
        apvts.replaceState(ValueTree::fromXml(*xmlState));
}

std::array<juce::Component*, 4> AnalytiksAudioProcessor::getComponentArray()
{
    auto arr = std::array<juce::Component*, 4>();
    auto spec_n_analyser = fft_engine.get()->getSpectrogramAndAnalyser();

    arr[0] = spec_n_analyser[1];
    arr[1] = spec_n_analyser[0];
    arr[2] = oscilloscope_component.get();
    arr[3] = phase_correlation_component.get();

    return arr;
}

void AnalytiksAudioProcessor::setFreeze(bool freeze_val)
{
    freeze.store(freeze_val);
}

//==============================================================================
// This creates new instances of the plugin..
AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new AnalytiksAudioProcessor();
}
