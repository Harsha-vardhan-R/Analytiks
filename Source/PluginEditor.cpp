/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
AnalytiksAudioProcessorEditor::AnalytiksAudioProcessorEditor (AnalytiksAudioProcessor& p)
    : AudioProcessorEditor (&p), 
    audioProcessor (p), 
    apvts_ref(audioProcessor.apvts),
    move_drag_comp(move_callback)
{
    // Make sure that before the constructor has finished, you've set the
    // editor's size to whatever you need it to be.
    setSize (
        apvts_ref.getRawParameterValue("ui_width")->load(),
        apvts_ref.getRawParameterValue("ui_height")->load());

    setResizable(true, true);
    setResizeLimits(
        500,
        500,
        4000,
        3000
    );
    setSize(1300, 750);

    addAndMakeVisible(move_drag_comp);

}

AnalytiksAudioProcessorEditor::~AnalytiksAudioProcessorEditor()
{
}

//==============================================================================
void AnalytiksAudioProcessorEditor::paint (juce::Graphics& g)
{

    g.fillAll(juce::Colours::black);
    
    float hue = apvts_ref.getRawParameterValue("ui_acc_hue")->load();
    
    juce::Colour accentColour = juce::Colour::fromHSV(hue, 1.0, 0.34, 1.0);

    auto bounds = getLocalBounds();
    int height = bounds.getHeight();
    int width = bounds.getWidth();

    int ribbonHeight = std::clamp<int>(ribbonHeightFraction * height, 25, 35);

    auto ribbon = bounds.removeFromBottom(ribbonHeight);
    int paddingInPixels = std::clamp<int>(paddingWidthFraction * width, 2, 6);

    auto topPaddedRectangle = bounds.removeFromTop(paddingInPixels);
    auto leftPaddingRectangle = bounds.removeFromLeft(paddingInPixels);
    auto rightPaddingRectangle = bounds.removeFromRight(paddingInPixels);

    g.setColour(accentColour);

    g.fillRect(ribbon);
    g.fillRect(topPaddedRectangle);
    g.fillRect(leftPaddingRectangle);
    g.fillRect(rightPaddingRectangle);

    float v_sep_x = apvts_ref.getRawParameterValue("ui_sep_x")->load();
    float h_sep_y = apvts_ref.getRawParameterValue("ui_sep_y")->load();

    // snap feature, if the space is too small snap and close that component.
    if (v_sep_x < 0.1) v_sep_x = 0.0;
    if (v_sep_x > 0.9) v_sep_x = 1.0;
    if (h_sep_y < 0.1) h_sep_y = 0.0;
    if (h_sep_y > 0.9) h_sep_y = 1.0;

    int verticalSeperator_x =
        ((bounds.getWidth() - seperatorBarWidthInPixels) * v_sep_x) + bounds.getX();
    int horizontalSeperator_y =
        ((bounds.getHeight() - seperatorBarWidthInPixels) * h_sep_y) + bounds.getY();

    auto verticalSeperator = juce::Rectangle<int>(
        verticalSeperator_x,
        bounds.getY(),
        seperatorBarWidthInPixels,
        bounds.getHeight()
    );
    auto horizontalSeperator = juce::Rectangle<int>(
        bounds.getX(),
        horizontalSeperator_y,
        bounds.getWidth(),
        seperatorBarWidthInPixels
    );

    g.fillRect(verticalSeperator);
    g.fillRect(horizontalSeperator);
}

void AnalytiksAudioProcessorEditor::resized()
{
    auto bounds = getLocalBounds();
    int height = bounds.getHeight();
    int width = bounds.getWidth();

    int ribbonHeight = std::clamp<int>(ribbonHeightFraction * height, 25, 35);

    auto ribbon = bounds.removeFromBottom(ribbonHeight);

    int paddingInPixels = std::clamp<int>(paddingWidthFraction * width, 2, 6);

    auto topPaddedRectangle = bounds.removeFromTop(paddingInPixels);
    auto leftPaddingRectangle = bounds.removeFromLeft(paddingInPixels);
    auto rightPaddingRectangle = bounds.removeFromRight(paddingInPixels);

    // based on the parameters `ui_sep_x` and `ui_sep_y`divide bounds into 4 parts.
    // 0.0 and 1.0 here mean the min and max where the movable can be moved.
    float v_sep_x = apvts_ref.getRawParameterValue("ui_sep_x")->load();
    float h_sep_y = apvts_ref.getRawParameterValue("ui_sep_y")->load();

    int verticalSeperator_x, horizontalSeperator_y;

    // snap feature, if the space is too small snap and close that component.
    if (v_sep_x < 0.1) v_sep_x = 0.0;
    if (v_sep_x > 0.9) v_sep_x = 1.0;
    if (h_sep_y < 0.1) h_sep_y = 0.0;
    if (h_sep_y > 0.9) h_sep_y = 1.0;

    // relative to the present bounds.
    verticalSeperator_x =
        ((bounds.getWidth() - seperatorBarWidthInPixels) * v_sep_x);
    horizontalSeperator_y =
        ((bounds.getHeight() - seperatorBarWidthInPixels) * h_sep_y);

    auto top_portion = bounds.removeFromTop(horizontalSeperator_y);
    auto horizontalSeperator = bounds.removeFromTop(seperatorBarWidthInPixels);

    auto top_left_component = bounds.removeFromLeft(verticalSeperator_x);
    auto verticalSepTopBounds = bounds.removeFromLeft(seperatorBarWidthInPixels);
    auto top_right_component = bounds;

    auto horizontalSepLeftPart = horizontalSeperator.removeFromLeft(verticalSeperator_x);
    auto moveButtonBounds = horizontalSeperator.removeFromLeft(seperatorBarWidthInPixels);
    auto horizontalSepRightPart = horizontalSeperator;

    auto bottom_left_component = bounds.removeFromLeft(verticalSeperator_x);
    auto verticalSepBottomBounds = bounds.removeFromLeft(seperatorBarWidthInPixels);
    auto bottom_right_component = bounds;

    move_drag_comp.setBounds(moveButtonBounds);
}
