#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>

#include "UI_Comp/widgets/drag_move.h"
#include "UI_Comp/widgets/seperator_bar_labeler.h"
#include "UI_Comp/widgets/seperator_bar_dynamic_labeler.h"
#include "UI_Comp/widgets/icon_button.h"

using namespace juce;

class MainPage : public Component,
    public AudioProcessorValueTreeState::Listener
{
public:

    MainPage(
        AudioProcessorValueTreeState& apvts_r,
        std::function<void(bool)>& freeze_button_callback,
        std::function<void(bool)>& settings_button_callback,
        std::array<juce::Component*, 4> display_components
    );
    ~MainPage() override;

    void paint(Graphics& g) override;
    void resized() override;

    void parameterChanged(const String& parameterID, float newValue) override;

    iconButton freeze_toggle_button;
    iconButton settings_toggle_button;

private:
    AudioProcessorValueTreeState& apvts_ref;
    std::array<juce::Component*, 4> display_component_pointers;

    Label time_history_label;

    //==================== LOCAL UI CACHE ====================
    float ui_sep_x_local = 0.5f;
    float ui_sep_y_local = 0.5f;
    //========================================================

    Typeface::Ptr typeface_regular{
        Typeface::createSystemTypefaceFor(
            BinaryData::LatoRegular_ttf,
            BinaryData::LatoRegular_ttfSize)
    };
    Font cust_font_regular{ typeface_regular };

    Typeface::Ptr typeface_bold{
        Typeface::createSystemTypefaceFor(
            BinaryData::SpaceMonoBold_ttf,
            BinaryData::SpaceMonoBold_ttfSize)
    };
    Font cust_font_bold{ typeface_bold };

    std::unique_ptr<Drawable> settings_icon = Drawable::createFromImageData(
        BinaryData::settings_icon_svg, BinaryData::settings_icon_svgSize);
    std::unique_ptr<Drawable> freeze_icon = Drawable::createFromImageData(
        BinaryData::freeze_icon_svg, BinaryData::freeze_icon_svgSize);

    std::function<void(Point<int>)> move_callback =
        [this](Point<int> delta)
    {
        float width = getWidth();
        float height = getHeight();

        float paddingInPixels =
            std::clamp<int>(paddingWidthFraction * width, 2, 6);

        float ribbonHeight =
            std::clamp<int>(ribbonHeightFraction * height, 25, 35);

        ui_sep_x_local +=
            (float)delta.x / (width - 2 * paddingInPixels);

        ui_sep_y_local +=
            (float)delta.y / (height - paddingInPixels - ribbonHeight);

        ui_sep_x_local = std::clamp(ui_sep_x_local, 0.0f, 1.0f);
        ui_sep_y_local = std::clamp(ui_sep_y_local, 0.0f, 1.0f);

        resized();
    };

    std::function<void()> commit_callback =
        [this]()
    {
        auto* px = apvts_ref.getParameter("ui_sep_x");
        auto* py = apvts_ref.getParameter("ui_sep_y");

        px->beginChangeGesture();
        py->beginChangeGesture();

        px->setValueNotifyingHost(px->convertTo0to1(ui_sep_x_local));
        py->setValueNotifyingHost(py->convertTo0to1(ui_sep_y_local));

        px->endChangeGesture();
        py->endChangeGesture();
    };

    float ribbonHeightFraction = 0.045;
    float paddingWidthFraction = 0.005;
    
    float seperatorBarWidthInPixels = 25;

    MoveDragComponent move_drag_comp;

    const std::vector<std::vector<String>> volume_seperator_labels{
        {
            { "0", "", "-20", "", "-40", "", "-60", " - ", "-60", "", "-40", "", "-20", "", "0"},
            { "0", "-10", "-20", "-30", "-40", "-50", "-60", " - ", "-60", "-50", "-40", "-30", "-20", "-10", "0"},
        }
    };

    const std::vector<std::vector<String>> spectrum_volume_seperator_labels{
        {
            { "", "-60", "", "", "", "-20", "", ""},
            { "", "-60", "", "-40", "", "-20", "", "0"},
            { "", "-60", "-50", "-40", "-30", "-20", "-10", "0"},
        }
    };

    SeperatorBarLabeler volume_labels;
    SeperatorBarLabeler specrtum_volume_labels;
    LogSeperatorBarLabeler spectrum_frequency_labels;

    Label plugin_name_label;
    Label plugin_build_name_label;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainPage)
};
