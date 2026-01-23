#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_opengl/juce_opengl.h>

#include "../../ColourMaps.h"
#include "../../ds/dataStructure.h"

using namespace juce;

#define AMPLITUDE_DATA_SIZE 8192
#define REFRESH_RATE 60

class SpectrumAnalyserComponent
    : public Component,
      public OpenGLRenderer,
      public Timer
{
public:
    SpectrumAnalyserComponent(
        AudioProcessorValueTreeState& apvts_reference,
        std::function<void(string)>& label_callback);
    ~SpectrumAnalyserComponent();

    void prepareToPlay(float SampleRate, float BlockSize);
    void clearData();
    
    void timerCallback() override;

    // Call this with FFT bin data. num_bins should be (fft_size / 2) + 1
    void newData(float* data, int num_bins);

    void newOpenGLContextCreated() override;
    void renderOpenGL() override;
    void openGLContextClosing() override;

    void paint(Graphics& g) override {}
    void resized() override {
        if (send_triggerRepaint) opengl_context.triggerRepaint();
    }

    OpenGLContext opengl_context;

private:
    AudioProcessorValueTreeState& apvts_ref;
    std::atomic<float> SR = 44100.0f;
    std::atomic<bool> pause = false;
    std::atomic<bool> send_triggerRepaint = false;
    std::atomic<bool> newDataAvailable = false;
    std::atomic<int> bins_number = 257;

    // Amplitude data: direct FFT bin data
    GLfloat amplitude_data[AMPLITUDE_DATA_SIZE];
    
    // Ribbon data: smoothed/onion-skin version (background bars)
    GLfloat ribbon_data[AMPLITUDE_DATA_SIZE];

    // Powers of 2 for FFT sizes
    int po2[5] = { 512, 1024, 2048, 4096, 8192 };

    void createShaders();

    struct Uniforms
    {
        Uniforms(OpenGLContext& OpenGL_Context, OpenGLShaderProgram& shader_program);

        std::unique_ptr<OpenGLShaderProgram::Uniform>
            resolution,
            amplitudeData,
            ribbonData,
            colorMap_lower,
            colorMap_higher,
            numBars,
            numBins,
            sampleRate,
            minFreq,
            maxFreq;

    private:
        static OpenGLShaderProgram::Uniform* createUniform(
            OpenGLContext& gl_context,
            OpenGLShaderProgram& shader_program,
            const char* uniform_name);
    };

    GLuint dataTexture, ribbonTexture;
    GLuint VBO, EBO;

    std::unique_ptr<OpenGLShaderProgram> shader;
    std::unique_ptr<Uniforms> shader_uniforms;

    const char* vertexShader = R"(
        attribute vec2 position;
        void main() {
            gl_Position = vec4(position, 0.0, 1.0);
        }
    )";

   const char* fragmentShader = R"(
        uniform vec2 resolution;
        uniform sampler1D amplitudeData;
        uniform sampler1D ribbonData;
        uniform vec3 colorMap_lower;
        uniform vec3 colorMap_higher;

        uniform int numBars;
        uniform int numBins;

        uniform float sampleRate;
        uniform float minFreq;
        uniform float maxFreq;

        float freqFromNorm(float t)
        {
            return minFreq * pow(maxFreq / minFreq, t);
        }

        int binFromFreq(float freq)
        {
            float fftSize = float(2 * (numBins - 1));
            return int(freq * fftSize / sampleRate);
        }

        float fetch1DLinear(sampler1D tex, float binF)
        {
            binF = clamp(binF, 0.0, float(numBins - 1));
            int b0 = int(floor(binF));
            int b1 = min(b0 + 1, numBins - 1);
            float t = fract(binF);

            float v0 = texelFetch(tex, b0, 0).r;
            float v1 = texelFetch(tex, b1, 0).r;
            return mix(v0, v1, t);
        }

        void main()
        {
            float separatorThreshold = 0.3;
            vec2 uv = gl_FragCoord.xy / resolution.xy;

            float barHeight = 1.0 / float(numBars);
            int barIndex = int(floor(uv.y / barHeight));

            if (barIndex < 0 || barIndex >= numBars)
                discard;

            float pixelsPerBar = resolution.y / float(numBars);

            // Only draw separators if bars are tall enough
            if (pixelsPerBar >= (1.0 / separatorThreshold))
            {
                float localY = fract(uv.y / barHeight);
                float separatorUV = 1.0 / resolution.y;

                if (localY < separatorUV / barHeight)
                {
                    gl_FragColor = vec4(0.0, 0.0, 0.0, 1.0);
                    return;
                }
            }

            // ---- IMPORTANT: integrate the full bar band (matches spectrogram axis) ----
            float t0 = float(barIndex)     / float(numBars);
            float t1 = float(barIndex + 1) / float(numBars);

            float f0 = freqFromNorm(t0);
            float f1 = freqFromNorm(t1);

            // clamp to visible range
            f0 = clamp(f0, minFreq, maxFreq);
            f1 = clamp(f1, minFreq, maxFreq);

            if (f1 <= f0)
                discard;

            float fftSize = float(2 * (numBins - 1));

            float binF0 = clamp(f0 * fftSize / sampleRate, 0.0, float(numBins - 1));
            float binF1 = clamp(f1 * fftSize / sampleRate, 0.0, float(numBins - 1));

            if (binF1 <= binF0)
                binF1 = min(binF0 + 1.0, float(numBins - 1));

            // sample a few points inside the band and take MAX (better for peaks)
            // this avoids losing high frequency content
            const int S = 6;
            float amp = 0.0;
            float ribbon = 0.0;

            for (int s = 0; s < S; ++s)
            {
                float u = (float(s) + 0.5) / float(S);
                float bf = mix(binF0, binF1, u);

                amp    = max(amp,    fetch1DLinear(amplitudeData, bf));
                ribbon = max(ribbon, fetch1DLinear(ribbonData, bf));
            }

            float bar     = step(uv.x, amp);
            float ribbonB = step(uv.x, ribbon) * 0.4;

            vec3 colour = mix(colorMap_lower, colorMap_higher, amp);

            // your vertical grid lines
            if (fract(uv.x * 7.0) > 0.98)
                colour *= 0.0;

            gl_FragColor = vec4(colour * (bar + ribbonB), 1.0);
        }
        )";

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SpectrumAnalyserComponent)
};