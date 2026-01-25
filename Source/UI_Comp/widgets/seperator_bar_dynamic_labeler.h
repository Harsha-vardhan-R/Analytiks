#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>

using namespace juce;

class LogSeperatorBarLabeler
    : public Component,
      private AudioProcessorValueTreeState::Listener
{
public:
    LogSeperatorBarLabeler(AudioProcessorValueTreeState& apvtsRef,
                           Typeface::Ptr font_typeface)
        : apvts(apvtsRef), CustomFont(font_typeface)
    {
        setOpaque(false);

        apvts.addParameterListener("sp_rng_min", this);
        apvts.addParameterListener("sp_rng_max", this);

        // init from current values
        if (auto* pMin = apvts.getRawParameterValue("sp_rng_min"))
            minFreq.store(pMin->load());

        if (auto* pMax = apvts.getRawParameterValue("sp_rng_max"))
            maxFreq.store(pMax->load());
    }

    ~LogSeperatorBarLabeler() override
    {
        apvts.removeParameterListener("sp_rng_min", this);
        apvts.removeParameterListener("sp_rng_max", this);
    }

    void paint(Graphics& g) override
    {
        const int w = getWidth();
        const int h = getHeight();
        if (w <= 0 || h <= 0)
            return;

        float fMin = minFreq.load();
        float fMax = maxFreq.load();

        if (!std::isfinite(fMin) || !std::isfinite(fMax))
            return;

        fMin = jlimit(1.0f, 20000.0f, fMin);
        fMax = jlimit(fMin + 1.0f, 20000.0f, fMax);

        Font custom_font(CustomFont);
        float fnt_height = textHeightFraction * (float)std::min(w, h);
        g.setFont(custom_font.withHeight(fnt_height));

        const float minLabelGapPx = jmax(10.0f, fnt_height * 1.2f);

        auto ticks = generateLogTicks(fMin, fMax, (float)h, minLabelGapPx);

        for (auto& t : ticks)
        {
            const float freq = t.freq;
            const float y = freqToY(freq, fMin, fMax, (float)h);

            g.setColour(Colours::white.withAlpha(0.05f));
            g.drawHorizontalLine(roundToInt(y), 0.0f, (float)w);

            g.setColour(t.major ? Colours::white : Colours::lightgrey);

			int yyy = roundToInt(y - (float)TextWidthPixels * 0.5f);
			
			if (yyy < 0)
				yyy = 0;
			else if (yyy + TextWidthPixels > h)
				yyy = h - TextWidthPixels;
			
            Rectangle<int> bounds(
                0,
                yyy,
                w,
                TextWidthPixels
            );

            g.drawText(formatFrequency(freq), bounds, Justification::centred);
        }
    }

    void resized() override {}

private:
    struct Tick
    {
        float freq = 0.0f;
        bool major = false;
    };

    AudioProcessorValueTreeState& apvts;
    Typeface::Ptr CustomFont;

    std::atomic<float> minFreq { 10.0f };
    std::atomic<float> maxFreq { 20000.0f };

    float textHeightFraction = 0.55f;
    int TextWidthPixels = 15;

private:
    void parameterChanged(const String& parameterID, float newValue) override
    {
        if (parameterID == "sp_rng_min")
            minFreq.store(newValue);
        else if (parameterID == "sp_rng_max")
            maxFreq.store(newValue);

        MessageManager::callAsync([safe = Component::SafePointer<LogSeperatorBarLabeler>(this)]()
        {
            if (safe != nullptr)
                safe->repaint();
        });
    }

    static float safeLog(float x)
    {
        return std::log10(jmax(1e-6f, x));
    }

    static float freqToY(float freq, float fMin, float fMax, float height)
    {
        const float a = safeLog(fMin);
        const float b = safeLog(fMax);
        const float v = safeLog(freq);

        const float t = jmap(v, a, b, 0.0f, 1.0f);
        return (1.0f - t) * height;
    }

    static String formatFrequency(float f)
    {
        if (f >= 1000.0f)
        {
            float k = f / 1000.0f;
            if (k < 10.0f)
                return String(k, 1) + "k";
            return String((int)std::round(k)) + "k";
        }
        return String((int)std::round(f));
    }

    static std::vector<Tick> generateLogTicks(float fMin, float fMax, float height, float minGapPx)
    {
        std::vector<Tick> out;

        const float logMin = safeLog(fMin);
        const float logMax = safeLog(fMax);

        const float decades = jmax(1e-3f, logMax - logMin);
        const float pxPerDecade = height / decades;

        std::vector<float> bases = { 1.0f, 2.0f, 5.0f, 7.0 };

        if (pxPerDecade > 220.0f) bases = { 1,2,3,4,5,6,7,8,9 };
        else if (pxPerDecade > 120.0f) bases = { 1,2,3,5,7 };
        else bases = { 1,2,5 };
        int nMin = (int)std::floor(logMin);
        int nMax = (int)std::ceil(logMax);
 
        float lastY = -1e9f;

        for (int n = nMin; n <= nMax; ++n)
        {
            const float decade = std::pow(10.0f, (float)n);

            for (float b : bases)
            {
                float f = b * decade;
                if (f < fMin || f > fMax)
                    continue;

                float y = freqToY(f, fMin, fMax, height);

                if (std::abs(y - lastY) < minGapPx)
                    continue;

                lastY = y;

                Tick t;
                t.freq = f;
                t.major = (std::abs(b - 1.0f) < 1e-6f);

                out.push_back(t);
            }
        }

        auto forceTick = [&](float f, bool major)
        {
            Tick t { f, major };
            out.push_back(t);
        };

        forceTick(fMin, true);
        forceTick(fMax, true);

        std::sort(out.begin(), out.end(), [](const Tick& a, const Tick& b)
        {
            return a.freq < b.freq;
        });

        out.erase(std::unique(out.begin(), out.end(), [](const Tick& a, const Tick& b)
        {
            return std::abs(a.freq - b.freq) < 1e-3f;
        }), out.end());

        return out;
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LogSeperatorBarLabeler)
};
