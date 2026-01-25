
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
        addAndMakeVisible(fftorder_combobox_label);
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

        accent_colour_slider_label.setText("UI Colour", juce::dontSendNotification);
        num_bars_slider_label.setText("Number of Bars", juce::dontSendNotification);
        bar_speed_slider_label.setText("Bar Speed(ms)", juce::dontSendNotification);
        spec_higlight_gate_slider_label.setText("Gate(dB)", juce::dontSendNotification);
        colourmap_bias_slider_label.setText("Gate", juce::dontSendNotification);
        colourmap_curve_slider_label.setText("Curve", juce::dontSendNotification);
        volume_rms_time_label.setText("Volume RMS Window(ms)", juce::dontSendNotification);
        listen_button_label.setText("Listen", juce::dontSendNotification);
        colourmap_combobox_label.setText("Colourmap", juce::dontSendNotification);
        channel_combobox_label.setText("Channel", juce::dontSendNotification);
        scrollmode_combobox_label.setText("Scrolling", juce::dontSendNotification);
        fftorder_combobox_label.setText("FFT order", juce::dontSendNotification);
        measure_combobox_label.setText("Base Measure", juce::dontSendNotification);
        spec_history_multiply_slider_label.setText("Multiple(history : Base Measure*Multiple)", juce::dontSendNotification);
        freq_rng_min_label.setText("Min Freq (Hz)", juce::dontSendNotification);
        freq_rng_max_label.setText("Max Freq (Hz)", juce::dontSendNotification);

        global_settings_label.setText("GLOBAL", juce::dontSendNotification);
        analyser_settings_label.setText("ANALYSER", juce::dontSendNotification);
        spectrogram_settings_label.setText("SPECTROGRAM", juce::dontSendNotification);
        correlation_settings_label.setText("CORRELATION", juce::dontSendNotification);
        ocsilloscope_settings_label.setText("OSCILLOSCOPE", juce::dontSendNotification);
        

        for (auto box_ : {
                &colourmap_combobox,
                &channel_combobox,
                &scrollmode_combobox,
                &fftorder_combobox,
                &measure_combobox
            })
        {
            box_->setLookAndFeel(&styles);
        }

        for (auto button_ : {
                &listen_button,
            })
        {
            button_->setLookAndFeel(&styles);

            button_->setColour(ToggleButton::ColourIds::textColourId, Colours::lightgrey);
            button_->setColour(ToggleButton::ColourIds::tickColourId, Colours::red);
            button_->setColour(ToggleButton::ColourIds::tickDisabledColourId, Colours::grey);
        }

        listen_button.setButtonText("Listen");

        listen_button.setToggleable(true);

        for (auto label_ : {
                &global_settings_label,
                &analyser_settings_label,
                &spectrogram_settings_label,
                &correlation_settings_label,
                &ocsilloscope_settings_label,
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
            label_->setColour(Label::ColourIds::textColourId, Colours::darkgrey);
        };

        for (auto label_ : {
                &global_settings_label,
                &analyser_settings_label,
                &spectrogram_settings_label,
                &correlation_settings_label,
                &ocsilloscope_settings_label
            })
        {
            label_->setColour(Label::ColourIds::backgroundColourId, Colours::white);
            label_->setColour(Label::ColourIds::outlineColourId, Colours::darkgrey);
            label_->setJustificationType(Justification::centred);
        };

        freq_rng_min_slider_attachment =
            std::make_unique < SliderParameterAttachment >(
                *apvts_ref.getParameter("sp_rng_min"),
                freq_rng_min_slider);
        freq_rng_max_slider_attachment =
            std::make_unique < SliderParameterAttachment >(
                *apvts_ref.getParameter("sp_rng_max"),
                freq_rng_max_slider);

        accent_colour_slider_attachment =
            std::make_unique < SliderParameterAttachment >(
                *apvts_ref.getParameter("ui_acc_hue"),
                accent_colour_slider);
        num_bars_slider_attachment =
            std::make_unique < SliderParameterAttachment >(
                *apvts_ref.getParameter("sp_num_brs"),
                num_bars_slider);
        bar_speed_slider_attachment =
            std::make_unique < SliderParameterAttachment >(
                *apvts_ref.getParameter("sp_bar_spd"),
                bar_speed_slider);
        spec_higlight_gate_slider_attachment =
            std::make_unique < SliderParameterAttachment >(
                *apvts_ref.getParameter("sp_high_gt"),
                spec_higlight_gate_slider);
        colourmap_bias_slider_attachment =
            std::make_unique < SliderParameterAttachment >(
                *apvts_ref.getParameter("sg_cm_bias"),
                colourmap_bias_slider);
        colourmap_curve_slider_attachment =
            std::make_unique < SliderParameterAttachment >(
                *apvts_ref.getParameter("sg_cm_curv"),
                colourmap_curve_slider);
        volume_rms_time_slider_attachment =
            std::make_unique < SliderParameterAttachment >(
                *apvts_ref.getParameter("v_rms_time"),
                volume_rms_time_slider);
        spec_history_multiply_slider_attachment =
            std::make_unique < SliderParameterAttachment >(
                *apvts_ref.getParameter("sp_multiple"),
                spec_history_multiply_slider);
        
        colourmap_combobox_attachment = 
            std::make_unique < ComboBoxParameterAttachment >
                (*apvts_ref.getParameter("gb_clrmap"), colourmap_combobox);
        channel_combobox_attachment = 
            std::make_unique < ComboBoxParameterAttachment >
                (*apvts_ref.getParameter("gb_chnl"), channel_combobox);
        scrollmode_combobox_attachment = 
            std::make_unique < ComboBoxParameterAttachment >
                (*apvts_ref.getParameter("gb_vw_mde"), scrollmode_combobox);
        fftorder_combobox_attachment = 
            std::make_unique < ComboBoxParameterAttachment >
                (*apvts_ref.getParameter("gb_fft_ord"), fftorder_combobox);
        measure_combobox_attachment =
            std::make_unique < ComboBoxParameterAttachment >
            (*apvts_ref.getParameter("sp_measure"), measure_combobox);

        listen_button_attachment =
            std::make_unique < ButtonParameterAttachment >
            (*apvts_ref.getParameter("gb_listen"), listen_button);

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
            slider_->setNumDecimalPlacesToDisplay(0);
            slider_->setSliderStyle(Slider::SliderStyle::LinearBar);
            slider_->setTextBoxStyle(juce::Slider::TextBoxRight, false, 60, 15);
            slider_->setLookAndFeel(&styles);
            slider_->setColour(Slider::ColourIds::textBoxTextColourId, Colours::orange);
            slider_->setColour(Slider::ColourIds::backgroundColourId, Colours::white);
            slider_->setColour(Slider::ColourIds::thumbColourId, Colours::crimson);
        }

    }

    void paint(Graphics& g) override
    {
        g.fillAll(Colours::white);
    }

    void resized() override
    {
        auto bounds = getLocalBounds();
        bounds.removeFromRight(10);
        bounds.removeFromLeft(10);

        auto slider_height = 0.02 * bounds.getHeight();
        auto label_height = slider_height;
        auto heading_label_height = label_height*1.4;

        auto padding = 0.1 * slider_height;

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
            label_->setFont(label_height*0.9);
        };

        for (auto label_ : {
                &global_settings_label,
                &analyser_settings_label,
                &spectrogram_settings_label,
                &correlation_settings_label,
                &ocsilloscope_settings_label })
        {
            label_->setFont(heading_label_height * 0.9);
        };

        bounds.reduce(padding, padding);

        global_settings_label.setBounds(bounds.removeFromTop(heading_label_height));

        accent_colour_slider_label.setBounds(bounds.removeFromTop(label_height));
        accent_colour_slider.setBounds(bounds.removeFromTop(slider_height));

        colourmap_combobox_label.setBounds(bounds.removeFromTop(label_height));
        colourmap_combobox.setBounds(bounds.removeFromTop(slider_height));

        channel_combobox_label.setBounds(bounds.removeFromTop(label_height));
        channel_combobox.setBounds(bounds.removeFromTop(slider_height));

        scrollmode_combobox_label.setBounds(bounds.removeFromTop(label_height));
        scrollmode_combobox.setBounds(bounds.removeFromTop(slider_height));

        fftorder_combobox_label.setBounds(bounds.removeFromTop(label_height));
        fftorder_combobox.setBounds(bounds.removeFromTop(slider_height));

        //listen_button_label.setBounds(bounds.removeFromTop(label_height));
        listen_button.setBounds(bounds.removeFromTop(slider_height));

        bounds.removeFromTop(10);

        analyser_settings_label.setBounds(bounds.removeFromTop(heading_label_height));

        freq_rng_min_label.setBounds(bounds.removeFromTop(label_height));
        freq_rng_min_slider.setBounds(bounds.removeFromTop(slider_height));
        freq_rng_max_label.setBounds(bounds.removeFromTop(label_height));
        freq_rng_max_slider.setBounds(bounds.removeFromTop(slider_height));

        num_bars_slider_label.setBounds(bounds.removeFromTop(label_height));
        num_bars_slider.setBounds(bounds.removeFromTop(slider_height));

        bar_speed_slider_label.setBounds(bounds.removeFromTop(label_height));
        bar_speed_slider.setBounds(bounds.removeFromTop(slider_height));

        spec_higlight_gate_slider_label.setBounds(bounds.removeFromTop(label_height));
        spec_higlight_gate_slider.setBounds(bounds.removeFromTop(slider_height));

        bounds.removeFromTop(10);
        spectrogram_settings_label.setBounds(bounds.removeFromTop(heading_label_height));
        
        colourmap_curve_slider_label.setBounds(bounds.removeFromTop(label_height));
        colourmap_curve_slider.setBounds(bounds.removeFromTop(slider_height));

        colourmap_bias_slider_label.setBounds(bounds.removeFromTop(label_height));
        colourmap_bias_slider.setBounds(bounds.removeFromTop(slider_height));

        measure_combobox_label.setBounds(bounds.removeFromTop(label_height));
        measure_combobox.setBounds(bounds.removeFromTop(slider_height));

        spec_history_multiply_slider_label.setBounds(bounds.removeFromTop(label_height));
        spec_history_multiply_slider.setBounds(bounds.removeFromTop(slider_height));

        bounds.removeFromTop(10);
        correlation_settings_label.setBounds(bounds.removeFromTop(heading_label_height));
        
        volume_rms_time_label.setBounds(bounds.removeFromTop(label_height));
        volume_rms_time_slider.setBounds(bounds.removeFromTop(slider_height));

    }

private:
    AudioProcessorValueTreeState& apvts_ref;

    LookAndFeel_V4 styles;

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