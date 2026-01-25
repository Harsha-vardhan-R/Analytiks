#include "Oscilloscope.h"

OscilloscopeComponent::OscilloscopeComponent(AudioProcessorValueTreeState& apvts_reference)
    : apvts_ref(apvts_reference)
{
    setOpaque(true);

    apvts_ref.addParameterListener("gb_vw_mde", this);
    apvts_ref.addParameterListener("gb_chnl", this);
    apvts_ref.addParameterListener("gb_clrmap", this);

    clearData();

    opengl_context.setOpenGLVersionRequired(OpenGLContext::OpenGLVersion::openGL3_2);
    opengl_context.setRenderer(this);
    opengl_context.setComponentPaintingEnabled(false);
    opengl_context.setContinuousRepainting(false);
    opengl_context.attachTo(*this);

    startTimerHz(OSC_FPS);
    opengl_context.setSwapInterval(1);
}

OscilloscopeComponent::~OscilloscopeComponent()
{
    apvts_ref.removeParameterListener("gb_vw_mde", this);
    apvts_ref.removeParameterListener("gb_chnl", this);
    apvts_ref.removeParameterListener("gb_clrmap", this);

    opengl_context.detach();
}

void OscilloscopeComponent::timerCallback()
{
    if (trigger_repaint)
        opengl_context.triggerRepaint();
}

void OscilloscopeComponent::parameterChanged(const String& parameterID, float)
{
    new_data_flag = true;

    if (parameterID == "gb_clrmap")
        colourmap_dirty = true;
}

void OscilloscopeComponent::clearData()
{
    for (int c = 0; c < OSC_MAX_WIDTH; ++c)
    {
        oscMin[0][c] =  1.0f;
        oscMax[0][c] = -1.0f;
        oscMin[1][c] =  1.0f;
        oscMax[1][c] = -1.0f;
    }

    writeIndex.store(0);
    validColumnsInData.store(0);
    accumulator = 0.0f;

    new_data_flag = true;
}

void OscilloscopeComponent::newAudioBatch(const float* left, const float* right, int numSamples,
                                         float bpm, float sample_rate, int N)
{
    static const float measureTable[] = {
        1.0f / 4.0f,
        1.0f / 3.0f,
        1.0f / 7.0f,
        1.0f / 5.0f
    };

    float osc_measure  = apvts_ref.getRawParameterValue("sp_measure")->load();
    float osc_multiple = apvts_ref.getRawParameterValue("sp_multiple")->load();

    float barsPerWindow   = measureTable[(int) osc_measure] * osc_multiple;
    float secondsPerBar   = (60.0f / bpm) * N;
    float historySeconds  = barsPerWindow * secondsPerBar;

    float samples_per_column = (historySeconds * sample_rate) / (float) OSC_MAX_WIDTH;
    if (samples_per_column < 1.0f)
        samples_per_column = 1.0f;

    int mode = (int) apvts_ref.getRawParameterValue("gb_chnl")->load();

    for (int id = 0; id < numSamples; ++id)
    {
        float L = left  ? left[id]  : 0.0f;
        float R = right ? right[id] : 0.0f;

        L = jlimit(-1.0f, 1.0f, L);
        R = jlimit(-1.0f, 1.0f, R);

        int wi = writeIndex.load(std::memory_order_relaxed);

        if (mode == 0)
        {
            oscMin[0][wi] = std::min(oscMin[0][wi], L);
            oscMax[0][wi] = std::max(oscMax[0][wi], L);

            oscMin[1][wi] = std::min(oscMin[1][wi], R);
            oscMax[1][wi] = std::max(oscMax[1][wi], R);
        }
        else if (mode == 1)
        {
            oscMin[0][wi] = std::min(oscMin[0][wi], L);
            oscMax[0][wi] = std::max(oscMax[0][wi], L);
        }
        else if (mode == 2)
        {
            oscMin[0][wi] = std::min(oscMin[0][wi], R);
            oscMax[0][wi] = std::max(oscMax[0][wi], R);
        }
        else
        {
            float S = jlimit(-1.0f, 1.0f, L - R);
            oscMin[0][wi] = std::min(oscMin[0][wi], S);
            oscMax[0][wi] = std::max(oscMax[0][wi], S);
        }

        accumulator += 1.0f;

        if (accumulator >= samples_per_column)
        {
            accumulator -= samples_per_column;

            int next = (wi + 1) % OSC_MAX_WIDTH;

            oscMin[0][next] =  1.0f;
            oscMax[0][next] = -1.0f;
            oscMin[1][next] =  1.0f;
            oscMax[1][next] = -1.0f;

            writeIndex.store(next, std::memory_order_relaxed);

            int v = validColumnsInData.load(std::memory_order_relaxed);
            validColumnsInData.store(jmin(v + 1, OSC_MAX_WIDTH), std::memory_order_relaxed);
        }
    }

    SR.store(sample_rate, std::memory_order_relaxed);
    new_data_flag = true;
}

void OscilloscopeComponent::uploadColourMap()
{
    using namespace juce::gl;

    int code = (int) apvts_ref.getRawParameterValue("gb_clrmap")->load();
    const float* cmap = getBrightColourMapForCode(code);

    glBindTexture(GL_TEXTURE_1D, colourMapTexture);

    glTexSubImage1D(GL_TEXTURE_1D,
                    0,
                    0,
                    COLOUR_MAP_NUM_COLOURS,
                    GL_RGB,
                    GL_FLOAT,
                    cmap);
}

void OscilloscopeComponent::newOpenGLContextCreated()
{
    createShaders();

    using namespace juce::gl;

    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);

    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    // waveform texture: 2 rows, RG32F (min/max)
    glGenTextures(1, &dataTexture);
    glBindTexture(GL_TEXTURE_2D, dataTexture);

    // NOTE: NEAREST gives stable rendering for min/max bars
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    float initData[2][OSC_MAX_WIDTH * 2];
    for (int c = 0; c < OSC_MAX_WIDTH; ++c)
    {
        initData[0][2*c + 0] = oscMin[0][c];
        initData[0][2*c + 1] = oscMax[0][c];
        initData[1][2*c + 0] = oscMin[1][c];
        initData[1][2*c + 1] = oscMax[1][c];
    }

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RG32F,
                 OSC_MAX_WIDTH, 2,
                 0, GL_RG, GL_FLOAT, initData);

    // colourmap texture 1D
    glGenTextures(1, &colourMapTexture);
    glBindTexture(GL_TEXTURE_1D, colourMapTexture);

    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);

    int code = (int) apvts_ref.getRawParameterValue("gb_clrmap")->load();
    const float* cmap = getColourMapForCode(code);

    glTexImage1D(GL_TEXTURE_1D, 0, GL_RGB32F,
                 COLOUR_MAP_NUM_COLOURS, 0,
                 GL_RGB, GL_FLOAT, cmap);

    colourmap_dirty = false;
    trigger_repaint = true;
}

void OscilloscopeComponent::renderOpenGL()
{
    if (!shader)
        return;

    using namespace juce::gl;

    OpenGLHelpers::clear(Colours::black);

    int fft_ord = (int) apvts_ref.getRawParameterValue("gb_fft_ord")->load();

    const float renderingScale = (float) opengl_context.getRenderingScale();
    glViewport(0, 0,
               roundToInt(renderingScale * getWidth()),
               roundToInt(renderingScale * getHeight()));

    glBindVertexArray(VAO);

    shader->use();

    shader_uniforms->resolution->set(renderingScale * getWidth(),
                                     renderingScale * getHeight());

    shader_uniforms->imageData->set(0);
    shader_uniforms->colourMapTex->set(1);

    shader_uniforms->startIndex->set(writeIndex.load()); // offset for alignment
    shader_uniforms->numIndex->set(OSC_MAX_WIDTH);
    shader_uniforms->validColumns->set(validColumnsInData.load());

    int scroll_mode = (apvts_ref.getRawParameterValue("gb_vw_mde")->load() > 0.5f) ? 0 : 1;
    shader_uniforms->scroll->set(scroll_mode);

    int mode = (int) apvts_ref.getRawParameterValue("gb_chnl")->load();
    shader_uniforms->mode->set(mode);

    float th = 2.5f * renderingScale;
    th = jlimit(1.0f, 6.0f, th);
    shader_uniforms->thickness->set(th);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, dataTexture);

    if (new_data_flag)
    {
        static float uploadData[2][OSC_MAX_WIDTH * 2];

        for (int c = 0; c < OSC_MAX_WIDTH; ++c)
        {
            uploadData[0][2*c + 0] = oscMin[0][c];
            uploadData[0][2*c + 1] = oscMax[0][c];
            uploadData[1][2*c + 0] = oscMin[1][c];
            uploadData[1][2*c + 1] = oscMax[1][c];
        }

        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0,
                        OSC_MAX_WIDTH, 2,
                        GL_RG, GL_FLOAT, uploadData);

        new_data_flag = false;
    }

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_1D, colourMapTexture);

    if (colourmap_dirty)
    {
        uploadColourMap();
        colourmap_dirty = false;
    }

    GLfloat verts[] = { 1,1,  1,-1,  -1,-1,  -1,1 };
    GLuint inds[]   = { 0,1,3,  1,2,3 };

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STREAM_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, nullptr);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(inds), inds, GL_STREAM_DRAW);

    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);

    glDisableVertexAttribArray(0);
}

void OscilloscopeComponent::openGLContextClosing()
{
    using namespace juce::gl;

    trigger_repaint = false;

    if (VAO) glDeleteVertexArrays(1, &VAO);
    if (VBO) glDeleteBuffers(1, &VBO);
    if (EBO) glDeleteBuffers(1, &EBO);

    VAO = 0;
    VBO = 0;
    EBO = 0;

    if (dataTexture) glDeleteTextures(1, &dataTexture);
    if (colourMapTexture) glDeleteTextures(1, &colourMapTexture);

    dataTexture = 0;
    colourMapTexture = 0;

    shader.reset();
    shader_uniforms.reset();
}

void OscilloscopeComponent::resized()
{
    if (trigger_repaint)
        opengl_context.triggerRepaint();
}

void OscilloscopeComponent::createShaders()
{
    std::unique_ptr<OpenGLShaderProgram> attempt = std::make_unique<OpenGLShaderProgram>(opengl_context);

    if (attempt->addVertexShader(OpenGLHelpers::translateVertexShaderToV3(vertexShader))
        && attempt->addFragmentShader(OpenGLHelpers::translateFragmentShaderToV3(fragmentShader))
        && attempt->link())
    {
        shader = std::move(attempt);
        shader_uniforms.reset(new Uniforms(opengl_context, *shader));
    }
    else
    {
        DBG("Oscilloscope shader error: " + attempt->getLastError());
    }
}

OscilloscopeComponent::Uniforms::Uniforms(OpenGLContext& OpenGL_Context, OpenGLShaderProgram& shader_program)
{
    resolution.reset(createUniform(OpenGL_Context, shader_program, "resolution"));
    imageData.reset(createUniform(OpenGL_Context, shader_program, "imageData"));
    colourMapTex.reset(createUniform(OpenGL_Context, shader_program, "colourMapTex"));
    startIndex.reset(createUniform(OpenGL_Context, shader_program, "startIndex"));
    numIndex.reset(createUniform(OpenGL_Context, shader_program, "numIndex"));
    validColumns.reset(createUniform(OpenGL_Context, shader_program, "validColumns"));
    scroll.reset(createUniform(OpenGL_Context, shader_program, "scroll"));
    mode.reset(createUniform(OpenGL_Context, shader_program, "mode"));
    thickness.reset(createUniform(OpenGL_Context, shader_program, "thickness"));
}

OpenGLShaderProgram::Uniform* OscilloscopeComponent::Uniforms::createUniform(
    OpenGLContext& gl_context,
    OpenGLShaderProgram& shader_program,
    const char* uniform_name)
{
    if (gl_context.extensions.glGetUniformLocation(shader_program.getProgramID(), uniform_name) < 0)
        return nullptr;

    return new OpenGLShaderProgram::Uniform(shader_program, uniform_name);
}
