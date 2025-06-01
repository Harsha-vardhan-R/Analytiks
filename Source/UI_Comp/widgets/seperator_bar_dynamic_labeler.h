#pragma once

#include<juce_gui_basics/juce_gui_basics.h>

class LogSeperatorBarLabeler : public juce::Component
{
public:

	LogSeperatorBarLabeler(
		juce::Typeface::Ptr font_typeface
	) : CustomFont(font_typeface)
	{
		setOpaque(false);
	}

	const std::vector<int> possible_frequencies =
	{
		10,
		20,
		30,
		40,
		50,
		60,
		70,
		80,
		90,
		100,
		200,
		300,
		400,
		500,
		600,
		700,
		800,
		900,
		1000,
		2000,
		3000,
		4000,
		5000,
		6000,
		7000,
		8000,
		9000,
		10000,
		15000,
		20000
	};

	std::function<float(float)> linToLog = 
		[](float val) -> float 
	{
			return log10f(std::max<float>(1,val));
	};

	int removeTrailingZeros(int value)
	{
		if (value == 0)
			return 0;
		while (value % 10 == 0)
			value /= 10;
		return value;
	}

	void paint(juce::Graphics& g) override
	{
		// Decide on how many text labels we are going to use.
		// do not worry it is never going to be a square.
		float bigger_dim_val = std::max<int>(getWidth(), getHeight());
		float smaller_dim_val = std::min<int>(getWidth(), getHeight());

		g.setColour(juce::Colours::lightgrey);

		juce::Font custom_font(CustomFont);
		g.setFont(custom_font);
		g.setFont(textHeightFraction * smaller_dim_val);

		// ranges in log.
		float logd_min = linToLog(min_val);
		float logd_max = linToLog(max_val);

		int val_count = 0;
		for (auto& freq_val : possible_frequencies)
		{
			if (freq_val <= max_val && freq_val >= min_val)
			{
				val_count++;
			}
		}

		// increasing ignore_level makes more values to get ignored.
		float heuristic_approx = (float)(val_count*TextWidthPixels*2.5) / (bigger_dim_val);
 		int ignore_level = std::clamp<int>(heuristic_approx, 0, 5);

		// we know the range for which we need to show the frequencies,
		// map the bigger dim values to the range and show the 
		// values from `possible_frequencies` from the given range mapped with a
		// log scale.
		for (auto& freq_val : possible_frequencies)
		{

			if (freq_val <= max_val && freq_val >= min_val)
			{
				int rem_trail = removeTrailingZeros(freq_val);

				// looks bad but works.
				if (ignore_level > 0) {
					if (rem_trail == 9) {
						continue;
					}
					else if (ignore_level > 1) {
						if (rem_trail == 7) {
							continue;
						}
						else if (ignore_level > 2) {
							if (rem_trail == 6 || rem_trail==4) {
								continue;
							}
							else if (ignore_level > 3)
							{
								if (rem_trail == 8 || rem_trail == 15) {
									continue;
								} 
								else if (ignore_level > 4)
								{
									if (rem_trail != 1) continue;
								}
							}
						}
					}
				}


				float logd_val = linToLog(freq_val);

				// see where this lies in our range.
				auto offset =
					juce::jmap<float>(
						logd_val,
						logd_min,
						logd_max,
						0.0,
						getHeight() - TextWidthPixels
					);
				// but we  want inverse of this as the order is small at the bottom.
				auto text_y = getHeight() - offset;

				juce::Rectangle<int> text_bounds(
					1.0,
					text_y-TextWidthPixels,
					getWidth(),
					TextWidthPixels
				);

				g.drawText(
					
					juce::String((freq_val > 999) ? freq_val/1000 : freq_val) +
					((freq_val > 999) ? "K" : ""),

					text_bounds,
					juce::Justification::centredLeft
				);
			}
		}
		
	}

	void resized() override
	{
	}

	void setNewRange(juce::Range<float>& rng)
	{
		min_val = rng.getStart();
		max_val = rng.getEnd();

		repaint();
	}

private:
	juce::Typeface::Ptr CustomFont;

	float textHeightFraction = 0.7;

	std::atomic<int>
		min_val = 10,
		max_val = 20000;

	// This is required as it will be used to calculate 
	// how many labels can we fit, and how many to hide.
	int TextWidthPixels = 15;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LogSeperatorBarLabeler)
};