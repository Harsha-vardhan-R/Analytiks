#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
AnalytiksAudioProcessorEditor::AnalytiksAudioProcessorEditor(
    AnalytiksAudioProcessor& p,
    AudioProcessorValueTreeState& apvts_ref
)
    : 
    AudioProcessorEditor(p),
    audioProcessor(p),
    mainUIComponent(apvts_ref, freeze_button_callback, settings_button_callback, p.getComponentArray()),
    settings_page_component(apvts_ref)
{
    setOpaque(true);
    
    setResizable(true, true);
    setResizeLimits(
        600,
        600,
        4000,
        3000
    );

    setSize (
        audioProcessor.apvts.getRawParameterValue("ui_width")->load(),
        audioProcessor.apvts.getRawParameterValue("ui_height")->load());

    addAndMakeVisible(mainUIComponent);
    addAndMakeVisible(settings_page_component);

    audioProcessor.apvts.getParameter("ui_acc_hue")->addListener(this);
}

AnalytiksAudioProcessorEditor::~AnalytiksAudioProcessorEditor()
{
    audioProcessor.apvts.getParameter("ui_acc_hue")->removeListener(this);
}

//==============================================================================
void AnalytiksAudioProcessorEditor::paint (Graphics& g)
{
    g.fillAll(Colours::white);
}

void AnalytiksAudioProcessorEditor::resized()
{

    auto bounds = getLocalBounds();

    auto setting_page_width= std::clamp<int>(
        bounds.getWidth() * 0.25,
        settings_width_min, 
        settings_width_max);

    if (mainUIComponent.settings_toggle_button.state)
    {
        settings_page_component.setBounds(bounds.removeFromRight(setting_page_width));
    }
    else 
    {
        settings_page_component.setBounds(bounds.removeFromRight(0));
    }

    mainUIComponent.setBounds(bounds);

    // save the width and height, for the next time this opens.
    if (getParentComponent())
    {
        float normWidth = (float)(getParentWidth() - MIN_WIDTH) / (float)(MAX_WIDTH - MIN_WIDTH);
        float normHeight = (float)(getParentHeight() - MIN_HEIGHT) / (float)(MAX_HEIGHT - MIN_HEIGHT);

        normWidth = std::clamp(normWidth, 0.0f, 1.0f);
        normHeight = std::clamp(normHeight, 0.0f, 1.0f);

        audioProcessor.apvts.getParameter("ui_width")->setValueNotifyingHost(normWidth);
        audioProcessor.apvts.getParameter("ui_height")->setValueNotifyingHost(normHeight);
    }
}

void AnalytiksAudioProcessorEditor::parameterValueChanged
    (int param_index, float new_value)
{ 
    mainUIComponent.repaint();
}

void AnalytiksAudioProcessorEditor::parameterGestureChanged
(int param_index, bool new_value)
{
}