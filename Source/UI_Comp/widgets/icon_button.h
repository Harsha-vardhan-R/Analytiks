#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

class iconButton : public juce::Component
{
public:
    iconButton(
        std::unique_ptr<juce::Drawable>& icon,
        std::function<void(bool)>& onClickCallback
    ) : local_icon(icon),
        local_onClickCallback(onClickCallback)
    {
		setOpaque(false);
    }

	void paint(juce::Graphics& g) override
	{
		auto bounds = getLocalBounds();
		auto amnt = (float)bounds.getWidth() * 0.1;
		bounds.reduce(amnt, amnt);

		if (down)
		{
			g.setColour(juce::Colours::white.withAlpha(0.4f));
			g.fillAll();
			bounds.reduce(amnt, amnt);
		}
		else if (hovering)
		{
			g.setColour(juce::Colours::white.withAlpha(0.2f));
			g.fillAll();
		}

		g.setColour(juce::Colours::white);
		
		auto svgBounds = local_icon->getDrawableBounds();

		juce::AffineTransform transform = juce::AffineTransform::fromTargetPoints(
			svgBounds.getTopLeft(), bounds.getTopLeft().toFloat(),
			svgBounds.getTopRight(), bounds.getTopRight().toFloat(),
			svgBounds.getBottomLeft(), bounds.getBottomLeft().toFloat()
		);

		local_icon->draw(g, 1.0f, transform);
	}

	void resized() override {}

	void mouseEnter(const juce::MouseEvent& event) override
	{
		hovering = true;
		repaint();
	}
	void mouseExit(const juce::MouseEvent& event) override
	{
		hovering = false;
		repaint();
	}
	void mouseUp(const juce::MouseEvent& event) override
	{
		state = !state;
		down = false;
		local_onClickCallback(state);
		repaint();
	}
	void mouseDown(const juce::MouseEvent& event) override
	{
		down = true;
		repaint();
	}

    bool state = false;

private:

    std::unique_ptr<juce::Drawable>& local_icon;
    std::function<void(bool)>& local_onClickCallback;
	bool down = false;
	bool hovering = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(iconButton)
};