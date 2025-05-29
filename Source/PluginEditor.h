/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

#include "UI_Comp/widgets/drag_move.h"

//==============================================================================
/**
*/
class AnalytiksAudioProcessorEditor  : public juce::AudioProcessorEditor
{
public:
    AnalytiksAudioProcessorEditor (AnalytiksAudioProcessor&);
    ~AnalytiksAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;

    std::function<void(juce::Point<int>)> move_callback =
        [this](juce::Point<int> delta) {
        float width = getWidth();
        float height = getHeight();
        float paddingInPixels = std::clamp<int>(paddingWidthFraction * width, 2, 6);
        float ribbonHeight = std::clamp<int>(ribbonHeightFraction * height, 25, 35);

        // Get current values
        float v_sep_x = apvts_ref.getRawParameterValue("ui_sep_x")->load();
        float h_sep_y = apvts_ref.getRawParameterValue("ui_sep_y")->load();

        // Calculate new positions
        float new_v_sep_x = v_sep_x + (float)delta.x / (width - 2 * paddingInPixels);
        float new_h_sep_y = h_sep_y + (float)delta.y / (height - paddingInPixels - ribbonHeight);

        apvts_ref.getRawParameterValue("ui_sep_x")->store(
            std::clamp<float>(new_v_sep_x, 0.0f, 1.0f)
        );
        apvts_ref.getRawParameterValue("ui_sep_y")->store(
            std::clamp<float>(new_h_sep_y, 0.0f, 1.0f)
        );

        resized();
        repaint();
    };

private:
    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    AnalytiksAudioProcessor& audioProcessor;

    // Fractions of width or height taken by base components.
    float ribbonHeightFraction = 0.045;
    float paddingWidthFraction = 0.005;
    
    float seperatorBarWidthInPixels = 30;

    juce::AudioProcessorValueTreeState& apvts_ref;

    MoveDragComponent move_drag_comp;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AnalytiksAudioProcessorEditor)
};
