#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

using namespace juce;

class ModernLookAndFeel : public LookAndFeel_V4
{
public:
    ModernLookAndFeel()
    {
        // Modern color scheme
        setColour(ComboBox::backgroundColourId, Colour(0xff2a2a2a));
        setColour(ComboBox::textColourId, Colours::white);
        setColour(ComboBox::outlineColourId, Colour(0xff404040));
        setColour(ComboBox::arrowColourId, Colour(0xff00a8ff));
        
        setColour(PopupMenu::backgroundColourId, Colour(0xff2a2a2a));
        setColour(PopupMenu::textColourId, Colours::white);
        setColour(PopupMenu::highlightedBackgroundColourId, Colour(0xff00a8ff));
        setColour(PopupMenu::highlightedTextColourId, Colours::white);
        
        setColour(Slider::backgroundColourId, Colour(0xff1a1a1a));
        setColour(Slider::thumbColourId, Colour(0xff00a8ff));
        setColour(Slider::trackColourId, Colour(0xff404040));
        setColour(Slider::textBoxTextColourId, Colours::white);
        setColour(Slider::textBoxBackgroundColourId, Colour(0xff2a2a2a));
        setColour(Slider::textBoxOutlineColourId, Colours::transparentBlack);
        
        setColour(ToggleButton::textColourId, Colours::white);
        setColour(ToggleButton::tickColourId, Colour(0xff00a8ff));
        setColour(ToggleButton::tickDisabledColourId, Colour(0xff404040));
    }
    
    void drawComboBox(Graphics& g, int width, int height, bool isButtonDown,
                      int buttonX, int buttonY, int buttonW, int buttonH,
                      ComboBox& box) override
    {
        auto cornerSize = jmin(height * 0.15f, 6.0f);
        Rectangle<int> boxBounds(0, 0, width, height);
        
        // Background with gradient
        g.setGradientFill(ColourGradient(Colour(0xff2a2a2a), 0, 0,
                                         Colour(0xff1f1f1f), 0, (float)height, false));
        g.fillRoundedRectangle(boxBounds.toFloat(), cornerSize);
        
        // Border
        g.setColour(box.hasKeyboardFocus(true) ? Colour(0xff00a8ff) : Colour(0xff404040));
        g.drawRoundedRectangle(boxBounds.toFloat().reduced(0.5f, 0.5f), cornerSize, 1.0f);
        
        // Arrow - scale with height
        auto arrowWidth = jmin(height * 0.5f, 20.0f);
        Rectangle<int> arrowZone(width - arrowWidth - 5, 0, arrowWidth, height);
        Path path;
        auto arrowSize = height * 0.25f;
        path.startNewSubPath((float)arrowZone.getCentreX() - arrowSize * 0.4f, (float)arrowZone.getCentreY() - arrowSize * 0.2f);
        path.lineTo((float)arrowZone.getCentreX(), (float)arrowZone.getCentreY() + arrowSize * 0.3f);
        path.lineTo((float)arrowZone.getCentreX() + arrowSize * 0.4f, (float)arrowZone.getCentreY() - arrowSize * 0.2f);
        
        auto strokeWidth = jmax(1.0f, height * 0.08f);
        g.setColour(box.findColour(ComboBox::arrowColourId).withAlpha(box.isEnabled() ? 0.9f : 0.2f));
        g.strokePath(path, PathStrokeType(strokeWidth));
    }
    
    void drawLinearSlider(Graphics& g, int x, int y, int width, int height,
                         float sliderPos, float minSliderPos, float maxSliderPos,
                         const Slider::SliderStyle style, Slider& slider) override
    {
        if (style == Slider::LinearBar || style == Slider::LinearBarVertical)
        {
            auto cornerSize = jmin(height * 0.15f, 6.0f);
            auto bounds = Rectangle<int>(x, y, width, height).toFloat();
            
            // Background
            g.setColour(Colour(0xff1a1a1a));
            g.fillRoundedRectangle(bounds, cornerSize);
            
            // Fill (progress bar)
            auto fillBounds = bounds;
            if (style == Slider::LinearBar)
                fillBounds.setWidth((sliderPos - x));
            else
                fillBounds.setHeight((sliderPos - y));
            
            // Gradient fill
            g.setGradientFill(ColourGradient(Colour(0xff00a8ff), fillBounds.getX(), fillBounds.getY(),
                                             Colour(0xff0088cc), fillBounds.getRight(), fillBounds.getBottom(), false));
            g.fillRoundedRectangle(fillBounds, cornerSize);
            
            // Border
            g.setColour(slider.hasKeyboardFocus(true) ? Colour(0xff00a8ff) : Colour(0xff404040));
            g.drawRoundedRectangle(bounds.reduced(0.5f), cornerSize, 1.0f);
        }
        else
        {
            LookAndFeel_V4::drawLinearSlider(g, x, y, width, height, sliderPos, 
                                            minSliderPos, maxSliderPos, style, slider);
        }
    }
    
    void drawToggleButton(Graphics& g, ToggleButton& button,
                         bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override
    {
        auto fontSize = jmin(15.0f, (float)button.getHeight() * 0.65f);
        auto tickWidth = fontSize * 1.5f;
        
        auto bounds = button.getLocalBounds().toFloat();
        auto tickBounds = bounds.removeFromLeft(tickWidth).reduced(2);
        
        // Draw rounded rectangle for toggle
        auto cornerSize = tickBounds.getHeight() * 0.2f;
        
        if (button.getToggleState())
        {
            g.setColour(Colour(0xff00a8ff));
            g.fillRoundedRectangle(tickBounds, cornerSize);
        }
        else
        {
            g.setColour(Colour(0xff2a2a2a));
            g.fillRoundedRectangle(tickBounds, cornerSize);
            g.setColour(Colour(0xff404040));
            g.drawRoundedRectangle(tickBounds, cornerSize, 1.0f);
        }
        
        // Draw checkmark if toggled
        if (button.getToggleState())
        {
            Path tick;
            auto strokeWidth = jmax(1.5f, tickBounds.getHeight() * 0.1f);
            tick.startNewSubPath(tickBounds.getX() + tickBounds.getWidth() * 0.25f,
                               tickBounds.getCentreY());
            tick.lineTo(tickBounds.getX() + tickBounds.getWidth() * 0.45f,
                       tickBounds.getCentreY() + tickBounds.getHeight() * 0.2f);
            tick.lineTo(tickBounds.getRight() - tickBounds.getWidth() * 0.2f,
                       tickBounds.getY() + tickBounds.getHeight() * 0.3f);
            
            g.setColour(Colours::white);
            g.strokePath(tick, PathStrokeType(strokeWidth));
        }
        
        // Draw text
        g.setColour(button.findColour(ToggleButton::textColourId));
        g.setFont(fontSize);
        
        g.drawText(button.getButtonText(),
                  bounds.withTrimmedLeft(4.0f),
                  Justification::centredLeft, true);
    }
    
    Font getComboBoxFont(ComboBox& box) override
    {
        return Font(jmin(16.0f, (float)box.getHeight() * 0.85f));
    }
    
    Font getPopupMenuFont() override
    {
        return Font(15.0f);
    }
    
    void drawPopupMenuBackground(Graphics& g, int width, int height) override
    {
        g.fillAll(Colour(0xff2a2a2a));
        g.setColour(Colour(0xff404040));
        g.drawRect(0, 0, width, height, 1);
    }
    
    void drawPopupMenuItem(Graphics& g, const Rectangle<int>& area,
                          bool isSeparator, bool isActive, bool isHighlighted,
                          bool isTicked, bool hasSubMenu, const String& text,
                          const String& shortcutKeyText, const Drawable* icon,
                          const Colour* textColour) override
    {
        if (isSeparator)
        {
            auto r = area.reduced(5, 0);
            r.removeFromTop(r.getHeight() / 2);
            g.setColour(Colour(0xff404040));
            g.fillRect(r.removeFromTop(1));
        }
        else
        {
            auto textColourToUse = findColour(PopupMenu::textColourId);
            
            if (isHighlighted && isActive)
            {
                g.setColour(Colour(0xff00a8ff));
                g.fillRect(area);
                textColourToUse = findColour(PopupMenu::highlightedTextColourId);
            }
            
            if (!isActive)
                textColourToUse = textColourToUse.withAlpha(0.3f);
            
            g.setColour(textColourToUse);
            g.setFont(getPopupMenuFont());
            
            auto r = area.reduced(1);
            
            if (isTicked)
            {
                auto tickBounds = r.removeFromLeft(r.getHeight()).toFloat().reduced(4.0f);
                Path tick;
                tick.addEllipse(tickBounds);
                g.fillPath(tick);
            }
            
            r.removeFromLeft(10);
            g.drawText(text, r, Justification::centredLeft, true);
        }
    }
};