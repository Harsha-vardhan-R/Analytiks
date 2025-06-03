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
		auto amnt = (float)bounds.getWidth() * 0.15;

		if (down || state)
		{
			g.setColour(juce::Colours::white.withAlpha(0.2f));
			g.fillEllipse(bounds.toFloat());
		}
		if (hovering)
		{
			g.setColour(juce::Colours::white.withAlpha(0.2f));
			g.fillEllipse(bounds.toFloat());
		}

		bounds.reduce(amnt, amnt);

		auto svgBounds = local_icon->getDrawableBounds();

		juce::AffineTransform transform = juce::AffineTransform::fromTargetPoints(
			svgBounds.getTopLeft(), bounds.getTopLeft().toFloat(),
			svgBounds.getTopRight(), bounds.getTopRight().toFloat(),
			svgBounds.getBottomLeft(), bounds.getBottomLeft().toFloat()
		);

		local_icon->draw(g, (state) ? 1.0f : 0.7 , transform);
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
		down = false;
		repaint();
	}
	void mouseDown(const juce::MouseEvent& event) override
	{
		state = !state;
		local_onClickCallback(state);
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