#pragma once

#include<juce_gui_basics/juce_gui_basics.h>

class SeperatorBarLabeler : public juce::Component
{
public:

	SeperatorBarLabeler(
		juce::Typeface::Ptr fontTypeface
	) : CustomFont(fontTypeface)
	{
		setOpaque(false);
	}

	std::vector<int> possible_frequencies =
	{
		
	};

	std::function<float(float)> lin_to_log = 
		[](float val) -> float 
	{
		
	};

	void paint(juce::Graphics& g) override
	{
		// Decide on how many text labels we are going to use.
		// do not worry it is never going to be a square.
		float bigger_dim_val = std::max<int>(getWidth(), getHeight());
		float smaller_dim_val = std::min<int>(getWidth(), getHeight());

		int index = 0;

		g.setColour(juce::Colours::white);

		
	}

	void resized() override
	{
	}

	void setNewRange(juce::Range<float>& rng)
	{
		min_val = rng.getStart();
		max_val = rng.getEnd();
	}

private:
	juce::Typeface::Ptr CustomFont;

	float textHeightFraction = 0.6;

	int
		min_val = 10,
		max_val = 20000;

	// This is required as it will be used to calculate 
	// how many labels can we fit, and how many to hide.
	int TextWidthPixels = 20;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SeperatorBarLabeler)
};