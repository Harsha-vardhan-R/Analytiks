// The Phase correlation meter.

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_opengl/juce_opengl.h>
#include "../../colourMaps.h"

using namespace juce;

// x, y interleaved.
#define RING_BUFFER_SIZE 1024 
#define RING_BUFFER_TEXEL_SIZE 512
#define REFRESH_RATE_HZ 100

class PhaseCorrelationAnalyserComponent 
    :   public Component,
        public Timer, // to call the repaint method.
        public AudioProcessorParameter::Listener
{
public:

    PhaseCorrelationAnalyserComponent(
        AudioProcessorValueTreeState& apvts_reference
    );
    ~PhaseCorrelationAnalyserComponent();

    void parameterValueChanged (int parameterIndex, float newValue) override;
    void parameterGestureChanged (int , bool) override {};

    std::atomic<bool> dirty = true;

    void paint(Graphics& g) override;
    void resized() override;

    void timerCallback() override;

    void prepareToPlay(float sample_rate, float block_size);
    void zeroOutMeters();

    void processBlock(AudioBuffer<float>& buffer);
    void recalculateSums();

    const int TARGET_TRIGGER_HZ = 1200;
    std::atomic<float> rms_time = 18; // in ms.
    std::atomic<float> sample_rate = 44100.0;
    std::atomic<int> window_length_samples = sample_rate * rms_time;
    std::atomic<int> update_window_samples = std::max<int>(sample_rate / (float)TARGET_TRIGGER_HZ, 2);
    int sample_counter = 0;

    // used to compute both the rms volume and the cosine similarity.
    // a continuous sliding window is used based on the 
    // window_length_samples and update_window_samples.
    float sumLR = 0, sumLL = 0, sumRR = 0;
    std::queue<float> SquareQueLR, SquareQueLL, SquareQueRR;

    class CorrelationOpenGLComponent 
        : public Component,
          public OpenGLRenderer,
          public AudioProcessorParameter::Listener
    {
    public :
        CorrelationOpenGLComponent();
        ~CorrelationOpenGLComponent();

        void parameterValueChanged (int parameterIndex, float newValue);
        void parameterGestureChanged (int , bool) {};

        // called from the `PhaseCorrelationAnalyserComponent`,
        // triggers repaint for the open gl component.
        void newDataPoint(float x, float y);

        // ================================================
        void newOpenGLContextCreated() override;
        void renderOpenGL() override;
        void openGLContextClosing() override;

        // ================================================
        void paint(Graphics& g) override {};
        void resized() override {
            if (send_triggerRepaint) opengl_context.triggerRepaint();
        };

        OpenGLContext opengl_context;

        float colours[6] = {
            0.0f, 0.0f, 1.0f,
            1.0f, 0.2f, 0.2f
        };

    private:

        std::mutex ringBufferMutex;
        GLfloat ring_buffer[RING_BUFFER_SIZE];
        std::atomic<int> ring_buf_read_index = 0;
        std::atomic<bool> dirty = false;

        const char* vertexShader =
        R"(
            attribute vec2 position;
                            
            void main() {
                gl_Position = vec4(position, 0.0, 1.0);
            }
        )";

        const char* fragmentShader = R"(
        uniform vec2 resolution;
        uniform sampler1D pointsTex;
        uniform int readIndex;
        uniform int maxIndex;
        uniform vec3 col1;
        uniform vec3 col2;

        // Returns 1.0 if the fragment is within the line width, 0.0 otherwise
        float drawLine(vec2 p1, vec2 p2, vec2 uv, float thicknessPx)
        {
            float halfThickness = 0.5 * thicknessPx;
            float one_px = halfThickness / resolution.x;

            float d = distance(p1, p2);
            float t = clamp(dot(uv - p1, p2 - p1) / (d * d), 0.0, 1.0);
            vec2 closest = mix(p1, p2, t);

            float dist = distance(uv, closest);

            return smoothstep(one_px, 0.0, dist);
        }

        float drawPoint(vec2 p1, vec2 uv, float thicknessPx)
        {
            float one_px = thicknessPx / resolution.x;
            float d = distance(p1, uv);
            return float(d < one_px);
        }

        vec2 fetchPoint(int index) {
            return texelFetch(pointsTex, index, 0).rg;
        }

        void main() {
            vec2 uv = gl_FragCoord.xy / resolution.xy;

            vec2 p1 = vec2(0.25, 0.75);
            vec2 p2 = vec2(0.25, 0.25);
            vec2 p3 = vec2(0.75, 0.25);
            vec2 p4 = vec2(0.75, 0.75);

            float thicknessPx = 3.0;

            float lines =
                drawLine(vec2(0.0, 0.5), vec2(0.5, 0.0), uv, thicknessPx) +
                drawLine(vec2(0.0, 0.5), vec2(0.5, 1.0), uv, thicknessPx) +
                drawLine(vec2(1.0, 0.5), vec2(0.5, 0.0), uv, thicknessPx) +
                drawLine(vec2(1.0, 0.5), vec2(0.5, 1.0), uv, thicknessPx) +

                drawLine(p1, p3, uv, thicknessPx) +
                drawLine(p2, p4, uv, thicknessPx);

            // colour of the pixel before the path.
            float highlight = 0.3 * lines;
            float path = 0.0;                

            vec3 path_colour = vec3(0.0);
            float total_weight = 0.0;

            // now the path.
            for (int i = 0; i < maxIndex; ++i) {
                int i0 = (readIndex + i) % maxIndex;
                int i1 = (readIndex + i + 1) % maxIndex;

                vec2 p0 = fetchPoint(i0);
                vec2 p1 = fetchPoint(i1);

                // stuff is reversed because that is how the points are written.
                float t = float(i) / float(maxIndex - 1);
                float fade = pow(t, 2.0);

                float segment = drawLine(p0, p1, uv, thicknessPx) * fade;

                vec3 segment_colour = mix(col1, col2, t);

                path_colour += segment_colour * segment;
                total_weight += segment;
            }

            // Normalize so overlapping segments don't brighten path
            vec3 highlight_colour = vec3(highlight);

            gl_FragColor = vec4( highlight + path_colour, 1.0);
        })";

        struct Uniforms
        {
        public:

            Uniforms(
                OpenGLContext& OpenGL_Context,
                OpenGLShaderProgram& shader_program);

            std::unique_ptr<OpenGLShaderProgram::Uniform> 
                readIndex,
                resolution,
                pointsTex,
                maxIndex,
                col1,
                col2;

        private:

            static OpenGLShaderProgram::Uniform* createUniform(
                OpenGLContext& gl_context,
                OpenGLShaderProgram& shader_program,
                const char* uniform_name
            );

        };

        GLuint dataTexture;
        GLuint VBO, EBO;

        std::unique_ptr<OpenGLShaderProgram> shader;
        std::unique_ptr<Uniforms> shader_uniforms;

        // set to true in the newOpenGLContextCreated,
        // so that we do not trigger repaint before the shaders are done.
        std::atomic<bool> send_triggerRepaint = false;

        void createShaders();

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CorrelationOpenGLComponent)
    };

    // the bar thing below the graph that shows a smoothed 
    // version of the amount of similarity between the signals.

    class ValueMeterComponent
        :   public Component
    {
    public:
        ValueMeterComponent(String low_value_str, String high_value_str);

        void paint(Graphics& g) override;
        void resized() override;

        void newPoint(float y_val);

        juce::Colour accent_colour = juce::Colours::red;
    private:

        String low_val, high_val;

        // smoothing while updating values.
        const float alpha = 0.1;
        std::atomic<float> pres_value = 1.0;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ValueMeterComponent)
    };

    class VolumeMeterComponent
        : public Component
    {
    public:
        VolumeMeterComponent();

        void paint(Graphics& g) override;
        void resized() override;

        void newPoint(float l_vol, float r_vol);

        void zeroOut();

        juce::Colour accent_colour = juce::Colours::red;
    private:

        // smoothing while updating values.
        const float alpha = 0.001;
        std::atomic<float> l_rms_vol = 0.0;
        std::atomic<float> r_rms_vol = 0.0;

        float l_rms_vol_smoothed = 0.0;
        float r_rms_vol_smoothed = 0.0;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VolumeMeterComponent)
    };

private:
    AudioProcessorValueTreeState& apvts_ref;

    CorrelationOpenGLComponent opengl_comp;

    ValueMeterComponent correl_amnt_comp;
    ValueMeterComponent balance_amnt_comp;

    VolumeMeterComponent volume_meter_comp;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PhaseCorrelationAnalyserComponent)
};