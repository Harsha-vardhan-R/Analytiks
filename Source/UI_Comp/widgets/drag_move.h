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

    MoveDragComponent(std::function<void(Point<int>)> move_callback,
                      std::function<void()> release_callback)
        : move_delta_callback(move_callback),
          release_callback_fn(release_callback)
    {
        setOpaque(false);

        auto centre = getScreenBounds().getCentre().toFloat();

        dragStartScreen  = centre;
        accumulatedDrag  = { 0.0f, 0.0f };
    }

    void paint(Graphics& g) override
    {
        if (isMouseButtonDown()) g.setColour(Colours::darkgrey);
        else g.setColour(Colours::grey);

        g.drawRect(g.getClipBounds(), 1.0f);

        if (!svgDrawableDull || !svgDrawableBright)
            return;

        if (isMouseButtonDown())
            svgDrawableBright->draw(g, 1.0f, cachedTransform);
        else if (isMouseOver())
            svgDrawableBright->draw(g, 0.6f, cachedTransform);
        else
            svgDrawableDull->draw(g, 1.0f, cachedTransform);
    }

    void resized() override
    {
        if (!svgDrawableDull)
            return;

        auto bounds = getLocalBounds().reduced(3);
        auto svgBounds = svgDrawableDull->getDrawableBounds();

        cachedTransform = AffineTransform::fromTargetPoints(
            svgBounds.getTopLeft(),     bounds.getTopLeft().toFloat(),
            svgBounds.getTopRight(),    bounds.getTopRight().toFloat(),
            svgBounds.getBottomLeft(),  bounds.getBottomLeft().toFloat()
        );
    }

    void mouseEnter(const MouseEvent&) override
    {
        repaint();
    }

    void mouseExit(const MouseEvent&) override
    {
        repaint();
    }

    void mouseDown(const MouseEvent& e) override
    {
        dragStartScreen = e.position + e.eventComponent->getScreenPosition().toFloat();
        accumulatedDrag = { 0.0f, 0.0f };

        setMouseCursor(MouseCursor::NoCursor);
        beginDragAutoRepeat(50);
    }

    void mouseUp(const MouseEvent&) override
    {
        setMouseCursor(MouseCursor::NormalCursor);

        if (release_callback_fn)
            release_callback_fn();

        Desktop::setMousePosition(getScreenBounds().getCentre());
        repaint();
    }

    void mouseDrag(const MouseEvent& e) override
    {
        auto current =
            e.position + e.eventComponent->getScreenPosition().toFloat();

        auto totalDrag = current - dragStartScreen;

        auto deltaFloat = totalDrag - accumulatedDrag;
        accumulatedDrag = totalDrag;

        Point<int> deltaInt{
            (int)std::round(deltaFloat.x),
            (int)std::round(deltaFloat.y)
        };

        if (deltaInt != Point<int>())
        {
            if (move_delta_callback)
                move_delta_callback(deltaInt);
        }
    }

private:

    std::function<void(Point<int>)> move_delta_callback;
    std::function<void()> release_callback_fn;

    Point<float> dragStartScreen;
    Point<float> accumulatedDrag;

    AffineTransform cachedTransform;

    std::unique_ptr<Drawable> svgDrawableDull =
        Drawable::createFromImageData(BinaryData::move_svg_dull_svg,
                                      BinaryData::move_svg_dull_svgSize);

    std::unique_ptr<Drawable> svgDrawableBright =
        Drawable::createFromImageData(BinaryData::move_svg_bright_svg,
                                      BinaryData::move_svg_bright_svgSize);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MoveDragComponent);
};
