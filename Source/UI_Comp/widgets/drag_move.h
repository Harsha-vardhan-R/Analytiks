#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_gui_basics/juce_gui_basics.h>

#include <BinaryData.h>

// The move kind of thing on the seperators that is used to resize different
// components.
class MoveDragComponent : 
	public juce::Component,
	public juce::MouseListener		
{
public:

	MoveDragComponent(std::function<void(juce::Point<int>)> move_callback)
		: move_delta_callback(move_callback)
	{
		setOpaque(false);
	}

	void paint(juce::Graphics& g) override 
	{
		if (isMouseButtonDown()) g.setColour(juce::Colours::grey);
		else g.setColour(juce::Colours::white);
		
		g.drawRect(g.getClipBounds(), 1.0);
		
		if (isMouseOver()) g.setColour(juce::Colours::lightgrey);
		else if (isMouseButtonDown()) g.setColour(juce::Colours::white);
		else g.setColour(juce::Colours::grey);

		auto bounds = g.getClipBounds().reduced(3);

		if (!svgDrawableDull || !svgDrawableBright) return;

		auto svgBounds = svgDrawableDull->getDrawableBounds();

		juce::AffineTransform transform = juce::AffineTransform::fromTargetPoints(
			svgBounds.getTopLeft(), bounds.getTopLeft().toFloat(),
			svgBounds.getTopRight(), bounds.getTopRight().toFloat(),
			svgBounds.getBottomLeft(), bounds.getBottomLeft().toFloat()
		);

		if (isMouseOverOrDragging()) svgDrawableBright->draw(g, 1.0f, transform);
		else svgDrawableDull->draw(g, 1.0f, transform);

	}

	void resized() override {}

	void mouseEnter(const juce::MouseEvent& event) override 
	{
		repaint();
	}
	void mouseExit(const juce::MouseEvent& event) override 
	{
		repaint();
	}
	void mouseUp(const juce::MouseEvent& event) override
	{
		repaint();
	}
	void mouseDown(const juce::MouseEvent& event) override
	{
		mousePrevPosition = event.getScreenPosition();
	}
	void mouseDrag(const juce::MouseEvent& event) override
	{
		auto currentPos = event.getScreenPosition();
		auto delta = currentPos - mousePrevPosition;
		mousePrevPosition = currentPos;

		if (move_delta_callback)
			move_delta_callback(delta);

		repaint();
	}

private:
	std::function<void(juce::Point<int>)> move_delta_callback;

	// difference between these two is sent to the 
	// parent, it takes the required actions.

	juce::Point<int> mousePrevPosition;

	std::unique_ptr<juce::Drawable> svgDrawableDull = juce::Drawable::createFromImageData(
		BinaryData::move_svg_dull_svg, BinaryData::move_svg_dull_svgSize);
	std::unique_ptr<juce::Drawable> svgDrawableBright = juce::Drawable::createFromImageData(
		BinaryData::move_svg_bright_svg, BinaryData::move_svg_bright_svgSize);

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MoveDragComponent);
};