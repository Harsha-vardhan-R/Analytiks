#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

using namespace juce;

class iconButton : public Component
{
public:
    iconButton(
        std::unique_ptr<Drawable>& icon,
        std::function<void(bool)>& onClickCallback
    ) : local_icon(icon),
        local_onClickCallback(onClickCallback)
    {
		setOpaque(false);
    }

	void paint(Graphics& g) override
	{
		auto bounds = getLocalBounds();
		auto amnt = (float)bounds.getWidth() * 0.15;

		if (down || state)
		{
			g.setColour(Colours::white.withAlpha(0.2f));
			g.fillEllipse(bounds.toFloat());
		}
		if (hovering)
		{
			g.setColour(Colours::white.withAlpha(0.2f));
			g.fillEllipse(bounds.toFloat());
		}

		bounds.reduce(amnt, amnt);

		auto svgBounds = local_icon->getDrawableBounds();

		AffineTransform transform = AffineTransform::fromTargetPoints(
			svgBounds.getTopLeft(), bounds.getTopLeft().toFloat(),
			svgBounds.getTopRight(), bounds.getTopRight().toFloat(),
			svgBounds.getBottomLeft(), bounds.getBottomLeft().toFloat()
		);

		local_icon->draw(g, (state) ? 1.0f : 0.7 , transform);
	}

	void resized() override {}

	void mouseEnter(const MouseEvent& event) override
	{
		hovering = true;
		repaint();
	}
	void mouseExit(const MouseEvent& event) override
	{
		hovering = false;
		repaint();
	}
	void mouseUp(const MouseEvent& event) override
	{
		down = false;
		repaint();
	}
	void mouseDown(const MouseEvent& event) override
	{
		state = !state;
		local_onClickCallback(state);
		down = true;
		repaint();
	}

    bool state = false;

private:

    std::unique_ptr<Drawable>& local_icon;
    std::function<void(bool)>& local_onClickCallback;
	bool down = false;
	bool hovering = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(iconButton)
};