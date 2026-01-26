#pragma once

#include <juce_core/juce_core.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_opengl/juce_opengl.h>

#include "../../colourmaps.h"
#include "../util.h"

using namespace juce;

#define OSC_FPS 60
#define OSC_MAX_WIDTH 1500

class OscilloscopeComponent
    : public Component,
      public OpenGLRenderer,
      private Timer,
      private AudioProcessorValueTreeState::Listener
{
public:
    OscilloscopeComponent(AudioProcessorValueTreeState& apvts_reference);
    ~OscilloscopeComponent() override;

    void newAudioBatch(const float* left, const float* right, int numSamples,
                       float bpm, float sample_rate, int N);

    void timerCallback() override;
    void parameterChanged(const String& parameterID, float newValue) override;

    void clearData();

    void newOpenGLContextCreated() override;
    void renderOpenGL() override;
    void openGLContextClosing() override;

    void paint(Graphics&) override {}
    void resized() override;

private:
    AudioProcessorValueTreeState& apvts_ref;

    std::atomic<bool> new_data_flag { false };
    std::atomic<bool> trigger_repaint { false };
    std::atomic<bool> colourmap_dirty { true };

    OpenGLContext opengl_context;

    std::atomic<int> writeIndex { 0 };
    std::atomic<int> validColumnsInData { OSC_MAX_WIDTH };

    int getDelayColumns() const;

    float accumulator = 0.0f;
    std::atomic<float> SR { 44100.0f };

    float oscMin[2][OSC_MAX_WIDTH] = {};
    float oscMax[2][OSC_MAX_WIDTH] = {};

    float oscSample[2][OSC_MAX_WIDTH] = {};
    std::atomic<int> renderMode { 0 };

    void createShaders();
    void uploadColourMap();

    struct Uniforms
    {
        Uniforms(OpenGLContext& ctx, OpenGLShaderProgram& prog);

        std::unique_ptr<OpenGLShaderProgram::Uniform>
            resolution,
            imageData,
            colourMapTex,
            startIndex,
            numIndex,
            validColumns,
            scroll,
            mode,
            thickness,
            delayColumns,
            renderMode;

        static OpenGLShaderProgram::Uniform* createUniform(OpenGLContext& gl_context,
                                                           OpenGLShaderProgram& shader_program,
                                                           const char* uniform_name);
    };

    std::unique_ptr<OpenGLShaderProgram> shader;
    std::unique_ptr<Uniforms> shader_uniforms;

    GLuint dataTexture = 0;
    GLuint colourMapTexture = 0;

    GLuint VAO = 0;
    GLuint VBO = 0;
    GLuint EBO = 0;

    const char* vertexShader = R"(
        attribute vec2 position;
        void main()
        {
            gl_Position = vec4(position, 0.0, 1.0);
        }
    )";

    const char* fragmentShader = R"(
        uniform vec2 resolution;
        uniform sampler2D imageData;
        uniform sampler1D colourMapTex;

        uniform int startIndex;
        uniform int numIndex;
        uniform int validColumns;

        uniform int scroll;
        uniform int mode;
        uniform float thickness;
        uniform int renderMode;

        vec3 fetchMinMaxS(int c, int chan)
        {
            return texelFetch(imageData, ivec2(c, chan), 0).rgb;
        }

        float drawSegment(float y0, float y1, float y, float th)
        {
            float lo = min(y0, y1);
            float hi = max(y0, y1);
            float inside = smoothstep(lo - th, lo + th, y) * (1.0 - smoothstep(hi - th, hi + th, y));
            float edge0  = smoothstep(th, 0.0, abs(y - lo));
            float edge1  = smoothstep(th, 0.0, abs(y - hi));
            return max(inside, max(edge0, edge1));
        }

        void main()
        {
            vec2 uv = gl_FragCoord.xy / resolution.xy;

            int col = int(floor(uv.x * float(numIndex)));
            col = clamp(col, 0, numIndex - 1);
            if (scroll == 1)
                col = (startIndex + col) % numIndex;

            vec3 bg = vec3(0.0);
            float gridY = abs(fract(uv.y * 8.0) - 0.5);
            bg += vec3(0.1 * step(gridY, 0.01));

            float th = (thickness + 1.0) / resolution.y;
            float y  = uv.y;

            vec3 outCol = bg;
            const float flatlineAmpThreshold = 0.01;

            bool showBoth = (mode == 0);

            if (showBoth)
            {
                bool top = (y > 0.5);
                float regionMin = top ? 0.5 : 0.0;
                float regionMax = top ? 1.0 : 0.5;

                float regionH = regionMax - regionMin;
                float bandH   = regionH * 0.80;
                float bandMin = regionMin + (regionH - bandH) * 0.5;
                float bandMax = bandMin + bandH;

                if (y < bandMin || y > bandMax)
                {
                    gl_FragColor = vec4(outCol, 1.0);
                    return;
                }

                float yLocal = (y - bandMin) / (bandMax - bandMin);
                int chan = top ? 0 : 1;

                vec3 mms = fetchMinMaxS(col, chan);
                float mn = mms.r, mx = mms.g, s = mms.b;
                if (mn > mx) { mn = 0.0; mx = 0.0; s = 0.0; }

                float intensity = 0.0;
                float amp = 0.0;

                if (renderMode == 0)
                {
                    amp = max(abs(mn), abs(mx));
                    float y0 = 0.5, y1 = 0.5;
                    if (amp >= flatlineAmpThreshold) { y0 = 0.5 + 0.5 * mn; y1 = 0.5 + 0.5 * mx; }
                    else amp = flatlineAmpThreshold;
                    intensity = drawSegment(y0, y1, yLocal, th);
                }
                else
                {
                    int col2 = (col + 1) % numIndex;
                    float s2 = fetchMinMaxS(col2, chan).b;

                    float y0 = 0.5 + 0.5 * s;
                    float y1 = 0.5 + 0.5 * s2;

                    amp = max(abs(s), abs(s2));
                    if (amp < flatlineAmpThreshold) amp = flatlineAmpThreshold;

                    intensity = drawSegment(y0, y1, yLocal, th);
                }

                vec3 colour = texture(colourMapTex, clamp(amp, 0.0, 1.0)).rgb;
                outCol += colour * intensity;
            }
            else
            {
                float bandH   = 0.80;
                float bandMin = (1.0 - bandH) * 0.5;
                float bandMax = bandMin + bandH;

                if (y < bandMin || y > bandMax)
                {
                    gl_FragColor = vec4(outCol, 1.0);
                    return;
                }

                float yLocal = (y - bandMin) / (bandMax - bandMin);

                vec3 mms = fetchMinMaxS(col, 0);
                float mn = mms.r, mx = mms.g, s = mms.b;
                if (mn > mx) { mn = 0.0; mx = 0.0; s = 0.0; }

                float intensity = 0.0;
                float amp = 0.0;

                if (renderMode == 0)
                {
                    amp = max(abs(mn), abs(mx));
                    float y0 = 0.5, y1 = 0.5;
                    if (amp >= flatlineAmpThreshold) { y0 = 0.5 + 0.5 * mn; y1 = 0.5 + 0.5 * mx; }
                    else amp = flatlineAmpThreshold;
                    intensity = drawSegment(y0, y1, yLocal, th);
                }
                else
                {
                    int col2 = (col + 1) % numIndex;
                    float s2 = fetchMinMaxS(col2, 0).b;

                    float y0 = 0.5 + 0.5 * s;
                    float y1 = 0.5 + 0.5 * s2;

                    amp = max(abs(s), abs(s2));
                    if (amp < flatlineAmpThreshold) amp = flatlineAmpThreshold;

                    intensity = drawSegment(y0, y1, yLocal, th);
                }

                vec3 colour = texture(colourMapTex, clamp(amp, 0.0, 1.0)).rgb;
                outCol += colour * intensity;
            }

            gl_FragColor = vec4(outCol, 1.0);
        }
        )";


    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OscilloscopeComponent)
};
