// The Phase correlation meter.

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_opengl/juce_opengl.h>
#include "../../colourMaps.h"

using namespace juce;

// x, y interleaved.
#define RING_BUFFER_SIZE 4096
#define RING_BUFFER_TEXEL_SIZE 2048
#define REFRESH_RATE_HZ 60

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
            attribute vec3 color;
            varying vec3 vColor;
                            
            void main() {
                gl_Position = vec4(position * 2.0 - 1.0, 0.0, 1.0);
                vColor = color;
            }
        )";

        const char* fragmentShader = R"(
        varying vec3 vColor;

        void main() {
            gl_FragColor = vec4(vColor, 1.0);
        })";

        struct Uniforms
        {
        public:

            Uniforms(
                OpenGLContext& OpenGL_Context,
                OpenGLShaderProgram& shader_program);

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