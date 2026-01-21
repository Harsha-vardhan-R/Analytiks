// The Spectrogram.

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_opengl/juce_opengl.h>

#include "../../ds/dataStructure.h"

#include "../../colourmaps.h"

using namespace juce;

#define SPECTROGRAM_FPS 60
#define SPECTROGRAM_MAX_WIDTH 2000
// This is required because we can zoom into the spectrogram, by giving the min_freq and max_freq.
// so we cannot decimate even though the visible pixels might alwas be less than this.
#define SPECTROGRAM_FFT_BINS_MAX 4097

class SpectrogramComponent 
    : public Component,
      public OpenGLRenderer,
      private Timer,
      private AudioProcessorValueTreeState::Listener
{
public:

    SpectrogramComponent(AudioProcessorValueTreeState& apvts_reference);

    void timerCallback() override;
    void newDataBatch(std::array<std::vector<float>, 32>& data, int valid, int numBins, float bpm, float sample_rate, int N, int D);

    void parameterChanged(const String& parameterID, float newValue) override;

    // Zero out all spectrogram data.
    void clearData();

    void newOpenGLContextCreated() override;
    void renderOpenGL() override;
    void openGLContextClosing() override;

    void paint(Graphics& g) override {};
    void resized() override;

private:
    //linkDS& linker_ref;
    AudioProcessorValueTreeState& apvts_ref;

    std::atomic<bool> new_data_flag = false;
    std::atomic<bool> trigger_repaint = false;

    std::atomic<int> numValidBins = 512;
    OpenGLContext opengl_context;

    // Write index says where the next incoming data batch should be written to.
    std::atomic<int> writeIndex = 0;
    // How many columns of data are currently stored in the spectrogram.
    std::atomic<int> validColumnsInData = 0;
    // number of fft frames accumulated so far for the current column.
    // used to decide when to switch to a new column.
    float accumulator = 0.0f;

    float SR = 44100.0f;

    float spectrogram_data[SPECTROGRAM_FFT_BINS_MAX][SPECTROGRAM_MAX_WIDTH] = { 0.0f };

    void createShaders();

    struct Uniforms
    {
        Uniforms(OpenGLContext& OpenGL_Context, OpenGLShaderProgram& shader_program);

        std::unique_ptr<OpenGLShaderProgram::Uniform>
            resolution,
            imageData,
            colorMap_lower,
            colorMap_higher,
            numBins,
            startIndex,
            numIndex,
            minFreq,
            maxFreq,
            sampleRate,
            scroll,
            bias,
            curve;

    private:
        static OpenGLShaderProgram::Uniform* createUniform(
            OpenGLContext& gl_context,
            OpenGLShaderProgram& shader_program,
            const char* uniform_name);
    };

    GLuint dataTexture = 0;
    GLuint VBO = 0, EBO = 0;

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
        uniform sampler2D imageData;

        uniform vec3 colorMap_lower;
        uniform vec3 colorMap_higher;

        uniform int numBins;
        uniform int startIndex;
        uniform int numIndex;

        uniform float minFreq;
        uniform float maxFreq;
        uniform float sampleRate;

        uniform float bias;
        uniform float curve;

        uniform int scroll;

        float freqFromNorm(float t)
        {
            return minFreq * pow(maxFreq / minFreq, t);
        }

        int binFromFreq(float freq)
        {
            float fftSize = float(2 * (numBins - 1));
            return int(freq * fftSize / sampleRate);
        }

        float sCurve(float x, float curve)
        {
            float strength = mix(0.0, 6.0, abs(curve));
            float y = x * 2.0 - 1.0;
            float s = tanh(y * strength);
            s = s * 0.5 + 0.5;
            if (curve < 0.0)
                s = 1.0 - s;
            return mix(x, s, abs(curve));
        }

        void main()
        {
            vec2 uv = gl_FragCoord.xy / resolution.xy;

            float colF = uv.x * float(numIndex);
            int col0 = int(floor(colF));
            int col1 = min(col0 + 1, numIndex - 1);

            if (scroll == 1)
            {
                // ring-buffer scrolling
                col0 = (startIndex + col0) % numIndex;
                col1 = (startIndex + col1) % numIndex;
            }
            else
            {
                // grow-from-left (no wrap)
                col0 = clamp(col0, 0, numIndex - 1);
                col1 = clamp(col1, 0, numIndex - 1);

                // nothing written yet on the right side
                if (col0 >= startIndex)
                    discard;
            }

            float pixelY = 1.0 / resolution.y;

            float t0 = uv.y;
            float t1 = min(1.0, uv.y + pixelY);

            float f0 = freqFromNorm(t0);
            float f1 = freqFromNorm(t1);

            if (f1 < minFreq || f0 > maxFreq)
                discard;

            int visBinStart = binFromFreq(minFreq);
            int visBinEnd   = binFromFreq(maxFreq);

            int binStart = clamp(binFromFreq(f0), visBinStart, visBinEnd);
            int binEnd   = clamp(binFromFreq(f1), binStart + 1, visBinEnd);

            binStart = clamp(binStart, 0, numBins - 1);
            binEnd   = clamp(binEnd,   0, numBins);

            float value = 0.0;

            for (int b = binStart; b < binEnd; ++b)
            {
                float v0 = texelFetch(imageData, ivec2(col0, b), 0).r;
                float v1 = texelFetch(imageData, ivec2(col1, b), 0).r;
                value = max(value, max(v0, v1));
            }

            float curvedValue = sCurve(value, curve);
            vec3 colour = mix(colorMap_lower, colorMap_higher, curvedValue);

            if (curvedValue < bias)
                colour *= 0.01;

            gl_FragColor = vec4(colour, 1.0);
        }
    )";

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SpectrogramComponent)
};