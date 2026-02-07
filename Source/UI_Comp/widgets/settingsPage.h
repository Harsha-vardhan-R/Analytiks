#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include "ModernLookAndFeel.h"

using namespace juce;

class settingsPage : public Component
{
public:

    settingsPage(AudioProcessorValueTreeState& apvts_r)
        : apvts_ref(apvts_r)
    {
        // Add all components
        addAndMakeVisible(global_settings_label);
        addAndMakeVisible(analyser_settings_label);
        addAndMakeVisible(spectrogram_settings_label);
        addAndMakeVisible(correlation_settings_label);
        addAndMakeVisible(ocsilloscope_settings_label);
        addAndMakeVisible(accent_colour_slider_label);
        addAndMakeVisible(num_bars_slider_label);
        addAndMakeVisible(bar_speed_slider_label);
        addAndMakeVisible(spec_higlight_gate_slider_label);
        addAndMakeVisible(colourmap_bias_slider_label);
        addAndMakeVisible(colourmap_curve_slider_label);
        addAndMakeVisible(volume_rms_time_label);
        addAndMakeVisible(listen_button_label);
        addAndMakeVisible(colourmap_combobox_label);
        addAndMakeVisible(channel_combobox_label);
        addAndMakeVisible(scrollmode_combobox_label);
        addAndMakeVisible(fftorder_combobox_label);
        addAndMakeVisible(spec_history_multiply_slider_label);
        addAndMakeVisible(measure_combobox_label);
        addAndMakeVisible(freq_rng_min_label);
        addAndMakeVisible(freq_rng_max_label);

        
        addAndMakeVisible(accent_colour_slider);
        addAndMakeVisible(num_bars_slider);
        addAndMakeVisible(bar_speed_slider);
        addAndMakeVisible(spec_higlight_gate_slider);
        addAndMakeVisible(colourmap_bias_slider);
        addAndMakeVisible(colourmap_curve_slider);
        addAndMakeVisible(volume_rms_time_slider);
        addAndMakeVisible(freq_rng_min_slider);
        addAndMakeVisible(freq_rng_max_slider);

        addAndMakeVisible(listen_button);

        addAndMakeVisible(colourmap_combobox);
        addAndMakeVisible(channel_combobox);
        addAndMakeVisible(scrollmode_combobox);
        addAndMakeVisible(fftorder_combobox);
        addAndMakeVisible(spec_history_multiply_slider);
        addAndMakeVisible(measure_combobox);

        // Populate combo boxes
        auto* param1 = dynamic_cast<juce::AudioParameterChoice*>(apvts_r.getParameter("gb_clrmap"));
        for (int i = 0; i < param1->choices.size(); ++i)
            colourmap_combobox.addItem(param1->choices[i], i + 1);

        auto* param2 = dynamic_cast<juce::AudioParameterChoice*>(apvts_r.getParameter("gb_chnl"));
        for (int i = 0; i < param2->choices.size(); ++i)
            channel_combobox.addItem(param2->choices[i], i + 1);

        auto* param3 = dynamic_cast<juce::AudioParameterChoice*>(apvts_r.getParameter("gb_vw_mde"));
        for (int i = 0; i < param3->choices.size(); ++i)
            scrollmode_combobox.addItem(param3->choices[i], i + 1);

        auto* param5 = dynamic_cast<juce::AudioParameterChoice*>(apvts_r.getParameter("gb_fft_ord"));
        for (int i = 0; i < param5->choices.size(); ++i)
            fftorder_combobox.addItem(param5->choices[i], i + 1);

        auto* param6 = dynamic_cast<juce::AudioParameterChoice*>(apvts_r.getParameter("sp_measure"));
        for (int i = 0; i < param6->choices.size(); ++i)
            measure_combobox.addItem(param6->choices[i], i + 1);

        // Set label text
        accent_colour_slider_label.setText("UI Colour", juce::dontSendNotification);
        num_bars_slider_label.setText("Number of Bars", juce::dontSendNotification);
        bar_speed_slider_label.setText("Bar Speed (ms)", juce::dontSendNotification);
        spec_higlight_gate_slider_label.setText("Gate (dB)", juce::dontSendNotification);
        colourmap_bias_slider_label.setText("Gate", juce::dontSendNotification);
        colourmap_curve_slider_label.setText("Curve", juce::dontSendNotification);
        volume_rms_time_label.setText("Volume RMS Window (ms)", juce::dontSendNotification);
        listen_button_label.setText("Listen", juce::dontSendNotification);
        colourmap_combobox_label.setText("Colourmap", juce::dontSendNotification);
        channel_combobox_label.setText("Channel", juce::dontSendNotification);
        scrollmode_combobox_label.setText("Scrolling", juce::dontSendNotification);
        fftorder_combobox_label.setText("FFT Order", juce::dontSendNotification);
        measure_combobox_label.setText("Base Measure", juce::dontSendNotification);
        spec_history_multiply_slider_label.setText("History Multiple", juce::dontSendNotification);
        freq_rng_min_label.setText("Min Frequency (Hz)", juce::dontSendNotification);
        freq_rng_max_label.setText("Max Frequency (Hz)", juce::dontSendNotification);

        global_settings_label.setText("GLOBAL", juce::dontSendNotification);
        analyser_settings_label.setText("ANALYSER", juce::dontSendNotification);
        spectrogram_settings_label.setText("SPECTROGRAM", juce::dontSendNotification);
        correlation_settings_label.setText("CORRELATION", juce::dontSendNotification);
        ocsilloscope_settings_label.setText("OSCILLOSCOPE", juce::dontSendNotification);
        
        // Apply modern look and feel
        for (auto box_ : {
                &colourmap_combobox,
                &channel_combobox,
                &scrollmode_combobox,
                &fftorder_combobox,
                &measure_combobox
            })
        {
            box_->setLookAndFeel(&modernStyle);
        }

        for (auto slider_ : {
                &accent_colour_slider,
                &num_bars_slider,
                &bar_speed_slider,
                &freq_rng_min_slider,
                &freq_rng_max_slider,
                &spec_higlight_gate_slider,
                &colourmap_bias_slider,
                &colourmap_curve_slider,
                &volume_rms_time_slider,
                &spec_history_multiply_slider
            })
        {
            slider_->setLookAndFeel(&modernStyle);
            slider_->setNumDecimalPlacesToDisplay(0);
            slider_->setSliderStyle(Slider::SliderStyle::LinearBar);
            slider_->setTextBoxStyle(juce::Slider::TextBoxRight, false, 70, 20);
        }

        listen_button.setLookAndFeel(&modernStyle);
        listen_button.setButtonText("Listen");
        listen_button.setToggleable(true);

        // Style heading labels
        for (auto label_ : {
                &global_settings_label,
                &analyser_settings_label,
                &spectrogram_settings_label,
                &correlation_settings_label,
                &ocsilloscope_settings_label
            })
        {
            label_->setColour(Label::ColourIds::textColourId, Colour(0xff00a8ff));
            label_->setColour(Label::ColourIds::backgroundColourId, Colour(0xff1a1a1a));
            label_->setJustificationType(Justification::centredLeft);
        };

        // Style regular labels
        for (auto label_ : {
                &accent_colour_slider_label,
                &num_bars_slider_label,
                &bar_speed_slider_label,
                &spec_higlight_gate_slider_label,
                &colourmap_bias_slider_label,
                &colourmap_curve_slider_label,
                &volume_rms_time_label,
                &freq_rng_min_label,
                &freq_rng_max_label,
                &listen_button_label,
                &colourmap_combobox_label,
                &channel_combobox_label,
                &scrollmode_combobox_label,
                &fftorder_combobox_label,
                &spec_history_multiply_slider_label,
                &measure_combobox_label
            })
        {
            label_->setColour(Label::ColourIds::textColourId, Colour(0xffcccccc));
            label_->setJustificationType(Justification::centredLeft);
        };

        // Create parameter attachments
        freq_rng_min_slider_attachment =
            std::make_unique<SliderParameterAttachment>(
                *apvts_ref.getParameter("sp_rng_min"),
                freq_rng_min_slider);
        freq_rng_max_slider_attachment =
            std::make_unique<SliderParameterAttachment>(
                *apvts_ref.getParameter("sp_rng_max"),
                freq_rng_max_slider);

        accent_colour_slider_attachment =
            std::make_unique<SliderParameterAttachment>(
                *apvts_ref.getParameter("ui_acc_hue"),
                accent_colour_slider);
        num_bars_slider_attachment =
            std::make_unique<SliderParameterAttachment>(
                *apvts_ref.getParameter("sp_num_brs"),
                num_bars_slider);
        bar_speed_slider_attachment =
            std::make_unique<SliderParameterAttachment>(
                *apvts_ref.getParameter("sp_bar_spd"),
                bar_speed_slider);
        spec_higlight_gate_slider_attachment =
            std::make_unique<SliderParameterAttachment>(
                *apvts_ref.getParameter("sp_high_gt"),
                spec_higlight_gate_slider);
        colourmap_bias_slider_attachment =
            std::make_unique<SliderParameterAttachment>(
                *apvts_ref.getParameter("sg_cm_bias"),
                colourmap_bias_slider);
        colourmap_curve_slider_attachment =
            std::make_unique<SliderParameterAttachment>(
                *apvts_ref.getParameter("sg_cm_curv"),
                colourmap_curve_slider);
        volume_rms_time_slider_attachment =
            std::make_unique<SliderParameterAttachment>(
                *apvts_ref.getParameter("v_rms_time"),
                volume_rms_time_slider);
        spec_history_multiply_slider_attachment =
            std::make_unique<SliderParameterAttachment>(
                *apvts_ref.getParameter("sp_multiple"),
                spec_history_multiply_slider);
        
        colourmap_combobox_attachment = 
            std::make_unique<ComboBoxParameterAttachment>
                (*apvts_ref.getParameter("gb_clrmap"), colourmap_combobox);
        channel_combobox_attachment = 
            std::make_unique<ComboBoxParameterAttachment>
                (*apvts_ref.getParameter("gb_chnl"), channel_combobox);
        scrollmode_combobox_attachment = 
            std::make_unique<ComboBoxParameterAttachment>
                (*apvts_ref.getParameter("gb_vw_mde"), scrollmode_combobox);
        fftorder_combobox_attachment = 
            std::make_unique<ComboBoxParameterAttachment>
                (*apvts_ref.getParameter("gb_fft_ord"), fftorder_combobox);
        measure_combobox_attachment =
            std::make_unique<ComboBoxParameterAttachment>
            (*apvts_ref.getParameter("sp_measure"), measure_combobox);

        listen_button_attachment =
            std::make_unique<ButtonParameterAttachment>
            (*apvts_ref.getParameter("gb_listen"), listen_button);
    }

    ~settingsPage()
    {
        // Clean up look and feel
        for (auto box_ : {
                &colourmap_combobox,
                &channel_combobox,
                &scrollmode_combobox,
                &fftorder_combobox,
                &measure_combobox
            })
        {
            box_->setLookAndFeel(nullptr);
        }

        for (auto slider_ : {
                &accent_colour_slider,
                &num_bars_slider,
                &bar_speed_slider,
                &freq_rng_min_slider,
                &freq_rng_max_slider,
                &spec_higlight_gate_slider,
                &colourmap_bias_slider,
                &colourmap_curve_slider,
                &volume_rms_time_slider,
                &spec_history_multiply_slider
            })
        {
            slider_->setLookAndFeel(nullptr);
        }

        listen_button.setLookAndFeel(nullptr);
    }

    void paint(Graphics& g) override
    {
        // Dark modern background
        g.fillAll(Colour(0xff0f0f0f));
    }

    void resized() override
    {
        auto bounds = getLocalBounds();
        auto totalHeight = bounds.getHeight();
        
        // Calculate responsive sizes based on component height
        auto itemHeight = totalHeight * 0.025f;
        auto labelHeight = totalHeight * 0.025f;
        auto headingHeight = totalHeight * 0.045f;
        auto sectionSpacing = totalHeight * 0.005f;
        auto itemSpacing = totalHeight * 0.0001f;
        auto sidePadding = bounds.getWidth() * 0.02f;
        auto verticalPadding = totalHeight * 0.02f;
        
        bounds.reduce(sidePadding, verticalPadding);

        // Helper lambda for adding label + control pairs
        auto addLabeledControl = [&](Label& label, Component& control) {
            label.setBounds(bounds.removeFromTop(labelHeight));
            bounds.removeFromTop(itemSpacing * 0.3f);
            control.setBounds(bounds.removeFromTop(itemHeight));
            bounds.removeFromTop(itemSpacing);
        };

        // Set responsive fonts
        auto regularFontSize = labelHeight * 0.7f;
        auto headingFontSize = headingHeight * 0.5f;
        
        for (auto label_ : {
                &accent_colour_slider_label,
                &num_bars_slider_label,
                &bar_speed_slider_label,
                &spec_higlight_gate_slider_label,
                &freq_rng_min_label,
                &freq_rng_max_label,
                &colourmap_bias_slider_label,
                &colourmap_curve_slider_label,
                &volume_rms_time_label,
                &listen_button_label,
                &colourmap_combobox_label,
                &channel_combobox_label,
                &scrollmode_combobox_label,
                &fftorder_combobox_label,
                &measure_combobox_label,
                &spec_history_multiply_slider_label
            })
        {
            label_->setFont(Font(regularFontSize));
        }

        for (auto label_ : {
                &global_settings_label,
                &analyser_settings_label,
                &spectrogram_settings_label,
                &correlation_settings_label,
                &ocsilloscope_settings_label })
        {
            label_->setFont(Font(headingFontSize, Font::bold));
        }
        
        // Update slider text box sizes
        auto textBoxWidth = bounds.getWidth() * 0.12f;
        auto textBoxHeight = itemHeight * 0.7f;
        
        for (auto slider_ : {
                &accent_colour_slider,
                &num_bars_slider,
                &bar_speed_slider,
                &freq_rng_min_slider,
                &freq_rng_max_slider,
                &spec_higlight_gate_slider,
                &colourmap_bias_slider,
                &colourmap_curve_slider,
                &volume_rms_time_slider,
                &spec_history_multiply_slider
            })
        {
            slider_->setTextBoxStyle(juce::Slider::TextBoxRight, false, textBoxWidth, textBoxHeight);
        }

        // Layout sections
        global_settings_label.setBounds(bounds.removeFromTop(headingHeight));
        bounds.removeFromTop(itemSpacing * 1.5f);

        addLabeledControl(accent_colour_slider_label, accent_colour_slider);
        addLabeledControl(colourmap_combobox_label, colourmap_combobox);
        addLabeledControl(channel_combobox_label, channel_combobox);
        addLabeledControl(scrollmode_combobox_label, scrollmode_combobox);
        addLabeledControl(fftorder_combobox_label, fftorder_combobox);
        
        listen_button.setBounds(bounds.removeFromTop(itemHeight));
        bounds.removeFromTop(sectionSpacing);

        analyser_settings_label.setBounds(bounds.removeFromTop(headingHeight));
        bounds.removeFromTop(itemSpacing * 1.5f);

        addLabeledControl(freq_rng_min_label, freq_rng_min_slider);
        addLabeledControl(freq_rng_max_label, freq_rng_max_slider);
        addLabeledControl(num_bars_slider_label, num_bars_slider);
        addLabeledControl(bar_speed_slider_label, bar_speed_slider);

        bounds.removeFromTop(sectionSpacing);
        spectrogram_settings_label.setBounds(bounds.removeFromTop(headingHeight));
        bounds.removeFromTop(itemSpacing * 1.5f);
        
        addLabeledControl(colourmap_curve_slider_label, colourmap_curve_slider);
        addLabeledControl(colourmap_bias_slider_label, colourmap_bias_slider);
        addLabeledControl(measure_combobox_label, measure_combobox);
        addLabeledControl(spec_history_multiply_slider_label, spec_history_multiply_slider);

        bounds.removeFromTop(sectionSpacing);
        correlation_settings_label.setBounds(bounds.removeFromTop(headingHeight));
        bounds.removeFromTop(itemSpacing * 1.5f);
        
        addLabeledControl(volume_rms_time_label, volume_rms_time_slider);
    }

private:
    AudioProcessorValueTreeState& apvts_ref;

    ModernLookAndFeel modernStyle;

    Label
        global_settings_label,
        analyser_settings_label,
        spectrogram_settings_label,
        correlation_settings_label,
        ocsilloscope_settings_label,
        accent_colour_slider_label,
        num_bars_slider_label,
        bar_speed_slider_label,
        freq_rng_min_label,
        freq_rng_max_label,
        spec_higlight_gate_slider_label,
        colourmap_bias_slider_label,
        colourmap_curve_slider_label,
        volume_rms_time_label,
        listen_button_label,
        colourmap_combobox_label,
        channel_combobox_label,
        scrollmode_combobox_label,
        fftorder_combobox_label,
        spec_history_multiply_slider_label,
        measure_combobox_label;

    Slider
        accent_colour_slider,
        num_bars_slider,
        bar_speed_slider,
        freq_rng_min_slider,
        freq_rng_max_slider,
        spec_higlight_gate_slider,
        colourmap_bias_slider,
        colourmap_curve_slider,
        volume_rms_time_slider,
        spec_history_multiply_slider;

    ToggleButton
        listen_button;

    ComboBox
        colourmap_combobox,
        measure_combobox,
        channel_combobox,
        scrollmode_combobox,
        fftorder_combobox;

    std::unique_ptr<SliderParameterAttachment>
        accent_colour_slider_attachment,
        num_bars_slider_attachment,
        bar_speed_slider_attachment,
        freq_rng_min_slider_attachment,
        freq_rng_max_slider_attachment,
        spec_higlight_gate_slider_attachment,
        colourmap_bias_slider_attachment,
        colourmap_curve_slider_attachment,
        volume_rms_time_slider_attachment,
        spec_history_multiply_slider_attachment;

    std::unique_ptr<ComboBoxParameterAttachment>
        colourmap_combobox_attachment,
        channel_combobox_attachment,
        scrollmode_combobox_attachment,
        fftorder_combobox_attachment,
        measure_combobox_attachment;

    std::unique_ptr<ButtonParameterAttachment>
        listen_button_attachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(settingsPage)
};