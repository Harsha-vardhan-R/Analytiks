#pragma once

#include<juce_gui_basics/juce_gui_basics.h>

using namespace juce;

// TODO : Refactor this class's paint.
class SeperatorBarLabeler : public Component
{
public:

	SeperatorBarLabeler(
		Typeface::Ptr fontTypeface,
		// I'm taking the easy way out,
		// instead of dynamically calculating which labels to show and hide while 
		// resizing, just take like 3-4 different lists of labels and switch between them based on
		// the current height/width of the component.
		std::vector<std::vector<String>> label_sets
	) : CustomFont(fontTypeface),
		label_sets(label_sets)
	{
		setOpaque(false);
	}
	
	std::function<int(const std::vector<String>&)> num_non_empty_strs = 
		[](const std::vector<String>& vec) -> int
	{
		int count = 0;
		for (const auto& val : vec)
		{
			if (val != "") count++;
		}
		return count;
	};

	void paint(Graphics& g) override 
	{
		// Decide on how many text labels we are going to use.
		// do not worry it is never going to be a square.
		float bigger_dim_val = std::max<int>(getWidth(), getHeight());
		float smaller_dim_val = std::min<int>(getWidth(), getHeight());
	
		int index = 0;

		g.setColour(Colours::lightgrey);

		// Draw the text strings at a constant seperation between them.
		// The spacing between the center b/w each text bounds should be the same.
		Font custom_font(CustomFont);
		g.setFont(custom_font);
		g.setFont(textHeightFraction * smaller_dim_val);

		if (label_sets.size() > 1)
		{
			if (getHeight() > getWidth())
			{
				for (int i = 1; i < label_sets.size(); ++i)
				{
					const auto& label_vec = label_sets[i];
					if (num_non_empty_strs(label_vec) * TextWidthPixels < bigger_dim_val)
					{
						index = i;
					}
					else 
					{
						break;
					}
				}
			}
			else
			{
				for (int i = 1; i < label_sets.size(); ++i)
				{
					const auto& label_vec = label_sets[i];
					String result = "";
					for (String str : label_vec) {
						result += str;
					}

					if (g.getCurrentFont().getStringWidthFloat(result) * 1.7 < bigger_dim_val)
					{
						index = i;
					}
					else
					{
						break;
					}
				}
			}
		}

		auto& labels = label_sets[index];
	
		if (labels.size() == 0) return;

		//auto bounds = g.getClipBounds();

		if (getHeight() > getWidth())
		{
			float fontHeight = textHeightFraction * smaller_dim_val;
			float totalLabelHeight = fontHeight * labels.size();
			float availableSpace = (float)getHeight() - totalLabelHeight;
			float separation = labels.size() > 1 ? availableSpace / (labels.size() - 1) : 0.0f;

			float x = 3.0f;
			float y = 0.0f;

			for (int label_index = 0; label_index < labels.size(); ++label_index)
			{
				Rectangle<float> textBounds(
					x,
					y,
					(float)getWidth() - x,
					fontHeight
				);

				g.drawText(labels[label_index], textBounds, Justification::centredLeft);

				y += fontHeight + separation;
			}
		}
		else 
		{
			float fontHeight = textHeightFraction * smaller_dim_val;

			String result = "";
			for (String str : labels) {
				result += str;
			}

			float totalLabelWidth = g.getCurrentFont().getStringWidthFloat(result);
			float availableSpace = (float)getWidth() - totalLabelWidth;

			float separation = labels.size() > 1 ? availableSpace / (labels.size() - 1) : 0.0f;

			float x = 0.0f;
			float y = 0.0f;

			for (int label_index = 0; label_index < labels.size(); ++label_index)
			{
				float fontWidth = g.getCurrentFont().getStringWidthFloat(labels[label_index]);
				Rectangle<float> textBounds(
					x,
					y,
					fontWidth,
					getHeight()
				);

				g.drawText(labels[label_index], textBounds, Justification::left);

				x += fontWidth + separation;
			}
		}
	}
	
	void resized() override
	{
	}


private:
	Typeface::Ptr CustomFont;

	float textHeightFraction = 0.55;
	std::vector<std::vector<String>> label_sets;

	// This is required as it will be used to calculate 
	// how many labels can we fit, and how many to hide.
	int TextWidthPixels = 20;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SeperatorBarLabeler)
};