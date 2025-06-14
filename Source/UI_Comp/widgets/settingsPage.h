
#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>

using namespace juce;

class settingsPage : public Component
{
public:

    settingsPage(AudioProcessorValueTreeState& apvts_r)
        : apvts_ref(apvts_r)
    {
        addAndMakeVisible(UI_accent_colour_slider);
        addAndMakeVisible(UI_accent_colour_slider_label);
        
        UI_accent_colour_slider.setSliderStyle(Slider::SliderStyle::LinearBar);
        UI_accent_colour_slider.setLookAndFeel(&styles);
        UI_accent_colour_slider.setColour(Slider::ColourIds::textBoxTextColourId, Colours::black);
        UI_accent_colour_slider.setColour(Slider::ColourIds::trackColourId, Colours::orange);
        UI_accent_colour_slider_label.setText("UI Accent Colour", dontSendNotification);
        UI_accent_colour_slider_label.setFont(16.0f); // Set font size here if you want
        UI_accent_colour_slider_label.setColour(Label::ColourIds::textColourId, Colours::darkgrey);
        UI_accent_colour_slider_attachment =
            std::make_unique < SliderParameterAttachment >(
                *apvts_ref.getParameter("ui_acc_hue"),
                UI_accent_colour_slider);
    }

    void paint(Graphics& g) override
    {
        g.fillAll(Colours::white);
    }

    void resized() override
    {
        auto bounds = getLocalBounds();
        auto slider_height = 0.05 * bounds.getWidth();
        auto label_height = slider_height;
        auto padding = 0.1 * slider_height;
        bounds.reduce(padding, padding);

        UI_accent_colour_slider_label.setBounds(bounds.removeFromTop(label_height));
        UI_accent_colour_slider.setBounds(bounds.removeFromTop(slider_height));
    }

private:
    AudioProcessorValueTreeState& apvts_ref;

    LookAndFeel_V4 styles;

    Label UI_accent_colour_slider_label;

    Slider UI_accent_colour_slider;

    std::unique_ptr<SliderParameterAttachment> UI_accent_colour_slider_attachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(settingsPage)
};