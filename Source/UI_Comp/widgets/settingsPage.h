
#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>

class settingsPage : public juce::Component
{
public:

    settingsPage(juce::AudioProcessorValueTreeState& apvts_r)
        : apvts_ref(apvts_r)
    {
        addAndMakeVisible(UI_accent_colour_slider);
        addAndMakeVisible(UI_accent_colour_slider_label);

        
        UI_accent_colour_slider.setSliderStyle(juce::Slider::SliderStyle::LinearBar);
        UI_accent_colour_slider.setLookAndFeel(&styles);
        UI_accent_colour_slider_label.setText("UI Accent Colour", juce::dontSendNotification);
        UI_accent_colour_slider_label.setFont(16.0f); // Set font size here if you want
        UI_accent_colour_slider_attachment =
            std::make_unique < juce::SliderParameterAttachment >(
                *apvts_ref.getParameter("ui_acc_hue"),
                UI_accent_colour_slider);
    }

    void paint(juce::Graphics& g) override
    {
        g.fillAll(juce::Colours::grey);
    }

    void resized() override
    {
        auto bounds = getLocalBounds();
        auto slider_height = 0.1 * bounds.getWidth();
        auto label_height = 0.5 * slider_height;
        auto padding = 0.1 * slider_height;
        bounds.reduce(padding, padding);

        UI_accent_colour_slider_label.setBounds(bounds.removeFromTop(label_height));
        UI_accent_colour_slider.setBounds(bounds.removeFromTop(slider_height));
    }

private:
    juce::AudioProcessorValueTreeState& apvts_ref;

    juce::LookAndFeel_V2 styles;

    juce::Label UI_accent_colour_slider_label;

    juce::Slider UI_accent_colour_slider;

    std::unique_ptr<juce::SliderParameterAttachment> UI_accent_colour_slider_attachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(settingsPage)
};