#include "Analyser.h"

SpectrumAnalyserComponent::SpectrumAnalyserComponent(
    AudioProcessorValueTreeState& apvts_reference,
    std::function<void(string)>& label_callback)
    : apvts_ref(apvts_reference)
{
    setOpaque(true);

    for (int i = 0; i < AMPLITUDE_DATA_SIZE; ++i) {
        amplitude_data[i] = 0.0f;
        ribbon_data[i] = 0.0f;
    }

    opengl_context.setOpenGLVersionRequired(OpenGLContext::OpenGLVersion::openGL3_2);
    opengl_context.setRenderer(this);
    opengl_context.setComponentPaintingEnabled(false);
    opengl_context.setContinuousRepainting(false);
    opengl_context.attachTo(*this);

    startTimer(1000.0f / (float)REFRESH_RATE);
    
    bool vSync_success = opengl_context.setSwapInterval(1);
    if (!vSync_success) DBG("V SYNC NOT SUPPORTED");
    else DBG("V SYNC ENABLED");
}

SpectrumAnalyserComponent::~SpectrumAnalyserComponent()
{
}

void SpectrumAnalyserComponent::prepareToPlay(float SampleRate, float BlockSize)
{
    SR = SampleRate;
}

void SpectrumAnalyserComponent::clearData()
{
    for (int i = 0; i < AMPLITUDE_DATA_SIZE; ++i) {
        amplitude_data[i] = 0.0f;
        ribbon_data[i] = 0.0f;
    }
}

void SpectrumAnalyserComponent::timerCallback()
{
    if (send_triggerRepaint) opengl_context.triggerRepaint();
}

void SpectrumAnalyserComponent::newOpenGLContextCreated()
{
    using namespace juce::gl;
    createShaders();

    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);
    
    glGenTextures(1, &dataTexture);
    glBindTexture(GL_TEXTURE_1D, dataTexture);
    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexImage1D(GL_TEXTURE_1D, 0, GL_R32F, AMPLITUDE_DATA_SIZE, 0, GL_RED, GL_FLOAT, amplitude_data);

    glGenTextures(1, &ribbonTexture);
    glBindTexture(GL_TEXTURE_1D, ribbonTexture);
    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexImage1D(GL_TEXTURE_1D, 0, GL_R32F, AMPLITUDE_DATA_SIZE, 0, GL_RED, GL_FLOAT, ribbon_data);

    send_triggerRepaint = true;
}

void SpectrumAnalyserComponent::renderOpenGL()
{
    jassert(OpenGLHelpers::isContextActive());

    if (pause) return;

    int pres_fft_size = po2[(int)apvts_ref.getRawParameterValue("gb_fft_ord")->load()];

    using namespace ::juce::gl;
    const float renderingScale = (float)opengl_context.getRenderingScale();
    glViewport(0, 0, roundToInt(renderingScale * getWidth()), roundToInt(renderingScale * getHeight()));

    if (!shader) return;
    shader->use();

    if (shader_uniforms && shader_uniforms->amplitudeData) shader_uniforms->amplitudeData->set(0);
    if (shader_uniforms && shader_uniforms->ribbonData)    shader_uniforms->ribbonData->set(1);    

    if (shader_uniforms) {
        if (shader_uniforms->resolution)
            shader_uniforms->resolution->set((GLfloat)(renderingScale * getWidth()), (GLfloat)(renderingScale * getHeight()));
        if (shader_uniforms->numBars)
            shader_uniforms->numBars->set((GLint)apvts_ref.getRawParameterValue("sp_num_brs")->load());
        if (shader_uniforms->numBins)
            shader_uniforms->numBins->set((GLint)bins_number.load());

        if (shader_uniforms->sampleRate)
            shader_uniforms->sampleRate->set((GLfloat)SR.load());
        if (shader_uniforms->minFreq && shader_uniforms->maxFreq) {
            float min_freq = (float)apvts_ref.getRawParameterValue("sp_rng_min")->load();
            float max_freq = max((float)apvts_ref.getRawParameterValue("sp_rng_max")->load(), min_freq + 100.0f);
            shader_uniforms->minFreq->set((GLfloat)min_freq);
            shader_uniforms->maxFreq->set((GLfloat)max_freq);
        }

        // if (shader_uniforms->colourmapBias)
        //     shader_uniforms->colourmapBias->set((GLfloat)apvts_ref.getRawParameterValue("sg_cm_bias")->load());

        const float* clr_data = getAccentColoursForCode((int)(apvts_ref.getRawParameterValue("gb_clrmap")->load()));
        if (shader_uniforms->colorMap_lower)
            shader_uniforms->colorMap_lower->set(clr_data[0], clr_data[1], clr_data[2]);
        if (shader_uniforms->colorMap_higher)
            shader_uniforms->colorMap_higher->set(clr_data[3], clr_data[4], clr_data[5]);
    }
    
    // Update amplitude texture
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_1D, dataTexture);
    if (newDataAvailable) {
        glTexSubImage1D(GL_TEXTURE_1D, 0, 0, AMPLITUDE_DATA_SIZE, GL_RED, GL_FLOAT, amplitude_data);
    }

    // Update ribbon texture
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_1D, ribbonTexture);
    if (newDataAvailable) {
        glTexSubImage1D(GL_TEXTURE_1D, 0, 0, AMPLITUDE_DATA_SIZE, GL_RED, GL_FLOAT, ribbon_data);
        newDataAvailable = false;
    }

    // Draw full viewport quad
    GLfloat vertices[] = { 1.0f, 1.0f, 1.0f, -1.0f, -1.0f, -1.0f, -1.0f, 1.0f };
    GLuint indices[] = { 0, 1, 3, 1, 2, 3 };

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STREAM_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STREAM_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(GLfloat), (GLvoid*)0);
    glEnableVertexAttribArray(0);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    glDisableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

void SpectrumAnalyserComponent::openGLContextClosing()
{
    send_triggerRepaint = false;

    if (VBO != 0) {
        opengl_context.extensions.glDeleteBuffers(1, &VBO);
        VBO = 0;
    }
    if (EBO != 0) {
        opengl_context.extensions.glDeleteBuffers(1, &EBO);
        EBO = 0;
    }

    shader.reset();
    shader_uniforms.reset();
}

void SpectrumAnalyserComponent::newData(float* data, int num_bins)
{
    int bars = (int)apvts_ref.getRawParameterValue("sp_num_brs")->load();
    float onion_speed = apvts_ref.getRawParameterValue("sp_bar_spd")->load() * 0.001f;

    // Exponential smoothing for the onion/background layer
    float alpha_onion = 1.0f - expf(-1.0f / (onion_speed * SR));
    float rem_alpha = 1.0f - alpha_onion;

    // Update amplitude data and ribbon (onion) data
    for (int i = 0; i < num_bins; ++i) {
        amplitude_data[i] = data[i];

        // Onion maintains peak, but decays when new data is lower
        if (data[i] > ribbon_data[i]) {
            ribbon_data[i] = data[i];
        } else {
            ribbon_data[i] = data[i] * alpha_onion + ribbon_data[i] * rem_alpha;
        }
    }

    newDataAvailable = true;
    bins_number = num_bins;

    // cout << "Analyser received new data batch with " + String(num_bins) + " bins." << "\n";

}

void SpectrumAnalyserComponent::createShaders()
{
    std::unique_ptr<OpenGLShaderProgram> shaderProgramAttempt = std::make_unique<OpenGLShaderProgram>(opengl_context);

    if (shaderProgramAttempt->addVertexShader(OpenGLHelpers::translateVertexShaderToV3(vertexShader))
        && shaderProgramAttempt->addFragmentShader(OpenGLHelpers::translateFragmentShaderToV3(fragmentShader))
        && shaderProgramAttempt->link())
    {
        shader_uniforms.release();
        shader = std::move(shaderProgramAttempt);
        shader_uniforms.reset(new Uniforms(opengl_context, *shader));
        DBG("GLSL: v" + String(OpenGLShaderProgram::getLanguageVersion(), 2));
    }
    else
    {
        DBG(shaderProgramAttempt->getLastError());
    }
}

SpectrumAnalyserComponent::Uniforms::Uniforms(OpenGLContext& OpenGL_Context, OpenGLShaderProgram& shader_program)
{
    resolution.reset(createUniform(OpenGL_Context, shader_program, "resolution"));
    amplitudeData.reset(createUniform(OpenGL_Context, shader_program, "amplitudeData"));
    ribbonData.reset(createUniform(OpenGL_Context, shader_program, "ribbonData"));
    colorMap_lower.reset(createUniform(OpenGL_Context, shader_program, "colorMap_lower"));
    colorMap_higher.reset(createUniform(OpenGL_Context, shader_program, "colorMap_higher"));
    numBars.reset(createUniform(OpenGL_Context, shader_program, "numBars"));
    numBins.reset(createUniform(OpenGL_Context, shader_program, "numBins"));
    sampleRate.reset(createUniform(OpenGL_Context, shader_program, "sampleRate"));
    minFreq.reset(createUniform(OpenGL_Context, shader_program, "minFreq"));
    maxFreq.reset(createUniform(OpenGL_Context, shader_program, "maxFreq"));
}

OpenGLShaderProgram::Uniform* SpectrumAnalyserComponent::Uniforms::createUniform(
    OpenGLContext& gl_context,
    OpenGLShaderProgram& shader_program,
    const char* uniform_name)
{
    if (gl_context.extensions.glGetUniformLocation(shader_program.getProgramID(), uniform_name) < 0)
        return nullptr;
    return new OpenGLShaderProgram::Uniform(shader_program, uniform_name);
}