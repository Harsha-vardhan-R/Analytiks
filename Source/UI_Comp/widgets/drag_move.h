#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_gui_basics/juce_gui_basics.h>

#include <BinaryData.h>

using namespace juce;

// The move kind of thing on the seperators that is used to resize different
// components.
class MoveDragComponent : 
	public Component
{
public:

	MoveDragComponent(std::function<void(Point<int>)> move_callback)
		: move_delta_callback(move_callback)
	{
		setOpaque(false);
	}

	void paint(Graphics& g) override 
	{
		if (isMouseButtonDown()) g.setColour(Colours::darkgrey);
		else g.setColour(Colours::grey);
		
		g.drawRect(g.getClipBounds(), 1.0);
		
		if (isMouseOver()) g.setColour(Colours::lightgrey);
		else if (isMouseButtonDown()) g.setColour(Colours::white);
		else g.setColour(Colours::grey);

		auto bounds = g.getClipBounds().reduced(3);

		if (!svgDrawableDull || !svgDrawableBright) return;

		auto svgBounds = svgDrawableDull->getDrawableBounds();

		AffineTransform transform = AffineTransform::fromTargetPoints(
			svgBounds.getTopLeft(), bounds.getTopLeft().toFloat(),
			svgBounds.getTopRight(), bounds.getTopRight().toFloat(),
			svgBounds.getBottomLeft(), bounds.getBottomLeft().toFloat()
		);

		if (isMouseButtonDown()) svgDrawableBright->draw(g, 1.0f, transform);
		else if (isMouseOver()) svgDrawableBright->draw(g, 0.6f, transform);
		else svgDrawableDull->draw(g, 1.0f, transform);

	}

	void resized() override {}

	void mouseEnter(const MouseEvent& event) override 
	{
		repaint();
	}
	void mouseExit(const MouseEvent& event) override 
	{
		repaint();
	}
	void mouseUp(const MouseEvent& event) override
	{
		Desktop::setMousePosition(
			getScreenBounds().getCentre()
		);
		setMouseCursor(MouseCursor::NormalCursor);
		repaint();
	}
	void mouseDown(const MouseEvent& event) override
	{
		mousePrevPosition = event.getScreenPosition();
		setMouseCursor(MouseCursor::NoCursor);
		repaint();
	}
	void mouseDrag(const MouseEvent& event) override
	{
		auto currentPos = event.getScreenPosition();
		auto delta = currentPos - mousePrevPosition;
		mousePrevPosition = currentPos;

		if (move_delta_callback)
			move_delta_callback(delta);

		repaint();
	}

private:
	std::function<void(Point<int>)> move_delta_callback;

	// difference between these two is sent to the 
	// parent, it takes the required actions.

	Point<int> mousePrevPosition;

	std::unique_ptr<Drawable> svgDrawableDull = Drawable::createFromImageData(
		BinaryData::move_svg_dull_svg, BinaryData::move_svg_dull_svgSize);
	std::unique_ptr<Drawable> svgDrawableBright = Drawable::createFromImageData(
		BinaryData::move_svg_bright_svg, BinaryData::move_svg_bright_svgSize);

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MoveDragComponent);
};