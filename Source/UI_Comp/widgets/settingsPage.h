
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
        addAndMakeVisible(freq_range_min_slider_label);
        addAndMakeVisible(freq_range_max_slider_label);
        addAndMakeVisible(num_bars_slider_label);
        addAndMakeVisible(bar_speed_slider_label);
        addAndMakeVisible(bar_peak_hold_speed_slider_label);
        addAndMakeVisible(spec_higlight_gate_slider_label);
        addAndMakeVisible(spec_higlight_time_slider_label);
        addAndMakeVisible(colourmap_bias_slider_label);
        addAndMakeVisible(colourmap_curve_slider_label);
        addAndMakeVisible(volume_rms_time_label);
        addAndMakeVisible(listen_button_label);
        addAndMakeVisible(colourise_analyser_button_label);
        addAndMakeVisible(analyser_highlighting_button_label);
        addAndMakeVisible(oscilloscope_highlighting_button_label);
        addAndMakeVisible(colourmap_combobox_label);
        addAndMakeVisible(channel_combobox_label);
        addAndMakeVisible(scrollmode_combobox_label);
        addAndMakeVisible(vieworientation_combobox_label);
        addAndMakeVisible(fftorder_combobox_label);
        addAndMakeVisible(fftorder_combobox_label);
        addAndMakeVisible(fftorder_combobox_label);
        addAndMakeVisible(spec_history_multiply_slider_label);
        addAndMakeVisible(measure_combobox_label);
        
        addAndMakeVisible(accent_colour_slider);
        addAndMakeVisible(freq_range_min_slider);
        addAndMakeVisible(freq_range_max_slider);
        addAndMakeVisible(num_bars_slider);
        addAndMakeVisible(bar_speed_slider);
        addAndMakeVisible(bar_peak_hold_speed_slider);
        addAndMakeVisible(spec_higlight_gate_slider);
        addAndMakeVisible(spec_higlight_time_slider);
        addAndMakeVisible(colourmap_bias_slider);
        addAndMakeVisible(colourmap_curve_slider);
        addAndMakeVisible(volume_rms_time_slider);

        addAndMakeVisible(listen_button);
        addAndMakeVisible(colourise_analyser_button);
        addAndMakeVisible(analyser_highlighting_button);
        addAndMakeVisible(oscilloscope_highlighting_button);

        addAndMakeVisible(colourmap_combobox);
        addAndMakeVisible(channel_combobox);
        addAndMakeVisible(scrollmode_combobox);
        addAndMakeVisible(vieworientation_combobox);
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

        auto* param4 = dynamic_cast<juce::AudioParameterChoice*>(apvts_r.getParameter("gb_vw_ortn"));
        for (int i = 0; i < param4->choices.size(); ++i)
            vieworientation_combobox.addItem(param4->choices[i], i + 1);

        auto* param5 = dynamic_cast<juce::AudioParameterChoice*>(apvts_r.getParameter("gb_fft_ord"));
        for (int i = 0; i < param5->choices.size(); ++i)
            fftorder_combobox.addItem(param5->choices[i], i + 1);

        auto* param6 = dynamic_cast<juce::AudioParameterChoice*>(apvts_r.getParameter("sp_measure"));
        for (int i = 0; i < param6->choices.size(); ++i)
            measure_combobox.addItem(param6->choices[i], i + 1);

        accent_colour_slider_label.setText("UI Colour", juce::dontSendNotification);
        freq_range_min_slider_label.setText("Freq range min(Hz)", juce::dontSendNotification);
        freq_range_max_slider_label.setText("Freq range max(Hz)", juce::dontSendNotification);
        num_bars_slider_label.setText("Number of Bars", juce::dontSendNotification);
        bar_speed_slider_label.setText("Bar Speed(ms)", juce::dontSendNotification);
        bar_peak_hold_speed_slider_label.setText("Peak Hold(ms)", juce::dontSendNotification);
        spec_higlight_gate_slider_label.setText("Gate(dB)", juce::dontSendNotification);
        spec_higlight_time_slider_label.setText("Highlight Speed(ms)", juce::dontSendNotification);
        colourmap_bias_slider_label.setText("Bias", juce::dontSendNotification);
        colourmap_curve_slider_label.setText("Curve", juce::dontSendNotification);
        volume_rms_time_label.setText("Volume RMS Window(ms)", juce::dontSendNotification);
        listen_button_label.setText("Listen", juce::dontSendNotification);
        colourise_analyser_button_label.setText("Colourise", juce::dontSendNotification);
        analyser_highlighting_button_label.setText("Highlight", juce::dontSendNotification);
        oscilloscope_highlighting_button_label.setText("Highlight", juce::dontSendNotification);
        colourmap_combobox_label.setText("Colourmap", juce::dontSendNotification);
        channel_combobox_label.setText("Channel", juce::dontSendNotification);
        scrollmode_combobox_label.setText("Scrolling", juce::dontSendNotification);
        vieworientation_combobox_label.setText("Orientation", juce::dontSendNotification);
        fftorder_combobox_label.setText("FFT order", juce::dontSendNotification);
        spec_history_multiply_slider_label.setText("Multiple", juce::dontSendNotification);
        measure_combobox_label.setText("Bars", juce::dontSendNotification);

        global_settings_label.setText("GLOBAL", juce::dontSendNotification);
        analyser_settings_label.setText("ANALYSER", juce::dontSendNotification);
        spectrogram_settings_label.setText("SPECTROGRAM", juce::dontSendNotification);
        correlation_settings_label.setText("CORRELATION", juce::dontSendNotification);
        ocsilloscope_settings_label.setText("OSCILLOSCOPE", juce::dontSendNotification);
        

        for (auto box_ : {
                &colourmap_combobox,
                &channel_combobox,
                &scrollmode_combobox,
                &vieworientation_combobox,
                &fftorder_combobox,
                &measure_combobox
            })
        {
            box_->setLookAndFeel(&styles);
        }

        for (auto button_ : {
                &listen_button,
                &colourise_analyser_button,
                &analyser_highlighting_button,
                &oscilloscope_highlighting_button
            })
        {
            button_->setLookAndFeel(&styles);

            button_->setColour(ToggleButton::ColourIds::textColourId, Colours::black);
            button_->setColour(ToggleButton::ColourIds::tickColourId, Colours::black);
            button_->setColour(ToggleButton::ColourIds::tickDisabledColourId, Colours::black);
        }

        listen_button.setButtonText("Listen");
        colourise_analyser_button.setButtonText("Colourise");
        analyser_highlighting_button.setButtonText("Highlight");
        oscilloscope_highlighting_button.setButtonText("Highlight");

        listen_button.setToggleable(true);
        colourise_analyser_button.setToggleable(true);
        analyser_highlighting_button.setToggleable(true);
        oscilloscope_highlighting_button.setToggleable(true);

        for (auto label_ : {
                &global_settings_label,
                &analyser_settings_label,
                &spectrogram_settings_label,
                &correlation_settings_label,
                &ocsilloscope_settings_label,
                &accent_colour_slider_label,
                &freq_range_min_slider_label,
                &freq_range_max_slider_label,
                &num_bars_slider_label,
                &bar_speed_slider_label,
                &bar_peak_hold_speed_slider_label,
                &spec_higlight_gate_slider_label,
                &spec_higlight_time_slider_label,
                &colourmap_bias_slider_label,
                &colourmap_curve_slider_label,
                &volume_rms_time_label,
                &listen_button_label,
                &colourise_analyser_button_label,
                &analyser_highlighting_button_label,
                &oscilloscope_highlighting_button_label,
                &colourmap_combobox_label,
                &channel_combobox_label,
                &scrollmode_combobox_label,
                &vieworientation_combobox_label,
                &fftorder_combobox_label,
                &spec_history_multiply_slider_label,
                &measure_combobox_label
            })
        {
            label_->setColour(Label::ColourIds::textColourId, Colours::black);
        };

        accent_colour_slider_attachment =
            std::make_unique < SliderParameterAttachment >(
                *apvts_ref.getParameter("ui_acc_hue"),
                accent_colour_slider);
        freq_range_min_slider_attachment =
            std::make_unique < SliderParameterAttachment >(
                *apvts_ref.getParameter("sp_rng_min"),
                freq_range_min_slider);
        freq_range_max_slider_attachment =
            std::make_unique < SliderParameterAttachment >(
                *apvts_ref.getParameter("sp_rng_max"),
                freq_range_max_slider);
        num_bars_slider_attachment =
            std::make_unique < SliderParameterAttachment >(
                *apvts_ref.getParameter("sp_num_brs"),
                num_bars_slider);
        bar_speed_slider_attachment =
            std::make_unique < SliderParameterAttachment >(
                *apvts_ref.getParameter("sp_bar_spd"),
                bar_speed_slider);
        bar_peak_hold_speed_slider_attachment =
            std::make_unique < SliderParameterAttachment >(
                *apvts_ref.getParameter("sp_pek_hld"),
                bar_peak_hold_speed_slider);
        spec_higlight_gate_slider_attachment =
            std::make_unique < SliderParameterAttachment >(
                *apvts_ref.getParameter("sp_high_gt"),
                spec_higlight_gate_slider);
        spec_higlight_time_slider_attachment =
            std::make_unique < SliderParameterAttachment >(
                *apvts_ref.getParameter("sp_high_tm"),
                spec_higlight_time_slider);
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
        vieworientation_combobox_attachment = 
            std::make_unique < ComboBoxParameterAttachment >
                (*apvts_ref.getParameter("gb_vw_ortn"), vieworientation_combobox);
        fftorder_combobox_attachment = 
            std::make_unique < ComboBoxParameterAttachment >
                (*apvts_ref.getParameter("gb_fft_ord"), fftorder_combobox);
        measure_combobox_attachment =
            std::make_unique < ComboBoxParameterAttachment >
            (*apvts_ref.getParameter("sp_measure"), measure_combobox);

        listen_button_attachment =
            std::make_unique < ButtonParameterAttachment >
            (*apvts_ref.getParameter("gb_listen"), listen_button);
        colourise_analyser_button_attachment =
            std::make_unique < ButtonParameterAttachment >
            (*apvts_ref.getParameter("sp_accent"), colourise_analyser_button);
        analyser_highlighting_button_attachment =
            std::make_unique < ButtonParameterAttachment >
            (*apvts_ref.getParameter("sp_high"), analyser_highlighting_button);
        oscilloscope_highlighting_button_attachment =
            std::make_unique < ButtonParameterAttachment >
            (*apvts_ref.getParameter("os_high"), oscilloscope_highlighting_button);

        for (auto slider_ : {
                &accent_colour_slider,
                &freq_range_min_slider,
                &freq_range_max_slider,
                &num_bars_slider,
                &bar_speed_slider,
                &bar_peak_hold_speed_slider,
                &spec_higlight_gate_slider,
                &spec_higlight_time_slider,
                &colourmap_bias_slider,
                &colourmap_curve_slider,
                &volume_rms_time_slider,
                &spec_history_multiply_slider
            })
        {
            slider_->setNumDecimalPlacesToDisplay(0);
            slider_->setSliderStyle(Slider::SliderStyle::LinearHorizontal);
            slider_->setTextBoxStyle(juce::Slider::TextBoxRight, false, 60, 15);
            slider_->setLookAndFeel(&styles);
            slider_->setColour(Slider::ColourIds::textBoxTextColourId, Colours::black);
            //slider_->setColour(Slider::ColourIds::trackColourId, Colours::orange);
        }

    }

    void paint(Graphics& g) override
    {
        g.fillAll(Colours::white);
    }

    void resized() override
    {
        auto bounds = getLocalBounds();

        auto slider_height = 0.022 * bounds.getHeight();
        auto label_height = slider_height*0.75;
        auto heading_label_height = slider_height*1.1;

        auto padding = 0.1 * slider_height;

        for (auto label_ : {
                &accent_colour_slider_label,
                &freq_range_min_slider_label,
                &freq_range_max_slider_label,
                &num_bars_slider_label,
                &bar_speed_slider_label,
                &bar_peak_hold_speed_slider_label,
                &spec_higlight_gate_slider_label,
                &spec_higlight_time_slider_label,
                &colourmap_bias_slider_label,
                &colourmap_curve_slider_label,
                &volume_rms_time_label,
                &listen_button_label,
                &colourise_analyser_button_label,
                &analyser_highlighting_button_label,
                &oscilloscope_highlighting_button_label,
                &colourmap_combobox_label,
                &channel_combobox_label,
                &scrollmode_combobox_label,
                &vieworientation_combobox_label,
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

        vieworientation_combobox_label.setBounds(bounds.removeFromTop(label_height));
        vieworientation_combobox.setBounds(bounds.removeFromTop(slider_height));

        fftorder_combobox_label.setBounds(bounds.removeFromTop(label_height));
        fftorder_combobox.setBounds(bounds.removeFromTop(slider_height));

        freq_range_min_slider_label.setBounds(bounds.removeFromTop(label_height));
        freq_range_min_slider.setBounds(bounds.removeFromTop(slider_height));

        freq_range_max_slider_label.setBounds(bounds.removeFromTop(label_height));
        freq_range_max_slider.setBounds(bounds.removeFromTop(slider_height));

        //listen_button_label.setBounds(bounds.removeFromTop(label_height));
        listen_button.setBounds(bounds.removeFromTop(slider_height));

        analyser_settings_label.setBounds(bounds.removeFromTop(heading_label_height));

        num_bars_slider_label.setBounds(bounds.removeFromTop(label_height));
        num_bars_slider.setBounds(bounds.removeFromTop(slider_height));

        bar_speed_slider_label.setBounds(bounds.removeFromTop(label_height));
        bar_speed_slider.setBounds(bounds.removeFromTop(slider_height));

        bar_peak_hold_speed_slider_label.setBounds(bounds.removeFromTop(label_height));
        bar_peak_hold_speed_slider.setBounds(bounds.removeFromTop(slider_height));

        auto temp_bounds1 = bounds.removeFromTop(slider_height);

        colourise_analyser_button.setBounds(temp_bounds1.removeFromLeft(temp_bounds1.getWidth() / 2));
        analyser_highlighting_button.setBounds(temp_bounds1);

        spec_higlight_gate_slider_label.setBounds(bounds.removeFromTop(label_height));
        spec_higlight_gate_slider.setBounds(bounds.removeFromTop(slider_height));

        spec_higlight_time_slider_label.setBounds(bounds.removeFromTop(label_height));
        spec_higlight_time_slider.setBounds(bounds.removeFromTop(slider_height));

        spectrogram_settings_label.setBounds(bounds.removeFromTop(heading_label_height));

        colourmap_bias_slider_label.setBounds(bounds.removeFromTop(label_height));
        colourmap_bias_slider.setBounds(bounds.removeFromTop(slider_height));
        
        colourmap_curve_slider_label.setBounds(bounds.removeFromTop(label_height));
        colourmap_curve_slider.setBounds(bounds.removeFromTop(slider_height));

        measure_combobox_label.setBounds(bounds.removeFromTop(label_height));
        measure_combobox.setBounds(bounds.removeFromTop(slider_height));

        spec_history_multiply_slider_label.setBounds(bounds.removeFromTop(label_height));
        spec_history_multiply_slider.setBounds(bounds.removeFromTop(slider_height));

        ocsilloscope_settings_label.setBounds(bounds.removeFromTop(heading_label_height));

        //oscilloscope_highlighting_button_label.setBounds(bounds.removeFromTop(label_height));
        oscilloscope_highlighting_button.setBounds(bounds.removeFromTop(slider_height));

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
        freq_range_min_slider_label,
        freq_range_max_slider_label,
        num_bars_slider_label,
        bar_speed_slider_label,
        bar_peak_hold_speed_slider_label,
        spec_higlight_gate_slider_label,
        spec_higlight_time_slider_label,
        colourmap_bias_slider_label,
        colourmap_curve_slider_label,
        volume_rms_time_label,
        listen_button_label,
        colourise_analyser_button_label,
        analyser_highlighting_button_label,
        oscilloscope_highlighting_button_label,
        colourmap_combobox_label,
        channel_combobox_label,
        scrollmode_combobox_label,
        vieworientation_combobox_label,
        fftorder_combobox_label,
        spec_history_multiply_slider_label,
        measure_combobox_label;

    Slider
        accent_colour_slider,
        freq_range_min_slider,
        freq_range_max_slider,
        num_bars_slider,
        bar_speed_slider,
        bar_peak_hold_speed_slider,
        spec_higlight_gate_slider,
        spec_higlight_time_slider,
        colourmap_bias_slider,
        colourmap_curve_slider,
        volume_rms_time_slider,
        spec_history_multiply_slider;

    ToggleButton
        listen_button,
        colourise_analyser_button,
        analyser_highlighting_button,
        oscilloscope_highlighting_button;

    ComboBox
        colourmap_combobox,
        measure_combobox,
        channel_combobox,
        scrollmode_combobox,
        vieworientation_combobox,
        fftorder_combobox;

    std::unique_ptr<SliderParameterAttachment>
        accent_colour_slider_attachment,
        freq_range_min_slider_attachment,
        freq_range_max_slider_attachment,
        num_bars_slider_attachment,
        bar_speed_slider_attachment,
        bar_peak_hold_speed_slider_attachment,
        spec_higlight_gate_slider_attachment,
        spec_higlight_time_slider_attachment,
        colourmap_bias_slider_attachment,
        colourmap_curve_slider_attachment,
        volume_rms_time_slider_attachment,
        spec_history_multiply_slider_attachment;

    std::unique_ptr<ComboBoxParameterAttachment>
        colourmap_combobox_attachment,
        channel_combobox_attachment,
        scrollmode_combobox_attachment,
        vieworientation_combobox_attachment,
        fftorder_combobox_attachment,
        measure_combobox_attachment;
        
    std::unique_ptr<ButtonParameterAttachment>
        listen_button_attachment,
        colourise_analyser_button_attachment,
        analyser_highlighting_button_attachment,
        oscilloscope_highlighting_button_attachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(settingsPage)

};