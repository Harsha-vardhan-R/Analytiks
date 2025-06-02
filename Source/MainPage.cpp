/*
  ==============================================================================

    MainPage.cpp
    Created: 2 Jun 2025 9:36:55pm
    Author:  iamde

  ==============================================================================
*/

#include "MainPage.h"

MainPage::MainPage(
    juce::AudioProcessorValueTreeState& apvts_r,
    std::function<void(bool)>& freeze_button_callback, 
    std::function<void(bool)>& settings_button_callback
): 
    freeze_toggle_button(freeze_icon, freeze_button_callback),
    settings_toggle_button(settings_icon, settings_button_callback),
    apvts_ref(apvts_r),
    move_drag_comp(move_callback),
    volume_labels(typeface_regular, volume_seperator_labels),
    specrtum_volume_labels(typeface_regular, spectrum_volume_seperator_labels),
    spectrum_frequency_labels(typeface_regular)
{

    addAndMakeVisible(freeze_toggle_button);
    addAndMakeVisible(settings_toggle_button);

    addAndMakeVisible(move_drag_comp);

    addAndMakeVisible(volume_labels);
    addAndMakeVisible(specrtum_volume_labels);
    addAndMakeVisible(spectrum_frequency_labels);

    addAndMakeVisible(plugin_name_label);
    addAndMakeVisible(plugin_build_name_label);
    
    plugin_name_label.setText("Analytiks", juce::dontSendNotification);
    plugin_name_label.setColour(juce::Label::ColourIds::textColourId, juce::Colours::white);
    plugin_build_name_label.setText(JucePlugin_VersionString, juce::dontSendNotification);
    plugin_build_name_label.setColour(juce::Label::ColourIds::textColourId, juce::Colours::white);
    plugin_build_name_label.setJustificationType(juce::Justification::bottomLeft);

}

void MainPage::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colours::black);
    
    float hue = apvts_ref.getRawParameterValue("ui_acc_hue")->load();

    juce::Colour accentColour = juce::Colour::fromHSV(hue, 0.25, 0.1, 1.0);

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
    if (v_sep_x < 0.2) v_sep_x = 0.0;
    else if (v_sep_x > 0.8) v_sep_x = 1.0;

    if (h_sep_y < 0.2) h_sep_y = 0.0;
    else if (h_sep_y > 0.8) h_sep_y = 1.0;

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

    // draw a small triangle at the bottom,
    // to let user know it can be resized.
    auto bottom_right_point = ribbon.getBottomRight();
    juce::Path resize_triangle;
    resize_triangle.startNewSubPath(bottom_right_point.toFloat());
    resize_triangle.lineTo(bottom_right_point.x, bottom_right_point.y - 10);
    resize_triangle.lineTo(bottom_right_point.x-10, bottom_right_point.y);
    resize_triangle.closeSubPath();
    g.setColour(juce::Colours::lightgrey);
    g.fillPath(resize_triangle);
}

void MainPage::resized()
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
    if (v_sep_x < 0.2) v_sep_x = 0.0;
    else if (v_sep_x > 0.8) v_sep_x = 1.0;

    if (h_sep_y < 0.2) h_sep_y = 0.0;
    else if (h_sep_y > 0.8) h_sep_y = 1.0;

    // relative to the present bounds.
    verticalSeperator_x =
        ((bounds.getWidth() - seperatorBarWidthInPixels) * v_sep_x);
    horizontalSeperator_y =
        ((bounds.getHeight() - seperatorBarWidthInPixels) * h_sep_y);

    auto top_portion = bounds.removeFromTop(horizontalSeperator_y);
    auto horizontalSeperator = bounds.removeFromTop(seperatorBarWidthInPixels);

    auto top_left_component = top_portion.removeFromLeft(verticalSeperator_x);
    auto verticalSepTopBounds = top_portion.removeFromLeft(seperatorBarWidthInPixels);
    auto top_right_component = top_portion;

    auto horizontalSepLeftPart = horizontalSeperator.removeFromLeft(verticalSeperator_x);
    auto moveButtonBounds = horizontalSeperator.removeFromLeft(seperatorBarWidthInPixels);
    auto horizontalSepRightPart = horizontalSeperator;

    auto bottom_left_component = bounds.removeFromLeft(verticalSeperator_x);
    auto verticalSepBottomBounds = bounds.removeFromLeft(seperatorBarWidthInPixels);
    auto bottom_right_component = bounds;

    move_drag_comp.setBounds(moveButtonBounds);

    volume_labels.setBounds(verticalSepBottomBounds);
    specrtum_volume_labels.setBounds(horizontalSepRightPart);
    spectrum_frequency_labels.setBounds(verticalSepTopBounds);

    cust_font_bold.setHeight(0.9 * ribbonHeight);
    plugin_name_label.setFont(cust_font_bold);
    auto wid = cust_font_bold.getStringWidthFloat(plugin_name_label.getText());
    
    cust_font_regular.setHeight(0.5* ribbonHeight);
    plugin_build_name_label.setFont(cust_font_regular);
    wid += cust_font_regular.getStringWidthFloat(plugin_build_name_label.getText());
    
    wid *= 1.2;
    auto plugin_name_build_version_bounds = juce::Rectangle<int>(
        getWidth() - wid,
        ribbon.getY(),
        wid,
        ribbonHeight
    );

    plugin_name_label.
        setBounds(plugin_name_build_version_bounds.removeFromLeft(plugin_name_build_version_bounds.getWidth()*0.7));
    plugin_build_name_label.setBounds(plugin_name_build_version_bounds);

    ribbon.removeFromLeft(10);
    float ribbon_button_padding = ribbon.getHeight() * 0.1;
    settings_toggle_button.setBounds(ribbon.removeFromLeft(ribbon.getHeight()).reduced(ribbon_button_padding));
    freeze_toggle_button.setBounds(ribbon.removeFromLeft(ribbon.getHeight()).reduced(ribbon_button_padding));
}