// The Phase correlation meter.

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_opengl/juce_opengl.h>

using namespace juce;

// x, y interleaved.
#define RING_BUFFER_SIZE 256 
#define RING_BUFFER_TEXEL_SIZE 128

class PhaseCorrelationAnalyserComponent 
    : public Component
{
public:

    PhaseCorrelationAnalyserComponent(
        AudioProcessorValueTreeState& apvts_reference
    );
    ~PhaseCorrelationAnalyserComponent();

    void paint(Graphics& g) override;
    void resized() override;

    void prepareToPlay(float sample_rate, float block_size);

    void processBlock(AudioBuffer<float>& buffer);

    // i.e for any sample rate.
    const int TARGET_TRIGGER_HZ = 120;
    int window_length_samples = std::floor(44100.0 / (float)TARGET_TRIGGER_HZ);
    int sample_counter = 0;

    // used to compute both the volume and the cosine similarity.
    float sumLR, sumLL, sumRR;


    class CorrelationOpenGLComponent 
        : public Component,
          public OpenGLRenderer
    {
    public :
        CorrelationOpenGLComponent();
        ~CorrelationOpenGLComponent();

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

    private:

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

                vec2 p1 = vec2(0.0, 0.5);
                vec2 p2 = vec2(0.5, 1.0);
                vec2 p3 = vec2(1.0, 0.5);
                vec2 p4 = vec2(0.5, 0.0);

                float thicknessPx = 3.0;

                float lines =
                    drawLine(p1, p2, uv, thicknessPx) +
                    drawLine(p2, p3, uv, thicknessPx) +
                    drawLine(p3, p4, uv, thicknessPx) +
                    drawLine(p4, p1, uv, thicknessPx) +
                    drawLine(p1, p3, uv, thicknessPx) +
                    drawLine(p2, p4, uv, thicknessPx);

                // colour of the pixel before the path.
                float highlight = 0.3 * lines;
                float path = 0.0;                

                // now the path.
                for (int i = 0; i < maxIndex; ++i) {
                    int i0 = (readIndex + i) % maxIndex;
                    int i1 = (readIndex + i + 1) % maxIndex;

                    vec2 p0 = fetchPoint(i0);
                    vec2 p1 = fetchPoint(i1);
                    
                    path += (float(i)/float(maxIndex)) * drawLine(p0, p1, uv, thicknessPx+1);
                }

                vec3 path_colour = vec3(path);
                vec3 highlight_colour = vec3(highlight);
                
                gl_FragColor = vec4( highlight + path_colour, 1.0);
            }
        )";

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
                maxIndex;

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

    class CorrelAmntComponent
        :   public Component,
            public Timer // to call the repaint method.
    {
    public:
        CorrelAmntComponent();

        void paint(Graphics& g) override;
        void resized() override;

        void timerCallback() override;

        void newPoint(float y_val);

        juce::Colour accent_colour = juce::Colours::red;
    private:

        const int refreshRate = 45;
        const float alpha = 0.8;
        std::atomic<float> correl_value = 1.0;


        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CorrelAmntComponent)
    };

private:
    AudioProcessorValueTreeState& apvts_ref;

    CorrelationOpenGLComponent opengl_comp;
    CorrelAmntComponent correl_amnt_comp;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PhaseCorrelationAnalyserComponent)
};