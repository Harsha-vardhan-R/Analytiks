/*
  ==============================================================================

    MainPage.h
    Created: 2 Jun 2025 9:36:55pm
    Author:  iamde

  ==============================================================================
*/

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>

#include "UI_Comp/widgets/drag_move.h"
#include "UI_Comp/widgets/seperator_bar_labeler.h"
#include "UI_Comp/widgets/seperator_bar_dynamic_labeler.h"
#include "UI_Comp/widgets/icon_button.h"

class MainPage : public juce::Component
{
public:

    MainPage(
        juce::AudioProcessorValueTreeState& apvts_r,
        std::function<void(bool)>& freeze_button_callback,
        std::function<void(bool)>& settings_button_callback
    );

    void paint(juce::Graphics& g) override;
    void resized() override;

    // visible from the plugin editor.
    iconButton freeze_toggle_button;
    iconButton settings_toggle_button;

private:
    juce::AudioProcessorValueTreeState& apvts_ref;

    //==============================================================================
    //==============================================================================
    juce::Typeface::Ptr typeface_regular{
        juce::Typeface::createSystemTypefaceFor(
            BinaryData::LatoRegular_ttf,
            BinaryData::LatoRegular_ttfSize)
    };
    juce::Font cust_font_regular{ typeface_regular };

    juce::Typeface::Ptr typeface_bold{
        juce::Typeface::createSystemTypefaceFor(
            BinaryData::SpaceMonoBold_ttf,
            BinaryData::SpaceMonoBold_ttfSize)
    };
    juce::Font cust_font_bold{ typeface_bold };

    std::unique_ptr<juce::Drawable> settings_icon = juce::Drawable::createFromImageData(
        BinaryData::settings_icon_svg, BinaryData::settings_icon_svgSize);
    std::unique_ptr<juce::Drawable> freeze_icon = juce::Drawable::createFromImageData(
        BinaryData::freeze_icon_svg, BinaryData::freeze_icon_svgSize);
    //==============================================================================
    //==============================================================================

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

    // Fractions of width or height taken by base components.
    float ribbonHeightFraction = 0.045;
    float paddingWidthFraction = 0.005;
    
    float seperatorBarWidthInPixels = 25;

    MoveDragComponent move_drag_comp;

    const std::vector<std::vector<juce::String>> volume_seperator_labels{
        {
            { "0", "", "-20", "", "-40", "", "-60", " - ", "-60", "", "-40", "", "-20", "", "0"},
            { "0", "-10", "-20", "-30", "-40", "-50", "-60", " - ", "-60", "-50", "-40", "-30", "-20", "-10", "0"},
        }
    };

    const std::vector<std::vector<juce::String>> spectrum_volume_seperator_labels{
        {
            { "", "-60", "", "", "", "-20", "", ""},
            { "", "-60", "", "-40", "", "-20", "", "0"},
            { "", "-60", "-50", "-40", "-30", "-20", "-10", "0"},
        }
    };

    SeperatorBarLabeler volume_labels;
    SeperatorBarLabeler specrtum_volume_labels;
    LogSeperatorBarLabeler spectrum_frequency_labels;

    juce::Label plugin_name_label;
    juce::Label plugin_build_name_label;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainPage)
};