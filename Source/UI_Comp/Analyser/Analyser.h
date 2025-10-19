// The spectrum analyser.

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_opengl/juce_opengl.h>

#include "../../ColourMaps.h"

#include "../../ds/dataStructure.h"


using namespace juce;

#define AMPLITUDE_DATA_SIZE 8192

class SpectrumAnalyserComponent
    :   public Component,
        public OpenGLRenderer
{
public:
    SpectrumAnalyserComponent(
        AudioProcessorValueTreeState& apvts_reference, 
        linkDS& lnk_reference);
    ~SpectrumAnalyserComponent();

    void prepareToPlay(float SampleRate, float BlockSize);

    void clearData();

    // ================================================
    void newOpenGLContextCreated() override;
    void renderOpenGL() override;
    void openGLContextClosing() override;

    // set state to pause showing or continue.
    void setState(bool state);

    // ================================================
    void paint(Graphics& g) override {};
    void resized() override {
        if (send_triggerRepaint) opengl_context.triggerRepaint();
    };

    OpenGLContext opengl_context;

private:

    // powers of 2 based on the fft order.
    int po2[5] = {
        512,
        1024,
        2048,
        4096,
        8192
    };

    void createShaders();

    linkDS& linker_ref;
    atomic<bool> pause;

    AudioProcessorValueTreeState& apvts_ref;

    GLfloat amplitude_data[AMPLITUDE_DATA_SIZE];
    GLfloat ribbon_data[AMPLITUDE_DATA_SIZE];

    std::atomic<float> SR = 44100.0;
    
    struct Uniforms
    {
    public:

        Uniforms(
            OpenGLContext& OpenGL_Context,
            OpenGLShaderProgram& shader_program);

        std::unique_ptr<OpenGLShaderProgram::Uniform>
            resolution,
            amplitudeData, // sampler 1d
            ribbonData, // sampler 1d
            colorMap_lower, // vec3
            colorMap_higher, // vec3 
            rangeMin, // int
            rangeMax, // int
            numBars, // int
            colourmapGate, // float
            colourmapBias; // float

    private:

        static OpenGLShaderProgram::Uniform* createUniform(
            OpenGLContext& gl_context,
            OpenGLShaderProgram& shader_program,
            const char* uniform_name
        );

    };

    GLuint 
        dataTexture,
        ribbonTexture;

    GLuint VBO, EBO;

    std::unique_ptr<OpenGLShaderProgram> shader;
    std::unique_ptr<Uniforms> shader_uniforms;

    // set to true in the newOpenGLContextCreated,
    // so that we do not trigger repaint before the shaders are done.
    std::atomic<bool> send_triggerRepaint = false;

    const char* vertexShader =
    R"(

attribute vec2 position;
                
void main() {
    gl_Position = vec4(position, 0.0, 1.0);
}

)";

    const char* fragmentShader =
    R"(

uniform vec2 resolution;

uniform sampler1D amplitudeData;
uniform sampler1D ribbonData;

//uniform sampler1D colorMap;

uniform vec3 colorMap_lower;
uniform vec3 colorMap_higher;

// valid indexes can be calculated using range.
// range min and range max are the indexes which we need to show.
uniform int rangeMin;
uniform int rangeMax;
uniform int numBars;

uniform float colourmapBias;

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

// texture, position float b/w 0.0 and 1.0
vec3 fetchTexelAtPosition(sampler1D sampler_texture, float position) {
    return texture( sampler_texture, position).rgb;
}

float fetchPointAtIndex(sampler1D Texture, int index) {
    return texelFetch(Texture, index, 0).r;
}

void main() {
    vec2 uv = gl_FragCoord.xy / resolution.xy;

    float bar = floor(numBars * (uv.y * 0.999));
    float x_pos = uv.x;

    float bins_to_bars_fraction = float(rangeMax - rangeMin) / float(numBars);
    
    // index from which the averaging starts for this bar.
    int startIndex = rangeMin + int(floor(bins_to_bars_fraction * bar));
    int endIndex = rangeMin + int(floor(bins_to_bars_fraction * (bar + 1)));

    int num_bins = endIndex - startIndex;
    
    /*float onePixelY = 1.0 / resolution.y;

    bool isBorder = (uv.y - barStart < onePixelY) || (barEnd - uv.y < onePixelY);

    if (isBorder) {
        gl_FragColor = vec4(0.0, 0.0, 0.0, 1.0);
        return;
    }*/

    float sum_ = 0.0;

    for (int index = startIndex; index < endIndex; ++index)
    {
        sum_ += fetchPointAtIndex(amplitudeData, index);
    }
    
    float avg_ = sum_ / float(num_bins);
    
    // now we got the value.
    float pixel_on = float(x_pos < avg_);

    // get the colour.
    float transformed_colour_level = pow(avg_ , colourmapBias);

    vec3 colour;
    if (avg_ < colourmapBias) {
        colour = colorMap_lower;
    } else {
        float denom = 1.0 - colourmapBias;
        float numer = avg_ - colourmapBias;
        colour = mix(colorMap_lower, colorMap_higher, numer / denom);
    }
    
    gl_FragColor = vec4(colour * pixel_on, 1.0);
}

)";

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SpectrumAnalyserComponent)
};