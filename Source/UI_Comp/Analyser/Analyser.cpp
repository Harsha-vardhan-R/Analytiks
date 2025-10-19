#include "Analyser.h"

SpectrumAnalyserComponent::SpectrumAnalyserComponent(
    AudioProcessorValueTreeState& apvts_reference,
    linkDS& lnk_reference)
    :linker_ref(lnk_reference),
    apvts_ref(apvts_reference)
{
    setOpaque(true);

    opengl_context.setOpenGLVersionRequired(OpenGLContext::OpenGLVersion::openGL3_2);
    opengl_context.setRenderer(this);

    // no overlay for this.
    opengl_context.setComponentPaintingEnabled(false);

    opengl_context.attachTo(*this);
    bool vSync_success = opengl_context.setSwapInterval(1);

    if (!vSync_success) DBG("V SYNC NOT SUPPORTED");
    else DBG("V SYNC ENABLED");
}

SpectrumAnalyserComponent::~SpectrumAnalyserComponent()
{
}

void SpectrumAnalyserComponent::prepareToPlay
(float SampleRate, float BlockSize)
{
    SR = SampleRate;
}

void SpectrumAnalyserComponent::setState(bool s) {
    pause = s;
}

void SpectrumAnalyserComponent::clearData()
{
    for (int index = 0; index < AMPLITUDE_DATA_SIZE; ++index)
    {
        amplitude_data[index] = 0.0;
        ribbon_data[index] = 0.0;
    }
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
    glTexImage1D(
        GL_TEXTURE_1D,
        0,
        GL_R32F, // only one value
        AMPLITUDE_DATA_SIZE,
        0,
        GL_RED,
        GL_FLOAT,
        amplitude_data);

    glGenTextures(1, &ribbonTexture);
    glBindTexture(GL_TEXTURE_1D, dataTexture);
    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexImage1D(
        GL_TEXTURE_1D,
        0,
        GL_R32F,
        AMPLITUDE_DATA_SIZE,
        0,
        GL_RED,
        GL_FLOAT,
        ribbon_data);

    send_triggerRepaint = true;
    opengl_context.triggerRepaint();
}



void SpectrumAnalyserComponent::renderOpenGL()
{
    jassert(OpenGLHelpers::isContextActive());

    if (pause) return;

    bool newDataAvailable = linker_ref.isLatestDataPresent();

    int pres_fft_size = po2[(int)apvts_ref.getRawParameterValue("gb_fft_ord")->load()];

    using namespace ::juce::gl;
    // Setup viewport
    const float renderingScale = (float)opengl_context.getRenderingScale();
    glViewport(0, 0, roundToInt(renderingScale * getWidth()), roundToInt(renderingScale * getHeight()));

    //// Enable alpha blending
    //glEnable(GL_BLEND);
    //glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Make sure there is a shader.
    if (!shader) return;

    shader->use();

    if (shader_uniforms)
    {
        shader_uniforms->resolution->
            set((GLfloat)(renderingScale * getWidth()), (GLfloat)(renderingScale * getHeight()));

        // here the range min and the range max are the indexes in the amplitude and ribbon data.
        // TODO 
        shader_uniforms->rangeMin->set((GLint)apvts_ref.getRawParameterValue("sp_rng_min")->load());
        shader_uniforms->rangeMax->set((GLint)apvts_ref.getRawParameterValue("sp_rng_max")->load());

        shader_uniforms->numBars->set((GLint)apvts_ref.getRawParameterValue("sp_num_brs")->load());
        shader_uniforms->colourmapBias->set((GLfloat)apvts_ref.getRawParameterValue("sg_cm_bias")->load());

        if ((GLboolean)apvts_ref.getRawParameterValue("sp_high")->load()) {
            // load the colour map accent colours.
            const float* clr_data = getAccentColoursForCode((int)apvts_ref.getRawParameterValue("gb_clrmap")->load());
            shader_uniforms->colorMap_lower->set(clr_data, 3);
            shader_uniforms->colorMap_higher->set(clr_data + 3, 3);
        }
        else {
            float clr_data[6] = { 1.0, 1.0, 1.0, 1.0, 1.0, 1.0 };
            shader_uniforms->colorMap_lower->set(clr_data, 3);
            shader_uniforms->colorMap_higher->set(clr_data + 3, 3);
        }
    }
    
    if (newDataAvailable) {
        float time1 = (GLfloat)apvts_ref.getRawParameterValue("sp_bar_spd")->load() / 1000.0;
        float alpha1 =
            1.0 - expf(-1.0 / (time1 * SR));
        
        float time2 = (GLfloat)apvts_ref.getRawParameterValue("sp_pek_hld")->load() / 1000.0;
        float alpha2 =
            1.0 - expf(-1.0 / (time2 * SR));

        linker_ref.fillLatestData(
            amplitude_data,
            amplitude_data,
            alpha1,
            pres_fft_size);

        float rem_frac = 1.0 - alpha2;
        // based on the present aplitude values and the 
        // old ribbon values, new ribbon values are calculated.
        for (int sample = 0; sample < pres_fft_size; ++sample) {
            // if value grows instantaneously set it there.
            if (ribbon_data[sample] < amplitude_data[sample])
                ribbon_data[sample] = amplitude_data[sample];
            else
                ribbon_data[sample] =   amplitude_data[sample] * alpha2 + 
                                        ribbon_data[sample] * rem_frac;
        }

    }

    // update the ribbon values.
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_1D, dataTexture);
    if (newDataAvailable)
    {
        glTexSubImage1D(
            GL_TEXTURE_1D,
            0,
            0,
            AMPLITUDE_DATA_SIZE,
            GL_RED,
            GL_FLOAT,
            amplitude_data);
    }

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_1D, ribbonTexture);
    if (newDataAvailable)
    {
        glTexSubImage1D(
            GL_TEXTURE_1D,
            0,
            0,
            AMPLITUDE_DATA_SIZE,
            GL_RED,
            GL_FLOAT,
            ribbon_data);
    }

    // Define vertices and indices for a square (full viewport).
    GLfloat vertices[] = {
        1.0f,  1.0f,
        1.0f, -1.0f,
       -1.0f, -1.0f,
       -1.0f,  1.0f,
    };
    GLuint indices[] = {
        0, 1, 3,
        1, 2, 3
    };

    // Bind and fill VBO
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STREAM_DRAW);

    // Bind and fill EBO
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STREAM_DRAW);

    // Setup vertex attribute pointer (location 0, 3 floats per vertex)
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(GLfloat), (GLvoid*)0);
    glEnableVertexAttribArray(0);

    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

    // Cleanup: unbind buffers and disable attribute
    glDisableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

void SpectrumAnalyserComponent::openGLContextClosing()
{
    send_triggerRepaint = false;
    opengl_context.setContinuousRepainting(false);

    // Delete OpenGL buffers
    if (VBO != 0)
    {
        opengl_context.extensions.glDeleteBuffers(1, &VBO);
        VBO = 0;
    }
    if (EBO != 0)
    {
        opengl_context.extensions.glDeleteBuffers(1, &EBO);
        EBO = 0;
    }

    // Delete shader program and uniforms
    shader.reset();
    shader_uniforms.reset();
}

void SpectrumAnalyserComponent::createShaders()
{
    std::unique_ptr<OpenGLShaderProgram> shaderProgramAttempt = std::make_unique<OpenGLShaderProgram>(opengl_context);

    // Sets up pipeline of shaders and compiles the program
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

SpectrumAnalyserComponent::Uniforms::
Uniforms(OpenGLContext& OpenGL_Context, OpenGLShaderProgram& shader_program)
{
    resolution.reset(createUniform(OpenGL_Context, shader_program, "resolution"));
    amplitudeData.reset(createUniform(OpenGL_Context, shader_program, "amplitudeData"));
    ribbonData.reset(createUniform(OpenGL_Context, shader_program, "ribbonData"));
    colorMap_lower.reset(createUniform(OpenGL_Context, shader_program, "colorMap_lower"));
    colorMap_higher.reset(createUniform(OpenGL_Context, shader_program, "colorMap_higher"));
    rangeMin.reset(createUniform(OpenGL_Context, shader_program, "rangeMin"));
    rangeMax.reset(createUniform(OpenGL_Context, shader_program, "rangeMax"));
    numBars.reset(createUniform(OpenGL_Context, shader_program, "numBars"));
    colourmapGate.reset(createUniform(OpenGL_Context, shader_program, "colourmapGate"));
    colourmapBias.reset(createUniform(OpenGL_Context, shader_program, "colourmapBias"));
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